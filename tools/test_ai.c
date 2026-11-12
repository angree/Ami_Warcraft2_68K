/* test_ai.c - host test of src/ai.c (computer opponents).
 *
 * Usage: test_ai <data dir with "War Data"> <scratch out dir>
 *
 * Links ai.c with the real pud/assets loaders, the real unit.c (unit_create/place/remove) and the weak
 * stubs of the other modules. Because order handlers are still stubs, this file supplies STRONG toy
 * versions of the cmd_* functions
 * plus a tiny "toy simulation" (walk in straight lines, harvest, build, train, research, melee/ranged
 * damage) so the AI's decisions can be followed over a game timeline. The toy sim is NOT the game's
 * rules - it only lets orders have plausible consequences. econ_place_ok stays the weak stub
 * (returns 0) so the AI's own build-site fallback is exercised.
 *
 * Tests:
 *  1. build-site search (fallback terrain test, no overlaps, lanes, mine distance, corridor)
 *  2. economy decisions from a given state (N workers, resources, buildings)
 *  3. attack-wave logic (sizes, reserve, target = explored building / enemy start)
 *  4. home defence (enemy near home, attacker of a hurt unit)
 *  5. full AI-vs-AI timeline on builtin map 220 (6 computer players), decision log
 *  6. determinism (timeline run twice gives the same log hash)
 * Output: <out>/ai_decisions.log (complete AI decision log of the timeline) and a summary on stdout.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "w2.h"
#include "pud.h"
#include "assets.h"
#include "w2cfg.h"
#include "ai.h"
#include "unit.h"

/* ---- platform bits the loaders need ---------------------------------------------------------- */
void amigagfx_set_palette(const unsigned char *rgb, int first, int count) { (void)rgb; (void)first; (void)count; }

static FILE *g_logf;
static unsigned long g_loghash = 5381, g_loglines;
static int g_echo;
void w2_log(const char *fmt, ...)
{
	char b[256];
	const char *c;
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(b, sizeof b, fmt, ap);
	va_end(ap);
	for (c = b; *c; c++) g_loghash = g_loghash * 33u + (unsigned char)*c;
	g_loglines++;
	if (g_logf) fprintf(g_logf, "%s\n", b);
	if (g_echo) printf("  | %s\n", b);
}

static int g_fail, g_pass;
#define CHECK(cond, ...) do { if (cond) { g_pass++; } else { g_fail++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ---- toy world ------------------------------------------------------------------------------- */
static uint16_t sim_hslot[W2_MAX_UNITS];   /* harvest target slot (mine) or W2_NO_UNIT for wood */
static uint8_t  sim_htx[W2_MAX_UNITS], sim_hty[W2_MAX_UNITS];
static uint8_t  sim_mv[W2_MAX_UNITS];      /* movement cooldown */
static uint8_t  sim_am[W2_MAX_UNITS];      /* 1: return to attack-move after the fight */
static uint8_t  sim_amx[W2_MAX_UNITS], sim_amy[W2_MAX_UNITS];
static uint16_t sim_link[W2_MAX_UNITS];    /* builder <-> building */
static uint8_t  sim_btype[W2_MAX_UNITS];
static uint8_t  sim_chops[W2_MAP_MAX * W2_MAP_MAX];
static int cnt_cmd_build, cnt_cmd_build_bad, cnt_cmd_train, cnt_cmd_research, cnt_cmd_upgrade, cnt_cmd_harvest,
           cnt_cmd_attack, cnt_cmd_attack_move, cnt_cmd_move, cnt_overlap;

static int iabs(int v) { return v < 0 ? -v : v; }
static const w2_unit_type *T(int slot) { return w2_unit(w2.unit[slot].type); }

static void sim_kill(int s)
{
	if (sim_link[s] != W2_NO_UNIT && (w2.unit[sim_link[s]].flags & W2_UF_INSIDE)) {   /* builder inside */
		int bl = sim_link[s];
		w2.unit[bl].flags &= (uint16_t)~W2_UF_INSIDE;
		w2.unit[bl].order = W2_ORD_STILL;
		unit_place(bl, w2.unit[bl].tx, w2.unit[bl].ty);
		sim_link[bl] = W2_NO_UNIT;
	}
	sim_link[s] = W2_NO_UNIT;
	unit_remove(s);   /* real unit.c: off the map, counts, slot freed (no corpse needed here) */
}

static int sim_create(int type, int owner, int tx, int ty)
{
	int s = unit_create(type, owner, tx, ty);   /* real unit.c */
	if (s >= 0) { sim_link[s] = W2_NO_UNIT; sim_hslot[s] = W2_NO_UNIT; sim_am[s] = 0; sim_mv[s] = 0; }
	return s;
}

/* chebyshev distance from a unit's tile to a slot's footprint */
static int dist_to_slot(int a, int b)
{
	const W2Unit *u = &w2.unit[a], *v = &w2.unit[b];
	const w2_unit_type *t = w2_unit(v->type);
	int dx = 0, dy = 0;
	if (u->tx < v->tx) dx = v->tx - u->tx; else if (u->tx >= v->tx + t->tile_w) dx = u->tx - (v->tx + t->tile_w - 1);
	if (u->ty < v->ty) dy = v->ty - u->ty; else if (u->ty >= v->ty + t->tile_h) dy = u->ty - (v->ty + t->tile_h - 1);
	return dx > dy ? dx : dy;
}

/* toy walking: one tile per 6 cycles in a straight line, ignores terrain; 1 = within range */
static int sim_walk(int s, int gx, int gy, int range)
{
	W2Unit *u = &w2.unit[s];
	int dx = gx - u->tx, dy = gy - u->ty;
	if (iabs(dx) <= range && iabs(dy) <= range) return 1;
	if (sim_mv[s]) { sim_mv[s]--; return 0; }
	sim_mv[s] = 5;
	unit_place(s, u->tx + (dx > 0) - (dx < 0), u->ty + (dy > 0) - (dy < 0));
	return 0;
}

static int nearest_hall(int s)
{
	int i, best = -1, bd = 9999;
	for (i = 0; i < w2.unit_hi; i++) {
		const W2Unit *v = &w2.unit[i];
		if (!(v->flags & W2_UF_ALIVE) || v->owner != w2.unit[s].owner || (v->flags & W2_UF_UNDERCONST)) continue;
		if (!(w2_unit(v->type)->flags & W2UF_STORE_HALL)) continue;
		if (dist_to_slot(s, i) < bd) { bd = dist_to_slot(s, i); best = i; }
	}
	return best;
}

