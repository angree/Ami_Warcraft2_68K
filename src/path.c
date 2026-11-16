/* path.c - A* path search on the tile grid (8 directions, no corner cutting), static memory only.
 *
 * Costs: straight step 10, diagonal 11. In the original a diagonal step takes as many cycles as a
 * straight one (the MOVE animation moves both axes), so the true time cost is uniform; the +1 only
 * breaks ties toward straighter paths. Heuristic 10*max(dx,dy) + min(dx,dy) (consistent for these costs),
 * measured to the goal rectangle shrunk by `range`.
 *
 * Search nodes use a fixed 128-wide index (y<<7 | x); map arrays use y*w + x (row table).
 * Open list: binary min-heap of 32-bit keys (f<<16 | node) with lazy duplicates (a node whose cost
 * improves is pushed again; stale entries are skipped when popped as closed).
 * Budget: path_node_budget expansions; then the best node seen (smallest heuristic, then smallest g)
 * becomes the end of a partial path toward the goal.
 *
 * Other units: buildings and units without a movement goal block everywhere. Units that are moving
 * (move_is_active) block only on tiles adjacent to the start - far away they will have moved on, and
 * treating them as walls makes groups gridlock.
 *
 * Ships (Mac 1.23: movement types at unit+0x2A, blocking masks A5+1714 = land 0x09CE, air 0x0200,
 * ship 0x0903, docking ship 0x0901; doubled direction tables A5-3FA0/-3F98 selected by unit+0x1C bit
 * value 2, Path_TakeStep 0x0016A896 tests only the destination tile):
 *  - a ship on even coordinates moves in 2-tile steps and searches the even grid (node = x,y both
 *    even, neighbour = x+2dx,y+2dy, only that tile is tested - no corner rule, no middle tile);
 *    a ship at odd coordinates first takes one single step to an even water tile (Mac undock search
 *    0x00150D36 / docking mode 0x00151038 use single steps off the even grid);
 *  - MC_DOCK (a transport docking, set by transport.c with move_set_dock): mask 0x0901, i.e. any tile
 *    without the SQM land bit 0x01 (water 0x40 AND shallow 0x02) - 2-tile steps only when the start
 *    and a 1x1 goal are both even, else single steps (0x00151038);
 *  - a 2-tile step is encoded in the returned directions as dir | PATH_STEP2.
 * The SQM bits come from the tileset group flags (equal to SQM on all maps, see pud.c) and are cached
 * per map in s_terr (MT_* bits; land/shallow/water never change in play: a cut tree or a destroyed
 * wall/rock stays land).
 */
#include <string.h>
#include "w2.h"
#include "move.h"
#include "assets.h"

