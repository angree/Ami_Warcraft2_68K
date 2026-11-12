/* test_economy.c - host test of src/economy.c (build: tools/build_test_economy.sh).
 *
 * Uses the real unit.c / mapdraw.c / unitdraw.c / pud.c / assets.c. move.c does not exist yet, so a
 * simple tile-step mover is defined here (BFS, one tile per 16 cycles = the peasant MOVE animation
 * length). ui_message / snd_unit are logged. PNGs go to $ECON_OUT (outside the repository).
 * Env: ECON_DATA = directory with "War Data" etc., ECON_OUT = output directory.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "w2.h"
#include "pud.h"
#include "assets.h"
#include "w2cfg.h"
#include "economy.h"
#include "unit.h"
#include "mapdraw.h"

static const char *g_out = ".";
static int g_fail;
static int g_msgs;
static char g_last_msg[160];
static int g_ready_snd;

#define CHECK(c, ...) do { if (c) { printf("  ok   "); printf(__VA_ARGS__); printf("\n"); } \
	else { printf("  FAIL "); printf(__VA_ARGS__); printf("\n"); g_fail++; } } while (0)

/* ------------------------------------------------------------------ shell services */
void w2_log(const char *fmt, ...)
{
	va_list ap; va_start(ap, fmt);
	if (getenv("ECON_VERBOSE")) { vprintf(fmt, ap); printf("\n"); }
	va_end(ap);
}
unsigned long w2_tick_ms(void) { return 33; }
unsigned long w2_frame_count(void) { return 0; }

void ui_message(const char *t)
{
	g_msgs++;
	snprintf(g_last_msg, sizeof g_last_msg, "%s", t);
	printf("  [msg cycle %u] %s\n", (unsigned)w2.cycle, t);
}
void snd_unit(int slot, int what) { (void)slot; if (what == 2) g_ready_snd++; }

