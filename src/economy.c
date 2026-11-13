/* economy.c - gathering (gold, wood, oil), building, training, research, building upgrades,
 * food, costs, dependencies and upgrade-modified stats. See the [economy.c] block of w2.h and
 * economy.h. Game facts: docs/SPEC_RULES.md sections 2, 7, 8 (w2_harvest_info, w2_dependencies,
 * w2_modifiers). Tree removal follows the forest-edge rules documented by Stratagus (read as a
 * description of behaviour only).
 *
 * Plain C (gnu99), no floats, no big locals, deterministic (no RNG used here).
 */
#include <string.h>
#include "w2.h"
#include "pud.h"
#include "assets.h"
#include "economy.h"
#include "unit.h"

/* ------------------------------------------------------------------------------------------- */
/* module-private state                                                                         */

/* per worker: what it harvests, to come back after unloading */
static uint8_t  g_mem_kind[W2_MAX_UNITS];       /* W2_RES_* */
static uint16_t g_mem_target[W2_MAX_UNITS];     /* mine / platform slot */
static uint8_t  g_mem_tx[W2_MAX_UNITS], g_mem_ty[W2_MAX_UNITS];   /* tree tile */
static uint16_t g_chop[W2_MAX_UNITS];           /* wood gathered so far on this tree */
static uint8_t  g_fails[W2_MAX_UNITS];          /* path failures while looking for a tree */
/* per building */
static uint16_t g_builder[W2_MAX_UNITS];        /* worker inside a construction site */
static uint8_t  g_inside[W2_MAX_UNITS];         /* workers inside a gold mine / at an oil platform */
static uint8_t  g_rep_cnt[W2_MAX_UNITS];        /* repair steps on this (repaired) unit, mod 2 */
static uint16_t g_rep_gen[W2_MAX_UNITS];        /* unit_gen of the slot g_rep_cnt belongs to */

/* trees: special one-tree / stump tiles that have no MTXM value (drawn megatile set directly) */
enum { SP_NONE, SP_BOT, SP_TOP, SP_MID, SP_STUMP };
static uint8_t  g_special[W2_MAP_MAX * W2_MAP_MAX];
/* megatiles of the special forest pieces, same in all three eras (Wargus tileset "special" slots) */
static const uint16_t k_special_mega[5] = { 0, 123, 121, 122, 126 };
#define TILE_GRASS  0x0050u      /* light grass, variant 0: flags of a cleared forest tile */
#define TILE_FOREST 0x0070u      /* solid forest, variant 0 */

/* stat cache (players 0..7) */
#define NSTAT 5                  /* PIERCING, ARMOR, SIGHT, RANGE, REGEN */
static int8_t   g_bonus[8][W2_NUM_UNIT_TYPES][NSTAT];
static uint8_t  g_conv[8][W2_NUM_UNIT_TYPES];
static uint32_t g_key_lo[8], g_key_hi[8];
static uint8_t  g_key_ok[8];

/* owned completed unit types per player, recomputed at most once per cycle */
static uint32_t g_owned[8][4];
static uint32_t g_owned_stamp;   /* w2.cycle + 1 of the last computation, 0 = invalid */

static uint32_t g_last_cycle;

/* ------------------------------------------------------------------------------------------- */
/* small helpers                                                                                */

void econ_reset(void)
{
	memset(g_mem_kind, 0, sizeof g_mem_kind);
	memset(g_mem_target, 0xFF, sizeof g_mem_target);
	memset(g_mem_tx, 0, sizeof g_mem_tx);
	memset(g_mem_ty, 0, sizeof g_mem_ty);
	memset(g_chop, 0, sizeof g_chop);
	memset(g_fails, 0, sizeof g_fails);
	memset(g_builder, 0xFF, sizeof g_builder);
	memset(g_inside, 0, sizeof g_inside);
	memset(g_special, 0, sizeof g_special);
	memset(g_key_ok, 0, sizeof g_key_ok);
	memset(g_rep_cnt, 0, sizeof g_rep_cnt);
	memset(g_rep_gen, 0, sizeof g_rep_gen);
	g_owned_stamp = 0;
	g_last_cycle = w2.cycle;
}

/* a new game restarts w2.cycle: forget everything */
static void check_reset(void)
{
	if (w2.cycle < g_last_cycle) econ_reset();
	g_last_cycle = w2.cycle;
}

static int alive(int s)
{
	return s >= 0 && s < W2_MAX_UNITS &&
	       (w2.unit[s].flags & (W2_UF_ALIVE | W2_UF_DYING)) == W2_UF_ALIVE;
}

static const w2_unit_type *T(int s) { return w2_unit(w2.unit[s].type); }

static int is_local(int player) { return player == (int)w2.local_player; }

static void msg_idx(int player, int idx, const char *fallback)
{
	const char *s;
	if (!is_local(player)) return;
	s = assets_string(1, idx);
	ui_message((s && *s) ? s : fallback);
}

static void msg_cost(int player, int gold, int wood, int oil)
{
	const W2Player *p = &w2.player[player];
	if (p->gold < gold) msg_idx(player, ECON_STR_GOLD, "Not enough gold...mine more gold.");
	else if (p->wood < wood) msg_idx(player, ECON_STR_LUMBER, "Not enough lumber...chop more trees.");
	else if (p->oil < oil) msg_idx(player, ECON_STR_OIL, "Not enough oil...drill for oil.");
}

static int in_map(int x, int y) { return x >= 0 && y >= 0 && x < w2.map.w && y < w2.map.h; }
static int midx(int x, int y) { return y * w2.map.w + x; }

/* gap between two footprints in tiles: 0 overlap, 1 adjacent (Chebyshev) */
static int fp_gap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
	int dx, dy;
	if (ax + aw <= bx) dx = bx - (ax + aw) + 1;
	else if (bx + bw <= ax) dx = ax - (bx + bw) + 1;
	else dx = 0;
	if (ay + ah <= by) dy = by - (ay + ah) + 1;
	else if (by + bh <= ay) dy = ay - (by + bh) + 1;
	else dy = 0;
	return dx > dy ? dx : dy;
}

static int unit_gap(int a, int b)
{
	const w2_unit_type *ta = T(a), *tb = T(b);
	return fp_gap(w2.unit[a].tx, w2.unit[a].ty, ta->tile_w, ta->tile_h,
	              w2.unit[b].tx, w2.unit[b].ty, tb->tile_w, tb->tile_h);
}

static int is_sea(int s) { return T(s)->kind == W2_KIND_SEA; }

static void set_anim(int s, int kind)
{
	if (w2.unit[s].anim_kind != kind) unit_set_anim(s, kind);
}

static void go_still(int s)
{
	unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, 0, 0);
	set_anim(s, W2_ANIM_STILL);
}

/* ------------------------------------------------------------------------------------------- */
/* free tiles around a footprint (exits, spawns)                                                */

static int tile_free_for(int kind, int x, int y)
{
	int i; uint16_t f;
	if (!in_map(x, y)) return 0;
	i = midx(x, y); f = w2.map.flags[i];
	if (kind == W2_KIND_AIR) return w2.map.occ_air[i] == W2_NO_UNIT;
	if (f & W2_TF_BUILDING) return 0;
	if (w2.map.occ_ground[i] != W2_NO_UNIT) return 0;
	if (kind == W2_KIND_SEA) return (f & W2_TF_WATER) != 0;
	return !(f & W2_TF_NOWALK);
}

/* Nearest free tile on the rings around the footprint (bx,by,bw,bh), closest to (px,py) among
 * the tiles of the first ring that has any. Rings 1..8. */