static int acquire(int s, int range)
{
	int i, best = -1, bd = range + 1;
	for (i = 0; i < w2.unit_hi; i++) {
		const W2Unit *v = &w2.unit[i];
		int d;
		if (!(v->flags & W2_UF_ALIVE) || v->owner >= 8 || !((w2.player[w2.unit[s].owner].enemies >> v->owner) & 1)) continue;
		if (v->flags & W2_UF_INSIDE) continue;
		d = dist_to_slot(s, i);
		if (d < bd || (d == bd && best >= 0 && (w2.unit[best].flags & W2_UF_BUILDING) && !(v->flags & W2_UF_BUILDING))) { bd = d; best = i; }
	}
	return best;
}

static void sim_damage(int a, int v)
{
	const w2_unit_type *ta = T(a), *tv = T(v);
	int dmg = ta->basic_damage > tv->armor ? ta->basic_damage - tv->armor : 1;
	dmg = (dmg + ta->piercing_damage) * (50 + (int)(w2_rand() % 51)) / 100;
	if (dmg < 1) dmg = 1;
	if (w2.unit[v].hp > dmg) w2.unit[v].hp = (uint16_t)(w2.unit[v].hp - dmg);
	else sim_kill(v);
}

static void spawn_near(int b, int type)
{
	const W2Unit *u = &w2.unit[b];
	const w2_unit_type *t = w2_unit(u->type);
	int r, x, y;
	for (r = 1; r < 6; r++)
		for (y = u->ty - r; y < u->ty + t->tile_h + r; y++)
			for (x = u->tx - r; x < u->tx + t->tile_w + r; x++) {
				int ix;
				if (x < 0 || y < 0 || x >= w2.map.w || y >= w2.map.h) continue;
				ix = y * w2.map.w + x;
				if (w2.map.occ_ground[ix] == W2_NO_UNIT && !(w2.map.flags[ix] & (W2_TF_BUILDING | W2_TF_WATER | W2_TF_TREE | W2_TF_ROCK))) {
					sim_create(type, u->owner, x, y);
					return;
				}
			}
}

static int site_free(int type, int tx, int ty)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	int x, y;
	if (tx < 0 || ty < 0 || tx + t->tile_w > w2.map.w || ty + t->tile_h > w2.map.h) return 0;
	for (y = ty; y < ty + t->tile_h; y++)
		for (x = tx; x < tx + t->tile_w; x++) {
			uint16_t f = w2.map.flags[y * w2.map.w + x];
			if (f & (W2_TF_BUILDING | W2_TF_WATER | W2_TF_TREE | W2_TF_ROCK | W2_TF_WALL)) return 0;
		}
	return 1;
}

