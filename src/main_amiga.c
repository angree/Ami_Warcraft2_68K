/* main_amiga.c - AmiW2 platform shell: entry point on AmigaOS 3.x / AROS 68k / MorphOS (one 68k binary).
 *
 * What it does, in order (every step is one line in PROGDIR:w2.log):
 *   1. log PROGDIR:w2.log (removed first - freopen("w") does not truncate here), line 1 = version + build stamp
 *   2. CPU-exception trap armed (a Guru becomes a "TRAP" block in the log, then a clean exit)
 *   3. settings: ENV:w2.cfg (w2setup "Use") else PROGDIR:w2.cfg, else the defaults (w2cfg.h)
 *   4. screen: amigagfx_open with the configured backend / resolution, the fallbacks logged
 *   5. audio: Paula (audio.device) or AHI, never on MorphOS Paula
 *   6. game_init(), then the loop: input (+ the PROGDIR:autoinput.txt script) -> game_event,
 *      game_tick at the fixed game_speed rate, game_draw + blit, WaitTOF
 *   7. ESC (raw key 0x45, when the game did not consume it) or the script's "quit" ends it
 *
 * The game hooks (game_init/tick/draw/event/shutdown/wants_quit, declared in w2cfg.h) live in other src/
 * files. Until one of them defines game_init, build.sh does NOT pass -DW2_HAVE_GAME and the stub
 * versions at the end of this file run instead: a palette test pattern, a moving box, input echo.
 *
 * Rules (!instrukcje 10-68k-2D.md): no Amiga headers here (they live in the C files of native/), no floats, no
 * sprintf, no argv (libnix hands out a broken one), PROGDIR: paths only. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "amiga_gfx.h"
#include "amiga_audio.h"
#include "amiga_trap.h"
#include "w2_sys.h"
#include "w2cfg.h"
#include "version.h"
#ifdef W2_HAVE_GAME
#include "assets.h"            /* assets_set_ehb(): the game must know an EHB screen opened */
#endif

#define LOGFILE   "PROGDIR:w2.log"
#define AUTOINPUT "PROGDIR:autoinput.txt"
#define RAW_ESC   0x45

static unsigned long g_frames;
static int g_quit, g_audio;

/* ---------------------------------------------------------------- log */
void w2_log(const char *fmt, ...)
{
	char b[512];
	va_list a;
	va_start(a, fmt);
	vsnprintf(b, sizeof b, fmt, a);
	va_end(a);
	fputs(b, stdout);
	fputc('\n', stdout);
	fflush(stdout);
}

/* A start-up error: into the log and, since a player who started us from an icon sees no log, a requester */
static void fatal_(const char *fmt, ...)
{
	char b[300];
	va_list ap;
	va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
	w2_log("FATAL: %s", b);
	amigagfx_message(b);
}

unsigned long w2_frame_count(void) { return g_frames; }

/* ms per logic cycle by game_speed 1..5. The original (docs/RE_NOTES.md) runs 16.7/18.75/21.4/25/30/
 * 37.5/50 cycles/s for its speeds 0..6, default 37.5. Ours: 1 = 21.4, 2 = 30, 3 = 37.5 (default),
 * 4 = 50 (original fastest), 5 = 62.5 (beyond the original). */
unsigned long w2_tick_ms(void)
{
	static const unsigned char ms[5] = { 47, 33, 27, 20, 16 };
	int s = w2_cfg.game_speed;
	return ms[(s < 1 ? 1 : s > 5 ? 5 : s) - 1];
}

