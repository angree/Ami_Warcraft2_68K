/* test_move.c - host test of the movement + pathfinding module (src/move.c, src/path.c).
 *
 * Loads built-in skirmish maps from the original data, places units with the real unit.c
 * (unit_create), runs unit_tick_all() (animation + order dispatch) and checks EVERY cycle:
 *  - occupancy is exact: every unit holds its footprint tile(s) plus the reserved destination of a
 *    running step, nothing else is marked in occ_ground/occ_air (=> no two units ever overlap);
 *  - every tile a unit holds is passable for its class (no walking through blocked tiles);
 *  - tile changes are single 8-neighbour steps without corner cutting.
 * Scenarios: long cross-map walk, 9 units through a 1-tile gap, 40-unit random stress, ships on a
 * water map, flyers, unreachable goals, follow, head-on in a corridor.
 * PNG shots (terrain from assets_megatile + trails + units) go to the scratch dir given as argv-free
 * env W2_SHOTDIR (default ../_AmiW2-poza-repo/scratch/move) - copyrighted art never enters the repo.
 * Exit code 0 = all checks passed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "w2.h"
#include "pud.h"
#include "assets.h"
#include "w2cfg.h"
#include "move.h"
#include "unit.h"

/* ------------------------------------------------------------------ shell services */
void w2_log(const char *fmt, ...)
{
	va_list ap; va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); fflush(stdout);
}
void amigagfx_set_palette(const unsigned char *rgb, int first, int count) { (void)rgb; (void)first; (void)count; }

static int g_errors;
#define CHECK(c, ...) do { if (!(c)) { g_errors++; if (g_errors < 40) { printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } } while (0)


/* ------------------------------------------------------------------ simulation + checks */
#define MAXU W2_MAX_UNITS
static uint8_t  g_ltx[MAXU], g_lty[MAXU], g_seen[MAXU];
/* trails for the overlay: per unit up to TRAIL_MAX tiles */
#define TRAIL_MAX 600
static uint8_t  g_trail[MAXU][TRAIL_MAX][2];
static uint16_t g_trail_n[MAXU];
static uint8_t  g_track[MAXU];
static uint32_t g_steps[MAXU];
static uint32_t g_max_searches_cycle;

static void trail_reset(void)
{
	memset(g_trail_n, 0, sizeof g_trail_n); memset(g_track, 0, sizeof g_track);
	memset(g_seen, 0, sizeof g_seen); memset(g_steps, 0, sizeof g_steps);
}

static void trail_add(int s)
{
	if (g_trail_n[s] < TRAIL_MAX) { g_trail[s][g_trail_n[s]][0] = w2.unit[s].tx; g_trail[s][g_trail_n[s]][1] = w2.unit[s].ty; g_trail_n[s]++; }
}

static int g_stale_dead;

static void check_world(void)
{
	static uint8_t mark_g[W2_MAP_MAX * W2_MAP_MAX], mark_a[W2_MAP_MAX * W2_MAP_MAX];
	int s, i, n = w2.map.w * w2.map.h;
	memset(mark_g, 0, sizeof mark_g); memset(mark_a, 0, sizeof mark_a);
	for (s = 0; s < w2.unit_hi; s++) {
		W2Unit *u = &w2.unit[s];
		const w2_unit_type *t;
		int w = 1, h = 1, x, y, rx, ry, air, mc;
		uint16_t *occ; uint8_t *mark;
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN))) continue;
		t = w2_unit(u->type);
		air = (u->flags & W2_UF_AIR) != 0;
		occ = air ? w2.map.occ_air : w2.map.occ_ground; mark = air ? mark_a : mark_g;
		if (u->flags & W2_UF_BUILDING) { w = t->tile_w; h = t->tile_h; }
		for (y = u->ty; y < u->ty + h; y++)
			for (x = u->tx; x < u->tx + w; x++) {
				i = y * w2.map.w + x;
				CHECK(occ[i] == s, "cycle %u unit %d (%s) at %d,%d: occ=%d", w2.cycle, s, t->ident, x, y, occ[i]);
				CHECK(!mark[i], "cycle %u tile %d,%d held twice", w2.cycle, x, y);
				mark[i] = 1;
			}
		if (u->flags & W2_UF_BUILDING) continue;
		mc = move_class(s);
		CHECK(move_tile_ok(mc, u->tx, u->ty) || !g_track[s], "cycle %u unit %d (%s) on blocked tile %d,%d", w2.cycle, s, t->ident, u->tx, u->ty);
		if (move_reserved(s, &rx, &ry)) {
			i = ry * w2.map.w + rx;
			CHECK(occ[i] == s, "cycle %u unit %d reserved %d,%d but occ=%d", w2.cycle, s, rx, ry, occ[i]);
			CHECK(!mark[i], "cycle %u reserved tile %d,%d held twice", w2.cycle, rx, ry);
			mark[i] = 1;
			CHECK(abs(rx - u->tx) <= 1 && abs(ry - u->ty) <= 1, "cycle %u unit %d reserved non-neighbour", w2.cycle, s);
			CHECK(move_tile_ok(mc, rx, ry), "cycle %u unit %d steps onto blocked %d,%d", w2.cycle, s, rx, ry);
			CHECK(abs(u->ox) < 32 && abs(u->oy) < 32, "cycle %u bad offset", w2.cycle);
			CHECK(u->ox == 0 || (u->ox > 0) == (rx > u->tx), "cycle %u unit %d ox %d not toward %d", w2.cycle, s, u->ox, rx);
			CHECK(u->oy == 0 || (u->oy > 0) == (ry > u->ty), "cycle %u unit %d oy %d not toward %d", w2.cycle, s, u->oy, ry);
		} else {
			CHECK(u->ox == 0 && u->oy == 0, "cycle %u unit %d offset %d,%d without a step", w2.cycle, s, u->ox, u->oy);
		}
		if (g_seen[s] && (g_ltx[s] != u->tx || g_lty[s] != u->ty)) {
			int dx = u->tx - g_ltx[s], dy = u->ty - g_lty[s], d;
			CHECK(abs(dx) <= 1 && abs(dy) <= 1, "cycle %u unit %d jumped %d,%d -> %d,%d", w2.cycle, s, g_ltx[s], g_lty[s], u->tx, u->ty);
			for (d = 0; d < 8; d++) if (w2_dir_dx[d] == dx && w2_dir_dy[d] == dy) break;
			if (d < 8) CHECK(move_diag_ok(mc, g_ltx[s], g_lty[s], d), "cycle %u unit %d cut a corner at %d,%d dir %d", w2.cycle, s, g_ltx[s], g_lty[s], d);
			g_steps[s]++;
			if (g_track[s]) trail_add(s);
		}
		if (!g_seen[s] && g_track[s]) trail_add(s);
		g_seen[s] = 1; g_ltx[s] = u->tx; g_lty[s] = u->ty;
	}
	for (i = 0; i < n; i++) {
		/* the reserved tile of a unit that died this cycle is swept at the first move call of the next
		 * cycle (unit.c does not call move_release): allow exactly one cycle */
		static uint8_t stale_prev[W2_MAP_MAX * W2_MAP_MAX];
		uint16_t o = w2.map.occ_ground[i];
		if (o != W2_NO_UNIT && !mark_g[i] && !(w2.unit[o].flags & W2_UF_ALIVE) && !stale_prev[i]) { stale_prev[i] = 1; g_stale_dead++; continue; }
		stale_prev[i] = 0;
		CHECK((w2.map.occ_ground[i] == W2_NO_UNIT) == !mark_g[i], "cycle %u stale ground occupancy at %d,%d (%d)", w2.cycle, i % w2.map.w, i / w2.map.w, w2.map.occ_ground[i]);
		CHECK((w2.map.occ_air[i] == W2_NO_UNIT) == !mark_a[i], "cycle %u stale air occupancy at %d,%d", w2.cycle, i % w2.map.w, i / w2.map.w);
	}
}

