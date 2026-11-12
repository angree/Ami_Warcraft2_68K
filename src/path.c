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
 */
#include <string.h>
#include "w2.h"
#include "move.h"

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
	if (u->flags & W2_UF_SEA) return MC_SEA;
	t = w2_unit(u->type);
	if (t && t->kind == W2_KIND_AIR) return MC_AIR;
	if (t && t->kind == W2_KIND_SEA) return MC_SEA;
	return MC_LAND;
}

int move_tile_ok(int mc, int tx, int ty)
{
	unsigned f;
	if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 0;
	if (mc == MC_AIR) return 1;
	f = w2.map.flags[ty * w2.map.w + tx];
	if (f & W2_TF_BUILDING) return 0;
	if (mc == MC_SEA) return (f & W2_TF_WATER) != 0;
	return (f & W2_TF_NOWALK) == 0;
}

int move_diag_ok(int mc, int tx, int ty, int dir)
{
	if (!(dir & 1) || mc == MC_AIR) return 1;
	return move_tile_ok(mc, tx + w2_dir_dx[dir], ty) && move_tile_ok(mc, tx, ty + w2_dir_dy[dir]);
}

int path_search(int slot, int gx, int gy, int gw, int gh, int range, uint8_t *dirs, int max, int *flags)
{
	const W2Unit *u = &w2.unit[slot];
	const uint16_t *occ;
	int mc = move_class(slot), sx = u->tx, sy = u->ty, mw = w2.map.w, mh = w2.map.h;
	int start = (sy << 7) | sx, best, besth, bestg = 0, exp = 0, found = 0;
	int fl = 0, n, k, node, budget = path_node_budget;

	if (gw < 1) gw = 1;
	if (gh < 1) gh = 1;
	if (range < 0) range = 0;
	s_gx0 = gx - range; s_gy0 = gy - range; s_gx1 = gx + gw - 1 + range; s_gy1 = gy + gh - 1 + range;
	occ = mc == MC_AIR ? w2.map.occ_air : w2.map.occ_ground;
	path_stats.searches++;
	if (s_row_w != mw) { for (k = 0; k < W2_MAP_MAX; k++) s_row[k] = k * mw; s_row_w = mw; }

	besth = heur(sx, sy);
	if (besth == 0) { if (flags) *flags = PATH_COMPLETE; return 0; }
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
			int nx = x + w2_dir_dx[d], ny = y + w2_dir_dy[d], nn, ng, i;
			unsigned o;
			if ((unsigned)nx >= (unsigned)mw || (unsigned)ny >= (unsigned)mh) continue;
			nn = (ny << 7) | nx;
			if (s_stamp[nn] == s_cur && (s_info[nn] & CLOSED)) continue;
			i = s_row[ny] + nx;
			if (mc != MC_AIR) {
				unsigned f = w2.map.flags[i];
				if (f & W2_TF_BUILDING) continue;
				if (mc == MC_SEA ? !(f & W2_TF_WATER) : (f & W2_TF_NOWALK) != 0) continue;
				if ((d & 1) && !move_diag_ok(mc, x, y, d)) continue;
			}
			o = occ[i];
			if (o != W2_NO_UNIT && (int)o != slot) {
				if (w2.unit[o].flags & W2_UF_BUILDING) continue;
				if (!move_is_active((int)o)) continue;
				if (nx - sx <= 1 && sx - nx <= 1 && ny - sy <= 1 && sy - ny <= 1) { fl |= PATH_MOVERBLOCK; continue; }
			}
			ng = g + ((d & 1) ? 11 : 10);
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
	if (best == start) { path_stats.nopath++; if (flags) *flags = fl; return -1; }

	/* length, then the first `max` directions (walking back from the end) */
	n = 0;
	for (node = best; node != start; ) { int d = s_info[node] & 7; node -= w2_dir_dy[d] * 128 + w2_dir_dx[d]; n++; }
	k = n;
	for (node = best; node != start; ) {
		int d = s_info[node] & 7;
		k--;
		if (k < max) dirs[k] = (uint8_t)d;
		node -= w2_dir_dy[d] * 128 + w2_dir_dx[d];
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
