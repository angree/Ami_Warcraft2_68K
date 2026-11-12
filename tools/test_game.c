/* test_game.c - host test of game.c + mapdraw.c (terrain, fog, minimap, view, game shell).
 * Built by tools/build_test_game.sh with the host platform layer (native/host_gfx/audio/sys.c), every src .c file except
 * main_amiga.c / setup_amiga.c, and the weak stubs of the other modules (w2_stubs.c).
 *
 * Env: W2_TEST_DATA  game data dir ("War Data" ...), W2_TEST_PUD  a custom PUD file,
 *      W2_TEST_LORES 1 = 320x240, W2_SHOTDIR where the PNGs go.
 * Units: unit_create is a stub (-1), so test units are written straight into w2.unit[]. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "w2.h"
#include "pud.h"
#include "assets.h"
#include "w2cfg.h"
#include "game.h"
#include "mapdraw.h"

int host_shot(const char *name);

void w2_log(const char *fmt, ...)
{
	va_list a;
	va_start(a, fmt);
	vfprintf(stderr, fmt, a);
	va_end(a);
	fputc('\n', stderr);
}
unsigned long w2_tick_ms(void) { return 50; }
unsigned long w2_frame_count(void) { return 0; }

static int g_fail;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "CHECK FAILED %s:%d: %s\n", __FILE__, __LINE__, #c); g_fail++; } } while (0)

static int add_unit(int type, int owner, int tx, int ty)
{
	int s = w2.unit_hi;
	const w2_unit_type *t = w2_unit(type);
	W2Unit *u;
	if (s >= W2_MAX_UNITS || !t) return -1;
	u = &w2.unit[s];
	memset(u, 0, sizeof *u);
	u->flags = W2_UF_ALIVE | ((t->flags & W2UF_BUILDING) ? W2_UF_BUILDING : 0);
	u->type = (uint8_t)type; u->owner = (uint8_t)owner; u->tx = (uint8_t)tx; u->ty = (uint8_t)ty;
	u->hp = t->hp; u->target = W2_NO_UNIT;
	w2.unit_hi = (uint16_t)(s + 1);
	return s;
}

#ifndef HAVE_UNIT_C
/* minimal unit_create / unit_remove while src/unit.c does not exist, so the PUD's own units load */
int unit_create(int type, int owner, int tx, int ty) { return add_unit(type, owner, tx, ty); }
void unit_remove(int slot) { if (slot >= 0 && slot < W2_MAX_UNITS) w2.unit[slot].flags = 0; }
#endif

static void frame(const char *shot)
{
	if (game_draw(amigagfx_chunky(), amigagfx_pitch(), amigagfx_game_width(), amigagfx_game_height()))
		amigagfx_blit(0, 0, amigagfx_game_width(), amigagfx_game_height());
	if (shot) host_shot(shot);
}

static int count_bits(const uint8_t *a, int n, int bit)
{
	int i, c = 0;
	for (i = 0; i < n; i++) if (a[i] & bit) c++;
	return c;
}

