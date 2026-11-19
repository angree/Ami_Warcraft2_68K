/* combat.c - attack orders, target acquisition, damage, death, combat commands.
 * See combat.h for the rules and the assumptions about unit.c / move.c. Plain C, no floats. */
#include <string.h>
#include "amiga_uclock.h"
#include "w2.h"
#include "combat.h"
#include "unit.h"
#include "spells.h"
#include "pud.h"

#define ALERT_PERIOD   (W2_CYCLES_PER_SECOND_NORMAL * 10u)   /* "under attack" at most every 10 s */
#define MOVE_FAIL_MAX  3                                     /* move says "arrived" but out of range */

/* ---- module-private per-unit state ------------------------------------------------------- */
static uint8_t  c_resume[W2_MAX_UNITS];      /* order to return to after an auto engagement (0 none) */
static uint8_t  c_rtx[W2_MAX_UNITS], c_rty[W2_MAX_UNITS];
static uint16_t c_rtag[W2_MAX_UNITS];        /* target the resume belongs to */
static uint8_t  c_hold[W2_MAX_UNITS];        /* stand ground: never move while attacking */
static uint8_t  c_swing[W2_MAX_UNITS];       /* 1 = inside an attack swing */
static uint8_t  c_struck[W2_MAX_UNITS];      /* hit already delivered in this swing */
static uint8_t  c_last[W2_MAX_UNITS];        /* anim_step seen last cycle */
static uint8_t  c_fail[W2_MAX_UNITS];
static uint8_t  c_ptx[W2_MAX_UNITS], c_pty[W2_MAX_UNITS];   /* patrol: the other end */
static uint8_t  c_wall[W2_MAX_UNITS];        /* ATTACK_GROUND is the Mac's wall attack (order 11) */
/* units on the map sorted by their top row (sorted_build) */
static uint16_t c_sorted[W2_MAX_UNITS];
static uint16_t c_rows[W2_MAP_MAX + 1];
static uint32_t c_sorted_cycle = 0xFFFFFFFFu;
static uint16_t c_sorted_hi;
static uint16_t c_acq_target[W2_MAX_UNITS];
static uint32_t c_acq_cycle[W2_MAX_UNITS];

static uint32_t g_score[W2_MAX_PLAYERS];
static uint16_t g_kills[W2_MAX_PLAYERS];
static uint32_t g_last_alert;
static uint8_t  g_alerted;

/* upgrade deltas per player/type: [0] basic, [1] piercing, [2] armor, [3] range */
static int8_t   g_upg[8][W2_NUM_UNIT_TYPES][4];
static uint32_t g_upg_mask[8];
static uint8_t  g_upg_valid[8];

void combat_reset(void)
{
	memset(c_resume, 0, sizeof c_resume); memset(c_hold, 0, sizeof c_hold);
	memset(c_swing, 0, sizeof c_swing); memset(c_struck, 0, sizeof c_struck);
	memset(c_last, 0, sizeof c_last); memset(c_fail, 0, sizeof c_fail); memset(c_wall, 0, sizeof c_wall);
	memset(c_rtag, 0xFF, sizeof c_rtag); memset(c_acq_target, 0xFF, sizeof c_acq_target); c_sorted_cycle = 0xFFFFFFFFu;
	memset(c_acq_cycle, 0, sizeof c_acq_cycle);
	memset(g_score, 0, sizeof g_score); memset(g_kills, 0, sizeof g_kills);
	memset(g_upg_valid, 0, sizeof g_upg_valid);
	g_last_alert = 0; g_alerted = 0;
	missile_reset();
	spells_reset();
}

uint32_t combat_score(int p) { return (p >= 0 && p < W2_MAX_PLAYERS) ? g_score[p] : 0; }
uint16_t combat_kills(int p) { return (p >= 0 && p < W2_MAX_PLAYERS) ? g_kills[p] : 0; }

/* ---- helpers ------------------------------------------------------------------------------ */
static int alive(int s)
{
	const W2Unit *u;
	if (s < 0 || s >= W2_MAX_UNITS) return 0;
	u = &w2.unit[s];
	if (!(u->flags & W2_UF_ALIVE) || (u->flags & (W2_UF_DYING | W2_UF_HIDDEN | W2_UF_INSIDE))) return 0;
	/* killed this cycle: unit.c processes the death at its next tick */
	if (u->order == W2_ORD_DIE) return 0;
	if (u->hp == 0) { const w2_unit_type *t = w2_unit(u->type); if (t && t->hp && !(t->xflags & W2UX_INDESTRUCTIBLE)) return 0; }
	return 1;
}

static const w2_unit_type *ty(int s) { return w2_unit(w2.unit[s].type); }

static int is_archer(unsigned type)
{
	return type == W2U_ARCHER || type == W2U_AXETHROWER || type == W2U_RANGER || type == W2U_BERSERKER;
}