static int find_spot(int kind, int bx, int by, int bw, int bh, int px, int py, int *ox, int *oy)
{
	int r, x, y, best = -1;
	long bd = 0x7FFFFFFFL;
	for (r = 1; r <= 8 && best < 0; r++) {
		int x0 = bx - r, y0 = by - r, x1 = bx + bw - 1 + r, y1 = by + bh - 1 + r;
		for (y = y0; y <= y1; y++) {
			int step = (y == y0 || y == y1) ? 1 : (x1 - x0);
			if (step <= 0) step = 1;
			for (x = x0; x <= x1; x += step) {
				long dx, dy, d;
				if (!tile_free_for(kind, x, y)) continue;
				dx = x - px; dy = y - py; d = dx * dx + dy * dy;
				if (d < bd) { bd = d; best = 1; *ox = x; *oy = y; }
			}
		}
	}
	return best > 0;
}

/* put a unit that was inside a building/mine back on the map next to the footprint */
static int exit_near(int s, int bx, int by, int bw, int bh, int px, int py)
{
	int x, y;
	W2Unit *u = &w2.unit[s];
	if (!(u->flags & W2_UF_INSIDE)) return 1;
	if (!find_spot(T(s)->kind, bx, by, bw, bh, px, py, &x, &y)) return 0;
	u->flags &= (uint16_t)~W2_UF_INSIDE;
	u->ox = 0; u->oy = 0;
	unit_place(s, x, y);
	return 1;
}

static int exit_unit(int s, int b, int px, int py)
{
	const w2_unit_type *t = T(b);
	return exit_near(s, w2.unit[b].tx, w2.unit[b].ty, t->tile_w, t->tile_h, px, py);
}

static void go_inside(int s)
{
	if (is_sea(s)) return;               /* tankers dock beside platforms/depots, stay visible */
	unit_unplace(s);
	w2.unit[s].flags |= W2_UF_INSIDE;
}

/* ------------------------------------------------------------------------------------------- */
/* costs, food, counts                                                                          */

int econ_can_afford(int player, int gold, int wood, int oil)
{
	const W2Player *p = &w2.player[player];
	return p->gold >= gold && p->wood >= wood && p->oil >= oil;
}

void econ_pay(int player, int gold, int wood, int oil)
{
	W2Player *p = &w2.player[player];
	p->gold -= gold; p->wood -= wood; p->oil -= oil;
}

static void refund_pct(int player, int gold, int wood, int oil, int pct)
{
	W2Player *p = &w2.player[player];
	p->gold += gold * pct / 100; p->wood += wood * pct / 100; p->oil += oil * pct / 100;
}

void econ_recount(int player)
{
	int s, supply = 0, used = 0, units = 0, bld = 0;
	W2Player *p;
	if (player < 0 || player >= W2_MAX_PLAYERS) return;
	for (s = 0; s < w2.unit_hi; s++) {
		const w2_unit_type *t;
		if (!alive(s) || w2.unit[s].owner != player) continue;
		t = T(s);
		if (w2.unit[s].flags & W2_UF_BUILDING) {
			bld++;
			if (!(w2.unit[s].flags & W2_UF_UNDERCONST) && t->food > 0) supply += t->food;
		} else {
			units++;
			if (t->food < 0) used -= t->food;
		}
	}
	p = &w2.player[player];
	p->food_supply = (int16_t)supply; p->food_used = (int16_t)used;
	p->unit_count = (uint16_t)units; p->building_count = (uint16_t)bld;
}

int econ_food_free(int player)
{
	int s, reserved = 0;
	econ_recount(player);
	for (s = 0; s < w2.unit_hi; s++) {
		const w2_unit_type *t;
		if (!alive(s) || w2.unit[s].owner != player || w2.unit[s].order != W2_ORD_TRAIN) continue;
		t = w2_unit(w2.unit[s].queue_type);
		if (t && t->food < 0) reserved -= t->food;
	}
	return w2.player[player].food_supply - w2.player[player].food_used - reserved;
}

/* ------------------------------------------------------------------------------------------- */
/* upgrades and dependencies                                                                    */

static int researched(int player, int u)
{
	const W2Player *p;
	if (player < 0 || player >= W2_MAX_PLAYERS || u < 0 || u >= (int)W2_NUM_UPGRADES) return 0;
	p = &w2.player[player];
	if (u < 32) return (p->upgrades >> u) & 1u;
	return p->upgrade_level[u - 32] != 0;
}

static void set_researched(int player, int u)
{
	W2Player *p = &w2.player[player];
	if (u < 32) p->upgrades |= 1ul << u;
	else p->upgrade_level[u - 32] = 1;
}

int econ_has_upgrade(int player, int upg)
{
	if (upg < 0 || upg >= (int)W2_NUM_UPGRADES) return 0;
	if (w2_upgrades[upg].researched_at == W2_NONE) return 1;   /* free spells */
	return researched(player, upg);
}

int econ_research_in_progress(int player, int upg)
{
	int s;
	for (s = 0; s < w2.unit_hi; s++)
		if (alive(s) && w2.unit[s].owner == player && w2.unit[s].order == W2_ORD_RESEARCH &&
		    w2.unit[s].queue_type == upg) return 1;
	return 0;
}

static void owned_refresh(void)
{
	int s;
	if (g_owned_stamp == w2.cycle + 1u) return;
	memset(g_owned, 0, sizeof g_owned);
	for (s = 0; s < w2.unit_hi; s++) {
		const W2Unit *u = &w2.unit[s];
		if (!alive(s) || u->owner >= 8 || (u->flags & W2_UF_UNDERCONST)) continue;
		if (u->type >= W2_NUM_UNIT_TYPES) continue;
		g_owned[u->owner][u->type >> 5] |= 1ul << (u->type & 31);
	}
	g_owned_stamp = w2.cycle + 1u;
}

static int owns(int player, int type)
{
	if (player < 0 || player >= 8 || type < 0 || type >= (int)W2_NUM_UNIT_TYPES) return 0;
	owned_refresh();
	return (g_owned[player][type >> 5] >> (type & 31)) & 1u;
}

/* Ownership of a required building, counting higher hall tiers (a castle is also a keep, a keep
 * also a town hall). OR alternatives come from the alt groups of w2_dependencies. */
static int req_unit_met(int player, int req)
{
	if (owns(player, req)) return 1;
	switch (req) {
	case W2U_TOWN_HALL:  return owns(player, W2U_KEEP) || owns(player, W2U_CASTLE);
	case W2U_GREAT_HALL: return owns(player, W2U_STRONGHOLD) || owns(player, W2U_FORTRESS);
	case W2U_KEEP:       return owns(player, W2U_CASTLE);
	case W2U_STRONGHOLD: return owns(player, W2U_FORTRESS);
	}
	return 0;
}

static int deps_met(int player, int kind, int id)
{
	unsigned i, has = 0, fail = 0;
	for (i = 0; i < w2_num_dependencies; i++) {
		const w2_dependency *d = &w2_dependencies[i];
		unsigned bit;
		int ok;
		if (d->kind != kind || d->id != id) continue;
		bit = 1u << (d->alt & 15);
		has |= bit;
		if (d->req_kind == W2_DEP_UPGRADE) ok = econ_has_upgrade(player, d->req_id);
		else ok = req_unit_met(player, d->req_id);
		if (!ok) fail |= bit;
	}
	if (!has) return 1;
	return (has & ~fail) != 0;
}

int econ_allowed_unit(int player, int type)
{
	if (!w2_unit((unsigned)type)) return 0;
	return deps_met(player, W2_DEP_UNIT, type);
}

int econ_allowed_upgrade(int player, int upg)
{
	if (upg < 0 || upg >= (int)W2_NUM_UPGRADES) return 0;
	return deps_met(player, W2_DEP_UPGRADE, upg);
}

/* the unit type's command card has this action with this argument (any level, original buttons) */
static int has_button(int type, int action, int arg)
{
	unsigned n, i;
	const w2_button *b = w2_unit_buttons((unsigned)type, &n);
	for (i = 0; b && i < n; i++)
		if (b[i].action == action && b[i].arg == arg && !(b[i].bflags & W2BF_WARGUS_EXTRA)) return 1;
	return 0;
}

