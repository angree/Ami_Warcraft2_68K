/* transport.c - transports: docking at the shore, boarding, carrying, unloading (see transport.h).
 *
 * Behaviour spec: Warcraft II Mac 1.23 (addresses in RE notes form; read as a spec, own code here):
 *  - Right-click dispatch f9_58F2 (0x0014D8F2): a transport goes through 0x0014D61A; a land unit
 *    (movement type 0) clicking its own transport (unit flag 0x400) gets order 34 "board" and, unless
 *    the transport is already docking (order 30), the transport is commanded toward the unit
 *    (0x0014D74A).
 *  - Command entry 0x0016C166: a transport commanded onto a land (SQM 0x01) or shallow (0x02) tile
 *    looks for a dock point (0x00151584 -> spiral 0x0013098C, ring limit 8): a shallow tile not
 *    blocked for a docking ship (mask 0x0901: no land bit, no unit, no building) in the same region as
 *    the clicked tile.
 *  - 0x0014D61A: docked (on a shallow tile) and the dock point is on the same coast -> stay (order 32
 *    "docked, waiting"); docked and going elsewhere -> order 31 undock first (0x001511C4: single
 *    steps to an even water tile found by the ring search 0x00150D36, then the command is re-issued);
 *    dock point -> order 30 dock (0x00151038: movement type 3, 2-tile steps only when position and
 *    goal are even; on arrival order 29 unload if the unload flag unit+0x1C bit 0x40 is set (Unload
 *    command f9_5710) else order 32).
 *  - Board (order 34, 0x001329BE / 0x0013284C): the unit walks to the transport if it stands on the
 *    unit's coast, else to the transport's dock point (0x00150AA6); it boards when within 2 tiles on
 *    each axis (0x001327EE) of a transport in order 32 that has a free slot of 6 (0x0016C116): hidden
 *    (unit+0x1E bit 8), position = the transport's, order 36 (inside). If the transport's dock point is
 *    not in the unit's region, the unit gives up (order 2).
 *  - Unload (order 29, f6_2A4C 0x00132A4C): only when the transport stands on a shallow tile; passengers
 *    5..0 are each put on the first free land tile of the ring search around the transport (0x0016C02A,
 *    rings up to size 6) in the transport tile's region; they become idle. Then order 32.
 *  - A dying transport kills its passengers (SPEC_MECHANICS 9).
 * Ours: the Mac's order 30/32 are W2_ORD_LOAD (TRS_DOCKING / TRS_WAIT), 30+flag 0x40 is
 * W2_ORD_UNLOAD; order 34/36 are W2_ORD_LOAD on the passenger (TRS_START / TRS_INSIDE). The undock
 * order 31 is not a separate state: the docking / ship path search starts from the shallow tile and
 * the ship's first step goes to an even water tile (path.c). REGM (the PUD region map the Mac uses,
 * where a shallow tile carries the region of its land) is replaced by our own land regions: a shallow
 * tile "is in" region r when one of its 8 neighbours is walkable land of region r. Ring searches scan
 * square rings outward (the Mac's spiral visits the same rings in a different order inside a ring);
 * within the first ring that has a dock point the one nearest the transport is taken. A dock point
 * must also touch open water (so the transport can reach it).
 */
#include <string.h>
#include "w2.h"
#include "move.h"
#include "unit.h"
#include "transport.h"

#define TR_SHORE_RING    8     /* dock point search around the clicked tile (0x0013098C) */
#define TR_SHORE_FAR     40    /* ours, beyond that: the nearest coast of the region (see find_dock) */
#define TR_DROP_RING     3     /* passenger placement around the transport (0x0016C02A: size up to 6) */
#define TR_REACH         2     /* boarding distance per axis (0x001327EE) */
#define TR_REGION_PERIOD 1000  /* cycles between two relabellings after terrain changes (trees cut) */
#define TR_BAD_CYCLES    2000  /* a dock point the transport could not reach is avoided this long */

#define NODES (W2_MAP_MAX * W2_MAP_MAX)

