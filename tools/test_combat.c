/*
 * test_combat.c - host test of src/combat.c + src/missile.c.
 *
 * Usage: test_combat <data dir with "War Data"> <scratch out dir>
 *
 * The other engine modules are not there yet, so this file carries a MINIMAL stand-in for unit.c /
 * move.c (creation, occupancy, animation player with events, a greedy tile walker, ord_still that
 * auto-acquires) as strong definitions overriding src/w2_stubs.c. Terrain comes from the built-in
 * skirmish map 220 (its own units are not created). Skirmishes run until one side is dead and
 * print kill times / hp curves; PNG shots (mid-fight, missiles in flight) go to the scratch dir,
 * never into the repository (they contain original art).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "w2.h"
#include "pud.h"
#include "assets.h"
#include "w2cfg.h"
#include "combat.h"
#include "unit.h"

static const char *g_out = ".";
static int g_fail;
#define CHECK(c, ...) do { if (!(c)) { g_fail++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

/* ======================================================================================= platform */
void w2_log(const char *fmt, ...)
{
	va_list ap; if (!getenv("W2_TESTLOG")) return; va_start(ap, fmt); fprintf(stderr, "[log] "); vfprintf(stderr, fmt, ap); fprintf(stderr, "\n"); va_end(ap);
}
void amigagfx_set_palette(const unsigned char *rgb, int first, int count) { (void)rgb; (void)first; (void)count; }

/* counters of hooks called by combat */
static int g_msgs, g_help_snd, g_dead_snd, g_impact_snd;
void ui_message(const char *t) { (void)t; g_msgs++; }
void snd_unit(int slot, int what) { (void)slot; if (what == 3) g_help_snd++; if (what == 4) g_dead_snd++; }
void snd_play(int id, int px, int py) { (void)id; (void)px; (void)py; g_impact_snd++; }

/* ================================================= the real unit.c + a mini move.c stand-in */
static uint16_t blocked[W2_MAX_UNITS];

static const w2_unit_type *T(int s) { return w2_unit(w2.unit[s].type); }

static int heading8(int dx, int dy)
{
	int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
	if (!dx && !dy) return -1;
	if (ax * 12 < ay * 5) return dy < 0 ? 0 : 4;
	if (ay * 12 < ax * 5) return dx < 0 ? 6 : 2;
	if (dx > 0) return dy < 0 ? 1 : 3;
	return dy < 0 ? 7 : 5;
}

/* --- greedy walker --- */
static const int8_t DX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 }, DY[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
static int passable(int s, int x, int y)
{
	const w2_unit_type *t = T(s); int i;
	if (x < 0 || y < 0 || x >= w2.map.w || y >= w2.map.h) return 0;
	i = y * w2.map.w + x;
	if (t->kind == W2_KIND_AIR) return w2.map.occ_air[i] == W2_NO_UNIT;
	if (w2.map.occ_ground[i] != W2_NO_UNIT) return 0;
	if (t->kind == W2_KIND_SEA) return (w2.map.flags[i] & W2_TF_WATER) != 0;
	return !(w2.map.flags[i] & (W2_TF_NOWALK | W2_TF_WATER));
}
/* one step toward (gx,gy); returns 0 moving/started, -1 blocked for long */
static int step_toward(int s, int gx, int gy)
{
	W2Unit *u = &w2.unit[s]; int h, k;
	static const int8_t tryo[5] = { 0, 1, -1, 2, -2 };
	h = heading8(gx - u->tx, gy - u->ty);
	if (h < 0) return 1;
	for (k = 0; k < 5; k++) {
		int d = (h + tryo[k] + 8) & 7, nx = u->tx + DX[d], ny = u->ty + DY[d];
		if (!passable(s, nx, ny)) continue;
		unit_unplace(s);
		unit_place(s, nx, ny);
		u->ox = (int8_t)(-DX[d] * 32); u->oy = (int8_t)(-DY[d] * 32); u->dir = (uint8_t)d;
		unit_set_anim(s, W2_ANIM_MOVE);
		blocked[s] = 0;
		return 0;
	}
	if (u->anim_kind == W2_ANIM_MOVE) unit_set_anim(s, W2_ANIM_STILL);
	return ++blocked[s] > 90 ? -1 : 0;
}
int move_toward(int s, int gx, int gy, int range)
{
	W2Unit *u = &w2.unit[s];
	if (u->ox || u->oy) return 0;
	if (unit_dist_to_tile(s, gx, gy) <= range) { if (u->anim_kind == W2_ANIM_MOVE) unit_set_anim(s, W2_ANIM_STILL); return 1; }
	return step_toward(s, gx, gy);
}
int move_toward_unit(int s, int t, int range)
{
	W2Unit *u = &w2.unit[s];
	if (u->ox || u->oy) return 0;
	if (unit_dist_tiles(s, t) <= range) { if (u->anim_kind == W2_ANIM_MOVE) unit_set_anim(s, W2_ANIM_STILL); return 1; }
	return step_toward(s, w2.unit[t].tx, w2.unit[t].ty);
}
void move_stop(int s) { (void)s; }
void ord_move(int s)
{
	W2Unit *u = &w2.unit[s];
	if (move_toward(s, u->goal_tx, u->goal_ty, 0) != 0) unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, u->tx, u->ty);
}
void ord_follow(int s)
{
	W2Unit *u = &w2.unit[s];
	if (u->target >= W2_MAX_UNITS || !(w2.unit[u->target].flags & W2_UF_ALIVE)) { unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, 0, 0); return; }
	move_toward_unit(s, u->target, 1);
}
void ord_still(int s)
{
	W2Unit *u = &w2.unit[s]; int t;
	if (u->ox || u->oy) { move_toward(s, u->tx, u->ty, 0); return; }
	if (u->anim_kind != W2_ANIM_STILL && T(s)->kind != W2_KIND_BUILDING) unit_set_anim(s, W2_ANIM_STILL);
	if (unit_thinks(s) && (t = combat_acquire(s)) >= 0) cmd_attack(s, t);   /* same-cycle convention */
}
void ord_stand(int s)
{
	W2Unit *u = &w2.unit[s]; int t;
	if (u->ox || u->oy) { move_toward(s, u->tx, u->ty, 0); return; }
	if (unit_thinks(s) && (t = combat_acquire(s)) >= 0) combat_engage(s, t);
}
void cmd_move(int s, int tx, int ty) { unit_set_order(s, W2_ORD_MOVE, W2_NO_UNIT, tx, ty); }
void cmd_follow(int s, int t) { unit_set_order(s, W2_ORD_FOLLOW, t, 0, 0); }

