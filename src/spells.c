/* spells.c - spells, mana, timed unit effects, runes, demolition, computer casters (see spells.h).
 *
 * Spec: the Mac 68k binary (addresses as in docs/RE_NOTES.md). Spell = order 38..56; per order the binary
 * has a mana cost (A5-739C), a cast range (A5+1C60, Chebyshev tiles caster tile -> target tile, 0xFF =
 * unlimited), a "position spell" flag (A5+1AEE) and an attack-event hook (A5+1D94) that does the effect.
 * The caster walks into range, plays its ATTACK animation and the hook runs on the hit step.
 *
 * Cost: one pass per cycle over the small list of units with an effect; mana regeneration and the caster
 * AI visit 1/40 resp. 1/50 of the slots per cycle (staggered); no full scans per cycle. Plain C, no floats. */
#include <string.h>
#include "w2.h"
#include "unit.h"
#include "move.h"
#include "combat.h"
#include "economy.h"
#include "spells.h"
#include "w2cfg.h"

#define NSP W2_NUM_SPELLS
#define MANA_START 85u           /* CreateUnit / type change 0x0016D3A8: casters (flag 0x20000) get 85 */
#define MANA_PERIOD 40u          /* UnitTimersTick 0x0016BBFA: +1 every 40 cycles, up to 255 */
#define AI_PERIOD 50u            /* caster AI 0x0013D722 runs in the 50-cycle unit AI pass */
#define HV_CYCLES 100u           /* holy vision: how long the stamped area stays visible (UNKNOWN, see notes) */
#define NRUNES 50                /* rune slots A5+716E/A5+71A0/A5+71D2 */

uint32_t spells_ai_casts;
uint32_t spells_casts[19];

/* ---- the original's per-spell numbers, in our spell id order ------------------------------------ */
/* id:            HV  heal exo fire slow fsh inv poly bliz eye blood rune coil haste raise whirl unh dnd demo */
static const uint8_t k_cost[NSP]  = { 70, 6, 4, 100, 50, 80, 200, 200, 25, 70, 50, 200, 100, 50, 50, 100, 100, 25, 0 };
static const uint8_t k_range[NSP] = { 255, 6, 10, 10, 10, 6, 6, 10, 12, 255, 6, 10, 10, 6, 6, 12, 6, 12, 1 };
/* target rule: TK_POS position (A5+1AEE = 1), unit spells with the action's pre-check: f12_04F8 = not a
 * building, f12_0542 = organic; TK_SELF = no target (eye of kilrogg appears at the caster) */
enum { TK_POS, TK_NOBLD, TK_ORGANIC, TK_SELF };
static const uint8_t k_tk[NSP] = { TK_POS, TK_ORGANIC, TK_POS, TK_POS, TK_NOBLD, TK_NOBLD, TK_NOBLD, TK_ORGANIC,
	TK_POS, TK_SELF, TK_ORGANIC, TK_POS, TK_POS, TK_NOBLD, TK_POS, TK_POS, TK_NOBLD, TK_POS, TK_SELF };

int spells_mana_cost(int sp) { return sp >= 0 && sp < NSP ? k_cost[sp] : 0; }
int spells_range(int sp) { return sp >= 0 && sp < NSP ? k_range[sp] : 0; }
int spells_targets_unit(int sp) { return sp >= 0 && sp < NSP && (k_tk[sp] == TK_NOBLD || k_tk[sp] == TK_ORGANIC); }

/* ---- per-slot effect state (valid while s_gen == unit_gen) --------------------------------------- */
#define FX_INV    0x01
#define FX_UNHOLY 0x02
#define FX_BLOOD  0x04
#define FX_HASTE  0x08     /* haste or slow */
#define FX_SHIELD 0x10
#define FX_DECAY  0x20     /* summoned / decaying unit (UDTA decay rate) */
static uint8_t  s_fx[W2_MAX_UNITS];
static uint16_t s_gen[W2_MAX_UNITS];
static uint16_t s_inv[W2_MAX_UNITS], s_unholy[W2_MAX_UNITS], s_blood[W2_MAX_UNITS], s_shield[W2_MAX_UNITS];
static int16_t  s_haste[W2_MAX_UNITS];
static uint8_t  s_dec[W2_MAX_UNITS], s_life[W2_MAX_UNITS], s_dcnt[W2_MAX_UNITS], s_lstep[W2_MAX_UNITS], s_lkind[W2_MAX_UNITS];
static uint16_t s_list[W2_MAX_UNITS];
static uint8_t  s_inlist[W2_MAX_UNITS];
static int      s_n;
/* mana pass: the unit (generation + type) seen last time in this slot */
static uint16_t s_mgen[W2_MAX_UNITS];
static uint8_t  s_mtype[W2_MAX_UNITS], s_mvalid[W2_MAX_UNITS];

typedef struct { uint8_t x, y; uint16_t t; } Rune;
static Rune s_rune[NRUNES];
typedef struct { uint8_t x, y, p, pad; uint16_t t; } Vision;
#define NVIS 16
static Vision s_vis[NVIS];

void spells_reset(void)
{
	memset(s_fx, 0, sizeof s_fx); memset(s_inlist, 0, sizeof s_inlist); s_n = 0;
	memset(s_inv, 0, sizeof s_inv); memset(s_unholy, 0, sizeof s_unholy); memset(s_blood, 0, sizeof s_blood);
	memset(s_shield, 0, sizeof s_shield); memset(s_haste, 0, sizeof s_haste); memset(s_dec, 0, sizeof s_dec);
	memset(s_mvalid, 0, sizeof s_mvalid);
	memset(s_rune, 0, sizeof s_rune); memset(s_vis, 0, sizeof s_vis);
	spells_ai_casts = 0; memset(spells_casts, 0, sizeof spells_casts);
}

static int fx_ok(int s, int bit)
{
	return s >= 0 && s < W2_MAX_UNITS && (s_fx[s] & bit) && s_gen[s] == unit_gen(s);
}

int spells_invisible(int s)    { return fx_ok(s, FX_INV) ? s_inv[s] : 0; }
int spells_unholy(int s)       { return fx_ok(s, FX_UNHOLY) ? s_unholy[s] : 0; }
int spells_bloodlust(int s)    { return fx_ok(s, FX_BLOOD) ? s_blood[s] : 0; }
int spells_haste(int s)        { return fx_ok(s, FX_HASTE) ? s_haste[s] : 0; }
int spells_flame_shield(int s) { return fx_ok(s, FX_SHIELD) ? s_shield[s] : 0; }

int spells_hidden_from(int s, int player)
{
	return fx_ok(s, FX_INV) && w2.unit[s].owner != player;
}

