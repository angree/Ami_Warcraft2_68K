/* menu.c - front-end menus and end screens (see menu.h and the [menu.c] block of w2.h).
 *
 * Art: title War Data 299 (+pal 300); main menu War Resources 13 (background with the title, pal 14);
 * sub-screens War Resources 15 (background without title, pal 16 == pal 14) with the panel
 * War Resources 11; widgets War Resources 0 (human) / 1 (orc), raw "gfu" frames:
 *   224x28 buttons 15 disabled / 16 normal / 17 pressed, 164x28 12/13/14, 106x28 9/10/11,
 *   checkbox 24 off / 26 on, arrows up 29/30 down 32/33 left 35/36 right 38/39, slider knob 40,
 *   pull-down field 44, slider track 46.
 * Victory / defeat War Data 359-362 (+pal 363-366); logo movie War Data 430 (Smacker).
 * Text: assets_text (FONT_LARGE on buttons). Strings from War Strings / War Music 0 where the
 * original has them ("s\4S\1ingle Player Game": first char = hotkey, \4..\1 = highlight).
 *
 * All clickable regions live ONCE in g_items[] (HIRES 640x480 coordinates) and that table is used
 * both for drawing and for hit testing. Lores (320x240) halves everything at draw time and doubles
 * the mouse coordinates of incoming events.
 * Pictures that bring their own palette (title, victory/defeat) get the widgets and the text
 * through g_xmap: widget/font colour (menu palette 14) -> nearest pen of the picture's palette.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w2.h"
#include "menu.h"
#include "assets.h"
#include "pud.h"
#include "w2cfg.h"
#include "smk.h"
#include "version.h"
#include "amiga_gfx.h"
#include "w2_sys.h"
#include "snd.h"
#include "campaign.h"

#define SCR_W 640
#define SCR_H 480

/* ---------------------------------------------------------------- the region table */

enum { K_BUTTON, K_SLIDER, K_CYCLE, K_CHECK, K_LIST, K_UP, K_DOWN };
enum {
	ID_SINGLE, ID_OPTIONS, ID_EXIT,
	ID_OPT_SFX, ID_OPT_MUSICVOL, ID_OPT_SPEED, ID_OPT_SCROLL, ID_OPT_MUSIC, ID_OPT_OK, ID_OPT_CANCEL,
	ID_SET_RACE, ID_SET_RES, ID_SET_UNITS, ID_SET_OPP, ID_SET_TYPE, ID_SET_START, ID_SET_SELECT, ID_SET_CANCEL,
	ID_MAP_LIST, ID_MAP_UP, ID_MAP_DOWN, ID_MAP_OK, ID_MAP_CANCEL,
	ID_END_CONTINUE,
	ID_SP_CAMPAIGN, ID_SP_CUSTOM, ID_SP_PREV,
	ID_CA_HUMAN, ID_CA_ORC, ID_CA_PREV,
	ID_LV_LIST, ID_LV_UP, ID_LV_DOWN, ID_LV_OK, ID_LV_CANCEL,
	ID_BR_CONTINUE, ID_FIN_CONTINUE
};
enum { S_EN, S_STR, S_MUS };   /* text source: English, War Strings entry, War Music entry (0 / 47) */

typedef struct MItem {
	uint8_t scr, kind, id, src;
	int16_t x, y, w, h;        /* HIRES; the drawn widget and the hit box are the same rectangle */
	int16_t entry, idx;        /* original string (src S_STR / S_MUS) */
	const char *text;          /* English text, also the fallback */
} MItem;

#define PANEL_X 144
#define PANEL_Y 64
#define LIST_X  164
#define LIST_Y  104
#define LIST_ROW 18
#define LIST_ROWS 12
#define CYC_X   430

static const MItem g_items[] = {
	/* main menu */
	{ MENU_SCR_MAIN, K_BUTTON, ID_SINGLE,  S_STR, 208, 240, 224, 28,  4, 1, "Single Player Game" },
	{ MENU_SCR_MAIN, K_BUTTON, ID_OPTIONS, S_STR, 208, 276, 224, 28, 10, 0, "Options" },
	{ MENU_SCR_MAIN, K_BUTTON, ID_EXIT,    S_STR, 208, 312, 224, 28, 14, 1, "Exit Program" },
	/* options (panel 11 at PANEL_X,PANEL_Y); slider label is drawn 18 px above the track */
	{ MENU_SCR_OPTIONS, K_SLIDER, ID_OPT_SFX,      S_EN,  170, 124, 300, 18,  0, 0, "Sound Volume" },
	{ MENU_SCR_OPTIONS, K_SLIDER, ID_OPT_MUSICVOL, S_EN,  170, 170, 300, 18,  0, 0, "Music Volume" },
	{ MENU_SCR_OPTIONS, K_SLIDER, ID_OPT_SPEED,    S_STR, 170, 216, 300, 18, 13, 4, "Game Speed" },
	{ MENU_SCR_OPTIONS, K_SLIDER, ID_OPT_SCROLL,   S_EN,  170, 262, 300, 18,  0, 0, "Scroll Speed" },
	{ MENU_SCR_OPTIONS, K_CHECK,  ID_OPT_MUSIC,    S_EN,  170, 298, 160, 20,  0, 0, "Music" },
	{ MENU_SCR_OPTIONS, K_BUTTON, ID_OPT_OK,       S_STR, 170, 364, 106, 28, 11, 1, "OK" },
	{ MENU_SCR_OPTIONS, K_BUTTON, ID_OPT_CANCEL,   S_STR, 364, 364, 106, 28, 11, 2, "Cancel" },
	/* custom game setup: label left of the pull-down field */
	{ MENU_SCR_SETUP, K_CYCLE,  ID_SET_RACE,   S_MUS, CYC_X,  72, 172, 19, 0,  5, "Your Race:" },
	{ MENU_SCR_SETUP, K_CYCLE,  ID_SET_RES,    S_MUS, CYC_X, 104, 172, 19, 0,  7, "Resources:" },
	{ MENU_SCR_SETUP, K_CYCLE,  ID_SET_UNITS,  S_MUS, CYC_X, 136, 172, 19, 0,  9, "Units:" },
	{ MENU_SCR_SETUP, K_CYCLE,  ID_SET_OPP,    S_MUS, CYC_X, 168, 172, 19, 0,  6, "Opponents:" },
	{ MENU_SCR_SETUP, K_CYCLE,  ID_SET_TYPE,   S_EN,  CYC_X, 200, 172, 19, 0,  0, "Game Type:" },
	{ MENU_SCR_SETUP, K_BUTTON, ID_SET_START,  S_MUS, 380, 296, 224, 28, 0,  1, "Start Game" },
	{ MENU_SCR_SETUP, K_BUTTON, ID_SET_SELECT, S_MUS, 380, 332, 224, 28, 0,  2, "Select Scenario" },
	{ MENU_SCR_SETUP, K_BUTTON, ID_SET_CANCEL, S_MUS, 380, 368, 224, 28, 0,  3, "Cancel Game" },
	/* select scenario (panel 11) */
	{ MENU_SCR_MAPS, K_LIST,   ID_MAP_LIST,   S_EN,  LIST_X, LIST_Y, 280, LIST_ROW * LIST_ROWS, 0, 0, "" },
	{ MENU_SCR_MAPS, K_UP,     ID_MAP_UP,     S_EN,  452, LIST_Y, 19, 20, 0, 0, "" },
	{ MENU_SCR_MAPS, K_DOWN,   ID_MAP_DOWN,   S_EN,  452, LIST_Y + LIST_ROW * LIST_ROWS - 20, 19, 20, 0, 0, "" },
	{ MENU_SCR_MAPS, K_BUTTON, ID_MAP_OK,     S_STR, 170, 364, 106, 28, 11, 1, "OK" },
	{ MENU_SCR_MAPS, K_BUTTON, ID_MAP_CANCEL, S_STR, 364, 364, 106, 28, 11, 2, "Cancel" },
	/* end screen */
	{ MENU_SCR_END, K_BUTTON, ID_END_CONTINUE, S_STR, 208, 430, 224, 28, 22, 1, "Continue" },
	/* single player (War Music 47 "SinglePlayer": New Campaign, Load Game, Custom Scenario, Previous Menu;
	 * there are no saved games yet, so no "Load Game") */
	{ MENU_SCR_SINGLE, K_BUTTON, ID_SP_CAMPAIGN, S_MUS, 208, 240, 224, 28, 47, 1, "New Campaign" },
	{ MENU_SCR_SINGLE, K_BUTTON, ID_SP_CUSTOM,   S_MUS, 208, 276, 224, 28, 47, 3, "Custom Scenario" },
	{ MENU_SCR_SINGLE, K_BUTTON, ID_SP_PREV,     S_MUS, 208, 312, 224, 28, 47, 4, "Previous Menu" },
	/* campaign race (War Strings 95 "NewCampaign") */
	{ MENU_SCR_CAMPAIGN, K_BUTTON, ID_CA_HUMAN, S_STR, 208, 240, 224, 28, 95, 1, "Human Campaign" },
	{ MENU_SCR_CAMPAIGN, K_BUTTON, ID_CA_ORC,   S_STR, 208, 276, 224, 28, 95, 2, "Orc Campaign" },
	{ MENU_SCR_CAMPAIGN, K_BUTTON, ID_CA_PREV,  S_STR, 208, 312, 224, 28, 95, 3, "Previous Menu" },
	/* missions reached (panel 11) */
	{ MENU_SCR_LEVELS, K_LIST,   ID_LV_LIST,   S_EN,  LIST_X, LIST_Y, 280, LIST_ROW * LIST_ROWS, 0, 0, "" },
	{ MENU_SCR_LEVELS, K_UP,     ID_LV_UP,     S_EN,  452, LIST_Y, 19, 20, 0, 0, "" },
	{ MENU_SCR_LEVELS, K_DOWN,   ID_LV_DOWN,   S_EN,  452, LIST_Y + LIST_ROW * LIST_ROWS - 20, 19, 20, 0, 0, "" },
	{ MENU_SCR_LEVELS, K_BUTTON, ID_LV_OK,     S_STR, 170, 364, 106, 28, 11, 1, "OK" },
	{ MENU_SCR_LEVELS, K_BUTTON, ID_LV_CANCEL, S_STR, 364, 364, 106, 28, 11, 2, "Cancel" },
	/* briefing ("Human_Dispatch" / "Orc_Dispatch": Continue) and the campaign's end */
	{ MENU_SCR_BRIEF, K_BUTTON, ID_BR_CONTINUE,  S_STR, 440, 440, 164, 28, 55, 1, "Continue" },
	{ MENU_SCR_FINAL, K_BUTTON, ID_FIN_CONTINUE, S_STR, 440, 440, 164, 28, 55, 1, "Continue" },
};
#define NITEMS ((int)(sizeof g_items / sizeof g_items[0]))