/* one logic cycle like game.c: units (unit.c dispatches the orders), the mini mover's pixel offsets,
 * missiles */
static void game_cycle(void)
{
	int s;
	unit_tick_all();
	for (s = 0; s < w2.unit_hi; s++) {
		W2Unit *u = &w2.unit[s];
		int px;
		if (!(u->flags & W2_UF_ALIVE) || (!u->ox && !u->oy)) continue;
		px = u->anim_kind == W2_ANIM_MOVE ? unit_anim_move_pixels(s) : 2;
		if (u->ox > 0) u->ox = (int8_t)(u->ox > px ? u->ox - px : 0); else if (u->ox < 0) u->ox = (int8_t)(-u->ox > px ? u->ox + px : 0);
		if (u->oy > 0) u->oy = (int8_t)(u->oy > px ? u->oy - px : 0); else if (u->oy < 0) u->oy = (int8_t)(-u->oy > px ? u->oy + px : 0);
	}
	missile_tick_all();
	w2.cycle++;
}

/* ===================================================================================== drawing */
static uint8_t *g_buf;
#define SW 640
#define SH 480

static void png_chunk(FILE *f, const char *type, const unsigned char *d, unsigned long n)
{
	static unsigned long tab[256]; static int init; unsigned long c = 0xFFFFFFFFul, i; unsigned char b[4];
	if (!init) { unsigned long k, v; int j; for (k = 0; k < 256; k++) { v = k; for (j = 0; j < 8; j++) v = (v & 1) ? 0xEDB88320ul ^ (v >> 1) : v >> 1; tab[k] = v; } init = 1; }
	b[0] = (unsigned char)(n >> 24); b[1] = (unsigned char)(n >> 16); b[2] = (unsigned char)(n >> 8); b[3] = (unsigned char)n;
	fwrite(b, 1, 4, f); fwrite(type, 1, 4, f); if (n) fwrite(d, 1, n, f);
	for (i = 0; i < 4; i++) c = tab[(c ^ (unsigned char)type[i]) & 0xFF] ^ (c >> 8);
	for (i = 0; i < n; i++) c = tab[(c ^ d[i]) & 0xFF] ^ (c >> 8);
	c ^= 0xFFFFFFFFul;
	b[0] = (unsigned char)(c >> 24); b[1] = (unsigned char)(c >> 16); b[2] = (unsigned char)(c >> 8); b[3] = (unsigned char)c;
	fwrite(b, 1, 4, f);
}
static void write_png(const char *name)
{
	char path[512]; FILE *f; const unsigned char *pal = assets_palette();
	unsigned long raw_n = (unsigned long)SH * (SW * 3 + 1), blocks = (raw_n + 65534) / 65535, zn = 2 + raw_n + blocks * 5 + 4, i, p = 0, a = 1, b = 0;
	unsigned char *raw = malloc(raw_n), *z = malloc(zn), ih[13]; int x, y;
	snprintf(path, sizeof path, "%s/%s", g_out, name);
	for (y = 0; y < SH; y++) {
		raw[p++] = 0;
		for (x = 0; x < SW; x++) { int c = g_buf[y * SW + x]; raw[p++] = pal[c * 3]; raw[p++] = pal[c * 3 + 1]; raw[p++] = pal[c * 3 + 2]; }
	}
	p = 0; z[p++] = 0x78; z[p++] = 0x01;
	for (i = 0; i < raw_n; i += 65535) {
		unsigned long n = raw_n - i > 65535 ? 65535 : raw_n - i;
		z[p++] = (unsigned char)(i + n >= raw_n); z[p++] = (unsigned char)n; z[p++] = (unsigned char)(n >> 8);
		z[p++] = (unsigned char)~n; z[p++] = (unsigned char)(~n >> 8);
		memcpy(z + p, raw + i, n); p += n;
	}
	for (i = 0; i < raw_n; i++) { a = (a + raw[i]) % 65521; b = (b + a) % 65521; }
	z[p++] = (unsigned char)(b >> 8); z[p++] = (unsigned char)b; z[p++] = (unsigned char)(a >> 8); z[p++] = (unsigned char)a;
	f = fopen(path, "wb");
	if (f) {
		fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
		ih[0] = 0; ih[1] = 0; ih[2] = SW >> 8; ih[3] = SW & 255; ih[4] = 0; ih[5] = 0; ih[6] = SH >> 8; ih[7] = SH & 255;
		ih[8] = 8; ih[9] = 2; ih[10] = 0; ih[11] = 0; ih[12] = 0;
		png_chunk(f, "IHDR", ih, 13); png_chunk(f, "IDAT", z, p); png_chunk(f, "IEND", NULL, 0);
		fclose(f);
		printf("  shot %s (missiles in flight: %d)\n", path, missile_count());
	}
	free(raw); free(z);
}