static void tick(void)
{
	uint32_t before = path_stats.searches;
	unit_tick_all();                    /* the real unit module: animation + order dispatch */
	if (path_stats.searches - before > g_max_searches_cycle) g_max_searches_cycle = path_stats.searches - before;
	w2.cycle++;
	check_world();
}

/* run until none of the listed units has a MOVE/FOLLOW order and none is stepping */
static uint32_t run_until_idle(const int *units, int n, uint32_t limit)
{
	uint32_t c0 = w2.cycle;
	while (w2.cycle - c0 < limit) {
		int i, busy = 0;
		for (i = 0; i < n; i++) if (w2.unit[units[i]].order == W2_ORD_MOVE || move_is_stepping(units[i]) || move_is_active(units[i])) busy = 1;
		if (!busy) break;
		tick();
	}
	return w2.cycle - c0;
}

/* ------------------------------------------------------------------ oracle: BFS reachability */
static uint8_t g_reach[W2_MAP_MAX * W2_MAP_MAX];
static uint16_t g_bfsq[W2_MAP_MAX * W2_MAP_MAX];
static int bfs_dist_to(int mc, int sx, int sy, int gx, int gy)   /* chebyshev-step distance on terrain, -1 */
{
	static uint16_t dist[W2_MAP_MAX * W2_MAP_MAX];
	int h = 0, t = 0, w = w2.map.w;
	memset(g_reach, 0, sizeof g_reach);
	g_reach[sy * w + sx] = 1; dist[sy * w + sx] = 0; g_bfsq[t++] = (uint16_t)(sy * w + sx);
	while (h < t) {
		int i = g_bfsq[h++], x = i % w, y = i / w, d;
		if (x == gx && y == gy) return dist[i];
		for (d = 0; d < 8; d++) {
			int nx = x + w2_dir_dx[d], ny = y + w2_dir_dy[d], j;
			if (!move_tile_ok(mc, nx, ny) || !move_diag_ok(mc, x, y, d)) continue;
			j = ny * w + nx;
			if (g_reach[j]) continue;
			g_reach[j] = 1; dist[j] = (uint16_t)(dist[i] + 1); g_bfsq[t++] = (uint16_t)j;
		}
	}
	return -1;
}

static uint32_t g_seed = 12345;
static int rnd(int n) { g_seed = g_seed * 1103515245u + 12345u; return (int)((g_seed >> 16) % (uint32_t)n); }

static int free_tile(int mc, int x, int y)
{
	uint16_t *occ = mc == MC_AIR ? w2.map.occ_air : w2.map.occ_ground;
	return move_tile_ok(mc, x, y) && occ[y * w2.map.w + x] == W2_NO_UNIT;
}

static int random_free(int mc, int *x, int *y)
{
	int k;
	for (k = 0; k < 100000; k++) {
		*x = rnd(w2.map.w); *y = rnd(w2.map.h);
		if (free_tile(mc, *x, *y)) return 1;
	}
	return 0;
}