/* layer bits (W2_TGT_*) a unit lives in */
static int layer_of(int s)
{
	const w2_unit_type *t = ty(s);
	if (!t) return 0;
	if (t->kind == W2_KIND_AIR) return W2_TGT_AIR;
	if (t->kind == W2_KIND_SEA) return W2_TGT_SEA;
	if (t->kind == W2_KIND_BUILDING) {
		if (t->flags & W2UF_OIL_PLATFORM) return W2_TGT_SEA;
		if (t->flags & W2UF_SHORE_BUILDING) return W2_TGT_LAND | W2_TGT_SEA;
	}
	return W2_TGT_LAND;
}

static int researched(int p, int upg)
{
	return p < 8 && upg < 32 && ((w2.player[p].upgrades >> upg) & 1u);
}

/* recompute the upgrade delta table of one player from w2_modifiers[] */
static void upg_refresh(int p)
{
	unsigned i;
	memset(g_upg[p], 0, sizeof g_upg[p]);
	for (i = 0; i < w2_num_modifiers; i++) {
		const w2_modifier *m = &w2_modifiers[i];
		int8_t *d;
		if (m->unit >= W2_NUM_UNIT_TYPES || !researched(p, m->upgrade)) continue;
		d = g_upg[p][m->unit];
		switch (m->stat) {
		case W2_STAT_PIERCING:
			if (m->upgrade == W2UP_RANGER_MARKSMANSHIP) {
				/* original: marksmanship moves the ranger's basic damage into piercing */
				const w2_unit_type *t = w2_unit(m->unit);
				d[0] = (int8_t)(d[0] - t->basic_damage); d[1] = (int8_t)(d[1] + t->basic_damage);
			} else if (is_archer(m->unit)) d[1] = (int8_t)(d[1] + m->value);
			else d[0] = (int8_t)(d[0] + m->value);            /* swords, cannons, catapults: basic */
			break;
		case W2_STAT_ARMOR: d[2] = (int8_t)(d[2] + m->value); break;
		case W2_STAT_RANGE: d[3] = (int8_t)(d[3] + m->value); break;
		default: break;
		}
	}
	g_upg_mask[p] = w2.player[p].upgrades;
	g_upg_valid[p] = 1;
}

static const int8_t *upg(int s)
{
	int p = w2.unit[s].owner;
	static const int8_t none[4] = { 0, 0, 0, 0 };
	if (p >= 8 || w2.unit[s].type >= W2_NUM_UNIT_TYPES) return none;
	if (!g_upg_valid[p] || g_upg_mask[p] != w2.player[p].upgrades) upg_refresh(p);
	return g_upg[p][w2.unit[s].type];
}

int combat_weapon(int s, int *basic, int *piercing)
{
	const w2_unit_type *t = ty(s);
	const int8_t *d;
	int b, p, r, e;
	if (!t) { *basic = *piercing = 0; return 0; }
	d = upg(s);
	b = t->basic_damage + d[0];
	p = t->piercing_damage + d[1];
	r = t->max_range + d[3];
	/* economy.c's effective stats win when it implements them (weak stub returns 0) */
	e = econ_stat(s, W2_STAT_PIERCING);
	if (e > 0) {
		int delta = e - t->piercing_damage;
		if (is_archer(w2.unit[s].type)) { b = t->basic_damage; p = e; }
		else { b = t->basic_damage + delta; p = t->piercing_damage; }
	}
	e = econ_stat(s, W2_STAT_RANGE);
	if (e > 0) r = e;
	p += w2.unit[s].damage_bonus;
	if (spells_bloodlust(s)) { b *= 2; p *= 2; }     /* Mac 0x00131D9A / 0x00131E9E: doubled while +0x48 > 0 */
	if (b < 0) b = 0;
	if (p < 0) p = 0;
	*basic = b; *piercing = p;
	return r;
}

int combat_armor(int s)
{
	const w2_unit_type *t = ty(s);
	int a, e;
	if (!t) return 0;
	a = t->armor + upg(s)[2];
	e = econ_stat(s, W2_STAT_ARMOR);
	if (e > 0) a = e;
	a += w2.unit[s].armor_bonus;
	return a < 0 ? 0 : a;
}

int combat_roll(int basic, int piercing, int armor)
{
	int b = basic - armor, t, h, d;
	if (b < 0) b = 0;
	t = b + piercing;
	if (t <= 0) return 0;
	h = (t + 1) / 2;
	d = h + (int)(w2_rand() % (uint32_t)(h + 1));
	return d > 255 ? 255 : d;
}