/* ------------------------------------------------------------------------------------------- */
/* stats with upgrades                                                                          */

static void stat_cache(int p)
{
	uint32_t lo = w2.player[p].upgrades, hi = 0;
	unsigned i;
	for (i = 0; i < W2_NUM_UPGRADES - 32; i++) if (w2.player[p].upgrade_level[i]) hi |= 1ul << i;
	if (g_key_ok[p] && g_key_lo[p] == lo && g_key_hi[p] == hi) return;
	memset(g_bonus[p], 0, sizeof g_bonus[p]);
	memset(g_conv[p], 0xFF, sizeof g_conv[p]);
	for (i = 0; i < w2_num_modifiers; i++) {
		const w2_modifier *m = &w2_modifiers[i];
		if (m->unit >= W2_NUM_UNIT_TYPES || !researched(p, m->upgrade)) continue;
		if (m->stat < NSTAT) g_bonus[p][m->unit][m->stat] = (int8_t)(g_bonus[p][m->unit][m->stat] + m->value);
		else if (m->stat == W2_STAT_CONVERT) g_conv[p][m->unit] = (uint8_t)m->value;
	}
	g_key_lo[p] = lo; g_key_hi[p] = hi; g_key_ok[p] = 1;
}

int econ_stat(int slot, int stat)
{
	const W2Unit *u;
	const w2_unit_type *t;
	int v;
	if (slot < 0 || slot >= W2_MAX_UNITS) return 0;
	u = &w2.unit[slot];
	t = w2_unit(u->type);
	if (!t) return 0;
	switch (stat) {
	case W2_STAT_PIERCING: v = t->piercing_damage; break;
	case W2_STAT_ARMOR:    v = t->armor; break;
	case W2_STAT_SIGHT:    v = t->sight; break;
	case W2_STAT_RANGE:    v = t->max_range; break;
	case W2_STAT_REGEN:    v = 0; break;
	case W2_STAT_CONVERT:  v = u->type; break;
	default: return 0;
	}
	if (u->owner >= 8) return v;
	stat_cache(u->owner);
	if (stat < NSTAT) return v + g_bonus[u->owner][u->type][stat];
	if (stat == W2_STAT_CONVERT && g_conv[u->owner][u->type] != 0xFF) return g_conv[u->owner][u->type];
	return v;
}

/* change a unit into another type in place (upgrade to keep, archer -> ranger) */
static void morph(int s, int type)
{
	W2Unit *u = &w2.unit[s];
	const w2_unit_type *o = T(s), *n = w2_unit((unsigned)type);
	int hp;
	if (!n) return;
	hp = (int)u->hp + (int)n->hp - (int)o->hp;
	if (hp < 1) hp = 1;
	if (hp > n->hp) hp = n->hp;
	u->type = (uint8_t)type;
	u->hp = (uint16_t)hp;
}

/* ------------------------------------------------------------------------------------------- */
/* trees                                                                                        */

static int special_at(int i)
{
	int sp = g_special[i];
	if (sp && w2.map.mega[i] != k_special_mega[sp]) { g_special[i] = 0; sp = 0; }  /* redrawn by someone */
	return sp;
}

/* which corners of tile hold trees: 1 bottom-left 2 bottom-right 4 top-right 8 top-left,
 * +16 bottom one-tree piece, +32 top one-tree piece (+48 mid) */
static const uint8_t k_mixed_mask[16] = { 8, 4, 12, 1, 9, 5, 13, 2, 10, 6, 14, 3, 11, 7, 0, 0 };

static int tree_mask(int x, int y)
{
	int i; unsigned v;
	if (!in_map(x, y)) return 15;       /* off the map counts as forest */
	i = midx(x, y);
	switch (special_at(i)) {
	case SP_BOT: return 12 + 16;
	case SP_TOP: return 3 + 32;
	case SP_MID: return 15 + 48;
	case SP_STUMP: return 0;
	}
	if (!(w2.map.flags[i] & W2_TF_TREE)) return 0;
	v = w2.map.tile[i];
	if ((v & 0xFFF0u) == TILE_FOREST) return 15;
	if ((v & 0xFF00u) == 0x0700u) return k_mixed_mask[(v >> 4) & 15];
	return 0;
}

/* replacement by surrounding: >0 MTXM value, <0 -special piece, 0 = no tree can stay */
static const int16_t k_wood[20] = {
	0, 0x0730, 0x0770, 0x07B0, 0x0710, 0x0750, 0x0790, 0x07D0, 0x0700, 0x0740,
	0x0780, 0x07C0, 0x0720, 0x0760, 0x07A0, 0x0070, 0, -SP_BOT, -SP_TOP, -SP_MID
};

static int wood_by_surrounding(int up, int right, int down, int left)
{
	int t = 0, r;
	t += ((up & 1) && (left & 4)) * 8;
	t += ((up & 2) && (right & 8)) * 4;
	t += ((right & 1) && (down & 4)) * 2;
	t += ((left & 2) && (down & 8)) * 1;
	if (down & 0x10) { t |= (left & 6) ? 1 : 0; t |= (right & 9) ? 2 : 0; }
	if (up & 0x20)   { t |= (left & 6) ? 8 : 0; t |= (right & 9) ? 4 : 0; }
	r = k_wood[t];
	if (r == 0) {
		t = 16 + (((up & 1) || (up & 2)) ? 1 : 0) + (((down & 4) || (down & 8)) ? 2 : 0);
		r = k_wood[t];
	}
	return r;
}

static void set_tile(int x, int y, int code)
{
	int i = midx(x, y);
	if (code > 0) {
		w2.map.tile[i] = (uint16_t)code;
		g_special[i] = SP_NONE;
		pud_reclassify_tile(x, y);
	} else {
		int sp = code < 0 ? -code : SP_STUMP;
		w2.map.tile[i] = (uint16_t)(sp == SP_STUMP ? TILE_GRASS : TILE_FOREST);
		pud_reclassify_tile(x, y);
		w2.map.mega[i] = k_special_mega[sp];
		g_special[i] = (uint8_t)sp;
		if (sp == SP_STUMP) w2.map.wood[i] = 0;
	}
}

static int code_mask(int code)
{
	switch (code) {
	case -SP_BOT: return 12 + 16;
	case -SP_TOP: return 3 + 32;
	case -SP_MID: return 15 + 48;
	}
	if (code == (int)TILE_FOREST) return 15;
	if (code > 0) return k_mixed_mask[(code >> 4) & 15];
	return 0;
}

static void fix_tile(int x, int y)
{
	int cur, code;
	if (!in_map(x, y) || !(w2.map.flags[midx(x, y)] & W2_TF_TREE)) return;
	cur = tree_mask(x, y);
	code = wood_by_surrounding(tree_mask(x, y - 1), tree_mask(x + 1, y), tree_mask(x, y + 1), tree_mask(x - 1, y));
	if (code != 0 && code_mask(code) == cur) return;      /* same shape: keep its variant */
	set_tile(x, y, code);
}

void econ_clear_tree(int tx, int ty)
{
	static const int8_t off[8][2] = { {1,0}, {-1,0}, {0,1}, {0,-1}, {-1,-1}, {-1,1}, {1,-1}, {1,1} };
	int k;
	check_reset();
	if (!in_map(tx, ty) || !(w2.map.flags[midx(tx, ty)] & W2_TF_TREE)) return;
	set_tile(tx, ty, 0);
	for (k = 0; k < 8; k++) fix_tile(tx + off[k][0], ty + off[k][1]);
}

/* a tree with at least one walkable neighbour */
static int tree_ok(int x, int y)
{
	int dx, dy;
	if (!in_map(x, y) || !(w2.map.flags[midx(x, y)] & W2_TF_TREE)) return 0;
	for (dy = -1; dy <= 1; dy++)
		for (dx = -1; dx <= 1; dx++) {
			int nx = x + dx, ny = y + dy;
			if ((dx || dy) && in_map(nx, ny) &&
			    !(w2.map.flags[midx(nx, ny)] & (W2_TF_NOWALK | W2_TF_BUILDING))) return 1;
		}
	return 0;
}