int spells_status_icons(int s)
{
	int b = 0;
	if (s < 0 || s >= W2_MAX_UNITS || !(s_fx[s] & (FX_INV | FX_UNHOLY | FX_BLOOD | FX_HASTE)) || s_gen[s] != unit_gen(s)) return 0;
	if (s_blood[s]) b |= 1;
	if (s_haste[s] > 0) b |= 2;
	if (s_haste[s] < 0) b |= 4;
	if (s_inv[s]) b |= 8;
	if (s_unholy[s]) b |= 16;
	return b;
}

static void fx_bits(int s)
{
	uint8_t b = 0;
	if (s_inv[s]) b |= FX_INV;
	if (s_unholy[s]) b |= FX_UNHOLY;
	if (s_blood[s]) b |= FX_BLOOD;
	if (s_haste[s]) b |= FX_HASTE;
	if (s_shield[s]) b |= FX_SHIELD;
	if (s_dec[s]) b |= FX_DECAY;
	s_fx[s] = b;
}

static void fx_clear(int s)
{
	s_fx[s] = 0; s_inv[s] = s_unholy[s] = s_blood[s] = s_shield[s] = 0; s_haste[s] = 0;
	s_dec[s] = s_life[s] = s_dcnt[s] = 0; s_lstep[s] = s_lkind[s] = 0xFF;
}

/* make slot s current for writing an effect (clears a previous unit's leftovers) and list it */
static void fx_touch(int s)
{
	uint16_t g = unit_gen(s);
	if (s_gen[s] != g) { fx_clear(s); s_gen[s] = g; }
	if (!s_inlist[s] && s_n < W2_MAX_UNITS) { s_inlist[s] = 1; s_list[s_n++] = (uint16_t)s; }
}

void spells_break_invis(int s)
{
	if (fx_ok(s, FX_INV)) { s_inv[s] = 0; fx_bits(s); }
}

static void fx_decay_register(int s)
{
	const w2_unit_type *t = unit_type(s);
	if (!t || !t->decay) return;
	fx_touch(s);
	if (s_dec[s]) return;
	s_dec[s] = t->decay; s_life[s] = 255; s_dcnt[s] = t->decay;   /* lifetime 255 periods, then -5 HP each */
	fx_bits(s);
}

/* ---- small helpers -------------------------------------------------------------------------------- */
static int live(int s)
{
	const W2Unit *u;
	if (s < 0 || s >= W2_MAX_UNITS) return 0;
	u = &w2.unit[s];
	if (!(u->flags & W2_UF_ALIVE) || (u->flags & (W2_UF_DYING | W2_UF_HIDDEN | W2_UF_INSIDE))) return 0;
	return u->order != W2_ORD_DIE;
}

static uint32_t tflags(int s) { const w2_unit_type *t = unit_type(s); return t ? t->flags : 0; }
static int max_hp(int s) { const w2_unit_type *t = unit_type(s); return t ? t->hp : 0; }
static int in_map(int x, int y) { return x >= 0 && y >= 0 && x < w2.map.w && y < w2.map.h; }
static int friendly(int a, int b) { return !unit_is_enemy(a, b); }   /* diplomacy matrix entry 0 (self, allies, neutral) */

static void play_sound(int sp, int px, int py)
{
	int snd = (sp >= 0 && sp < NSP) ? w2_spells[sp].sound : W2_NONE;
	if (snd == W2_NONE || !in_map(px >> 5, py >> 5) || !fog_visible(px >> 5, py >> 5)) return;
	snd_play(snd, px, py);
}

static void effect_at_unit(int v, int type)
{
	int x, y;
	unit_center(v, &x, &y);
	missile_effect(type, x, y - 4);    /* SpawnEffectAtUnitBox 0x00129A2A: box centre - 4 px */
}

static void sound_at_unit(int sp, int v) { int x, y; unit_center(v, &x, &y); play_sound(sp, x, y); }

/* free tile near (tx,ty) for a new unit: air layer or walkable ground; rings up to r */
static int free_spot(int air, int tx, int ty, int r, int *ox, int *oy)
{
	int d, x, y;
	for (d = 0; d <= r; d++)
		for (y = ty - d; y <= ty + d; y++)
			for (x = tx - d; x <= tx + d; x++) {
				int i;
				if ((x != tx - d && x != tx + d && y != ty - d && y != ty + d) || !in_map(x, y)) continue;
				i = y * w2.map.w + x;
				if (air) { if (w2.map.occ_air[i] != W2_NO_UNIT) continue; }
				else if (w2.map.occ_ground[i] != W2_NO_UNIT || !move_tile_ok(MC_LAND, x, y)) continue;
				*ox = x; *oy = y;
				return 1;
			}
	return 0;
}

static int target_ok(int sp, int t)
{
	uint32_t f;
	if (!live(t)) return 0;
	f = tflags(t);
	if (k_tk[sp] == TK_ORGANIC) return (f & W2UF_ORGANIC) != 0;
	if (k_tk[sp] == TK_NOBLD) return !(f & W2UF_BUILDING) && !(w2.unit[t].flags & W2_UF_BUILDING);
	return 1;
}

/* FUN_00160596: pay the order's mana or the cast fails */
static int pay(int s, int sp)
{
	W2Unit *u = &w2.unit[s];
	if (u->mana < k_cost[sp]) return 0;
	u->mana = (uint8_t)(u->mana - k_cost[sp]);
	spells_casts[sp]++;
	return 1;
}

/* ---- holy vision ------------------------------------------------------------------------------------ */
/* sight-9 stamp (reveal routine 0x00144D40): 19x19 square, three cells per corner left out */
static void stamp(int cx, int cy, int p, int explore)
{
	int x, y, r = 9, bit = 1 << p;
	for (y = cy - r; y <= cy + r; y++) {
		int dy = y < cy ? cy - y : y - cy;
		if (y < 0 || y >= w2.map.h) continue;
		for (x = cx - r; x <= cx + r; x++) {
			int dx = x < cx ? cx - x : x - cx, i;
			if (x < 0 || x >= w2.map.w || dx + dy > 2 * r - 2) continue;
			i = y * w2.map.w + x;
			w2.map.visible[i] |= (uint8_t)bit;
			if (explore) w2.map.explored[i] |= (uint8_t)bit;
		}
	}
}

void spells_fog_stamp(void)
{
	int i;
	for (i = 0; i < NVIS; i++) if (s_vis[i].t) stamp(s_vis[i].x, s_vis[i].y, s_vis[i].p, 0);
}

static void vision_add(int x, int y, int p)
{
	int i, k = 0;
	if (p < 0 || p >= 8) return;
	stamp(x, y, p, 1);
	for (i = 0; i < NVIS; i++) {
		if (!s_vis[i].t) { k = i; break; }
		if (s_vis[i].t < s_vis[k].t) k = i;     /* full: replace the oldest */
	}
	s_vis[k].x = (uint8_t)x; s_vis[k].y = (uint8_t)y; s_vis[k].p = (uint8_t)p; s_vis[k].t = HV_CYCLES;
}