/* ---------------------------------------------------------------- state */

static int g_scr = MENU_SCR_NONE;
static int g_redraw = 1;          /* something visible changed since the last menu_draw */
static int g_first = 1, g_quit, g_movies = 1;
static int g_focus = -1, g_press = -1;
static int g_mx, g_my;                  /* last mouse position, HIRES */
static const char *g_err;               /* error line on the setup screen */
static unsigned long g_ticks;
static int g_click_snd = -1;

/* options working copy */
static int o_sfx, o_mvol, o_speed, o_scroll, o_music;
/* custom game choices */
static int s_race, s_res, s_units, s_opp, s_type;
static int g_game_type;

/* maps */
#define MAX_MAPS 64
typedef struct MapEnt {
	int16_t builtin;          /* War Data entry or 0 */
	uint8_t players, era;
	uint16_t w, h;
	char name[40];
	char path[96];
	char line[64];
} MapEnt;
static MapEnt g_maps[MAX_MAPS];
static int g_nmaps, g_maps_built, g_msel, g_lsel, g_ltop;

/* campaign screens */
static int g_crace;                                  /* race chosen on the campaign screen */
static unsigned long g_scr_t0;                       /* millis when the current screen was entered */
static int g_br_off, g_br_voice, g_br_music, g_br_th;   /* briefing: scroll px, voice part, music on, text height */
#define WR_MAX 80
#define WR_LEN 96
static char g_wr[WR_MAX][WR_LEN];                    /* wrapped briefing / final text */
static int g_wr_n;
static char g_ob[8][WR_LEN];                         /* wrapped objectives */
static int g_ob_n;
#define BR_X 64          /* briefing text area (HIRES) */
#define BR_Y 72
#define BR_W 290
#define BR_H 360
#define OB_X 380         /* objectives box (the dark box of the briefing pictures) */
#define OB_Y 308
#define OB_W 236

/* end screen */
typedef struct EndRow { uint8_t player, local; int32_t units, buildings, gold, wood, oil, kills; } EndRow;
static EndRow g_end[8];
static int g_nend, g_result, g_wset;

/* ---------------------------------------------------------------- small helpers */

static int lo(void) { return assets_lores() ? 1 : 0; }
static int bw(void) { return SCR_W >> lo(); }
static int bh(void) { return SCR_H >> lo(); }
static void clip_all(W2Clip *c) { c->x0 = 0; c->y0 = 0; c->x1 = bw(); c->y1 = bh(); }

static void click(void)
{
	if (g_click_snd < 0) {
		int i;
		g_click_snd = 0xFF;
		for (i = 0; i < w2_num_sounds; i++)
			if (w2_sounds[i].name && !strcmp(w2_sounds[i].name, "click")) { g_click_snd = i; break; }
	}
	if (g_click_snd != 0xFF) snd_play(g_click_snd, -1, -1);
}

/* Original menu string -> display text: "s\4S\1ingle" -> "Single". The first char is the hotkey
 * when the string carries a \4 highlight or starts with a control char. */
static const char *clean_str(const char *raw, const char *fallback, int *hotkey)
{
	static char buf[4][72];
	static int k;
	char *o = buf[k = (k + 1) & 3];
	const char *s = raw;
	int n = 0;
	if (hotkey) *hotkey = 0;
	if (!s || !*s) return fallback;
	if (strchr(s, 4) || (unsigned char)s[0] < 0x20) {
		if (hotkey && s[0] >= 'a' && s[0] <= 'z') *hotkey = s[0];
		s++;
	}
	for (; *s && n < 71; s++) if ((unsigned char)*s >= 0x20) o[n++] = *s;
	o[n] = 0;
	return n ? o : fallback;
}

/* War Music string tables (same layout as War Strings tables): entry 0 "Custom Game Setup",
 * entry 47 "SinglePlayer" */
static unsigned char *g_mus[2]; static unsigned long g_mus_n[2]; static int g_mus_tried[2];
static const char *mus_string(int entry, int idx)
{
	unsigned n, off;
	int k = entry == 47 ? 1 : 0;
	unsigned char *d;
	unsigned long dn;
	if (!g_mus_tried[k]) { g_mus_tried[k] = 1; g_mus[k] = assets_load(ASSET_SPEECH, k ? 47 : 0, &g_mus_n[k]); }
	d = g_mus[k]; dn = g_mus_n[k];
	if (!d || dn < 4) return "";
	n = W2_LE16(d);
	/* idx + 1 < n: the last offset is the end of the table */
	if (idx < 0 || (unsigned)idx >= n || 4 + (unsigned long)idx * 2 > dn) return "";
	off = W2_LE16(d + 2 + idx * 2);
	if (off >= dn) return "";
	/* make sure the string ends inside the entry */
	if (!memchr(d + off, 0, dn - off)) return "";
	return (const char *)d + off;
}

static const char *str_of(int src, int entry, int idx, const char *fallback, int *hotkey)
{
	if (src == S_STR) return clean_str(assets_string(entry, idx), fallback, hotkey);
	if (src == S_MUS) return clean_str(mus_string(entry, idx), fallback, hotkey);
	if (hotkey) *hotkey = 0;
	return fallback;
}
static const char *item_text(const MItem *m, int *hotkey) { return str_of(m->src, m->entry, m->idx, m->text, hotkey); }

/* ---------------------------------------------------------------- palette remap */

static unsigned char g_ref[768]; static int g_ref_ok;   /* menu palette (War Resources 14), 8-bit */
static unsigned char g_xmap[256];                       /* menu-art pen -> pen of the current palette */
static int g_xid;                                       /* 1: current palette is the menu palette */

static int nearest_pen(const unsigned char *pal, int r, int g, int b)
{
	int i, best = 0; long bd = 0x7fffffffL;
	for (i = 0; i < 256; i++) {
		long dr, dg, db, d;
		if (i >= 17 && i <= 19 && !assets_ehb()) continue;   /* hardware pointer pens (EHB: shared, real colours) */
		dr = pal[i * 3] - r; dg = pal[i * 3 + 1] - g; db = pal[i * 3 + 2] - b;
		d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
		if (d < bd) { bd = d; best = i; if (!d) break; }
	}
	return best;
}

static void build_xmap(void)
{
	const unsigned char *pal = assets_palette(), *base = assets_base_remap();
	int i;
	if (!g_ref_ok) {
		unsigned long n; unsigned char *p = assets_load(ASSET_RESOURCES, 14, &n);
		if (p && n >= 768) {
			for (i = 0; i < 768; i++) { int v = p[i] & 63; g_ref[i] = (unsigned char)((v << 2) | (v >> 4)); }
			g_ref_ok = 1;
		}
		war_free(p);
	}
	g_xid = 1;
	if (assets_ehb()) {
		/* EHB: the screen shows 64 other colours - every menu-art pen goes by RGB; text is drawn
		 * directly (assets_text picks its colours from the current 64) */
		const unsigned char *sm = assets_src_map();
		for (i = 0; i < 256; i++)
			g_xmap[i] = g_ref_ok ? (unsigned char)nearest_pen(pal, g_ref[i * 3], g_ref[i * 3 + 1], g_ref[i * 3 + 2]) : sm[i];
		return;
	}
	for (i = 0; i < 256; i++) {
		if (!g_ref_ok || !memcmp(g_ref + i * 3, pal + i * 3, 3)) g_xmap[i] = base[i];
		else { g_xmap[i] = (unsigned char)nearest_pen(pal, g_ref[i * 3], g_ref[i * 3 + 1], g_ref[i * 3 + 2]); g_xid = 0; }
	}
}

/* a menu-palette colour by RGB in the current palette */
static int pen_rgb(int r, int g, int b) { return nearest_pen(assets_palette(), r, g, b); }

/* ---------------------------------------------------------------- drawing primitives */

static void fill(int x, int y, int w, int h, int col)
{
	int s = lo(), x0 = x >> s, y0 = y >> s, x1 = (x + w) >> s, y1 = (y + h) >> s, r;
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > bw()) x1 = bw();
	if (y1 > bh()) y1 = bh();
	if (x0 >= x1 || !view.buf) return;
	for (r = y0; r < y1; r++) memset(view.buf + (long)r * view.pitch + x0, col, (size_t)(x1 - x0));
}

static void frame_rect(int x, int y, int w, int h, int col)
{
	int t = 1 << lo();
	fill(x, y, w, t, col); fill(x, y + h - t, w, t, col);
	fill(x, y, t, h, col); fill(x + w - t, y, t, h, col);
}

/* text in menu-palette colours; on a foreign palette it goes through g_xmap via a scratch strip */
#define TS_W 640
#define TS_H 64
#define TS_KEY 255
static unsigned char g_ts[TS_W * TS_H];
static int g_cy0 = 0, g_cy1 = SCR_H;   /* vertical text clip, HIRES (scrolling briefing text) */