int econ_find_tree(int tx, int ty, int radius, int *ox, int *oy)
{
	int r, x, y, found = 0;
	long bd = 0x7FFFFFFFL;
	for (r = 0; r <= radius && !found; r++) {
		int x0 = tx - r, y0 = ty - r, x1 = tx + r, y1 = ty + r;
		for (y = y0; y <= y1; y++) {
			int step = (y == y0 || y == y1) ? 1 : (x1 - x0);
			if (step <= 0) step = 1;
			for (x = x0; x <= x1; x += step) {
				long dx, dy, d;
				if (!tree_ok(x, y)) continue;
				dx = x - tx; dy = y - ty; d = dx * dx + dy * dy;
				if (d < bd) { bd = d; *ox = x; *oy = y; found = 1; }
			}
		}
	}
	return found;
}

/* ------------------------------------------------------------------------------------------- */
/* resources and depots                                                                         */

static int is_mine(int s) { return alive(s) && (T(s)->flags & W2UF_GOLD_MINE); }

static int is_platform_of(int s, int player)
{
	return alive(s) && (T(s)->flags & W2UF_OIL_PLATFORM) && w2.unit[s].owner == player &&
	       !(w2.unit[s].flags & W2_UF_UNDERCONST);
}

static int depot_ok(int s, int player, int kind)
{
	if (!alive(s) || w2.unit[s].owner != player || kind < W2_RES_GOLD || kind > W2_RES_OIL) return 0;
	if (!(w2.unit[s].flags & W2_UF_BUILDING) || (w2.unit[s].flags & W2_UF_UNDERCONST)) return 0;
	return (T(s)->store & (1u << (kind - 1))) != 0;
}

int econ_find_depot(int slot, int kind)
{
	int s, best = -1, bd = 0x7FFF;
	const W2Unit *u = &w2.unit[slot];
	for (s = 0; s < w2.unit_hi; s++) {
		int d;
		if (!depot_ok(s, u->owner, kind)) continue;
		d = unit_gap(slot, s);
		if (d < bd) { bd = d; best = s; }
	}
	return best;
}

int econ_mine_workers(int mine) { return (mine >= 0 && mine < W2_MAX_UNITS) ? g_inside[mine] : 0; }

static int bonus_pct(int player, int kind)
{
	int s, best = 0;
	for (s = 0; s < w2.unit_hi; s++) {
		const w2_unit_type *t;
		if (!alive(s) || w2.unit[s].owner != player || (w2.unit[s].flags & W2_UF_UNDERCONST)) continue;
		t = T(s);
		if (t->improve_res == kind && t->improve_pct > best) best = t->improve_pct;
	}
	return best;
}

static void centre_tile(int s, int *x, int *y)
{
	const w2_unit_type *t = T(s);
	*x = w2.unit[s].tx + (t->tile_w >> 1);
	*y = w2.unit[s].ty + (t->tile_h >> 1);
}

static void mine_enter(int m)
{
	if (g_inside[m] < 255) g_inside[m]++;
	if (g_inside[m] == 1) set_anim(m, W2_ANIM_ACTIVE);
}

static void mine_leave(int m)
{
	if (g_inside[m]) g_inside[m]--;
	if (!g_inside[m] && alive(m)) set_anim(m, W2_ANIM_STILL);
}

/* ------------------------------------------------------------------------------------------- */
/* harvesting                                                                                   */

enum { HV_START, HV_GO, HV_INSIDE, HV_EXIT, HV_CHOP };
enum { RT_START, RT_GO, RT_INSIDE, RT_EXIT };

static int harvest_kind_of(int target, int player)
{
	if (target == W2_NO_UNIT || target < 0) return W2_RES_WOOD;
	if (is_mine(target)) return W2_RES_GOLD;
	if (is_platform_of(target, player)) return W2_RES_OIL;
	return W2_RES_NONE;
}

void cmd_harvest(int slot, int target, int tx, int ty)
{
	W2Unit *u;
	int kind;
	check_reset();
	if (!alive(slot)) return;
	u = &w2.unit[slot];
	if (target < 0) target = W2_NO_UNIT;
	kind = harvest_kind_of(target, u->owner);
	if (!kind || !w2_unit_harvest(u->type, kind)) return;
	if (kind == W2_RES_WOOD && !in_map(tx, ty)) return;
	unit_set_order(slot, W2_ORD_HARVEST, target, kind == W2_RES_WOOD ? tx : 0, kind == W2_RES_WOOD ? ty : 0);
	g_chop[slot] = 0;
	g_fails[slot] = 0;
}

/* worker loaded: remember the source and go unload */
static void to_return(int s)
{
	unit_set_order(s, W2_ORD_RETURN, W2_NO_UNIT, 0, 0);
	set_anim(s, W2_ANIM_STILL);
}

static void mine_collapse(int m, int player)
{
	if (is_local(player)) ui_message("Gold mine depleted.");
	g_inside[m] = 0;
	unit_remove(m);
}