static int nearest_col(int r, int g, int b)
{
	const unsigned char *p = assets_palette(); int i, best = 0; long bd = 1L << 30;
	for (i = 1; i < 256; i++) { long d = (long)(p[i*3]-r)*(p[i*3]-r) + (long)(p[i*3+1]-g)*(p[i*3+1]-g) + (long)(p[i*3+2]-b)*(p[i*3+2]-b); if (d < bd) { bd = d; best = i; } }
	return best;
}

static void draw_world(void)
{
	int x, y, s, c_green = nearest_col(0, 220, 0), c_red = nearest_col(230, 0, 0), c_black = nearest_col(0, 0, 0);
	view.buf = g_buf; view.pitch = SW; view.scr_x = 0; view.scr_y = 0; view.scr_w = SW; view.scr_h = SH; view.lores = 0;
	for (y = 0; y < SH; y++) for (x = 0; x < SW; x++) {
		int mx = view.map_x + x, my = view.map_y + y, tx = mx >> 5, ty = my >> 5;
		const unsigned char *t;
		if (tx >= w2.map.w || ty >= w2.map.h) { g_buf[y * SW + x] = 0; continue; }
		t = assets_megatile(w2.map.mega[ty * w2.map.w + tx]);
		g_buf[y * SW + x] = t ? t[(my & 31) * 32 + (mx & 31)] : 0;
	}
	unitdraw_draw();                 /* the real unitdraw.c: corpses, buildings, units */
	/* hp bars (test aid) */
	for (s = 0; s < w2.unit_hi; s++) {
		const W2Unit *u = &w2.unit[s]; const w2_unit_type *t = T(s); int i, bw, bx, by, fill;
		if (!(u->flags & W2_UF_ALIVE) || !t || !t->hp) continue;
		bw = t->tile_w * 32 - 4; bx = u->tx * 32 + u->ox + 2 - view.map_x; by = u->ty * 32 + u->oy - 3 - view.map_y;
		fill = (int)u->hp * bw / t->hp;
		for (i = 0; i < bw; i++) {
			int xx = bx + i;
			if (xx < 0 || xx >= SW || by < 0 || by + 2 >= SH) continue;
			g_buf[by * SW + xx] = (uint8_t)c_black;
			g_buf[(by + 1) * SW + xx] = (uint8_t)(i < fill ? (fill * 2 > bw ? c_green : c_red) : c_black);
		}
	}
	missile_draw_all();
}

static void shot(const char *name, int cx, int cy)
{
	view.map_x = cx - SW / 2; view.map_y = cy - SH / 2;
	if (view.map_x > w2.map.w * 32 - SW) view.map_x = w2.map.w * 32 - SW;
	if (view.map_y > w2.map.h * 32 - SH) view.map_y = w2.map.h * 32 - SH;
	if (view.map_x < 0) view.map_x = 0;
	if (view.map_y < 0) view.map_y = 0;
	draw_world();
	write_png(name);
}

/* ===================================================================================== scenarios */
static void reset_world(uint32_t seed)
{
	int i;
	if (pud_load_builtin(220)) { printf("FATAL: cannot load map 220\n"); exit(2); }
	for (i = w2.unit_hi - 1; i >= 0; i--) if (w2.unit[i].flags) unit_remove(i);   /* terrain only */
	unit_corpses_clear();
	memset(w2.map.visible, 0xFF, sizeof w2.map.visible);    /* no fog module: everybody sees all */
	memset(w2.map.explored, 0xFF, sizeof w2.map.explored);
	w2.unit_hi = 0; w2.cycle = 0; w2.rng = seed;
	w2.player[0].control = W2_CTRL_HUMAN; w2.player[1].control = W2_CTRL_COMPUTER;
	w2.player[0].race = 0; w2.player[1].race = 1;
	for (i = 0; i < 8; i++) { w2.player[i].upgrades = 0; w2.player[i].enemies = (uint16_t)(0x00FF & ~(1u << i)); }
	w2.local_player = 0;
	combat_reset();
	memset(blocked, 0, sizeof blocked);
	g_msgs = g_help_snd = g_dead_snd = g_impact_snd = 0;
}

/* first rectangle of w x h tiles where every tile has the wanted flags and none of the forbidden */
static int find_rect(int w, int h, uint16_t need, uint16_t forbid, int *ox, int *oy)
{
	int x, y, i, j;
	for (y = 2; y + h < w2.map.h - 2; y++)
		for (x = 2; x + w < w2.map.w - 2; x++) {
			int ok = 1;
			for (j = 0; j < h && ok; j++) for (i = 0; i < w && ok; i++) {
				uint16_t f = w2.map.flags[(y + j) * w2.map.w + x + i];
				if ((f & need) != need || (f & forbid)) ok = 0;
			}
			if (ok) { *ox = x; *oy = y; return 1; }
		}
	return 0;
}

