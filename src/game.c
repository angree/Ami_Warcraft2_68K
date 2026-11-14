/* game.c - the game shell: the world (w2), the view, the platform hooks of w2cfg.h, the state machine
 * menu <-> play <-> end screen, skirmish setup, the logic-cycle driver, scrolling input.
 *
 * Layout (HIRES, wargus ui MapArea / Minimap): left panel 0..175 (minimap 24,26 128x128), top bar
 * 0..15, viewport 176,16 448x448, right border 624..639, bottom bar 464..479. Lores = the same halved.
 * Plain C, no floats, no sprintf. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w2.h"
#include "pud.h"
#include "assets.h"
#include "w2cfg.h"
#include "game.h"
#include "mapdraw.h"
#include "move.h"
#include "economy.h"
#include "menu.h"
#include "combat.h"
#include "amiga_uclock.h"
#include "ui.h"

W2World w2;
W2View view;
int game_state = GS_MENU;
int w2_think_period = 1;

/* Logic follows the original (user decision 2026-11-14, measured 0.3.x): end check every cycle, every
 * unit decides every cycle (no logic skip), path search budget 5000. Kept simplifications: fog of war
 * rebuilt every 4 cycles (every cycle costs 11-15 % fps on a 68020) and at most 3 NEW path searches per
 * cycle (unlimited gave 1.6-3x more stalls when an army starts moving).  Measuring switches (w2.cfg):
 * faithful=1 fog every cycle + unlimited searches (the full original), 2 fog only, 3 unlimited searches only. */
int w2_faithful;          /* fog every cycle */
int w2_dirty = 1;         /* dirty rectangles (w2.cfg dirty=0: draw and convert everything every frame) */

static void apply_mode(void)
{
	int f = w2cfg_extra_int("faithful", 0);
	w2_dirty = w2cfg_extra_int("dirty", 1) != 0;
	w2_faithful = (f == 1 || f == 2);
	w2_think_period = 1;
	path_node_budget = 5000;
	move_search_quota = (f == 1 || f == 3) ? 10000 : 3;
	w2_log("game: logic mode %d (fog every %d, path budget %d, searches/cycle %d)", f, w2_faithful ? 1 : 4,
	       path_node_budget, move_search_quota);
}
int game_mouse_x, game_mouse_y;
int game_draws_minimap = 1;
unsigned long game_prof_us[5];   /* draw sections (profile=1): map+fog, units, missiles, minimap, ui */

static int g_quit;
static int g_menu_was_play;      /* the last frame drawn was the game: the menu must repaint all */
static int g_mouse_seen;
static int g_keys;                 /* arrow keys held: 1 up 2 down 4 left 8 right */
static int g_clear;                /* frames that still clear the whole buffer (entering play) */
static uint8_t g_present[8];       /* player had units/buildings at the start */
static int g_end_wait;
static int g_full_frame;            /* this frame must be converted whole */
static int g_was_menu = 1;          /* the previous frame was not the game */             /* cycles from game over to the end screen */

#define RAW_UP    0x4C
#define RAW_DOWN  0x4D
#define RAW_RIGHT 0x4E
#define RAW_LEFT  0x4F

/* palette entry per era in War Data (DATA_FORMAT.md: summer 2, winter 18, wasteland 10) */
static const unsigned char k_era_pal[3] = { 2, 18, 10 };

void game_quit(void) { g_quit = 1; }

/* ---------------------------------------------------------------- setup */
static int count_units(int player, int *buildings)
{
	int s, n = 0, b = 0;
	for (s = 0; s < w2.unit_hi; s++) {
		const W2Unit *u = &w2.unit[s];
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & W2_UF_DYING) || u->owner != player) continue;
		n++;
		if (u->flags & W2_UF_BUILDING) b++;
	}
	if (buildings) *buildings = b;
	return n;
}

int game_player_alive(int player)
{
	return player >= 0 && player < 8 && g_present[player] && count_units(player, NULL) > 0;
}