static int txt(int font, int col, int x, int y, const char *s)
{
	W2Clip c;
	int sh = lo(), rows, adv, r, cx, w, x0, y0;
	if (!view.buf || !s || !*s) return 0;
	clip_all(&c);
	if (c.y0 < (g_cy0 >> sh)) c.y0 = g_cy0 >> sh;
	if (c.y1 > (g_cy1 >> sh)) c.y1 = g_cy1 >> sh;
	if (c.y0 >= c.y1) return 0;
	if (g_xid) return assets_text(font, col, view.buf, view.pitch, &c, x, y, s);
	rows = ((assets_font_height(font) + 4) >> sh);
	if (rows > TS_H) rows = TS_H;
	memset(g_ts, TS_KEY, (size_t)TS_W * rows);
	c.x0 = 0; c.y0 = 0; c.x1 = TS_W >> sh; c.y1 = rows;
	adv = assets_text(font, col, g_ts, TS_W, &c, 0, 0, s);
	w = (adv >> sh) + 2;
	if (w > (TS_W >> sh)) w = TS_W >> sh;
	x0 = x >> sh; y0 = y >> sh;
	for (r = 0; r < rows; r++) {
		unsigned char *d; const unsigned char *src = g_ts + r * TS_W;
		if (y0 + r < 0 || y0 + r >= bh() || y0 + r < (g_cy0 >> sh) || y0 + r >= (g_cy1 >> sh)) continue;
		d = view.buf + (long)(y0 + r) * view.pitch;
		for (cx = 0; cx < w; cx++) {
			int dx = x0 + cx;
			if (src[cx] == TS_KEY || dx < 0 || dx >= bw()) continue;
			d[dx] = g_xmap[src[cx]];
		}
	}
	return adv;
}

static int txt_c(int font, int col, int cx, int y, const char *s)
{
	return txt(font, col, cx - assets_text_width(font, s) / 2, y, s);
}

/* ---- widgets (raw gfu frames of War Resources 0/1; pen 0 = transparent) */
static unsigned char *g_wid[2]; static unsigned long g_wid_n[2]; static int g_wid_ok[2];

static void wdraw(int f, int x, int y)
{
	const unsigned char *d; unsigned long n; W2Frame fr; int sh = lo(), r, c, w, h;
	int set = g_wset & 1;
	if (!g_wid[set]) {
		int cnt;
		g_wid[set] = assets_load(ASSET_RESOURCES, set, &g_wid_n[set]);
		g_wid_ok[set] = g_wid[set] && w2_sprite_probe(g_wid[set], g_wid_n[set], &cnt, NULL, NULL) == W2_SPR_RAW;
		if (!g_wid_ok[set]) w2_log("menu: War Resources %d is not the widget sheet", set);
	}
	if (!g_wid_ok[set] || !view.buf) return;
	d = g_wid[set]; n = g_wid_n[set];
	if (w2_sprite_frame(d, n, f, &fr) || fr.offset + (unsigned long)fr.w * fr.h > n) return;
	w = fr.w >> sh; h = fr.h >> sh;
	x >>= sh; y >>= sh;
	for (r = 0; r < h; r++) {
		const unsigned char *s = d + fr.offset + (unsigned long)(r << sh) * fr.w;
		unsigned char *o;
		if (y + r < 0 || y + r >= bh()) continue;
		o = view.buf + (long)(y + r) * view.pitch;
		for (c = 0; c < w; c++) {
			unsigned char p = s[c << sh];
			if (p && x + c >= 0 && x + c < bw()) o[x + c] = g_xmap[p];
		}
	}
}

/* ---- background cache: picture (+ panel) for the current screen, already remapped & halved */
static unsigned char *g_bg; static long g_bg_key = -1;

static void bg_put(const unsigned char *img, unsigned long n, int px, int py)
{
	int w, h, sh = lo(), r, c, W = bw(), H = bh();
	const unsigned char *pix = w2_image_probe(img, n, &w, &h), *base = assets_src_map();
	if (!pix) return;
	px >>= sh; py >>= sh;
	for (r = 0; r < (h >> sh); r++) {
		const unsigned char *s = pix + (long)(r << sh) * w;
		unsigned char *o;
		if (py + r < 0 || py + r >= H) continue;
		o = g_bg + (long)(py + r) * W;
		for (c = 0; c < (w >> sh); c++) {
			int x = px + c;
			if (x >= 0 && x < W) o[x] = base[s[c << sh]];
		}
	}
}

static int bg_load(int which, int entry, int panel)
{
	long key = ((long)which << 24) | ((long)entry << 12) | panel | ((long)lo() << 30) | ((long)assets_ehb() << 29);
	unsigned long n; unsigned char *img;
	if (key == g_bg_key && g_bg) return 0;
	if (!g_bg) g_bg = (unsigned char *)malloc((size_t)SCR_W * SCR_H);
	if (!g_bg) return -1;
	memset(g_bg, 0, (size_t)bw() * bh());
	img = assets_load(which, entry, &n);
	if (img) bg_put(img, n, 0, 0);
	else w2_log("menu: picture %d/%d missing", which, entry);
	war_free(img);
	if (panel > 0) {
		img = assets_load(ASSET_RESOURCES, panel, &n);
		if (img) bg_put(img, n, PANEL_X, PANEL_Y);
		war_free(img);
	}
	g_bg_key = key;
	return 0;
}

static void bg_draw(void)
{
	int r, W = bw();
	if (!view.buf) return;
	if (!g_bg) { for (r = 0; r < bh(); r++) memset(view.buf + (long)r * view.pitch, 0, (size_t)W); return; }
	for (r = 0; r < bh(); r++) memcpy(view.buf + (long)r * view.pitch, g_bg + (long)r * W, (size_t)W);
}

/* darken a rectangle of the cached background (stats table on the victory picture) */
static void bg_darken(int x, int y, int w, int h)
{
	static unsigned char dk[256];
	const unsigned char *pal = assets_palette();
	int i, r, c, sh = lo(), W = bw();
	if (!g_bg) return;
	for (i = 0; i < 256; i++) dk[i] = (unsigned char)nearest_pen(pal, pal[i * 3] / 3, pal[i * 3 + 1] / 3, pal[i * 3 + 2] / 3);
	x >>= sh; y >>= sh; w >>= sh; h >>= sh;
	for (r = y; r < y + h && r < bh(); r++)
		for (c = x; c < x + w && c < W; c++) g_bg[(long)r * W + c] = dk[g_bg[(long)r * W + c]];
	g_bg_key = -1;   /* modified: rebuild next time */
}

/* ---------------------------------------------------------------- maps */

static void map_describe(MapEnt *m, const unsigned char *d, unsigned long n)
{
	unsigned long len; const unsigned char *s; int w = 0, h = 0, i, pl = 0;
	char name[40];
	pud_describe(d, (uint32_t)n, name, (int)sizeof name, &w, &h);
	for (i = 0; name[i]; i++) if ((unsigned char)name[i] < 0x20) name[i] = ' ';
	if (name[0]) snprintf(m->name, sizeof m->name, "%s", name);
	m->w = (uint16_t)w; m->h = (uint16_t)h;
	s = w2_pud_section(d, n, "OWNR", &len);
	for (i = 0; s && i < 8 && (unsigned long)i < len; i++) if (s[i] == 4 || s[i] == 5 || s[i] == 1) pl++;
	m->players = (uint8_t)pl;
	s = w2_pud_section(d, n, "ERAX", &len);
	if (!s) s = w2_pud_section(d, n, "ERA ", &len);
	m->era = (uint8_t)((s && len >= 2) ? W2_LE16(s) : 0);
	snprintf(m->line, sizeof m->line, "%d players, %dx%d", pl, w, h);
}