const int8_t w2_dir_dx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
const int8_t w2_dir_dy[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

int path_node_budget = 4096;
PathStats path_stats;

#define NODES     (W2_MAP_MAX * W2_MAP_MAX)
#define HEAP_MAX  8192
#define CLOSED    0x80

static uint16_t s_stamp[NODES];   /* == s_cur when the node was touched by this search */
static uint16_t s_g[NODES];
static uint8_t  s_info[NODES];    /* CLOSED | direction of the step that entered this node */
static uint32_t s_heap[HEAP_MAX];
static int      s_heapn;
static uint16_t s_cur;
static int      s_row[W2_MAP_MAX];   /* y * map.w */
static int      s_row_w = -1;

/* search parameters (file-static to keep the inner loop's argument list short) */
static int s_gx0, s_gy0, s_gx1, s_gy1;   /* goal rectangle grown by range (inclusive) */

/* ---- SQM terrain bits (MT_*) per tile, built once per map ---- */
static uint8_t  s_terr[NODES];
static int      s_terr_w, s_terr_h;      /* map size it was built for, 0 = not built */
static uint32_t s_terr_cycle;            /* w2.cycle when built (a new game restarts the cycle count) */
static uint8_t  s_dock[W2_MAX_UNITS];    /* docking mode (transport.c) */

void path_terrain_reset(void) { s_terr_w = 0; memset(s_dock, 0, sizeof s_dock); }

static void terr_build(void)
{
	int i, n = w2.map.w * w2.map.h;
	for (i = 0; i < n; i++) {
		unsigned gf = assets_tile_group_flags(w2.map.tile[i]), b = 0;
		if (gf & 0x01) b |= MT_LAND;
		if (gf & 0x02) b |= MT_SHALLOW;
		if (gf & 0x40) b |= MT_WATER;
		if (!gf) b = (w2.map.flags[i] & W2_TF_WATER) ? MT_WATER : MT_LAND;   /* no tile map: our flags */
		s_terr[i] = (uint8_t)b;
	}
	s_terr_w = w2.map.w; s_terr_h = w2.map.h; s_terr_cycle = w2.cycle;
}

static void terr_check(void)
{
	if (s_terr_w != w2.map.w || s_terr_h != w2.map.h || w2.cycle < s_terr_cycle) terr_build();
}

int move_terrain(int tx, int ty)
{
	if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 0;
	terr_check();
	return s_terr[ty * w2.map.w + tx];
}

void move_set_dock(int slot, int on) { if (slot >= 0 && slot < W2_MAX_UNITS) s_dock[slot] = (uint8_t)(on != 0); }
int  move_get_dock(int slot) { return slot >= 0 && slot < W2_MAX_UNITS && s_dock[slot]; }

static int heur(int x, int y)
{
	int dx = 0, dy = 0;
	if (x < s_gx0) dx = s_gx0 - x; else if (x > s_gx1) dx = x - s_gx1;
	if (y < s_gy0) dy = s_gy0 - y; else if (y > s_gy1) dy = y - s_gy1;
	return dx > dy ? dx * 10 + dy : dy * 10 + dx;
}

static void heap_push(uint32_t key)
{
	int i = s_heapn++;
	while (i > 0) {
		int p = (i - 1) >> 1;
		if (s_heap[p] <= key) break;
		s_heap[i] = s_heap[p];
		i = p;
	}
	s_heap[i] = key;
}

static uint32_t heap_pop(void)
{
	uint32_t top = s_heap[0], last = s_heap[--s_heapn];
	int i = 0, n = s_heapn;
	for (;;) {
		int c = 2 * i + 1;
		if (c >= n) break;
		if (c + 1 < n && s_heap[c + 1] < s_heap[c]) c++;
		if (last <= s_heap[c]) break;
		s_heap[i] = s_heap[c];
		i = c;
	}
	if (n > 0) s_heap[i] = last;
	return top;
}

int move_class(int slot)
{
	const W2Unit *u = &w2.unit[slot];
	const struct w2_unit_type *t;
	if (u->flags & W2_UF_AIR) return MC_AIR;
	t = (u->flags & W2_UF_SEA) ? NULL : w2_unit(u->type);
	if (t && t->kind == W2_KIND_AIR) return MC_AIR;
	if (!(u->flags & W2_UF_SEA) && !(t && t->kind == W2_KIND_SEA)) return MC_LAND;
	/* docking mode holds while the transport runs a transport order; a ship standing on a shallow
	 * tile (docked) leaves it in docking mode too (Mac order 31 undock, movement type 3) */
	if (s_dock[slot] && (u->order == W2_ORD_LOAD || u->order == W2_ORD_UNLOAD)) return MC_DOCK;
	terr_check();
	if (u->tx < w2.map.w && u->ty < w2.map.h && !(s_terr[u->ty * w2.map.w + u->tx] & MT_WATER)) return MC_DOCK;
	return MC_SEA;
}

int move_tile_ok(int mc, int tx, int ty)
{
	unsigned f;
	if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 0;
	if (mc == MC_AIR) return 1;
	f = w2.map.flags[ty * w2.map.w + tx];
	if (f & W2_TF_BUILDING) return 0;
	if (mc == MC_SEA) return (f & W2_TF_WATER) != 0;
	if (mc == MC_DOCK) { terr_check(); return !(s_terr[ty * w2.map.w + tx] & MT_LAND); }
	return (f & W2_TF_NOWALK) == 0;
}

int move_diag_ok(int mc, int tx, int ty, int dir)
{
	if (!(dir & 1) || mc == MC_AIR) return 1;
	return move_tile_ok(mc, tx + w2_dir_dx[dir], ty) && move_tile_ok(mc, tx, ty + w2_dir_dy[dir]);
}

/* tile passable for a class, for the search's inner loop (buildings, terrain; not units) */
static int pass_ok(int mc, int i)
{
	unsigned f = w2.map.flags[i];
	if (f & W2_TF_BUILDING) return 0;
	switch (mc) {
	case MC_SEA:  return (f & W2_TF_WATER) != 0;
	case MC_DOCK: return !(s_terr[i] & MT_LAND);
	case MC_AIR:  return 1;
	default:      return (f & W2_TF_NOWALK) == 0;
	}
}

/* a ship at odd coordinates: the best single step onto an even (x,y) water tile, -1 none
 * (closest to the goal rectangle; free of other units) */
static int even_entry(int slot, int mc, int sx, int sy, const uint16_t *occ)
{
	int d, best = -1, bh = 0x7FFF;
	for (d = 0; d < 8; d++) {
		int nx = sx + w2_dir_dx[d], ny = sy + w2_dir_dy[d], i, h;
		unsigned o;
		if ((nx | ny) & 1) continue;
		if ((unsigned)nx >= (unsigned)w2.map.w || (unsigned)ny >= (unsigned)w2.map.h) continue;
		i = s_row[ny] + nx;
		if (!pass_ok(mc, i) || ((d & 1) && !move_diag_ok(mc, sx, sy, d))) continue;
		o = occ[i];
		if (o != W2_NO_UNIT && (int)o != slot) continue;
		h = heur(nx, ny);
		if (h < bh) { bh = h; best = d; }
	}
	return best;
}

int path_search(int slot, int gx, int gy, int gw, int gh, int range, uint8_t *dirs, int max, int *flags)
{
	const W2Unit *u = &w2.unit[slot];
	const uint16_t *occ;
	int mc = move_class(slot), sx = u->tx, sy = u->ty, mw = w2.map.w, mh = w2.map.h;
	int start, best, besth, bestg = 0, exp = 0, found = 0;
	int fl = 0, n, k, node, budget = path_node_budget, mul = 1, pre = -1, undock = 0;

	if (gw < 1) gw = 1;
	if (gh < 1) gh = 1;
	if (range < 0) range = 0;
	s_gx0 = gx - range; s_gy0 = gy - range; s_gx1 = gx + gw - 1 + range; s_gy1 = gy + gh - 1 + range;
	occ = mc == MC_AIR ? w2.map.occ_air : w2.map.occ_ground;
	path_stats.searches++;
	if (s_row_w != mw) { for (k = 0; k < W2_MAP_MAX; k++) s_row[k] = k * mw; s_row_w = mw; }
	if (mc == MC_SEA || mc == MC_DOCK) terr_check();
	/* undocking (a ship on a shallow tile, not in a transport order): the docking-mode path is kept
	 * only up to the first even open-water tile, the rest is planned as a ship (Mac 0x001511C4) */
	if (mc == MC_DOCK && !(s_dock[slot] && (u->order == W2_ORD_LOAD || u->order == W2_ORD_UNLOAD))) undock = 1;

	besth = heur(sx, sy);
	if (besth == 0) { if (flags) *flags = PATH_COMPLETE; return 0; }

	/* step size: ships 2 tiles on the even grid; docking 2 only for an even start and even 1x1 goal */
	if (mc == MC_SEA) {
		mul = 2;
		if ((sx | sy) & 1) {
			pre = even_entry(slot, mc, sx, sy, occ);
			if (pre < 0) mul = 1;                     /* boxed in off the grid: single steps */
			else { sx += w2_dir_dx[pre]; sy += w2_dir_dy[pre]; }
		}
	} else if (mc == MC_DOCK && !((sx | sy) & 1) && gw == 1 && gh == 1 && range == 0 && !((gx | gy) & 1))
		mul = 2;
	if (mul == 2) {   /* the goal rectangle must hold an even tile on each axis */
		if (s_gx0 == s_gx1 && (s_gx0 & 1)) s_gx0--;
		if (s_gy0 == s_gy1 && (s_gy0 & 1)) s_gy0--;
		if (pre >= 0 && heur(sx, sy) == 0) {     /* the alignment step already arrives */
			dirs[0] = (uint8_t)pre;
			if (flags) *flags = PATH_COMPLETE;
			return 1;
		}
	}
	start = (sy << 7) | sx;
	besth = heur(sx, sy);
	if (budget < 16) budget = 16;
	if (budget > 5000) budget = 5000;      /* keeps f = g + h inside 16 bits */

	if (++s_cur == 0) { memset(s_stamp, 0, sizeof s_stamp); s_cur = 1; }
	s_heapn = 0;
	s_stamp[start] = s_cur; s_g[start] = 0; s_info[start] = 0;
	heap_push(((uint32_t)besth << 16) | (uint32_t)start);
	best = start;

	while (s_heapn > 0) {
		uint32_t key = heap_pop();
		int x, y, g, h, d;
		node = (int)(key & 0xFFFF);
		if (s_info[node] & CLOSED) continue;
		s_info[node] |= CLOSED;
		x = node & 127; y = node >> 7; g = s_g[node];
		h = heur(x, y);
		if (h < besth || (h == besth && g < bestg)) { best = node; besth = h; bestg = g; }
		if (h == 0) { found = 1; break; }
		if (++exp > budget) break;
		for (d = 0; d < 8; d++) {
			int nx = x + w2_dir_dx[d] * mul, ny = y + w2_dir_dy[d] * mul, nn, ng, i;
			unsigned o;
			if ((unsigned)nx >= (unsigned)mw || (unsigned)ny >= (unsigned)mh) continue;
			nn = (ny << 7) | nx;
			if (s_stamp[nn] == s_cur && (s_info[nn] & CLOSED)) continue;
			i = s_row[ny] + nx;
			if (mc != MC_AIR) {
				if (!pass_ok(mc, i)) continue;
				if ((d & 1) && mul == 1 && !move_diag_ok(mc, x, y, d)) continue;
			}
			o = occ[i];
			if (o != W2_NO_UNIT && (int)o != slot) {
				if (w2.unit[o].flags & W2_UF_BUILDING) continue;
				if (!move_is_active((int)o)) continue;
				if (nx - sx <= mul && sx - nx <= mul && ny - sy <= mul && sy - ny <= mul) { fl |= PATH_MOVERBLOCK; continue; }
			}
			ng = g + ((d & 1) ? 11 : 10) * mul;
			if (s_stamp[nn] != s_cur) { s_stamp[nn] = s_cur; s_info[nn] = (uint8_t)d; s_g[nn] = (uint16_t)ng; }
			else if (ng < s_g[nn]) { s_info[nn] = (uint8_t)d; s_g[nn] = (uint16_t)ng; }
			else continue;
			if (s_heapn >= HEAP_MAX) { exp = budget + 1; continue; }   /* overflow = budget spent */
			heap_push(((uint32_t)(ng + heur(nx, ny)) << 16) | (uint32_t)nn);
		}
	}
	if (exp > budget) exp = budget;
	path_stats.expansions += (uint32_t)exp;
	if ((uint32_t)exp > path_stats.max_expansions) path_stats.max_expansions = (uint32_t)exp;
	if (!found && s_heapn == 0) fl |= PATH_EXHAUSTED;
	if (!found) path_stats.partial++;
	if (best == start && pre < 0) { path_stats.nopath++; if (flags) *flags = fl; return -1; }

	/* length, then the first `max` directions (walking back from the end); the alignment step first */
	n = pre >= 0 ? 1 : 0;
	for (node = best; node != start; ) { int d = s_info[node] & 7; node -= (w2_dir_dy[d] * 128 + w2_dir_dx[d]) * mul; n++; }
	k = n;
	for (node = best; node != start; ) {
		int d = s_info[node] & 7;
		k--;
		if (k < max) dirs[k] = (uint8_t)(d | (mul == 2 ? PATH_STEP2 : 0));
		node -= (w2_dir_dy[d] * 128 + w2_dir_dx[d]) * mul;
	}
	if (pre >= 0 && max > 0) dirs[0] = (uint8_t)pre;
	if (undock) {
		int x = sx, y = sy, m = n < max ? n : max;
		for (k = 0; k < m; k++) {
			int d = dirs[k] & 7, mm = (dirs[k] & PATH_STEP2) ? 2 : 1;
			x += w2_dir_dx[d] * mm; y += w2_dir_dy[d] * mm;
			if (!((x | y) & 1) && (s_terr[s_row[y] + x] & MT_WATER)) { n = k + 1; found = 0; break; }
		}
	}
	if (found && n <= max) fl |= PATH_COMPLETE;
	if (flags) *flags = fl;
	return n < max ? n : max;
}

int path_find(int slot, int gx, int gy, int range, uint8_t *dirs, int max)
{
	int fl;
	return path_search(slot, gx, gy, 1, 1, range, dirs, max, &fl);
}