/* ---- per transport ---- */
static uint16_t tr_pass[W2_MAX_UNITS][TR_CAP];   /* passenger slots, W2_NO_UNIT */
static uint16_t tr_pgen[W2_MAX_UNITS][TR_CAP];   /* their unit_gen */
static uint16_t tr_gen[W2_MAX_UNITS];            /* unit_gen of the transport the list belongs to */
static uint16_t tr_bad[W2_MAX_UNITS];            /* dock point it could not reach (y*128+x) ... */
static uint32_t tr_badc[W2_MAX_UNITS];           /* ... at cycle + 1 (skipped for TR_BAD_CYCLES) */
/* ---- per passenger ---- */
static uint16_t ps_tr[W2_MAX_UNITS];             /* transport carrying / to board, W2_NO_UNIT */
static uint16_t ps_trgen[W2_MAX_UNITS];
static uint16_t ps_gen[W2_MAX_UNITS];            /* unit_gen of the passenger the record belongs to */

/* ---- land regions, water bodies ---- */
static uint8_t  rg[NODES];
static uint8_t  wb[NODES];                        /* water body: connected tiles a docking ship may enter */
static uint16_t rg_q[NODES];
static int      rg_w, rg_h;
static uint32_t rg_ver, rg_cycle;
static unsigned rg_bld;                           /* building count the water bodies were made with */
static uint32_t g_cycle;                          /* to notice a new game (cycle counter restarts) */

void transport_reset(void)
{
	int s, k;
	for (s = 0; s < W2_MAX_UNITS; s++) {
		for (k = 0; k < TR_CAP; k++) tr_pass[s][k] = W2_NO_UNIT;
		ps_tr[s] = W2_NO_UNIT;
	}
	memset(tr_gen, 0, sizeof tr_gen); memset(ps_gen, 0, sizeof ps_gen); memset(tr_badc, 0, sizeof tr_badc);
	rg_w = 0; g_cycle = w2.cycle;
}

static void check_world(void)
{
	static int once;
	if (!once || w2.cycle < g_cycle) { once = 1; transport_reset(); }
	g_cycle = w2.cycle;
}

static int iabs(int v) { return v < 0 ? -v : v; }
static int on_map(int s)
{
	return s >= 0 && s < W2_MAX_UNITS && (w2.unit[s].flags & W2_UF_ALIVE) && !(w2.unit[s].flags & (W2_UF_INSIDE | W2_UF_HIDDEN));
}
static int walkable(int i) { return !(w2.map.flags[i] & W2_TF_NOWALK); }
static int sailable(int i)
{
	int x = i % w2.map.w, y = i / w2.map.w, mt = move_terrain(x, y);
	return !(mt & MT_LAND) && (mt & (MT_WATER | MT_SHALLOW)) && !(w2.map.flags[i] & W2_TF_BUILDING);
}

static unsigned buildings(void)
{
	unsigned n = 0; int p;
	for (p = 0; p < W2_MAX_PLAYERS; p++) n += w2.player[p].building_count;
	return n;
}

/* connected components (8 neighbours, no corner cutting, as units move) of tiles passing ok() */
static void label(uint8_t *lab, int (*ok)(int))
{
	int w = w2.map.w, h = w2.map.h, n = w * h, i, next = 1;
	memset(lab, 0, (size_t)n);
	for (i = 0; i < n; i++) {
		int head = 0, tail = 0, id;
		if (lab[i] || !ok(i)) continue;
		id = next < 255 ? next++ : 255;          /* 255: "some component" (more than 254) */
		lab[i] = (uint8_t)id; rg_q[tail++] = (uint16_t)i;
		while (head < tail) {
			int c = rg_q[head++], cx = c % w, cy = c / w, d;
			for (d = 0; d < 8; d++) {
				int nx = cx + w2_dir_dx[d], ny = cy + w2_dir_dy[d], j;
				if ((unsigned)nx >= (unsigned)w || (unsigned)ny >= (unsigned)h) continue;
				j = ny * w + nx;
				if (lab[j] || !ok(j)) continue;
				if ((d & 1) && (!ok(cy * w + nx) || !ok(ny * w + cx))) continue;   /* no corner cutting */
				lab[j] = (uint8_t)id; rg_q[tail++] = (uint16_t)j;
			}
		}
	}
}