/* ---- runes (0x0016077C place, 0x00160656 tick) ------------------------------------------------------ */
int spells_rune_count(void) { int i, n = 0; for (i = 0; i < NRUNES; i++) if (s_rune[i].t) n++; return n; }
int spells_rune_at(int x, int y)
{
	int i;
	for (i = 0; i < NRUNES; i++) if (s_rune[i].t && s_rune[i].x == x && s_rune[i].y == y) return 1;
	return 0;
}

static int rune_place(int x, int y)
{
	int i;
	if (!in_map(x, y) || w2.map.occ_ground[y * w2.map.w + x] != W2_NO_UNIT || spells_rune_at(x, y)) return 0;
	for (i = 0; i < NRUNES && s_rune[i].t; i++) ;
	if (i >= NRUNES) return 0;
	s_rune[i].x = (uint8_t)x; s_rune[i].y = (uint8_t)y; s_rune[i].t = 0x800;   /* 2048 cycles */
	missile_effect(W2M_RUNE, x * W2_TILE + 16, y * W2_TILE + 16);
	return 1;
}

static void runes_tick(void)
{
	int i;
	for (i = 0; i < NRUNES; i++) {
		Rune *r = &s_rune[i];
		int v;
		if (!r->t) continue;
		r->t--;
		if ((r->t & 0xFF) == 1) missile_effect(W2M_RUNE, r->x * W2_TILE + 16, r->y * W2_TILE + 16);  /* glint every 256 */
		v = w2.map.occ_ground[r->y * w2.map.w + r->x];
		if (v == W2_NO_UNIT || !live(v) || (unit_type(v)->xflags & W2UX_INDESTRUCTIBLE)) continue;
		/* any ground unit (friend or foe) sets it off: 50 HP, unless unholy armored; no attacker credit */
		r->t = 0;
		missile_effect(W2M_EXPLOSION, r->x * W2_TILE + 16, r->y * W2_TILE + 16);
		if (!spells_unholy(v)) {
			if (w2.unit[v].hp < 51) unit_kill(v);
			else w2.unit[v].hp = (uint16_t)(w2.unit[v].hp - 50);
		}
	}
}

/* ---- the spell effects (the hooks A5+1D94[38..56]); return 1 = cast again (blizzard, death and decay) */
static void exorcise(int s, int v)          /* FUN_00160818 */
{
	W2Unit *u = &w2.unit[s];
	int n;
	if (v == W2_NO_UNIT || !live(v) || !(tflags(v) & W2UF_UNDEAD)) return;
	n = u->mana / k_cost[W2SP_EXORCISM];
	if (n > w2.unit[v].hp) n = w2.unit[v].hp;
	effect_at_unit(v, W2M_EXORCISM);
	sound_at_unit(W2SP_EXORCISM, v);
	if (n > 255) n = 255;
	unit_damage(v, n, s);
	u->mana = (uint8_t)(u->mana - n * k_cost[W2SP_EXORCISM]);
}

static int coil_victim(int s, int v)        /* FUN_00160912: other owner, alive, organic */
{
	return v != W2_NO_UNIT && live(v) && w2.unit[v].owner != w2.unit[s].owner && (tflags(v) & W2UF_ORGANIC);
}

