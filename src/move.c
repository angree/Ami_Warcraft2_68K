/* move.c - unit movement: tile steps driven by the MOVE animation, path following with waiting,
 * repathing and giving up, the movement orders (move/follow/stand/still) and cmd_move/cmd_follow.
 * Model and integration duties: see move.h. Path search: path.c.
 *
 * Per unit (module-private, indexed by slot):
 *  - step state: stepping flag, direction, pixels done (derived from unit.c's MOVE animation state),
 *    the destination tile (held in the occupancy map from the start of the step);
 *  - path state: up to MV_PATH_MAX planned directions, the tile where the next one starts, the goal
 *    (rectangle + range + target), wait / failure / retry counters.
 *
 * Blocked next step:
 *  - by a unit that is walking (stepped recently): wait (no failure), after MV_WAIT_LONG cycles repath;
 *  - by a unit that is waiting itself: wait a few cycles (slot-staggered), then repath (+1 failure);
 *  - by a standing unit / building / changed terrain: repath at once (+1 failure).
 * A search that cannot bring the unit closer costs a failure and a short back-off; MV_GIVEUP failures
 * in a row (or an exhaustive search with no moving unit involved) -> unreachable, move_toward() = -1.
 * A taken step clears the failures.
 *
 * Search spreading: at most move_search_quota searches per cycle; other requests wait in a FIFO and
 * are granted on the following cycles (oldest first).
 */
#include <string.h>
#include "w2.h"
#include "move.h"
#include "unit.h"
#include "combat.h"

#define MV_PATH_MAX   64      /* planned steps kept per unit; a longer route is re-planned at the end */
#define MV_GIVEUP     8       /* failures in a row before giving up */
#define MV_WAIT_LONG  32      /* max cycles behind a walking unit before trying another way */
#define MV_PROGRESS   24      /* a unit that started a step within this many cycles is "walking" */
#define MV_REPLAN_TRACK 24    /* re-plan toward a moving target unit at most this often */
#define MV_DETOUR     8       /* a way around a blocking walker may be this many steps longer */

int move_search_quota = 3;

/* ---- step state ---- */
static uint8_t  st_on[W2_MAX_UNITS];      /* MOVE sequence playing */
static uint8_t  st_dir[W2_MAX_UNITS];
static uint8_t  st_px[W2_MAX_UNITS];
static uint8_t  st_dx[W2_MAX_UNITS], st_dy[W2_MAX_UNITS];   /* destination tile */
static uint32_t st_tick[W2_MAX_UNITS];    /* cycle + 1 of the last move_tick */
static uint32_t st_last[W2_MAX_UNITS];    /* cycle + 1 of the last step start */

/* ---- path state ---- */
static uint8_t  mv_dirs[W2_MAX_UNITS][MV_PATH_MAX];
static uint8_t  mv_len[W2_MAX_UNITS], mv_idx[W2_MAX_UNITS];
static uint8_t  mv_active[W2_MAX_UNITS];
static uint8_t  mv_ex[W2_MAX_UNITS], mv_ey[W2_MAX_UNITS];   /* tile where mv_dirs[mv_idx] starts */
static uint8_t  mv_gx[W2_MAX_UNITS], mv_gy[W2_MAX_UNITS], mv_gw[W2_MAX_UNITS], mv_gh[W2_MAX_UNITS], mv_gr[W2_MAX_UNITS];
static uint16_t mv_target[W2_MAX_UNITS];
static uint8_t  mv_wait[W2_MAX_UNITS], mv_fail[W2_MAX_UNITS], mv_retry[W2_MAX_UNITS];
static uint32_t mv_plan[W2_MAX_UNITS];    /* cycle of the last search */
static uint8_t  mv_detour[W2_MAX_UNITS];  /* re-plan around a blocking walker; keep the old path if shorter */
static uint8_t  s_tmp[MV_PATH_MAX];

/* ---- search request FIFO ---- */
static uint16_t q_slot[W2_MAX_UNITS];
static int      q_head, q_n;
static uint8_t  q_in[W2_MAX_UNITS];
static uint32_t q_req[W2_MAX_UNITS];      /* cycle + 1 of the last request */
static uint32_t q_grant[W2_MAX_UNITS];    /* cycle + 1 the grant is valid for */
static uint32_t q_cycle;                  /* cycle + 1 the counters below belong to */
static int      q_used, q_popped;
static uint32_t sw_cycle;                 /* cycle + 1 of the last reservation sweep */