static void sim_unit(int s)
{
	W2Unit *u = &w2.unit[s];
	const w2_unit_type *t = T(s);
	switch (u->order) {
	case W2_ORD_MOVE:
		if (sim_walk(s, u->goal_tx, u->goal_ty, 0)) u->order = W2_ORD_STILL;
		break;
	case W2_ORD_HARVEST:
		if (u->flags & W2_UF_INSIDE) {
			if (u->timer && --u->timer) break;
			u->flags &= (uint16_t)~W2_UF_INSIDE;
			u->res_amount = 100;
			u->res_kind = (sim_hslot[s] != W2_NO_UNIT) ? W2_RES_GOLD : W2_RES_WOOD;
			if (u->res_kind == W2_RES_GOLD) {
				W2Unit *m = &w2.unit[sim_hslot[s]];
				if (m->res_left >= 100) m->res_left -= 100; else m->res_left = 0;
				if (!m->res_left) sim_kill(sim_hslot[s]);
			} else {
				int ix = sim_hty[s] * w2.map.w + sim_htx[s];
				if (++sim_chops[ix] >= 4) { w2.map.flags[ix] &= (uint16_t)~W2_TF_TREE; w2.map.version++; }
			}
			u->order = W2_ORD_RETURN;
			break;
		}
		if (sim_hslot[s] != W2_NO_UNIT) {
			if (!(w2.unit[sim_hslot[s]].flags & W2_UF_ALIVE)) { u->order = W2_ORD_STILL; break; }
			if (dist_to_slot(s, sim_hslot[s]) <= 1) { u->flags |= W2_UF_INSIDE; u->timer = 150; }
			else sim_walk(s, w2.unit[sim_hslot[s]].tx + 1, w2.unit[sim_hslot[s]].ty + 1, 1);
		} else {
			if (!(w2.map.flags[sim_hty[s] * w2.map.w + sim_htx[s]] & W2_TF_TREE)) { u->order = W2_ORD_STILL; break; }
			if (sim_walk(s, sim_htx[s], sim_hty[s], 1)) { u->flags |= W2_UF_INSIDE; u->timer = 250; }
		}
		break;
	case W2_ORD_RETURN: {
		int h = nearest_hall(s);
		if (h < 0) { u->order = W2_ORD_STILL; break; }
		if (dist_to_slot(s, h) <= 1) {
			if (u->res_kind == W2_RES_GOLD) w2.player[u->owner].gold += u->res_amount;
			if (u->res_kind == W2_RES_WOOD) w2.player[u->owner].wood += u->res_amount;
			u->res_amount = 0; u->res_kind = 0;
			u->order = W2_ORD_HARVEST;
		} else sim_walk(s, w2.unit[h].tx + 1, w2.unit[h].ty + 1, 2);
		break;
	}
	case W2_ORD_BUILD:
		if (u->flags & W2_UF_INSIDE) break;   /* building it */
		if (sim_walk(s, u->goal_tx, u->goal_ty, 1)) {
			const w2_unit_type *bt = w2_unit(sim_btype[s]);
			W2Player *pl = &w2.player[u->owner];
			int b;
			if (!site_free(sim_btype[s], u->goal_tx, u->goal_ty)) { cnt_overlap++; u->order = W2_ORD_STILL; break; }
			if (pl->gold < bt->gold || pl->wood < bt->wood) { u->order = W2_ORD_STILL; break; }
			pl->gold -= bt->gold; pl->wood -= bt->wood; pl->oil -= bt->oil;
			b = sim_create(sim_btype[s], u->owner, u->goal_tx, u->goal_ty);
			if (b < 0) { u->order = W2_ORD_STILL; break; }
			w2.unit[b].flags |= W2_UF_UNDERCONST;
			w2.unit[b].order = W2_ORD_BUILT;
			w2.unit[b].timer = (uint16_t)w2_time_to_cycles(bt->build_time);
			sim_link[b] = (uint16_t)s; sim_link[s] = (uint16_t)b;
			unit_unplace(s);
			u->flags |= W2_UF_INSIDE;
		}
		break;
	case W2_ORD_BUILT:
		if (u->timer && --u->timer) break;
		u->flags &= (uint16_t)~W2_UF_UNDERCONST;
		u->order = W2_ORD_STILL;
		if (sim_link[s] != W2_NO_UNIT) {
			W2Unit *bl = &w2.unit[sim_link[s]];
			bl->flags &= (uint16_t)~W2_UF_INSIDE; bl->order = W2_ORD_STILL;
			unit_place(sim_link[s], bl->tx, bl->ty);
			sim_link[sim_link[s]] = W2_NO_UNIT; sim_link[s] = W2_NO_UNIT;
		}
		break;
	case W2_ORD_TRAIN:
		if (u->timer && --u->timer) break;
		spawn_near(s, u->queue_type);
		u->order = W2_ORD_STILL;
		break;
	case W2_ORD_RESEARCH:
		if (u->timer && --u->timer) break;
		if (u->queue_type < 32) w2.player[u->owner].upgrades |= 1u << u->queue_type;
		u->order = W2_ORD_STILL;
		break;
	case W2_ORD_UPGRADE_TO:
		if (u->timer && --u->timer) break;
		u->type = u->queue_type;
		u->hp = w2_unit(u->type)->hp;
		u->order = W2_ORD_STILL;
		break;
	case W2_ORD_ATTACK: {
		int v = u->target;
		if (v == W2_NO_UNIT || !(w2.unit[v].flags & W2_UF_ALIVE) || (w2.unit[v].flags & W2_UF_INSIDE)) {
			if (sim_am[s]) { u->order = W2_ORD_ATTACK_MOVE; u->goal_tx = sim_amx[s]; u->goal_ty = sim_amy[s]; }
			else u->order = W2_ORD_STILL;
			u->target = W2_NO_UNIT;
			break;
		}
		if (dist_to_slot(s, v) <= (t->max_range ? t->max_range : 1)) {
			if (++u->timer >= 30) { u->timer = 0; sim_damage(s, v); }
		} else sim_walk(s, w2.unit[v].tx, w2.unit[v].ty, 1);
		break;
	}
	case W2_ORD_ATTACK_MOVE:
		if (((w2.cycle + (unsigned)s) % 10) == 0) {
			int v = acquire(s, 5);
			if (v >= 0) {
				sim_am[s] = 1; sim_amx[s] = u->goal_tx; sim_amy[s] = u->goal_ty;
				u->order = W2_ORD_ATTACK; u->target = (uint16_t)v; u->timer = 0;
				break;
			}
		}
		if (sim_walk(s, u->goal_tx, u->goal_ty, 0)) { u->order = W2_ORD_STILL; sim_am[s] = 0; }
		break;
	case W2_ORD_NONE:
	case W2_ORD_STILL:
		if (!(u->flags & W2_UF_BUILDING) && (t->flags & W2UF_CAN_ATTACK) && !(t->xflags & W2UX_COWARD) &&
		    ((w2.cycle + (unsigned)s) % 15) == 0) {
			int v = acquire(s, 4);
			if (v >= 0) { sim_am[s] = 0; u->order = W2_ORD_ATTACK; u->target = (uint16_t)v; u->timer = 0; }
		}
		break;
	}
}

static void sim_tick(void)
{
	int i;
	for (i = 0; i < w2.unit_hi; i++)
		if (w2.unit[i].flags & W2_UF_ALIVE) sim_unit(i);
	if (w2.cycle % 300 == 0) {
		int p;
		uint16_t n[8] = { 0 };
		for (i = 0; i < w2.unit_hi; i++)
			if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].owner < 8) n[w2.unit[i].owner]++;
		for (p = 0; p < 8; p++)
			if (w2.player[p].control == W2_CTRL_COMPUTER && !w2.player[p].defeated && !n[p]) {
				w2.player[p].defeated = 1;
				w2_log("sim: player %d defeated at cycle %lu", p, (unsigned long)w2.cycle);
			}
	}
	w2.cycle++;
}