/* start a map, put a town + a walking scout for the local player and an enemy base, run 100 cycles */
static void run_map(int builtin, const char *path, const char *tag)
{
	W2Setup s;
	int lp, sx, sy, scout, i, enemy = -1, p, ex = 0, ey = 0;
	char name[64];
	memset(&s, 0, sizeof s);
	s.builtin_pud = builtin; s.local_player = -1; s.local_race = -1;
	if (path) snprintf(s.pud_path, sizeof s.pud_path, "%s", path);
	if (game_start(&s)) { fprintf(stderr, "game_start %s failed\n", tag); g_fail++; return; }
	CHECK(game_state == GS_PLAY);
	lp = w2.local_player;
	sx = w2.player[lp].start_tx != 0xFFFF ? w2.player[lp].start_tx : w2.map.w / 2;
	sy = w2.player[lp].start_ty != 0xFFFF ? w2.player[lp].start_ty : w2.map.h / 2;
	for (p = 0; p < 8; p++)
		if (p != lp && w2.player[p].start_tx != 0xFFFF) { enemy = p; ex = w2.player[p].start_tx; ey = w2.player[p].start_ty; break; }
	fprintf(stderr, "[%s] %dx%d era %d local %d start %d,%d enemy %d at %d,%d\n", tag, w2.map.w, w2.map.h, w2.map.era, lp, sx, sy, enemy, ex, ey);

	/* test units (stub unit_create) */
	add_unit(w2.player[lp].race ? W2U_GREAT_HALL : W2U_TOWN_HALL, lp, sx, sy);
	for (i = 0; i < 3; i++) add_unit(w2.player[lp].race ? W2U_PEON : W2U_PEASANT, lp, sx + i, sy + 4 < w2.map.h ? sy + 4 : sy);
	scout = add_unit(w2.player[lp].race ? W2U_PEON : W2U_PEASANT, lp, sx, sy);
	if (enemy >= 0) {
		add_unit(w2.player[enemy].race ? W2U_GREAT_HALL : W2U_TOWN_HALL, enemy, ex, ey);
		add_unit(w2.player[enemy].race ? W2U_PEON : W2U_PEASANT, enemy, ex + 4 < w2.map.w ? ex + 4 : ex, ey);
	}
	/* a gold mine near the start, neutral */
	add_unit(W2U_GOLD_MINE, W2_NEUTRAL, sx + 6 < w2.map.w - 3 ? sx + 6 : sx - 6 > 0 ? sx - 6 : 0, sy);
	fog_update();
	CHECK(fog_visible(sx, sy));
	CHECK(fog_explored(sx, sy));
	CHECK(!fog_visible(sx + 30 < w2.map.w ? sx + 30 : sx - 30, sy) || mapdraw_reveal);

	/* 100 cycles; the scout walks toward the map centre, one tile every 4 cycles */
	for (i = 0; i < 100; i++) {
		W2Unit *u = &w2.unit[scout];
		if ((i & 3) == 0) {
			int gx = w2.map.w / 2, gy = w2.map.h / 2;
			if (u->tx < gx) u->tx++; else if (u->tx > gx) u->tx--;
			if (u->ty < gy) u->ty++; else if (u->ty > gy) u->ty--;
		}
		game_tick();
	}
	CHECK(w2.cycle == 100);
	CHECK(game_state == GS_PLAY);
	fprintf(stderr, "[%s] after 100 cycles: visible %d explored %d tiles (local), scout at %d,%d\n", tag,
	        count_bits(w2.map.visible, w2.map.w * w2.map.h, 1 << lp), count_bits(w2.map.explored, w2.map.w * w2.map.h, 1 << lp),
	        w2.unit[scout].tx, w2.unit[scout].ty);

	/* view between start and scout */
	view_center_on(((sx + w2.unit[scout].tx) / 2) * 32 + 16, ((sy + w2.unit[scout].ty) / 2) * 32 + 16);
	snprintf(name, sizeof name, "%s_%s", tag, assets_lores() ? "lores" : "hires");
	frame(name);

	/* view_to_map / minimap_to_map consistency */
	{
		int px, py, tx, ty;
		CHECK(view_to_map(GAME_VIEW_X + 10, GAME_VIEW_Y + 20, &px, &py) == 1);
		CHECK(px == view.map_x + 10 && py == view.map_y + 20);
		CHECK(view_to_map(10, 10, &px, &py) == 0);
		CHECK(minimap_to_map(GAME_MINIMAP_X + 64, GAME_MINIMAP_Y + 64, &tx, &ty) == 1);
		CHECK(tx >= w2.map.w / 2 - 2 && tx <= w2.map.w / 2 + 1);
		CHECK(minimap_to_map(GAME_MINIMAP_X - 1, GAME_MINIMAP_Y, &tx, &ty) == 0);
	}

	/* shaded fog variant */
	mapdraw_fog_mode = MAPDRAW_FOG_SHADE;
	snprintf(name, sizeof name, "%s_%s_shade", tag, assets_lores() ? "lores" : "hires");
	frame(name);
	mapdraw_fog_mode = MAPDRAW_FOG_DITHER;

	/* everything revealed, scrolled into the far corner (clamped) */
	mapdraw_reveal = 1;
	view_scroll(100000, 100000);
	CHECK(view.map_x == w2.map.w * 32 - GAME_VIEW_W && view.map_y == w2.map.h * 32 - GAME_VIEW_H);
	snprintf(name, sizeof name, "%s_%s_reveal_corner", tag, assets_lores() ? "lores" : "hires");
	frame(name);
	view_scroll(-100000, -100000);
	CHECK(view.map_x == 0 && view.map_y == 0);
	/* an odd scroll position (partial tiles, unaligned copies) */
	view_scroll(37, 13);
	snprintf(name, sizeof name, "%s_%s_reveal_odd", tag, assets_lores() ? "lores" : "hires");
	frame(name);
	mapdraw_reveal = 0;
}