static int side_alive(int owner, int *hp)
{
	int s, n = 0; *hp = 0;
	for (s = 0; s < w2.unit_hi; s++) {
		const W2Unit *u = &w2.unit[s];
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & W2_UF_DYING) || u->owner != owner) continue;
		n++; *hp += u->hp;
	}
	return n;
}

typedef struct { int type, owner, dx, dy; } Placement;
typedef struct Result { int cycles, winner, alive0, alive1, hp0, hp1; uint32_t hash; } Result;

static uint32_t world_hash(void)
{
	uint32_t h = 2166136261u; int s;
	for (s = 0; s < w2.unit_hi; s++) {
		const W2Unit *u = &w2.unit[s];
		uint32_t v = (uint32_t)u->flags ^ ((uint32_t)u->hp << 8) ^ ((uint32_t)u->tx << 20) ^ ((uint32_t)u->ty << 26) ^ u->kills;
		h = (h ^ v) * 16777619u;
	}
	return (h ^ w2.rng) * 16777619u;
}

/* mode: 0 = idle auto-acquire, 1 = side 0 attack-moves onto side 1, 2 = everybody attack-moves to the enemy */
static Result run_battle(const char *name, const Placement *pl, int n, int ox, int oy, int mode, int maxc,
                         int verbose, const char *shot_name, int shot_after)
{
	Result r; int i, slots[64], shot_done = 0, seen = 0, e0x = 0, e0y = 0, e1x = 0, e1y = 0, c0 = 0, c1 = 0, last_print = -1;
	memset(&r, 0, sizeof r);
	for (i = 0; i < n; i++) {
		slots[i] = unit_create(pl[i].type, pl[i].owner, ox + pl[i].dx, oy + pl[i].dy);
		if (slots[i] < 0) { printf("FAIL cannot place %s\n", w2_unit(pl[i].type)->name); g_fail++; }
		if (pl[i].owner == 0) { e0x += pl[i].dx; e0y += pl[i].dy; c0++; } else { e1x += pl[i].dx; e1y += pl[i].dy; c1++; }
	}
	if (c0) { e0x /= c0; e0y /= c0; }
	if (c1) { e1x /= c1; e1y /= c1; }
	for (i = 0; i < n; i++) {
		if (slots[i] < 0 || w2_unit(pl[i].type)->kind == W2_KIND_BUILDING) continue;
		if (mode == 1 && pl[i].owner == 0) cmd_attack_move(slots[i], ox + e1x, oy + e1y);
		if (mode == 2) cmd_attack_move(slots[i], ox + (pl[i].owner ? e0x : e1x), oy + (pl[i].owner ? e0y : e1y));
	}
	if (verbose) printf("== %s\n", name);
	for (r.cycles = 0; r.cycles < maxc; r.cycles++) {
		int hp0, hp1, a0 = side_alive(0, &hp0), a1 = side_alive(1, &hp1);
		if (verbose && (r.cycles % 60 == 0) && r.cycles / 60 != last_print) {
			last_print = r.cycles / 60;
			if (r.cycles % 300 == 0) printf("  t=%4ds  side0 %d units %4d hp | side1 %d units %4d hp | missiles %d\n", r.cycles / 30, a0, hp0, a1, hp1, missile_count());
		}
		if (!a0 || !a1) break;
		if (shot_name && !shot_done && r.cycles >= shot_after && missile_count() > 0 && ++seen >= 3) {
			int cx = 0, cy = 0, k = 0, s;
			for (s = 0; s < w2.unit_hi; s++) if (w2.unit[s].flags & W2_UF_ALIVE) { int x, y; unit_center(s, &x, &y); cx += x; cy += y; k++; }
			if (k) shot(shot_name, cx / k, cy / k);
			shot_done = 1;
		}
		game_cycle();
	}
	r.alive0 = side_alive(0, &r.hp0); r.alive1 = side_alive(1, &r.hp1);
	r.winner = r.alive0 && !r.alive1 ? 0 : r.alive1 && !r.alive0 ? 1 : -1;
	/* let deaths and missiles settle */
	for (i = 0; i < 400; i++) game_cycle();
	r.hash = world_hash();
	if (verbose)
		printf("  END %s after %d cycles (%d.%01d s): winner side %d, survivors %d (%d hp) vs %d (%d hp), kills p0=%d p1=%d score p0=%u p1=%u\n",
		       name, r.cycles, r.cycles / 30, (r.cycles % 30) / 3, r.winner, r.alive0, r.hp0, r.alive1, r.hp1,
		       combat_kills(0), combat_kills(1), (unsigned)combat_score(0), (unsigned)combat_score(1));
	return r;
}

/* =========================================================================================== tests */
static int LX, LY, WX, WY;    /* land and water areas */