void move_reset_all(void)
{
	memset(st_on, 0, sizeof st_on); sw_cycle = 0;
	memset(st_tick, 0, sizeof st_tick); memset(st_last, 0, sizeof st_last);
	memset(mv_len, 0, sizeof mv_len); memset(mv_idx, 0, sizeof mv_idx); memset(mv_active, 0, sizeof mv_active);
	memset(mv_wait, 0, sizeof mv_wait); memset(mv_detour, 0, sizeof mv_detour); memset(mv_fail, 0, sizeof mv_fail); memset(mv_retry, 0, sizeof mv_retry);
	memset(q_in, 0, sizeof q_in); memset(q_req, 0, sizeof q_req); memset(q_grant, 0, sizeof q_grant);
	q_head = q_n = 0; q_cycle = 0; q_used = q_popped = 0;
	memset(&path_stats, 0, sizeof path_stats);
}

static int on_map(int slot)
{
	unsigned f = w2.unit[slot].flags;
	return (f & W2_UF_ALIVE) && !(f & (W2_UF_INSIDE | W2_UF_HIDDEN));
}

static uint16_t *occ_of(int slot) { return move_class(slot) == MC_AIR ? w2.map.occ_air : w2.map.occ_ground; }

int move_is_active(int slot) { return mv_active[slot] || st_on[slot]; }
int move_is_stepping(int slot) { return st_on[slot]; }
int move_is_progressing(int slot)
{
	return st_on[slot] || (st_last[slot] && w2.cycle + 1 - st_last[slot] <= MV_PROGRESS);
}

int move_reserved(int slot, int *tx, int *ty)
{
	if (!st_on[slot]) return 0;
	*tx = st_dx[slot]; *ty = st_dy[slot];
	return 1;
}

int move_path_get(int slot, uint8_t *dirs, int max)
{
	int n = 0, i;
	if (!mv_active[slot]) return 0;
	for (i = mv_idx[slot]; i < mv_len[slot] && n < max; i++) dirs[n++] = mv_dirs[slot][i];
	return n;
}

/* ---- animation (played by unit.c; this module only reads its progress) ---- */

static void set_still(int slot)
{
	if (w2.unit[slot].anim_kind != W2_ANIM_STILL) unit_set_anim(slot, W2_ANIM_STILL);
}

/* Pixels of the current tile step done so far, from the animation state: the sum of `arg` of the
 * MOVE steps entered in this loop (unit.c applies the same rule to unit_anim_move_pixels). Derived from
 * state, not accumulated per cycle, so a cycle without a move_* call loses nothing. -1 when the MOVE
 * animation is no longer playing (replaced by another animation or wrapped into STILL). */
static int anim_px(int slot)
{
	const W2Unit *u = &w2.unit[slot];
	unsigned n, i; int p = 0;
	const w2_anim_step *a = w2_unit_anim(u->type, W2_ANIM_MOVE, &n);
	if (!a || !n) {                         /* type without a MOVE animation: 4 px per cycle */
		if (st_tick[slot] != w2.cycle + 1) { st_tick[slot] = w2.cycle + 1; return st_px[slot] + 4; }
		return st_px[slot];
	}
	if (u->anim_kind != W2_ANIM_MOVE) return -1;
	if (u->anim_wait == 0) return 0;        /* restarted, step 0 not entered yet */
	for (i = 0; i <= u->anim_step && i < n; i++)
		if (!(a[i].flags & (W2AF_SOUND | W2AF_IDLE_TURN))) p += a[i].arg;
	return p;
}

static void tile_reached(int slot)
{
	W2Unit *u = &w2.unit[slot];
	uint16_t *occ = occ_of(slot);
	int i = u->ty * w2.map.w + u->tx;
	if (occ[i] == (uint16_t)slot) occ[i] = W2_NO_UNIT;
	u->tx = st_dx[slot]; u->ty = st_dy[slot];
	u->ox = 0; u->oy = 0;
	st_on[slot] = 0;
}

/* once per cycle: drop reservations of units that left the map without move_release (death,
 * removal, entering a mine/transport) */
static void sweep(void)
{
	int s;
	if (sw_cycle == w2.cycle + 1) return;
	sw_cycle = w2.cycle + 1;
	for (s = 0; s < w2.unit_hi; s++) if ((st_on[s] || mv_active[s]) && !on_map(s)) move_release(s);
}

