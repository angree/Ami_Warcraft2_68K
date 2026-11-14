/* game.h - extras of game.c beyond the [game.c] block of w2.h and the hooks of w2cfg.h. */
#ifndef W2_GAME_H
#define W2_GAME_H

#include "w2.h"

/* W2 in-game layout, HIRES screen pixels (wargus ui: MapArea 176,16 .. 623,463; minimap 24,26 128x128). */
#define GAME_VIEW_X     176
#define GAME_VIEW_Y     16
#define GAME_VIEW_W     448
#define GAME_VIEW_H     448
#define GAME_MINIMAP_X  24
#define GAME_MINIMAP_Y  26
#define GAME_MINIMAP_SZ 128

/* Ask the main loop to end (menu "Exit program"). */
void game_quit(void);

/* Mouse position last seen by game_event, HIRES screen pixels (lores events are doubled). */
extern int game_mouse_x, game_mouse_y;

/* 1 (default): game_draw draws the minimap itself (minimap_draw at GAME_MINIMAP_*) just BEFORE ui_draw,
 * so the UI only draws the panel/frame around it (and must leave the 128x128 inside alone).
 * ui.c may set this to 0 and call minimap_draw itself. */
extern int game_draws_minimap;

/* Player counts as present for victory/defeat: had units or buildings when the game started. */
int game_player_alive(int player);

/* rectangles drawn by the last game_draw (buffer pixels x,y,w,h); -1: convert the whole screen */
int game_dirty(short (*out)[4], int max);
extern int w2_dirty;   /* 1: dirty rectangles on (default) */

#endif