static void remove_units_of(int player)
{
	int s;
	for (s = 0; s < w2.unit_hi; s++)
		if ((w2.unit[s].flags & W2_UF_ALIVE) && w2.unit[s].owner == player) unit_remove(s);
}

/* the other race's equivalent of every unit of the player (map units of a race-swapped slot) */
static void convert_race(int player, int race)
{
	int s;
	for (s = 0; s < w2.unit_hi; s++) {
		W2Unit *u = &w2.unit[s];
		const w2_unit_type *t;
		if (!(u->flags & W2_UF_ALIVE) || u->owner != player) continue;
		t = w2_unit(u->type);
		if (t && t->race != race && t->equiv != W2_NONE && w2_unit(t->equiv)) u->type = t->equiv;
	}
}

static void start_point(int p, int *tx, int *ty)
{
	int s;
	*tx = -1; *ty = -1;
	if (w2.player[p].start_tx != 0xFFFF) { *tx = w2.player[p].start_tx; *ty = w2.player[p].start_ty; return; }
	for (s = 0; s < w2.unit_hi; s++)
		if ((w2.unit[s].flags & W2_UF_ALIVE) && w2.unit[s].owner == p) { *tx = w2.unit[s].tx; *ty = w2.unit[s].ty; return; }
}

static void center_on_local(void)
{
	int p = w2.local_player, s, n = 0;
	long sx = 0, sy = 0;
	if (p < 8 && w2.player[p].start_tx != 0xFFFF) {
		view_center_on(w2.player[p].start_tx * W2_TILE + W2_TILE / 2, w2.player[p].start_ty * W2_TILE + W2_TILE / 2);
		return;
	}
	for (s = 0; s < w2.unit_hi; s++)
		if ((w2.unit[s].flags & W2_UF_ALIVE) && w2.unit[s].owner == p) { sx += w2.unit[s].tx; sy += w2.unit[s].ty; n++; }
	if (n) view_center_on((int)(sx / n) * W2_TILE + W2_TILE / 2, (int)(sy / n) * W2_TILE + W2_TILE / 2);
	else view_center_on(w2.map.w * W2_TILE / 2, w2.map.h * W2_TILE / 2);
}