/* ---- strong command implementations (toy) --------------------------------------------------- */
void cmd_stop(int s) { w2.unit[s].order = W2_ORD_STILL; w2.unit[s].target = W2_NO_UNIT; }
void cmd_move(int s, int tx, int ty) { cnt_cmd_move++; w2.unit[s].order = W2_ORD_MOVE; w2.unit[s].goal_tx = (uint8_t)tx; w2.unit[s].goal_ty = (uint8_t)ty; }
void cmd_attack(int s, int t) { cnt_cmd_attack++; w2.unit[s].order = W2_ORD_ATTACK; w2.unit[s].target = (uint16_t)t; w2.unit[s].timer = 0; sim_am[s] = 0; }
void cmd_attack_move(int s, int tx, int ty) { cnt_cmd_attack_move++; w2.unit[s].order = W2_ORD_ATTACK_MOVE; w2.unit[s].goal_tx = (uint8_t)tx; w2.unit[s].goal_ty = (uint8_t)ty; }
void cmd_harvest(int s, int t, int tx, int ty)
{
	cnt_cmd_harvest++;
	w2.unit[s].order = W2_ORD_HARVEST;
	w2.unit[s].target = (uint16_t)t;   /* -1 -> W2_NO_UNIT: tree */
	sim_hslot[s] = (uint16_t)t;
	sim_htx[s] = (uint8_t)tx; sim_hty[s] = (uint8_t)ty;
}
void cmd_return(int s) { w2.unit[s].order = W2_ORD_RETURN; }
void cmd_build(int s, int type, int tx, int ty)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	cnt_cmd_build++;
	if (!t || !(T(s)->flags & W2UF_WORKER) || !site_free(type, tx, ty)) { cnt_cmd_build_bad++; return; }
	w2.unit[s].order = W2_ORD_BUILD; w2.unit[s].goal_tx = (uint8_t)tx; w2.unit[s].goal_ty = (uint8_t)ty;
	sim_btype[s] = (uint8_t)type;
}
static int sim_pay(int p, int g, int w, int o)
{
	W2Player *pl = &w2.player[p];
	if (pl->gold < g || pl->wood < w || pl->oil < o) return 0;
	pl->gold -= g; pl->wood -= w; pl->oil -= o;
	return 1;
}
void cmd_train(int b, int type)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	W2Unit *u = &w2.unit[b];
	cnt_cmd_train++;
	if (!t || (u->order != W2_ORD_STILL && u->order != W2_ORD_NONE) || (u->flags & W2_UF_UNDERCONST)) return;
	if (!sim_pay(u->owner, t->gold, t->wood, t->oil)) return;
	u->order = W2_ORD_TRAIN; u->queue_type = (uint8_t)type; u->timer = (uint16_t)w2_time_to_cycles(t->build_time);
}
void cmd_research(int b, int upg)
{
	const w2_upgrade *g = &w2_upgrades[upg];
	W2Unit *u = &w2.unit[b];
	cnt_cmd_research++;
	if (u->order != W2_ORD_STILL && u->order != W2_ORD_NONE) return;
	if (!sim_pay(u->owner, g->gold, g->wood, g->oil)) return;
	u->order = W2_ORD_RESEARCH; u->queue_type = (uint8_t)upg; u->timer = (uint16_t)w2_time_to_cycles(g->time);
}
void cmd_upgrade_to(int b, int type)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	W2Unit *u = &w2.unit[b];
	cnt_cmd_upgrade++;
	if (u->order != W2_ORD_STILL && u->order != W2_ORD_NONE) return;
	if (!sim_pay(u->owner, t->gold, t->wood, t->oil)) return;
	u->order = W2_ORD_UPGRADE_TO; u->queue_type = (uint8_t)type; u->timer = (uint16_t)w2_time_to_cycles(t->build_time);
}

/* ---- scenario helpers ------------------------------------------------------------------------- */
static void reset_counts(void)
{
	cnt_cmd_build = cnt_cmd_build_bad = cnt_cmd_train = cnt_cmd_research = cnt_cmd_upgrade = cnt_cmd_harvest = 0;
	cnt_cmd_attack = cnt_cmd_attack_move = cnt_cmd_move = cnt_overlap = 0;
}

static int load_map(void)
{
	int p;
	memset(sim_chops, 0, sizeof sim_chops);
	memset(sim_link, 0xFF, sizeof sim_link); memset(sim_hslot, 0xFF, sizeof sim_hslot);
	memset(sim_am, 0, sizeof sim_am); memset(sim_mv, 0, sizeof sim_mv);
	if (pud_load_builtin(220)) return -1;
	for (p = 0; p < 8; p++)
		if (w2.player[p].start_tx != 0xFFFF) w2.player[p].control = W2_CTRL_COMPUTER;
	return 0;
}

/* remove every unit except gold mines */
static void clear_units(void)
{
	int i;
	for (i = 0; i < w2.unit_hi; i++)
		if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].type != W2U_GOLD_MINE) sim_kill(i);
}

static int nearest_mine_to(int x, int y)
{
	int i, best = -1, bd = 9999;
	for (i = 0; i < w2.unit_hi; i++)
		if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].type == W2U_GOLD_MINE) {
			int d = iabs(w2.unit[i].tx - x) > iabs(w2.unit[i].ty - y) ? iabs(w2.unit[i].tx - x) : iabs(w2.unit[i].ty - y);
			if (d < bd) { bd = d; best = i; }
		}
	return best;
}

/* place a building at the first site the AI itself would choose (keeps scenarios terrain-correct) */
static int place(int p, int type, int cx, int cy)
{
	int tx, ty, s;
	ai_debug_scan(p);
	if (!ai_find_site(p, type, cx, cy, -1, &tx, &ty)) return -1;
	s = sim_create(type, p, tx, ty);
	return s;
}

/* a home base for player p: town hall near its start, nothing else */
static int make_base(int p)
{
	const W2Player *pl = &w2.player[p];
	int m = nearest_mine_to(pl->start_tx, pl->start_ty), hall, cx, cy;
	int htype = pl->race ? W2U_GREAT_HALL : W2U_TOWN_HALL;
	cx = m >= 0 ? w2.unit[m].tx + 1 : pl->start_tx; cy = m >= 0 ? w2.unit[m].ty + 1 : pl->start_ty;
	hall = place(p, htype, cx, cy);
	return hall;
}

static int count_orders(int p, int order)
{
	int i, n = 0;
	for (i = 0; i < w2.unit_hi; i++)
		if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].owner == p && w2.unit[i].order == order) n++;
	return n;
}