/* ---------------------------------------------------------------- tiny 5x7 font (our own, for the shell) */
static const unsigned char g_font[][7] = {
	{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
	{0x1F,0x02,0x04,0x02,0x01,0x11,0x0E}, {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
	{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
	{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},                                                                   /* 0-9 */
	{0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
	{0x1C,0x12,0x11,0x11,0x11,0x12,0x1C}, {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
	{0x0E,0x11,0x10,0x17,0x11,0x11,0x0F}, {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
	{0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
	{0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, {0x11,0x11,0x19,0x15,0x13,0x11,0x11}, {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
	{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
	{0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
	{0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, {0x11,0x11,0x11,0x15,0x15,0x15,0x0A}, {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
	{0x11,0x11,0x11,0x0A,0x04,0x04,0x04}, {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},                             /* A-Z */
	{0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}, {0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00}, {0x00,0x00,0x00,0x1F,0x00,0x00,0x00},
	{0x00,0x01,0x02,0x04,0x08,0x10,0x00}, {0x00,0x00,0x00,0x00,0x00,0x00,0x00}                              /* . : - / space */
};

static int glyph(int c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'z') c -= 32;
	if (c >= 'A' && c <= 'Z') return 10 + c - 'A';
	switch (c) { case '.': return 36; case ':': return 37; case '-': return 38; case '/': return 39; default: return 40; }
}

/* text at x,y in colour col, each font pixel scale x scale; clipped to w x h. Returns the width drawn. */
static int draw_text(unsigned char *buf, int pitch, int w, int h, int x, int y, int scale, int col, const char *s)
{
	int x0 = x;
	for (; *s; s++, x += 6 * scale) {
		const unsigned char *g = g_font[glyph((unsigned char)*s)];
		int r, c, sy, sx;
		for (r = 0; r < 7; r++)
			for (c = 0; c < 5; c++)
				if (g[r] & (0x10 >> c))
					for (sy = 0; sy < scale; sy++)
						for (sx = 0; sx < scale; sx++) {
							int px = x + c * scale + sx, py = y + r * scale + sy;
							if (px >= 0 && py >= 0 && px < w && py < h) buf[(long)py * pitch + px] = (unsigned char)col;
						}
	}
	return x - x0;
}

static void fill(unsigned char *buf, int pitch, int x, int y, int w, int h, int col)
{
	for (; h > 0; h--, y++) memset(buf + (long)y * pitch + x, col, (size_t)w);
}

/* ---------------------------------------------------------------- test palette
 * 256 colours: 0 black, 1-15 primaries, 16-231 a 6x6x6 cube, 232-255 a grey ramp (255 white).
 * EHB: 64 colours, 0-31 settable, 32-63 = their hardware halves (the rule amigagfx_set_ehb_palette wants). */
static unsigned char g_pal[768];
static int g_ncol = 256, g_white = 255, g_black = 0, g_grey = 244;

static void make_palette(int ehb)
{
	static const unsigned char prim[16][3] = {
		{0,0,0},{255,255,255},{255,0,0},{0,255,0},{0,0,255},{255,255,0},{0,255,255},{255,0,255},
		{255,128,0},{128,64,0},{0,128,0},{0,0,128},{128,0,128},{255,160,160},{160,200,255},{200,200,200}
	};
	int i;
	memset(g_pal, 0, sizeof g_pal);
	if (ehb) {
		for (i = 0; i < 16; i++) memcpy(g_pal + 3 * i, prim[i], 3);
		for (i = 16; i < 32; i++) { int v = (i - 16) * 17; g_pal[3 * i] = (unsigned char)v; g_pal[3 * i + 1] = (unsigned char)v; g_pal[3 * i + 2] = (unsigned char)v; }
		for (i = 32; i < 64; i++) { g_pal[3 * i] = g_pal[3 * (i - 32)] >> 1; g_pal[3 * i + 1] = g_pal[3 * (i - 32) + 1] >> 1; g_pal[3 * i + 2] = g_pal[3 * (i - 32) + 2] >> 1; }
		g_ncol = 64; g_white = 1; g_black = 0; g_grey = 24;
		return;
	}
	for (i = 0; i < 16; i++) memcpy(g_pal + 3 * i, prim[i], 3);
	for (i = 0; i < 216; i++) {
		g_pal[3 * (16 + i)]     = (unsigned char)((i / 36) * 51);
		g_pal[3 * (16 + i) + 1] = (unsigned char)(((i / 6) % 6) * 51);
		g_pal[3 * (16 + i) + 2] = (unsigned char)((i % 6) * 51);
	}
	for (i = 0; i < 24; i++) { int v = 8 + i * 247 / 23; g_pal[3 * (232 + i)] = (unsigned char)v; g_pal[3 * (232 + i) + 1] = (unsigned char)v; g_pal[3 * (232 + i) + 2] = (unsigned char)v; }
	g_ncol = 256; g_white = 255; g_black = 0; g_grey = 244;
}

/* ---------------------------------------------------------------- scripted input
 * PROGDIR:autoinput.txt drives the program from inside the guest (never synthesise host input). One action
 * per line, "<ms since the loop started> <action> [args]", lines starting with # ignored:
 *   key <rawkey>          a raw key press (decimal or 0x..), followed by its release
 *   click <x> <y>         left button down + up at x,y      rclick <x> <y>   the same with the right button
 *   move <x> <y>          pointer move                      down/up <x> <y>  one half of a left click
 *   snap <file.ppm>       amigagfx_dump_screen into that file (slow; PROGDIR: path)
 *   log <text>            a line into the log
 *   quit                  end the program cleanly (headless tests) */
#define AIN_MAX 256
typedef struct { unsigned long ms; char act[8]; int a, b; char arg[64]; } AutoIn;
static AutoIn g_ain[AIN_MAX];
static int g_nain, g_ain_pos;
static AmigaGfxEvent g_ain_pending;     /* the second half of a click / key */
static int g_ain_has_pending;
static unsigned long g_ain_t0;

static void autoinput_load(void)
{
	FILE *f = fopen(AUTOINPUT, "r");
	char line[128];
	if (!f) return;
	while (g_nain < AIN_MAX && fgets(line, sizeof line, f)) {
		AutoIn *a = &g_ain[g_nain];
		char *p = line, *q;
		if (*p == '#' || *p < ' ') continue;
		a->ms = strtoul(p, &q, 10);
		if (q == p) continue;
		p = q; while (*p == ' ' || *p == '\t') p++;
		for (q = a->act; *p > ' ' && q < a->act + 7; ) *q++ = *p++;
		*q = 0;
		while (*p == ' ' || *p == '\t') p++;
		snprintf(a->arg, sizeof a->arg, "%s", p);
		for (q = a->arg; *q; q++) if (*q == '\n' || *q == '\r') *q = 0;
		a->a = (int)strtol(p, &q, 0); a->b = (int)strtol(q, NULL, 0);
		g_nain++;
	}
	fclose(f);
	g_ain_t0 = amigagfx_millis();
	w2_log("autoinput: %d actions from %s", g_nain, AUTOINPUT);
}

/* amigagfx_poll plus the script */
static int poll_ev(AmigaGfxEvent *e)
{
	if (g_ain_has_pending) { *e = g_ain_pending; g_ain_has_pending = 0; return 1; }
	while (g_ain_pos < g_nain && amigagfx_millis() - g_ain_t0 >= g_ain[g_ain_pos].ms) {
		const AutoIn *a = &g_ain[g_ain_pos++];
		memset(e, 0, sizeof *e);
		e->x = a->a; e->y = a->b;
		w2_log("autoinput: %lu ms: %s %s", amigagfx_millis() - g_ain_t0, a->act, a->arg);
		if (!strcmp(a->act, "snap")) { w2_log("snap %s: %s", a->arg, amigagfx_dump_screen(a->arg) == 0 ? "ok" : "FAILED"); continue; }
		if (!strcmp(a->act, "log")) continue;
		if (!strcmp(a->act, "quit")) { g_quit = 1; continue; }
		if (!strcmp(a->act, "key")) {
			e->type = AMIGAGFX_EV_KEY; e->code = a->a & 0x7F;
			g_ain_pending = *e; g_ain_pending.code |= 0x80; g_ain_has_pending = 1;
			return 1;
		}
		if (!strcmp(a->act, "move")) { e->type = AMIGAGFX_EV_MOUSEMOVE; return 1; }
		if (!strcmp(a->act, "down") || !strcmp(a->act, "up")) {
			e->type = a->act[0] == 'd' ? AMIGAGFX_EV_MOUSEDOWN : AMIGAGFX_EV_MOUSEUP; e->code = AMIGAGFX_BUTTON_LEFT; return 1;
		}
		if (!strcmp(a->act, "click") || !strcmp(a->act, "rclick")) {
			e->type = AMIGAGFX_EV_MOUSEDOWN; e->code = a->act[0] == 'r' ? AMIGAGFX_BUTTON_RIGHT : AMIGAGFX_BUTTON_LEFT;
			g_ain_pending = *e; g_ain_pending.type = AMIGAGFX_EV_MOUSEUP; g_ain_has_pending = 1;
			return 1;
		}
		w2_log("autoinput: unknown action \"%s\"", a->act);
	}
	return amigagfx_poll(e);
}

/* ---------------------------------------------------------------- frame counter (show_fps=1) */
static void draw_fps(int fps10)
{
	unsigned char *buf = amigagfx_chunky();
	int pitch = amigagfx_pitch(), w = amigagfx_game_width(), h = amigagfx_game_height();
	int scale = w >= 640 ? 2 : 1, bw = 9 * 6 * scale + 4, bh = 7 * scale + 4;
	char t[16];
	if (bw > w || bh > h) return;
	snprintf(t, sizeof t, "FPS %d.%d", fps10 / 10, fps10 % 10);
	fill(buf, pitch, 0, 0, bw, bh, g_black);
	draw_text(buf, pitch, w, h, 2, 2, scale, g_white, t);
	amigagfx_blit(0, 0, bw, bh);
}

/* ---------------------------------------------------------------- display */
static int open_display(void)
{
	int want = AMIGAGFX_BACKEND_AGA, w = w2cfg_screen_w(&w2_cfg), h = w2cfg_screen_h(&w2_cfg), got;
	int aga = w2_has_aga();
	switch (w2_cfg.backend) {
	case W2_BACKEND_AGA: want = AMIGAGFX_BACKEND_AGA; break;
	case W2_BACKEND_RTG: want = AMIGAGFX_BACKEND_RTG; break;
	case W2_BACKEND_EHB: want = AMIGAGFX_BACKEND_EHB; break;
	case W2_BACKEND_WB:  want = AMIGAGFX_BACKEND_WB;  break;
	default:                                     /* auto: a graphics card, else the chipset */
		if (amigagfx_is_morphos() || amigagfx_rtg_has_mode(w, h)) want = AMIGAGFX_BACKEND_RTG;
		else want = aga ? AMIGAGFX_BACKEND_AGA : AMIGAGFX_BACKEND_EHB;
		break;
	}
	if (want == AMIGAGFX_BACKEND_EHB && w > 320) {   /* EHB is a lores mode (6 planes, OCS/ECS) */
		w2_log("display: EHB is 320x240 only - resolution lores");
		w = 320; h = 240;
	}
	if (want == AMIGAGFX_BACKEND_EHB) w2_cfg.resolution = W2_RES_LORES;   /* the game's art follows (also after a fallback to AGA) */
	w2_log("display: chipset %s, asking for %s %dx%d (backend=%s)", aga ? "AGA" : "OCS/ECS or none",
	       want == AMIGAGFX_BACKEND_RTG ? "RTG" : want == AMIGAGFX_BACKEND_EHB ? "EHB" : want == AMIGAGFX_BACKEND_WB ? "WB window" : "AGA",
	       w, h, w2cfg_backend_name(w2_cfg.backend));
	make_palette(want == AMIGAGFX_BACKEND_EHB);
	if (want == AMIGAGFX_BACKEND_EHB) amigagfx_set_ehb_palette(g_pal);
	amigagfx_set_screen_title(W2_NAME " " W2_VERSION);
	amigagfx_set_rtg_depth(w2_cfg.rtg_depth);
	if (amigagfx_open(w, h, w2_cfg.titlebar, want)) {
		if (want == AMIGAGFX_BACKEND_AGA) return -1;
		w2_log("display: %dx%d refused - trying AGA", w, h);
		make_palette(0);
		if (amigagfx_open(w, h, w2_cfg.titlebar, AMIGAGFX_BACKEND_AGA)) return -1;
	}
	got = amigagfx_backend();
	if (got != want) {
		w2_log("display: fell back from %s to %s", want == AMIGAGFX_BACKEND_RTG ? "RTG" : want == AMIGAGFX_BACKEND_EHB ? "EHB" : "WB",
		       got == AMIGAGFX_BACKEND_RTG ? "RTG" : got == AMIGAGFX_BACKEND_EHB ? "EHB" : got == AMIGAGFX_BACKEND_WB ? "WB" : "AGA");
		if (got != AMIGAGFX_BACKEND_EHB && g_ncol == 64) make_palette(0);
	}
	w2_log("display: %s %dx%d, pitch %d%s", got == AMIGAGFX_BACKEND_RTG ? "RTG" : got == AMIGAGFX_BACKEND_EHB ? "EHB" : got == AMIGAGFX_BACKEND_WB ? "WB window" : "AGA",
	       amigagfx_game_width(), amigagfx_game_height(), amigagfx_pitch(), amigagfx_is_morphos() ? " (MorphOS)" : "");
	/* an EHB screen has 32 colour registers; 32..63 are the hardware's half-brite copies */
	amigagfx_set_palette(g_pal, 0, got == AMIGAGFX_BACKEND_EHB ? 32 : g_ncol);
#ifdef W2_HAVE_GAME
	/* the game converts its art to the 64 EHB colours and uploads its own palettes from here on;
	 * 0 / 19 are black / the bright pointer colour in every EHB palette (FPS overlay) */
	assets_set_ehb(got == AMIGAGFX_BACKEND_EHB);
	if (got == AMIGAGFX_BACKEND_EHB) { g_black = 0; g_white = 19; g_grey = 18; }
#endif
	return 0;
}

/* ---------------------------------------------------------------- entry (called by native/amiga_stack.c on a 2 MB stack) */
int w2_main(void)
{
	const char *cfgpath;
	int rc = 0, fps10 = 0, nfps = 0;
	unsigned long t_tick, t_fps, f_fps;

	remove(LOGFILE);                                   /* freopen("w") does not truncate here */
	freopen(LOGFILE, "w", stdout);
	w2_log(W2_NAME " %s built: %s %s", W2_VERSION, __DATE__, __TIME__);
	w2_log("system: exec %lu.%lu, 680%02d, %s%s", w2_kickstart() >> 16, w2_kickstart() & 0xFFFF, w2_cpu(),
	       w2_has_aga() ? "AGA" : "no AGA", amigagfx_is_morphos() ? ", MorphOS" : "");
	if (amiga_trap_arm()) {                            /* a CPU exception anywhere below lands here */
		char b[600];
		amiga_trap_describe(b, sizeof b);
		w2_log("TRAP\n%s", b);
		amigagfx_close();
		if (g_audio) AmigaAudio_Close();
		return 20;
	}

	cfgpath = w2cfg_load_default(&w2_cfg);
	w2_log("config: %s", cfgpath ? cfgpath : "none - defaults");
	w2_log("config: backend=%s resolution=%s rtg_depth=%d titlebar=%d audio=%s music=%d sfx_volume=%d music_volume=%d",
	       w2cfg_backend_name(w2_cfg.backend), w2_cfg.resolution == W2_RES_LORES ? "lores" : "hires", w2_cfg.rtg_depth,
	       w2_cfg.titlebar, w2cfg_audio_name(w2_cfg.audio), w2_cfg.music, w2_cfg.sfx_volume, w2_cfg.music_volume);
	w2_log("config: game_speed=%d logic_skip=%d scroll_speed=%d show_fps=%d data_dir=%s",
	       w2_cfg.game_speed, w2_cfg.logic_skip, w2_cfg.scroll_speed, w2_cfg.show_fps, w2_cfg.data_dir);

	if (open_display()) { fatal_("Cannot open a %dx%d screen.", w2cfg_screen_w(&w2_cfg), w2cfg_screen_h(&w2_cfg)); return 20; }

	if (w2_cfg.audio != W2_AUDIO_OFF) {
		AmigaAudio_PreferAHI(w2_cfg.audio == W2_AUDIO_AHI || amigagfx_is_morphos());
		g_audio = AmigaAudio_Open();
		w2_log("audio: %s", !g_audio ? "not available - silent" : AmigaAudio_IsAHI() ? "ahi.device" : "audio.device (Paula)");
	} else w2_log("audio: off");

	if (game_init()) { fatal_("The game could not start - see w2.log."); amigagfx_close(); if (g_audio) AmigaAudio_Close(); return 20; }
	w2_log("game_init ok, tick %lu ms", w2_tick_ms());
	autoinput_load();

	t_tick = t_fps = amigagfx_millis();
	f_fps = 0;
	while (!g_quit) {
		AmigaGfxEvent ev;
		unsigned long now;
		int n;
		while (!g_quit && poll_ev(&ev)) {
			if (ev.type == AMIGAGFX_EV_QUIT) { w2_log("quit: window closed"); g_quit = 1; break; }
			if (!game_event(&ev) && ev.type == AMIGAGFX_EV_KEY && ev.code == RAW_ESC) { w2_log("quit: ESC"); g_quit = 1; }
		}
		if (g_quit || game_wants_quit()) break;
		now = amigagfx_millis();
		for (n = 0; now - t_tick >= w2_tick_ms() && n < 5; n++) { game_tick(); t_tick += w2_tick_ms(); }
		if (now - t_tick >= w2_tick_ms()) t_tick = now;                 /* too far behind: drop the backlog */
		if (game_draw(amigagfx_chunky(), amigagfx_pitch(), amigagfx_game_width(), amigagfx_game_height()))
			amigagfx_blit(0, 0, amigagfx_game_width(), amigagfx_game_height());
		g_frames++;
		if (now - t_fps >= 2000) {
			fps10 = (int)((g_frames - f_fps) * 10000UL / (now - t_fps));
			if (++nfps % 5 == 1) w2_log("frames %lu, %d.%d fps", g_frames, fps10 / 10, fps10 % 10);
			t_fps = now; f_fps = g_frames;
		}
		if (w2_cfg.show_fps) draw_fps(fps10);
		if (g_audio) AmigaAudio_MusicService();
		w2_wait_tof();
	}
	w2_log("exit after %lu frames", g_frames);
	game_shutdown();
	amigagfx_close();
	if (g_audio) AmigaAudio_Close();
	amiga_trap_disarm();
	w2_log("bye");
	return rc;
}

#ifndef W2_HAVE_GAME
/* ================================================================ stub game (test pattern)
 * Compiled only while no src/ file defines game_init (see build.sh). Static pattern drawn once - a border,
 * the version line, every palette colour as a grid, a grey ramp - then per frame only a box bouncing in
 * its own strip (dirty-rectangle blits). Clicks leave a marker; every event is logged. */
static int s_drawn, s_bx, s_dx = 4, s_by, s_obx = -1, s_strip_y, s_strip_h, s_box;

int game_init(void)
{
	w2_log("game: stub test pattern (no game files yet)");
	return 0;
}

void game_tick(void)
{
	int w = amigagfx_game_width();
	s_bx += s_dx;
	if (s_bx < 1) { s_bx = 1; s_dx = -s_dx; }
	if (s_bx + s_box > w - 1) { s_bx = w - 1 - s_box; s_dx = -s_dx; }
}

static void stub_static(unsigned char *buf, int pitch, int w, int h)
{
	int scale = w >= 640 ? 2 : 1, cols = 16, rows = g_ncol / 16, cw, ch, gx, gy, i, y;
	char t[64];
	fill(buf, pitch, 0, 0, w, h, g_black);
	fill(buf, pitch, 0, 0, w, 1, g_white); fill(buf, pitch, 0, h - 1, w, 1, g_white);
	fill(buf, pitch, 0, 0, 1, h, g_white); fill(buf, pitch, w - 1, 0, 1, h, g_white);
	snprintf(t, sizeof t, "AMIW2 68K %s", W2_VERSION);
	draw_text(buf, pitch, w, h, 8 * scale, 14 * scale, scale, g_white, t);
	snprintf(t, sizeof t, "%s %dX%d %d COLOURS", amigagfx_backend() == AMIGAGFX_BACKEND_RTG ? "RTG" : amigagfx_backend() == AMIGAGFX_BACKEND_EHB ? "EHB" :
	         amigagfx_backend() == AMIGAGFX_BACKEND_WB ? "WINDOW" : "AGA", w, h, g_ncol);
	draw_text(buf, pitch, w, h, 8 * scale, 24 * scale, scale, g_grey, t);
	/* the palette grid */
	s_box = 12 * scale;
	gy = 36 * scale; ch = (h - gy - 36 * scale) / rows; cw = (w - 16 * scale) / cols;
	if (ch > cw) ch = cw;
	gx = (w - cw * cols) / 2;
	for (i = 0; i < g_ncol; i++) fill(buf, pitch, gx + (i % cols) * cw + 1, gy + (i / cols) * ch + 1, cw - 2, ch - 2, i);
	/* the strip the box runs in, then a grey ramp under it */
	s_strip_y = gy + rows * ch + 4 * scale; s_strip_h = s_box;
	s_by = s_strip_y;
	y = s_strip_y + s_strip_h + 4 * scale;
	if (y + 6 * scale < h - 1)
		for (i = 1; i < w - 1; i++) fill(buf, pitch, i, y, 1, 6 * scale, g_ncol == 64 ? 16 + (i * 16) / w : 232 + (i * 24) / w);
}

int game_draw(unsigned char *buf, int pitch, int w, int h)
{
	if (!s_drawn) { stub_static(buf, pitch, w, h); s_drawn = 1; s_obx = -1; return 1; }
	if (s_obx == s_bx) return 0;
	if (s_obx >= 0) fill(buf, pitch, s_obx, s_by, s_box, s_box, g_black);
	fill(buf, pitch, s_bx, s_by, s_box, s_box, g_ncol == 64 ? 5 : 226);   /* yellow */
	{
		int x0 = s_obx < 0 ? s_bx : s_obx < s_bx ? s_obx : s_bx, x1 = (s_obx > s_bx ? s_obx : s_bx) + s_box;
		amigagfx_blit(x0, s_by, x1 - x0, s_box);
	}
	s_obx = s_bx;
	return 0;
}

int game_event(const AmigaGfxEvent *ev)
{
	switch (ev->type) {
	case AMIGAGFX_EV_KEY:
		w2_log("game: key 0x%02x %s", ev->code & 0x7F, ev->code & 0x80 ? "up" : "down");
		break;
	case AMIGAGFX_EV_MOUSEDOWN: {
		unsigned char *buf = amigagfx_chunky();
		int p = amigagfx_pitch(), x = ev->x - 3, y = ev->y - 3;
		w2_log("game: %s button at %d,%d", ev->code == AMIGAGFX_BUTTON_RIGHT ? "right" : "left", ev->x, ev->y);
		if (x >= 0 && y >= 0 && x + 7 <= amigagfx_game_width() && y + 7 <= amigagfx_game_height()) {
			fill(buf, p, x, y, 7, 7, ev->code == AMIGAGFX_BUTTON_RIGHT ? 4 : 2);
			amigagfx_blit(x, y, 7, 7);
		}
		break;
	}
	default: break;
	}
	return 0;                                   /* nothing consumed: ESC still quits */
}

void game_shutdown(void) { w2_log("game: shutdown"); }
int game_wants_quit(void) { return 0; }
#endif