int game_start(const W2Setup *s)
{
	static const int32_t res_tab[4][3] = { { 0, 0, 0 }, { 2000, 1000, 1000 }, { 5000, 2000, 2000 }, { 10000, 5000, 5000 } };
	int p, local = -1, kept = 0, r;
	W2Setup def;
	if (!s) { memset(&def, 0, sizeof def); def.builtin_pud = PUD_BUILTIN_SKIRMISH_FIRST; def.local_player = -1; def.local_race = -1; s = &def; }

	r = s->builtin_pud > 0 ? pud_load_builtin(s->builtin_pud) : pud_load_file(s->pud_path);
	if (r) { w2_log("game: cannot load map %d %s", s->builtin_pud, s->builtin_pud > 0 ? "" : s->pud_path); return -1; }
	move_reset_all();
	combat_reset();
	econ_reset();

	/* the local player: the given slot, else the first human slot, else the first slot with anything */
	if (s->local_player >= 0 && s->local_player < 8) local = s->local_player;
	for (p = 0; p < 8 && local < 0; p++) if (w2.player[p].control == W2_CTRL_HUMAN) local = p;
	for (p = 0; p < 8 && local < 0; p++) if (w2.player[p].control != W2_CTRL_NOBODY) local = p;
	if (local < 0) local = 0;
	w2.local_player = (uint8_t)local;
	w2.player[local].control = W2_CTRL_HUMAN;
	if (w2cfg_extra_int("spectate", 0)) w2.player[local].control = W2_CTRL_COMPUTER;  /* test: AI plays the local slot too */

	/* every other playing slot is the computer (single player skirmish); optionally only N of them */
	for (p = 0; p < 8; p++) {
		W2Player *pl = &w2.player[p];
		if (p == local) continue;
		if (pl->control == W2_CTRL_HUMAN) pl->control = W2_CTRL_COMPUTER;
		if (pl->control == W2_CTRL_COMPUTER && s->opponents > 0) {
			if (kept >= s->opponents) { remove_units_of(p); pl->control = W2_CTRL_NOBODY; continue; }
			kept++;
		}
	}

	if (s->local_race == 0 || s->local_race == 1) {
		if (w2.player[local].race != s->local_race) convert_race(local, s->local_race);
		w2.player[local].race = (uint8_t)s->local_race;
	}

	if (s->resources >= 1 && s->resources <= 3)
		for (p = 0; p < 8; p++) {
			w2.player[p].gold = res_tab[s->resources][0];
			w2.player[p].wood = res_tab[s->resources][1];
			w2.player[p].oil  = res_tab[s->resources][2];
		}

	if (s->units == 1)       /* "one peasant only": every playing slot keeps a single worker at its start */
		for (p = 0; p < 8; p++) {
			int tx, ty, c = w2.player[p].control;
			if (c != W2_CTRL_HUMAN && c != W2_CTRL_COMPUTER) continue;
			start_point(p, &tx, &ty);
			remove_units_of(p);
			if (tx >= 0) unit_create(w2.player[p].race == 1 ? W2U_PEON : W2U_PEASANT, p, tx, ty);
		}

	for (p = 0; p < 8; p++) econ_recount(p);
	for (p = 0; p < 8; p++) {
		int c = w2.player[p].control;
		g_present[p] = (uint8_t)((c == W2_CTRL_HUMAN || c == W2_CTRL_COMPUTER) && count_units(p, NULL) > 0);
		w2.player[p].defeated = 0;
	}

	/* palette of the map's era (a menu may have loaded a picture palette since the era was cached) */
	if (assets_era() >= 0 && assets_era() < 3) assets_load_palette(ASSET_DATA, k_era_pal[assets_era()]);
	assets_apply_palette();

	{
		int seed = w2cfg_extra_int("seed", 0);
		w2.rng = seed ? (uint32_t)seed : (uint32_t)amigagfx_millis() * 2654435761u + 1u;
	}
	w2.cycle = 0;
	w2.game_over = 0;
	g_end_wait = 0;
	apply_mode();

	view.scr_x = GAME_VIEW_X; view.scr_y = GAME_VIEW_Y; view.scr_w = GAME_VIEW_W; view.scr_h = GAME_VIEW_H;
	view.lores = assets_lores();
	mapdraw_reset();
	center_on_local();
	ui_init();
	ai_init();
	fog_update();
	snd_music(1);
	game_state = GS_PLAY;
	g_clear = 2;
	g_keys = 0;
	w2_log("game: start map %d%s%s, %dx%d era %d, local player %d (%s), think period %d",
	       s->builtin_pud, s->builtin_pud > 0 ? "" : " ", s->builtin_pud > 0 ? "" : s->pud_path, w2.map.w, w2.map.h, w2.map.era,
	       local, w2.player[local].race ? "orc" : "human", w2_think_period);
	return 0;
}

void game_end(int result)
{
	snd_music(0);
	w2.game_over = (uint8_t)(result == 1 || result == 2 ? result : w2.game_over);
	w2_log("game: end %d at cycle %lu", result, (unsigned long)w2.cycle);
	if (result >= 1 && result <= 3) { game_state = GS_END; endscreen_enter(result); }
	else { game_state = GS_MENU; menu_enter(); }
}

/* all present enemies gone -> victory; the local player has nothing left -> defeat */
static void check_end(void)
{
	int p, local = w2.local_player, enemies = 0, had = 0;
	for (p = 0; p < 8; p++) {
		if (!g_present[p] || w2.player[p].defeated) continue;
		if (count_units(p, NULL) == 0) {
			w2.player[p].defeated = 1;
			w2_log("game: player %d defeated at cycle %lu", p, (unsigned long)w2.cycle);
			continue;
		}
		if (p != local && ((w2.player[local].enemies >> p) & 1)) enemies++;
	}
	if (g_present[local] && w2.player[local].defeated) { w2.game_over = 2; g_end_wait = 90; return; }
	for (p = 0; p < 8; p++) if (p != local && g_present[p] && ((w2.player[local].enemies >> p) & 1)) had++;
	if (had && !enemies) { w2.game_over = 1; g_end_wait = 90; }
}