void move_tick(int slot)
{
	int p, d;
	W2Unit *u = &w2.unit[slot];
	sweep();
	if (!st_on[slot]) return;
	if (!on_map(slot)) { move_release(slot); return; }
	p = anim_px(slot);
	if (p < 0 || p < st_px[slot] || p >= W2_TILE) { tile_reached(slot); return; }   /* done / wrapped / stopped */
	st_px[slot] = (uint8_t)p;
	d = st_dir[slot];
	u->ox = (int8_t)(w2_dir_dx[d] * p);
	u->oy = (int8_t)(w2_dir_dy[d] * p);
}

static void step_start(int slot, int d)
{
	W2Unit *u = &w2.unit[slot];
	int nx = u->tx + w2_dir_dx[d], ny = u->ty + w2_dir_dy[d];
	occ_of(slot)[ny * w2.map.w + nx] = (uint16_t)slot;
	st_on[slot] = 1; st_dir[slot] = (uint8_t)d; st_px[slot] = 0;
	st_dx[slot] = (uint8_t)nx; st_dy[slot] = (uint8_t)ny;
	st_last[slot] = w2.cycle + 1;
	st_tick[slot] = w2.cycle + 1;
	u->dir = (uint8_t)d;
	unit_set_anim(slot, W2_ANIM_MOVE);      /* step 0 is entered (pixels moved) at the next unit tick */
}

void move_release(int slot)
{
	if (st_on[slot]) {
		uint16_t *occ = occ_of(slot);
		int i = st_dy[slot] * w2.map.w + st_dx[slot];
		if (occ[i] == (uint16_t)slot) occ[i] = W2_NO_UNIT;
		w2.unit[slot].ox = 0; w2.unit[slot].oy = 0;
	}
	st_on[slot] = 0;
	mv_active[slot] = 0; mv_len[slot] = 0; mv_idx[slot] = 0;
	mv_wait[slot] = 0; mv_fail[slot] = 0; mv_retry[slot] = 0; mv_detour[slot] = 0;
	st_last[slot] = 0;
}

/* ---- search quota ---- */
static int search_granted(int slot)
{
	uint32_t c = w2.cycle + 1;
	if (q_cycle != c) {                     /* first request of a new cycle: grant the oldest */
		q_cycle = c; q_used = 0; q_popped = 0;
		while (q_n > 0 && q_popped < move_search_quota) {
			int s = q_slot[q_head];
			q_head = (q_head + 1) % W2_MAX_UNITS; q_n--;
			q_in[s] = 0;
			if (q_req[s] + 1 < c || !mv_active[s]) continue;   /* stale request */
			q_grant[s] = c; q_popped++;
		}
	}
	if (q_grant[slot] == c) { q_grant[slot] = 0; return 1; }
	q_req[slot] = c;
	if (q_in[slot]) return 0;
	if (q_n == 0 && q_popped + q_used < move_search_quota) { q_used++; return 1; }
	q_slot[(q_head + q_n) % W2_MAX_UNITS] = (uint16_t)slot; q_n++; q_in[slot] = 1;
	path_stats.deferred++;
	return 0;
}

/* ---- path following ---- */
static int in_range(int slot)
{
	const W2Unit *u = &w2.unit[slot];
	int r = mv_gr[slot], dx = 0, dy = 0;
	int x0 = mv_gx[slot], y0 = mv_gy[slot], x1 = x0 + mv_gw[slot] - 1, y1 = y0 + mv_gh[slot] - 1;
	if (u->tx < x0) dx = x0 - u->tx; else if (u->tx > x1) dx = u->tx - x1;
	if (u->ty < y0) dy = y0 - u->ty; else if (u->ty > y1) dy = u->ty - y1;
	return dx <= r && dy <= r;
}

static void finish(int slot)
{
	mv_active[slot] = 0; mv_len[slot] = 0; mv_idx[slot] = 0;
	mv_wait[slot] = 0; mv_fail[slot] = 0; mv_retry[slot] = 0; mv_detour[slot] = 0;
	if (q_in[slot]) q_req[slot] = 0;        /* leaves the queue as stale */
	if (!st_on[slot]) set_still(slot);
}

static int fail(int slot)
{
	mv_len[slot] = 0;
	if (++mv_fail[slot] >= MV_GIVEUP) { finish(slot); return -1; }
	mv_retry[slot] = (uint8_t)(3 + (slot & 7));
	return 0;
}