static void test_damage_formula(void)
{
	int i, mn = 999, mx = 0, sum = 0, b, p;
	printf("== damage formula\n");
	w2.rng = 1;
	for (i = 0; i < 10000; i++) { int d = combat_roll(6, 3, 2); if (d < mn) mn = d; if (d > mx) mx = d; sum += d; }
	printf("  footman vs grunt (6+3 vs armor 2): min %d max %d avg %d.%02d (expect 4..8)\n", mn, mx, sum / 10000, sum % 10000 / 100);
	CHECK(mn == 4 && mx == 8, "footman roll range %d..%d", mn, mx);
	CHECK(combat_roll(6, 3, 30) >= 2 && combat_roll(6, 3, 30) <= 4, "armor above basic leaves piercing");
	CHECK(combat_roll(0, 0, 0) == 0, "no damage");
	/* upgrades: sword 1 + shield 1 for player 0 */
	reset_world(1);
	{
		int f = unit_create(W2U_FOOTMAN, 0, LX, LY), a = unit_create(W2U_ARCHER, 0, LX + 1, LY), r;
		combat_weapon(f, &b, &p);
		CHECK(b == 6 && p == 3 && combat_armor(f) == 2, "base footman %d+%d a%d", b, p, combat_armor(f));
		w2.player[0].upgrades = (1u << W2UP_SWORD1) | (1u << W2UP_HUMAN_SHIELD1) | (1u << W2UP_ARROW1) | (1u << W2UP_LONGBOW);
		combat_weapon(f, &b, &p);
		CHECK(b == 8 && p == 3 && combat_armor(f) == 4, "sword1/shield1 footman %d+%d a%d", b, p, combat_armor(f));
		r = combat_weapon(a, &b, &p);
		CHECK(b == 3 && p == 7 && r == 5, "arrow1+longbow archer %d+%d r%d", b, p, r);
		printf("  upgrades: footman sword1 -> basic 8, shield1 -> armor 4; archer arrow1 -> piercing 7, longbow range 5\n");
	}
}

/* 1v1 footman vs grunt, many seeds: duration distribution and fairness */
static void test_duel_stats(void)
{
	static const Placement pl[] = { { W2U_FOOTMAN, 0, 0, 0 }, { W2U_GRUNT, 1, 3, 0 } };
	int seed, wins0 = 0, tot = 0, mn = 1 << 30, mx = 0;
	printf("== footman vs grunt duel x100 seeds\n");
	for (seed = 1; seed <= 100; seed++) {
		Result r;
		reset_world((uint32_t)seed * 7919u);
		r = run_battle("duel", pl, 2, LX, LY, 0, 3000, 0, NULL, 0);
		if (r.winner == 0) wins0++;
		tot += r.cycles; if (r.cycles < mn) mn = r.cycles; if (r.cycles > mx) mx = r.cycles;
		CHECK(r.winner >= 0, "duel seed %d no winner", seed);
	}
	printf("  footman wins %d/100, fight length avg %d cycles (%d.%d s) min %d max %d\n", wins0, tot / 100, tot / 100 / 30, tot / 100 % 30 / 3, mn, mx);
	printf("  expected: 60 hp / avg 6 per hit = ~10 hits x 25-cycle swing ~ 250 cycles + approach\n");
	CHECK(wins0 > 25 && wins0 < 75, "duel fairness %d", wins0);
	CHECK(tot / 100 > 180 && tot / 100 < 420, "duel length %d", tot / 100);
}