static int cast_hook(int s, int sp, int tgt, int gx, int gy)
{
	W2Unit *u = &w2.unit[s];
	int i, x, y;
	switch (sp) {
	case W2SP_HOLY_VISION: {     /* 0x00160954: 7 sight-9 stamps around the point */
		static const int8_t ox[7] = { 0, 0, 0, 6, 6, -6, -6 }, oy[7] = { 0, -8, 8, 4, -4, 4, -4 };
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		for (i = 0; i < 7; i++) {
			x = gx + ox[i]; y = gy + oy[i];
			if (x < 0 || y < 0 || x > w2.map.w || y > w2.map.h) continue;
			vision_add(x, y, u->owner);
			missile_effect(W2M_NORMAL_SPELL, x * W2_TILE + 8, y * W2_TILE + 8);
			play_sound(sp, x * W2_TILE + 8, y * W2_TILE + 8);
		}
		return 0; }
	case W2SP_HEALING: {         /* 0x001609FA: min(mana/6, 40, missing) HP at 6 mana each */
		int miss, n;
		spells_break_invis(s);
		miss = max_hp(tgt) - w2.unit[tgt].hp;
		if (miss <= 0) return 0;
		n = u->mana / k_cost[sp];
		if (n > 40) n = 40;
		if (n > miss) n = miss;
		if (!n) return 0;
		w2.unit[tgt].hp = (uint16_t)(w2.unit[tgt].hp + n);
		u->mana = (uint8_t)(u->mana - n * k_cost[sp]);
		spells_casts[sp]++;
		effect_at_unit(tgt, W2M_HEAL_EFFECT);
		sound_at_unit(sp, tgt);
		return 0; }
	case W2SP_EXORCISM: {        /* 0x00160A9A: rings r = 0..3 around the point while mana >= 4 (ground units) */
		int r;
		spells_break_invis(s);
		spells_casts[sp]++;
		for (r = 0; r < 4 && u->mana >= k_cost[sp]; r++) {
			int k;
			for (k = -r; k <= r; k++) {
				x = gx + k;
				if (x < 0 || x >= w2.map.w) continue;
				if (in_map(x, gy + r)) exorcise(s, w2.map.occ_ground[(gy + r) * w2.map.w + x]);
				if (in_map(x, gy - r)) exorcise(s, w2.map.occ_ground[(gy - r) * w2.map.w + x]);   /* r = 0: centre twice (original) */
			}
			for (k = -r + 1; k < r; k++) {
				y = gy + k;
				if (y < 0 || y >= w2.map.h) continue;
				if (in_map(gx + r, y)) exorcise(s, w2.map.occ_ground[y * w2.map.w + gx + r]);
				if (in_map(gx - r, y)) exorcise(s, w2.map.occ_ground[y * w2.map.w + gx - r]);
			}
		}
		return 0; }
	case W2SP_FLAME_SHIELD:      /* 0x00160BB6: not on flyers, not while one is active; 500 cycles */
		spells_break_invis(s);
		if (spells_flame_shield(tgt) || (tflags(tgt) & W2UF_AIR) || (w2.unit[tgt].flags & W2_UF_AIR)) return 0;
		if (!pay(s, sp)) return 0;
		fx_touch(tgt); s_shield[tgt] = 500; fx_bits(tgt);
		missile_spell(MSP_SHIELD_GO, s, tgt, 0, 0, 0);
		sound_at_unit(sp, tgt);
		return 0;
	case W2SP_FIREBALL:          /* 0x00160C24: breath missile, rating 40 */
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		missile_spell(MSP_BREATH, s, -1, gx * W2_TILE + 16, gy * W2_TILE + 16, 40);
		sound_at_unit(sp, s);
		return 0;
	case W2SP_SLOW:              /* 0x00160C4E: slowed -> -1000, else -1000 added (cancels haste) */
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		fx_touch(tgt);
		s_haste[tgt] = (int16_t)(s_haste[tgt] < 0 ? -1000 : s_haste[tgt] - 1000);
		fx_bits(tgt);
		effect_at_unit(tgt, W2M_NORMAL_SPELL); sound_at_unit(sp, tgt);
		return 0;
	case W2SP_INVISIBILITY:      /* 0x00160CA2: 2000 cycles */
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		fx_touch(tgt); s_inv[tgt] = 2000; fx_bits(tgt);
		effect_at_unit(tgt, W2M_NORMAL_SPELL); sound_at_unit(sp, tgt);
		return 0;
	case W2SP_POLYMORPH: {       /* 0x00160CE6: the target vanishes (no corpse), a neutral critter on its tile */
		int tx = w2.unit[tgt].tx, ty = w2.unit[tgt].ty, nx, ny, c;
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		sound_at_unit(sp, tgt);
		unit_remove(tgt);
		if (free_spot(0, tx, ty, 2, &nx, &ny) && (c = unit_create(W2U_CRITTER, W2_NEUTRAL, nx, ny)) >= 0) (void)c;
		missile_effect(W2M_NORMAL_SPELL, tx * W2_TILE + 16, ty * W2_TILE + 16);
		return 0; }
	case W2SP_BLIZZARD:          /* 0x00160D62: 5 fields of 11 shards per cast step, repeats */
	case W2SP_DEATH_AND_DECAY:   /* 0x0016133A: 5 decay spots per cast step, repeats */
		if (!pay(s, sp)) return 0;
		play_sound(sp, gx * W2_TILE + 16, gy * W2_TILE + 16);
		for (i = 0; i < 5; i++) {
			if (sp == W2SP_BLIZZARD) {
				int r1 = (int)(w2_rand() % 5u), r2 = (int)(w2_rand() % 5u);
				missile_spell(MSP_BLIZZARD, s, -1, (r1 - 2) * W2_TILE + gx * W2_TILE + 16, (r2 - 2) * W2_TILE + gy * W2_TILE + 16, 10);
			} else missile_spell(MSP_DND, s, -1, gx * W2_TILE + 16, gy * W2_TILE + 16, 10);
		}
		spells_break_invis(s);
		return 1;
	case W2SP_EYE_OF_VISION: {   /* 0x00160DC2: unit 0x2D next to the caster */
		int nx, ny, e;
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		if (free_spot(1, u->tx, u->ty, 3, &nx, &ny) && (e = unit_create(W2U_EYE_OF_VISION, u->owner, nx, ny)) >= 0) {
			fx_decay_register(e);
			sound_at_unit(sp, e);
		}
		return 0; }
	case W2SP_BLOODLUST:         /* 0x00160E1C: 1000 cycles */
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		fx_touch(tgt); s_blood[tgt] = 1000; fx_bits(tgt);
		effect_at_unit(tgt, W2M_NORMAL_SPELL); sound_at_unit(sp, tgt);
		return 0;
	case W2SP_RAISE_DEAD:        /* 0x00160E5E: every dead body with dx^2+dy^2 < 37, 50 mana each */
		spells_break_invis(s);
		for (i = 0; i < W2_MAX_CORPSES && u->mana >= k_cost[sp]; i++) {
			W2Corpse *c = &w2_corpses[i];
			int dx, dy, k, ii;
			if (!c->used || (c->kind != W2_CORPSE_HUMAN_DEAD_BODY && c->kind != W2_CORPSE_ORC_DEAD_BODY)) continue;
			dx = gx - c->tx; dy = gy - c->ty;
			if (dx * dx + dy * dy >= 37) continue;
			u->mana = (uint8_t)(u->mana - k_cost[sp]);     /* paid even when the tile is taken (original) */
			spells_casts[sp]++;
			ii = c->ty * w2.map.w + c->tx;
			if (w2.map.occ_ground[ii] != W2_NO_UNIT || !move_tile_ok(MC_LAND, c->tx, c->ty)) continue;
			k = unit_create(W2U_SKELETON, u->owner, c->tx, c->ty);
			if (k < 0) continue;
			fx_decay_register(k);
			effect_at_unit(k, W2M_NORMAL_SPELL); sound_at_unit(sp, k);
			unit_corpse_remove(i);
		}
		return 0;
	case W2SP_DEATH_COIL: {      /* 0x00160F42: 5x5 tiles, weakest first, 50 HP budget, caster healed */
		uint16_t vs[50]; int hp[50], n = 0, j, sum = 0, taken = 0;
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		for (y = gy - 2; y <= gy + 2; y++)
			for (x = gx - 2; x <= gx + 2; x++) {
				int layer;
				if (!in_map(x, y)) continue;
				for (layer = 0; layer < 2; layer++) {
					int v = layer ? w2.map.occ_air[y * w2.map.w + x] : w2.map.occ_ground[y * w2.map.w + x];
					if (!coil_victim(s, v) || n >= 50) continue;
					for (j = 0; j < n && vs[j] != v; j++) ;
					if (j < n) continue;              /* multi-tile unit: once (the original lists it per tile) */
					vs[n] = (uint16_t)v; hp[n] = w2.unit[v].hp; n++;
				}
			}
		for (i = 0; i < n - 1; i++)                   /* the original's exchange sort, ascending HP */
			for (j = i + 1; j < n; j++)
				if (hp[j] < hp[i]) { int th = hp[i]; uint16_t tv = vs[i]; hp[i] = hp[j]; vs[i] = vs[j]; hp[j] = th; vs[j] = tv; }
		while (taken < n && hp[taken] + sum <= 50) { sum += hp[taken]; taken++; }
		if (taken < n && sum < 50 && hp[taken] + sum > 50) { hp[taken] = 50 - sum; sum = 50; taken++; }
		for (i = 0; i < taken; i++) {
			int sx, sy, tx, ty, mt = unit_type(s)->missile;
			if (mt >= W2M_NONE) mt = W2M_TOUCH_OF_DEATH;
			unit_center(s, &sx, &sy); unit_center(vs[i], &tx, &ty);
			tx += (int)(w2_rand() & 7u) - 3; ty += (int)(w2_rand() & 7u) - 3;   /* Missile_Spawn jitter */
			missile_fire_dmg(s, mt, sx, sy, tx, ty, vs[i], hp[i]);
			sound_at_unit(sp, vs[i]);
		}
		if ((int)u->hp + sum > max_hp(s)) u->hp = (uint16_t)max_hp(s);
		else u->hp = (uint16_t)(u->hp + sum);
		return 0; }
	case W2SP_WHIRLWIND:         /* 0x001611AE */
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		missile_spell(MSP_WHIRL, s, -1, gx * W2_TILE + 16, gy * W2_TILE + 16, 800);
		sound_at_unit(sp, s);
		return 0;
	case W2SP_HASTE:             /* 0x001611E4: slowed -> +1000 (cancels), else 1000 */
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		fx_touch(tgt);
		s_haste[tgt] = (int16_t)(s_haste[tgt] < 1 ? s_haste[tgt] + 1000 : 1000);
		fx_bits(tgt);
		effect_at_unit(tgt, W2M_NORMAL_SPELL); sound_at_unit(sp, tgt);
		return 0;
	case W2SP_UNHOLY_ARMOR:      /* 0x00161238: 500 cycles, HP halved */
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		fx_touch(tgt); s_unholy[tgt] = 500; fx_bits(tgt);
		if (w2.unit[tgt].hp > 1) w2.unit[tgt].hp = (uint16_t)(w2.unit[tgt].hp >> 1);
		effect_at_unit(tgt, W2M_NORMAL_SPELL); sound_at_unit(sp, tgt);
		return 0;
	case W2SP_RUNES: {           /* 0x00161290: 5 runes in a + shape, a fifth of the cost back per failed one */
		int ok;
		if (!pay(s, sp)) return 0;
		spells_break_invis(s);
		ok = rune_place(gx, gy) + rune_place(gx, gy - 1) + rune_place(gx + 1, gy) + rune_place(gx, gy + 1) + rune_place(gx - 1, gy);
		u->mana = (uint8_t)(u->mana + (k_cost[sp] / 5) * (5 - ok));
		play_sound(sp, gx * W2_TILE + 16, gy * W2_TILE + 16);
		return 0; }
	default:
		return 0;
	}
}