int combat_burn_level(int s)
{
	const w2_unit_type *t;
	if (s < 0 || s >= W2_MAX_UNITS || !(w2.unit[s].flags & W2_UF_ALIVE)) return 0;
	t = ty(s);
	if (!t || t->kind != W2_KIND_BUILDING || !t->hp || (w2.unit[s].flags & W2_UF_UNDERCONST)) return 0;
	if ((uint32_t)w2.unit[s].hp * 2u < t->hp) return 2;
	if ((uint32_t)w2.unit[s].hp * 4u < (uint32_t)t->hp * 3u) return 1;
	return 0;
}

int combat_can_attack(int s)
{
	const w2_unit_type *t;
	if (!alive(s) || (w2.unit[s].flags & W2_UF_UNDERCONST)) return 0;
	t = ty(s);
	return t && (t->flags & W2UF_CAN_ATTACK) && t->can_target;
}

int combat_can_target(int s, int target)
{
	const w2_unit_type *a, *b;
	if (!alive(target) || target == s) return 0;
	a = ty(s); b = ty(target);
	if (!a || !b) return 0;
	if (b->xflags & (W2UX_INDESTRUCTIBLE | W2UX_RESOURCE)) return 0;
	if (b->kind == W2_KIND_MARKER || !b->hp) return 0;
	if (!(a->can_target & layer_of(target))) return 0;
	if ((b->flags & W2UF_SUBMARINE) && !(a->flags & (W2UF_SEE_SUBMARINE | W2UF_SUBMARINE))) return 0;
	/* invisible: only its owner sees it (Mac visibility mask +0x29 = owner bit) */
	if (w2.unit[target].owner != w2.unit[s].owner && spells_invisible(target)) return 0;
	return 1;
}

int combat_in_range(int s, int target)
{
	int b, p, d = unit_dist_tiles(s, target), r = combat_weapon(s, &b, &p);
	return d <= r && d >= ty(s)->min_range;
}

static int cannot_move(int s)
{
	const w2_unit_type *t = ty(s);
	return !t || t->kind == W2_KIND_BUILDING;
}

/* ---- acquisition --------------------------------------------------------------------------- */
/* units on the map sorted by their top row, rebuilt once per cycle (counting sort, O(units + rows)) */

static void sorted_build(void)
{
	uint16_t pos[W2_MAP_MAX + 1];
	int sl, y;
	memset(pos, 0, sizeof pos);
	for (sl = 0; sl < w2.unit_hi; sl++) {
		const W2Unit *u = &w2.unit[sl];
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN | W2_UF_DYING))) continue;
		pos[u->ty]++;
	}
	c_rows[0] = 0;
	for (y = 0; y < W2_MAP_MAX; y++) c_rows[y + 1] = (uint16_t)(c_rows[y] + pos[y]);
	memcpy(pos, c_rows, sizeof pos);
	for (sl = 0; sl < w2.unit_hi; sl++) {
		const W2Unit *u = &w2.unit[sl];
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN | W2_UF_DYING))) continue;
		c_sorted[pos[u->ty]++] = (uint16_t)sl;
	}
	c_sorted_cycle = w2.cycle; c_sorted_hi = w2.unit_hi;
}

int combat_acquire(int s)
{
	const W2Unit *u;
	const w2_unit_type *t;
	int r, x0, y0, x1, y1, x, y, best = -1, best_pri = -1, best_d = 0x7FFF, minr, maxr, b, p, last = -1;
	int fixed;
	if (!combat_can_attack(s)) return -1;
	u = &w2.unit[s]; t = ty(s);
	if (t->xflags & W2UX_COWARD) return -1;
	if (spells_invisible(s)) return -1;          /* an invisible unit does not auto-acquire (0x00120D14) */
	r = (u->owner < W2_MAX_PLAYERS && w2.player[u->owner].control == W2_CTRL_COMPUTER) ? t->react_cpu : t->react_human;
	maxr = combat_weapon(s, &b, &p);
	minr = t->min_range;
	fixed = cannot_move(s) || c_hold[s] || u->order == W2_ORD_STAND;
	if (fixed || r < maxr) r = maxr;              /* towers/stand ground: weapon range only */
	if (r <= 0) return -1;
	x0 = u->tx - r; y0 = u->ty - r;
	x1 = u->tx + t->tile_w - 1 + r; y1 = u->ty + t->tile_h - 1 + r;
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= w2.map.w) x1 = w2.map.w - 1;
	if (y1 >= w2.map.h) y1 = w2.map.h - 1;
	/* candidates from the row-sorted unit list (as the Mac's y-sorted list, 0x0012022E) instead of a
	 * scan of every tile of the box: the cost follows the units near the box, not its area */
	if (c_sorted_cycle != w2.cycle || c_sorted_hi != w2.unit_hi) sorted_build();
	{
		int i, e, ya = y0 - 3 < 0 ? 0 : y0 - 3;      /* a building's top row may lie above the box */
		(void)x; (void)y; (void)last;
		e = c_rows[y1 + 1];
		for (i = c_rows[ya]; i < e; i++) {
			int c = c_sorted[i], d, pri, tw, th;
			const W2Unit *v = &w2.unit[c];
			const w2_unit_type *vt;
			if (c == s || !(v->flags & W2_UF_ALIVE) || (v->flags & (W2_UF_INSIDE | W2_UF_HIDDEN | W2_UF_DYING))) continue;
			vt = ty(c);
			if (!vt) continue;
			tw = vt->tile_w ? vt->tile_w : 1; th = vt->tile_h ? vt->tile_h : 1;
			if (v->tx > x1 || v->tx + tw - 1 < x0 || v->ty > y1 || v->ty + th - 1 < y0) continue;
			if (v->flags & W2_UF_AIR) { if (!(t->can_target & W2_TGT_AIR)) continue; }
			else if (!(t->can_target & (W2_TGT_LAND | W2_TGT_SEA))) continue;
			if (!unit_is_enemy(s, c) || !combat_can_target(s, c) || !unit_owner_sees(u->owner, c)) continue;
			d = unit_dist_tiles(s, c);
			if (d < minr || d > r || (fixed && d > maxr)) continue;
			pri = vt->priority;
			if (pri > best_pri || (pri == best_pri && d < best_d)) { best = c; best_pri = pri; best_d = d; }
		}
	}
	c_acq_target[s] = (uint16_t)(best < 0 ? W2_NO_UNIT : best);
	c_acq_cycle[s] = w2.cycle;
	return best;
}