/* ---------------------------------------------------------------- hooks */
int game_init(void)
{
	const char *a;
	apply_mode();
	if (assets_init()) return -1;
	if (snd_init()) w2_log("game: sound init failed - silent");
	if (assets_lores() && lores_prepare()) w2_log("game: lores_prepare failed - point-sampled art");
	view.lores = assets_lores();
	view.scr_x = GAME_VIEW_X; view.scr_y = GAME_VIEW_Y; view.scr_w = GAME_VIEW_W; view.scr_h = GAME_VIEW_H;
	a = w2cfg_extra("autostart");
	if (a && *a) {
		W2Setup s;
		char *end;
		long v = strtol(a, &end, 0);
		memset(&s, 0, sizeof s);
		s.local_player = w2cfg_extra_int("autostart_player", -1);
		s.local_race = w2cfg_extra_int("autostart_race", -1);
		s.opponents = w2cfg_extra_int("autostart_opponents", 0);
		s.resources = w2cfg_extra_int("autostart_resources", 0);
		s.units = w2cfg_extra_int("autostart_units", 0);
		if (end != a && *end == 0 && v > 0) s.builtin_pud = (int)v;
		else snprintf(s.pud_path, sizeof s.pud_path, "%s", a);
		w2_log("game: autostart %s", a);
		if (game_start(&s) == 0) return 0;
		w2_log("game: autostart failed - menu");
	}
	game_state = GS_MENU;
	menu_enter();
	return 0;
}

static void scroll_tick(void)
{
	int step = w2_cfg.scroll_speed * 4, dx = 0, dy = 0, sw = 640, sh = 480, edge = 2;
	if (step < 4) step = 4;
	if (g_keys & 1) dy -= step;
	if (g_keys & 2) dy += step;
	if (g_keys & 4) dx -= step;
	if (g_keys & 8) dx += step;
	if (g_mouse_seen) {
		if (game_mouse_x < edge) dx -= step;
		if (game_mouse_x >= sw - edge) dx += step;
		if (game_mouse_y < edge) dy -= step;
		if (game_mouse_y >= sh - edge) dy += step;
	}
	if (dx || dy) view_scroll(dx, dy);
}

void game_tick(void)
{
	if (game_state != GS_PLAY) { menu_tick(); return; }
	if (ui_menu_active()) { ui_tick(); return; }   /* in-game menu open: the world stands still */
	scroll_tick();
	unit_tick_all();
	missile_tick_all();
	ai_tick();
	if (w2_faithful || (w2.cycle & 3) == 0) fog_update();
	ui_tick();
	w2.cycle++;
	if (!w2.game_over) {
		check_end();
	} else if (g_end_wait > 0 && --g_end_wait == 0) {
		game_end(w2.game_over);
	}
}