void ord_harvest(int s)
{
	W2Unit *u = &w2.unit[s];
	const w2_harvest *h;
	int kind, r;
	check_reset();
	if (!alive(s)) return;

	if (u->order_state == HV_START) {
		kind = harvest_kind_of(u->target, u->owner);
		h = kind ? w2_unit_harvest(u->type, kind) : NULL;
		if (!h) { go_still(s); return; }
		if (u->res_kind && u->res_amount) {
			if (u->res_kind != kind) { u->res_kind = 0; u->res_amount = 0; }   /* drops it */
			else {
				g_mem_kind[s] = (uint8_t)kind; g_mem_target[s] = u->target;
				g_mem_tx[s] = u->goal_tx; g_mem_ty[s] = u->goal_ty;
				to_return(s);
				return;
			}
		}
		if (kind == W2_RES_WOOD && !tree_ok(u->goal_tx, u->goal_ty)) {
			int x, y;
			if (!unit_thinks(s)) return;
			if (!econ_find_tree(u->goal_tx, u->goal_ty, 16, &x, &y)) { go_still(s); return; }
			u->goal_tx = (uint8_t)x; u->goal_ty = (uint8_t)y;
		}
		g_mem_kind[s] = (uint8_t)kind; g_mem_target[s] = u->target;
		g_mem_tx[s] = u->goal_tx; g_mem_ty[s] = u->goal_ty;
		u->order_state = HV_GO;
	}

	kind = g_mem_kind[s];
	h = w2_unit_harvest(u->type, kind);
	if (!h) { go_still(s); return; }

	switch (u->order_state) {
	case HV_GO:
		if (kind == W2_RES_WOOD) {
			if (!tree_ok(u->goal_tx, u->goal_ty)) {      /* someone else cut it: next tree */
				int x, y;
				if (!unit_thinks(s)) return;
				if (!econ_find_tree(u->tx, u->ty, 16, &x, &y)) { go_still(s); return; }
				u->goal_tx = (uint8_t)x; u->goal_ty = (uint8_t)y;
				g_mem_tx[s] = (uint8_t)x; g_mem_ty[s] = (uint8_t)y;
			}
			r = move_toward(s, u->goal_tx, u->goal_ty, 1);
			if (r < 0) {
				int x, y;
				if (++g_fails[s] > 3 || !econ_find_tree(u->tx, u->ty, 16 + g_fails[s] * 4, &x, &y)) { go_still(s); return; }
				u->goal_tx = (uint8_t)x; u->goal_ty = (uint8_t)y;
				return;
			}
			if (r == 0) return;
			g_fails[s] = 0;
			unit_set_dir_to(s, u->goal_tx * W2_TILE + W2_TILE / 2, u->goal_ty * W2_TILE + W2_TILE / 2);
			unit_set_anim(s, W2_ANIM_HARVEST);
			u->timer = h->wait_at_resource;
			u->order_state = HV_CHOP;
			return;
		}
		/* gold / oil */
		if (kind == W2_RES_GOLD ? !is_mine(u->target) : !is_platform_of(u->target, u->owner)) { go_still(s); return; }
		if (w2.unit[u->target].res_left == 0) { go_still(s); return; }
		r = move_toward_unit(s, u->target, 1);
		if (r < 0) { go_still(s); return; }
		if (r == 0) return;
		go_inside(s);
		mine_enter(u->target);
		set_anim(s, W2_ANIM_STILL);
		u->timer = h->wait_at_resource;
		u->order_state = HV_INSIDE;
		return;

	case HV_INSIDE: {
		W2Unit *m;
		uint32_t amount;
		if (u->timer) { u->timer--; return; }
		if (kind == W2_RES_GOLD ? !is_mine(u->target) : !alive(u->target)) {
			/* source vanished while inside: come out empty near where we stand */
			int x = u->tx, y = u->ty;
			if (!exit_near(s, x, y, 1, 1, x, y)) return;
			go_still(s);
			return;
		}
		m = &w2.unit[u->target];
		amount = m->res_left < h->capacity ? m->res_left : h->capacity;
		m->res_left -= amount;
		m->res_amount = (uint16_t)(m->res_left / 100u > 0xFFFFu ? 0xFFFFu : m->res_left / 100u);
		u->res_kind = (uint8_t)kind;
		u->res_amount = (uint16_t)amount;
		u->order_state = HV_EXIT;
	}
	/* fall through */
	case HV_EXIT: {
		int d, px, py, m = u->target;
		if (!alive(m)) { int x = u->tx, y = u->ty; if (!exit_near(s, x, y, 1, 1, x, y)) return; }
		else {
			d = econ_find_depot(s, kind);
			if (d >= 0) centre_tile(d, &px, &py); else { px = u->tx; py = u->ty; }
			if (!exit_unit(s, m, px, py)) return;      /* no room: try again next cycle */
			mine_leave(m);
			if (kind == W2_RES_GOLD && w2.unit[m].res_left == 0 && g_inside[m] == 0) mine_collapse(m, u->owner);
		}
		if (u->res_amount) to_return(s); else go_still(s);
		return;
	}

	case HV_CHOP: {
		int i;
		if (!tree_ok(u->goal_tx, u->goal_ty)) {           /* tree gone under the axe */
			u->order_state = HV_GO;
			set_anim(s, W2_ANIM_STILL);
			return;
		}
		set_anim(s, W2_ANIM_HARVEST);
		if (u->timer) { u->timer--; return; }
		g_chop[s] = (uint16_t)(g_chop[s] + (h->step ? h->step : h->capacity));
		u->timer = h->wait_at_resource;
		if (g_chop[s] < h->capacity) return;
		i = midx(u->goal_tx, u->goal_ty);
		if (w2.map.wood[i]) w2.map.wood[i]--;
		if (!w2.map.wood[i]) econ_clear_tree(u->goal_tx, u->goal_ty);
		g_chop[s] = 0;
		u->res_kind = W2_RES_WOOD;
		u->res_amount = h->capacity;
		to_return(s);
		return;
	}
	default:
		go_still(s);
	}
}

void cmd_return(int slot)
{
	check_reset();
	if (!alive(slot) || !w2.unit[slot].res_kind || !w2.unit[slot].res_amount) return;
	unit_set_order(slot, W2_ORD_RETURN, W2_NO_UNIT, 0, 0);
}

/* after unloading: back to the remembered resource */
static void resume_harvest(int s)
{
	W2Unit *u = &w2.unit[s];
	switch (g_mem_kind[s]) {
	case W2_RES_GOLD:
		if (is_mine(g_mem_target[s])) { unit_set_order(s, W2_ORD_HARVEST, g_mem_target[s], 0, 0); return; }
		break;
	case W2_RES_OIL:
		if (is_platform_of(g_mem_target[s], u->owner)) { unit_set_order(s, W2_ORD_HARVEST, g_mem_target[s], 0, 0); return; }
		break;
	case W2_RES_WOOD:
		unit_set_order(s, W2_ORD_HARVEST, W2_NO_UNIT, g_mem_tx[s], g_mem_ty[s]);
		g_chop[s] = 0;
		return;
	}
	go_still(s);
}

void ord_return(int s)
{
	W2Unit *u = &w2.unit[s];
	const w2_harvest *h;
	int r;
	check_reset();
	if (!alive(s)) return;
	h = w2_unit_harvest(u->type, u->res_kind ? u->res_kind : g_mem_kind[s]);

	switch (u->order_state) {
	case RT_START: {
		int d;
		if (!u->res_kind || !u->res_amount) { resume_harvest(s); return; }
		if (!unit_thinks(s)) return;
		d = econ_find_depot(s, u->res_kind);
		if (d < 0) { msg_idx(u->owner, ECON_STR_NO_RETURN, "Nowhere to return to...cannot return."); go_still(s); return; }
		u->target = (uint16_t)d;
		u->order_state = RT_GO;
	}
	/* fall through */
	case RT_GO:
		if (!depot_ok(u->target, u->owner, u->res_kind)) { u->order_state = RT_START; return; }
		r = move_toward_unit(s, u->target, 1);
		if (r < 0) { go_still(s); return; }
		if (r == 0) return;
		u->goal_tx = w2.unit[u->target].tx; u->goal_ty = w2.unit[u->target].ty;
		u->queue_type = w2.unit[u->target].type;
		go_inside(s);
		set_anim(s, W2_ANIM_STILL);
		u->timer = h ? h->wait_at_depot : 0;
		u->order_state = RT_INSIDE;
		return;

	case RT_INSIDE: {
		W2Player *p = &w2.player[u->owner];
		int32_t amt;
		if (u->timer) { u->timer--; return; }
		amt = (int32_t)u->res_amount;
		amt += amt * bonus_pct(u->owner, u->res_kind) / 100;
		if (u->res_kind == W2_RES_GOLD) p->gold += amt;
		else if (u->res_kind == W2_RES_WOOD) p->wood += amt;
		else if (u->res_kind == W2_RES_OIL) p->oil += amt;
		u->res_kind = 0; u->res_amount = 0;
		u->order_state = RT_EXIT;
	}
	/* fall through */
	case RT_EXIT: {
		int px = u->tx, py = u->ty;
		if (g_mem_kind[s] == W2_RES_WOOD) { px = g_mem_tx[s]; py = g_mem_ty[s]; }
		else if (alive(g_mem_target[s])) centre_tile(g_mem_target[s], &px, &py);
		if (alive(u->target) && w2.unit[u->target].tx == u->goal_tx && w2.unit[u->target].ty == u->goal_ty) {
			if (!exit_unit(s, u->target, px, py)) return;
		} else {
			const w2_unit_type *t = w2_unit(u->queue_type);
			if (!exit_near(s, u->goal_tx, u->goal_ty, t ? t->tile_w : 1, t ? t->tile_h : 1, px, py)) return;
		}
		resume_harvest(s);
		return;
	}
	default:
		go_still(s);
	}
}

/* ------------------------------------------------------------------------------------------- */
/* placement                                                                                    */

/* terrain class of a tile from its MTXM value (layout: docs/DATA_FORMAT.md section 8) */
enum { TC_OTHER, TC_WATER, TC_SHORE, TC_COAST, TC_GRASS };
static int tile_class(unsigned v)
{
	unsigned k;
	if (v < 0x100u) {
		k = (v >> 4) & 15;
		if (k == 1 || k == 2) return TC_WATER;
		if (k == 3 || k == 4) return TC_COAST;
		if (k == 5 || k == 6) return TC_GRASS;
		return TC_OTHER;
	}
	k = (v >> 8) & 15;
	if (k == 1) return TC_WATER;     /* dark water / water */
	if (k == 2) return TC_SHORE;     /* water / coast boundary */
	if (k == 3 || k == 5) return TC_COAST;
	if (k == 6) return TC_GRASS;     /* dark grass / grass */
	return TC_OTHER;
}