void combat_engage(int s, int target)
{
	W2Unit *u = &w2.unit[s];
	if (!combat_can_attack(s) || !combat_can_target(s, target)) return;
	switch (u->order) {
	case W2_ORD_ATTACK:
		if (c_resume[s] && c_rtag[s] == u->target) c_rtag[s] = (uint16_t)target;
		break;
	case W2_ORD_STAND:
		c_resume[s] = W2_ORD_STAND; c_hold[s] = 1; c_rtx[s] = u->tx; c_rty[s] = u->ty; c_rtag[s] = (uint16_t)target;
		break;
	case W2_ORD_ATTACK_MOVE: case W2_ORD_PATROL:
		c_resume[s] = u->order; c_rtx[s] = u->goal_tx; c_rty[s] = u->goal_ty; c_rtag[s] = (uint16_t)target;
		c_hold[s] = 0;
		break;
	default:
		c_resume[s] = W2_ORD_STILL; c_rtx[s] = u->tx; c_rty[s] = u->ty; c_rtag[s] = (uint16_t)target;
		c_hold[s] = 0;
		break;
	}
	c_fail[s] = 0;
	unit_set_order(s, W2_ORD_ATTACK, target, u->goal_tx, u->goal_ty);
}

/* ---- the attack itself ------------------------------------------------------------------- */
static int attack_step_index(int s)
{
	unsigned n, i;
	const w2_anim_step *a = w2_unit_anim(w2.unit[s].type, W2_ANIM_ATTACK, &n);
	for (i = 0; a && i < n; i++) if (a[i].flags & W2AF_ATTACK) return (int)i;
	return 0;
}

static void strike(int s, int target, int gx, int gy)
{
	const w2_unit_type *t = ty(s);
	int sx, sy, dx, dy, b, p;
	if (target >= 0) { unit_center(target, &dx, &dy); }
	else { dx = gx * W2_TILE + W2_TILE / 2; dy = gy * W2_TILE + W2_TILE / 2; }
	unit_set_dir_to(s, dx, dy);
	spells_break_invis(s);                       /* attacking ends invisibility (0x001201CC) */
	if (t->missile >= W2M_NONE || t->missile >= W2_NUM_MISSILES) {
		if (target >= 0 && alive(target)) {
			combat_weapon(s, &b, &p);
			unit_damage(target, combat_roll(b, p, combat_armor(target)), s);
		} else if (target < 0) {
			/* Order_AttackHit 0x0012016A: no target unit -> DamageWallTile(+0x84, CalcDamage(u, NULL)) */
			combat_weapon(s, &b, &p);
			pud_wall_hit(gx, gy, combat_roll(b, p, 0));
		}
		return;
	}
	unit_center(s, &sx, &sy);
	missile_fire(s, t->missile, sx, sy, dx, dy, target);
}

static void leave_attack_anim(int s)
{
	c_swing[s] = 0;
	if (w2.unit[s].anim_kind == W2_ANIM_ATTACK) unit_set_anim(s, W2_ANIM_STILL);
}