/* ---- 1. build sites --------------------------------------------------------------------------- */
static void test_sites(void)
{
	int p = 0, hall, i, k, tx, ty, ok, x, y, bad = 0, w = 0;
	printf("== 1. build-site search\n");
	load_map(); clear_units();
	hall = make_base(p);
	CHECK(hall >= 0, "town hall site near the mine found");
	if (hall < 0) return;
	w2_log("test1: hall at %d,%d", w2.unit[hall].tx, w2.unit[hall].ty);
	CHECK(econ_place_ok(p, W2U_FARM, 0, 0, -1) == 0, "econ_place_ok is the weak stub here (fallback path)");
	for (k = 0; k < 14; k++) {
		int type = (k % 4 == 3) ? W2U_HUMAN_BARRACKS : W2U_FARM, s;
		const w2_unit_type *t = w2_unit(type);
		ai_debug_scan(p);
		ok = ai_find_site(p, type, w2.unit[hall].tx + 2, w2.unit[hall].ty + 2, -1, &tx, &ty);
		CHECK(ok, "site %d for %s found", k, t->ident);
		if (!ok) break;
		/* footprint must be free land, not touching other buildings */
		for (y = ty - 1; y <= ty + t->tile_h; y++)
			for (x = tx - 1; x <= tx + t->tile_w; x++) {
				uint16_t f = w2.map.flags[y * w2.map.w + x];
				int in = x >= tx && y >= ty && x < tx + t->tile_w && y < ty + t->tile_h;
				if (f & W2_TF_BUILDING) bad++;
				if (in && (!(f & W2_TF_LAND) || (f & (W2_TF_NOWALK | W2_TF_NOBUILD | W2_TF_TREE | W2_TF_WATER | W2_TF_ROCK)))) bad++;
			}
		s = sim_create(type, p, tx, ty);
		w2_log("test1: %s #%d at %d,%d", t->ident, s, tx, ty);
		w++;
	}
	CHECK(bad == 0, "no site overlaps/touches a building or bad terrain (%d bad tiles)", bad);
	/* mine gap + hall-mine corridor */
	{
		int m = nearest_mine_to(w2.unit[hall].tx, w2.unit[hall].ty), viol = 0;
		for (i = 0; i < w2.unit_hi; i++) {
			const W2Unit *u = &w2.unit[i];
			const w2_unit_type *t = w2_unit(u->type);
			int gx, gy, g;
			if (!(u->flags & W2_UF_ALIVE) || u->owner != p || i == hall) continue;
			gx = u->tx >= w2.unit[m].tx + 3 ? u->tx - (w2.unit[m].tx + 3) : (w2.unit[m].tx >= u->tx + t->tile_w ? w2.unit[m].tx - (u->tx + t->tile_w) : -1);
			gy = u->ty >= w2.unit[m].ty + 3 ? u->ty - (w2.unit[m].ty + 3) : (w2.unit[m].ty >= u->ty + t->tile_h ? w2.unit[m].ty - (u->ty + t->tile_h) : -1);
			g = gx > gy ? gx : gy;
			if (g < 2) viol++;
		}
		CHECK(viol == 0, "every building at least 2 tiles from the main mine (%d violations)", viol);
	}
	/* explicit rejections */
	{
		int water = -1, tree = -1;
		for (i = 0; i < w2.map.w * w2.map.h && (water < 0 || tree < 0); i++) {
			if (water < 0 && (w2.map.flags[i] & W2_TF_WATER) && i % w2.map.w > 2 && i / w2.map.w > 2) water = i;
			if (tree < 0 && (w2.map.flags[i] & W2_TF_TREE) && i % w2.map.w > 2 && i / w2.map.w > 2) tree = i;
		}
		if (water >= 0) CHECK(!ai_site_ok(p, W2U_FARM, water % w2.map.w, water / w2.map.w), "site on water rejected");
		if (tree >= 0) CHECK(!ai_site_ok(p, W2U_FARM, tree % w2.map.w, tree / w2.map.w), "site on trees rejected");
		CHECK(!ai_site_ok(p, W2U_FARM, w2.unit[hall].tx, w2.unit[hall].ty), "site on the hall rejected");
		CHECK(!ai_site_ok(p, W2U_FARM, 0, 0), "site at the map edge rejected");
	}
	printf("   placed %d buildings around the hall at %d,%d\n", w, w2.unit[hall].tx, w2.unit[hall].ty);
}

/* ---- 2. economy decisions from a given state -------------------------------------------------- */
static void test_economy(void)
{
	int p = 0, hall, i, nw = 0, ngold = 0, nwood = 0, nbuild, ntrain;
	AiInfo inf;
	printf("== 2. economy decisions\n");
	load_map(); clear_units();
	hall = make_base(p);
	for (i = 0; i < 5; i++) sim_create(W2U_PEASANT, p, w2.unit[hall].tx + i, w2.unit[hall].ty + 5);
	w2.player[p].gold = 2000; w2.player[p].wood = 1000;
	reset_counts();
	w2_log("test2: hall + 5 idle peasants, 2000 gold / 1000 wood");
	ai_think_now(p);
	ai_get_info(p, &inf);
	for (i = 0; i < w2.unit_hi; i++) {
		const W2Unit *u = &w2.unit[i];
		if (!(u->flags & W2_UF_ALIVE) || u->owner != p || u->type != W2U_PEASANT) continue;
		nw++;
		if (u->order == W2_ORD_HARVEST && u->target != W2_NO_UNIT) ngold++;
		if (u->order == W2_ORD_HARVEST && u->target == W2_NO_UNIT) nwood++;
	}
	nbuild = count_orders(p, W2_ORD_BUILD);
	ntrain = count_orders(p, W2_ORD_TRAIN);
	printf("   5 peasants -> %d gold, %d wood, %d building; hall training: %d; worker target %d\n", ngold, nwood, nbuild, ntrain, inf.worker_target);
	CHECK(ngold + nwood + nbuild == 5, "every idle worker got a job");
	CHECK(ngold >= 2 && ngold >= nwood, "about 2/3 on gold (%d gold %d wood)", ngold, nwood);
	CHECK(nbuild >= 1 && nbuild <= 2, "one or two builders sent (farm, barracks)");
	CHECK(inf.food_supply == 1 && inf.food_used >= 5, "food from the hall only (%d/%d)", inf.food_used, inf.food_supply);

	/* food is short (5/1): next think must not train, and a farm must be under way */
	{
		int farm_b = 0;
		for (i = 0; i < w2.unit_hi; i++)
			if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].owner == p && w2.unit[i].order == W2_ORD_BUILD && sim_btype[i] == W2U_FARM) farm_b++;
		CHECK(farm_b == 1, "farm ordered first when food is short");
		CHECK(ntrain == 0, "no worker trained without food");
	}

	/* state 2: plenty of food (farms), 3 workers: train up to target, barracks */
	load_map(); clear_units();
	hall = make_base(p);
	for (i = 0; i < 3; i++) place(p, W2U_FARM, w2.unit[hall].tx, w2.unit[hall].ty);
	for (i = 0; i < 3; i++) sim_create(W2U_PEASANT, p, w2.unit[hall].tx + i, w2.unit[hall].ty + 5);
	w2.player[p].gold = 3000; w2.player[p].wood = 1500;
	w2_log("test2b: hall + 3 farms + 3 peasants, 3000 gold");
	ai_think_now(p);
	ai_get_info(p, &inf);
	CHECK(count_orders(p, W2_ORD_TRAIN) == 1, "hall trains a peasant (workers %d < target %d)", inf.workers, inf.worker_target);
	CHECK(count_orders(p, W2_ORD_BUILD) == 0, "barracks waits for 5 workers (build order step %d)", inf.step);

	/* state 3: 8 workers + barracks + 3000 gold: lumber mill next, army training */
	load_map(); clear_units();
	hall = make_base(p);
	for (i = 0; i < 4; i++) place(p, W2U_FARM, w2.unit[hall].tx, w2.unit[hall].ty);
	place(p, W2U_HUMAN_BARRACKS, w2.unit[hall].tx, w2.unit[hall].ty);
	for (i = 0; i < 8; i++) sim_create(W2U_PEASANT, p, w2.unit[hall].tx + (i & 3), w2.unit[hall].ty + 5 + (i >> 2));
	w2.player[p].gold = 3000; w2.player[p].wood = 2000;
	w2_log("test2c: hall + 4 farms + barracks + 8 peasants, 3000 gold / 2000 wood");
	reset_counts();
	ai_think_now(p);
	{
		int mill = 0, footmen = 0;
		for (i = 0; i < w2.unit_hi; i++) {
			const W2Unit *u = &w2.unit[i];
			if (!(u->flags & W2_UF_ALIVE) || u->owner != p) continue;
			if (u->order == W2_ORD_BUILD && sim_btype[i] == W2U_ELVEN_LUMBER_MILL) mill++;
			if (u->order == W2_ORD_TRAIN && u->queue_type == W2U_FOOTMAN) footmen++;
		}
		CHECK(mill == 1, "lumber mill ordered after barracks");
		CHECK(footmen == 1, "barracks trains a footman with spare gold");
	}
}