int game_draw(unsigned char *buf, int pitch, int w, int h)
{
	int lo;
	snd_service();      /* every frame in every state: effect queue + music stream (menus too) */
	lo = assets_lores();
	view.buf = buf; view.pitch = pitch; view.lores = lo;   /* the menu draws through view.buf too */
	if (game_state != GS_PLAY) {
		if (g_clear > 0 || g_menu_was_play) { g_menu_was_play = 0; menu_force_redraw(); }
		/* the game's own pointer in the menus too: the Workbench prefs pointer is drawn for lores
		 * sprites and shows half as wide on our 70 ns hires sprites */
		ui_cursor_set(UI_CUR_POINT, 0);
		if (!menu_needs_redraw()) return 0;
		menu_draw();
		return 1;
	}
	if (!g_menu_was_play) g_was_menu = 1;
	g_menu_was_play = 1;
	view.scr_x = GAME_VIEW_X; view.scr_y = GAME_VIEW_Y;
	view.scr_w = GAME_VIEW_W; view.scr_h = GAME_VIEW_H;
	/* a smaller buffer (Workbench window): shrink the viewport, never draw outside */
	if ((view.scr_x + view.scr_w) >> lo > w) view.scr_w = (w << lo) - view.scr_x;
	if ((view.scr_y + view.scr_h) >> lo > h) view.scr_h = (h << lo) - view.scr_y;
	if (view.scr_w < 0) view.scr_w = 0;
	if (view.scr_h < 0) view.scr_h = 0;
	g_full_frame = 0;
	if (g_clear > 0) {
		int y;
		for (y = 0; y < h; y++) memset(buf + (long)y * pitch, mapdraw_black(), (size_t)w);
		g_clear--;
		g_full_frame = 1;
		ui_panel_force();
	}
	if (g_was_menu) { g_was_menu = 0; g_full_frame = 1; ui_panel_force(); }
	{
		unsigned long t0 = amiga_uclock_us(), t1, t2, t3, t4, t5;
		mapdraw_draw();
		t1 = amiga_uclock_us();
		unitdraw_draw();
		t2 = amiga_uclock_us();
		missile_draw_all();
		t3 = amiga_uclock_us();
		if (game_draws_minimap && (GAME_MINIMAP_X + GAME_MINIMAP_SZ) >> lo <= w && (GAME_MINIMAP_Y + GAME_MINIMAP_SZ) >> lo <= h)
			minimap_draw(GAME_MINIMAP_X, GAME_MINIMAP_Y, GAME_MINIMAP_SZ);
		t4 = amiga_uclock_us();
		ui_draw();
		t5 = amiga_uclock_us();
		game_prof_us[0] += t1 - t0; game_prof_us[1] += t2 - t1; game_prof_us[2] += t3 - t2;
		game_prof_us[3] += t4 - t3; game_prof_us[4] += t5 - t4;
	}
	return 1;
}

/* the rectangles drawn by the last game_draw, in BUFFER pixels; -1 = convert the whole screen */
int game_dirty(short (*out)[4], int max)
{
	int n, i, lo = view.lores;
	if (game_state != GS_PLAY || g_full_frame || max < 2 || !w2_dirty) return -1;
	out[0][0] = (short)view.scr_x; out[0][1] = (short)view.scr_y; out[0][2] = (short)view.scr_w; out[0][3] = (short)view.scr_h;
	n = 1 + ui_dirty_rects(out + 1, max - 1);
	for (i = 0; i < n; i++) {
		out[i][0] = (short)(out[i][0] >> lo); out[i][1] = (short)(out[i][1] >> lo);
		out[i][2] = (short)(out[i][2] >> lo); out[i][3] = (short)(out[i][3] >> lo);
	}
	return n;
}

int game_event(const AmigaGfxEvent *ev)
{
	int lo = assets_lores();
	if (ev->type == AMIGAGFX_EV_MOUSEMOVE || ev->type == AMIGAGFX_EV_MOUSEDOWN || ev->type == AMIGAGFX_EV_MOUSEUP) {
		game_mouse_x = ev->x << lo; game_mouse_y = ev->y << lo;
		if (ev->type == AMIGAGFX_EV_MOUSEMOVE) g_mouse_seen = 1;
	}
	if (game_state != GS_PLAY) return menu_event(ev);
	if (ui_event(ev)) return 1;
	if (ev->type == AMIGAGFX_EV_KEY) {
		int k = ev->code & 0x7F, bit = k == RAW_UP ? 1 : k == RAW_DOWN ? 2 : k == RAW_LEFT ? 4 : k == RAW_RIGHT ? 8 : 0;
		if (bit) {
			if (ev->code & 0x80) g_keys &= ~bit; else g_keys |= bit;
			return 1;
		}
	}
	return 0;
}

void game_shutdown(void)
{
	snd_music(0);
	snd_shutdown();
	assets_shutdown();
	w2_log("game: shutdown after %lu cycles", (unsigned long)w2.cycle);
}

int game_wants_quit(void) { return g_quit || menu_wants_quit() || ui_wants_quit(); }