/* ---- the cast order --------------------------------------------------------------------------------- */
static void cast_end(int s)
{
	W2Unit *u = &w2.unit[s];
	if (u->anim_kind == W2_ANIM_ATTACK) unit_set_anim(s, W2_ANIM_STILL);
	unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
}

static int attack_step(int s)
{
	unsigned n, i;
	const w2_anim_step *a = w2_unit_anim(w2.unit[s].type, W2_ANIM_ATTACK, &n);
	for (i = 0; a && i < n; i++) if (a[i].flags & W2AF_ATTACK) return (int)i;
	return 0;
}

/* the cast's target: unit spells -> *tgt and its tile; position -> goal tile; 0 = target lost */
static int cast_target(int s, int sp, int *tgt, int *gx, int *gy)
{
	W2Unit *u = &w2.unit[s];
	*tgt = -1;
	if (k_tk[sp] == TK_SELF) { *gx = u->tx; *gy = u->ty; return 1; }
	if (k_tk[sp] == TK_POS) { *gx = u->goal_tx; *gy = u->goal_ty; return 1; }
	if (!unit_target_same(s) || !target_ok(sp, u->target)) return 0;     /* FUN_00160382 + pre-checks */
	*tgt = u->target; *gx = w2.unit[*tgt].tx; *gy = w2.unit[*tgt].ty;
	return 1;
}

/* order_state: 0 walking into range, 1 swing (cast on the hit step), 2 swing done -> again, 3 -> stop,
 * 4 walking into range after a repeated cast (keeps the looping ATTACK animation); u->timer counts moves
 * that "arrived" out of range */
void ord_cast(int s)
{
	W2Unit *u = &w2.unit[s];
	const w2_unit_type *t = unit_type(s);
	int sp = u->queue_type, tgt, gx, gy, rng;
	if (!t || sp >= W2SP_SUICIDE_BOMBER || !(t->flags & W2UF_SPELLCASTER)) { cast_end(s); return; }
	if (u->order_state >= 1 && u->order_state <= 3) {
		if (u->order_state == 1) {
			int ev = unit_anim_event(s);
			if ((ev & W2AF_ATTACK) || (u->anim_kind == W2_ANIM_ATTACK && u->anim_step >= attack_step(s) && attack_step(s) > 0)) {
				int again = 0;
				uint16_t g = unit_gen(s);
				if (cast_target(s, sp, &tgt, &gx, &gy)) again = cast_hook(s, sp, tgt, gx, gy);
				if (!unit_same(s, g) || u->order != W2_ORD_CAST) return;   /* polymorphed itself / killed */
				u->order_state = (uint8_t)(again ? 2 : 3);
			}
		}
		if (unit_anim_done(s) || u->anim_kind != W2_ANIM_ATTACK) {
			if (u->order_state == 3) { cast_end(s); return; }
			u->order_state = (uint8_t)(u->order_state == 2 ? 4 : 0);
		}
		return;
	}
	if (!cast_target(s, sp, &tgt, &gx, &gy)) { cast_end(s); return; }
	rng = k_range[sp];
	if (rng != 255 && unit_dist_to_tile(s, gx, gy) > rng) {    /* f10_0B5E range test, else walk */
		int r = (tgt >= 0 && tgt != s) ? move_toward_unit(s, tgt, rng) : move_toward(s, gx, gy, rng);
		if (u->anim_kind == W2_ANIM_ATTACK) unit_set_anim(s, W2_ANIM_STILL);
		u->order_state = 0;
		if (r < 0 || (r == 1 && ++u->timer > 3)) cast_end(s);
		return;
	}
	if (u->ox || u->oy) { move_stop(s); return; }              /* finish the tile step first */
	u->timer = 0;
	if (tgt >= 0 && tgt != s) { int cx, cy; unit_center(tgt, &cx, &cy); unit_set_dir_to(s, cx, cy); }
	else if (k_tk[sp] == TK_POS) unit_set_dir_to(s, gx * W2_TILE + 16, gy * W2_TILE + 16);
	if (u->anim_kind != W2_ANIM_ATTACK || u->order_state != 4) unit_set_anim(s, W2_ANIM_ATTACK);
	u->order_state = 1;
}

static void issue(int s, int sp, int target, int tx, int ty)
{
	if (target >= 0 && k_tk[sp] == TK_POS) { tx = w2.unit[target].tx; ty = w2.unit[target].ty; target = -1; }
	if (k_tk[sp] == TK_SELF) { target = -1; tx = w2.unit[s].tx; ty = w2.unit[s].ty; }
	if (k_tk[sp] != TK_POS && k_tk[sp] != TK_SELF && target < 0) return;
	unit_set_order(s, W2_ORD_CAST, target >= 0 ? target : W2_NO_UNIT, tx, ty);
	w2.unit[s].queue_type = (uint8_t)sp;
}