/* ---- 3. attack waves -------------------------------------------------------------------------- */
static void test_waves(void)
{
	int p = 0, hall, i, n, e = -1, q, sent;
	AiInfo inf;
	printf("== 3. attack waves\n");
	load_map(); clear_units();
	hall = make_base(p);
	for (i = 0; i < 6; i++) place(p, W2U_FARM, w2.unit[hall].tx, w2.unit[hall].ty);
	w2.player[p].gold = 0; w2.player[p].wood = 0;   /* no spending: isolate the military logic */
	/* nearest enemy start */
	{
		int bd = 9999;
		for (q = 1; q < 8; q++)
			if (w2.player[q].start_tx != 0xFFFF) {
				int d = iabs(w2.player[q].start_tx - w2.unit[hall].tx) + iabs(w2.player[q].start_ty - w2.unit[hall].ty);
				if (d < bd) { bd = d; e = q; }
			}
		/* give every enemy a unit so it counts as alive */
		for (q = 1; q < 8; q++)
			if (w2.player[q].start_tx != 0xFFFF) sim_create(q == e ? W2U_GRUNT : W2U_PEON, q, w2.player[q].start_tx, w2.player[q].start_ty);
	}
	CHECK(ai_wave_size(0) == 3 && ai_wave_size(1) == 5 && ai_wave_size(2) == 8 && ai_wave_size(50) == 24, "wave sizes 3,5,8 .. 24");
	for (i = 0; i < 2; i++) sim_create(W2U_FOOTMAN, p, w2.unit[hall].tx + 5 + i, w2.unit[hall].ty + 5);
	ai_think_now(p);
	ai_get_info(p, &inf);
	CHECK(inf.wave == 0 && count_orders(p, W2_ORD_ATTACK_MOVE) == 0, "2 soldiers < wave size 3: no attack");
	for (i = 0; i < 2; i++) sim_create(W2U_FOOTMAN, p, w2.unit[hall].tx + 5 + i, w2.unit[hall].ty + 6);
	reset_counts();
	w2_log("test3: 4 idle footmen, enemy %d start %d,%d", e, w2.player[e].start_tx, w2.player[e].start_ty);
	ai_think_now(p);
	ai_get_info(p, &inf);
	n = 0;
	for (i = 0; i < w2.unit_hi; i++) {
		const W2Unit *u = &w2.unit[i];
		if ((u->flags & W2_UF_ALIVE) && u->owner == p && u->order == W2_ORD_ATTACK_MOVE &&
		    u->goal_tx == w2.player[e].start_tx && u->goal_ty == w2.player[e].start_ty) n++;
	}
	printf("   wave 1: %d units -> enemy %d start %d,%d\n", n, e, w2.player[e].start_tx, w2.player[e].start_ty);
	CHECK(inf.wave == 1 && n == 4, "wave 1 sends all 4 idle soldiers to the nearest enemy start (wave %d, n %d)", inf.wave, n);

	/* attackers are not re-counted as home army: 3 new soldiers do not trigger wave 2 (needs 5+) */
	for (i = 0; i < 3; i++) sim_create(W2U_FOOTMAN, p, w2.unit[hall].tx + 5 + i, w2.unit[hall].ty + 7);
	ai_think_now(p);
	ai_get_info(p, &inf);
	CHECK(inf.wave == 1 && inf.attackers == 4 && inf.home_army == 3, "wave 2 waits (attackers %d home %d)", inf.attackers, inf.home_army);
	/* enough for wave 2 */
	for (i = 0; i < 4; i++) sim_create(W2U_ARCHER, p, w2.unit[hall].tx + 5 + i, w2.unit[hall].ty + 8);
	ai_think_now(p);
	ai_get_info(p, &inf);
	CHECK(inf.wave == 2 && inf.attackers == 11, "wave 2 sends 7 (attackers %d)", inf.attackers);

	/* wave 3 keeps a home reserve of 2; an explored enemy building becomes the target */
	{
		int farm = sim_create(W2U_PIG_FARM, e, w2.player[e].start_tx - 4, w2.player[e].start_ty), cx, cy;
		CHECK(farm >= 0, "enemy farm placed");
		if (farm < 0) return;
		w2.map.explored[w2.unit[farm].ty * w2.map.w + w2.unit[farm].tx] |= 1u << p;
		cx = w2.unit[farm].tx + 1; cy = w2.unit[farm].ty + 1;
		ai_get_info(p, &inf);
		for (i = 0; i < inf.next_wave_size; i++) sim_create(W2U_FOOTMAN, p, w2.unit[hall].tx + 4 + (i % 6), w2.unit[hall].ty + 9 + i / 6);
		sent = inf.next_wave_size - 2;
		w2_log("test3: explored enemy farm at %d,%d; %d fresh soldiers", w2.unit[farm].tx, w2.unit[farm].ty, inf.next_wave_size);
		ai_think_now(p);
		ai_get_info(p, &inf);
		n = 0;
		for (i = 0; i < w2.unit_hi; i++) {
			const W2Unit *u = &w2.unit[i];
			if ((u->flags & W2_UF_ALIVE) && u->owner == p && u->order == W2_ORD_ATTACK_MOVE && u->goal_tx == cx && u->goal_ty == cy) n++;
		}
		printf("   wave 3: %d units -> enemy farm %d,%d, home reserve %d\n", n, cx, cy, inf.home_army);
		CHECK(inf.wave == 3 && inf.home_army == 2, "wave 3 leaves 2 at home (home %d)", inf.home_army);
		CHECK(n >= sent, "wave 3 goes to the explored building (%d >= %d)", n, sent);
	}
}