static void test_skirmishes(void)
{
	static const Placement melee[] = {
		{ W2U_FOOTMAN, 0, 0, 0 }, { W2U_FOOTMAN, 0, 0, 1 }, { W2U_FOOTMAN, 0, 0, 2 }, { W2U_FOOTMAN, 0, 0, 3 },
		{ W2U_GRUNT, 1, 6, 0 }, { W2U_GRUNT, 1, 6, 1 }, { W2U_GRUNT, 1, 6, 2 }, { W2U_GRUNT, 1, 6, 3 } };
	static const Placement ranged[] = {
		{ W2U_ARCHER, 0, 0, 0 }, { W2U_ARCHER, 0, 0, 2 }, { W2U_ARCHER, 0, 0, 4 },
		{ W2U_AXETHROWER, 1, 7, 0 }, { W2U_AXETHROWER, 1, 7, 2 }, { W2U_AXETHROWER, 1, 7, 4 } };
	static const Placement knights[] = {
		{ W2U_KNIGHT, 0, 0, 0 }, { W2U_KNIGHT, 0, 0, 2 }, { W2U_OGRE, 1, 5, 0 }, { W2U_OGRE, 1, 5, 2 } };
	static const Placement mixed[] = {
		{ W2U_KNIGHT, 0, 0, 1 }, { W2U_FOOTMAN, 0, 0, 3 }, { W2U_ARCHER, 0, -2, 2 },
		{ W2U_GRUNT, 1, 8, 1 }, { W2U_GRUNT, 1, 8, 3 }, { W2U_AXETHROWER, 1, 10, 2 } };
	static const Placement siege[] = { { W2U_CATAPULT, 0, 0, 1 }, { W2U_ORC_BARRACKS, 1, 6, 0 } };
	static const Placement tower[] = {
		{ W2U_FOOTMAN, 0, 0, 0 }, { W2U_FOOTMAN, 0, 0, 1 }, { W2U_FOOTMAN, 0, 0, 2 }, { W2U_ORC_GUARD_TOWER, 1, 10, 1 } };
	static const Placement ctower[] = {
		{ W2U_KNIGHT, 0, 0, 0 }, { W2U_KNIGHT, 0, 0, 2 }, { W2U_ORC_CANNON_TOWER, 1, 9, 1 } };
	static const Placement ships[] = {
		{ W2U_HUMAN_DESTROYER, 0, 0, 0 }, { W2U_HUMAN_DESTROYER, 0, 0, 2 }, { W2U_ORC_DESTROYER, 1, 6, 0 }, { W2U_ORC_DESTROYER, 1, 6, 2 } };
	static const Placement bship[] = { { W2U_BATTLESHIP, 0, 0, 1 }, { W2U_ORC_DESTROYER, 1, 7, 0 }, { W2U_ORC_DESTROYER, 1, 7, 2 } };
	static const Placement air[] = { { W2U_GRYPHON_RIDER, 0, 0, 0 }, { W2U_DRAGON, 1, 6, 0 }, { W2U_ARCHER, 0, 0, 3 } };
	Result r;

	reset_world(11); r = run_battle("4 footmen vs 4 grunts (attack-move both)", melee, 8, LX, LY, 2, 6000, 1, "melee.png", 200);
	CHECK(!r.alive0 || !r.alive1, "melee: fight did not end");
	reset_world(12); r = run_battle("3 archers vs 3 axethrowers (auto)", ranged, 6, LX, LY, 2, 6000, 1, "ranged.png", 100);
	CHECK(!r.alive0 || !r.alive1, "ranged: fight did not end");
	reset_world(13); r = run_battle("2 knights vs 2 ogres", knights, 4, LX, LY, 2, 6000, 1, NULL, 0);
	CHECK(!r.alive0 || !r.alive1, "knights: fight did not end");
	reset_world(14); r = run_battle("mixed 3v3", mixed, 6, LX + 2, LY, 2, 6000, 1, "mixed.png", 150);
	CHECK(!r.alive0 || !r.alive1, "mixed: fight did not end");
	reset_world(15); r = run_battle("catapult vs orc barracks (cmd_attack)", siege, 2, LX, LY, 0, 1, 0, NULL, 0);
	{
		int cat = -1, bar = -1, s;
		for (s = 0; s < w2.unit_hi; s++) if (w2.unit[s].flags & W2_UF_ALIVE) { if (w2.unit[s].type == W2U_CATAPULT) cat = s; else if (w2.unit[s].type == W2U_ORC_BARRACKS) bar = s; }
		/* run_battle already settled 400 cycles idle: the catapult auto-acquired it (react 9) */
		printf("== catapult vs orc barracks: barracks hp after idle start %d/%d, burn level %d\n", bar >= 0 ? w2.unit[bar].hp : 0, 800, bar >= 0 ? combat_burn_level(bar) : -1);
		if (cat >= 0 && bar >= 0) {
			int c, shot_done = 0, seen = 0;
			cmd_attack(cat, bar);
			for (c = 0; c < 9000 && (w2.unit[bar].flags & W2_UF_ALIVE) && !(w2.unit[bar].flags & W2_UF_DYING); c++) {
				if (!shot_done && missile_count() > 0 && c > 300 && ++seen >= 12) { int x, y; unit_center(bar, &x, &y); shot("catapult.png", x - 96, y); shot_done = 1; }
				if (c % 600 == 0) printf("  t=%4ds barracks hp %4d burn %d\n", c / 30, w2.unit[bar].hp, combat_burn_level(bar));
				game_cycle();
			}
			printf("  barracks destroyed after %d more cycles (%d s); expected ~ 800 hp / 45 avg = ~18 shots x 200 cycles\n", c, c / 30);
			CHECK(!(w2.unit[bar].flags & W2_UF_ALIVE) || (w2.unit[bar].flags & W2_UF_DYING), "barracks survived");
			CHECK(combat_score(0) == w2_unit(W2U_ORC_BARRACKS)->points, "score %u", (unsigned)combat_score(0));
		}
	}
	reset_world(16); r = run_battle("guard tower vs 3 footmen (attack-move)", tower, 4, LX, LY, 1, 6000, 1, "tower.png", 30);
	CHECK(!r.alive0 || !r.alive1, "tower: fight did not end");
	reset_world(17); r = run_battle("cannon tower vs 2 knights (attack-move)", ctower, 3, LX, LY, 1, 6000, 1, "cannontower.png", 30);
	CHECK(!r.alive0 || !r.alive1, "cannon tower: fight did not end");
	reset_world(18); r = run_battle("2 elven vs 2 troll destroyers", ships, 4, WX, WY, 2, 6000, 1, "ships.png", 60);
	CHECK(!r.alive0 || !r.alive1, "ships: fight did not end");
	reset_world(19); r = run_battle("battleship vs 2 troll destroyers", bship, 3, WX, WY, 2, 6000, 1, "battleship.png", 60);
	CHECK(!r.alive0 || !r.alive1, "battleship: fight did not end");
	reset_world(20); r = run_battle("gryphon + archer vs dragon", air, 3, LX, LY, 2, 6000, 1, "air.png", 60);
	CHECK(!r.alive0 || !r.alive1, "air: fight did not end");
}