/* ------------------------------------------------------------------ test mover */
static uint8_t mv_wait[W2_MAX_UNITS];
static uint8_t mv_block[W2_MAX_UNITS];
static int16_t bfs_par[W2_MAP_MAX * W2_MAP_MAX];
static uint16_t bfs_q[W2_MAP_MAX * W2_MAP_MAX];
static const int8_t DX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 }, DY[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

static int rect_gap(int x, int y, int gx, int gy, int gw, int gh)
{
	int dx = x < gx ? gx - x : (x >= gx + gw ? x - (gx + gw - 1) : 0);
	int dy = y < gy ? gy - y : (y >= gy + gh ? y - (gy + gh - 1) : 0);
	return dx > dy ? dx : dy;
}

static int pass(int slot, int x, int y, int ign)
{
	int i; uint16_t f;
	if (x < 0 || y < 0 || x >= w2.map.w || y >= w2.map.h) return 0;
	i = y * w2.map.w + x; f = w2.map.flags[i];
	if (w2.unit[slot].flags & W2_UF_SEA) { if (!(f & W2_TF_WATER) || (f & W2_TF_BUILDING)) return 0; }
	else if (f & (W2_TF_NOWALK | W2_TF_BUILDING)) return 0;
	if (!ign && w2.map.occ_ground[i] != W2_NO_UNIT && w2.map.occ_ground[i] != slot) return 0;
	return 1;
}

static int bfs(int slot, int gx, int gy, int gw, int gh, int range, int ign, int *nx, int *ny)
{
	int mw = w2.map.w, n = w2.map.w * w2.map.h, qh = 0, qt = 0, s, d;
	W2Unit *u = &w2.unit[slot];
	for (s = 0; s < n; s++) bfs_par[s] = -1;
	s = u->ty * mw + u->tx; bfs_par[s] = 8; bfs_q[qt++] = (uint16_t)s;
	while (qh < qt) {
		int c = bfs_q[qh++], cx = c % mw, cy = c / mw;
		if (rect_gap(cx, cy, gx, gy, gw, gh) <= range && c != s) {
			while (1) {           /* back to the first step */
				int p = bfs_par[c], px = (c % mw) - DX[p], py = (c / mw) - DY[p];
				if (py * mw + px == s) { *nx = c % mw; *ny = c / mw; return 1; }
				c = py * mw + px;
			}
		}
		for (d = 0; d < 8; d++) {
			int x = cx + DX[d], y = cy + DY[d], i;
			if (!pass(slot, x, y, ign)) continue;
			if (DX[d] && DY[d] && (!pass(slot, cx + DX[d], cy, 1) || !pass(slot, cx, cy + DY[d], 1))) continue;
			i = y * mw + x;
			if (bfs_par[i] >= 0) continue;
			bfs_par[i] = (int16_t)d; bfs_q[qt++] = (uint16_t)i;
		}
	}
	return 0;
}

static int mover(int slot, int gx, int gy, int gw, int gh, int range)
{
	W2Unit *u = &w2.unit[slot];
	int nx, ny, i;
	if (mv_wait[slot]) { mv_wait[slot]--; return 0; }
	if (rect_gap(u->tx, u->ty, gx, gy, gw, gh) <= range) { mv_block[slot] = 0; return 1; }
	if (!bfs(slot, gx, gy, gw, gh, range, 0, &nx, &ny)) {
		if (!bfs(slot, gx, gy, gw, gh, range, 1, &nx, &ny)) return -1;
	}
	i = ny * w2.map.w + nx;
	if (w2.map.occ_ground[i] != W2_NO_UNIT) {          /* blocked by a unit: wait a bit */
		if (++mv_block[slot] > 60) { mv_block[slot] = 0; return -1; }
		mv_wait[slot] = 3;
		return 0;
	}
	mv_block[slot] = 0;
	unit_set_dir_to(slot, nx * 32 + 16, ny * 32 + 16);
	unit_place(slot, nx, ny);
	if (u->anim_kind != W2_ANIM_MOVE) unit_set_anim(slot, W2_ANIM_MOVE);
	mv_wait[slot] = (u->flags & W2_UF_SEA) ? 15 : 15;   /* 16 cycles per tile */
	return 0;
}

int move_toward(int slot, int gx, int gy, int range) { return mover(slot, gx, gy, 1, 1, range); }
int move_toward_unit(int slot, int t, int range)
{
	const w2_unit_type *tt = w2_unit(w2.unit[t].type);
	return mover(slot, w2.unit[t].tx, w2.unit[t].ty, tt->tile_w, tt->tile_h, range);
}
void move_stop(int slot) { (void)slot; }

/* ------------------------------------------------------------------ PNG (stored deflate) */
static unsigned long crc_tab[256];
static unsigned long crc(unsigned long c, const unsigned char *b, size_t n)
{
	size_t i;
	if (!crc_tab[1]) { unsigned long k, v; int j; for (k = 0; k < 256; k++) { v = k; for (j = 0; j < 8; j++) v = (v & 1) ? 0xEDB88320UL ^ (v >> 1) : v >> 1; crc_tab[k] = v; } }
	c ^= 0xFFFFFFFFUL;
	for (i = 0; i < n; i++) c = crc_tab[(c ^ b[i]) & 0xFF] ^ (c >> 8);
	return c ^ 0xFFFFFFFFUL;
}
static void be32(unsigned char *p, unsigned long v) { p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16); p[2] = (unsigned char)(v >> 8); p[3] = (unsigned char)v; }
static void chunk(FILE *f, const char *t, const unsigned char *d, size_t n)
{
	unsigned char h[8]; unsigned long c;
	be32(h, (unsigned long)n); memcpy(h + 4, t, 4); fwrite(h, 1, 8, f);
	if (n) fwrite(d, 1, n, f);
	c = crc(0, (const unsigned char *)t, 4); c = crc(c, d, n);
	be32(h, c); fwrite(h, 1, 4, f);
}
static void png_write(const char *name, const unsigned char *pix, int w, int h, int pitch)
{
	char path[512]; FILE *f; size_t raw = (size_t)(w * 3 + 1) * h, zn, pos = 0, o = 0; unsigned char *r, *z, ih[13];
	unsigned long a = 1, b = 0; const unsigned char *pal = assets_palette(); int x, y;
	snprintf(path, sizeof path, "%s/%s", g_out, name);
	r = malloc(raw); z = malloc(raw + raw / 65535 * 5 + 64);
	for (y = 0; y < h; y++) {
		r[pos++] = 0;
		for (x = 0; x < w; x++) { int c = pix[y * pitch + x]; r[pos++] = pal[c * 3]; r[pos++] = pal[c * 3 + 1]; r[pos++] = pal[c * 3 + 2]; }
	}
	z[o++] = 0x78; z[o++] = 0x01;
	for (pos = 0; pos < raw;) {
		size_t n = raw - pos > 65535 ? 65535 : raw - pos;
		z[o++] = (unsigned char)(pos + n == raw); z[o++] = (unsigned char)n; z[o++] = (unsigned char)(n >> 8);
		z[o++] = (unsigned char)~n; z[o++] = (unsigned char)(~n >> 8);
		memcpy(z + o, r + pos, n); o += n; pos += n;
	}
	for (pos = 0; pos < raw; pos++) { a = (a + r[pos]) % 65521; b = (b + a) % 65521; }
	be32(z + o, (b << 16) | a); o += 4; zn = o;
	f = fopen(path, "wb");
	if (!f) { printf("  cannot write %s\n", path); free(r); free(z); return; }
	fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
	be32(ih, (unsigned long)w); be32(ih + 4, (unsigned long)h); ih[8] = 8; ih[9] = 2; ih[10] = ih[11] = ih[12] = 0;
	chunk(f, "IHDR", ih, 13); chunk(f, "IDAT", z, zn); chunk(f, "IEND", NULL, 0);
	fclose(f); free(r); free(z);
	printf("  png  %s\n", path);
}

static unsigned char g_shot[640 * 480];
static void shot(const char *name, int tx, int ty)
{
	memset(g_shot, 0, sizeof g_shot);
	view.buf = g_shot; view.pitch = 640; view.lores = 0;
	view.scr_x = 0; view.scr_y = 0; view.scr_w = 640; view.scr_h = 480;
	view.map_x = tx * 32; view.map_y = ty * 32;
	if (view.map_x > w2.map.w * 32 - 640) view.map_x = w2.map.w * 32 - 640;
	if (view.map_y > w2.map.h * 32 - 480) view.map_y = w2.map.h * 32 - 480;
	if (view.map_x < 0) view.map_x = 0;
	if (view.map_y < 0) view.map_y = 0;
	mapdraw_draw();
	unitdraw_draw();
	png_write(name, g_shot, 640, 480, 640);
}