int cmd_cast(int s, int sp, int target, int tx, int ty)
{
	const w2_unit_type *t;
	if (!live(s) || sp < 0 || sp >= W2SP_SUICIDE_BOMBER) return 0;
	t = unit_type(s);
	if (!t || !(t->flags & W2UF_SPELLCASTER) || w2.unit[s].mana < k_cost[sp]) return 0;
	if (w2_spells[sp].upgrade != W2_NONE && !econ_has_upgrade(w2.unit[s].owner, w2_spells[sp].upgrade)) return 0;
	if (target >= 0 && (target >= W2_MAX_UNITS || !live(target))) target = -1;
	if (spells_targets_unit(sp) && (target < 0 || !target_ok(sp, target))) return 0;
	issue(s, sp, target, tx, ty);
	return 1;
}

/* ---- demolition (orders 20/21, f4_1468) ------------------------------------------------------------- */
static void demolish_hit(int s, int x, int y, uint16_t *hit, int k)
{
	int v, j;
	hit[k] = W2_NO_UNIT;
	if (!in_map(x, y)) return;
	v = w2.map.occ_ground[y * w2.map.w + x];
	if (v == W2_NO_UNIT || v == s) return;
	hit[k] = (uint16_t)v;
	for (j = 0; j < k; j++) if (hit[j] == v) return;
	{ int cx, cy; unit_center(v, &cx, &cy); missile_effect(W2M_EXPLOSION, cx, cy - 8); }
	unit_damage(v, 200, s);
	if (w2.map.occ_ground[y * w2.map.w + x] == v && live(v)) unit_damage(v, 200, s);   /* still there: again */
}

static void demolish(int s)
{
	static const int8_t ndx[8] = { 1, -1, 1, -1, 0, 0, 1, -1 }, ndy[8] = { 1, 1, -1, -1, 1, -1, 0, 0 };
	static const int8_t tdx[12] = { 1, 1, -1, -1, 0, 0, 1, -1, 2, -2, 0, 0 }, tdy[12] = { 1, -1, 1, -1, 1, -1, 0, 0, 0, 0, 2, -2 };
	uint16_t hit[8];
	int x = w2.unit[s].tx, y = w2.unit[s].ty, i;
	/* f13_1C3C: trees within the diamond of radius 2 are blown away (rocks and walls: see the report) */
	for (i = 0; i < 12; i++)
		if (in_map(x + tdx[i], y + tdy[i]) && (w2.map.flags[(y + tdy[i]) * w2.map.w + x + tdx[i]] & W2_TF_TREE))
			econ_clear_tree(x + tdx[i], y + tdy[i]);
	for (i = 0; i < 8; i++) demolish_hit(s, x + ndx[i], y + ndy[i], hit, i);
	unit_kill(s);
}

void ord_demolish(int s)
{
	W2Unit *u = &w2.unit[s];
	int tgt = -1, gx = u->goal_tx, gy = u->goal_ty, range, d, r;
	if (!(tflags(s) & W2UF_VOLATILE)) { cast_end(s); return; }
	if (u->target != W2_NO_UNIT) {
		if (!unit_target_same(s) || !live(u->target)) { cast_end(s); return; }
		tgt = u->target;
	}
	if (tgt >= 0) { range = 1; d = unit_dist_tiles(s, tgt); }
	else {
		/* order 21 walks onto the tile (range 0); a blocked tile (rock, wall, tree) is blown from next to it */
		range = (in_map(gx, gy) && move_tile_ok(MC_LAND, gx, gy)) ? 0 : 1;
		d = unit_dist_to_tile(s, gx, gy);
	}
	if (d > range) {
		r = tgt >= 0 ? move_toward_unit(s, tgt, range) : move_toward(s, gx, gy, range);
		if (r < 0 || (r == 1 && ++u->timer > 3)) cast_end(s);
		return;
	}
	if (u->ox || u->oy) { move_stop(s); return; }
	demolish(s);
}

int cmd_demolish(int s, int target, int tx, int ty)
{
	if (!live(s) || !(tflags(s) & W2UF_VOLATILE)) return 0;
	if (target >= 0 && (target >= W2_MAX_UNITS || !live(target) || target == s)) target = -1;
	unit_set_order(s, W2_ORD_DEMOLISH, target >= 0 ? target : W2_NO_UNIT,
	               target >= 0 ? w2.unit[target].tx : tx, target >= 0 ? w2.unit[target].ty : ty);
	return 1;
}

/* ---- computer casters (0x0013D722; target searches 0x0013C404 box, 0x0013C49A random, nearest) ------ */
typedef int (*Pred)(int c, int t);
enum { AI_BOX, AI_RANDOM, AI_NEAREST };

static int inv(int s) { return spells_invisible(s) != 0; }
static int bld(int t) { return (tflags(t) & W2UF_BUILDING) != 0; }
static int attacking(int t) { return w2.unit[t].order == W2_ORD_ATTACK; }

/* FUN_0013C4FA: a unit not hostile to 'a' within r tiles of t's tile (t itself excluded; ground map) */
static int friend_near(int a, int t, int r)
{
	int x, y, cx = w2.unit[t].tx, cy = w2.unit[t].ty;
	for (y = cy - r; y <= cy + r; y++)
		for (x = cx - r; x <= cx + r; x++) {
			int v;
			if (!in_map(x, y)) continue;
			v = w2.map.occ_ground[y * w2.map.w + x];
			if (v == W2_NO_UNIT || v == t || !live(v)) continue;
			if (friendly(a, v)) return 1;
		}
	return 0;
}