/* Mac rule (placement check at 0x130B2A): per-axis rectangle margin. On each axis the gap is the
 * number of empty tiles between the two footprints (negative when they overlap); the pair is too
 * near only when BOTH gaps are below the margin (town hall vs gold mine 3, shore building vs oil
 * patch / platform 4). */
static int too_near(int ax, int ay, int aw, int ah, int b, int margin)
{
	const w2_unit_type *t = T(b);
	int bx = w2.unit[b].tx, by = w2.unit[b].ty;
	int gx = bx < ax ? ax - (bx + t->tile_w) : bx - (ax + aw);
	int gy = by < ay ? ay - (by + t->tile_h) : by - (ay + ah);
	return gx < margin && gy < margin;
}

static int oil_patch_at(int tx, int ty)
{
	int s;
	for (s = 0; s < w2.unit_hi; s++)
		if (alive(s) && (T(s)->flags & W2UF_OIL_PATCH) && w2.unit[s].tx == tx && w2.unit[s].ty == ty) return s;
	return -1;
}

int econ_place_reason(int player, int type, int tx, int ty, int builder)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	int w, h, x, y, s, shore, need_explore, coastal = 0;
	if (!t || t->kind != W2_KIND_BUILDING) return 391;
	w = t->tile_w; h = t->tile_h;
	if (tx < 0 || ty < 0 || tx + w > w2.map.w || ty + h > w2.map.h) return 390;
	need_explore = player >= 0 && player < 8 && w2.player[player].control == W2_CTRL_HUMAN;
	if (need_explore)
		for (y = ty; y < ty + h; y++)
			for (x = tx; x < tx + w; x++)
				if (!(w2.map.explored[midx(x, y)] & (1u << player))) return 393;
	if (t->flags & W2UF_OIL_PLATFORM) return oil_patch_at(tx, ty) >= 0 ? 0 : 394;

	shore = (t->flags & W2UF_SHORE_BUILDING) != 0;
	for (y = ty; y < ty + h; y++)
		for (x = tx; x < tx + w; x++) {
			int i = midx(x, y), c = tile_class(w2.map.tile[i]);
			uint16_t f = w2.map.flags[i];
			/* terrain kind from the MTXM value (tile_class) - the static flags of water/coast
			 * boundary tiles carry ROCK/NOWALK bits, so they are not used for the terrain test */
			if (f & W2_TF_BUILDING) return 391;
			if (w2.map.occ_ground[i] != W2_NO_UNIT && w2.map.occ_ground[i] != builder) return 391;
			if (shore) {
				if (c == TC_SHORE) coastal = 1;
				else if (c != TC_WATER) return 392;
			} else {
				if (c != TC_GRASS || (f & (W2_TF_NOWALK | W2_TF_NOBUILD))) return 391;
			}
		}
	if (shore && !coastal) return 392;

	/* Mac: the town hall tiers (STORE_HALL flag) keep 3 tiles from gold mines; every shore building
	 * (shipyard, refinery) keeps 4 from oil patches and platforms, the foundry excepted */
	if (t->flags & W2UF_STORE_HALL) {
		for (s = 0; s < w2.unit_hi; s++)
			if (alive(s) && (T(s)->flags & W2UF_GOLD_MINE) && too_near(tx, ty, w, h, s, 3)) return 395;
	} else if (shore && type != W2U_HUMAN_FOUNDRY && type != W2U_ORC_FOUNDRY) {
		for (s = 0; s < w2.unit_hi; s++)
			if (alive(s) && (T(s)->flags & (W2UF_OIL_PATCH | W2UF_OIL_PLATFORM)) && too_near(tx, ty, w, h, s, 4)) return 396;
	}
	return 0;
}

int econ_place_ok(int player, int type, int tx, int ty, int builder)
{
	check_reset();
	return econ_place_reason(player, type, tx, ty, builder) == 0;
}

static const char *place_fallback(int r)
{
	switch (r) {
	case 390: return "You cannot build off the map.";
	case 392: return "You must build this building on the coast.";
	case 393: return "You must explore there first.";
	case 394: return "You must build an oil platform over a patch of oil.";
	case 395: return "You cannot build a townhall too near a goldmine.";
	case 396: return "You cannot build too near a patch of oil.";
	}
	return "You cannot build there.";
}

/* ------------------------------------------------------------------------------------------- */
/* building                                                                                     */

enum { BD_GO, BD_START, BD_INSIDE };

void cmd_build(int slot, int type, int tx, int ty)
{
	W2Unit *u;
	const w2_unit_type *t = w2_unit((unsigned)type);
	int r;
	check_reset();
	if (!alive(slot) || !t) return;
	u = &w2.unit[slot];
	if (!has_button(u->type, W2B_BUILD, type) || !econ_allowed_unit(u->owner, type)) return;
	r = econ_place_reason(u->owner, type, tx, ty, slot);
	if (r) { msg_idx(u->owner, r, place_fallback(r)); return; }
	if (!econ_can_afford(u->owner, t->gold, t->wood, t->oil)) { msg_cost(u->owner, t->gold, t->wood, t->oil); return; }
	unit_set_order(slot, W2_ORD_BUILD, W2_NO_UNIT, tx, ty);
	u->queue_type = (uint8_t)type;
	u->timer = 0;
}

void ord_build(int s)
{
	W2Unit *u = &w2.unit[s];
	const w2_unit_type *t = w2_unit(u->queue_type);
	int r, outside;
	check_reset();
	if (!alive(s)) return;
	if (!t) { go_still(s); return; }
	outside = (t->xflags & W2UX_BUILDER_OUTSIDE) != 0;

	switch (u->order_state) {
	case BD_GO: {
		int cx = u->goal_tx + ((t->tile_w - 1) >> 1), cy = u->goal_ty + ((t->tile_h - 1) >> 1);
		int range = (((t->tile_w > t->tile_h) ? t->tile_w : t->tile_h) + 1) >> 1;
		if (t->flags & W2UF_OIL_PLATFORM) {
			int p = oil_patch_at(u->goal_tx, u->goal_ty);
			if (p < 0) { msg_idx(u->owner, 394, place_fallback(394)); go_still(s); return; }
			r = move_toward_unit(s, p, 1);
		} else {
			if (is_sea(s)) range++;
			r = move_toward(s, cx, cy, range);
		}
		if (r < 0) { go_still(s); return; }
		if (r == 0) return;
		u->order_state = BD_START;
		u->timer = 0;
	}
	/* fall through */
	case BD_START: {
		int b, reason, left = 0;
		reason = econ_place_reason(u->owner, u->queue_type, u->goal_tx, u->goal_ty, s);
		if (reason == 391 && u->timer < 60) { u->timer++; return; }   /* units passing through */
		if (reason) { msg_idx(u->owner, reason, place_fallback(reason)); go_still(s); return; }
		if (!econ_can_afford(u->owner, t->gold, t->wood, t->oil)) {
			msg_cost(u->owner, t->gold, t->wood, t->oil);
			go_still(s);
			return;
		}
		if (t->flags & W2UF_OIL_PLATFORM) {
			int p = oil_patch_at(u->goal_tx, u->goal_ty);
			if (p < 0) { go_still(s); return; }
			left = (int)w2.unit[p].res_left;
			unit_remove(p);
		}
		if (!outside) go_inside(s);
		b = unit_create(u->queue_type, u->owner, u->goal_tx, u->goal_ty);
		if (b < 0) {
			if (u->flags & W2_UF_INSIDE) { u->flags &= (uint16_t)~W2_UF_INSIDE; unit_place(s, u->tx, u->ty); }
			if (t->flags & W2UF_OIL_PLATFORM) {
				int p = unit_create(W2U_OIL_PATCH, W2_NEUTRAL, u->goal_tx, u->goal_ty);
				if (p >= 0) w2.unit[p].res_left = (uint32_t)left;
			}
			msg_idx(u->owner, 391, place_fallback(391));
			go_still(s);
			return;
		}
		econ_pay(u->owner, t->gold, t->wood, t->oil);
		{
			W2Unit *bu = &w2.unit[b];
			bu->flags |= W2_UF_UNDERCONST;
			bu->hp = 1;
			unit_set_order(b, W2_ORD_BUILT, W2_NO_UNIT, 0, 0);
			bu->progress = 0;
			bu->queue_type = u->queue_type;
			if (t->flags & W2UF_OIL_PLATFORM) {
				bu->res_left = (uint32_t)left;
				bu->res_amount = (uint16_t)(left / 100);
			}
		}
		g_builder[b] = (uint16_t)s;
		u->target = (uint16_t)b;
		u->order_state = BD_INSIDE;
		set_anim(s, outside ? W2_ANIM_REPAIR : W2_ANIM_STILL);
		g_owned_stamp = 0;
		econ_recount(u->owner);
		return;
	}
	case BD_INSIDE: {
		int b = u->target;
		int done = !alive(b) || g_builder[b] != s || !(w2.unit[b].flags & W2_UF_UNDERCONST);
		if (!done) {
			if (outside) set_anim(s, W2_ANIM_REPAIR);
			return;
		}
		if (alive(b)) { if (!exit_unit(s, b, u->goal_tx + t->tile_w / 2, u->goal_ty + t->tile_h)) return; }
		else if (!exit_near(s, u->goal_tx, u->goal_ty, t->tile_w, t->tile_h, u->goal_tx, u->goal_ty + t->tile_h)) return;
		if (alive(b) && g_builder[b] == s) g_builder[b] = W2_NO_UNIT;
		go_still(s);
		return;
	}
	default:
		go_still(s);
	}
}