/* ------------------------------------------------------------------ world helpers */
static int load(int entry)
{
	int p;
	w2.rng = 12345;
	if (pud_load_builtin(entry)) { printf("cannot load map %d\n", entry); return -1; }
	econ_reset();
	memset(mv_wait, 0, sizeof mv_wait); memset(mv_block, 0, sizeof mv_block);
	w2.local_player = 0;
	w2.rng = 12345;
	w2.cycle = 0;
	w2_think_period = 1;
	for (p = 0; p < 8; p++) if (w2.player[p].control != W2_CTRL_NOBODY) w2.player[p].control = p ? W2_CTRL_COMPUTER : W2_CTRL_HUMAN;
	w2.player[0].control = W2_CTRL_HUMAN;
	memset(w2.map.explored, 0xFF, sizeof w2.map.explored);
	memset(w2.map.visible, 0xFF, sizeof w2.map.visible);
	mapdraw_reset();
	g_msgs = 0; g_last_msg[0] = 0; g_ready_snd = 0;
	return 0;
}

static void run(int n) { while (n-- > 0) { unit_tick_all(); w2.cycle++; } }

static int start_x(int p) { return w2.player[p].start_tx != 0xFFFF ? w2.player[p].start_tx : w2.map.w / 2; }
static int start_y(int p) { return w2.player[p].start_ty != 0xFFFF ? w2.player[p].start_ty : w2.map.h / 2; }

static int nearest_type_flag(uint32_t flag, int x, int y)
{
	int s, best = -1; long bd = 1L << 30;
	for (s = 0; s < w2.unit_hi; s++) {
		const w2_unit_type *t;
		long dx, dy;
		if (!(w2.unit[s].flags & W2_UF_ALIVE)) continue;
		t = w2_unit(w2.unit[s].type);
		if (!(t->flags & flag)) continue;
		dx = w2.unit[s].tx - x; dy = w2.unit[s].ty - y;
		if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; best = s; }
	}
	return best;
}

/* the placement closest to (x,y) that econ_place_ok accepts */
static int find_place(int p, int type, int x, int y, int *ox, int *oy)
{
	int r, dx, dy;
	for (r = 0; r < 40; r++)
		for (dy = -r; dy <= r; dy++)
			for (dx = -r; dx <= r; dx++) {
				if (dx != -r && dx != r && dy != -r && dy != r) continue;
				if (econ_place_ok(p, type, x + dx, y + dy, -1)) { *ox = x + dx; *oy = y + dy; return 1; }
			}
	return 0;
}

static int build_now(int p, int type, int x, int y)
{
	int bx, by;
	if (!find_place(p, type, x, y, &bx, &by)) return -1;
	return unit_create(type, p, bx, by);
}

static int spawn_near(int p, int type, int b)
{
	const w2_unit_type *t = w2_unit(w2.unit[b].type);
	int r, x, y;
	for (r = 1; r < 6; r++)
		for (y = w2.unit[b].ty - r; y < w2.unit[b].ty + t->tile_h + r; y++)
			for (x = w2.unit[b].tx - r; x < w2.unit[b].tx + t->tile_w + r; x++) {
				int i;
				if (x < 0 || y < 0 || x >= w2.map.w || y >= w2.map.h) continue;
				i = y * w2.map.w + x;
				if ((w2.map.flags[i] & (W2_TF_NOWALK | W2_TF_BUILDING)) || w2.map.occ_ground[i] != W2_NO_UNIT) continue;
				return unit_create(type, p, x, y);
			}
	return -1;
}