/* relabel after the terrain (trees, walls) or the buildings (shipyards close bays) changed, at most
 * every TR_REGION_PERIOD cycles; at once for a new map */
static void rg_check(void)
{
	unsigned nb;
	check_world();
	nb = buildings();
	if (!(rg_w != w2.map.w || rg_h != w2.map.h || w2.cycle < rg_cycle) &&
	    ((rg_ver == w2.map.version && rg_bld == nb) || w2.cycle - rg_cycle < TR_REGION_PERIOD)) return;
	label(wb, sailable);
	label(rg, walkable);
	rg_w = w2.map.w; rg_h = w2.map.h; rg_ver = w2.map.version; rg_cycle = w2.cycle; rg_bld = nb;
}

/* water body of a tile a ship can be on, 0 none */
static int body(int tx, int ty)
{
	if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 0;
	rg_check();
	return wb[ty * w2.map.w + tx];
}

int transport_region(int tx, int ty)
{
	if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 0;
	rg_check();
	return rg[ty * w2.map.w + tx];
}

static int shallow(int tx, int ty) { return (move_terrain(tx, ty) & MT_SHALLOW) != 0; }

int transport_coast_of(int tx, int ty, int r)
{
	int d;
	rg_check();
	for (d = 0; d < 8; d++) {
		int nx = tx + w2_dir_dx[d], ny = ty + w2_dir_dy[d], v;
		if ((unsigned)nx >= (unsigned)w2.map.w || (unsigned)ny >= (unsigned)w2.map.h) continue;
		v = rg[ny * w2.map.w + nx];
		if (v && (r == 0 || v == r || v == 255 || r == 255)) return 1;
	}
	return 0;
}

/* the first land region bordering tile (tx,ty) (itself when it is land), 0 none */
static int region_near(int tx, int ty)
{
	int r = transport_region(tx, ty), d;
	if (r) return r;
	for (d = 0; d < 8; d++) {
		r = transport_region(tx + w2_dir_dx[d], ty + w2_dir_dy[d]);
		if (r) return r;
	}
	return 0;
}

/* ---- transport records ---- */
int transport_is(int s)
{
	const w2_unit_type *t;
	if (s < 0 || s >= W2_MAX_UNITS || !(w2.unit[s].flags & W2_UF_ALIVE)) return 0;
	t = w2_unit(w2.unit[s].type);
	return t && (t->flags & W2UF_TRANSPORT) && t->kind == W2_KIND_SEA;
}

/* the passenger list of t, dropping entries of units that are gone */
static void list_fix(int t)
{
	int k;
	check_world();
	if (tr_gen[t] != unit_gen(t)) {         /* a new unit in this slot: empty list */
		for (k = 0; k < TR_CAP; k++) tr_pass[t][k] = W2_NO_UNIT;
		tr_gen[t] = unit_gen(t);
	}
	for (k = 0; k < TR_CAP; k++) {
		int p = tr_pass[t][k];
		if (p == W2_NO_UNIT) continue;
		if (!unit_same(p, tr_pgen[t][k]) || ps_tr[p] != t || w2.unit[p].order != W2_ORD_LOAD) tr_pass[t][k] = W2_NO_UNIT;
	}
}

int transport_count(int t)
{
	int k, n = 0;
	if (!transport_is(t)) return 0;
	list_fix(t);
	for (k = 0; k < TR_CAP; k++) if (tr_pass[t][k] != W2_NO_UNIT) n++;
	return n;
}

int transport_list(int t, uint16_t *out)
{
	int k, n = 0;
	if (!transport_is(t)) return 0;
	list_fix(t);
	for (k = 0; k < TR_CAP; k++) if (tr_pass[t][k] != W2_NO_UNIT) out[n++] = tr_pass[t][k];
	return n;
}