static int goal_step(int slot, int gx, int gy, int gw, int gh, int range, int target)
{
	W2Unit *u = &w2.unit[slot];
	uint16_t *occ;
	int d, nx, ny, mc, o;

	if (!on_map(slot)) { move_release(slot); return -1; }
	move_tick(slot);
	if (gx < 0) gx = 0;
	if (gy < 0) gy = 0;
	if (gx >= w2.map.w) gx = w2.map.w - 1;
	if (gy >= w2.map.h) gy = w2.map.h - 1;
	if (range < 0) range = 0;

	/* goal bookkeeping (a new goal drops the old plan; a moving target re-plans now and then) */
	if (!mv_active[slot] || mv_gr[slot] != range || mv_target[slot] != (uint16_t)target ||
	    (target == W2_NO_UNIT && (mv_gx[slot] != gx || mv_gy[slot] != gy || mv_gw[slot] != gw || mv_gh[slot] != gh))) {
		mv_active[slot] = 1; mv_len[slot] = 0; mv_idx[slot] = 0;
		mv_wait[slot] = 0; mv_fail[slot] = 0; mv_retry[slot] = 0; mv_detour[slot] = 0;
	} else if (mv_gx[slot] != gx || mv_gy[slot] != gy) {
		if (w2.cycle - mv_plan[slot] >= MV_REPLAN_TRACK) mv_len[slot] = 0;
	}
	mv_gx[slot] = (uint8_t)gx; mv_gy[slot] = (uint8_t)gy; mv_gw[slot] = (uint8_t)gw; mv_gh[slot] = (uint8_t)gh;
	mv_gr[slot] = (uint8_t)range; mv_target[slot] = (uint16_t)target;

	if (st_on[slot]) return 0;               /* inside a step (or its tail) */
	if (in_range(slot)) { finish(slot); return 1; }
	if (mv_retry[slot]) { mv_retry[slot]--; set_still(slot); return 0; }

	mc = move_class(slot);
	occ = occ_of(slot);
	if (mv_idx[slot] >= mv_len[slot] || mv_ex[slot] != u->tx || mv_ey[slot] != u->ty || mv_detour[slot]) {
		int n, fl = 0, rem = -1;
		if (!search_granted(slot)) { set_still(slot); return 0; }
		if (mv_detour[slot] && mv_idx[slot] < mv_len[slot] && mv_ex[slot] == u->tx && mv_ey[slot] == u->ty)
			rem = mv_len[slot] - mv_idx[slot];
		mv_detour[slot] = 0;
		n = path_search(slot, gx, gy, gw, gh, range, s_tmp, MV_PATH_MAX, &fl);
		mv_plan[slot] = w2.cycle;
		if (rem >= 0 && (n < 0 || n > rem + MV_DETOUR)) { set_still(slot); return 0; }   /* rather wait */
		if (n <= 0) {
			set_still(slot);
			if (n == 0) { finish(slot); return 1; }
			if ((fl & PATH_EXHAUSTED) && !(fl & PATH_MOVERBLOCK)) { finish(slot); return -1; }
			return fail(slot);
		}
		memcpy(mv_dirs[slot], s_tmp, (size_t)n);
		mv_len[slot] = (uint8_t)n; mv_idx[slot] = 0; mv_ex[slot] = u->tx; mv_ey[slot] = u->ty;
		mv_wait[slot] = 0;
	}

	d = mv_dirs[slot][mv_idx[slot]];
	nx = u->tx + w2_dir_dx[d]; ny = u->ty + w2_dir_dy[d];
	if (!move_tile_ok(mc, nx, ny) || !move_diag_ok(mc, u->tx, u->ty, d)) {   /* terrain changed */
		set_still(slot);
		return fail(slot);
	}
	o = occ[ny * w2.map.w + nx];
	if (o != W2_NO_UNIT && o != slot) {
		set_still(slot);
		u->dir = (uint8_t)d;
		if (!(w2.unit[o].flags & W2_UF_BUILDING) && move_is_active(o)) {
			int prog = move_is_progressing(o);
			if (++mv_wait[slot] < (prog ? MV_WAIT_LONG : 2 + (slot & 3))) return 0;
			mv_wait[slot] = 0;
			if (prog) { mv_detour[slot] = 1; return 0; }   /* way around? keep this path if that is long */
			if (++mv_fail[slot] >= MV_GIVEUP) { finish(slot); return -1; }
			mv_len[slot] = 0;                /* the blocker is stuck too: any way around */
			return 0;
		}
		return fail(slot);
	}
	mv_idx[slot]++;
	mv_ex[slot] = (uint8_t)nx; mv_ey[slot] = (uint8_t)ny;
	mv_wait[slot] = 0; mv_fail[slot] = 0;
	step_start(slot, d);
	return 0;
}