static uint32_t world_hash(void)
{
	uint32_t h = 2166136261u; const unsigned char *b; size_t i, n;
	b = (const unsigned char *)w2.unit; n = sizeof(W2Unit) * w2.unit_hi;
	for (i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
	b = (const unsigned char *)w2.player; n = sizeof w2.player;
	for (i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
	b = (const unsigned char *)w2.map.tile; n = sizeof(uint16_t) * w2.map.w * w2.map.h;
	for (i = 0; i < n; i++) h = (h ^ b[i]) * 16777619u;
	return h;
}

/* ------------------------------------------------------------------ scenarios */
static int g_hall, g_mine;

/* map 220, player 0: town hall at the start, nearest gold mine */
static int setup_base(int entry)
{
	int sx, sy;
	if (load(entry)) return -1;
	sx = start_x(0); sy = start_y(0);
	{
		int s;
		for (s = 0; s < w2.unit_hi; s++) if (w2.unit[s].flags && w2.unit[s].owner == 0) unit_remove(s);
	}
	g_mine = nearest_type_flag(W2UF_GOLD_MINE, sx, sy);
	g_hall = build_now(0, W2U_TOWN_HALL, sx, sy);
	return (g_hall >= 0 && g_mine >= 0) ? 0 : -1;
}

static void probe(void)
{
	int e, s;
	printf("== probe built-in skirmish maps\n");
	for (e = 220; e <= 240; e++) {
		int mines = 0, patches = 0, water = 0, i, n, p;
		if (pud_load_builtin(e)) continue;
		for (s = 0; s < w2.unit_hi; s++) {
			const w2_unit_type *t = w2_unit(w2.unit[s].type);
			if (!(w2.unit[s].flags & W2_UF_ALIVE)) continue;
			if (t->flags & W2UF_GOLD_MINE) mines++;
			if (t->flags & W2UF_OIL_PATCH) patches++;
		}
		n = w2.map.w * w2.map.h;
		for (i = 0; i < n; i++) if (w2.map.flags[i] & W2_TF_WATER) water++;
		printf("  map %d: %dx%d era %d units %d mines %d oil patches %d water %d%% starts:", e, w2.map.w, w2.map.h,
		       w2.map.era, w2.unit_hi, mines, patches, water * 100 / n);
		for (p = 0; p < 8; p++) if (w2.player[p].start_tx != 0xFFFF) printf(" p%d(%d,%d)", p, w2.player[p].start_tx, w2.player[p].start_ty);
		printf("\n");
	}
}

static void test_strings(void)
{
	printf("== original message strings (War Strings 1)\n");
	CHECK(!strncmp(assets_string(1, ECON_STR_GOLD), "Not enough gold", 15), "387 = '%s'", assets_string(1, ECON_STR_GOLD));
	CHECK(!strncmp(assets_string(1, ECON_STR_LUMBER), "Not enough lumber", 17), "388 = '%s'", assets_string(1, ECON_STR_LUMBER));
	CHECK(!strncmp(assets_string(1, ECON_STR_OIL), "Not enough oil", 14), "389 = '%s'", assets_string(1, ECON_STR_OIL));
	CHECK(!strncmp(assets_string(1, ECON_STR_FOOD), "Not enough food", 15), "386 = '%s'", assets_string(1, ECON_STR_FOOD));
	CHECK(!strncmp(assets_string(1, 395), "You cannot build a townhall", 27), "395 = '%s'", assets_string(1, 395));
}

/* gold: 1 peasant until N trips; trip time */
static void test_gold_trips(void)
{
	int pe, trips = 0, last_gold, last_cycle = 0, c, gap = 0, mine_frames = 0, inside_seen = 0;
	uint32_t left0;
	printf("== gold: one peasant, 5 trips (map 220)\n");
	if (setup_base(220)) { CHECK(0, "setup"); return; }
	pe = spawn_near(0, W2U_PEASANT, g_hall);
	w2.player[0].gold = 0;
	left0 = w2.unit[g_mine].res_left;
	printf("  hall slot %d at (%d,%d), mine slot %d at (%d,%d) res_left %u, peasant %d at (%d,%d)\n",
	       g_hall, w2.unit[g_hall].tx, w2.unit[g_hall].ty, g_mine, w2.unit[g_mine].tx, w2.unit[g_mine].ty,
	       (unsigned)left0, pe, w2.unit[pe].tx, w2.unit[pe].ty);
	cmd_harvest(pe, g_mine, 0, 0);
	last_gold = 0;
	for (c = 0; c < 20000 && trips < 5; c++) {
		run(1);
		if (w2.unit[pe].flags & W2_UF_INSIDE) inside_seen = 1;
		if (w2.unit[g_mine].anim_kind == W2_ANIM_ACTIVE) mine_frames++;
		if (w2.player[0].gold != last_gold) {
			trips++;
			printf("  trip %d at cycle %u: gold %d (+%d), %d cycles since last\n", trips, (unsigned)w2.cycle,
			       w2.player[0].gold, w2.player[0].gold - last_gold, (int)w2.cycle - last_cycle);
			if (trips > 1) gap = (int)w2.cycle - last_cycle;
			last_gold = w2.player[0].gold; last_cycle = (int)w2.cycle;
		}
		if (c == 200) shot("gold_walk.png", w2.unit[g_hall].tx - 6, w2.unit[g_hall].ty - 4);
	}
	CHECK(trips == 5 && w2.player[0].gold == 500, "5 trips, gold %d", w2.player[0].gold);
	CHECK(w2.unit[g_mine].res_left == left0 - 500, "mine res_left %u -> %u", (unsigned)left0, (unsigned)w2.unit[g_mine].res_left);
	CHECK(inside_seen && mine_frames >= 5 * 150, "peasant went inside, mine ACTIVE for %d cycles", mine_frames);
	CHECK(gap > 300, "steady trip %d cycles", gap);
	{
		const w2_unit_type *ht = w2_unit(W2U_TOWN_HALL);
		int d = 0, x;
		(void)ht; (void)x;
		printf("  (theory: 150 in mine + 150 in hall + 2 x walk; walk 16 cycles/tile)\n");
		(void)d;
	}
}

/* gold per minute with 1..5 peasants, wood per minute with 1..5 */
static void test_curves(void)
{
	int n, k;
	printf("== resource curves (3 game minutes = 5400 cycles at 30 c/s, map 220)\n");
	for (k = 0; k < 2; k++) {
		for (n = 1; n <= 5; n++) {
			int i, pe[5], tx = 0, ty = 0;
			if (setup_base(220)) { CHECK(0, "setup"); return; }
			w2.player[0].gold = 0; w2.player[0].wood = 0;
			for (i = 0; i < n; i++) pe[i] = spawn_near(0, W2U_PEASANT, g_hall);
			if (k == 1) econ_find_tree(w2.unit[g_hall].tx, w2.unit[g_hall].ty, 30, &tx, &ty);
			for (i = 0; i < n; i++) cmd_harvest(pe[i], k == 0 ? g_mine : -1, tx, ty);
			run(5400);
			printf("  %s, %d peasant%s: %5d per minute (%d in 3 min)\n", k ? "wood" : "gold", n, n > 1 ? "s" : " ",
			       (k ? w2.player[0].wood : w2.player[0].gold) / 3, k ? w2.player[0].wood : w2.player[0].gold);
		}
	}
}

/* wood: 2 peasants chop, trees become stumps; PNG before/after */
static void test_wood(void)
{
	int pe[3], i, tx, ty, trees0 = 0, trees1 = 0, x, y, vx, vy, stumps = 0, bad = 0;
	printf("== wood: 3 peasants chopping (map 220)\n");
	if (setup_base(220)) { CHECK(0, "setup"); return; }
	w2.player[0].wood = 0;
	for (i = 0; i < 3; i++) pe[i] = spawn_near(0, W2U_PEASANT, g_hall);
	if (!econ_find_tree(w2.unit[g_hall].tx, w2.unit[g_hall].ty, 30, &tx, &ty)) { CHECK(0, "no tree"); return; }
	vx = tx - 10; vy = ty - 7;
	for (y = 0; y < w2.map.h; y++) for (x = 0; x < w2.map.w; x++) if (w2.map.flags[y * w2.map.w + x] & W2_TF_TREE) trees0++;
	shot("wood_before.png", vx, vy);
	for (i = 0; i < 3; i++) cmd_harvest(pe[i], -1, tx, ty);
	run(400);
	shot("wood_chopping.png", vx, vy);
	run(30 * 60 * 4);
	shot("wood_after.png", vx, vy);
	for (y = 0; y < w2.map.h; y++)
		for (x = 0; x < w2.map.w; x++) {
			int j = y * w2.map.w + x;
			if (w2.map.flags[j] & W2_TF_TREE) trees1++;
			if (w2.map.mega[j] == 126) {
				stumps++;
				if (w2.map.flags[j] & (W2_TF_TREE | W2_TF_NOWALK)) bad++;
			}
		}
	printf("  wood %d, tree tiles %d -> %d, stump tiles %d\n", w2.player[0].wood, trees0, trees1, stumps);
	CHECK(w2.player[0].wood >= 500, "wood gathered %d", w2.player[0].wood);
	CHECK(trees1 < trees0 && stumps > 0 && !bad, "tree tiles disappear, stumps are walkable land");
	for (i = 0; i < 3; i++) printf("  peasant %d at (%d,%d) order %d state %d goal (%d,%d) res %d/%d\n", pe[i], w2.unit[pe[i]].tx, w2.unit[pe[i]].ty,
	                               w2.unit[pe[i]].order, w2.unit[pe[i]].order_state, w2.unit[pe[i]].goal_tx, w2.unit[pe[i]].goal_ty, w2.unit[pe[i]].res_kind, w2.unit[pe[i]].res_amount);
	for (i = 0; i < 3; i++) CHECK(w2.unit[pe[i]].order == W2_ORD_HARVEST || w2.unit[pe[i]].order == W2_ORD_RETURN,
	                                "peasant %d still working (order %d)", i, w2.unit[pe[i]].order);
}

/* build farm + barracks, train footmen, food limit, insufficient resources, research, keep */
static void test_build_train(void)
{
	int pe, pe2, farm, bar, smith, i, c, x, y, trained = 0, ok;
	W2Player *P;
	printf("== build / train / research / upgrade (map 220)\n");
	if (setup_base(220)) { CHECK(0, "setup"); return; }
	P = &w2.player[0];
	P->gold = 20000; P->wood = 10000; P->oil = 1000;
	pe = spawn_near(0, W2U_PEASANT, g_hall);
	pe2 = spawn_near(0, W2U_PEASANT, g_hall);
	econ_recount(0);
	printf("  food %d/%d\n", P->food_used, P->food_supply);
	CHECK(P->food_supply == 1 && P->food_used == 2, "town hall supplies 1 food, 2 peasants use 2");

	/* insufficient: barracks needs 700 gold */
	P->gold = 100;
	find_place(0, W2U_HUMAN_BARRACKS, w2.unit[g_hall].tx + 6, w2.unit[g_hall].ty, &x, &y);
	g_msgs = 0;
	cmd_build(pe, W2U_HUMAN_BARRACKS, x, y);
	CHECK(w2.unit[pe].order != W2_ORD_BUILD && g_msgs == 1 && strstr(g_last_msg, "gold"), "no gold -> '%s'", g_last_msg);
	P->gold = 20000; P->wood = 100;
	cmd_build(pe, W2U_HUMAN_BARRACKS, x, y);
	CHECK(w2.unit[pe].order != W2_ORD_BUILD && strstr(g_last_msg, "lumber"), "no lumber -> '%s'", g_last_msg);
	P->wood = 10000;
	/* placement refusals */
	CHECK(econ_place_reason(0, W2U_FARM, w2.unit[g_hall].tx, w2.unit[g_hall].ty, -1) == 391, "farm on the town hall refused (391)");
	{
		int n395 = 0, nok = 0, xx, yy;
		for (yy = 0; yy < w2.map.h; yy++)
			for (xx = 0; xx < w2.map.w; xx++) {
				int r = econ_place_reason(0, W2U_TOWN_HALL, xx, yy, -1);
				if (r == 395) n395++;
				if (!r) nok++;
			}
		CHECK(n395 > 0 && nok > 0, "town hall placements: %d refused as too near a gold mine (395), %d allowed", n395, nok);
	}
	CHECK(econ_place_reason(0, W2U_FARM, w2.map.w - 1, 0, -1) == 390, "off the map refused (390)");
	CHECK(econ_place_reason(0, W2U_HUMAN_SHIPYARD, x, y, -1) == 392, "shipyard inland refused (392)");
	/* stables need a keep */
	CHECK(!econ_allowed_unit(0, W2U_STABLES), "stables not allowed without a keep");

	/* farm + barracks */
	find_place(0, W2U_FARM, w2.unit[g_hall].tx - 3, w2.unit[g_hall].ty + 5, &x, &y);
	cmd_build(pe2, W2U_FARM, x, y);
	{
		int bx, by; find_place(0, W2U_HUMAN_BARRACKS, w2.unit[g_hall].tx - 2, w2.unit[g_hall].ty + 9, &bx, &by);
		cmd_build(pe, W2U_HUMAN_BARRACKS, bx, by);
	}
	CHECK(w2.unit[pe].order == W2_ORD_BUILD && w2.unit[pe2].order == W2_ORD_BUILD, "both peasants have build orders");
	farm = bar = -1;
	for (c = 0; c < 4000 && (farm < 0 || bar < 0); c++) {
		run(1);
		if (farm < 0 && w2.unit[pe2].order == W2_ORD_BUILD && w2.unit[pe2].order_state == 2) farm = w2.unit[pe2].target;
		if (bar < 0 && w2.unit[pe].order == W2_ORD_BUILD && w2.unit[pe].order_state == 2) bar = w2.unit[pe].target;
	}
	CHECK(farm >= 0 && bar >= 0, "construction started (farm %d barracks %d) at cycle %u, gold %d wood %d",
	      farm, bar, (unsigned)w2.cycle, P->gold, P->wood);
	if (farm < 0 || bar < 0) return;
	CHECK((w2.unit[pe].flags & W2_UF_INSIDE) && (w2.unit[farm].flags & W2_UF_UNDERCONST) && w2.unit[farm].hp < 20,
	      "builder inside, farm under construction hp %d", w2.unit[farm].hp);
	run(250);
	CHECK(econ_progress_percent(farm) > 30 && econ_progress_percent(farm) < 60, "farm %d%% hp %d/400", econ_progress_percent(farm), w2.unit[farm].hp);
	shot("build_progress.png", w2.unit[g_hall].tx - 6, w2.unit[g_hall].ty - 4);
	run(400);
	CHECK(!(w2.unit[farm].flags & W2_UF_UNDERCONST) && w2.unit[farm].hp == 400, "farm done, hp %d", w2.unit[farm].hp);
	run(2);
	CHECK(!(w2.unit[pe2].flags & W2_UF_INSIDE) && w2.unit[pe2].order == W2_ORD_STILL, "farm builder came out, idle");
	CHECK(P->food_supply == 5, "food supply %d (hall 1 + farm 4)", P->food_supply);
	for (c = 0; c < 2000 && (w2.unit[bar].flags & W2_UF_UNDERCONST); c++) run(1);
	run(2);
	CHECK(!(w2.unit[bar].flags & W2_UF_UNDERCONST) && w2.unit[bar].hp == 800, "barracks done after %u cycles total", (unsigned)w2.cycle);

	/* train footmen until food runs out: supply 5, used 2 -> 3 footmen */
	g_msgs = 0;
	for (i = 0; i < 5; i++) {
		cmd_train(bar, W2U_FOOTMAN);
		if (w2.unit[bar].order != W2_ORD_TRAIN) break;
		CHECK(econ_progress_percent(bar) == 0, "footman %d queued (gold %d)", i + 1, P->gold);
		cmd_train(bar, W2U_FOOTMAN);    /* busy: ignored (one at a time) */
		for (c = 0; c < 400 && w2.unit[bar].order == W2_ORD_TRAIN; c++) run(1);
		trained++;
	}
	econ_recount(0);
	CHECK(trained == 3 && strstr(g_last_msg, "food"), "%d footmen trained, then '%s' (food %d/%d)", trained, g_last_msg, P->food_used, P->food_supply);
	CHECK(g_ready_snd >= 5, "ready sounds %d", g_ready_snd);
	shot("trained.png", w2.unit[bar].tx - 6, w2.unit[bar].ty - 4);

	/* cancel training refunds */
	{
		int f2, g0;
		f2 = build_now(0, W2U_FARM, w2.unit[g_hall].tx, w2.unit[g_hall].ty + 6);
		econ_recount(0);
		g0 = P->gold;
		cmd_train(bar, W2U_FOOTMAN);
		run(10);
		cmd_cancel(bar);
		CHECK(P->gold == g0 && w2.unit[bar].order == W2_ORD_STILL, "cancel training refunds (gold %d)", P->gold);
		(void)f2;
	}

	/* research swords at a blacksmith */
	smith = build_now(0, W2U_HUMAN_BLACKSMITH, w2.unit[g_hall].tx + 3, w2.unit[g_hall].ty + 12);
	{
		int ft = -1;
		for (i = 0; i < w2.unit_hi; i++) if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].type == W2U_FOOTMAN) { ft = i; break; }
		CHECK(econ_stat(ft, W2_STAT_PIERCING) == 3 && !econ_allowed_upgrade(0, W2UP_SWORD2), "footman piercing 3, sword2 locked");
		cmd_research(smith, W2UP_SWORD1);
		CHECK(w2.unit[smith].order == W2_ORD_RESEARCH && econ_research_in_progress(0, W2UP_SWORD1), "research started");
		cmd_research(smith, W2UP_SWORD1);
		for (c = 0; c < 2000 && w2.unit[smith].order == W2_ORD_RESEARCH; c++) run(1);
		CHECK(econ_has_upgrade(0, W2UP_SWORD1) && econ_stat(ft, W2_STAT_PIERCING) == 5 && econ_allowed_upgrade(0, W2UP_SWORD2),
		      "sword1 after %d cycles: footman piercing %d, sword2 allowed", c, econ_stat(ft, W2_STAT_PIERCING));
		CHECK(econ_has_upgrade(0, W2UP_HOLY_VISION) && !econ_has_upgrade(0, W2UP_HEALING), "free spell known, healing not");
	}

	/* town hall -> keep (needs barracks), then stables allowed, gold +10% */
	{
		int g0 = P->gold;
		cmd_upgrade_to(g_hall, W2U_KEEP);
		CHECK(w2.unit[g_hall].order == W2_ORD_UPGRADE_TO && P->gold == g0 - 2000, "keep upgrade started (gold %d)", P->gold);
		for (c = 0; c < 2000 && w2.unit[g_hall].order == W2_ORD_UPGRADE_TO; c++) run(1);
		CHECK(w2.unit[g_hall].type == W2U_KEEP && w2.unit[g_hall].hp == 1400, "town hall is a keep after %d cycles, hp %d", c, w2.unit[g_hall].hp);
		CHECK(econ_allowed_unit(0, W2U_STABLES) && !econ_allowed_unit(0, W2U_CASTLE), "stables now allowed, castle not yet");
		shot("keep.png", w2.unit[g_hall].tx - 6, w2.unit[g_hall].ty - 4);
		P->gold = 0;
		cmd_harvest(pe2, g_mine, 0, 0);
		for (c = 0; c < 4000 && P->gold == 0; c++) run(1);
		printf("  pe2 %d at (%d,%d) order %d state %d flags %x target %d\n", pe2, w2.unit[pe2].tx, w2.unit[pe2].ty, w2.unit[pe2].order, w2.unit[pe2].order_state, w2.unit[pe2].flags, w2.unit[pe2].target);
		CHECK(P->gold == 110, "a gold trip into a keep yields %d (100 + 10%%)", P->gold);
	}

	/* construction cancel refunds 75 % */
	{
		int g0, w0, s0, site = -1;
		P->gold = 5000; P->wood = 5000;
		find_place(0, W2U_FARM, w2.unit[g_hall].tx - 4, w2.unit[g_hall].ty - 4, &x, &y);
		cmd_build(pe2, W2U_FARM, x, y);
		for (c = 0; c < 2000 && site < 0; c++) { run(1); if (w2.unit[pe2].order_state == 2 && w2.unit[pe2].order == W2_ORD_BUILD) site = w2.unit[pe2].target; }
		g0 = P->gold; w0 = P->wood; s0 = P->food_supply;
		cmd_cancel(site);
		run(2);
		CHECK(site >= 0 && P->gold == g0 + 375 && P->wood == w0 + 187 && !(w2.unit[pe2].flags & W2_UF_INSIDE),
		      "cancel construction: +375 gold +187 wood, builder out");
		(void)s0;
	}
	ok = 1; (void)ok;
}