static int p_enemy_worker(int c, int t) { return unit_is_enemy(c, t) && (tflags(t) & W2UF_WORKER); }
static int p_holy(int c, int t) { return !inv(c) && unit_is_enemy(c, t); }
static int p_heal(int c, int t) { return !inv(c) && friendly(c, t) && (tflags(t) & W2UF_ORGANIC) && w2.unit[t].hp < max_hp(t); }
static int p_exorc(int c, int t) { return unit_is_enemy(c, t) && (tflags(t) & W2UF_UNDEAD); }
static int p_blood(int c, int t) { return !inv(c) && friendly(c, t) && (tflags(t) & W2UF_ORGANIC) && !spells_bloodlust(t) && attacking(t); }
static int p_poly(int c, int t) { return unit_is_enemy(c, t) && (tflags(t) & W2UF_CAN_ATTACK) && (tflags(t) & W2UF_ORGANIC); }
static int p_poly_caster(int c, int t) { return p_poly(c, t) && (tflags(t) & W2UF_SPELLCASTER); }
static int p_unholy(int c, int t) { return !inv(c) && friendly(c, t) && !spells_unholy(t) && attacking(t) && w2.unit[t].hp <= max_hp(t) / 2; }
static int p_fshield(int c, int t)
{
	const w2_unit_type *ty;
	if (inv(c) || spells_flame_shield(t) || friend_near(c, t, 2) || bld(t)) return 0;
	if (unit_is_enemy(c, t)) return friend_near(t, t, 1);          /* an enemy among its own: burns them */
	ty = unit_type(t);
	return attacking(t) && ty && ty->max_range == 1;
}
static int p_fireball(int c, int t) { return unit_is_enemy(c, t) && (!friend_near(c, t, 3) || w2.unit[c].hp < max_hp(c) / 2); }
static int p_haste(int c, int t) { return !inv(c) && friendly(c, t) && !spells_haste(t) && attacking(t); }
static int p_coil(int c, int t) { return !inv(c) && unit_is_enemy(c, t) && (tflags(t) & W2UF_ORGANIC); }
static int p_coil2(int c, int t) { return unit_is_enemy(c, t) && (tflags(t) & W2UF_ORGANIC) && friend_near(t, t, 1); }
static int p_whirl(int c, int t) { return unit_is_enemy(c, t) && !friend_near(c, t, 4); }
static int p_whirl_wh(int c, int t) { return p_whirl(c, t) && (tflags(t) & (W2UF_WORKER | W2UF_STORE_HALL)); }
static int p_bld(int c, int t) { return unit_is_enemy(c, t) && bld(t) && !friend_near(c, t, 3); }
static int p_wh(int c, int t) { return unit_is_enemy(c, t) && (tflags(t) & (W2UF_WORKER | W2UF_STORE_HALL)) && !friend_near(c, t, 3); }
static int p_slow(int c, int t) { return !inv(c) && unit_is_enemy(c, t) && !bld(t) && spells_haste(t) >= 0 && w2.unit[t].order != W2_ORD_STILL; }
static int p_invis(int c, int t)
{
	const w2_unit_type *ty = unit_type(t);
	int o = w2.unit[c].order;
	/* original: the caster's AI role is "attacker" (+0x5E == 2); here: it is on an attack order */
	if (inv(c) || !friendly(c, t) || bld(t) || !ty) return 0;
	if (o != W2_ORD_ATTACK && o != W2_ORD_ATTACK_MOVE && o != W2_ORD_PATROL) return 0;
	return (ty->flags & W2UF_SPELLCASTER) || ty->max_range != 1;
}

static int find_box(int c, Pred p)
{
	int x, y, cx = w2.unit[c].tx, cy = w2.unit[c].ty;
	for (y = cy - 15; y <= cy + 15; y++) {
		const uint16_t *row;
		if (y < 0 || y >= w2.map.h) continue;
		row = &w2.map.occ_ground[y * w2.map.w];
		for (x = cx - 15; x <= cx + 15; x++) {
			int v;
			if (x < 0 || x >= w2.map.w) continue;
			v = row[x];
			if (v == W2_NO_UNIT || v == c || !live(v)) continue;
			if (p(c, v)) return v;
		}
	}
	return -1;
}

static int find_random(int c, Pred p)
{
	int i, hi = w2.unit_hi ? w2.unit_hi : 1;
	for (i = 0; i < 10; i++) {
		int v = (int)(w2_rand() % (uint32_t)hi);       /* the original: Random() % 600 slots */
		if (v != c && live(v) && p(c, v)) return v;
	}
	return -1;
}

static int find_nearest(int c, Pred p)
{
	int v, best = -1, bd = 0x7FFF;
	for (v = 0; v < w2.unit_hi; v++) {
		int d;
		if (v == c || !live(v) || !unit_is_enemy(c, v)) continue;
		d = unit_dist_tiles(c, v);
		if (d < bd && p(c, v)) { bd = d; best = v; }
	}
	return best;
}

static int can_cast(int c, int sp)
{
	return w2.unit[c].mana >= k_cost[sp] &&
	       (w2_spells[sp].upgrade == W2_NONE || econ_has_upgrade(w2.unit[c].owner, w2_spells[sp].upgrade));
}

static int ai_try(int c, int sp, int how, Pred p)
{
	int t;
	if (!can_cast(c, sp)) return 0;
	t = how == AI_BOX ? find_box(c, p) : how == AI_RANDOM ? find_random(c, p) : find_nearest(c, p);
	if (t < 0) return 0;
	issue(c, sp, t, w2.unit[t].tx, w2.unit[t].ty);
	spells_ai_casts++;
	return 1;
}

static int ai_raise(int c)
{
	int i, best = -1, bk = 0x7FFFFFFF, cx = w2.unit[c].tx, cy = w2.unit[c].ty;
	if (!can_cast(c, W2SP_RAISE_DEAD)) return 0;
	for (i = 0; i < W2_MAX_CORPSES; i++) {
		const W2Corpse *k = &w2_corpses[i];
		int key;
		if (!k->used || (k->kind != W2_CORPSE_HUMAN_DEAD_BODY && k->kind != W2_CORPSE_ORC_DEAD_BODY)) continue;
		if (k->tx < cx - 15 || k->tx > cx + 15 || k->ty < cy - 15 || k->ty > cy + 15) continue;
		key = k->ty * 256 + k->tx;                       /* the box search's row-major first hit */
		if (key < bk) { bk = key; best = i; }
	}
	if (best < 0) return 0;
	issue(c, W2SP_RAISE_DEAD, -1, w2_corpses[best].tx, w2_corpses[best].ty);
	spells_ai_casts++;
	return 1;
}

static int ai_self(int c, int sp)
{
	if (!can_cast(c, sp)) return 0;
	issue(c, sp, -1, w2.unit[c].tx, w2.unit[c].ty);
	spells_ai_casts++;
	return 1;
}