static unsigned char *map_load(const MapEnt *m, unsigned long *n)
{
	FILE *f; long sz; unsigned char *b;
	*n = 0;
	if (m->builtin) return assets_load(ASSET_DATA, m->builtin, n);
	f = fopen(m->path, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
	if (sz <= 0 || sz > 512L * 1024) { fclose(f); return NULL; }
	b = (unsigned char *)malloc((size_t)sz);
	if (b && fread(b, 1, (size_t)sz, f) != (size_t)sz) { free(b); b = NULL; }
	fclose(f);
	*n = b ? (unsigned long)sz : 0;
	return b;
}
static void map_free(const MapEnt *m, unsigned char *b) { if (m->builtin) war_free(b); else free(b); }

static int strcasecmp_ascii(const char *a, const char *b)
{
	for (;; a++, b++) {
		int x = (unsigned char)*a, y = (unsigned char)*b;
		if (x >= 'A' && x <= 'Z') x += 32;
		if (y >= 'A' && y <= 'Z') y += 32;
		if (x != y || !x) return x - y;
	}
}

static void map_dir_add(const char *nm, void *ud)   /* ud = the directory, "xxx/" or "DEV:" form */
{
	size_t l = strlen(nm);
	MapEnt *m;
	unsigned long n; unsigned char *d;
	if (g_nmaps >= MAX_MAPS) return;
	if (l < 5 || l > 80 || strcasecmp_ascii(nm + l - 4, ".pud")) return;
	m = &g_maps[g_nmaps];
	memset(m, 0, sizeof *m);
	snprintf(m->path, sizeof m->path, "%s%.80s", (const char *)ud, nm);
	snprintf(m->name, sizeof m->name, "%.*s", (int)(l - 4 < 39 ? l - 4 : 39), nm);
	d = map_load(m, &n);
	if (!d || !w2_pud_section(d, n, "DIM ", NULL)) { map_free(m, d); return; }
	map_describe(m, d, n);
	map_free(m, d);
	g_nmaps++;
}

static void maps_build(void)
{
	int e;
	if (g_maps_built) return;
	g_maps_built = 1;
	g_nmaps = 0;
	for (e = PUD_BUILTIN_SKIRMISH_FIRST; e <= PUD_BUILTIN_SKIRMISH_LAST && g_nmaps < MAX_MAPS; e++) {
		MapEnt *m = &g_maps[g_nmaps];
		unsigned long n; unsigned char *d = assets_load(ASSET_DATA, e, &n);
		if (!d) continue;
		memset(m, 0, sizeof *m);
		m->builtin = (int16_t)e;
		snprintf(m->name, sizeof m->name, "Map %d", e);
		map_describe(m, d, n);
		war_free(d);
		g_nmaps++;
	}
	{   /* the custom maps of the player's CD (w2conv: <data_dir>/pud) and the player's own (PROGDIR:maps) */
		static char pd[160];
		const char *dd = w2_cfg.data_dir[0] ? w2_cfg.data_dir : "PROGDIR:data";
		size_t l = strlen(dd);
		snprintf(pd, sizeof pd, "%s%spud", dd, (l && (dd[l - 1] == ':' || dd[l - 1] == '/')) ? "" : "/");
		{ static char pds[164]; snprintf(pds, sizeof pds, "%s/", pd); w2_dir_list(pd, map_dir_add, pds); }
		w2_dir_list("PROGDIR:maps", map_dir_add, (void *)"PROGDIR:maps/");
	}
	w2_log("menu: %d maps for the custom game", g_nmaps);
	if (g_msel >= g_nmaps) g_msel = 0;
}

int menu_map_count(void) { maps_build(); return g_nmaps; }
const char *menu_map_name(int i)
{
	static char b[112];
	if (i < 0 || i >= g_nmaps) return "";
	snprintf(b, sizeof b, "%s  (%s)%s", g_maps[i].name, g_maps[i].line, g_maps[i].builtin ? "" : " [file]");
	return b;
}

/* ---- minimap preview of the selected map: tile classes in menu-palette colours (cheap) */
#define PREV 128
static unsigned char g_prev[PREV * PREV];
static int g_prev_w, g_prev_h, g_prev_for = -1;

enum { TC_WATER, TC_DWATER, TC_COAST, TC_GROUND, TC_DGROUND, TC_FOREST, TC_ROCK, TC_WALL, TC_GOLD, TC_START, TC_N };
static const unsigned char g_tc_rgb[3][TC_N][3] = {
	{ {40,64,148},{28,44,112},{156,124,72},{72,112,40},{52,88,30},{20,64,20},{96,92,88},{150,150,150},{252,224,40},{252,40,40} },
	{ {40,64,148},{28,44,112},{150,150,170},{224,224,236},{192,196,212},{36,80,64},{96,96,110},{150,150,150},{252,224,40},{252,40,40} },
	{ {40,64,148},{28,44,112},{124,92,60},{132,100,60},{104,76,44},{72,60,28},{92,80,64},{150,150,150},{252,224,40},{252,40,40} },
};

static int tile_class(unsigned v)
{
	int solid = v < 0x100, k = solid ? (int)((v >> 4) & 15) : (int)((v >> 8) & 15);
	if (solid) {
		switch (k) {
		case 1: return TC_WATER; case 2: return TC_DWATER; case 3: case 4: return TC_COAST;
		case 5: return TC_GROUND; case 6: return TC_DGROUND; case 7: return TC_FOREST; case 8: return TC_ROCK;
		default: return TC_WALL;
		}
	}
	switch (k) {
	case 1: return TC_WATER; case 2: case 3: return TC_COAST; case 4: return TC_ROCK;
	case 5: case 6: return TC_GROUND; case 7: return TC_FOREST; default: return TC_WALL;
	}
}

static void preview_build(void)
{
	MapEnt *m; unsigned long n, len; unsigned char *d; const unsigned char *s;
	int pen[TC_N], i, era, sc, x, y, w, h;
	g_prev_w = g_prev_h = 0;
	g_prev_for = g_msel;
	if (g_msel < 0 || g_msel >= g_nmaps) return;
	m = &g_maps[g_msel];
	d = map_load(m, &n);
	if (!d) return;
	era = m->era; if (era == 3) era = 2; if (era > 2) era = 0;
	for (i = 0; i < TC_N; i++) pen[i] = pen_rgb(g_tc_rgb[era][i][0], g_tc_rgb[era][i][1], g_tc_rgb[era][i][2]);
	w = m->w; h = m->h;
	s = w2_pud_section(d, n, "MTXM", &len);
	if (s && w > 0 && h > 0 && w <= 128 && h <= 128 && len >= (unsigned long)w * h * 2) {
		sc = PREV / (w > h ? w : h);
		if (sc < 1) sc = 1;
		g_prev_w = w * sc; g_prev_h = h * sc;
		for (y = 0; y < h; y++)
			for (x = 0; x < w; x++) {
				int c = pen[tile_class(W2_LE16(s + (y * w + x) * 2))], yy;
				for (yy = 0; yy < sc; yy++) memset(g_prev + (y * sc + yy) * PREV + x * sc, c, (size_t)sc);
			}
		/* gold mines and start locations */
		s = w2_pud_section(d, n, "UNIT", &len);
		for (i = 0; s && (unsigned long)(i + 1) * 8 <= len; i++) {
			const unsigned char *u = s + i * 8;
			int ux = (int)W2_LE16(u), uy = (int)W2_LE16(u + 2), t = u[4], sz, c, yy, xx;
			if (t == 0x5C) { c = pen[TC_GOLD]; sz = 3; }
			else if (t == 0x5E || t == 0x5F) { c = pen[TC_START]; sz = 2; }
			else continue;
			for (yy = 0; yy < sz * sc; yy++)
				for (xx = 0; xx < sz * sc; xx++) {
					int px = ux * sc + xx, py = uy * sc + yy;
					if (px < g_prev_w && py < g_prev_h) g_prev[py * PREV + px] = (unsigned char)c;
				}
		}
	}
	map_free(m, d);
}

/* ---------------------------------------------------------------- movies (War Data 430 logo,
 * War Movies cut-scenes of the campaign) */

static SmkVideo g_smk; static unsigned char *g_smk_data; static int g_smk_on;
static unsigned long g_smk_t0;
static int g_mov_arch = ASSET_DATA, g_mov_entry = 430, g_mov_next = MENU_SCR_TITLE;

static void movie_stop(void)
{
	if (g_smk_on) smk_close(&g_smk);
	g_smk_on = 0;
	war_free(g_smk_data); g_smk_data = NULL;
}

static int movie_start(void)
{
	unsigned long n;
	g_smk_data = assets_war(g_mov_arch) ? assets_load(g_mov_arch, g_mov_entry, &n) : NULL;
	if (!g_smk_data || smk_open(&g_smk, g_smk_data, (uint32_t)n) != 0) {
		w2_log("menu: movie %d/%d not available", g_mov_arch, g_mov_entry);
		war_free(g_smk_data); g_smk_data = NULL;
		return -1;
	}
	g_smk_on = 1;
	g_smk_t0 = amigagfx_millis();
	w2_log("menu: movie %d/%d %dx%d, %d frames", g_mov_arch, g_mov_entry, g_smk.w, g_smk.h, g_smk.frames);
	return 0;
}

static void go(int scr);

static void movie_draw(void)
{
	unsigned long el = amigagfx_millis() - g_smk_t0;
	long due = g_smk.frame_us ? (long)((el * 1000UL) / g_smk.frame_us) : 0;
	int k = 0, sh = lo(), s = 2 >> sh, yd, ow, oh, x0, y0, r, c, W = bw(), H = bh();
	while (g_smk.cur < due && g_smk.cur < g_smk.frames - 1 && k < 4) {
		if (smk_next_frame(&g_smk, SMK_DECODE_VIDEO)) break;
		if (g_smk.palette_changed) amigagfx_set_palette(g_smk.palette, 0, 256);
		k++;
	}
	if (g_smk.cur < 0) { if (smk_next_frame(&g_smk, SMK_DECODE_VIDEO) == 0) amigagfx_set_palette(g_smk.palette, 0, 256); }
	if (due >= g_smk.frames + 15) { movie_stop(); go(g_mov_next); return; }   /* hold the last frame ~1 s */
	for (r = 0; r < H; r++) memset(view.buf + (long)r * view.pitch, 0, (size_t)W);
	if (g_smk.cur < 0 || !g_smk.pixels) return;
	yd = (g_smk.flags & (SMK_FLAG_Y_DOUBLE | SMK_FLAG_Y_INTERLACE)) ? 2 : 1;
	ow = g_smk.w * s; oh = g_smk.h * yd * s;
	if (oh > H) oh = H;
	if (ow > W) ow = W;
	x0 = (W - ow) / 2; y0 = (H - oh) / 2;
	for (r = 0; r < oh; r++) {
		const unsigned char *src = g_smk.pixels + (long)(r / (yd * s)) * g_smk.w;
		unsigned char *o = view.buf + (long)(y0 + r) * view.pitch + x0;
		if (s == 2) for (c = 0; c < ow; c++) o[c] = src[c >> 1];
		else memcpy(o, src, (size_t)ow);
	}
}

/* ---------------------------------------------------------------- campaign text */

/* word-wrap 't' (lines separated by '\n') to 'width' HIRES pixels of 'font' into out[max][WR_LEN] */
static int wrap_text(int font, const char *t, int width, char (*out)[WR_LEN], int max)
{
	int n = 0;
	if (!t) return 0;
	while (*t && n < max) {
		int len = 0, brk = -1;
		char *o = out[n];
		while (t[len] && t[len] != '\n' && len < WR_LEN - 1) {
			o[len] = t[len];
			o[len + 1] = 0;
			if (t[len] == ' ') brk = len;
			if (assets_text_width(font, o) > width && brk > 0) { len = brk; break; }
			len++;
		}
		o[len] = 0;
		while (len > 0 && o[len - 1] == ' ') o[--len] = 0;
		n++;
		t += len;
		while (*t == ' ') t++;
		if (*t == '\n') t++;
	}
	return n;
}

static int line_h(int font) { return assets_font_height(font) + 3; }

/* entering the briefing / the campaign's end: wrap the texts, start the first voice part */
static void brief_enter(int scr)
{
	g_br_off = 0; g_br_voice = 0; g_br_music = 0;
	if (scr == MENU_SCR_BRIEF) {
		g_wr_n = wrap_text(FONT_GAME, camp_briefing(), BR_W, g_wr, WR_MAX);
		g_ob_n = wrap_text(FONT_GAME, camp_objectives(), OB_W, g_ob, 8);
		if (camp_voice_count() > 0) camp_speech_play(camp_voice_entry(0));
		w2_log("menu: briefing %s mission %d '%s': %d text lines, %d objective lines, %d voice parts",
		       camp_race() ? "orc" : "human", camp_mission(), camp_title(), g_wr_n, g_ob_n, camp_voice_count());
	} else {
		g_wr_n = wrap_text(FONT_GAME, camp_final_text(), 512, g_wr, WR_MAX);
		g_ob_n = 0;
		camp_speech_play(camp_final_voice());
		w2_log("menu: campaign end text %d lines", g_wr_n);
	}
	g_br_th = g_wr_n * line_h(FONT_GAME);
}

/* ---------------------------------------------------------------- screens */

static const int g_era_pal[3] = { 2, 18, 10 };

static int focusable(const MItem *m) { return m->kind != K_LIST && m->kind != K_UP && m->kind != K_DOWN; }

static void screen_setup(int scr)
{
	/* the art shown under the screen's palette (assets_load_palette_for: picture, then the rest) */
	short art[8];
	int na = 0;
#define ART(w, e) (art[na++] = (short)(w), art[na++] = (short)(e), art[na] = -1)
	switch (scr) {
	case MENU_SCR_TITLE:
		ART(ASSET_DATA, 299);
		assets_load_palette_for(ASSET_DATA, 300, art); bg_load(ASSET_DATA, 299, 0); break;
	case MENU_SCR_MAIN: case MENU_SCR_SINGLE: case MENU_SCR_CAMPAIGN:
		ART(ASSET_RESOURCES, 13); ART(ASSET_RESOURCES, g_wset & 1); ART(ASSET_RESOURCES, 11);   /* same palette as the sub-screens */
		assets_load_palette_for(ASSET_RESOURCES, 14, art); bg_load(ASSET_RESOURCES, 13, 0); break;
	case MENU_SCR_OPTIONS: case MENU_SCR_MAPS: case MENU_SCR_LEVELS:
		ART(ASSET_RESOURCES, 15); ART(ASSET_RESOURCES, g_wset & 1); ART(ASSET_RESOURCES, 11);
		assets_load_palette_for(ASSET_RESOURCES, 16, art); bg_load(ASSET_RESOURCES, 15, 11); break;
	case MENU_SCR_SETUP:
		ART(ASSET_RESOURCES, 15); ART(ASSET_RESOURCES, g_wset & 1); ART(ASSET_RESOURCES, 11);
		assets_load_palette_for(ASSET_RESOURCES, 16, art); bg_load(ASSET_RESOURCES, 15, 0); break;
	case MENU_SCR_END: {
		int off = (w2.player[w2.local_player & 7].race ? 1 : 0) + (g_result == 2 ? 2 : 0);
		ART(ASSET_DATA, 359 + off); ART(ASSET_RESOURCES, g_wset & 1);
		assets_load_palette_for(ASSET_DATA, 363 + off, art); bg_load(ASSET_DATA, 359 + off, 0);
		break;
	}
	case MENU_SCR_ACT: {        /* act map: War Resources 19..26, palette War Resources 17 */
		int pic = camp_act_picture();
		ART(ASSET_RESOURCES, pic); ART(ASSET_RESOURCES, g_wset & 1);
		assets_load_palette_for(ASSET_RESOURCES, 17, art); bg_load(ASSET_RESOURCES, pic, 0);
		break;
	}
	case MENU_SCR_BRIEF: {      /* War Data 369..378 with palette 367 / 368 */
		int pal, pic = camp_brief_picture(&pal);
		ART(ASSET_DATA, pic); ART(ASSET_RESOURCES, g_wset & 1);
		assets_load_palette_for(ASSET_DATA, pal, art); bg_load(ASSET_DATA, pic, 0);
		break;
	}
	case MENU_SCR_FINAL: {      /* War Resources 29 / 30 */
		int pal, pic = camp_final_picture(&pal);
		ART(ASSET_RESOURCES, pic); ART(ASSET_RESOURCES, g_wset & 1);
		assets_load_palette_for(ASSET_RESOURCES, pal, art); bg_load(ASSET_RESOURCES, pic, 0);
		break;
	}
	default: return;
	}
#undef ART
	assets_apply_palette();
	build_xmap();
}

static void go(int scr)
{
	int i;
	if (g_scr == MENU_SCR_MOVIE && scr != MENU_SCR_MOVIE) movie_stop();
	g_scr = scr;
	g_redraw = 1;
	g_press = -1;
	g_focus = -1;
	for (i = 0; i < NITEMS; i++) if (g_items[i].scr == scr && focusable(&g_items[i])) { g_focus = i; break; }
	if (scr == MENU_SCR_ACT || scr == MENU_SCR_BRIEF || scr == MENU_SCR_FINAL) g_wset = camp_race();
	else if (scr == MENU_SCR_LEVELS) g_wset = g_crace;
	else if (scr != MENU_SCR_END) g_wset = 0;
	if (scr == MENU_SCR_MOVIE) { if (movie_start()) { go(g_mov_next); return; } }
	g_scr_t0 = amigagfx_millis();
	screen_setup(scr);
	if (scr == MENU_SCR_BRIEF || scr == MENU_SCR_FINAL) brief_enter(scr);
	if (scr == MENU_SCR_LEVELS) {
		g_lsel = camp_progress(g_crace) - 1;
		g_ltop = g_lsel - LIST_ROWS + 1; if (g_ltop < 0) g_ltop = 0;
	}
	if (scr == MENU_SCR_SETUP) {
		maps_build();
		if (g_prev_for != g_msel) preview_build();
	}
	if (scr == MENU_SCR_END && g_nend) bg_darken(40, 128, 560, 34 + g_nend * 24);
	w2_log("menu: screen %d", scr);
}

/* ---------------------------------------------------------------- values */

static int opp_max(void)
{
	int p = (g_msel >= 0 && g_msel < g_nmaps) ? g_maps[g_msel].players : 8;
	if (p < 2) p = 2;
	return p - 1;
}

static void slider_range(int id, int **v, int *mn, int *mx, int *step)
{
	*step = 1; *mn = 1; *mx = 5;
	switch (id) {
	case ID_OPT_SFX: *v = &o_sfx; *mn = 0; *mx = 100; *step = 10; break;
	case ID_OPT_MUSICVOL: *v = &o_mvol; *mn = 0; *mx = 100; *step = 10; break;
	case ID_OPT_SPEED: *v = &o_speed; break;
	default: *v = &o_scroll; break;
	}
}

static void slider_add(int id, int d)
{
	int *v, mn, mx, st;
	slider_range(id, &v, &mn, &mx, &st);
	*v += d * st;
	if (*v < mn) *v = mn;
	if (*v > mx) *v = mx;
}

/* knob track: x+20 .. x+w-20-17 */
static void slider_set_x(const MItem *m, int x)
{
	int *v, mn, mx, st, span = m->w - 40 - 17, p;
	slider_range(m->id, &v, &mn, &mx, &st);
	p = x - (m->x + 20) - 8;
	if (p < 0) p = 0;
	if (p > span) p = span;
	*v = mn + ((p * (mx - mn) + span / 2) / span);
	*v = mn + ((*v - mn + st / 2) / st) * st;
	if (*v > mx) *v = mx;
}

static int *cycle_var(int id, int *count)
{
	switch (id) {
	case ID_SET_RACE: *count = 3; return &s_race;
	case ID_SET_RES: *count = 4; return &s_res;
	case ID_SET_UNITS: *count = 2; return &s_units;
	case ID_SET_OPP: *count = opp_max() + 1; return &s_opp;
	default: *count = 3; return &s_type;
	}
}

static void cycle_add(int id, int d)
{
	int n, *v = cycle_var(id, &n);
	*v = (*v + d + n) % n;
}

static const char *cycle_text(int id)
{
	static char b[32];
	int n, v = *cycle_var(id, &n);
	if (v >= n) v = 0;
	switch (id) {
	case ID_SET_RACE:
		return v == 0 ? str_of(S_STR, 46, 10, "Map Default", NULL) : v == 1 ? str_of(S_STR, 46, 19, "Human", NULL) : str_of(S_STR, 46, 20, "Orc", NULL);
	case ID_SET_RES:
		if (v == 0) return str_of(S_STR, 46, 10, "Map Default", NULL);
		return str_of(S_STR, 46, 10 + v, v == 1 ? "Low" : v == 2 ? "Medium" : "High", NULL);
	case ID_SET_UNITS:
		return v == 0 ? str_of(S_STR, 46, 10, "Map Default", NULL) : str_of(S_STR, 46, 18, "One Peasant Only", NULL);
	case ID_SET_OPP:
		if (v == 0) return str_of(S_STR, 46, 10, "Map Default", NULL);
		snprintf(b, sizeof b, v == 1 ? "%d Opponent" : "%d Opponents", v);
		return str_of(S_STR, 46, 25 + v, b, NULL);
	default:
		return v == 0 ? "Use Map Settings" : v == 1 ? "Melee" : "Free For All";
	}
}

/* ---------------------------------------------------------------- lists (maps / missions) */

static int list_n(void) { return g_scr == MENU_SCR_LEVELS ? camp_progress(g_crace) : g_nmaps; }
static const char *list_name(int k)
{
	static char b[64];
	if (g_scr != MENU_SCR_LEVELS) return g_maps[k].name;
	snprintf(b, sizeof b, "%s", camp_title_of(g_crace, k + 1));
	if (!b[0]) snprintf(b, sizeof b, "Mission %d", k + 1);
	return b;
}

/* ---------------------------------------------------------------- campaign flow */

static void camp_after_act(int fresh)
{
	int m = camp_movie_before();
	if (fresh && m >= 0 && g_movies && !assets_ehb() && assets_war(ASSET_MOVIES)) {
		g_mov_arch = ASSET_MOVIES; g_mov_entry = m; g_mov_next = MENU_SCR_BRIEF;
		go(MENU_SCR_MOVIE);
		return;
	}
	go(MENU_SCR_BRIEF);
}

/* before a mission (Mac FUN_00142874 + f9_149C): act title, act movie, briefing. A retry after a
 * defeat goes straight to the briefing (the Mac remembers the act / movie it showed last). */
static void camp_flow(int fresh)
{
	if (fresh && camp_act_picture() >= 0) { go(MENU_SCR_ACT); return; }
	camp_after_act(fresh);
}

static void camp_final(void)
{
	int m = camp_movie_final();
	if (m >= 0 && g_movies && !assets_ehb() && assets_war(ASSET_MOVIES)) {
		g_mov_arch = ASSET_MOVIES; g_mov_entry = m; g_mov_next = MENU_SCR_FINAL;
		go(MENU_SCR_MOVIE);
		return;
	}
	go(MENU_SCR_FINAL);
}

static void to_main(void)
{
	camp_speech_stop();
	camp_stop();
	if (!snd_music_active() || snd_music_track() != 0) snd_music_play(SND_MUS_MENU, 0);
	go(MENU_SCR_MAIN);
}

static void camp_launch(void)
{
	int r;
	camp_speech_stop();
	g_err = NULL;
	r = camp_start_game();
	if (r != 0) {
		w2_log("menu: campaign mission %d could not start (%d)", camp_mission(), r);
		g_err = "Could not start the mission";
		to_main();
		return;
	}
	if (assets_era() >= 0 && assets_era() <= 2) { assets_load_palette(ASSET_DATA, g_era_pal[assets_era()]); assets_apply_palette(); }
	g_scr = MENU_SCR_NONE;
}

/* ---------------------------------------------------------------- actions */

static void start_game(void)
{
	W2Setup s;
	const MapEnt *m;
	int r;
	if (g_msel < 0 || g_msel >= g_nmaps) { g_err = "No map selected"; return; }
	m = &g_maps[g_msel];
	memset(&s, 0, sizeof s);
	s.builtin_pud = m->builtin;
	if (!m->builtin) snprintf(s.pud_path, sizeof s.pud_path, "%s", m->path);
	s.local_player = -1;
	s.local_race = s_race - 1;
	s.opponents = s_opp;
	s.resources = s_res;
	s.units = s_units;
	g_game_type = s_type;
	camp_stop();
	w2_log("menu: start '%s' (pud %d %s) race %d opp %d res %d units %d type %d", m->name, s.builtin_pud,
	       s.pud_path, s.local_race, s.opponents, s.resources, s.units, s_type);
	g_err = NULL;
	r = game_start(&s);
	if (r != 0) {
		g_err = "Could not start the map";
		w2_log("menu: game_start failed (%d)", r);
		screen_setup(MENU_SCR_SETUP);     /* the loader may have switched the palette */
		return;
	}
	/* in the game now: the era palette must be on screen (assets_set_era keeps the menu palette
	 * when the era did not change since the last game) */
	if (assets_era() >= 0 && assets_era() <= 2) { assets_load_palette(ASSET_DATA, g_era_pal[assets_era()]); assets_apply_palette(); }
	g_scr = MENU_SCR_NONE;
}

static void leave_end(void)
{
	game_state = GS_MENU;
	if (camp_active() && g_result == 1) {          /* won: the next mission, or the campaign's end */
		if (camp_after_end(1)) camp_final();
		else camp_flow(1);
		return;
	}
	if (camp_active() && g_result == 2) {          /* lost: the same mission again */
		camp_after_end(2);
		camp_flow(0);
		return;
	}
	camp_stop();
	go(MENU_SCR_MAIN);
}

static void activate(int i)
{
	const MItem *m = &g_items[i];
	click();
	w2_log("menu: activate item %d (screen %d)", m->id, g_scr);
	switch (m->id) {
	case ID_SINGLE: g_err = NULL; go(MENU_SCR_SINGLE); break;
	case ID_SP_CAMPAIGN: go(MENU_SCR_CAMPAIGN); break;
	case ID_SP_CUSTOM: g_err = NULL; camp_stop(); go(MENU_SCR_SETUP); break;
	case ID_SP_PREV: go(MENU_SCR_MAIN); break;
	case ID_CA_HUMAN: case ID_CA_ORC:
		g_crace = m->id == ID_CA_ORC;
		camp_progress_load();
		if (camp_progress(g_crace) <= 1) { camp_select(g_crace, 1); camp_flow(1); }   /* nothing to pick yet */
		else go(MENU_SCR_LEVELS);
		break;
	case ID_CA_PREV: go(MENU_SCR_SINGLE); break;
	case ID_LV_OK:
		if (g_lsel >= 0 && g_lsel < list_n()) { camp_select(g_crace, g_lsel + 1); camp_flow(1); }
		break;
	case ID_LV_CANCEL: go(MENU_SCR_CAMPAIGN); break;
	case ID_BR_CONTINUE: camp_launch(); break;
	case ID_FIN_CONTINUE: to_main(); break;
	case ID_OPTIONS:
		o_sfx = w2_cfg.sfx_volume; o_mvol = w2_cfg.music_volume; o_speed = w2_cfg.game_speed;
		o_scroll = w2_cfg.scroll_speed; o_music = w2_cfg.music;
		go(MENU_SCR_OPTIONS);
		break;
	case ID_EXIT: g_quit = 1; w2_log("menu: exit program"); break;
	case ID_OPT_MUSIC: o_music = !o_music; break;
	case ID_OPT_OK:
		w2_cfg.sfx_volume = o_sfx; w2_cfg.music_volume = o_mvol; w2_cfg.game_speed = o_speed;
		w2_cfg.scroll_speed = o_scroll; w2_cfg.music = o_music;
		if (w2cfg_save(&w2_cfg, W2CFG_FILE)) w2_log("menu: could not write %s", W2CFG_FILE);
		else w2_log("menu: options saved (sfx %d music %d/%d speed %d scroll %d)", o_sfx, o_music, o_mvol, o_speed, o_scroll);
		go(MENU_SCR_MAIN);
		break;
	case ID_OPT_CANCEL: go(MENU_SCR_MAIN); break;
	case ID_SET_RACE: case ID_SET_RES: case ID_SET_UNITS: case ID_SET_OPP: case ID_SET_TYPE: cycle_add(m->id, 1); break;
	case ID_SET_START: start_game(); break;
	case ID_SET_SELECT:
		g_lsel = g_msel; g_ltop = g_lsel - LIST_ROWS / 2; if (g_ltop < 0) g_ltop = 0;
		go(MENU_SCR_MAPS);
		break;
	case ID_SET_CANCEL: go(MENU_SCR_SINGLE); break;
	case ID_MAP_OK:
		if (g_lsel >= 0 && g_lsel < g_nmaps) g_msel = g_lsel;
		if (s_opp > opp_max()) s_opp = 0;
		g_err = NULL;
		go(MENU_SCR_SETUP);
		break;
	case ID_MAP_CANCEL: go(MENU_SCR_SETUP); break;
	case ID_END_CONTINUE: leave_end(); break;
	default: break;
	}
}

static void list_move(int d)
{
	int n = list_n();
	if (!n) return;
	g_lsel += d;
	if (g_lsel < 0) g_lsel = 0;
	if (g_lsel >= n) g_lsel = n - 1;
	if (g_lsel < g_ltop) g_ltop = g_lsel;
	if (g_lsel >= g_ltop + LIST_ROWS) g_ltop = g_lsel - LIST_ROWS + 1;
}

/* ---------------------------------------------------------------- draw */

static void draw_item(int i)
{
	const MItem *m = &g_items[i];
	int foc = (i == g_focus), hot;
	int pressed = (i == g_press && g_mx >= m->x && g_mx < m->x + m->w && g_my >= m->y && g_my < m->y + m->h);
	const char *t = item_text(m, &hot);
	switch (m->kind) {
	case K_BUTTON: {
		int f = m->w >= 224 ? 16 : m->w >= 164 ? 13 : 10, fh = assets_font_height(FONT_LARGE);
		wdraw(f + pressed, m->x, m->y);
		txt_c(FONT_LARGE, foc ? FC_WHITE : FC_YELLOW, m->x + m->w / 2 + pressed, m->y + (m->h - fh) / 2 + 1 + pressed, t);
		break;
	}
	case K_SLIDER: {
		int *v, mn, mx, st, span = m->w - 40 - 17; char b[16];
		slider_range(m->id, &v, &mn, &mx, &st);
		txt(FONT_GAME, foc ? FC_WHITE : FC_YELLOW, m->x, m->y - 17, t);
		snprintf(b, sizeof b, "%d", *v);
		txt(FONT_GAME, foc ? FC_WHITE : FC_YELLOW, m->x + m->w - assets_text_width(FONT_GAME, b), m->y - 17, b);
		wdraw(46, m->x, m->y);
		wdraw(35 + (pressed && g_mx < m->x + 20), m->x, m->y);
		wdraw(38 + (pressed && g_mx >= m->x + m->w - 20), m->x + m->w - 20, m->y);
		wdraw(40, m->x + 20 + (*v - mn) * span / (mx - mn), m->y + 1);
		break;
	}
	case K_CYCLE: {
		int fh = assets_font_height(FONT_GAME);
		txt(FONT_GAME, foc ? FC_WHITE : FC_YELLOW, m->x - 130, m->y + (m->h - fh) / 2 + 1, t);
		wdraw(44, m->x, m->y);
		txt(FONT_GAME, foc ? FC_WHITE : FC_YELLOW, m->x + 8, m->y + (m->h - fh) / 2 + 1, cycle_text(m->id));
		break;
	}
	case K_CHECK:
		wdraw(o_music ? 26 : 24, m->x, m->y + (o_music ? -1 : 1));
		txt(FONT_GAME, foc ? FC_WHITE : FC_YELLOW, m->x + 26, m->y + 3, t);
		break;
	case K_LIST: {
		int r;
		fill(m->x - 4, m->y - 2, m->w + 8, m->h + 4, g_xmap[0]);
		frame_rect(m->x - 4, m->y - 2, m->w + 8, m->h + 4, g_xmap[104]);
		for (r = 0; r < LIST_ROWS && g_ltop + r < list_n(); r++) {
			int k = g_ltop + r, y = m->y + r * LIST_ROW;
			if (k == g_lsel) fill(m->x - 2, y, m->w + 4, LIST_ROW, g_xmap[180]);
			txt(FONT_GAME, k == g_lsel ? FC_WHITE : FC_YELLOW, m->x + 2, y + 3, list_name(k));
		}
		break;
	}
	case K_UP: wdraw(29 + (i == g_press), m->x, m->y); break;
	case K_DOWN: wdraw(32 + (i == g_press), m->x, m->y); break;
	}
}

static void draw_setup_extra(void)
{
	const MapEnt *m = (g_msel >= 0 && g_msel < g_nmaps) ? &g_maps[g_msel] : NULL;
	int px = 40, py = 120, r, sh = lo();
	txt_c(FONT_LARGE, FC_YELLOW, 320, 24, str_of(S_MUS, 0, 4, "Custom Game Setup", NULL));
	txt(FONT_GAME, FC_YELLOW, px, 72, "Scenario:");
	if (m) {
		txt(FONT_GAME, FC_WHITE, px, 90, m->name);
		txt(FONT_GAME, FC_YELLOW, px, 104, m->line);
	} else txt(FONT_GAME, FC_WHITE, px, 90, "(no maps found)");
	/* preview, framed, centred in a 128x128 box */
	frame_rect(px - 2, py - 2, PREV + 4, PREV + 4, g_xmap[104]);
	fill(px, py, PREV, PREV, g_xmap[0]);
	if (g_prev_w && view.buf) {
		int ox = (px + (PREV - g_prev_w) / 2) >> sh, oy = (py + (PREV - g_prev_h) / 2) >> sh;
		for (r = 0; r < (g_prev_h >> sh); r++) {
			unsigned char *o = view.buf + (long)(oy + r) * view.pitch + ox;
			const unsigned char *s = g_prev + (r << sh) * PREV;
			int c;
			for (c = 0; c < (g_prev_w >> sh); c++) o[c] = s[c << sh];
		}
	}
	if (g_err) txt(FONT_LARGE, FC_WHITE, px, 300, g_err);
}

static void draw_end_extra(void)
{
	static const char *const def[6] = { "Units", "Buildings", "Gold", "Lumber", "Oil", "Kills" };
	static const int colx[7] = { 56, 196, 276, 366, 436, 506, 566 };
	int i, j, y = 136;
	char b[24];
	txt_c(FONT_LARGE_TITLE, g_result == 1 ? FC_YELLOW : FC_RED, 320, 40, g_result == 1 ? "Victory!" : "Defeat!");
	if (!g_nend) return;
	txt(FONT_GAME, FC_YELLOW, colx[0], y, "Player");
	for (j = 0; j < 6; j++) txt(FONT_GAME, FC_YELLOW, colx[j + 1], y, str_of(S_STR, 22, 5 + j, def[j], NULL));
	for (i = 0; i < g_nend; i++) {
		const EndRow *e = &g_end[i];
		int32_t v[6];
		y = 136 + 28 + i * 24;
		v[0] = e->units; v[1] = e->buildings; v[2] = e->gold; v[3] = e->wood; v[4] = e->oil; v[5] = e->kills;
		if (e->local) snprintf(b, sizeof b, "You (%d)", e->player + 1);
		else snprintf(b, sizeof b, "Player %d", e->player + 1);
		txt(FONT_GAME, e->local ? FC_WHITE : FC_GREY, colx[0], y, b);
		for (j = 0; j < 6; j++) {
			snprintf(b, sizeof b, "%ld", (long)v[j]);
			txt(FONT_GAME, e->local ? FC_WHITE : FC_GREY, colx[j + 1], y, b);
		}
	}
}

/* briefing / campaign end: title, scrolling text, objectives */
static void draw_brief_extra(void)
{
	int i, lh = line_h(FONT_GAME), y0 = g_scr == MENU_SCR_BRIEF ? BR_Y : 120, x0 = g_scr == MENU_SCR_BRIEF ? BR_X : 64;
	int h = g_scr == MENU_SCR_BRIEF ? BR_H : 300;
	if (g_scr == MENU_SCR_BRIEF) txt_c(FONT_LARGE, FC_WHITE, 320, 28, camp_title());
	g_cy0 = y0; g_cy1 = y0 + h;
	for (i = 0; i < g_wr_n; i++) {
		int y = y0 + i * lh - g_br_off;
		if (y + lh <= y0 || y >= y0 + h) continue;
		txt(FONT_GAME, FC_YELLOW, x0, y, g_wr[i]);
	}
	g_cy0 = 0; g_cy1 = SCR_H;
	if (g_scr != MENU_SCR_BRIEF) return;
	txt(FONT_LARGE, FC_WHITE, OB_X, OB_Y, str_of(S_STR, camp_race() ? 56 : 55, 2, "Objectives:", NULL));
	for (i = 0; i < g_ob_n; i++) txt(FONT_GAME, FC_YELLOW, OB_X, OB_Y + 24 + i * lh, g_ob[i]);
}

void menu_draw(void)
{
	int i;
	if (!view.buf) return;
	if (g_scr == MENU_SCR_NONE) return;
	g_redraw = 0;
	if (g_scr == MENU_SCR_MOVIE) { movie_draw(); return; }
	bg_draw();
	switch (g_scr) {
	case MENU_SCR_MAIN: {
		char b[48];
		snprintf(b, sizeof b, "%s %s", W2_NAME, W2_VERSION);
		txt(FONT_SMALL, FC_GREY, 12, 464, b);
		if (g_err) txt_c(FONT_GAME, FC_WHITE, 320, 440, g_err);
		break;
	}
	case MENU_SCR_OPTIONS:
		txt_c(FONT_LARGE, FC_YELLOW, 320, 78, str_of(S_STR, 10, 0, "Options", NULL));
		break;
	case MENU_SCR_SETUP: draw_setup_extra(); break;
	case MENU_SCR_MAPS: {
		const MapEnt *m = (g_lsel >= 0 && g_lsel < g_nmaps) ? &g_maps[g_lsel] : NULL;
		txt_c(FONT_LARGE, FC_YELLOW, 320, 78, str_of(S_MUS, 0, 2, "Select Scenario", NULL));
		if (m) txt(FONT_GAME, FC_YELLOW, LIST_X, LIST_Y + LIST_ROW * LIST_ROWS + 8, m->line);
		break;
	}
	case MENU_SCR_END: draw_end_extra(); break;
	case MENU_SCR_LEVELS: {
		txt_c(FONT_LARGE, FC_YELLOW, 320, 78, str_of(S_STR, 95, g_crace ? 2 : 1, g_crace ? "Orc Campaign" : "Human Campaign", NULL));
		break;
	}
	case MENU_SCR_ACT:
		txt_c(FONT_LARGE_TITLE, FC_YELLOW, 320, 190, camp_act_number());
		txt_c(FONT_LARGE_TITLE, FC_YELLOW, 320, 240, camp_act_name());
		break;
	case MENU_SCR_BRIEF: case MENU_SCR_FINAL: draw_brief_extra(); break;
	case MENU_SCR_SINGLE: case MENU_SCR_CAMPAIGN:
		if (g_err) txt_c(FONT_GAME, FC_WHITE, 320, 440, g_err);
		break;
	default: break;
	}
	for (i = 0; i < NITEMS; i++) if (g_items[i].scr == g_scr) draw_item(i);
}

/* ---------------------------------------------------------------- input */

static int hit(int x, int y)
{
	int i;
	for (i = 0; i < NITEMS; i++) {
		const MItem *m = &g_items[i];
		if (m->scr == g_scr && x >= m->x && x < m->x + m->w && y >= m->y && y < m->y + m->h) return i;
	}
	return -1;
}

static void focus_step(int d)
{
	int i, n = NITEMS, k = g_focus < 0 ? (d > 0 ? -1 : 0) : g_focus;
	for (i = 0; i < n; i++) {
		k = (k + d + n) % n;
		if (g_items[k].scr == g_scr && focusable(&g_items[k])) { g_focus = k; return; }
	}
}

static void back(void)
{
	switch (g_scr) {
	case MENU_SCR_OPTIONS: click(); go(MENU_SCR_MAIN); break;
	case MENU_SCR_SETUP: click(); go(MENU_SCR_SINGLE); break;
	case MENU_SCR_MAPS: click(); go(MENU_SCR_SETUP); break;
	case MENU_SCR_END: leave_end(); break;
	case MENU_SCR_SINGLE: click(); go(MENU_SCR_MAIN); break;
	case MENU_SCR_CAMPAIGN: click(); go(MENU_SCR_SINGLE); break;
	case MENU_SCR_LEVELS: click(); go(MENU_SCR_CAMPAIGN); break;
	case MENU_SCR_BRIEF: camp_launch(); break;
	case MENU_SCR_FINAL: to_main(); break;
	default: break;
	}
}

/* Amiga raw key -> lower-case letter (hotkeys), 0 if none */
static int raw_letter(int code)
{
	static const char r1[] = "qwertyuiop", r2[] = "asdfghjkl", r3[] = "zxcvbnm";
	if (code >= 0x10 && code < 0x10 + 10) return r1[code - 0x10];
	if (code >= 0x20 && code < 0x20 + 9) return r2[code - 0x20];
	if (code >= 0x31 && code < 0x31 + 7) return r3[code - 0x31];
	return 0;
}

static int on_key(int code)
{
	int i, letter;
	if (code & 0x80) return 1;   /* releases */
	if (g_scr == MENU_SCR_MOVIE) { movie_stop(); go(g_mov_next); return 1; }
	if (g_scr == MENU_SCR_TITLE) { go(MENU_SCR_MAIN); return 1; }
	if (g_scr == MENU_SCR_END) { leave_end(); return 1; }
	if (g_scr == MENU_SCR_ACT) { camp_after_act(1); return 1; }
	if (g_scr == MENU_SCR_BRIEF && (code == 0x45 || code == 0x44 || code == 0x43)) { camp_launch(); return 1; }   /* Esc / Return / Enter */
	if (g_scr == MENU_SCR_FINAL) { to_main(); return 1; }
	switch (code) {
	case 0x45: back(); return 1;                                            /* Esc */
	case 0x4C: if (g_scr == MENU_SCR_MAPS || g_scr == MENU_SCR_LEVELS) list_move(-1); else focus_step(-1); return 1;   /* up */
	case 0x4D: if (g_scr == MENU_SCR_MAPS || g_scr == MENU_SCR_LEVELS) list_move(1); else focus_step(1); return 1;     /* down */
	case 0x42: focus_step(1); return 1;                                     /* Tab */
	case 0x4F: case 0x4E: {                                                  /* left / right */
		int d = code == 0x4E ? 1 : -1;
		if (g_focus < 0) return 1;
		if (g_items[g_focus].kind == K_SLIDER) slider_add(g_items[g_focus].id, d);
		else if (g_items[g_focus].kind == K_CYCLE) cycle_add(g_items[g_focus].id, d);
		else focus_step(d);
		return 1;
	}
	case 0x44: case 0x43: case 0x40:                                        /* Return / Enter / Space */
		if ((g_scr == MENU_SCR_MAPS || g_scr == MENU_SCR_LEVELS) && code != 0x40) {
			int want = g_scr == MENU_SCR_MAPS ? ID_MAP_OK : ID_LV_OK;
			for (i = 0; i < NITEMS; i++) if (g_items[i].id == want) { activate(i); break; }
			return 1;
		}
		if (g_focus >= 0 && g_items[g_focus].kind != K_SLIDER) activate(g_focus);
		return 1;
	default: break;
	}
	letter = raw_letter(code);
	if (letter) {
		for (i = 0; i < NITEMS; i++) {
			int hk;
			if (g_items[i].scr != g_scr || g_items[i].kind != K_BUTTON) continue;
			item_text(&g_items[i], &hk);
			if (hk == letter) { g_focus = i; activate(i); return 1; }
		}
	}
	return 1;
}

static void on_down(int button)
{
	int i;
	if (g_scr == MENU_SCR_MOVIE) { movie_stop(); go(g_mov_next); return; }
	if (g_scr == MENU_SCR_TITLE) { go(MENU_SCR_MAIN); return; }
	if (g_scr == MENU_SCR_ACT) { camp_after_act(1); return; }
	i = hit(g_mx, g_my);
	if (g_scr == MENU_SCR_END && i < 0) { leave_end(); return; }
	if (i < 0) return;
	if (focusable(&g_items[i])) g_focus = i;
	if (button == AMIGAGFX_BUTTON_RIGHT) {
		if (g_items[i].kind == K_CYCLE) { click(); cycle_add(g_items[i].id, -1); }
		return;
	}
	g_press = i;
	switch (g_items[i].kind) {
	case K_SLIDER: {
		const MItem *m = &g_items[i];
		if (g_mx < m->x + 20) slider_add(m->id, -1);
		else if (g_mx >= m->x + m->w - 20) slider_add(m->id, 1);
		else slider_set_x(m, g_mx);
		break;
	}
	case K_LIST: {
		int k = g_ltop + (g_my - g_items[i].y) / LIST_ROW;
		if (k >= 0 && k < list_n()) g_lsel = k;
		break;
	}
	case K_UP: click(); list_move(-1); break;
	case K_DOWN: click(); list_move(1); break;
	default: break;
	}
}

static void on_up(void)
{
	int i = g_press;
	g_press = -1;
	if (i < 0 || hit(g_mx, g_my) != i) return;
	if (g_items[i].kind == K_BUTTON || g_items[i].kind == K_CYCLE || g_items[i].kind == K_CHECK) activate(i);
}

int menu_event(const void *amiga_gfx_event)
{
	const AmigaGfxEvent *ev = (const AmigaGfxEvent *)amiga_gfx_event;
	int sh;
	if (!ev || g_scr == MENU_SCR_NONE) return 0;
	sh = lo();
	switch (ev->type) {
	case AMIGAGFX_EV_KEY: g_redraw = 1; return on_key(ev->code);
	case AMIGAGFX_EV_MOUSEMOVE:
	case AMIGAGFX_EV_MOUSEDOWN:
	case AMIGAGFX_EV_MOUSEUP:
		g_mx = ev->x << sh; g_my = ev->y << sh;
		if (ev->type == AMIGAGFX_EV_MOUSEDOWN) { g_redraw = 1; on_down(ev->code); }
		else if (ev->type == AMIGAGFX_EV_MOUSEUP) { g_redraw = 1; if (ev->code == AMIGAGFX_BUTTON_LEFT) on_up(); }
		else {
			int i = hit(g_mx, g_my);
			if (g_press >= 0 && g_items[g_press].kind == K_SLIDER) {
				const MItem *m = &g_items[g_press];
				if (g_mx >= m->x + 20 && g_mx < m->x + m->w - 20) { slider_set_x(m, g_mx); g_redraw = 1; }
			} else if (g_press < 0 && i >= 0 && focusable(&g_items[i]) && g_focus != i) { g_focus = i; g_redraw = 1; }
		}
		return 1;
	default: return 0;
	}
}

/* ---------------------------------------------------------------- API */

void menu_enter(void)
{
	g_err = NULL;
	if (!snd_music_active() || snd_music_track() != 0) snd_music_play(SND_MUS_MENU, 0);   /* title song, looping */
	if (g_first) {
		g_first = 0;
		g_mov_arch = ASSET_DATA; g_mov_entry = 430; g_mov_next = MENU_SCR_TITLE;
		/* the logo movie brings a 256-colour palette per frame: not on an EHB screen */
		go(g_movies && !assets_ehb() && assets_war(ASSET_DATA) ? MENU_SCR_MOVIE : MENU_SCR_TITLE);
	} else go(MENU_SCR_MAIN);
}

void menu_tick(void)
{
	unsigned long el = amigagfx_millis() - g_scr_t0;
	g_ticks++;
	if (g_scr == MENU_SCR_ACT && el > 6000) { camp_after_act(1); return; }
	if (g_scr == MENU_SCR_BRIEF || g_scr == MENU_SCR_FINAL) {
		int h = g_scr == MENU_SCR_BRIEF ? BR_H : 300, off, mx = g_br_th - h;
		/* voice parts one after the other, then the briefing music */
		camp_speech_service();
		if (!camp_speech_active() && g_scr == MENU_SCR_BRIEF) {
			if (g_br_voice + 1 < camp_voice_count()) { g_br_voice++; camp_speech_play(camp_voice_entry(g_br_voice)); }
			else if (!g_br_music) { g_br_music = 1; snd_music_play(SND_MUS_BRIEFING, camp_race()); }
		}
		/* the text stands 3 s, then scrolls up 1 px / 80 ms until its last line is in the box */
		off = el > 3000 ? (int)((el - 3000) / 80) : 0;
		if (mx < 0) mx = 0;
		if (off > mx) off = mx;
		if (off != g_br_off) { g_br_off = off; g_redraw = 1; }
	}
}

/* 1 = menu_draw would change the screen (the logo movie always does); game_draw blits only then */
int menu_needs_redraw(void) { return g_redraw || g_scr == MENU_SCR_MOVIE; }
void menu_force_redraw(void) { g_redraw = 1; }

void endscreen_enter(int result)
{
	int p, i;
	if (result != 1 && result != 2) { game_state = GS_MENU; camp_stop(); go(MENU_SCR_MAIN); return; }
	g_result = result;
	g_nend = 0;
	snd_music_play(result == 1 ? SND_MUS_VICTORY : SND_MUS_DEFEAT, w2.player[w2.local_player].race ? 1 : 0);
	for (p = 0; p < 8; p++) {
		const W2Player *pl = &w2.player[p];
		EndRow *e;
		if (pl->control != W2_CTRL_HUMAN && pl->control != W2_CTRL_COMPUTER) continue;
		e = &g_end[g_nend];
		memset(e, 0, sizeof *e);
		e->player = (uint8_t)p; e->local = (uint8_t)(p == w2.local_player);
		e->gold = pl->gold; e->wood = pl->wood; e->oil = pl->oil;
		for (i = 0; i < w2.unit_hi && i < W2_MAX_UNITS; i++) {
			const W2Unit *u = &w2.unit[i];
			if (!(u->flags & W2_UF_ALIVE) || u->owner != p) continue;
			if (u->flags & W2_UF_BUILDING) e->buildings++; else e->units++;
			e->kills += u->kills;
		}
		g_nend++;
	}
	g_wset = w2.player[w2.local_player & 7].race ? 1 : 0;
	w2_log("menu: end screen %s, %d players", result == 1 ? "victory" : "defeat", g_nend);
	go(MENU_SCR_END);
}

int menu_wants_quit(void) { return g_quit; }
int menu_game_type(void) { return g_game_type; }
int menu_screen(void) { return g_scr; }
void menu_set_movies(int on) { g_movies = on; }