/* oil: tanker builds a platform on a patch, shipyard, hauls oil */
static void test_oil(int entry)
{
	int patch, s, x, y, yard, tank, plat = -1, c, trips = 0, last;
	printf("== oil (map %d)\n", entry);
	if (load(entry)) { CHECK(0, "load"); return; }
	patch = nearest_type_flag(W2UF_OIL_PATCH, start_x(0), start_y(0));
	if (patch < 0) { CHECK(0, "no oil patch"); return; }
	printf("  oil patch slot %d at (%d,%d) res_left %u\n", patch, w2.unit[patch].tx, w2.unit[patch].ty, (unsigned)w2.unit[patch].res_left);
	if (!w2.unit[patch].res_left) w2.unit[patch].res_left = 50000;
	w2.player[0].gold = 20000; w2.player[0].wood = 20000; w2.player[0].oil = 0;
	{
		int hist[400], i, n = w2.map.w * w2.map.h, sh = 0;
		memset(hist, 0, sizeof hist);
		for (i = 0; i < n; i++) if (w2.map.tile[i] >= 0x100 && ((w2.map.tile[i] >> 8) & 15) == 2) sh++;
		for (y = 0; y < w2.map.h - 2; y++) for (x = 0; x < w2.map.w - 2; x++) hist[econ_place_reason(0, W2U_HUMAN_SHIPYARD, x, y, -1)]++;
		printf("  shore tiles %d; shipyard reasons: ok %d 391 %d 392 %d 396 %d\n", sh, hist[0], hist[391], hist[392], hist[396]);
		for (i = 0; i < n; i++) if (w2.map.tile[i] >= 0x100 && ((w2.map.tile[i] >> 8) & 15) == 2) break;
		if (getenv("ECON_DUMP"))
			for (y = i / w2.map.w - 4; y < i / w2.map.w + 5; y++) {
				for (x = i % w2.map.w - 6; x < i % w2.map.w + 7; x++)
					if (x >= 0 && y >= 0 && x < w2.map.w && y < w2.map.h) printf(" %04x/%03x", w2.map.tile[y * w2.map.w + x], w2.map.flags[y * w2.map.w + x]);
				printf("\n");
			}
	}
	if (!find_place(0, W2U_HUMAN_SHIPYARD, w2.unit[patch].tx, w2.unit[patch].ty, &x, &y)) { CHECK(0, "no shipyard place"); return; }
	yard = unit_create(W2U_HUMAN_SHIPYARD, 0, x, y);
	CHECK(yard >= 0, "shipyard placed on the shore at (%d,%d), patch at (%d,%d)", x, y, w2.unit[patch].tx, w2.unit[patch].ty);
	/* tanker on water next to the shipyard */
	tank = -1;
	{
		int r, xx, yy;
		for (r = 1; r < 6 && tank < 0; r++)
			for (yy = y - r; yy < y + 3 + r && tank < 0; yy++)
				for (xx = x - r; xx < x + 3 + r && tank < 0; xx++) {
					int i;
					if (xx < 0 || yy < 0 || xx >= w2.map.w || yy >= w2.map.h) continue;
					i = yy * w2.map.w + xx;
					if ((w2.map.flags[i] & W2_TF_WATER) && !(w2.map.flags[i] & W2_TF_BUILDING) && w2.map.occ_ground[i] == W2_NO_UNIT)
						tank = unit_create(W2U_HUMAN_OIL_TANKER, 0, xx, yy);
				}
	}
	CHECK(tank >= 0, "tanker created");
	if (tank < 0) return;
	CHECK(econ_place_reason(0, W2U_HUMAN_OIL_PLATFORM, w2.unit[patch].tx + 1, w2.unit[patch].ty, tank) == 394, "platform off the patch refused (394)");
	cmd_build(tank, W2U_HUMAN_OIL_PLATFORM, w2.unit[patch].tx, w2.unit[patch].ty);
	CHECK(w2.unit[tank].order == W2_ORD_BUILD, "tanker builds a platform");
	for (c = 0; c < 6000; c++) {
		run(1);
		if (plat < 0 && w2.unit[tank].order_state == 2) plat = w2.unit[tank].target;
		if (plat >= 0 && !(w2.unit[plat].flags & W2_UF_UNDERCONST) && w2.unit[tank].order == W2_ORD_STILL) break;
	}
	CHECK(plat >= 0 && !(w2.unit[plat].flags & W2_UF_UNDERCONST) && w2.unit[plat].res_left > 0,
	      "platform built at cycle %u, holds %u oil", (unsigned)w2.cycle, plat >= 0 ? (unsigned)w2.unit[plat].res_left : 0);
	if (plat < 0) return;
	for (s = 0; s < w2.unit_hi; s++) if ((w2.unit[s].flags & W2_UF_ALIVE) && w2.unit[s].type == W2U_OIL_PATCH && s == patch) break;
	CHECK(!(w2.unit[patch].flags & W2_UF_ALIVE) || w2.unit[patch].type != W2U_OIL_PATCH, "oil patch replaced by the platform");
	cmd_harvest(tank, plat, 0, 0);
	last = 0;
	for (c = 0; c < 6000 && trips < 3; c++) {
		run(1);
		if (w2.player[0].oil != last) { trips++; printf("  oil trip %d at cycle %u: oil %d\n", trips, (unsigned)w2.cycle, w2.player[0].oil); last = w2.player[0].oil; }
	}
	CHECK(trips == 3 && w2.player[0].oil == 300, "3 oil trips, oil %d", w2.player[0].oil);
	shot("oil.png", w2.unit[yard].tx - 8, w2.unit[yard].ty - 6);
}