static void ai_caster(int c)
{
	W2Unit *u = &w2.unit[c];
	int o = u->order;
	if (!live(c) || u->owner >= 8 || w2.player[u->owner].control != W2_CTRL_COMPUTER) return;
	if (o == W2_ORD_CAST || o == W2_ORD_LOAD || o == W2_ORD_UNLOAD || o == W2_ORD_DEMOLISH) return;
	switch (u->type) {
	case W2U_PALADIN:            /* FUN_0013D4E8 */
		if (inv(c) && ai_try(c, W2SP_EXORCISM, AI_RANDOM, p_exorc)) return;
		if (ai_try(c, W2SP_HEALING, AI_BOX, p_heal)) return;
		if (ai_try(c, W2SP_EXORCISM, AI_BOX, p_exorc)) return;
		if (u->mana == 255) ai_try(c, W2SP_HOLY_VISION, AI_RANDOM, p_holy);
		return;
	case W2U_OGRE_MAGE:          /* FUN_0013D540 */
		if (inv(c) && ai_try(c, W2SP_RUNES, AI_RANDOM, p_enemy_worker)) return;
		if (ai_try(c, W2SP_BLOODLUST, AI_BOX, p_blood)) return;
		if (ai_try(c, W2SP_RUNES, AI_RANDOM, p_enemy_worker)) return;
		if (u->mana == 255) ai_self(c, W2SP_EYE_OF_VISION);
		return;
	case W2U_MAGE:               /* FUN_0013D598 */
		if (inv(c)) {
			if (u->mana >= 3 * k_cost[W2SP_BLIZZARD] && ai_try(c, W2SP_BLIZZARD, AI_NEAREST, p_wh)) return;
			if (ai_try(c, W2SP_POLYMORPH, AI_RANDOM, p_poly_caster)) return;
		}
		if (ai_try(c, W2SP_POLYMORPH, AI_BOX, p_poly)) return;
		if (ai_try(c, W2SP_FIREBALL, AI_BOX, p_fireball)) return;
		if (ai_try(c, W2SP_INVISIBILITY, AI_BOX, p_invis)) return;
		if (u->mana >= 3 * k_cost[W2SP_BLIZZARD] && ai_try(c, W2SP_BLIZZARD, AI_NEAREST, p_bld)) return;
		if (ai_try(c, W2SP_FLAME_SHIELD, AI_BOX, p_fshield)) return;
		ai_try(c, W2SP_SLOW, AI_BOX, p_slow);
		return;
	case W2U_DEATH_KNIGHT:       /* FUN_0013D654 */
		if (inv(c)) {
			if (u->mana >= 3 * k_cost[W2SP_DEATH_AND_DECAY] && ai_try(c, W2SP_DEATH_AND_DECAY, AI_NEAREST, p_wh)) return;
			if (ai_try(c, W2SP_WHIRLWIND, AI_RANDOM, p_whirl_wh)) return;
		}
		if (ai_raise(c)) return;
		if (ai_try(c, W2SP_UNHOLY_ARMOR, AI_BOX, p_unholy)) return;
		if (u->mana >= 3 * k_cost[W2SP_DEATH_AND_DECAY] && ai_try(c, W2SP_DEATH_AND_DECAY, AI_NEAREST, p_bld)) return;
		if (ai_try(c, W2SP_DEATH_COIL, AI_BOX, p_coil)) return;
		if (ai_try(c, W2SP_DEATH_COIL, AI_BOX, p_coil2)) return;
		if (ai_try(c, W2SP_WHIRLWIND, AI_NEAREST, p_whirl)) return;
		ai_try(c, W2SP_HASTE, AI_BOX, p_haste);
		return;
	default:
		return;
	}
}

/* ---- per cycle ------------------------------------------------------------------------------------- */
static void effects_tick(void)
{
	int i = 0;
	while (i < s_n) {
		int s = s_list[i];
		W2Unit *u = &w2.unit[s];
		if (!s_fx[s] || !unit_same(s, s_gen[s])) {
			fx_clear(s); s_inlist[s] = 0; s_list[i] = s_list[--s_n];
			continue;
		}
		/* dwarves / sappers do not survive invisibility or unholy armor (0x0016BBFA) */
		if ((u->type == W2U_DWARVES || u->type == W2U_GOBLIN_SAPPERS) && (s_inv[s] || s_unholy[s])) {
			unit_kill(s);
			fx_clear(s); s_inlist[s] = 0; s_list[i] = s_list[--s_n];
			continue;
		}
		if (s_inv[s]) s_inv[s]--;
		if (s_unholy[s]) s_unholy[s]--;
		if (s_blood[s]) s_blood[s]--;
		if (s_shield[s]) s_shield[s]--;
		if (s_haste[s]) {
			/* AnimStep opcode 07: wait /2 (min 1) hasted, x2 slowed - applied when a step is entered
			 * (the step index changed this cycle; unit.c has just set anim_wait to the step's wait) */
			if (u->anim_step != s_lstep[s] || u->anim_kind != s_lkind[s]) {
				if (u->anim_wait > 0) {
					if (s_haste[s] > 0) u->anim_wait = (uint8_t)(u->anim_wait > 1 ? u->anim_wait >> 1 : 1);
					else u->anim_wait = (uint8_t)(u->anim_wait > 127 ? 255 : u->anim_wait * 2);
				}
				s_lstep[s] = u->anim_step; s_lkind[s] = u->anim_kind;
			}
			if (s_haste[s] > 0) s_haste[s]--; else s_haste[s]++;
		}
		if (s_dec[s] && !(u->flags & W2_UF_UNDERCONST) && --s_dcnt[s] == 0) {
			s_dcnt[s] = s_dec[s];
			if (s_life[s]) s_life[s]--;
			else if (u->hp < 5) unit_kill(s);
			else u->hp = (uint16_t)(u->hp - 5);
		}
		fx_bits(s);
		i++;
	}
}

static void mana_tick(void)
{
	int s;
	for (s = (int)(w2.cycle % MANA_PERIOD); s < w2.unit_hi; s += (int)MANA_PERIOD) {
		W2Unit *u = &w2.unit[s];
		const w2_unit_type *t;
		uint16_t g;
		if (!(u->flags & W2_UF_ALIVE)) continue;
		t = w2_unit(u->type);
		if (!t) continue;
		g = unit_gen(s);
		if (!s_mvalid[s] || s_mgen[s] != g || s_mtype[s] != u->type) {
			/* a unit we have not seen in this slot yet: new, or a type change (knight -> paladin) */
			s_mvalid[s] = 1; s_mgen[s] = g; s_mtype[s] = u->type;
			if ((t->flags & W2UF_SPELLCASTER) && (u->mana == 0 || u->mana == W2_MANA_START)) u->mana = MANA_START;
			if (t->decay) fx_decay_register(s);
			continue;
		}
		if ((t->flags & W2UF_SPELLCASTER) && u->mana < 255) u->mana++;
	}
}

static void vision_tick(void)
{
	int i;
	for (i = 0; i < NVIS; i++) if (s_vis[i].t) s_vis[i].t--;
	spells_fog_stamp();
}

void spells_tick_all(void)
{
	int s;
	runes_tick();
	effects_tick();
	mana_tick();
	vision_tick();
	for (s = (int)(w2.cycle % AI_PERIOD); s < w2.unit_hi; s += (int)AI_PERIOD) {
		const w2_unit_type *t;
		if (!(w2.unit[s].flags & W2_UF_ALIVE)) continue;
		t = w2_unit(w2.unit[s].type);
		if (t && (t->flags & W2UF_SPELLCASTER)) ai_caster(s);
	}
	if (w2.cycle && w2.cycle % 4500u == 0 && (spells_ai_casts || spells_casts[W2SP_FIREBALL] || spells_casts[W2SP_BLOODLUST])) {
		uint32_t n = 0;
		for (s = 0; s < W2SP_SUICIDE_BOMBER; s++) n += spells_casts[s];
		w2_log("spells: cycle %lu, %lu casts (%lu ordered by the computer), %d runes, %d effects active",
		       (unsigned long)w2.cycle, (unsigned long)n, (unsigned long)spells_ai_casts, spells_rune_count(), s_n);
	}
}