int move_toward(int slot, int gx, int gy, int range)
{
	return goal_step(slot, gx, gy, 1, 1, range, W2_NO_UNIT);
}

int move_toward_unit(int slot, int target, int range)
{
	const W2Unit *t;
	const struct w2_unit_type *ty;
	int w = 1, h = 1;
	if (target < 0 || target >= W2_MAX_UNITS || !on_map(target)) { move_stop(slot); return -1; }
	t = &w2.unit[target];
	ty = w2_unit(t->type);
	if (ty && (t->flags & W2_UF_BUILDING)) { w = ty->tile_w; h = ty->tile_h; }
	if (range < 1) range = 1;               /* cannot stand on another unit */
	return goal_step(slot, t->tx, t->ty, w, h, range, target);
}

void move_stop(int slot)
{
	mv_active[slot] = 0; mv_len[slot] = 0; mv_idx[slot] = 0;
	mv_wait[slot] = 0; mv_fail[slot] = 0; mv_retry[slot] = 0; mv_detour[slot] = 0;
	if (!st_on[slot]) set_still(slot);
}

/* ---- orders ---- */
static int armed(int slot)
{
	const struct w2_unit_type *t = w2_unit(w2.unit[slot].type);
	return t && (t->basic_damage || t->piercing_damage) && t->can_target;
}

void ord_move(int slot)
{
	W2Unit *u = &w2.unit[slot];
	int r = move_toward(slot, u->goal_tx, u->goal_ty, 0);
	if (r != 0) unit_set_order(slot, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
}

void ord_follow(int slot)
{
	W2Unit *u = &w2.unit[slot];
	int t = u->target, r;
	if (t == W2_NO_UNIT || t == slot || !on_map(t)) {
		move_stop(slot);
		unit_set_order(slot, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
		return;
	}
	if (u->timer) { u->timer--; move_tick(slot); return; }
	r = move_toward_unit(slot, t, 1);
	if (r == 1) u->timer = 8;               /* next to it: look again in a moment */
	else if (r < 0) u->timer = 30;          /* cannot reach now: retry later, keep the order */
}

void ord_stand(int slot)
{
	if (mv_active[slot]) move_stop(slot);
	move_tick(slot);
	if (!st_on[slot]) set_still(slot);
	if (!st_on[slot] && armed(slot) && unit_thinks(slot)) {    /* combat_engage keeps STAND on its tile */
		int t = combat_acquire(slot);
		if (t >= 0) combat_engage(slot, t);
	}
}

void ord_still(int slot)
{
	if (mv_active[slot]) move_stop(slot);
	move_tick(slot);
	if (st_on[slot]) return;
	set_still(slot);
	if (armed(slot) && unit_thinks(slot)) {
		int t = combat_acquire(slot);
		if (t >= 0) combat_engage(slot, t);
	}
}

/* ---- commands ---- */
static int mobile(int slot)
{
	return slot >= 0 && slot < W2_MAX_UNITS && on_map(slot) &&
	       !(w2.unit[slot].flags & (W2_UF_BUILDING | W2_UF_DYING));
}

void cmd_move(int slot, int tx, int ty)
{
	if (!mobile(slot)) return;
	if (tx < 0) tx = 0;
	if (ty < 0) ty = 0;
	if (tx >= w2.map.w) tx = w2.map.w - 1;
	if (ty >= w2.map.h) ty = w2.map.h - 1;
	mv_len[slot] = 0;                       /* force a fresh plan even for the same goal */
	mv_active[slot] = 0;
	unit_set_order(slot, W2_ORD_MOVE, W2_NO_UNIT, tx, ty);
}

void cmd_follow(int slot, int target)
{
	if (!mobile(slot) || target == slot || target < 0 || target >= W2_MAX_UNITS || !on_map(target)) return;
	mv_len[slot] = 0;
	mv_active[slot] = 0;
	unit_set_order(slot, W2_ORD_FOLLOW, target, 0, 0);
	w2.unit[slot].timer = 0;
}