void ord_built(int b)
{
	W2Unit *u = &w2.unit[b];
	const w2_unit_type *t;
	uint32_t total, prev, mx;
	check_reset();
	if (!alive(b)) return;
	if (!(u->flags & W2_UF_UNDERCONST)) { go_still(b); return; }
	t = T(b);
	total = w2_time_to_cycles(t->build_time);
	if (total < 1) total = 1;
	prev = u->progress;
	if (u->progress < 0xFFFF) u->progress++;
	mx = t->hp;
	{
		/* hp grows with progress; damage taken meanwhile stays taken */
		uint32_t inc = mx * u->progress / total - mx * prev / total;
		uint32_t hp = u->hp + inc;
		u->hp = (uint16_t)(hp > mx ? mx : hp);
	}
	if (u->progress < total) return;
	u->flags &= (uint16_t)~W2_UF_UNDERCONST;
	u->progress = 0;
	unit_set_order(b, W2_ORD_STILL, W2_NO_UNIT, 0, 0);
	unit_set_anim(b, W2_ANIM_STILL);
	g_owned_stamp = 0;
	econ_recount(u->owner);
	snd_unit(b, 2);
	/* the worker inside notices in ord_build and comes out */
}

/* ------------------------------------------------------------------------------------------- */
/* training, research, upgrades                                                                 */

static int building_idle(int b)
{
	const W2Unit *u = &w2.unit[b];
	return alive(b) && (u->flags & W2_UF_BUILDING) && !(u->flags & W2_UF_UNDERCONST) &&
	       (u->order == W2_ORD_NONE || u->order == W2_ORD_STILL || u->order == W2_ORD_STAND);
}

void cmd_train(int bslot, int type)
{
	W2Unit *u;
	const w2_unit_type *t = w2_unit((unsigned)type);
	check_reset();
	if (!t || !building_idle(bslot)) return;
	u = &w2.unit[bslot];
	if (!has_button(u->type, W2B_TRAIN_UNIT, type) || !econ_allowed_unit(u->owner, type)) return;
	if (t->food < 0 && econ_food_free(u->owner) < -t->food) {
		msg_idx(u->owner, ECON_STR_FOOD, "Not enough food...build more farms.");
		return;
	}
	if (!econ_can_afford(u->owner, t->gold, t->wood, t->oil)) { msg_cost(u->owner, t->gold, t->wood, t->oil); return; }
	econ_pay(u->owner, t->gold, t->wood, t->oil);
	unit_set_order(bslot, W2_ORD_TRAIN, W2_NO_UNIT, 0, 0);
	u->queue_type = (uint8_t)type;
	u->progress = 0;
	unit_set_anim(bslot, W2_ANIM_TRAIN);
}

void ord_train(int b)
{
	W2Unit *u = &w2.unit[b];
	const w2_unit_type *t = w2_unit(u->queue_type), *bt;
	uint32_t total;
	int x, y, n;
	check_reset();
	if (!alive(b)) return;
	if (!t) { go_still(b); return; }
	total = w2_time_to_cycles(t->build_time);
	if (u->progress < total) { u->progress++; return; }
	if (!unit_thinks(b)) return;
	bt = T(b);
	/* spawn at the free tile nearest to the bottom middle of the building. Mac (0x1240B2): when no
	 * tile is free or the unit pool is full the production ends and the full cost comes back. */
	n = -1;
	if (find_spot(t->kind, u->tx, u->ty, bt->tile_w, bt->tile_h, u->tx + bt->tile_w / 2, u->ty + bt->tile_h, &x, &y))
		n = unit_create(u->queue_type, u->owner, x, y);
	if (n < 0) refund_pct(u->owner, t->gold, t->wood, t->oil, 100);
	unit_set_order(b, W2_ORD_STILL, W2_NO_UNIT, 0, 0);
	u->progress = 0;
	unit_set_anim(b, W2_ANIM_STILL);
	g_owned_stamp = 0;
	econ_recount(u->owner);
	if (n >= 0) snd_unit(n, 2);
}

void cmd_research(int bslot, int upg)
{
	W2Unit *u;
	const w2_upgrade *g;
	check_reset();
	if (upg < 0 || upg >= (int)W2_NUM_UPGRADES || !building_idle(bslot)) return;
	u = &w2.unit[bslot];
	g = &w2_upgrades[upg];
	if (!has_button(u->type, W2B_RESEARCH, upg) || econ_has_upgrade(u->owner, upg) ||
	    econ_research_in_progress(u->owner, upg) || !econ_allowed_upgrade(u->owner, upg)) return;
	if (!econ_can_afford(u->owner, g->gold, g->wood, g->oil)) { msg_cost(u->owner, g->gold, g->wood, g->oil); return; }
	econ_pay(u->owner, g->gold, g->wood, g->oil);
	unit_set_order(bslot, W2_ORD_RESEARCH, W2_NO_UNIT, 0, 0);
	u->queue_type = (uint8_t)upg;
	u->progress = 0;
	unit_set_anim(bslot, W2_ANIM_RESEARCH);
}

void ord_research(int b)
{
	W2Unit *u = &w2.unit[b];
	int upg = u->queue_type, s, p = u->owner;
	unsigned i;
	check_reset();
	if (!alive(b)) return;
	if (upg >= (int)W2_NUM_UPGRADES) { go_still(b); return; }
	if (u->progress < w2_time_to_cycles(w2_upgrades[upg].time)) { u->progress++; return; }
	set_researched(p, upg);
	/* conversions (ranger / berserker training, paladin / ogre-mage): existing units change */
	for (i = 0; i < w2_num_modifiers; i++) {
		const w2_modifier *m = &w2_modifiers[i];
		if (m->upgrade != upg || m->stat != W2_STAT_CONVERT) continue;
		for (s = 0; s < w2.unit_hi; s++)
			if (alive(s) && w2.unit[s].owner == p && w2.unit[s].type == m->unit) morph(s, (uint8_t)m->value);
	}
	unit_set_order(b, W2_ORD_STILL, W2_NO_UNIT, 0, 0);
	u->progress = 0;
	unit_set_anim(b, W2_ANIM_STILL);
	g_owned_stamp = 0;
	snd_unit(b, 2);
}

