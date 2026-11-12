/* w2cfg.h - the AmiW2 settings file, shared by the game (w2) and the settings tool (w2setup).
 *
 * Plain C, stdio only, NO Amiga headers and NO floats: game files may include this (it pulls in
 * native/amiga_gfx.h, which is Amiga-header-free by design - compile with -Inative).
 *
 * File: PROGDIR:w2.cfg, one "key=value" per line, integers or words, '#' starts a comment line.
 * Keys this module does not know are KEPT, in order, when the file is written again (game modules may
 * add their own keys and read them with w2cfg_extra()).
 * "Use" in w2setup writes ENV:w2.cfg instead (lost at reboot); w2cfg_load_default() prefers it while it
 * exists, and "Save" deletes it - the usual Amiga preferences Save / Use semantics.
 *
 *   key           values                            default        meaning
 *   backend       auto | aga | rtg | ehb | wb       auto           display; auto = RTG if a mode exists, else AGA
 *                                                                  (EHB on a non-AGA chipset); MorphOS always RTG
 *   resolution    hires | lores                     hires          hires 640x480, lores 320x240
 *   rtg_depth     0 | 8 | 15 | 16 | 24 | 32         0              RTG pixel depth; 0/8 = 8-bit LUT first
 *   titlebar      0 | 1                             0              Intuition screen title bar above the game
 *   audio         paula | ahi | off                 paula          sound device (MorphOS: ahi)
 *   music         on | off                          on
 *   sfx_volume    0..100                            80
 *   music_volume  0..100                            70
 *   game_speed    1..5                              3              1 slowest .. 5 fastest (3 = normal)
 *   logic_skip    0..8                              0              0 = off; N = the heavy unit logic is split
 *                                                                  across N frames (AmiSC-style logic frameskip)
 *   scroll_speed  1..5                              3
 *   show_fps      0 | 1                             0              frame counter in the top-left corner
 *   data_dir      path (max 127 chars)              PROGDIR:data   converted game data
 */
#ifndef W2CFG_H
#define W2CFG_H

#include "amiga_gfx.h"   /* AmigaGfxEvent; amiga_gfx.h is itself free of Amiga headers */

#ifdef __cplusplus
extern "C" {
#endif

#define W2CFG_FILE     "PROGDIR:w2.cfg"
#define W2CFG_ENV_FILE "ENV:w2.cfg"

enum { W2_BACKEND_AUTO, W2_BACKEND_AGA, W2_BACKEND_RTG, W2_BACKEND_EHB, W2_BACKEND_WB, W2_BACKEND_COUNT };
enum { W2_RES_HIRES, W2_RES_LORES };
enum { W2_AUDIO_PAULA, W2_AUDIO_AHI, W2_AUDIO_OFF, W2_AUDIO_COUNT };

typedef struct W2Cfg {
	int  backend;        /* W2_BACKEND_* */
	int  resolution;     /* W2_RES_* */
	int  rtg_depth;      /* 0, 8, 15, 16, 24, 32 */
	int  titlebar;       /* 0 / 1 */
	int  audio;          /* W2_AUDIO_* */
	int  music;          /* 0 / 1 */
	int  sfx_volume;     /* 0..100 */
	int  music_volume;   /* 0..100 */
	int  game_speed;     /* 1..5 */
	int  logic_skip;     /* 0..8 */
	int  scroll_speed;   /* 1..5 */
	int  show_fps;       /* 0 / 1 */
	char data_dir[128];
} W2Cfg;

/* The settings the game runs with (filled by main_amiga.c before game_init()). Read-only for game code. */
extern W2Cfg w2_cfg;

void w2cfg_defaults(W2Cfg *c);
/* Read path into c: defaults first, then every known key found; values are clamped to their ranges.
 * Unknown lines are remembered for w2cfg_save. Returns 0 if the file was read, -1 if it does not exist. */
int  w2cfg_load(W2Cfg *c, const char *path);
/* ENV:w2.cfg if it exists (w2setup "Use"), else PROGDIR:w2.cfg. Returns the path actually read, or NULL
 * when neither exists (c then holds the defaults). */
const char *w2cfg_load_default(W2Cfg *c);
/* Write all known keys plus the remembered unknown lines. 0 = ok. */
int  w2cfg_save(const W2Cfg *c, const char *path);

/* An unknown key's value as last read (or NULL): game modules can keep their own keys in w2.cfg. */
const char *w2cfg_extra(const char *key);
/* Integer version of w2cfg_extra with a default. */
int  w2cfg_extra_int(const char *key, int def);

/* Screen size the resolution key stands for (hires 640x480, lores 320x240). */
int  w2cfg_screen_w(const W2Cfg *c);
int  w2cfg_screen_h(const W2Cfg *c);

/* Value <-> name tables (for logs and the settings tool). */
const char *w2cfg_backend_name(int backend);
const char *w2cfg_audio_name(int audio);

/* ---------------------------------------------------------------------------------------------
 * Game hooks - the engine side, called by main_amiga.c (the platform shell). Implement them in
 * game files under src/; main_amiga.c carries stub versions that are compiled ONLY while no file
 * in src/ defines game_init (build.sh detects that and passes -DW2_HAVE_GAME).
 *
 *   int  game_init(void)         after w2_cfg is loaded, the screen is open and a grey palette is
 *                                set; 0 = ok, non-zero = abort (logged + requester).
 *   void game_tick(void)         one logic cycle; called at the fixed rate w2_tick_ms() (game_speed),
 *                                several times in a row when drawing fell behind (max 5).
 *   int  game_draw(unsigned char *buf, int pitch, int w, int h)
 *                                draw a frame into the chunky buffer (8-bit, pitch bytes per row).
 *                                Return 1 = blit the whole screen for me, 0 = I blitted what changed
 *                                myself with amigagfx_blit(x, y, w, h).
 *   int  game_event(const AmigaGfxEvent *ev)
 *                                input (mouse in game-area pixels, raw key codes incl. releases).
 *                                Return 1 = consumed. ESC (raw 0x45) quits only when NOT consumed.
 *   void game_shutdown(void)     before the screen closes.
 *   int  game_wants_quit(void)   non-zero ends the main loop.
 * --------------------------------------------------------------------------------------------- */
int  game_init(void);
void game_tick(void);
int  game_draw(unsigned char *buf, int pitch, int w, int h);
int  game_event(const AmigaGfxEvent *ev);
void game_shutdown(void);
int  game_wants_quit(void);

/* Shell services for game code (main_amiga.c): */
void          w2_log(const char *fmt, ...);   /* one line into PROGDIR:w2.log, flushed */
unsigned long w2_tick_ms(void);               /* ms per logic cycle for the current game_speed */
unsigned long w2_frame_count(void);           /* frames drawn so far */

#ifdef __cplusplus
}
#endif

#endif