/* victory: every enemy unit removed -> game_over 1 -> end screen after the delay;
 * defeat: every local unit removed -> game_over 2 */
static void run_end(int want)
{
	W2Setup s;
	int i, k, lp;
	memset(&s, 0, sizeof s);
	s.builtin_pud = 220; s.local_player = -1; s.local_race = -1;
	if (game_start(&s)) { g_fail++; return; }
	lp = w2.local_player;
	CHECK(game_player_alive(lp));
	for (i = 0; i < w2.unit_hi; i++) {
		int o = w2.unit[i].owner;
		if (o >= 8) continue;
		if ((want == 1 && o != lp) || (want == 2 && o == lp)) unit_remove(i);
	}
	for (k = 0; k < 200 && game_state == GS_PLAY; k++) game_tick();
	fprintf(stderr, "[end] want %d: game_over %d state %d after %d cycles\n", want, w2.game_over, game_state, k);
	CHECK(w2.game_over == want);
	CHECK(game_state == GS_END);
}

int main(void)
{
	const char *data = getenv("W2_TEST_DATA"), *pud = getenv("W2_TEST_PUD");
	int lores = getenv("W2_TEST_LORES") && atoi(getenv("W2_TEST_LORES"));
	int w = lores ? 320 : 640, h = lores ? 240 : 480;

	w2cfg_defaults(&w2_cfg);
	w2_cfg.resolution = lores ? W2_RES_LORES : W2_RES_HIRES;
	if (data) snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", data);
	if (amigagfx_open(w, h, 0, AMIGAGFX_BACKEND_RTG)) { fprintf(stderr, "screen open failed\n"); return 2; }
	if (game_init()) { fprintf(stderr, "game_init failed\n"); return 2; }
	CHECK(game_state == GS_MENU);

	run_map(220, NULL, "map220");
	run_map(230, NULL, "map230");
	if (pud) run_map(0, pud, "custom");

	/* arrow-key scrolling through game_event + game_tick */
	{
		AmigaGfxEvent ev;
		int x0;
		view_center_on(w2.map.w * 16, w2.map.h * 16);
		x0 = view.map_x;
		memset(&ev, 0, sizeof ev);
		ev.type = AMIGAGFX_EV_KEY; ev.code = 0x4E;     /* right down */
		CHECK(game_event(&ev) == 1);
		game_tick(); game_tick();
		ev.code = 0x4E | 0x80;
		game_event(&ev);
		game_tick();
		CHECK(view.map_x == x0 + 2 * w2_cfg.scroll_speed * 4);
	}
	/* end -> menu (stub) */
	game_end(3);
	CHECK(game_state == GS_MENU);
#ifndef HAVE_UNIT_C
	run_end(1);
	run_end(2);
	/* "one peasant only": a single worker per playing slot */
	{
		W2Setup s; int i, n = 0, other = 0;
		memset(&s, 0, sizeof s);
		s.builtin_pud = 220; s.local_player = -1; s.local_race = 1; s.units = 1; s.resources = 3;
		CHECK(game_start(&s) == 0);
		for (i = 0; i < w2.unit_hi; i++) {
			if (!(w2.unit[i].flags & W2_UF_ALIVE) || w2.unit[i].owner >= 8) continue;
			if (w2.unit[i].owner == w2.local_player) { n++; CHECK(w2.unit[i].type == W2U_PEON); } else other++;
		}
		fprintf(stderr, "[peasant] local units %d, other players' units %d, gold %ld\n", n, other, (long)w2.player[w2.local_player].gold);
		CHECK(n == 1);
		CHECK(w2.player[w2.local_player].gold == 10000);
		game_end(3);
	}
#endif
	game_shutdown();
	amigagfx_close();
	fprintf(stderr, "test_game: %s (%d failures)\n", g_fail ? "FAILED" : "OK", g_fail);
	return g_fail ? 1 : 0;
}