void cmd_upgrade_to(int bslot, int type)
{
	W2Unit *u;
	const w2_unit_type *t = w2_unit((unsigned)type);
	check_reset();
	if (!t || !building_idle(bslot)) return;
	u = &w2.unit[bslot];
	if (t->upgrade_from != u->type && !has_button(u->type, W2B_UPGRADE_TO, type)) return;
	if (!econ_allowed_unit(u->owner, type)) return;
	if (!econ_can_afford(u->owner, t->gold, t->wood, t->oil)) { msg_cost(u->owner, t->gold, t->wood, t->oil); return; }
	econ_pay(u->owner, t->gold, t->wood, t->oil);
	unit_set_order(bslot, W2_ORD_UPGRADE_TO, W2_NO_UNIT, 0, 0);
	u->queue_type = (uint8_t)type;
	u->progress = 0;
	unit_set_anim(bslot, W2_ANIM_UPGRADE);
}

void ord_upgrade_to(int b)
{
	W2Unit *u = &w2.unit[b];
	const w2_unit_type *t = w2_unit(u->queue_type);
	check_reset();
	if (!alive(b)) return;
	if (!t) { go_still(b); return; }
	if (u->progress < w2_time_to_cycles(t->build_time)) { u->progress++; return; }
	morph(b, u->queue_type);
	unit_set_order(b, W2_ORD_STILL, W2_NO_UNIT, 0, 0);
	u->progress = 0;
	unit_set_anim(b, W2_ANIM_STILL);
	g_owned_stamp = 0;
	econ_recount(u->owner);
	snd_unit(b, 2);
}

int econ_progress_percent(int slot)
{
	const W2Unit *u;
	uint32_t total = 0;
	if (!alive(slot)) return -1;
	u = &w2.unit[slot];
	if (u->flags & W2_UF_UNDERCONST) total = w2_time_to_cycles(T(slot)->build_time);
	else if (u->order == W2_ORD_TRAIN || u->order == W2_ORD_UPGRADE_TO) {
		const w2_unit_type *t = w2_unit(u->queue_type);
		if (t) total = w2_time_to_cycles(t->build_time);
	} else if (u->order == W2_ORD_RESEARCH && u->queue_type < W2_NUM_UPGRADES)
		total = w2_time_to_cycles(w2_upgrades[u->queue_type].time);
	else return -1;
	if (total < 1) return 100;
	return u->progress >= total ? 100 : (int)(u->progress * 100u / total);
}

void cmd_cancel(int b)
{
	W2Unit *u;
	check_reset();
	if (!alive(b)) return;
	u = &w2.unit[b];
	if (!(u->flags & W2_UF_BUILDING)) {
		if (u->order == W2_ORD_BUILD && u->order_state != BD_INSIDE) go_still(b);   /* nothing paid yet */
		return;
	}
	if (u->flags & W2_UF_UNDERCONST) {
		const w2_unit_type *t = T(b);
		int tx = u->tx, ty = u->ty, plat = (t->flags & W2UF_OIL_PLATFORM) != 0, owner = u->owner;
		uint32_t left = u->res_left;
		refund_pct(u->owner, t->gold, t->wood, 0, 75);   /* Mac 0x16C452: 3/4 of gold and wood, no oil */
		g_builder[b] = W2_NO_UNIT;
		unit_remove(b);
		if (plat) {
			int p = unit_create(W2U_OIL_PATCH, W2_NEUTRAL, tx, ty);
			if (p >= 0) { w2.unit[p].res_left = left; w2.unit[p].res_amount = (uint16_t)(left / 100u); }
		}
		/* the builder (still waiting in BD_INSIDE) sees the site gone and walks out */
		g_owned_stamp = 0;
		econ_recount(owner);                  /* u's slot is free now: its owner field is stale */
		return;
	}
	switch (u->order) {
	case W2_ORD_TRAIN:
	case W2_ORD_UPGRADE_TO: {
		const w2_unit_type *t = w2_unit(u->queue_type);
		if (t) refund_pct(u->owner, t->gold, t->wood, t->oil, 100);
		break;
	}
	case W2_ORD_RESEARCH:
		if (u->queue_type < W2_NUM_UPGRADES) {
			const w2_upgrade *g = &w2_upgrades[u->queue_type];
			refund_pct(u->owner, g->gold, g->wood, g->oil, 100);
		}
		break;
	default:
		return;
	}
	u->progress = 0;
	go_still(b);
}

/* ------------------------------------------------------------------------------------------- */
/* repair                                                                                       */

#define REPAIR_PERIOD 6     /* cycles per repair step (one W2 time unit) - NIEPEWNE */
#define REPAIR_HP     4     /* hp per step (Mac f6_2CC8 = 0x132CC8 repair step) */

/* Mac: a counter on the REPAIRED unit (shared by all its repairers) - every second step costs
 * 1 gold + 1 wood, whatever the type costs; never oil. Slot generation guards a reused slot. */

static int repairable(int s, int t)
{
	const w2_unit_type *tt;
	if (!alive(t) || w2.unit[t].owner != w2.unit[s].owner || (w2.unit[t].flags & W2_UF_UNDERCONST)) return 0;
	tt = T(t);
	if (tt->flags & (W2UF_GOLD_MINE | W2UF_OIL_PATCH)) return 0;
	return (w2.unit[t].flags & W2_UF_BUILDING) || tt->kind == W2_KIND_SEA ||
	       w2.unit[t].type == W2U_BALLISTA || w2.unit[t].type == W2U_CATAPULT;
}

void cmd_repair(int slot, int target)
{
	check_reset();
	if (!alive(slot) || !(T(slot)->xflags & W2UX_CAN_REPAIR) || !repairable(slot, target)) return;
	if (w2.unit[target].hp >= T(target)->hp) return;
	unit_set_order(slot, W2_ORD_REPAIR, target, 0, 0);
}

void ord_repair(int s)
{
	W2Unit *u = &w2.unit[s];
	int t = u->target, r;
	const w2_unit_type *tt;
	check_reset();
	if (!alive(s)) return;
	if (!repairable(s, t) || w2.unit[t].hp >= T(t)->hp) { go_still(s); return; }
	tt = T(t);
	if (u->order_state == 0 || unit_gap(s, t) > 1) {
		r = move_toward_unit(s, t, 1);
		if (r < 0) { go_still(s); return; }
		if (r == 0) return;
		u->order_state = 1;
		u->timer = 0;
		{ int cx, cy; unit_center(t, &cx, &cy); unit_set_dir_to(s, cx, cy); }
	}
	set_anim(s, W2_ANIM_REPAIR);
	if (++u->timer < REPAIR_PERIOD) return;
	u->timer = 0;
	/* Mac: the step needs at least 1 gold and 1 wood in stock (lumber message first) */
	if (!econ_can_afford(u->owner, 1, 1, 0)) {
		msg_cost(u->owner, w2.player[u->owner].wood < 1 ? 0 : 1, 1, 0);
		go_still(s);
		return;
	}
	if (g_rep_gen[t] != unit_gen(t)) { g_rep_gen[t] = unit_gen(t); g_rep_cnt[t] = 0; }
	if (++g_rep_cnt[t] > 1) { g_rep_cnt[t] -= 2; econ_pay(u->owner, 1, 1, 0); }
	r = w2.unit[t].hp + REPAIR_HP;
	w2.unit[t].hp = (uint16_t)(r > tt->hp ? tt->hp : r);
	if (w2.unit[t].hp >= tt->hp) go_still(s);
}
