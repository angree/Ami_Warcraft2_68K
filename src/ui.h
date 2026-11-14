/* ui.h - in-game interface: extras beyond the [ui.c] block of w2.h (ui_init/ui_draw/ui_event/
 * ui_message/ui_tick live there). Owner: ui.c + ui_cursor.c.
 *
 * Layout (hires 640x480 units, as the original / Wargus ui.lua):
 *   left panel 176 px: menu button strip 0..24, minimap frame 24..160 (map 128x128 at 24,26),
 *   info panel 160..336, command buttons 336..480 (3x3, icons 46x38 at x 9/65/121, y 340/387/434);
 *   resource bar 176..624 x 0..16, status line 176..624 x 464..480, right filler 624..640.
 *   The map viewport itself is view.scr_* (game.c sets it to 176,16 448x448).
 */
#ifndef W2_UI_H
#define W2_UI_H

/* ---- extra queries for game.c ---- */
int  ui_menu_active(void);     /* the in-game menu is open (game.c may pause the simulation) */
int  ui_wants_quit(void);      /* "Exit Program" was chosen in the in-game menu */

/* ---- hardware pointer shapes (ui_cursor.c) ---- */
enum {
	UI_CUR_POINT,        /* race gauntlet / claw (301/302) */
	UI_CUR_NO,           /* "don't click here" (303/304) */
	UI_CUR_TARGET,       /* yellow eagle / crosshair (305/306) - choosing a target */
	UI_CUR_TARGET_RED,   /* red (307/308) - over an enemy */
	UI_CUR_TARGET_GREEN, /* green (309/310) - over an own/allied unit */
	UI_CUR_GLASS,        /* magnifying glass (311) - over a unit */
	UI_CUR_CROSS,        /* small green cross (312) - drag-box selection */
	UI_CUR_WAIT,         /* hourglass (313) */
	UI_CUR_ARROW_N,      /* scroll arrows 315..322, N clockwise to NW */
	UI_CUR_ARROW_NE, UI_CUR_ARROW_E, UI_CUR_ARROW_SE, UI_CUR_ARROW_S,
	UI_CUR_ARROW_SW, UI_CUR_ARROW_W, UI_CUR_ARROW_NW,
	UI_CUR_COUNT
};
/* Show a shape as the hardware pointer (cached per shape, race, era and resolution). Cheap when the
 * shape did not change. race 0 human 1 orc. */
void ui_cursor_set(int shape, int race);
int  ui_cursor_shape(void);     /* shape shown now (-1 none) */
/* panel dirty regions: force a full panel redraw next frame; the regions drawn this frame (hires x,y,w,h) */
void ui_panel_force(void);
int  ui_dirty_rects(short (*out)[4], int max);
void ui_cursor_flush(void);    /* forget the current shape (after a screen reopen) */
/* Test/debug: the reduced pointer image of a shape (pix values 0..3), its hot spot and pen colours. */
int  ui_cursor_debug(int shape, int race, const unsigned char **pix, int *w, int *h,
                     int *hotx, int *hoty, unsigned char rgb9[9]);

/* ---- test/debug ---- */
/* One line describing the UI state: mode, card level, selection and the 9 card buttons. */
const char *ui_debug_state(void);

#endif