static int ps_valid(int p)
{
	check_world();
	return p >= 0 && p < W2_MAX_UNITS && ps_tr[p] != W2_NO_UNIT && ps_gen[p] == unit_gen(p);
}

int transport_of(int p)
{
	if (!ps_valid(p) || !(w2.unit[p].flags & W2_UF_INSIDE)) return -1;
	return unit_same(ps_tr[p], ps_trgen[p]) ? ps_tr[p] : -1;
}

int transport_docked(int t)
{
	return on_map(t) && shallow(w2.unit[t].tx, w2.unit[t].ty);
}

int transport_waiting(int t)
{
	return transport_is(t) && on_map(t) && w2.unit[t].order == W2_ORD_LOAD && w2.unit[t].order_state == TRS_WAIT;
}

int transport_dock_goal(int t, int *tx, int *ty)
{
	const W2Unit *u;
	if (!transport_is(t)) return 0;
	u = &w2.unit[t];
	if (u->order != W2_ORD_LOAD && u->order != W2_ORD_UNLOAD) return 0;
	*tx = u->goal_tx; *ty = u->goal_ty;
	return 1;
}

/* ---- dock point search ---- */
static int dock_ok(int t, int x, int y, int r)
{
	int i, d, water = 0;
	unsigned o;
	if ((unsigned)x >= (unsigned)w2.map.w || (unsigned)y >= (unsigned)w2.map.h) return 0;
	if ((move_terrain(x, y) & (MT_SHALLOW | MT_LAND)) != MT_SHALLOW) return 0;
	i = y * w2.map.w + x;
	if (w2.map.flags[i] & W2_TF_BUILDING) return 0;
	o = w2.map.occ_ground[i];
	if (o != W2_NO_UNIT && (int)o != t) return 0;
	if (!transport_coast_of(x, y, r)) return 0;
	if (body(x, y) != body(w2.unit[t].tx, w2.unit[t].ty)) return 0;   /* another sea / lake / closed bay */
	if (tr_badc[t] && w2.cycle + 1 - tr_badc[t] < TR_BAD_CYCLES && tr_bad[t] == y * W2_MAP_MAX + x) return 0;
	for (d = 0; d < 8 && !water; d++) if (move_terrain(x + w2_dir_dx[d], y + w2_dir_dy[d]) & MT_WATER) water = 1;
	return water;
}

/* dock point for a transport sent to land/shallow tile (gx,gy); 1 = found. The Mac searches 8 rings
 * around the tile; for a tile further inland it first moves the goal to the end of the transport's
 * path toward it (0x0015144E, not decoded) - ours continues the ring search up to TR_SHORE_FAR. */
static int find_dock(int t, int gx, int gy, int *ox, int *oy)
{
	const W2Unit *u = &w2.unit[t];
	int r = region_near(gx, gy), ring;
	if (dock_ok(t, gx, gy, r)) { *ox = gx; *oy = gy; return 1; }
	for (ring = 1; ring <= TR_SHORE_FAR; ring++) {
		int x, y, best = -1, bd = 0x7FFFFFFF;
		for (y = gy - ring; y <= gy + ring; y++) {
			int step = (y == gy - ring || y == gy + ring) ? 1 : 2 * ring;
			for (x = gx - ring; x <= gx + ring; x += step) {
				int dx, dy, dd;
				if (!dock_ok(t, x, y, r)) continue;
				dx = x - u->tx; dy = y - u->ty; dd = dx * dx + dy * dy;
				if (dd < bd) { bd = dd; best = y * W2_MAP_MAX + x; }
			}
		}
		if (best >= 0) { *ox = best % W2_MAP_MAX; *oy = best / W2_MAP_MAX; return 1; }
	}
	return 0;
}