/* the current target is gone: re-acquire, resume the interrupted order or go idle */
static void finish_target(int s)
{
	W2Unit *u = &w2.unit[s];
	int old = u->target, nt = -1, res = c_resume[s] && c_rtag[s] == old;
	if (!(ty(s)->xflags & W2UX_COWARD) && (res || u->order == W2_ORD_ATTACK)) nt = combat_acquire(s);
	if (nt >= 0 && nt != old) {
		unit_set_target(s, nt);
		if (res) c_rtag[s] = (uint16_t)nt;
		c_fail[s] = 0;
		return;
	}
	leave_attack_anim(s);
	if (res) {
		int o = c_resume[s];
		c_resume[s] = 0;
		unit_set_order(s, o, W2_NO_UNIT, c_rtx[s], c_rty[s]);
		if (o != W2_ORD_STAND) c_hold[s] = 0;
	} else {
		c_resume[s] = 0; c_hold[s] = 0;
		unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
	}
}

/* shared by ord_attack (target >= 0) and ord_attack_ground (target < 0, goal tile) */
static void attack_core(int s, int ground)
{
	W2Unit *u = &w2.unit[s];
	const w2_unit_type *t = ty(s);
	int target = ground ? -1 : (int)u->target, gx = u->goal_tx, gy = u->goal_ty;
	int valid, d, maxr, b, p, step, atk;

	if (!combat_can_attack(s)) { leave_attack_anim(s); unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty); return; }
	/* the order target by identity (unit_target_same): not a newcomer that reused a dead target's slot */
	valid = ground ? 1 : (unit_target_same(s) && combat_can_target(s, target) && unit_owner_sees(u->owner, target));
	maxr = combat_weapon(s, &b, &p);
	d = ground ? unit_dist_to_tile(s, gx, gy) : (valid ? unit_dist_tiles(s, target) : 0x7FFF);
	atk = attack_step_index(s);

	/* inside a swing: wait for the hit step, then for the end of the swing */
	if (u->anim_kind == W2_ANIM_ATTACK && c_swing[s]) {
		int ev = unit_anim_event(s);
		step = u->anim_step;
		if (step < c_last[s] || unit_anim_done(s)) {
			c_swing[s] = 0;                  /* wrapped: the swing is over, decide below */
		} else {
			c_last[s] = (uint8_t)step;
			if (!c_struck[s] && ((ev & W2AF_ATTACK) || step >= atk)) {
				c_struck[s] = 1;
				/* Mac Order_AttackHit 0x0012016A: no range test at the hit event (a target that stepped
				 * away during the swing is still hit / shot at) */
				if (valid) strike(s, target, gx, gy);
			}
			return;                          /* mid-swing: decide at the wrap */
		}
	} else c_swing[s] = 0;

	if (!valid) { finish_target(s); return; }

	if (d <= maxr && d >= t->min_range && u->ox == 0 && u->oy == 0) {
		int cx, cy;
		if (target >= 0) unit_center(target, &cx, &cy);
		else { cx = gx * W2_TILE + W2_TILE / 2; cy = gy * W2_TILE + W2_TILE / 2; }
		unit_set_dir_to(s, cx, cy);
		c_swing[s] = 1; c_struck[s] = 0; c_fail[s] = 0;
		if (u->anim_kind != W2_ANIM_ATTACK) {
			/* new swing: step 0 is entered (and its events reported) at the next unit tick */
			unit_set_anim(s, W2_ANIM_ATTACK);
			c_last[s] = 0;
			return;
		}
		/* continuing after a wrap: step 0 was entered this cycle */
		step = u->anim_step;
		c_last[s] = (uint8_t)step;
		if (step >= atk || (unit_anim_event(s) & W2AF_ATTACK)) {
			c_struck[s] = 1;
			strike(s, target, gx, gy);
		}
		return;
	}
	if (d < t->min_range && u->ox == 0 && u->oy == 0) {    /* too close for a siege weapon */
		if (ground) { leave_attack_anim(s); unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty); }
		else finish_target(s);
		return;
	}
	/* out of range (or finishing a tile step) */
	if (cannot_move(s) || c_hold[s]) {
		if (ground) { leave_attack_anim(s); unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty); }
		else finish_target(s);
		return;
	}
	if (u->anim_kind == W2_ANIM_ATTACK) unit_set_anim(s, W2_ANIM_STILL);
	{
		int r = ground ? move_toward(s, gx, gy, maxr) : move_toward_unit(s, target, maxr);
		if (r < 0 || (r == 1 && d > maxr && ++c_fail[s] >= MOVE_FAIL_MAX)) {
			c_fail[s] = 0;
			if (ground) unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
			else {
				/* unreachable: drop it (do not re-acquire the same one right away) */
				leave_attack_anim(s);
				if (c_resume[s] && c_rtag[s] == u->target) {
					int o = c_resume[s];
					c_resume[s] = 0;
					unit_set_order(s, o, W2_NO_UNIT, c_rtx[s], c_rty[s]);
				} else unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
			}
		}
	}
}

void ord_attack(int s) { attack_core(s, 0); }