/* ---- 4. defence -------------------------------------------------------------------------------- */
static void test_defence(void)
{
	int p = 0, hall, i, g, n, peasant, g2;
	AiInfo inf;
	printf("== 4. home defence\n");
	load_map(); clear_units();
	hall = make_base(p);
	for (i = 0; i < 6; i++) place(p, W2U_FARM, w2.unit[hall].tx, w2.unit[hall].ty);
	w2.player[p].gold = 0; w2.player[p].wood = 0;
	for (i = 0; i < 3; i++) sim_create(W2U_FOOTMAN, p, w2.unit[hall].tx + 5 + i, w2.unit[hall].ty + 5);
	g = sim_create(W2U_GRUNT, 1, w2.unit[hall].tx - 4, w2.unit[hall].ty + 1);
	w2_log("test4: enemy grunt #%d at %d,%d next to the hall", g, w2.unit[g].tx, w2.unit[g].ty);
	ai_think_now(p);
	ai_get_info(p, &inf);
	n = 0;
	for (i = 0; i < w2.unit_hi; i++)
		if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].owner == p && w2.unit[i].order == W2_ORD_ATTACK && w2.unit[i].target == g) n++;
	CHECK(inf.threat == g && n == 3, "3 home soldiers attack the intruder (threat %d, n %d)", inf.threat, n);

	/* attacker of a hurt peasant outside the home radius (but within tracking range) */
	sim_kill(g);
	for (i = 0; i < w2.unit_hi; i++)
		if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].owner == p && w2.unit[i].type == W2U_FOOTMAN) w2.unit[i].order = W2_ORD_STILL;
	peasant = sim_create(W2U_PEASANT, p, w2.unit[hall].tx + 2, w2.unit[hall].ty + 17 < w2.map.h - 1 ? w2.unit[hall].ty + 17 : w2.unit[hall].ty - 15);
	g2 = sim_create(W2U_AXETHROWER, 1, w2.unit[peasant].tx + 2, w2.unit[peasant].ty);
	ai_think_now(p);           /* records hp */
	w2.unit[peasant].hp -= 10; /* hit */
	ai_think_now(p);
	ai_get_info(p, &inf);
	n = 0;
	for (i = 0; i < w2.unit_hi; i++)
		if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].owner == p && w2.unit[i].order == W2_ORD_ATTACK && w2.unit[i].target == g2) n++;
	printf("   intruder: threat #%d; hurt peasant at %d,%d -> attacker #%d engaged by %d\n", g, w2.unit[peasant].tx, w2.unit[peasant].ty, g2, n);
	CHECK(inf.threat == g2 && n == 3, "soldiers go for whoever hurts the peasant (threat %d, n %d)", inf.threat, n);
}

/* ---- 5. timeline ------------------------------------------------------------------------------- */
typedef struct { int hall_at[8], barracks_at[8], wave_at[8], max_workers[8], max_army[8], waves[8], defeated_at[8]; } TlRes;

static unsigned long run_timeline(unsigned long cycles, int verbose, TlRes *res, double *ai_ms)
{
	int p, i;
	clock_t aic = 0;
	unsigned long h0 = g_loghash;
	g_loghash = 5381;
	load_map();
	w2.rng = 0x1234;
	memset(res, 0, sizeof *res);
	for (p = 0; p < 8; p++) { res->hall_at[p] = res->barracks_at[p] = res->wave_at[p] = res->defeated_at[p] = -1; }
	reset_counts();
	ai_init();
	while (w2.cycle < cycles) {
		clock_t c0 = clock();
		ai_tick();
		aic += clock() - c0;
		sim_tick();
		if (w2.cycle % 150 == 0) {
			for (p = 0; p < 8; p++) {
				AiInfo inf;
				if (w2.player[p].control != W2_CTRL_COMPUTER) continue;
				ai_get_info(p, &inf);
				if (res->hall_at[p] < 0 && inf.hall_tier > 0) res->hall_at[p] = (int)w2.cycle;
				if (res->wave_at[p] < 0 && inf.wave > 0) res->wave_at[p] = (int)w2.cycle;
				if (res->defeated_at[p] < 0 && w2.player[p].defeated) res->defeated_at[p] = (int)w2.cycle;
				if (inf.workers > res->max_workers[p]) res->max_workers[p] = inf.workers;
				if (inf.army > res->max_army[p]) res->max_army[p] = inf.army;
				res->waves[p] = inf.wave;
				if (res->barracks_at[p] < 0)
					for (i = 0; i < w2.unit_hi; i++)
						if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].owner == p && !(w2.unit[i].flags & W2_UF_UNDERCONST) &&
						    (w2.unit[i].type == W2U_HUMAN_BARRACKS || w2.unit[i].type == W2U_ORC_BARRACKS)) { res->barracks_at[p] = (int)w2.cycle; break; }
			}
		}
		if (w2.cycle % 6000 == 0) {
			if (verbose) printf("   -- cycle %lu (%lu min)\n", (unsigned long)w2.cycle, (unsigned long)w2.cycle / 1800);
			for (p = 0; p < 8; p++) {
				AiInfo inf;
				if (w2.player[p].control != W2_CTRL_COMPUTER) continue;
				ai_get_info(p, &inf);
				if (verbose) printf("   p%d %s tier %d wrk %2d/%2d (g%2d w%2d) food %3d/%3d army %2d home %2d att %2d waves %d gold %5d wood %5d step %2d%s\n",
				       p, w2.player[p].race ? "orc" : "hum", inf.hall_tier, inf.workers, inf.worker_target, inf.gold_workers, inf.wood_workers,
				       inf.food_used, inf.food_supply, inf.army, inf.home_army, inf.attackers, inf.wave,
				       (int)w2.player[p].gold, (int)w2.player[p].wood, inf.step, w2.player[p].defeated ? " DEFEATED" : "");
				w2_log("snapshot c%lu p%d tier %d workers %d/%d gold %d/wood %d workers food %d/%d army %d home %d att %d waves %d",
				       (unsigned long)w2.cycle, p, inf.hall_tier, inf.workers, inf.worker_target, inf.gold_workers, inf.wood_workers,
				       inf.food_used, inf.food_supply, inf.army, inf.home_army, inf.attackers, inf.wave);
			}
		}
	}
	if (ai_ms) *ai_ms = (double)aic * 1000.0 / CLOCKS_PER_SEC;   /* host-side timing only */
	(void)h0;
	return g_loghash;
}