/* same scenario twice -> identical world */
static uint32_t det_run(void)
{
	int i, pe[4], tx, ty;
	if (setup_base(220)) return 0;
	for (i = 0; i < 4; i++) pe[i] = spawn_near(0, W2U_PEASANT, g_hall);
	econ_find_tree(w2.unit[g_hall].tx, w2.unit[g_hall].ty, 30, &tx, &ty);
	cmd_harvest(pe[0], g_mine, 0, 0); cmd_harvest(pe[1], g_mine, 0, 0);
	cmd_harvest(pe[2], -1, tx, ty); cmd_harvest(pe[3], -1, tx, ty);
	w2.player[0].gold = 5000; w2.player[0].wood = 5000;
	build_now(0, W2U_FARM, w2.unit[g_hall].tx - 4, w2.unit[g_hall].ty + 6);
	cmd_train(g_hall, W2U_PEASANT);
	run(6000);
	return world_hash();
}

static void test_determinism(void)
{
	uint32_t a, b;
	printf("== determinism\n");
	a = det_run(); b = det_run();
	CHECK(a == b && a != 0, "two runs: %08x %08x", (unsigned)a, (unsigned)b);
}

static void test_tree_pieces(void)
{
	/* the special megatiles used for stumps / single trees: render them for a look */
	static unsigned char strip[32 * 6 * 32];
	int k, y; const int m[6] = { 121, 122, 123, 126, 125, 127 };
	printf("== tree special megatiles 121 122 123 126 (+125 127 solid forest)\n");
	if (load(220)) return;
	for (k = 0; k < 6; k++) {
		const unsigned char *t = assets_megatile(m[k]);
		for (y = 0; t && y < 32; y++) memcpy(strip + y * 192 + k * 32, t + y * 32, 32);
	}
	png_write("tree_megatiles.png", strip, 192, 32, 192);
}

int main(void)
{
	const char *d = getenv("ECON_DATA");
	if (getenv("ECON_OUT")) g_out = getenv("ECON_OUT");
	w2cfg_defaults(&w2_cfg);
	snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", d ? d : "/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/data");
	setvbuf(stdout, NULL, _IONBF, 0);
	if (assets_init()) { printf("assets_init failed (%s)\n", w2_cfg.data_dir); return 2; }
	probe();
	test_strings();
	test_tree_pieces();
	test_gold_trips();
	test_wood();
	test_build_train();
	test_curves();
	test_oil(getenv("ECON_OILMAP") ? atoi(getenv("ECON_OILMAP")) : 221);
	test_determinism();
	printf("== %s (%d failures)\n", g_fail ? "FAILED" : "ALL OK", g_fail);
	return g_fail ? 1 : 0;
}