void ord_attack_ground(int s)
{
	const w2_unit_type *t = ty(s);
	if (c_wall[s]) {
		/* FUN_001211F2: order 11 ends as soon as its tile is no wall (any unit kind, melee too) */
		W2Unit *u = &w2.unit[s];
		if (!pud_wall_at(u->goal_tx, u->goal_ty) || !t) {
			c_wall[s] = 0; leave_attack_anim(s);
			unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
			return;
		}
		attack_core(s, 1);
		return;
	}
	if (!t || t->missile >= W2M_NONE) { unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, w2.unit[s].tx, w2.unit[s].ty); return; }
	attack_core(s, 1);
}

void ord_attack_move(int s)
{
	W2Unit *u = &w2.unit[s];
	int r;
	if (unit_thinks(s) && combat_can_attack(s)) {
		int t = combat_acquire(s);
		if (t >= 0) { combat_engage(s, t); return; }
	}
	r = move_toward(s, u->goal_tx, u->goal_ty, 0);
	if (r != 0) unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
}

void ord_patrol(int s)
{
	W2Unit *u = &w2.unit[s];
	int r;
	if (unit_thinks(s) && combat_can_attack(s)) {
		int t = combat_acquire(s);
		if (t >= 0) { combat_engage(s, t); return; }
	}
	r = move_toward(s, u->goal_tx, u->goal_ty, 0);
	if (r < 0) { unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty); return; }
	if (r == 1) {
		int nx = c_ptx[s], ny = c_pty[s];
		c_ptx[s] = u->goal_tx; c_pty[s] = u->goal_ty;
		unit_set_order(s, W2_ORD_PATROL, W2_NO_UNIT, nx, ny);
	}
}

/* death hook: called ONCE by unit.c when it starts the death (the unit is already DYING, not ALIVE,
 * unplaced). unit.c plays the death animation, leaves the corpse and frees the slot. */
void ord_die(int s)
{
	const w2_unit_type *t;
	if (s < 0 || s >= W2_MAX_UNITS) return;
	t = ty(s);
	c_swing[s] = 0; c_resume[s] = 0; c_hold[s] = 0; c_fail[s] = 0; c_rtag[s] = W2_NO_UNIT; c_wall[s] = 0;
	snd_unit(s, 4);
	if (t && (t->kind == W2_KIND_BUILDING || (t->xflags & W2UX_EXPLODES))) {
		int cx, cy;
		unit_center(s, &cx, &cy);
		missile_fire_dmg(-1, W2M_EXPLOSION, cx, cy, cx, cy, -1, 0);
	}
}

/* ---- damage -------------------------------------------------------------------------------- */
void unit_damage(int v, int amount, int attacker)
{
	W2Unit *u;
	const w2_unit_type *t;
	int att_ok;
	if (v < 0 || v >= W2_MAX_UNITS || !alive(v)) return;
	u = &w2.unit[v]; t = ty(v);
	if (!t || (t->xflags & W2UX_INDESTRUCTIBLE)) return;
	att_ok = attacker >= 0 && attacker < W2_MAX_UNITS && attacker != v && (w2.unit[attacker].flags & W2_UF_ALIVE);
	if (spells_unholy(v)) amount = 0;            /* unholy armor: DamageUnit 0x00132148 ignores the hit */
	if (amount > 0) {
		if ((int)u->hp <= amount) {
			u->hp = 0;
			if (att_ok) {
				int p = w2.unit[attacker].owner;
				w2.unit[attacker].kills++;
				if (p != u->owner && p < W2_MAX_PLAYERS) { g_score[p] += t->points; g_kills[p]++; }
			}
			unit_set_order(v, W2_ORD_DIE, W2_NO_UNIT, u->tx, u->ty);   /* unit.c does the rest */
			c_swing[v] = 0;
			return;
		}
		u->hp = (uint16_t)(u->hp - amount);
	}
	if (!att_ok || w2.unit[attacker].owner == u->owner) return;
	/* under-attack alert for the local player */
	if (u->owner == w2.local_player && (!g_alerted || w2.cycle - g_last_alert >= ALERT_PERIOD)) {
		g_alerted = 1; g_last_alert = w2.cycle;
		ui_message(t->kind == W2_KIND_BUILDING ? "Your base is under attack" : "Your forces are under attack");
		snd_unit(v, 3);
	}
	/* idle units fight back */
	if ((u->order == W2_ORD_STILL || u->order == W2_ORD_NONE || u->order == W2_ORD_STAND) &&
	    combat_can_attack(v) && !(t->xflags & W2UX_COWARD) &&
	    unit_is_enemy(v, attacker) && combat_can_target(v, attacker)) {
		if ((u->order != W2_ORD_STAND && !cannot_move(v)) || combat_in_range(v, attacker))
			combat_engage(v, attacker);
	}
}

/* splash damage of the original (RE_NOTES §5): every unit with a footprint tile in the 7x7 tile area
 * around the impact is hit once; m = max(dx^2, dy^2) to the unit centre: m <= 0x1FF full rating,
 * m <= 0x6FF rating/4, else nothing; minus armor; d <= 0 nothing; then h + rand % (h+1). */