int transport_find_landing(int t, int tx, int ty, int *ox, int *oy)
{
	int r = transport_region(tx, ty), d0, k, b = body(w2.unit[t].tx, w2.unit[t].ty);
	if (!r) return 0;
	d0 = (int)(w2_rand() & 7);
	for (k = 0; k < 8; k++) {
		int d = (d0 + k) & 7, x = tx, y = ty;
		for (;;) {
			x += w2_dir_dx[d]; y += w2_dir_dy[d];
			if ((unsigned)x >= (unsigned)w2.map.w || (unsigned)y >= (unsigned)w2.map.h) break;
			if (shallow(x, y) && transport_coast_of(x, y, r) && body(x, y) == b) { *ox = x; *oy = y; return 1; }
		}
	}
	return 0;
}

/* ---- orders on the transport ---- */
static void to_wait(int t)
{
	W2Unit *u = &w2.unit[t];
	move_stop(t);
	u->order = W2_ORD_LOAD;
	u->order_state = TRS_WAIT;
	u->goal_tx = u->tx; u->goal_ty = u->ty;
	u->timer = 0;
}

int transport_cmd_dock(int t, int tx, int ty, int unload)
{
	W2Unit *u;
	int dx, dy, r;
	if (!transport_is(t) || !on_map(t)) return 0;
	if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 0;
	if (!(move_terrain(tx, ty) & (MT_LAND | MT_SHALLOW))) return 0;
	u = &w2.unit[t];
	r = region_near(tx, ty);
	/* already docked on that coast: stay (0x0014D61A); unload 2: that very dock point (AI retry) */
	if (unload == 2 && dock_ok(t, tx, ty, r)) { dx = tx; dy = ty; }
	else if (shallow(u->tx, u->ty) && transport_coast_of(u->tx, u->ty, r)) { dx = u->tx; dy = u->ty; }
	else if (!find_dock(t, tx, ty, &dx, &dy)) return 0;
	move_stop(t);
	unit_set_order(t, unload ? W2_ORD_UNLOAD : W2_ORD_LOAD, W2_NO_UNIT, dx, dy);
	u->order_state = TRS_DOCKING;
	move_set_dock(t, 1);
	return 1;
}

/* one passenger onto a free land tile next to the transport (0x0016C02A); 1 = placed */
static int drop_one(int t, int p)
{
	const W2Unit *ut = &w2.unit[t];
	int ring, x, y, tx = ut->tx, ty = ut->ty;
	for (ring = 1; ring <= TR_DROP_RING; ring++)
		for (y = ty - ring; y <= ty + ring; y++) {
			int step = (y == ty - ring || y == ty + ring) ? 1 : 2 * ring;
			for (x = tx - ring; x <= tx + ring; x += step) {
				int i, r;
				if ((unsigned)x >= (unsigned)w2.map.w || (unsigned)y >= (unsigned)w2.map.h) continue;
				i = y * w2.map.w + x;
				if (w2.map.flags[i] & (W2_TF_NOWALK | W2_TF_BUILDING)) continue;
				if (w2.map.occ_ground[i] != W2_NO_UNIT) continue;
				r = transport_region(x, y);
				if (!r || !transport_coast_of(tx, ty, r)) continue;
				{
					W2Unit *u = &w2.unit[p];
					u->flags &= (uint16_t)~W2_UF_INSIDE;
					u->ox = 0; u->oy = 0;
					unit_place(p, x, y);
					ps_tr[p] = W2_NO_UNIT;
					unit_set_order(p, W2_ORD_STILL, W2_NO_UNIT, x, y);
				}
				return 1;
			}
		}
	return 0;
}

static void unload_all(int t)
{
	int k;
	list_fix(t);
	if (!shallow(w2.unit[t].tx, w2.unit[t].ty)) return;   /* not docked: nothing leaves (f6_2A4C) */
	for (k = TR_CAP - 1; k >= 0; k--) {
		int p = tr_pass[t][k];
		if (p == W2_NO_UNIT) continue;
		if (drop_one(t, p)) tr_pass[t][k] = W2_NO_UNIT;
	}
}

