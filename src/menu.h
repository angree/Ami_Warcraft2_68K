/* menu.h - extras of menu.c beyond the [menu.c] block of w2.h (menu_enter/draw/event/tick,
 * endscreen_enter).
 *
 * Screens: (logo movie) -> title (War Data 299) -> main menu -> options / single player ->
 * custom game setup -> select scenario, or campaign race -> missions -> (act title, movie) ->
 * briefing -> mission; end screen (victory / defeat + stats) -> next briefing / the same again. Everything is drawn in HIRES coordinates
 * into view.buf / view.pitch (halved in lores, assets_lores()); mouse events arrive in the current
 * mode's pixels and are doubled in lores.
 *
 * game.c contract (requests, see the module report):
 *  - menu_draw()/menu_event() serve both GS_MENU and GS_END; leaving the end screen sets
 *    game_state = GS_MENU itself.
 *  - game_wants_quit() should return menu_wants_quit() (main menu "Exit Program").
 *  - menu_draw() redraws the whole screen every call: game_draw should return 1 (blit all).
 */
#ifndef W2_MENU_H
#define W2_MENU_H

/* 1 once the player chose "Exit Program". */
int menu_wants_quit(void);

/* Dirty flag: 1 if menu_draw would change the screen; game.c skips draw + blit otherwise (AGA c2p of a
 * whole 640x480 frame is expensive). menu_force_redraw after something else drew over the screen. */
int  menu_needs_redraw(void);
void menu_force_redraw(void);

/* Game type chosen in the custom game setup (W2Setup has no field for it yet):
 * 0 use map settings, 1 melee, 2 free for all. */
int menu_game_type(void);

/* The screen now shown (MENU_SCR_*), for tests and the game shell. */
enum { MENU_SCR_NONE, MENU_SCR_MOVIE, MENU_SCR_TITLE, MENU_SCR_MAIN, MENU_SCR_OPTIONS, MENU_SCR_SETUP,
       MENU_SCR_MAPS, MENU_SCR_END,
       MENU_SCR_SINGLE,     /* New Campaign / Custom Scenario / Previous Menu */
       MENU_SCR_CAMPAIGN,   /* Human Campaign / Orc Campaign */
       MENU_SCR_LEVELS,     /* the missions reached so far (campaign.cfg), the furthest one selected */
       MENU_SCR_ACT,        /* act title picture before the first mission of an act */
       MENU_SCR_BRIEF,      /* mission briefing: picture, title, scrolling text, objectives, voice */
       MENU_SCR_FINAL,      /* after the last mission: the race's ending text and voice */
       MENU_SCR_COUNT };
int menu_screen(void);

/* 0 = skip the logo movie before the title (default 1; the test switches it off/on). */
void menu_set_movies(int on);

/* Custom game map list (built the first time the setup screen opens): count, and one entry's
 * description line ("name  (N players, WxH)") - for tests and logs. */
int  menu_map_count(void);
const char *menu_map_name(int i);

#endif