static uint16_t g_victims[128];

void combat_area_damage(int px, int py, int rating, int attacker, int layer_mask, int ignore_armor)
{
	int tx = px >> 5, ty_ = py >> 5, x, y, n = 0, i;
	if (rating <= 0) return;
	for (y = ty_ - 3; y <= ty_ + 3; y++) {
		if (y < 0 || y >= w2.map.h) continue;
		for (x = tx - 3; x <= tx + 3; x++) {
			int pass;
			if (x < 0 || x >= w2.map.w) continue;
			for (pass = 0; pass < 2; pass++) {
				int c, j;
				if (pass == 0) { if (!(layer_mask & (W2_TGT_LAND | W2_TGT_SEA))) continue; c = w2.map.occ_ground[y * w2.map.w + x]; }
				else { if (!(layer_mask & W2_TGT_AIR)) continue; c = w2.map.occ_air[y * w2.map.w + x]; }
				if (c == W2_NO_UNIT || c == attacker || !alive(c)) continue;   /* Mac 0x00129C54 skips the shooter */
				for (j = 0; j < n && g_victims[j] != c; j++) ;
				if (j < n || n >= (int)(sizeof g_victims / sizeof g_victims[0])) continue;
				g_victims[n++] = (uint16_t)c;
			}
		}
	}
	for (i = 0; i < n; i++) {
		int v = g_victims[i], cx, cy, dx, dy, m, d, h, dmg;
		const w2_unit_type *t = ty(v);
		if (!alive(v) || !t || (t->xflags & (W2UX_INDESTRUCTIBLE | W2UX_RESOURCE)) || !t->hp) continue;
		unit_center(v, &cx, &cy);
		dx = cx - px; dy = cy - py;
		m = dx * dx > dy * dy ? dx * dx : dy * dy;
		if (m <= 0x1FF) d = rating;
		else if (m <= 0x6FF) d = rating / 4;
		else continue;
		if (!ignore_armor) d -= combat_armor(v);
		if (d <= 0) continue;
		h = (d + 1) >> 1;
		dmg = h + (int)(w2_rand() % (uint32_t)(h + 1));
		unit_damage(v, dmg > 255 ? 255 : dmg, attacker);
	}
}

/* ---- commands ------------------------------------------------------------------------------ */
static void clear_state(int s)
{
	c_resume[s] = 0; c_hold[s] = 0; c_fail[s] = 0; c_rtag[s] = W2_NO_UNIT; c_wall[s] = 0;
}

static int ok_slot(int s) { return s >= 0 && s < W2_MAX_UNITS && alive(s); }

void cmd_stop(int s)
{
	W2Unit *u;
	if (!ok_slot(s)) return;
	u = &w2.unit[s];
	clear_state(s);
	if (!cannot_move(s)) move_stop(s);
	leave_attack_anim(s);
	unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
}

void cmd_stand(int s)
{
	W2Unit *u;
	if (!ok_slot(s)) return;
	u = &w2.unit[s];
	clear_state(s);
	c_hold[s] = 1;
	if (!cannot_move(s)) move_stop(s);
	leave_attack_anim(s);
	unit_set_order(s, W2_ORD_STAND, W2_NO_UNIT, u->tx, u->ty);
}

void cmd_attack(int s, int target)
{
	W2Unit *u;
	if (!ok_slot(s) || target < 0 || target >= W2_MAX_UNITS || target == s || !alive(target)) return;
	u = &w2.unit[s];
	if (!combat_can_attack(s)) { if (!cannot_move(s)) cmd_follow(s, target); return; }
	if (!combat_can_target(s, target)) return;
	/* idle auto-attack issued as a command right after combat_acquire (see combat.h) */
	if (c_acq_cycle[s] == w2.cycle && c_acq_target[s] == target &&
	    (u->order == W2_ORD_STILL || u->order == W2_ORD_NONE || u->order == W2_ORD_STAND ||
	     u->order == W2_ORD_ATTACK_MOVE || u->order == W2_ORD_PATROL)) {
		combat_engage(s, target);
		return;
	}
	clear_state(s);
	if (u->order == W2_ORD_ATTACK && u->target == target) return;   /* already on it: keep the swing */
	unit_set_order(s, W2_ORD_ATTACK, target, u->tx, u->ty);
}

void cmd_attack_ground(int s, int tx, int ty_)
{
	const w2_unit_type *t;
	if (!ok_slot(s)) return;
	t = ty(s);
	if (!combat_can_attack(s) || !(t->xflags & W2UX_GROUND_ATTACK) || t->missile >= W2M_NONE) {
		cmd_attack_move(s, tx, ty_);
		return;
	}
	clear_state(s);
	unit_set_order(s, W2_ORD_ATTACK_GROUND, W2_NO_UNIT, tx, ty_);
}