static void test_determinism(void)
{
	static const Placement pl[] = {
		{ W2U_FOOTMAN, 0, 0, 0 }, { W2U_ARCHER, 0, 0, 1 }, { W2U_KNIGHT, 0, 0, 2 }, { W2U_CATAPULT, 0, -2, 1 },
		{ W2U_GRUNT, 1, 8, 0 }, { W2U_AXETHROWER, 1, 8, 1 }, { W2U_OGRE, 1, 8, 2 }, { W2U_CATAPULT, 1, 10, 1 } };
	Result a, b, c;
	printf("== determinism\n");
	reset_world(4242); a = run_battle("det A", pl, 8, LX + 2, LY, 2, 8000, 0, NULL, 0);
	reset_world(4242); b = run_battle("det B", pl, 8, LX + 2, LY, 2, 8000, 0, NULL, 0);
	reset_world(999);  c = run_battle("det C", pl, 8, LX + 2, LY, 2, 8000, 0, NULL, 0);
	printf("  seed 4242: %d cycles hash %08x / %d cycles hash %08x; seed 999: %d cycles hash %08x\n",
	       a.cycles, (unsigned)a.hash, b.cycles, (unsigned)b.hash, c.cycles, (unsigned)c.hash);
	CHECK(a.hash == b.hash && a.cycles == b.cycles, "same seed differs");
	CHECK(a.hash != c.hash, "different seeds gave the same world");
}

static void test_orders(void)
{
	int f, g, p, a, m, s, x, y, c, moved;
	printf("== orders / commands\n");
	/* stand ground: footman holds while an archer shoots it */
	reset_world(5);
	f = unit_create(W2U_FOOTMAN, 0, LX, LY); a = unit_create(W2U_AXETHROWER, 1, LX + 3, LY);
	cmd_stand(f); cmd_stand(a);
	x = w2.unit[f].tx; y = w2.unit[f].ty;
	for (c = 0; c < 600; c++) game_cycle();
	CHECK(w2.unit[f].tx == x && w2.unit[f].ty == y, "stand ground footman moved");
	CHECK(w2.unit[f].hp < 60, "axethrower in range did not shoot the footman (hp %d)", w2.unit[f].hp);
	printf("  stand: footman stayed at %d,%d, hp %d after 20 s of axes; ui messages %d, help sounds %d (rate-limited)\n", x, y, w2.unit[f].hp, g_msgs, g_help_snd);
	CHECK(g_msgs >= 1 && g_msgs <= 3, "under attack messages %d", g_msgs);

	/* idle retaliation of a human unit outside react range: footman attacked by a far axe fights back */
	reset_world(6);
	f = unit_create(W2U_FOOTMAN, 0, LX, LY); a = unit_create(W2U_AXETHROWER, 1, LX + 4, LY);
	cmd_attack(a, f);
	for (c = 0; c < 900 && (w2.unit[a].flags & W2_UF_ALIVE) && (w2.unit[f].flags & W2_UF_ALIVE); c++) game_cycle();
	printf("  retaliation: footman %s, axethrower %s after %d cycles\n", (w2.unit[f].flags & W2_UF_ALIVE) ? "alive" : "dead", (w2.unit[a].flags & W2_UF_ALIVE) ? "alive" : "dead", c);
	CHECK(w2.unit[f].order == W2_ORD_ATTACK || !(w2.unit[a].flags & W2_UF_ALIVE) || !(w2.unit[f].flags & W2_UF_ALIVE) || (w2.unit[f].flags & W2_UF_DYING), "no retaliation");

	/* patrol: back and forth, engage, then resume patrol */
	reset_world(7);
	p = unit_create(W2U_FOOTMAN, 0, LX, LY);
	cmd_patrol(p, LX + 6, LY);
	moved = 0;
	for (c = 0; c < 600; c++) { game_cycle(); if (w2.unit[p].tx == LX + 6) moved |= 1; if (moved & 1 && w2.unit[p].tx == LX) moved |= 2; }
	CHECK(moved == 3 && w2.unit[p].order == W2_ORD_PATROL, "patrol ends not reached (%d) order %d", moved, w2.unit[p].order);
	g = unit_create(W2U_PEON, 1, LX + 3, LY + 2);
	for (c = 0; c < 900 && (w2.unit[g].flags & W2_UF_ALIVE); c++) game_cycle();
	for (c = 0; c < 60; c++) game_cycle();
	printf("  patrol: reached both ends, killed a peon on the way: %s, back to order %d (PATROL=%d)\n", (w2.unit[g].flags & W2_UF_ALIVE) ? "no" : "yes", w2.unit[p].order, W2_ORD_PATROL);
	CHECK(!(w2.unit[g].flags & W2_UF_ALIVE) && w2.unit[p].order == W2_ORD_PATROL, "patrol engage/resume");

	/* attack-move: kill on the way, then arrive */
	reset_world(8);
	m = unit_create(W2U_KNIGHT, 0, LX, LY); g = unit_create(W2U_PEON, 1, LX + 4, LY + 1);
	cmd_attack_move(m, LX + 9, LY);
	for (c = 0; c < 1200 && w2.unit[m].order != W2_ORD_STILL; c++) game_cycle();
	printf("  attack-move: peon %s, knight at %d,%d (goal %d,%d) after %d cycles\n", (w2.unit[g].flags & W2_UF_ALIVE) ? "alive" : "killed", w2.unit[m].tx, w2.unit[m].ty, LX + 9, LY, c);
	CHECK(!(w2.unit[g].flags & W2_UF_ALIVE) && w2.unit[m].tx == LX + 9 && w2.unit[m].ty == LY, "attack-move");

	/* attack ground with a catapult: grunts around the point take splash */
	reset_world(9);
	s = unit_create(W2U_CATAPULT, 0, LX, LY + 1);
	g = unit_create(W2U_GRUNT, 1, LX + 6, LY + 1); f = unit_create(W2U_GRUNT, 1, LX + 6, LY + 2);
	w2.player[1].control = W2_CTRL_NOBODY;
	cmd_stand(g); cmd_stand(f);
	cmd_attack_ground(s, LX + 6, LY + 1);
	for (c = 0; c < 450; c++) game_cycle();
	printf("  attack ground: grunt on the point hp %d, neighbour hp %d (splash /4); order %d\n", w2.unit[g].hp, w2.unit[f].hp, w2.unit[s].order);
	CHECK(w2.unit[s].order == W2_ORD_ATTACK_GROUND || !(w2.unit[g].flags & W2_UF_ALIVE), "attack ground order lost");
	CHECK(!(w2.unit[g].flags & W2_UF_ALIVE) || w2.unit[g].hp < 60, "no ground hit");
	CHECK(!(w2.unit[f].flags & W2_UF_ALIVE) || w2.unit[f].hp < 60, "no splash");

	/* cannon tower min range: an adjacent peon is never hit */
	reset_world(10);
	s = unit_create(W2U_ORC_CANNON_TOWER, 1, LX + 2, LY);
	f = unit_create(W2U_PEASANT, 0, LX + 1, LY);
	cmd_stand(f);
	for (c = 0; c < 600; c++) game_cycle();
	CHECK(w2.unit[f].hp == 30, "cannon tower hit an adjacent unit (hp %d)", w2.unit[f].hp);
	printf("  cannon tower: adjacent peasant untouched (hp %d)\n", w2.unit[f].hp);

	/* smart (right-click) */
	reset_world(3);
	p = unit_create(W2U_PEASANT, 0, LX, LY); f = unit_create(W2U_FOOTMAN, 0, LX + 1, LY);
	g = unit_create(W2U_GRUNT, 1, LX + 6, LY + 4); m = unit_create(W2U_GOLD_MINE, 15, LX + 3, LY + 3);
	cmd_smart(p, LX + 3, LY + 3, m); CHECK(w2.unit[p].order == W2_ORD_HARVEST && w2.unit[p].target == m, "smart mine");
	cmd_smart(p, LX + 6, LY + 4, g); CHECK(w2.unit[p].order == W2_ORD_ATTACK, "smart peasant enemy");
	cmd_smart(f, LX + 6, LY + 4, g); CHECK(w2.unit[f].order == W2_ORD_ATTACK && w2.unit[f].target == g, "smart attack");
	cmd_smart(f, LX, LY, p);         CHECK(w2.unit[f].order == W2_ORD_FOLLOW, "smart follow");
	cmd_smart(f, LX + 2, LY + 2, -1); CHECK(w2.unit[f].order == W2_ORD_MOVE, "smart move");
	{
		int tx, ty, found = 0;
		for (ty = 0; ty < w2.map.h && !found; ty++) for (tx = 0; tx < w2.map.w && !found; tx++)
			if (w2.map.flags[ty * w2.map.w + tx] & W2_TF_TREE) { cmd_smart(p, tx, ty, W2_NO_UNIT); found = 1; }
		CHECK(found && w2.unit[p].order == W2_ORD_HARVEST && w2.unit[p].target == W2_NO_UNIT, "smart tree");
	}
	cmd_stop(f); CHECK(w2.unit[f].order == W2_ORD_STILL, "stop");
	printf("  smart: mine->harvest, enemy->attack, own unit->follow, ground->move, tree->harvest, stop ok\n");
}

