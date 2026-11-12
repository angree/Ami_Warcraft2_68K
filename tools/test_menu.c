/* test_menu.c - host test of src/menu.c: drives every menu screen with synthetic events and writes
 * a PNG of each screen (host_shot -> $W2_SHOTDIR, which must be OUTSIDE the repository).
 *
 * Environment: W2_DATA (game data dir, "War Data" etc.), W2_RES (hires | lores), W2_PROGDIR,
 * W2_SHOTDIR (see docs/HOST_TEST.md). Built by tools/build_test_menu.sh. Exit code 0 = all checks ok.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "w2.h"
#include "menu.h"
#include "assets.h"
#include "w2cfg.h"
#include "amiga_gfx.h"

void host_clock_advance_ms(unsigned long ms);
int  host_shot(const char *name);

/* ---- shell services normally in main_amiga.c */
void w2_log(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	printf("[w2] ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
	fflush(stdout);
}
unsigned long w2_tick_ms(void) { return 33; }
unsigned long w2_frame_count(void) { return 0; }

/* ---- game_start: fails while g_start_ok == 0 (like the weak stub), else "starts" the game */
static int g_start_ok, g_start_calls;
static W2Setup g_last_setup;
int game_start(const W2Setup *s)
{
	g_start_calls++;
	g_last_setup = *s;
	if (!g_start_ok) return -1;
	assets_set_era(0);
	game_state = GS_PLAY;
	return 0;
}

static int g_fail, g_lo, g_W, g_H;
static const char *g_tag;

static void check(int ok, const char *what)
{
	printf("%s %s\n", ok ? "PASS" : "FAIL", what);
	if (!ok) g_fail++;
}

static void frame(const char *shot)
{
	char n[64];
	menu_draw();
	amigagfx_blit(0, 0, g_W, g_H);
	if (shot) { snprintf(n, sizeof n, "%s_%s", g_tag, shot); host_shot(n); }
}

static void ev(int type, int x, int y, int code)   /* x,y HIRES; converted to the mode's pixels */
{
	AmigaGfxEvent e;
	e.type = type; e.x = x >> g_lo; e.y = y >> g_lo; e.code = code;
	menu_event(&e);
}
static void clickat(int x, int y) { ev(AMIGAGFX_EV_MOUSEMOVE, x, y, 0); ev(AMIGAGFX_EV_MOUSEDOWN, x, y, AMIGAGFX_BUTTON_LEFT); frame(NULL); ev(AMIGAGFX_EV_MOUSEUP, x, y, AMIGAGFX_BUTTON_LEFT); }
static void rclickat(int x, int y) { ev(AMIGAGFX_EV_MOUSEDOWN, x, y, AMIGAGFX_BUTTON_RIGHT); ev(AMIGAGFX_EV_MOUSEUP, x, y, AMIGAGFX_BUTTON_RIGHT); }
static void key(int code) { ev(AMIGAGFX_EV_KEY, 0, 0, code); ev(AMIGAGFX_EV_KEY, 0, 0, code | 0x80); }

static void fake_world(int race, int result_units)
{
	int i;
	memset(&w2.player, 0, sizeof w2.player);
	memset(&w2.unit, 0, sizeof w2.unit);
	w2.local_player = 0;
	w2.player[0].control = W2_CTRL_HUMAN; w2.player[0].race = (uint8_t)race;
	w2.player[0].gold = 2350; w2.player[0].wood = 1200; w2.player[0].oil = 400;
	w2.player[1].control = W2_CTRL_COMPUTER; w2.player[1].race = (uint8_t)!race;
	w2.player[1].gold = 900; w2.player[1].wood = 300; w2.player[1].oil = 0;
	w2.player[2].control = W2_CTRL_COMPUTER; w2.player[2].race = 1;
	w2.player[2].gold = 15000; w2.player[2].wood = 5000; w2.player[2].oil = 2000;
	w2.player[W2_NEUTRAL].control = W2_CTRL_NEUTRAL;
	for (i = 0; i < result_units; i++) {
		W2Unit *u = &w2.unit[i];
		u->flags = W2_UF_ALIVE | (i % 5 == 0 ? W2_UF_BUILDING : 0);
		u->owner = (uint8_t)(i % 3);
		u->kills = (uint16_t)(i % 4);
	}
	w2.unit_hi = (uint16_t)result_units;
}

int main(void)
{
	const char *data = getenv("W2_DATA"), *res = getenv("W2_RES");
	int i, n;
	unsigned char era_pal[768];
	w2cfg_defaults(&w2_cfg);
	if (data) snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", data);
	g_lo = res && !strcmp(res, "lores");
	w2_cfg.resolution = g_lo ? W2_RES_LORES : W2_RES_HIRES;
	g_tag = g_lo ? "lo" : "hi";
	g_W = 640 >> g_lo; g_H = 480 >> g_lo;
	if (assets_init()) { printf("FAIL assets_init\n"); return 1; }
	if (amigagfx_open(g_W, g_H, 0, AMIGAGFX_BACKEND_RTG)) { printf("FAIL amigagfx_open\n"); return 1; }
	view.buf = amigagfx_chunky(); view.pitch = amigagfx_pitch(); view.lores = g_lo;
	assets_set_era(0);                         /* a game ran before: era 0 already loaded */
	game_state = GS_MENU;

	/* 1. logo movie -> title */
	menu_set_movies(1);
	menu_enter();
	check(menu_screen() == MENU_SCR_MOVIE, "menu_enter starts with the logo movie");
	for (i = 0; i < 40; i++) { host_clock_advance_ms(66); frame(i == 39 ? "01_movie" : NULL); }
	key(0x40);
	check(menu_screen() == MENU_SCR_TITLE, "key skips the movie -> title");
	frame("02_title");

	/* 2. title -> main */
	clickat(300, 300);
	check(menu_screen() == MENU_SCR_MAIN, "click on the title -> main menu");
	ev(AMIGAGFX_EV_MOUSEMOVE, 300, 290, 0);   /* hover "Options" */
	frame("03_main");

	/* 3. options: change everything, OK writes w2_cfg + the cfg file */
	w2_cfg.sfx_volume = 50; w2_cfg.music_volume = 50; w2_cfg.game_speed = 3; w2_cfg.scroll_speed = 3; w2_cfg.music = 1;
	clickat(320, 290);
	check(menu_screen() == MENU_SCR_OPTIONS, "Options button -> options screen");
	frame("04_options");
	clickat(170 + 290, 124 + 9);               /* sfx: right arrow +10 */
	key(0x4D); key(0x4F); key(0x4F);           /* focus music volume, -20 */
	clickat(170 + 20 + 8 + 243, 216 + 9);      /* game speed: knob to the far right = 5 */
	key(0x4D); key(0x4F);                      /* scroll speed -1 */
	clickat(180, 305);                         /* music off */
	frame("05_options_changed");
	clickat(170 + 50, 364 + 14);               /* OK */
	check(menu_screen() == MENU_SCR_MAIN, "options OK -> main");
	printf("cfg: sfx %d music %d vol %d speed %d scroll %d\n", w2_cfg.sfx_volume, w2_cfg.music, w2_cfg.music_volume, w2_cfg.game_speed, w2_cfg.scroll_speed);
	check(w2_cfg.sfx_volume == 60 && w2_cfg.music_volume == 30 && w2_cfg.game_speed == 5 && w2_cfg.scroll_speed == 2 && w2_cfg.music == 0,
	      "options values written to w2_cfg");
	{
		W2Cfg c; check(w2cfg_load(&c, W2CFG_FILE) == 0 && c.sfx_volume == 60 && c.game_speed == 5, "options saved to PROGDIR:w2.cfg");
	}
	/* cancel does not write */
	clickat(320, 290); clickat(170 + 290, 124 + 9); clickat(364 + 50, 364 + 14);
	check(menu_screen() == MENU_SCR_MAIN && w2_cfg.sfx_volume == 60, "options Cancel keeps w2_cfg");

	/* 4. keyboard: focus Single Player Game, Return -> setup */
	key(0x4C); key(0x4C); key(0x4C);
	key(0x44);
	check(menu_screen() == MENU_SCR_SETUP, "keyboard (up + Return) -> custom game setup");
	n = menu_map_count();
	printf("maps: %d\n", n);
	for (i = 0; i < n; i++) printf("  map %2d: %s\n", i, menu_map_name(i));
	check(n >= 21, "map list has the 21 built-in skirmish maps");
	frame("06_setup");

	/* 5. setup choices */
	clickat(430 + 80, 72 + 9);                 /* race: Human */
	clickat(430 + 80, 72 + 9);                 /* race: Orc */
	clickat(430 + 80, 104 + 9); clickat(430 + 80, 104 + 9);   /* resources: Medium */
	clickat(430 + 80, 136 + 9);                /* units: one peasant */
	rclickat(430 + 80, 168 + 9);               /* opponents: last value */
	key(0x4D); key(0x4E);                      /* focus game type, right = Melee */
	frame("07_setup_changed");

	/* 6. select scenario */
	clickat(380 + 112, 332 + 14);
	check(menu_screen() == MENU_SCR_MAPS, "Select Scenario -> map list");
	frame("08_maps");
	for (i = 0; i < 14; i++) key(0x4D);
	frame("09_maps_scrolled");
	clickat(452 + 9, 104 + 10);                /* up arrow */
	key(0x44);                                 /* Return = OK */
	check(menu_screen() == MENU_SCR_SETUP, "map list Return -> setup");
	frame("10_setup_map");

	/* 7. Start Game with a failing game_start -> error line */
	g_start_ok = 0;
	clickat(380 + 112, 296 + 14);
	check(g_start_calls == 1 && menu_screen() == MENU_SCR_SETUP, "Start Game calls game_start; failure stays on setup");
	printf("setup: builtin %d path '%s' race %d opp %d res %d units %d type %d\n", g_last_setup.builtin_pud, g_last_setup.pud_path,
	       g_last_setup.local_race, g_last_setup.opponents, g_last_setup.resources, g_last_setup.units, menu_game_type());
	check(g_last_setup.local_race == 1 && g_last_setup.resources == 2 && g_last_setup.units == 1 && g_last_setup.opponents > 0,
	      "W2Setup filled from the choices");
	frame("11_setup_error");

	/* 8. Start Game succeeding -> menu idle, era palette on screen */
	g_start_ok = 1;
	key(0x21);                                 /* hotkey 's' = Start Game */
	{
		unsigned long pn; unsigned char *p = assets_load(ASSET_DATA, 2, &pn);
		for (i = 0; p && i < 768; i++) { int v = p[i] & 63; era_pal[i] = (unsigned char)((v << 2) | (v >> 4)); }
		war_free(p);
	}
	check(menu_screen() == MENU_SCR_NONE && game_state == GS_PLAY, "successful game_start leaves the menu");
	check(!memcmp(assets_palette(), era_pal, 768), "era palette restored after game_start");

	/* 9. end screens */
	game_state = GS_END;
	fake_world(0, 40);
	endscreen_enter(1);
	check(menu_screen() == MENU_SCR_END, "endscreen_enter(1) -> victory screen");
	frame("12_victory_human");
	clickat(100, 100);
	check(menu_screen() == MENU_SCR_MAIN && game_state == GS_MENU, "click on the end screen -> main menu, GS_MENU");
	game_state = GS_END;
	fake_world(1, 25);
	endscreen_enter(2);
	frame("13_defeat_orc");
	key(0x44);
	check(menu_screen() == MENU_SCR_MAIN && game_state == GS_MENU, "key on the end screen -> main menu");
	fake_world(1, 10);
	endscreen_enter(1);
	frame("14_victory_orc");
	clickat(320, 444);
	fake_world(0, 10);
	endscreen_enter(2);
	frame("15_defeat_human");
	key(0x45);
	check(menu_screen() == MENU_SCR_MAIN, "Esc on the end screen -> main");
	endscreen_enter(3);
	check(menu_screen() == MENU_SCR_MAIN, "endscreen_enter(3) = quit -> main");

	/* 10. exit program */
	check(!menu_wants_quit(), "no quit yet");
	clickat(320, 326);
	check(menu_wants_quit() == 1, "Exit Program sets menu_wants_quit");

	amigagfx_close();
	assets_shutdown();
	printf("%s: %d failure(s)\n", g_tag, g_fail);
	return g_fail ? 1 : 0;
}