int cmd_attack_wall(int s, int tx, int ty_)
{
	W2Unit *u;
	if (!ok_slot(s) || !combat_can_attack(s) || !pud_wall_at(tx, ty_)) return 0;
	u = &w2.unit[s];
	if (u->order == W2_ORD_ATTACK_GROUND && c_wall[s] && u->goal_tx == tx && u->goal_ty == ty_) return 1;
	clear_state(s);
	c_wall[s] = 1;
	unit_set_order(s, W2_ORD_ATTACK_GROUND, W2_NO_UNIT, tx, ty_);
	return 1;
}

void cmd_attack_move(int s, int tx, int ty_)
{
	if (!ok_slot(s)) return;
	clear_state(s);
	if (cannot_move(s)) return;
	if (!combat_can_attack(s)) { cmd_move(s, tx, ty_); return; }
	leave_attack_anim(s);
	unit_set_order(s, W2_ORD_ATTACK_MOVE, W2_NO_UNIT, tx, ty_);
}

void cmd_patrol(int s, int tx, int ty_)
{
	if (!ok_slot(s) || cannot_move(s)) return;
	clear_state(s);
	c_ptx[s] = w2.unit[s].tx; c_pty[s] = w2.unit[s].ty;
	leave_attack_anim(s);
	unit_set_order(s, W2_ORD_PATROL, W2_NO_UNIT, tx, ty_);
}

void cmd_smart(int s, int tx, int ty_, int target)
{
	const w2_unit_type *t, *tt;
	W2Unit *u;
	if (!ok_slot(s)) return;
	u = &w2.unit[s]; t = ty(s);
	if (target == W2_NO_UNIT || target == s || !(target >= 0 && target < W2_MAX_UNITS && alive(target))) target = -1;
	if (transport_smart(s, tx, ty_, target)) return;     /* boarding / landing (transport.c) */
	if (cannot_move(s)) {   /* towers: attack; other buildings: nothing (rally points are not in W2) */
		if (target >= 0 && unit_is_enemy(s, target) && combat_can_attack(s) && combat_can_target(s, target)) cmd_attack(s, target);
		return;
	}
	if (target >= 0) {
		tt = ty(target);
		/* demolition squad / sappers (UDTA right-mouse action 5): right-click on an enemy = blow it up */
		if (t->rmb_action == W2_RMB_DEMOLISH && unit_is_enemy(s, target) && cmd_demolish(s, target, tx, ty_)) return;
		if (unit_is_enemy(s, target)) {
			if (combat_can_attack(s) && combat_can_target(s, target)) { cmd_attack(s, target); return; }
			cmd_move(s, tx, ty_);
			return;
		}
		if (t->rmb_action == W2_RMB_HARVEST) {
			if (tt->flags & W2UF_GOLD_MINE) { cmd_harvest(s, target, tx, ty_); return; }
			if (w2.unit[target].owner == u->owner && tt->kind == W2_KIND_BUILDING) {
				if (u->res_kind >= W2_RES_GOLD && u->res_kind <= W2_RES_OIL && u->res_amount &&
				    (tt->store & (1u << (u->res_kind - 1))) && !(w2.unit[target].flags & W2_UF_UNDERCONST)) {
					cmd_return(s); return;
				}
				if ((t->xflags & W2UX_CAN_REPAIR) &&
				    ((w2.unit[target].flags & W2_UF_UNDERCONST) || w2.unit[target].hp < tt->hp)) {
					cmd_repair(s, target); return;
				}
			}
		}
		if (t->rmb_action == W2_RMB_HAUL_OIL) {
			if ((tt->flags & W2UF_OIL_PLATFORM) && w2.unit[target].owner == u->owner) { cmd_harvest(s, target, tx, ty_); return; }
			if (w2.unit[target].owner == u->owner && (tt->store & W2_STORE_OIL) && u->res_amount) { cmd_return(s); return; }
		}
		if (tt->kind != W2_KIND_BUILDING && w2.unit[target].owner == u->owner) { cmd_follow(s, target); return; }
		cmd_move(s, tx, ty_);
		return;
	}
	/* f10_4692 (right-click handler of the attack class): no enemy unit + a wall tile -> order 11 */
	if (t->rmb_action == W2_RMB_ATTACK && pud_wall_at(tx, ty_) && cmd_attack_wall(s, tx, ty_)) return;
	if (t->rmb_action == W2_RMB_HARVEST && tx >= 0 && ty_ >= 0 && tx < w2.map.w && ty_ < w2.map.h &&
	    (w2.map.flags[ty_ * w2.map.w + tx] & W2_TF_TREE)) {
		cmd_harvest(s, W2_NO_UNIT, tx, ty_);
		return;
	}
	cmd_move(s, tx, ty_);
}