/* ------------------------------------------------------------------ PNG */
static uint32_t crc_table[256];
static void crc_init(void) { uint32_t c; int n, k; for (n = 0; n < 256; n++) { c = (uint32_t)n; for (k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1; crc_table[n] = c; } }
static uint32_t crc_upd(uint32_t c, const uint8_t *b, size_t n) { while (n--) c = crc_table[(c ^ *b++) & 0xFF] ^ (c >> 8); return c; }
static void put32(FILE *f, uint32_t v) { fputc((int)(v >> 24), f); fputc((int)(v >> 16) & 255, f); fputc((int)(v >> 8) & 255, f); fputc((int)v & 255, f); }
static void chunk(FILE *f, const char *type, const uint8_t *d, uint32_t n)
{
	uint32_t c = 0xFFFFFFFFu;
	put32(f, n); fwrite(type, 1, 4, f);
	if (n) fwrite(d, 1, n, f);
	c = crc_upd(c, (const uint8_t *)type, 4); if (n) c = crc_upd(c, d, n);
	put32(f, c ^ 0xFFFFFFFFu);
}
static int write_png(const char *path, const uint8_t *rgb, int w, int h)
{
	FILE *f = fopen(path, "wb");
	size_t raw = (size_t)(w * 3 + 1) * h, nblk = (raw + 65534) / 65535, zn = 2 + raw + nblk * 5 + 4, p = 0, o = 0;
	uint8_t *z = (uint8_t *)malloc(zn), hdr[13];
	uint32_t a = 1, b = 0;
	int y;
	if (!f || !z) { if (f) fclose(f); free(z); return -1; }
	z[o++] = 0x78; z[o++] = 0x01;
	for (y = 0; y < h; y++) {
		int x;
		for (x = -1; x < w * 3; x++) {
			uint8_t v = x < 0 ? 0 : rgb[(size_t)y * w * 3 + x];
			if (p % 65535 == 0) {
				size_t left = raw - p, bl = left > 65535 ? 65535 : left;
				z[o++] = left <= 65535 ? 1 : 0; z[o++] = (uint8_t)bl; z[o++] = (uint8_t)(bl >> 8); z[o++] = (uint8_t)~bl; z[o++] = (uint8_t)(~bl >> 8);
			}
			z[o++] = v; p++;
			a = (a + v) % 65521; b = (b + a) % 65521;
		}
	}
	z[o++] = (uint8_t)(b >> 8); z[o++] = (uint8_t)b; z[o++] = (uint8_t)(a >> 8); z[o++] = (uint8_t)a;
	fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
	hdr[0] = (uint8_t)(w >> 24); hdr[1] = (uint8_t)(w >> 16); hdr[2] = (uint8_t)(w >> 8); hdr[3] = (uint8_t)w;
	hdr[4] = (uint8_t)(h >> 24); hdr[5] = (uint8_t)(h >> 16); hdr[6] = (uint8_t)(h >> 8); hdr[7] = (uint8_t)h;
	hdr[8] = 8; hdr[9] = 2; hdr[10] = 0; hdr[11] = 0; hdr[12] = 0;
	chunk(f, "IHDR", hdr, 13); chunk(f, "IDAT", z, (uint32_t)o); chunk(f, "IEND", NULL, 0);
	fclose(f); free(z);
	return 0;
}

/* ------------------------------------------------------------------ drawing */
static char g_shotdir[256];
static uint8_t *g_img; static int g_iw, g_ih, g_scale, g_ox, g_oy;   /* tile scale px, origin tile */

static void px_set(int x, int y, uint32_t c)
{
	uint8_t *p;
	if (x < 0 || y < 0 || x >= g_iw || y >= g_ih) return;
	p = g_img + ((size_t)y * g_iw + x) * 3;
	p[0] = (uint8_t)(c >> 16); p[1] = (uint8_t)(c >> 8); p[2] = (uint8_t)c;
}
static void line(int x0, int y0, int x1, int y1, uint32_t c, int thick)
{
	int dx = abs(x1 - x0), dy = -abs(y1 - y0), sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, e = dx + dy, t;
	for (;;) {
		for (t = 0; t < thick; t++) { px_set(x0 + t, y0, c); px_set(x0, y0 + t, c); }
		if (x0 == x1 && y0 == y1) break;
		if (2 * e >= dy) { e += dy; x0 += sx; }
		if (2 * e <= dx) { e += dx; y0 += sy; }
	}
}
static void rect(int x, int y, int w, int h, uint32_t c, int fill)
{
	int i, j;
	for (j = 0; j < h; j++) for (i = 0; i < w; i++) if (fill || i == 0 || j == 0 || i == w - 1 || j == h - 1) px_set(x + i, y + j, c);
}
static void blend_tile(int tx, int ty, uint32_t c)   /* 50% tint of a tile */
{
	int i, j, x0 = (tx - g_ox) * g_scale, y0 = (ty - g_oy) * g_scale;
	for (j = 0; j < g_scale; j++) for (i = 0; i < g_scale; i++) {
		int x = x0 + i, y = y0 + j; uint8_t *p;
		if (x < 0 || y < 0 || x >= g_iw || y >= g_ih) continue;
		p = g_img + ((size_t)y * g_iw + x) * 3;
		p[0] = (uint8_t)((p[0] + ((c >> 16) & 255)) / 2); p[1] = (uint8_t)((p[1] + ((c >> 8) & 255)) / 2); p[2] = (uint8_t)((p[2] + (c & 255)) / 2);
	}
}

static const uint32_t k_cols[12] = { 0xFF2020, 0x20FF20, 0x4080FF, 0xFFFF20, 0xFF20FF, 0x20FFFF, 0xFF8000, 0xFFFFFF, 0x8040FF, 0x80FF80, 0xFF8080, 0x000000 };

/* view: tiles (x0,y0,w,h) at scale px per tile (32 = full art with sprites) */
static void shot_begin(int x0, int y0, int w, int h, int scale)
{
	const unsigned char *pal = assets_palette();
	int tx, ty, x, y, step = 32 / scale;
	g_ox = x0; g_oy = y0; g_scale = scale; g_iw = w * scale; g_ih = h * scale;
	g_img = (uint8_t *)calloc((size_t)g_iw * g_ih, 3);
	for (ty = 0; ty < h; ty++)
		for (tx = 0; tx < w; tx++) {
			int mx = x0 + tx, my = y0 + ty;
			const unsigned char *m;
			if (mx >= w2.map.w || my >= w2.map.h) continue;
			m = assets_megatile(w2.map.mega[my * w2.map.w + mx]);
			if (!m) continue;
			for (y = 0; y < scale; y++)
				for (x = 0; x < scale; x++) {
					unsigned c = m[(y * step) * 32 + x * step];
					px_set(tx * scale + x, ty * scale + y, ((uint32_t)pal[c * 3] << 16) | ((uint32_t)pal[c * 3 + 1] << 8) | pal[c * 3 + 2]);
				}
			if (w2.map.flags[my * w2.map.w + mx] & W2_TF_NOWALK && !(w2.map.flags[my * w2.map.w + mx] & (W2_TF_WATER | W2_TF_TREE | W2_TF_ROCK)))
				blend_tile(mx, my, 0x800000);   /* test-made walls */
		}
}

static void draw_sprites(void)
{
	static uint8_t buf8[1024 * 1024];
	const unsigned char *pal = assets_palette();
	W2Clip clip; int s, i;
	if ((size_t)g_iw * g_ih > sizeof buf8) return;
	memset(buf8, 0, (size_t)g_iw * g_ih);
	clip.x0 = 0; clip.y0 = 0; clip.x1 = g_iw; clip.y1 = g_ih;
	for (s = 0; s < w2.unit_hi; s++) {
		W2Unit *u = &w2.unit[s];
		const w2_unit_type *t = w2_unit(u->type);
		const W2Spr *sp; int mirror = 0, fr, px, py, tw = 1, th = 1;
		if (!(u->flags & W2_UF_ALIVE) || !t || !t->gfx[w2.map.era < 4 ? w2.map.era : 0]) continue;
		sp = assets_sprite(ASSET_DATA, t->gfx[w2.map.era]);
		if (!sp) continue;
		if (u->flags & W2_UF_BUILDING) { tw = t->tile_w; th = t->tile_h; fr = 0; }
		else fr = (int)w2_dir_frame(u->frame, u->dir, &mirror);
		px = (u->tx - g_ox) * 32 + u->ox + (tw * 32 - sp->box_w) / 2;
		py = (u->ty - g_oy) * 32 + u->oy + (th * 32 - sp->box_h) / 2;
		if (px + sp->box_w < 0 || py + sp->box_h < 0 || px >= g_iw || py >= g_ih) continue;
		assets_draw_sprite(sp, fr, mirror, buf8, g_iw, px, py, &clip, assets_remap(u->owner));
	}
	for (i = 0; i < g_iw * g_ih; i++) if (buf8[i]) {
		unsigned c = buf8[i];
		g_img[i * 3] = pal[c * 3]; g_img[i * 3 + 1] = pal[c * 3 + 1]; g_img[i * 3 + 2] = pal[c * 3 + 2];
	}
}

static void draw_trails(const int *units, int n)
{
	int k, i, hs = g_scale / 2;
	for (k = 0; k < n; k++) {
		int s = units[k]; uint32_t c = k_cols[k % 12];
		for (i = 1; i < g_trail_n[s]; i++)
			line((g_trail[s][i - 1][0] - g_ox) * g_scale + hs, (g_trail[s][i - 1][1] - g_oy) * g_scale + hs,
			     (g_trail[s][i][0] - g_ox) * g_scale + hs, (g_trail[s][i][1] - g_oy) * g_scale + hs, c, g_scale >= 16 ? 2 : 1);
		if (g_trail_n[s]) rect((g_trail[s][0][0] - g_ox) * g_scale + hs - 2, (g_trail[s][0][1] - g_oy) * g_scale + hs - 2, 5, 5, c, 0);
	}
}
static void draw_units_boxes(const int *units, int n)
{
	int k;
	for (k = 0; k < n; k++) {
		const W2Unit *u = &w2.unit[units[k]];
		int sz = g_scale < 6 ? 3 : g_scale - 2;
		rect((u->tx - g_ox) * g_scale + (g_scale - sz) / 2 + u->ox * g_scale / 32, (u->ty - g_oy) * g_scale + (g_scale - sz) / 2 + u->oy * g_scale / 32, sz, sz, k_cols[k % 12], 1);
	}
}
static void draw_goal(int gx, int gy, uint32_t c)
{
	int x = (gx - g_ox) * g_scale, y = (gy - g_oy) * g_scale;
	line(x, y, x + g_scale - 1, y + g_scale - 1, c, 2); line(x + g_scale - 1, y, x, y + g_scale - 1, c, 2);
	rect(x - 1, y - 1, g_scale + 2, g_scale + 2, c, 0);
}
static void shot_end(const char *name)
{
	char path[400];
	snprintf(path, sizeof path, "%s/%s.png", g_shotdir, name);
	if (write_png(path, g_img, g_iw, g_ih) == 0) printf("  shot %s (%dx%d)\n", path, g_iw, g_ih);
	else { printf("  shot %s FAILED\n", path); g_errors++; }
	free(g_img); g_img = NULL;
}

/* ------------------------------------------------------------------ scenarios */
static int load_map(int entry)
{
	if (pud_load_builtin(entry)) { printf("cannot load builtin map %d\n", entry); return -1; }
	move_reset_all();
	trail_reset();
	memset(g_seen, 0, sizeof g_seen);
	g_max_searches_cycle = 0;
	printf("map %d: %dx%d era %d, %d unit slots used\n", entry, w2.map.w, w2.map.h, w2.map.era, w2.unit_hi);
	check_world();
	return 0;
}

static int place(int type, int x, int y)
{
	int s = unit_create(type, 0, x, y);
	if (s >= 0) { g_track[s] = 1; g_seen[s] = 0; }
	return s;
}

static int cheb(int ax, int ay, int bx, int by) { int dx = abs(ax - bx), dy = abs(ay - by); return dx > dy ? dx : dy; }

/* A: long walks across map 220 */
static void scen_cross(void)
{
	int u[3], gx[3], gy[3], i, sx, sy, mc = MC_LAND, best;
	uint32_t c;
	printf("\n== A: cross-map walks (map 220) ==\n");
	if (load_map(220)) { g_errors++; return; }
	/* start: free land tile closest to (4,4); goal: reachable free land tile closest to the far corner */
	for (i = 0; i < 3; i++) {
		int x, y, bd = 1 << 30;
		static const int cx[3] = { 4, 4, 120 }, cy[3] = { 4, 120, 4 };
		sx = sy = -1;
		for (y = 0; y < w2.map.h; y++) for (x = 0; x < w2.map.w; x++)
			if (free_tile(mc, x, y)) { int d = cheb(x, y, cx[i], cy[i]) * 1000 + x + y; if (d < bd) { bd = d; sx = x; sy = y; } }
		u[i] = place(i == 1 ? W2U_KNIGHT : W2U_FOOTMAN, sx, sy);
		bd = 1 << 30; gx[i] = gy[i] = -1;
		bfs_dist_to(mc, sx, sy, -1, -1);
		for (y = 0; y < w2.map.h; y++) for (x = 0; x < w2.map.w; x++)
			if (g_reach[y * w2.map.w + x] && free_tile(mc, x, y)) { int d = cheb(x, y, 127 - cx[i], 127 - cy[i]); if (d < bd) { bd = d; gx[i] = x; gy[i] = y; } }
		best = bfs_dist_to(mc, sx, sy, gx[i], gy[i]);
		printf("  unit %d %s %d,%d -> %d,%d  (BFS shortest %d steps)\n", u[i], w2_unit(w2.unit[u[i]].type)->ident, sx, sy, gx[i], gy[i], best);
		cmd_move(u[i], gx[i], gy[i]);
	}
	c = run_until_idle(u, 3, 20000);
	for (i = 0; i < 3; i++) {
		W2Unit *w = &w2.unit[u[i]];
		int best = 0;
		printf("  unit %d arrived at %d,%d after %u steps (cycle %u total)\n", u[i], w->tx, w->ty, g_steps[u[i]], c);
		CHECK(w->tx == gx[i] && w->ty == gy[i], "A: unit %d did not reach its goal", u[i]);
		(void)best;
	}
	printf("  searches %u expansions %u (max %u per search) partial %u deferred %u\n", path_stats.searches, path_stats.expansions, path_stats.max_expansions, path_stats.partial, path_stats.deferred);
	shot_begin(0, 0, w2.map.w, w2.map.h, 8);
	draw_trails(u, 3); draw_units_boxes(u, 3);
	for (i = 0; i < 3; i++) draw_goal(gx[i], gy[i], k_cols[i]);
	shot_end("a_cross_220");
	{	/* passability as the module sees it: red = not walkable, blue = water */
		int x, y;
		shot_begin(0, 0, w2.map.w, w2.map.h, 8);
		for (y = 0; y < w2.map.h; y++) for (x = 0; x < w2.map.w; x++) {
			unsigned f = w2.map.flags[y * w2.map.w + x];
			if (f & W2_TF_WATER) blend_tile(x, y, 0x0000FF);
			else if (f & W2_TF_NOWALK) blend_tile(x, y, 0xFF0000);
		}
		draw_trails(u, 3);
		shot_end("a_flags_220");
	}
}

/* find an all-free w x h land rectangle */
static int find_open(int w, int h, int *ox, int *oy, int from)
{
	int x, y, i, j;
	for (y = from; y + h < w2.map.h; y++)
		for (x = 2; x + w < w2.map.w; x++) {
			int ok = 1;
			for (j = 0; j < h && ok; j++) for (i = 0; i < w && ok; i++) if (!free_tile(MC_LAND, x + i, y + j)) ok = 0;
			if (ok) { *ox = x; *oy = y; return 1; }
		}
	return 0;
}

/* B: 9 units through a 1-tile gap */
static void scen_gap(void)
{
	int x0, y0, u[9], i, wx, gx, gy, n = 0;
	uint32_t c;
	printf("\n== B: group of 9 through a 1-tile gap (map 220) ==\n");
	if (load_map(220)) { g_errors++; return; }
	if (!find_open(18, 11, &x0, &y0, 10)) { printf("  no open area\n"); g_errors++; return; }
	wx = x0 + 8;
	/* wall at column wx with a gap at row y0+5, extended up and down until existing obstacles */
	for (i = y0 + 4; i >= 0 && !(w2.map.flags[i * w2.map.w + wx] & W2_TF_NOWALK); i--) w2.map.flags[i * w2.map.w + wx] |= W2_TF_NOWALK;
	for (i = y0 + 6; i < w2.map.h && !(w2.map.flags[i * w2.map.w + wx] & W2_TF_NOWALK); i++) w2.map.flags[i * w2.map.w + wx] |= W2_TF_NOWALK;
	for (i = 0; i < 9; i++) { u[n] = place(i & 1 ? W2U_GRUNT : W2U_FOOTMAN, x0 + 1 + i % 3, y0 + 4 + i / 3); if (u[n] >= 0) n++; }
	gx = x0 + 14; gy = y0 + 5;
	printf("  area %d,%d wall x=%d gap %d,%d goal %d,%d, %d units\n", x0, y0, wx, wx, y0 + 5, gx, gy, n);
	for (i = 0; i < n; i++) cmd_move(u[i], gx, gy);
	for (i = 0; i < 60; i++) tick();
	shot_begin(x0 - 1, y0 - 1, 20, 13, 32);
	draw_sprites(); draw_trails(u, n); draw_goal(gx, gy, 0xFFFF00);
	shot_end("b_gap_mid");
	c = run_until_idle(u, n, 5000);
	printf("  all idle after %u cycles\n", c + 60);
	for (i = 0; i < n; i++) {
		W2Unit *w = &w2.unit[u[i]];
		printf("  unit %d at %d,%d dist %d steps %u\n", u[i], w->tx, w->ty, cheb(w->tx, w->ty, gx, gy), g_steps[u[i]]);
		CHECK(w->tx > wx, "B: unit %d stayed behind the wall", u[i]);
		CHECK(cheb(w->tx, w->ty, gx, gy) <= 2, "B: unit %d too far from the goal", u[i]);
	}
	CHECK(c < 5000, "B: group never settled");
	printf("  searches %u partial %u deferred %u max searches/cycle %u\n", path_stats.searches, path_stats.partial, path_stats.deferred, g_max_searches_cycle);
	shot_begin(x0 - 1, y0 - 1, 20, 13, 32);
	draw_sprites(); draw_trails(u, n); draw_goal(gx, gy, 0xFFFF00);
	shot_end("b_gap_end");
}

/* C: 40 random units with random reachable goals, three rounds */
static void scen_stress(void)
{
	int u[40], n = 0, i, r, x, y, gx[40], gy[40];
	static const int types[6] = { W2U_FOOTMAN, W2U_GRUNT, W2U_PEASANT, W2U_KNIGHT, W2U_ARCHER, W2U_CATAPULT };
	uint32_t c, total = 0;
	printf("\n== C: 40 units, random reachable goals x3 rounds (map 220) ==\n");
	if (load_map(220)) { g_errors++; return; }
	for (i = 0; i < 40; i++) if (random_free(MC_LAND, &x, &y)) { u[n] = place(types[i % 6], x, y); if (u[n] >= 0) n++; }
	for (r = 0; r < 3; r++) {
		int exact = 0, near = 0, far = 0;
		for (i = 0; i < n; i++) {
			int k;
			for (k = 0; k < 1000; k++) {
				if (!random_free(MC_LAND, &x, &y)) break;
				if (bfs_dist_to(MC_LAND, w2.unit[u[i]].tx, w2.unit[u[i]].ty, x, y) > 0) break;
			}
			gx[i] = x; gy[i] = y;
			cmd_move(u[i], x, y);
		}
		c = run_until_idle(u, n, 20000); total += c;
		for (i = 0; i < n; i++) {
			int d = cheb(w2.unit[u[i]].tx, w2.unit[u[i]].ty, gx[i], gy[i]);
			if (d == 0) exact++; else if (d <= 2) near++; else { far++; printf("  round %d unit %d stopped %d tiles from %d,%d\n", r, u[i], d, gx[i], gy[i]); }
		}
		printf("  round %d: %u cycles, exact %d near %d far %d\n", r, c, exact, near, far);
		CHECK(far == 0, "C: round %d, %d units did not arrive", r, far);
		CHECK(c < 20000, "C: round %d never settled", r);
	}
	printf("  searches %u expansions %u (avg %u, max %u) partial %u nopath %u deferred %u, max searches/cycle %u (quota %d)\n",
	       path_stats.searches, path_stats.expansions, path_stats.searches ? path_stats.expansions / path_stats.searches : 0,
	       path_stats.max_expansions, path_stats.partial, path_stats.nopath, path_stats.deferred, g_max_searches_cycle, move_search_quota);
	CHECK((int)g_max_searches_cycle <= move_search_quota, "C: search quota exceeded");
	shot_begin(0, 0, w2.map.w, w2.map.h, 8);
	draw_trails(u, n); draw_units_boxes(u, n);
	shot_end("c_stress_220");
	(void)total;
}

/* D: ships on a water map */
static void scen_ships(void)
{
	int u[4], n = 0, i, x, y, gx[4], gy[4], bd;
	static const int types[4] = { W2U_HUMAN_DESTROYER, W2U_ORC_DESTROYER, W2U_BATTLESHIP, W2U_HUMAN_TRANSPORT };
	uint32_t c;
	printf("\n== D: ships (map 226) ==\n");
	if (load_map(226)) { g_errors++; return; }
	for (i = 0; i < 4; i++) {
		int sx, sy;
		if (!random_free(MC_SEA, &sx, &sy)) break;
		u[n] = place(types[i], sx, sy);
		if (u[n] < 0) continue;
		/* farthest reachable free water tile */
		bfs_dist_to(MC_SEA, sx, sy, -1, -1);
		bd = -1; gx[n] = sx; gy[n] = sy;
		for (y = 0; y < w2.map.h; y++) for (x = 0; x < w2.map.w; x++)
			if (g_reach[y * w2.map.w + x] && free_tile(MC_SEA, x, y) && cheb(x, y, sx, sy) > bd && rnd(4) == 0) { bd = cheb(x, y, sx, sy); gx[n] = x; gy[n] = y; }
		printf("  ship %d %s %d,%d -> %d,%d (BFS %d)\n", u[n], w2_unit(types[i])->ident, sx, sy, gx[n], gy[n], bfs_dist_to(MC_SEA, sx, sy, gx[n], gy[n]));
		cmd_move(u[n], gx[n], gy[n]);
		n++;
	}
	c = run_until_idle(u, n, 20000);
	for (i = 0; i < n; i++) {
		W2Unit *w = &w2.unit[u[i]];
		printf("  ship %d at %d,%d after %u steps\n", u[i], w->tx, w->ty, g_steps[u[i]]);
		CHECK(cheb(w->tx, w->ty, gx[i], gy[i]) <= 1, "D: ship %d did not arrive", u[i]);
	}
	printf("  %u cycles, searches %u partial %u\n", c, path_stats.searches, path_stats.partial);
	shot_begin(0, 0, w2.map.w, w2.map.h, 8);
	draw_trails(u, n); draw_units_boxes(u, n);
	for (i = 0; i < n; i++) draw_goal(gx[i], gy[i], k_cols[i]);
	shot_end("d_ships_226");
}

/* E: flyers over everything */
static void scen_fly(void)
{
	int u[3], i;
	static const int sx[3] = { 2, 120, 3 }, sy[3] = { 2, 3, 100 }, gx[3] = { 125, 10, 120 }, gy[3] = { 120, 118, 20 };
	uint32_t c;
	printf("\n== E: flyers (map 226) ==\n");
	if (load_map(226)) { g_errors++; return; }
	for (i = 0; i < 3; i++) { u[i] = place(i == 1 ? W2U_DRAGON : W2U_GRYPHON_RIDER, sx[i], sy[i]); cmd_move(u[i], gx[i], gy[i]); }
	c = run_until_idle(u, 3, 20000);
	for (i = 0; i < 3; i++) {
		W2Unit *w = &w2.unit[u[i]];
		printf("  flyer %d at %d,%d: %u steps (straight-line minimum %d), %u cycles\n", u[i], w->tx, w->ty, g_steps[u[i]], cheb(sx[i], sy[i], gx[i], gy[i]), c);
		CHECK(w->tx == gx[i] && w->ty == gy[i], "E: flyer %d did not arrive", u[i]);
		CHECK((int)g_steps[u[i]] == cheb(sx[i], sy[i], gx[i], gy[i]), "E: flyer %d path not minimal", u[i]);
	}
	shot_begin(0, 0, w2.map.w, w2.map.h, 8);
	draw_trails(u, 3); draw_units_boxes(u, 3);
	for (i = 0; i < 3; i++) draw_goal(gx[i], gy[i], k_cols[i]);
	shot_end("e_fly_226");
}

/* F: unreachable goals */
static void scen_unreach(void)
{
	int u[3], i, x0, y0, x, y, wx = -1, wy = -1, rs[3], gxs[3], gys[3];
	uint32_t c0, giveup[3];
	printf("\n== F: unreachable goals (map 226 water, enclosed tile on 220) ==\n");
	/* F1: land unit to a deep-water tile on 226 */
	if (load_map(226)) { g_errors++; return; }
	for (y = 10; y < w2.map.h - 10 && wx < 0; y++) for (x = 10; x < w2.map.w - 10; x++) {
		int i2, j2, ok = 1;
		for (j2 = -3; j2 <= 3 && ok; j2++) for (i2 = -3; i2 <= 3 && ok; i2++) if (!(w2.map.flags[(y + j2) * w2.map.w + x + i2] & W2_TF_WATER)) ok = 0;
		if (ok) { wx = x; wy = y; break; }
	}
	{   /* a start on the biggest land mass among 40 random tries */
		int bestc = -1, bx = 0, by = 0, tries;
		for (tries = 0; tries < 40; tries++) {
			int cnt = 0, k;
			if (!random_free(MC_LAND, &x, &y)) { g_errors++; return; }
			bfs_dist_to(MC_LAND, x, y, -1, -1);
			for (k = 0; k < w2.map.w * w2.map.h; k++) cnt += g_reach[k];
			if (cnt > bestc) { bestc = cnt; bx = x; by = y; }
		}
		x = bx; y = by;
		printf("  F1: start region %d tiles\n", bestc);
	}
	u[0] = place(W2U_FOOTMAN, x, y);
	gxs[0] = wx; gys[0] = wy;
	printf("  F1: footman %d at %d,%d -> water %d,%d\n", u[0], x, y, wx, wy);
	cmd_move(u[0], wx, wy);
	c0 = w2.cycle;
	giveup[0] = run_until_idle(u, 1, 20000);
	printf("  F1: gave up after %u cycles at %d,%d (%d tiles from goal), steps %u\n", giveup[0], w2.unit[u[0]].tx, w2.unit[u[0]].ty, cheb(w2.unit[u[0]].tx, w2.unit[u[0]].ty, wx, wy), g_steps[u[0]]);
	CHECK(w2.unit[u[0]].order == W2_ORD_STILL && giveup[0] < 20000, "F1: did not give up");
	{	/* oracle: the closest reachable tile */
		int k, bestd = 1 << 30;
		bfs_dist_to(MC_LAND, x, y, -1, -1);
		for (k = 0; k < w2.map.w * w2.map.h; k++) if (g_reach[k]) { int d = cheb(k % w2.map.w, k / w2.map.w, wx, wy); if (d < bestd) bestd = d; }
		printf("  F1: closest reachable tile is %d tiles from the goal\n", bestd);
		CHECK(cheb(w2.unit[u[0]].tx, w2.unit[u[0]].ty, wx, wy) == bestd, "F1: did not get as close as possible");
	}
	{
		static uint8_t dd[256]; int r = path_find(u[0], wx, wy, 0, dd, 256);
		printf("  F1: path_find from the stop tile = %d (expect -1)\n", r);
		CHECK(r == -1, "F1: path_find should return -1");
	}
	shot_begin(0, 0, w2.map.w, w2.map.h, 8);
	draw_trails(u, 1); draw_units_boxes(u, 1); draw_goal(wx, wy, 0xFF0000);
	shot_end("f1_unreach_water");
	(void)c0;

	/* F2: enclosed tile (ring of walls) on 220 + a ship ordered onto land */
	if (load_map(220)) { g_errors++; return; }
	if (!find_open(12, 9, &x0, &y0, 40)) { g_errors++; return; }
	for (i = 0; i < 5; i++) {   /* 5x5 ring at x0+6.. */
		int k;
		for (k = 0; k < 5; k++) {
			if (i == 0 || i == 4 || k == 0 || k == 4) w2.map.flags[(y0 + 2 + i) * w2.map.w + x0 + 6 + k] |= W2_TF_NOWALK;
		}
	}
	gxs[1] = x0 + 8; gys[1] = y0 + 4;
	u[1] = place(W2U_GRUNT, x0 + 1, y0 + 4);
	u[2] = place(W2U_PEASANT, x0 + 1, y0 + 6);
	gxs[2] = x0 + 8; gys[2] = y0 + 4;
	cmd_move(u[1], gxs[1], gys[1]);
	cmd_move(u[2], gxs[2], gys[2]);
	giveup[1] = run_until_idle(&u[1], 2, 20000);
	for (i = 1; i < 3; i++) {
		rs[i] = cheb(w2.unit[u[i]].tx, w2.unit[u[i]].ty, gxs[i], gys[i]);
		printf("  F2: unit %d stopped at %d,%d, %d tiles from enclosed goal, %u cycles, steps %u\n", u[i], w2.unit[u[i]].tx, w2.unit[u[i]].ty, rs[i], giveup[1], g_steps[u[i]]);
		CHECK(w2.unit[u[i]].order == W2_ORD_STILL, "F2: unit %d did not give up", u[i]);
		CHECK(rs[i] <= 3, "F2: unit %d did not get close", u[i]);
	}
	CHECK(giveup[1] < 2000, "F2: giving up took too long");
	shot_begin(x0 - 1, y0 - 1, 14, 11, 32);
	draw_sprites(); draw_trails(&u[1], 2); draw_goal(gxs[1], gys[1], 0xFF0000);
	shot_end("f2_unreach_enclosed");
}

/* G: follow + H: head-on in a corridor */
static void scen_follow_corridor(void)
{
	int x0, y0, a, b, i, u[2];
	uint32_t c;
	printf("\n== G: follow (map 220) ==\n");
	if (load_map(220)) { g_errors++; return; }
	if (!find_open(20, 6, &x0, &y0, 60)) { g_errors++; return; }
	a = place(W2U_KNIGHT, x0 + 1, y0 + 1);
	b = place(W2U_FOOTMAN, x0 + 1, y0 + 4);
	cmd_move(a, x0 + 18, y0 + 2);
	cmd_follow(b, a);
	u[0] = a; u[1] = b;
	for (i = 0; i < 400; i++) tick();
	printf("  leader %d at %d,%d order %d; follower %d at %d,%d order %d, distance %d\n", a, w2.unit[a].tx, w2.unit[a].ty, w2.unit[a].order,
	       b, w2.unit[b].tx, w2.unit[b].ty, w2.unit[b].order, cheb(w2.unit[a].tx, w2.unit[a].ty, w2.unit[b].tx, w2.unit[b].ty));
	CHECK(w2.unit[b].order == W2_ORD_FOLLOW, "G: follower lost its order");
	CHECK(cheb(w2.unit[a].tx, w2.unit[a].ty, w2.unit[b].tx, w2.unit[b].ty) <= 1, "G: follower not next to the leader");
	shot_begin(x0 - 1, y0 - 1, 22, 8, 32);
	draw_sprites(); draw_trails(u, 2);
	shot_end("g_follow");
	/* the leader dies in the middle of a step: its reserved tile must be freed, the follower stops */
	cmd_move(a, x0 + 2, y0 + 2);
	for (i = 0; i < 400 && !move_is_stepping(a); i++) tick();
	for (i = 0; i < 4; i++) tick();
	printf("  leader stepping=%d ox=%d oy=%d -> killed\n", move_is_stepping(a), w2.unit[a].ox, w2.unit[a].oy);
	CHECK(move_is_stepping(a), "G: leader not mid-step");
	unit_kill(a);                       /* unit.c processes the death at the next tick */
	for (i = 0; i < 60; i++) tick();
	printf("  follower order %d after the leader died; dead unit's reserved tile was stale for %d tile-cycle(s)\n", w2.unit[b].order, g_stale_dead);
	CHECK(!(w2.unit[a].flags & W2_UF_ALIVE), "G: leader not dead");
	CHECK(w2.unit[b].order == W2_ORD_STILL, "G: follower still following a dead unit");

	printf("\n== H: head-on in a 1-wide corridor (map 220) ==\n");
	if (load_map(220)) { g_errors++; return; }
	if (!find_open(16, 5, &x0, &y0, 30)) { g_errors++; return; }
	for (i = 0; i < 16; i++) {   /* corridor row y0+2 between walls at rows y0+1 and y0+3 */
		w2.map.flags[(y0 + 1) * w2.map.w + x0 + i] |= W2_TF_NOWALK;
		w2.map.flags[(y0 + 3) * w2.map.w + x0 + i] |= W2_TF_NOWALK;
	}
	a = place(W2U_FOOTMAN, x0 + 3, y0 + 2);
	b = place(W2U_GRUNT, x0 + 12, y0 + 2);
	cmd_move(a, x0 + 15, y0 + 2);
	cmd_move(b, x0, y0 + 2);
	u[0] = a; u[1] = b;
	c = run_until_idle(u, 2, 20000);
	printf("  settled after %u cycles: %d at %d,%d  %d at %d,%d\n", c, a, w2.unit[a].tx, w2.unit[a].ty, b, w2.unit[b].tx, w2.unit[b].ty);
	CHECK(c < 20000, "H: corridor deadlock never resolved");
	shot_begin(x0 - 2, y0 - 2, 20, 9, 32);
	draw_sprites(); draw_trails(u, 2);
	shot_end("h_corridor");

	/* stop order in the middle of a step with logic frameskip 4 (ord_still runs every 4th cycle) */
	w2_think_period = 4;
	cmd_move(a, x0 + 3, y0 + 2);
	for (i = 0; i < 200 && !move_is_stepping(a); i++) tick();
	for (i = 0; i < 5; i++) tick();
	printf("  stop mid-step (think period 4): stepping=%d ox=%d\n", move_is_stepping(a), w2.unit[a].ox);
	unit_set_order(a, W2_ORD_STILL, W2_NO_UNIT, 0, 0);
	for (i = 0; i < 40; i++) tick();
	printf("  after 40 cycles: stepping=%d tile %d,%d ox=%d anim %d\n", move_is_stepping(a), w2.unit[a].tx, w2.unit[a].ty, w2.unit[a].ox, w2.unit[a].anim_kind);
	CHECK(!move_is_stepping(a) && w2.unit[a].ox == 0 && w2.unit[a].oy == 0, "H: stopped unit left between tiles");
	w2_think_period = 1;
}

/* I: performance of a worst-case search (map 220, budget hit) */
static void scen_perf(void)
{
	int s, x, y, r, fl, k;
	static uint8_t dd[256];
	printf("\n== I: search budget ==\n");
	if (load_map(220)) { g_errors++; return; }
	if (!random_free(MC_LAND, &x, &y)) return;
	s = place(W2U_FOOTMAN, x, y);
	for (k = 0; k < 5; k++) {
		int gx, gy;
		random_free(MC_LAND, &gx, &gy);
		memset(&path_stats, 0, sizeof path_stats);
		r = path_search(s, gx, gy, 1, 1, 0, dd, 256, &fl);
		printf("  %d,%d -> %d,%d: steps %d flags %d expansions %u (BFS %d)\n", x, y, gx, gy, r, fl, path_stats.expansions, bfs_dist_to(MC_LAND, x, y, gx, gy));
		CHECK(path_stats.expansions <= (uint32_t)path_node_budget, "I: budget exceeded");
	}
}

int main(void)
{
	const char *dd = getenv("W2_DATA"), *sd = getenv("W2_SHOTDIR");
	crc_init();
	w2cfg_defaults(&w2_cfg);
	snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", dd ? dd : "/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/data");
	snprintf(g_shotdir, sizeof g_shotdir, "%s", sd ? sd : "/mnt/d/GITHUB/_AmiW2-poza-repo/scratch/move");
	if (assets_init()) { printf("assets_init failed (data in %s)\n", w2_cfg.data_dir); return 2; }
	scen_cross();
	scen_gap();
	scen_stress();
	scen_ships();
	scen_fly();
	scen_unreach();
	scen_follow_corridor();
	scen_perf();
	printf("\n%s: %d check failures\n", g_errors ? "FAILED" : "PASSED", g_errors);
	assets_shutdown();
	return g_errors ? 1 : 0;
}