int main(int argc, char **argv)
{
	char dir[120];
	(void)argc;
	if (argc < 2) { printf("usage: test_combat <data dir> [out dir]\n"); return 2; }
	if (argc > 2) g_out = argv[2];
	w2cfg_defaults(&w2_cfg);
	w2_cfg.resolution = W2_RES_HIRES;
	snprintf(dir, sizeof dir, "%s", argv[1]);
	snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", dir);
	if (assets_init()) { printf("FATAL: assets_init\n"); return 2; }
	g_buf = malloc(SW * SH);
	reset_world(1);
	if (!find_rect(16, 8, W2_TF_LAND, W2_TF_NOWALK | W2_TF_WATER, &LX, &LY)) { printf("FATAL: no land area\n"); return 2; }
	if (!find_rect(12, 6, W2_TF_WATER, W2_TF_COAST, &WX, &WY)) { printf("FATAL: no water area\n"); return 2; }
	LX += 2; LY += 1; WX += 2; WY += 1;
	printf("map 220: %dx%d era %d, land area at %d,%d, water area at %d,%d\n", w2.map.w, w2.map.h, w2.map.era, LX, LY, WX, WY);

	test_damage_formula();
	test_duel_stats();
	test_skirmishes();
	test_orders();
	test_determinism();

	free(g_buf);
	assets_shutdown();
	if (g_fail) printf("TEST_COMBAT: %d FAILURES\n", g_fail);
	else printf("TEST_COMBAT: ALL OK\n");
	return g_fail ? 1 : 0;
}