int main(int argc, char **argv)
{
	char path[512];
	TlRes r1, r2;
	unsigned long h1, h2, lines1;
	double ai_ms = 0;
	int p, ncomp = 0, nwave = 0;
	AiInfo inf;
	const unsigned long CYC = 54000;   /* 30 minutes of game time */

	if (argc < 3) { fprintf(stderr, "usage: test_ai <data dir> <out dir>\n"); return 2; }
	/* settings: data dir + the ai_log key through the real w2.cfg reader */
	snprintf(path, sizeof path, "%s/w2.cfg", argv[2]);
	{
		FILE *f = fopen(path, "w");
		if (!f) { fprintf(stderr, "cannot write %s\n", path); return 2; }
		fprintf(f, "# test_ai\nlogic_skip=0\nai_log=1\n");
		fclose(f);
	}
	w2cfg_load(&w2_cfg, path);
	snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", argv[1]);
	if (assets_init()) { fprintf(stderr, "assets_init failed for %s\n", argv[1]); return 2; }
	snprintf(path, sizeof path, "%s/ai_decisions.log", argv[2]);
	g_logf = fopen(path, "w");
	if (!g_logf) { fprintf(stderr, "cannot write %s\n", path); return 2; }

	ai_init();   /* reads ai_log from w2.cfg */
	ai_set_log(1);
	CHECK(w2cfg_extra_int("ai_log", 0) == 1, "ai_log read from w2.cfg");

	fprintf(g_logf, "#### 1. build sites\n");   test_sites();
	fprintf(g_logf, "#### 2. economy\n");       test_economy();
	fprintf(g_logf, "#### 3. waves\n");         test_waves();
	fprintf(g_logf, "#### 4. defence\n");       test_defence();

	printf("== 5. AI-vs-AI timeline, map 220, %lu cycles\n", CYC);
	fprintf(g_logf, "#### 5. timeline map 220\n");
	lines1 = g_loglines;
	h1 = run_timeline(CYC, 1, &r1, &ai_ms);
	lines1 = g_loglines - lines1;
	for (p = 0; p < 8; p++) {
		if (w2.player[p].control != W2_CTRL_COMPUTER) continue;
		ncomp++;
		ai_get_info(p, &inf);
		printf("   p%d: hall@%d barracks@%d wave1@%d maxworkers %d maxarmy %d waves %d defeated@%d thinks %d\n", p,
		       r1.hall_at[p], r1.barracks_at[p], r1.wave_at[p], r1.max_workers[p], r1.max_army[p], r1.waves[p], r1.defeated_at[p], inf.thinks);
		CHECK(r1.hall_at[p] >= 0 && r1.hall_at[p] < 6000, "p%d has a town hall within 200 s (%d)", p, r1.hall_at[p]);
		CHECK(r1.barracks_at[p] >= 0 && r1.barracks_at[p] < 20000, "p%d has barracks (%d)", p, r1.barracks_at[p]);
		CHECK(r1.max_workers[p] >= 8, "p%d grew to >= 8 workers (%d)", p, r1.max_workers[p]);
		CHECK(r1.wave_at[p] >= 0 || (r1.defeated_at[p] >= 0 && r1.defeated_at[p] < 30000), "p%d launched an attack wave (or was eliminated early)", p);
		if (r1.wave_at[p] >= 0) nwave++;
		CHECK(inf.thinks >= (int)(CYC / AI_THINK_PERIOD) - 2 || w2.player[p].defeated, "p%d thinks every %d cycles (%d)", p, AI_THINK_PERIOD, inf.thinks);
	}
	printf("   commands: build %d (rejected %d, site taken on arrival %d) train %d research %d upgrade %d harvest %d attack %d attack-move %d move %d\n",
	       cnt_cmd_build, cnt_cmd_build_bad, cnt_overlap, cnt_cmd_train, cnt_cmd_research, cnt_cmd_upgrade, cnt_cmd_harvest,
	       cnt_cmd_attack, cnt_cmd_attack_move, cnt_cmd_move);
	printf("   decision log lines %lu, host time in ai_tick %.1f ms for %d players x %lu cycles\n", lines1, ai_ms, ncomp, CYC);
	CHECK(ncomp == 6, "6 computer players on map 220");
	CHECK(nwave >= 4, "at least 4 of 6 players attacked (%d)", nwave);
	CHECK(cnt_cmd_build_bad == 0, "no build command on an occupied/bad site (%d)", cnt_cmd_build_bad);
	CHECK(cnt_cmd_research > 0 && cnt_cmd_upgrade > 0, "research and hall upgrades happened");

	printf("== 6. determinism\n");
	fprintf(g_logf, "#### 6. timeline again (determinism)\n");
	{
		FILE *keep = g_logf;
		snprintf(path, sizeof path, "%s/ai_decisions_run2.log", argv[2]);
		g_logf = fopen(path, "w");
		h2 = run_timeline(CYC, 0, &r2, NULL);
		if (g_logf) fclose(g_logf);
		g_logf = keep;
		snprintf(path, sizeof path, "%s/ai_decisions.log", argv[2]);
	}
	printf("   log hash run1 %08lx run2 %08lx\n", h1 & 0xFFFFFFFFul, h2 & 0xFFFFFFFFul);
	CHECK(h1 == h2, "identical decision log on a second run");

	fprintf(g_logf, "#### done: %d passed, %d failed\n", g_pass, g_fail);
	fclose(g_logf);
	printf("== %d checks passed, %d failed; decision log: %s\n", g_pass, g_fail, path);
	assets_shutdown();
	return g_fail ? 1 : 0;
}