static void transport_order(int t, int unload)
{
	W2Unit *u = &w2.unit[t];
	int r;
	if (!transport_is(t)) { unit_set_order(t, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty); return; }
	list_fix(t);
	if (u->order_state == TRS_WAIT) { move_tick(t); return; }   /* docked: nothing to do (Mac order 32) */
	if (u->order_state == TRS_START) {
		/* issued directly (UI Unload button: goal = the clicked tile): choose the dock point now */
		int gx = u->goal_tx, gy = u->goal_ty;
		if (!transport_cmd_dock(t, gx, gy, unload)) {
			if (unload) { move_stop(t); cmd_move(t, gx, gy); }   /* water / no coast: just sail there */
			else unit_set_order(t, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
			return;
		}
	}
	move_set_dock(t, 1);
	r = move_toward(t, u->goal_tx, u->goal_ty, 0);
	if (r == 0) return;
	if (r < 0 && !shallow(u->tx, u->ty)) {   /* could not dock (0x00151038: idle) */
		tr_bad[t] = (uint16_t)(u->goal_ty * W2_MAP_MAX + u->goal_tx); tr_badc[t] = w2.cycle + 1;   /* not that one again soon */
		move_set_dock(t, 0);
		unit_set_order(t, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
		return;
	}
	if (unload) unload_all(t);
	to_wait(t);
}

/* ---- orders on the passenger ---- */
static void deselect(int s)
{
	int i, j = 0, n = w2.select_n > W2_MAX_SELECT ? W2_MAX_SELECT : w2.select_n;
	for (i = 0; i < n; i++) if (w2.select[i] != s) w2.select[j++] = w2.select[i];
	w2.select_n = (uint8_t)j;
	w2.unit[s].flags &= (uint16_t)~W2_UF_SELECTED;
}

static int board(int p, int t)
{
	W2Unit *u = &w2.unit[p];
	int k;
	list_fix(t);
	for (k = 0; k < TR_CAP; k++) if (tr_pass[t][k] == W2_NO_UNIT) break;
	if (k >= TR_CAP) return 0;
	tr_pass[t][k] = (uint16_t)p; tr_pgen[t][k] = unit_gen(p);
	ps_tr[p] = (uint16_t)t; ps_trgen[p] = unit_gen(t); ps_gen[p] = unit_gen(p);
	move_stop(p);
	move_release(p);
	unit_unplace(p);
	deselect(p);
	u->flags |= W2_UF_INSIDE;
	u->tx = w2.unit[t].tx; u->ty = w2.unit[t].ty; u->ox = 0; u->oy = 0;
	u->order = W2_ORD_LOAD; u->order_state = TRS_INSIDE;
	unit_set_target(p, t);
	return 1;
}

static void passenger_order(int p)
{
	W2Unit *u = &w2.unit[p];
	int t = u->target, rp, gx, gy;
	const W2Unit *ut;
	if (u->order_state == TRS_INSIDE) { transport_inside_tick(p); return; }
	if (t == W2_NO_UNIT || !unit_target_same(p) || !transport_is(t) || !on_map(t) || w2.unit[t].owner != u->owner) {
		unit_set_order(p, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
		return;
	}
	ut = &w2.unit[t];
	if (!move_is_stepping(p) && iabs(ut->tx - u->tx) <= TR_REACH && iabs(ut->ty - u->ty) <= TR_REACH && transport_waiting(t)) {
		if (board(p, t)) return;
		move_stop(p);                    /* full: wait here (the Mac keeps the order) */
		return;
	}
	if (u->timer) { u->timer--; move_tick(p); return; }
	/* walk to the transport when it is at our coast, else to where it docks (0x00150AA6) */
	rp = transport_region(u->tx, u->ty);
	gx = ut->tx; gy = ut->ty;
	if (!(shallow(gx, gy) && transport_coast_of(gx, gy, rp))) {
		int dx, dy;
		if (!transport_dock_goal(t, &dx, &dy) || !transport_coast_of(dx, dy, rp)) {
			if (!(ut->order == W2_ORD_LOAD && ut->order_state == TRS_START)) {   /* not coming here: give up */
				unit_set_order(p, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
				return;
			}
			move_stop(p); u->timer = 16;          /* just ordered: look again in a moment */
			return;
		}
		gx = dx; gy = dy;
	}
	if (move_toward(p, gx, gy, 1) < 0) u->timer = 30;    /* blocked for now: try again */
}

void transport_inside_tick(int p)
{
	W2Unit *u = &w2.unit[p];
	int t;
	if (!(u->flags & W2_UF_ALIVE)) return;
	if (!ps_valid(p)) return;                /* inside something else (mine, building) */
	t = ps_tr[p];
	if (!unit_same(t, ps_trgen[p])) { ps_tr[p] = W2_NO_UNIT; unit_kill(p); return; }   /* went down with it */
	{
		int k;
		for (k = 0; k < TR_CAP; k++) if (tr_pass[t][k] == p && tr_pgen[t][k] == ps_gen[p]) break;
		if (k >= TR_CAP) { ps_tr[p] = W2_NO_UNIT; return; }   /* stale record: not in that transport */
	}
	u->tx = w2.unit[t].tx; u->ty = w2.unit[t].ty;
	if (u->order != W2_ORD_LOAD || u->order_state != TRS_INSIDE) {   /* someone gave an inside unit an order */
		u->order = W2_ORD_LOAD; u->order_state = TRS_INSIDE;
		unit_set_target(p, t);
	}
}

/* ---- w2.h entry points ---- */
void ord_load(int slot)
{
	if (transport_is(slot)) transport_order(slot, 0);
	else passenger_order(slot);
}

void ord_unload(int slot)
{
	if (transport_is(slot)) transport_order(slot, 1);
	else unit_set_order(slot, W2_ORD_STILL, W2_NO_UNIT, w2.unit[slot].tx, w2.unit[slot].ty);
}

static int land_unit(int s)
{
	const w2_unit_type *t = w2_unit(w2.unit[s].type);
	return t && t->kind == W2_KIND_LAND && !(w2.unit[s].flags & W2_UF_BUILDING);
}

void transport_cmd_board(int p, int t)
{
	if (!on_map(p) || !land_unit(p) || !transport_is(t) || !on_map(t) || w2.unit[t].owner != w2.unit[p].owner) return;
	move_stop(p);
	unit_set_order(p, W2_ORD_LOAD, t, w2.unit[t].tx, w2.unit[t].ty);
	ps_tr[p] = W2_NO_UNIT;                   /* the carried record is made when it boards */
	/* the transport comes to dock near the unit unless it is docking already (0x0014D74A) */
	if (!(w2.unit[t].order == W2_ORD_LOAD && w2.unit[t].order_state == TRS_DOCKING) && w2.unit[t].order != W2_ORD_UNLOAD)
		transport_cmd_dock(t, w2.unit[p].tx, w2.unit[p].ty, 0);
}

int transport_smart(int slot, int tx, int ty, int target)
{
	if (slot < 0 || slot >= W2_MAX_UNITS || !on_map(slot)) return 0;
	if (target >= 0 && target < W2_MAX_UNITS && target != slot && transport_is(target) && on_map(target) &&
	    w2.unit[target].owner == w2.unit[slot].owner && land_unit(slot)) {
		transport_cmd_board(slot, target);
		return 1;
	}
	if (transport_is(slot)) {
		/* a unit clicked: its tile (0x0016C166); land or coast: dock there and wait */
		if (target >= 0 && target < W2_MAX_UNITS && on_map(target)) {
			if (w2.unit[target].flags & W2_UF_BUILDING) return 0;
			tx = w2.unit[target].tx; ty = w2.unit[target].ty;
		}
		if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 0;
		if (!(move_terrain(tx, ty) & (MT_LAND | MT_SHALLOW))) return 0;   /* water: a plain move */
		return transport_cmd_dock(slot, tx, ty, 0);
	}
	return 0;
}
