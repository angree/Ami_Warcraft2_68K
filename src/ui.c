/* ui.c - the in-game interface (the [ui.c] block of w2.h; extras in ui.h).
 *
 * Draws the original Warcraft II 640x480 interface from the game's own art: panel pieces War Data
 * 287..298 (per race), info panel 354/355 (4 backgrounds), icons 356..358 (per era, 46x38, icon id =
 * frame), resource icons 187, the menu button widget (War Resources 0/1 frame 4/5), fonts through
 * assets_text. Layout from Wargus' human/orc ui (behaviour reference only) - see ui.h.
 *
 * Everything here is in HIRES 640x480 units. In lores (320x240) the helpers below halve positions
 * and point-sample the art; text uses the 3x5 font through assets_text. Mouse positions arrive in
 * buffer pixels and are converted (<<1) once, in ui_event.
 *
 * Input: selection (click, drag box, shift add/remove, double-click / ctrl-click = same type in
 * view), right click = cmd_smart, the 3x3 command card (click + hotkeys, build submenus, targeting
 * mode with cursor, building placement with a ghost), control groups (ctrl+1..9 / 1..9, double tap
 * centres), minimap (left = view, right = move), F10 / menu button = in-game menu, edge and arrow
 * key scrolling.
 *
 * No floats, no per-frame mallocs; all art is loaded once per race/era and kept.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w2.h"
#include "ui.h"
#include "assets.h"
#include "w2cfg.h"
#include "amiga_gfx.h"
#include "game.h"
#include "snd.h"
#include "transport.h"
#include "spells.h"
#include "unit.h"
#include "campaign.h"

/* ------------------------------------------------------------------ layout (hires units) */
#define PANEL_W      176
#define MENU_BX      24      /* "Menu (F10)" button */
#define MENU_BY      2
#define MENU_BW      128
#define MENU_BH      20
#define MINI_X       24
#define MINI_Y       26
#define MINI_S       128
#define INFO_Y       160
#define BTNP_Y       336
#define BTN_W        46
#define BTN_H        38
#define RES_X        176
#define STATUS_Y     464
#define FILL_X       624
/* original Mac layout (docs/RE_NOTES.md section 10): command buttons are 54x46 cells at x 5/61/117,
 * y 336/383/430 with the icon at +4; multi-select cells 54x53 at x 5/61/117, y 165/220/275. */
static const short g_btn_x[3] = { 9, 65, 121 };        /* icon positions (cell + 4) */
static const short g_btn_y[3] = { 340, 387, 434 };
#define CELL_W       54
#define CELL_H       46
static const short g_sel_x[3] = { 9, 65, 121 };
static const short g_sel_y[3] = { 169, 224, 279 };
#define PORTRAIT_X   9
#define PORTRAIT_Y   169
#define PROD_ICON_X  107
#define PROD_ICON_Y  238
#define STAT_Y(n)    (232 + 15 * (n))
static const short g_res_x[3] = { 264, 352, 440 };    /* gold, lumber, oil widgets (Mac 0xBDA) */
#define FOOD_X       536
/* in-game menu: the info panel background (176x176) centred on the map area */
#define GM_X         312
#define GM_Y         152
#define GM_W         176
#define GM_H         176
#define GM_N         5
static const short g_gm_by[GM_N] = { 188, 213, 238, 263, 288 };
#define GM_BX        (GM_X + 24)
/* Options page (g_menu == 2): 4 slider rows, then an OK button */
#define OPT_N        4
#define OPT_Y0       (GM_Y + 36)
#define OPT_DY       30
#define OPT_BX       (GM_X + 16)
#define OPT_BW       144
#define OPT_BH       8
#define OPT_OK_Y     (GM_Y + 148)

#define MSG_LINES    8          /* original: up to 8 lines at (186, 26 + n*line) */
#define MSG_TTL      (10 * 30)   /* cycles */
#define SCROLL_EDGE  3
#define CLICK_SOUND  20

/* raw key codes (Amiga) */
#define K_ESC   0x45
#define K_F10   0x59
#define K_UP    0x4C
#define K_DOWN  0x4D
#define K_RIGHT 0x4E
#define K_LEFT  0x4F
#define K_LSHIFT 0x60
#define K_RSHIFT 0x61
#define K_CTRL  0x63
#define K_LALT  0x64
#define K_RALT  0x65

enum { MODE_NORMAL, MODE_TARGET, MODE_PLACE };

/* ------------------------------------------------------------------ state */
static int g_mode, g_level;
static const w2_button *g_tgt;          /* targeting: the button that asked for a target */
static int g_place_type, g_place_builder;
static int g_mx = 320, g_my = 240, g_mouse_seen;
static int g_drag, g_dx0, g_dy0;        /* 0 none, 1 pressed in map, 2 box */
static int g_mm_drag;
static int g_btn_press = -1, g_menu_press, g_gm_press = -1;
static int g_shift, g_ctrl, g_alt;
static unsigned long g_click_ms; static int g_click_unit = -1;
static int g_menu, g_quit;
static char g_msg[MSG_LINES][80]; static int g_msg_ttl[MSG_LINES];
static int g_mark_x, g_mark_y, g_mark_t;
static uint16_t g_grp[10][W2_MAX_SELECT]; static uint8_t g_grp_n[10];
static int g_grp_last = -1; static unsigned long g_grp_ms;
static int g_edge_dir = -1;
static char g_status[96];
static const w2_button *g_card[9];

/* frame drawing context */
static unsigned char *B; static int P, L, SW, SH;
static W2Clip g_full, g_mapclip;

/* ------------------------------------------------------------------ colours */
enum { PEN_BLACK, PEN_LIGHT, PEN_DARK, PEN_GREEN, PEN_YELLOW, PEN_RED, PEN_BLUE, PEN_BAR,
       PEN_GGREEN, PEN_GRED, PEN_BOX, PEN_BARBG, PEN_COUNT };
static const unsigned char g_pen_rgb[PEN_COUNT][3] = {
	{ 0, 0, 0 }, { 196, 196, 196 }, { 56, 56, 56 }, { 0, 200, 0 }, { 252, 220, 0 }, { 220, 0, 0 },
	{ 40, 120, 252 }, { 48, 100, 4 }, { 0, 252, 0 }, { 252, 0, 0 }, { 0, 252, 0 }, { 20, 20, 20 }
};
static unsigned char g_pen[PEN_COUNT];
static int g_pen_era = -2;

static void pens_update(void)
{
	const unsigned char *pal;
	int i, c;
	if (g_pen_era == assets_era()) return;
	pal = assets_palette();
	for (i = 0; i < PEN_COUNT; i++) {
		long bd = 0x7fffffffL; int best = 0;
		for (c = 0; c < 256; c++) {
			long dr, dg, db, d;
			if (c >= 17 && c <= 19 && !assets_ehb()) continue;   /* hardware pointer pens (EHB: shared, real colours) */
			if (c >= 208 && c <= 211) continue;        /* player colour range (remapped) */
			dr = pal[c * 3] - g_pen_rgb[i][0]; dg = pal[c * 3 + 1] - g_pen_rgb[i][1]; db = pal[c * 3 + 2] - g_pen_rgb[i][2];
			d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
			if (d < bd) { bd = d; best = c; }
		}
		g_pen[i] = (unsigned char)best;
	}
	g_pen_era = assets_era();
}

/* ------------------------------------------------------------------ art (per race + era) */
enum { IMG_RESOURCE, IMG_FILLER, IMG_STATUS, IMG_MENUBTN, IMG_MINIMAP, IMG_BUTTONS, IMG_COUNT };
static struct {
	int key;                                   /* race*16 + era + 1, 0 = not loaded */
	unsigned char *img[IMG_COUNT]; unsigned long img_n[IMG_COUNT];
	unsigned char *info; unsigned long info_n;     /* gfu 4 x 176x176 */
	unsigned char *widget; unsigned long widget_n; /* gfu menu widgets */
	unsigned char *res; unsigned long res_n;       /* gfu 4 x 16x16 */
} g_art;

static void remap_bytes(unsigned char *p, unsigned long n)
{
	const unsigned char *r = assets_src_map();   /* raw art -> screen index (EHB: source -> EHB colour) */
	unsigned long i;
	for (i = 0; i < n; i++) p[i] = r[p[i]];
}

static void remap_gfu(unsigned char *d, unsigned long n)
{
	int cnt, bw, bh, i, j; W2Frame f, g;
	if (!d || !w2_sprite_probe(d, n, &cnt, &bw, &bh)) return;
	for (i = 0; i < cnt; i++) {
		if (w2_sprite_frame(d, n, i, &f) || f.offset + (unsigned long)f.w * f.h > n) continue;
		/* frames may share their pixels: map them once (the EHB map is not idempotent) */
		for (j = 0; j < i; j++) if (!w2_sprite_frame(d, n, j, &g) && g.offset == f.offset) break;
		if (j == i) remap_bytes(d + f.offset, (unsigned long)f.w * f.h);
	}
}

static int local_race(void)
{
	int r = w2.player[w2.local_player & 15].race;
	return r == 1 ? 1 : 0;
}

static void art_load(void)
{
	int race = local_race(), key = race * 16 + assets_era() + 2, i;
	if (g_art.key == key) return;
	for (i = 0; i < IMG_COUNT; i++) war_free(g_art.img[i]);
	war_free(g_art.info); war_free(g_art.widget); war_free(g_art.res);
	memset(&g_art, 0, sizeof g_art);
	g_art.key = key;
	for (i = 0; i < IMG_COUNT; i++) {
		g_art.img[i] = assets_load(ASSET_DATA, 287 + i * 2 + race, &g_art.img_n[i]);
		if (g_art.img[i] && g_art.img_n[i] > 4) remap_bytes(g_art.img[i] + 4, g_art.img_n[i] - 4);
		else w2_log("ui: panel image %d missing", 287 + i * 2 + race);
	}
	g_art.info = assets_load(ASSET_DATA, 354 + race, &g_art.info_n);   remap_gfu(g_art.info, g_art.info_n);
	g_art.widget = assets_load(ASSET_RESOURCES, race, &g_art.widget_n); remap_gfu(g_art.widget, g_art.widget_n);
	g_art.res = assets_load(ASSET_DATA, 187, &g_art.res_n);             remap_gfu(g_art.res, g_art.res_n);
	w2_log("ui: art for race %d era %d loaded", race, assets_era());
}

/* ------------------------------------------------------------------ primitives (hires in, buffer out) */
static void blit_raw(const unsigned char *src, int sw, int sh, int dx, int dy, const W2Clip *c)
{
	int s = 1 << L, ox = dx >> L, oy = dy >> L, w = sw >> L, h = sh >> L, y, x0 = 0, x1 = w;
	if (!src) return;
	if (ox + x0 < c->x0) x0 = c->x0 - ox;
	if (ox + x1 > c->x1) x1 = c->x1 - ox;
	if (x1 <= x0) return;
	for (y = 0; y < h; y++) {
		int by = oy + y;
		const unsigned char *sp;
		unsigned char *dp;
		if (by < c->y0 || by >= c->y1) continue;
		sp = src + (y * s) * sw;
		dp = B + by * P + ox;
		if (!L) memcpy(dp + x0, sp + x0, (size_t)(x1 - x0));
		else { int x; for (x = x0; x < x1; x++) dp[x] = sp[x << 1]; }
	}
}

static void draw_image(int idx, int dx, int dy)
{
	int w, h; const unsigned char *pix;
	if (!g_art.img[idx]) return;
	pix = w2_image_probe(g_art.img[idx], g_art.img_n[idx], &w, &h);
	if (pix) blit_raw(pix, w, h, dx, dy, &g_full);
}

static void draw_gfu(const unsigned char *d, unsigned long n, int frame, int dx, int dy, const W2Clip *c)
{
	W2Frame f;
	if (!d || w2_sprite_frame(d, n, frame, &f) || f.offset + (unsigned long)f.w * f.h > n) return;
	blit_raw(d + f.offset, f.w, f.h, dx + f.x, dy + f.y, c);
}

static void fill_c(int x, int y, int w, int h, int pen, const W2Clip *c)
{
	int x0, y0, x1, y1, yy;
	if (w <= 0 || h <= 0) return;
	x0 = x >> L; y0 = y >> L; x1 = (x + w) >> L; y1 = (y + h) >> L;
	if (x1 <= x0) x1 = x0 + 1;
	if (y1 <= y0) y1 = y0 + 1;
	if (x0 < c->x0) x0 = c->x0;
	if (y0 < c->y0) y0 = c->y0;
	if (x1 > c->x1) x1 = c->x1;
	if (y1 > c->y1) y1 = c->y1;
	if (x1 <= x0) return;
	for (yy = y0; yy < y1; yy++) memset(B + yy * P + x0, g_pen[pen], (size_t)(x1 - x0));
}
static void fill(int x, int y, int w, int h, int pen) { fill_c(x, y, w, h, pen, &g_full); }

static void rect_c(int x, int y, int w, int h, int pen, const W2Clip *c)
{
	int t = 1 << L;   /* a line is one BUFFER pixel thick */
	fill_c(x, y, w, t, pen, c); fill_c(x, y + h - t, w, t, pen, c);
	fill_c(x, y, t, h, pen, c); fill_c(x + w - t, y, t, h, pen, c);
}

static void bevel(int x, int y, int w, int h, int pressed)
{
	int t = 1 << L, lo = pressed ? PEN_DARK : PEN_LIGHT, hi = pressed ? PEN_LIGHT : PEN_DARK;
	fill(x, y, w, t, lo); fill(x, y, t, h, lo);
	fill(x, y + h - t, w, t, hi); fill(x + w - t, y, t, h, hi);
}

static int text(int font, int col, int x, int y, const char *s)
{
	return assets_text(font, col, B, P, &g_full, x, y, s);
}
static void text_c(int font, int col, int cx, int y, const char *s)
{
	int x = cx - assets_text_width(font, s) / 2;
	text(font, col, x < 2 ? 2 : x, y, s);
}
static void text_r(int font, int col, int rx, int y, const char *s)
{
	text(font, col, rx - assets_text_width(font, s), y, s);
}

static const W2Spr *icons(void)
{
	int e = assets_era();
	if (e < 0 || e > 3) e = 0;
	return assets_sprite(ASSET_DATA, w2_eras[e].icons);
}

static void draw_icon(int icon, int x, int y, int player, int pressed)
{
	const W2Spr *s = icons();
	bevel(x - 2, y - 2, BTN_W + 4, BTN_H + 4, pressed);
	if (!s || icon < 0 || icon >= s->count) { fill(x, y, BTN_W, BTN_H, PEN_BLACK); return; }
	if (pressed) { fill(x, y, BTN_W, BTN_H, PEN_BLACK); x++; y++; }
	assets_draw_sprite(s, icon, 0, B, P, x, y, &g_full, assets_remap(player));
}

static void hp_bar(int x, int y, int w, int h, int hp, int max)
{
	int fw, pen;
	if (max <= 0) max = 1;
	if (hp > max) hp = max;
	if (hp < 0) hp = 0;
	fw = (int)((long)w * hp / max);
	pen = hp * 2 > max ? PEN_GREEN : hp * 4 > max ? PEN_YELLOW : PEN_RED;
	fill(x, y, w, h, PEN_BLACK);
	if (fw > 0) fill(x, y, fw, h, pen);
}

/* ------------------------------------------------------------------ selection helpers */
static int me(void) { return w2.local_player & 15; }

static int unit_ok(int s)
{
	const W2Unit *u;
	const w2_unit_type *t;
	if (s < 0 || s >= W2_MAX_UNITS || s >= w2.unit_hi) return 0;
	u = &w2.unit[s];
	if (!(u->flags & W2_UF_ALIVE) || (u->flags & (W2_UF_DYING | W2_UF_HIDDEN | W2_UF_INSIDE))) return 0;
	t = w2_unit(u->type);
	return t && t->kind != W2_KIND_MARKER;
}
static int is_own(int s) { return w2.unit[s].owner == me(); }
static int is_bld(int s) { return (w2.unit[s].flags & W2_UF_BUILDING) != 0; }
/* neutral buildings (gold mines, oil patches) stay selectable once explored, also under fog */
static int visible(int s)
{
	if (is_own(s) || unit_owner_sees(me(), s)) return 1;
	return w2.unit[s].owner == W2_NEUTRAL && is_bld(s) && unit_visible_local(s);
}

static int pick_unit(int px, int py)
{
	int s = unit_at_pixel(px, py);
	return (s >= 0 && unit_ok(s) && visible(s)) ? s : -1;
}

static void sel_clear(void)
{
	int i;
	for (i = 0; i < w2.select_n; i++) w2.unit[w2.select[i]].flags &= (uint16_t)~W2_UF_SELECTED;
	w2.select_n = 0;
}
static int sel_has(int s)
{
	int i;
	for (i = 0; i < w2.select_n; i++) if (w2.select[i] == s) return 1;
	return 0;
}
static void sel_add(int s)
{
	if (w2.select_n >= W2_MAX_SELECT || sel_has(s)) return;
	w2.select[w2.select_n++] = (uint16_t)s;
	w2.unit[s].flags |= W2_UF_SELECTED;
}
static void sel_remove(int s)
{
	int i, j = 0;
	for (i = 0; i < w2.select_n; i++) if (w2.select[i] != s) w2.select[j++] = w2.select[i];
	w2.unit[s].flags &= (uint16_t)~W2_UF_SELECTED;
	w2.select_n = (uint8_t)j;
}
static int sel_own(void) { return w2.select_n > 0 && is_own(w2.select[0]); }
static int sel_lead(void) { return w2.select_n ? w2.select[0] : -1; }

static void sel_prune(void)
{
	int i, j = 0;
	for (i = 0; i < w2.select_n; i++) {
		int s = w2.select[i];
		if (unit_ok(s) && visible(s)) w2.select[j++] = (uint16_t)s;
		else if (s < W2_MAX_UNITS) w2.unit[s].flags &= (uint16_t)~W2_UF_SELECTED;
	}
	if (j != w2.select_n) {
		w2.select_n = (uint8_t)j;
		if (!j && g_mode != MODE_NORMAL) g_mode = MODE_NORMAL;
	}
}

static void selection_changed(void)
{
	g_level = 0;
	g_mode = MODE_NORMAL;
	if (sel_own()) snd_unit(w2.select[0], 0);
}

/* screen (hires) -> map pixels of the view; 1 if inside the map viewport */
static int in_map(int sx, int sy)
{
	return sx >= view.scr_x && sy >= view.scr_y && sx < view.scr_x + view.scr_w && sy < view.scr_y + view.scr_h;
}
static int in_rect(int x, int y, int rx, int ry, int rw, int rh) { return x >= rx && y >= ry && x < rx + rw && y < ry + rh; }
static int map_sx(int px) { return px - view.map_x + view.scr_x; }
static int map_sy(int py) { return py - view.map_y + view.scr_y; }

/* own units of 'type' in the viewport (max 9) */
static void select_same_type(int type)
{
	static uint16_t buf[256];
	int n, i;
	n = units_in_rect(view.map_x, view.map_y, view.map_x + view.scr_w - 1, view.map_y + view.scr_h - 1, buf, 256);
	sel_clear();
	for (i = 0; i < n && w2.select_n < W2_MAX_SELECT; i++)
		if (unit_ok(buf[i]) && is_own(buf[i]) && w2.unit[buf[i]].type == type) sel_add(buf[i]);
}

/* ------------------------------------------------------------------ requirements / command card */
static int has_upgrade(int p, int u)
{
	/* NOTE contract: W2Player.upgrades is 32 bits but there are 52 upgrades. Bits 0..31 = upgrades
	 * 0..31, upgrade_level[u-32] != 0 for 32..51 (see the final report / contract request). */
	if (u < 0 || u >= (int)W2_NUM_UPGRADES) return 0;
	if (u < 32) return (w2.player[p].upgrades >> u) & 1;
	return w2.player[p].upgrade_level[u - 32] != 0;
}

static int g_own_ok, g_prog_ok;
static uint8_t g_own[W2_NUM_UNIT_TYPES];
static uint32_t g_prog[2];

static int owns(int p, int type)
{
	if (type < 0 || type >= (int)W2_NUM_UNIT_TYPES) return 0;
	if (!g_own_ok) {
		int i;
		memset(g_own, 0, sizeof g_own);
		for (i = 0; i < w2.unit_hi; i++) {
			const W2Unit *u = &w2.unit[i];
			if (u->owner == p && unit_ok(i) && !(u->flags & W2_UF_UNDERCONST) && u->type < W2_NUM_UNIT_TYPES) g_own[u->type] = 1;
		}
		g_own_ok = 1;
	}
	return g_own[type];
}

static int researching(int p, int upg)
{
	if (!g_prog_ok) {
		int i;
		g_prog[0] = g_prog[1] = 0;
		for (i = 0; i < w2.unit_hi; i++) {
			const W2Unit *u = &w2.unit[i];
			if (u->owner == p && unit_ok(i) && u->order == W2_ORD_RESEARCH && u->queue_type < 64)
				g_prog[u->queue_type >> 5] |= 1ul << (u->queue_type & 31);
		}
		g_prog_ok = 1;
	}
	return upg >= 0 && upg < 64 && ((g_prog[upg >> 5] >> (upg & 31)) & 1);
}

static int busy(int s)
{
	int o = w2.unit[s].order;
	return o == W2_ORD_TRAIN || o == W2_ORD_RESEARCH || o == W2_ORD_UPGRADE_TO;
}

static int req_ok(const w2_button *b, int s)
{
	int p = w2.unit[s].owner;
	if (b->bflags & W2BF_WARGUS_EXTRA) return 0;
	switch (b->req) {
	case W2R_NONE: case W2R_TRUE: case W2R_NETWORK: break;
	case W2R_UPGRADE: if (!has_upgrade(p, b->req_arg[0])) return 0; break;
	case W2R_UNITS_OR: if (!owns(p, b->req_arg[0]) && !owns(p, b->req_arg[1])) return 0; break;
	case W2R_NO_RESEARCH: if (w2.unit[s].order == W2_ORD_RESEARCH || w2.unit[s].order == W2_ORD_UPGRADE_TO) return 0; break;
	case W2R_UPGRADE_TO: if (busy(s)) return 0; break;
	case W2R_SINGLE_RESEARCH: if (has_upgrade(p, b->arg) || researching(p, b->arg)) return 0; break;
	default: return 0;
	}
	switch (b->action) {
	case W2B_TRAIN_UNIT: case W2B_BUILD: case W2B_UPGRADE_TO: return econ_allowed_unit(p, b->arg);
	case W2B_RESEARCH: return econ_allowed_upgrade(p, b->arg);
	case W2B_EXPLORE: return 0;
	}
	return 1;
}

/* the last entry of (type, level, pos) whose requirement holds (icon variants) */
static const w2_button *pick_button(int s, int level, int pos)
{
	unsigned n, i;
	const w2_button *b = w2_unit_buttons(w2.unit[s].type, &n), *best = NULL;
	for (i = 0; b && i < n; i++)
		if (b[i].level == level && b[i].pos == pos && req_ok(&b[i], s)) best = &b[i];
	return best;
}

static const w2_button *generic(int level, int action)
{
	unsigned i;
	for (i = 0; i < w2_num_buttons; i++)
		if (w2_buttons[i].unit == W2_NONE && w2_buttons[i].level == level && w2_buttons[i].action == action) return &w2_buttons[i];
	return NULL;
}

static int type_has_level(int type, int level)
{
	unsigned n, i;
	const w2_button *b = w2_unit_buttons((unsigned)type, &n);
	for (i = 0; b && i < n; i++) if (b[i].level == level) return 1;
	return 0;
}

static void card_build(void)
{
	int lead, pos, i;
	memset(g_card, 0, sizeof g_card);
	g_own_ok = g_prog_ok = 0;
	if (g_menu || !sel_own()) return;
	lead = sel_lead();
	if (g_mode != MODE_NORMAL) { g_card[8] = generic(9, W2B_CANCEL); return; }
	if (w2.unit[lead].flags & W2_UF_UNDERCONST) { g_card[8] = generic(0, W2B_CANCEL_BUILD); return; }
	if (w2.unit[lead].order == W2_ORD_TRAIN) { g_card[8] = generic(0, W2B_CANCEL_TRAIN_UNIT); return; }
	if (w2.unit[lead].order == W2_ORD_RESEARCH || w2.unit[lead].order == W2_ORD_UPGRADE_TO) { g_card[8] = generic(0, W2B_CANCEL_UPGRADE); return; }
	if (g_level && !type_has_level(w2.unit[lead].type, g_level)) g_level = 0;
	for (pos = 1; pos <= 9; pos++) {
		const w2_button *b = pick_button(lead, g_level, pos);
		for (i = 1; b && i < w2.select_n; i++) {
			const w2_button *o = pick_button(w2.select[i], g_level, pos);
			if (!o || o->action != b->action || o->arg != b->arg) b = NULL;
		}
		g_card[pos - 1] = b;
	}
}

static int card_at(int x, int y)
{
	int i;
	for (i = 0; i < 9; i++)
		if (g_card[i] && in_rect(x, y, g_btn_x[i % 3] - 4, g_btn_y[i / 3] - 4, CELL_W, CELL_H)) return i;
	return -1;
}


static void hint_of(const w2_button *b, char *out, int n, int *g, int *w, int *o)
{
	const w2_unit_type *t;
	*g = *w = *o = 0;
	switch (b->action) {
	case W2B_MOVE: snprintf(out, (size_t)n, "MOVE"); break;
	case W2B_STOP: snprintf(out, (size_t)n, "STOP"); break;
	case W2B_ATTACK: snprintf(out, (size_t)n, "ATTACK"); break;
	case W2B_PATROL: snprintf(out, (size_t)n, "PATROL"); break;
	case W2B_STAND_GROUND: snprintf(out, (size_t)n, "STAND GROUND"); break;
	case W2B_ATTACK_GROUND: snprintf(out, (size_t)n, "ATTACK GROUND"); break;
	case W2B_HARVEST: snprintf(out, (size_t)n, "HARVEST LUMBER/MINE GOLD"); break;
	case W2B_RETURN_GOODS: snprintf(out, (size_t)n, "RETURN WITH GOODS"); break;
	case W2B_REPAIR: snprintf(out, (size_t)n, "REPAIR"); break;
	case W2B_BUTTON: snprintf(out, (size_t)n, b->arg == 1 ? "BUILD BASIC STRUCTURE" : b->arg == 2 ? "BUILD ADVANCED STRUCTURE" : "CANCEL"); break;
	case W2B_BUILD: case W2B_TRAIN_UNIT: case W2B_UPGRADE_TO:
		t = w2_unit(b->arg);
		snprintf(out, (size_t)n, "%s %s", b->action == W2B_BUILD ? "BUILD" : b->action == W2B_TRAIN_UNIT ? "TRAIN" : "UPGRADE TO", t ? t->name : "?");
		if (t) { *g = t->gold; *w = t->wood; *o = t->oil; }
		break;
	case W2B_RESEARCH:
		if (b->arg < W2_NUM_UPGRADES) {
			snprintf(out, (size_t)n, "%s", w2_upgrades[b->arg].name);
			*g = w2_upgrades[b->arg].gold; *w = w2_upgrades[b->arg].wood; *o = w2_upgrades[b->arg].oil;
		}
		break;
	case W2B_CAST_SPELL: snprintf(out, (size_t)n, "%s", b->arg < W2_NUM_SPELLS ? w2_spells[b->arg].name : "SPELL"); break;
	case W2B_UNLOAD: snprintf(out, (size_t)n, "UNLOAD"); break;
	case W2B_DEMOLISH: snprintf(out, (size_t)n, "DEMOLISH"); break;
	default: snprintf(out, (size_t)n, "CANCEL"); break;
	}
}

/* ------------------------------------------------------------------ commands */
static void click_sound(void) { snd_play(CLICK_SOUND, -1, -1); }

static int afford(int p, int g, int w, int o, int food)
{
	const char *what = NULL;
	if (w2.player[p].gold < g) what = "Not enough gold... mine more gold.";
	else if (w2.player[p].wood < w) what = "Not enough lumber... chop more trees.";
	else if (w2.player[p].oil < o) what = "Not enough oil... harvest more oil.";
	else if (food > 0 && w2.player[p].food_used + food > w2.player[p].food_supply)
		what = w2.player[p].race == 1 ? "Not enough food... build more pig farms." : "Not enough food... build more farms.";
	if (what) { ui_message(what); return 0; }
	return econ_can_afford(p, g, w, o) ? 1 : (ui_message("Not enough resources."), 0);
}

static void mark(int px, int py) { g_mark_x = px; g_mark_y = py; g_mark_t = 12; }

static void cast(int s, int spell, int target, int tx, int ty)
{
	if (spell >= (int)W2_NUM_SPELLS) return;
	if (w2.unit[s].mana < spells_mana_cost(spell)) { ui_message("Not enough mana."); return; }
	cmd_cast(s, spell, target, tx, ty);       /* checks mana, research, range rules (spells.c) */
}

static void ack(void)
{
	if (sel_own()) snd_unit(sel_lead(), 1);
}

static void do_button(const w2_button *b)
{
	int lead = sel_lead(), p = me(), i;
	const w2_unit_type *t;
	if (!b || lead < 0) return;
	click_sound();
	switch (b->action) {
	case W2B_MOVE: case W2B_ATTACK: case W2B_PATROL: case W2B_ATTACK_GROUND: case W2B_HARVEST:
	case W2B_UNLOAD:   /* Mac f9_5594: a docked, waiting transport unloads at once, otherwise ask where */
		if (transport_waiting(lead) || transport_docked(lead)) {
			for (i = 0; i < w2.select_n; i++) if (transport_is(w2.select[i]))
				unit_set_order(w2.select[i], W2_ORD_UNLOAD, W2_NO_UNIT, w2.unit[w2.select[i]].tx, w2.unit[w2.select[i]].ty);
			ack(); break;
		}
		g_mode = MODE_TARGET; g_tgt = b; break;
	case W2B_REPAIR: case W2B_DEMOLISH:
		g_mode = MODE_TARGET; g_tgt = b; break;
	case W2B_CAST_SPELL:
		if (b->arg < W2_NUM_SPELLS && w2_spells[b->arg].target == W2_ST_SELF) {
			for (i = 0; i < w2.select_n; i++) cast(w2.select[i], b->arg, w2.select[i], w2.unit[w2.select[i]].tx, w2.unit[w2.select[i]].ty);
			ack();
		} else { g_mode = MODE_TARGET; g_tgt = b; }
		break;
	case W2B_STOP: for (i = 0; i < w2.select_n; i++) cmd_stop(w2.select[i]); ack(); break;
	case W2B_STAND_GROUND: for (i = 0; i < w2.select_n; i++) cmd_stand(w2.select[i]); ack(); break;
	case W2B_RETURN_GOODS: for (i = 0; i < w2.select_n; i++) cmd_return(w2.select[i]); ack(); break;
	case W2B_BUTTON: g_level = b->arg; break;
	case W2B_BUILD:
		t = w2_unit(b->arg);
		if (t && afford(p, t->gold, t->wood, t->oil, 0)) { g_mode = MODE_PLACE; g_place_type = b->arg; g_place_builder = lead; }
		break;
	case W2B_TRAIN_UNIT:
		t = w2_unit(b->arg);
		if (t && afford(p, t->gold, t->wood, t->oil, t->food < 0 ? -t->food : 0)) cmd_train(lead, b->arg);
		break;
	case W2B_UPGRADE_TO:
		t = w2_unit(b->arg);
		if (t && afford(p, t->gold, t->wood, t->oil, 0)) cmd_upgrade_to(lead, b->arg);
		break;
	case W2B_RESEARCH:
		if (b->arg < W2_NUM_UPGRADES && afford(p, w2_upgrades[b->arg].gold, w2_upgrades[b->arg].wood, w2_upgrades[b->arg].oil, 0))
			cmd_research(lead, b->arg);
		break;
	case W2B_CANCEL: g_mode = MODE_NORMAL; g_level = 0; break;
	case W2B_CANCEL_UPGRADE: case W2B_CANCEL_TRAIN_UNIT: case W2B_CANCEL_BUILD: cmd_cancel(lead); break;
	default: break;
	}
}

/* a target was chosen for g_tgt at map pixel (px,py), unit tu (-1 none) */
static void do_target(int px, int py, int tu)
{
	int tx = px >> 5, ty = py >> 5, i, a;
	if (!g_tgt || !sel_own()) { g_mode = MODE_NORMAL; return; }
	a = g_tgt->action;
	for (i = 0; i < w2.select_n; i++) {
		int s = w2.select[i];
		if (is_bld(s) && a != W2B_ATTACK && a != W2B_STOP) continue;
		switch (a) {
		case W2B_MOVE: if (tu >= 0 && tu != s) cmd_follow(s, tu); else cmd_move(s, tx, ty); break;
		case W2B_ATTACK: if (tu >= 0 && tu != s) cmd_attack(s, tu); else cmd_attack_move(s, tx, ty); break;
		case W2B_PATROL: cmd_patrol(s, tx, ty); break;
		case W2B_ATTACK_GROUND: cmd_attack_ground(s, tx, ty); break;
		case W2B_HARVEST: cmd_harvest(s, tu, tx, ty); break;
		case W2B_REPAIR: if (tu >= 0) cmd_repair(s, tu); break;
		case W2B_UNLOAD: unit_set_order(s, W2_ORD_UNLOAD, W2_NO_UNIT, tx, ty); break;
		case W2B_DEMOLISH: unit_set_order(s, W2_ORD_DEMOLISH, tu >= 0 ? tu : W2_NO_UNIT, tx, ty); break;
		case W2B_CAST_SPELL: cast(s, g_tgt->arg, tu, tx, ty); break;
		}
	}
	mark(px, py);
	ack();
	if (!g_shift) { g_mode = MODE_NORMAL; g_level = 0; }
}

static void smart(int px, int py, int tu)
{
	int i, n = 0;
	if (!sel_own()) return;
	for (i = 0; i < w2.select_n; i++) {
		int s = w2.select[i];
		if (is_bld(s)) continue;
		cmd_smart(s, px >> 5, py >> 5, tu);
		n++;
	}
	if (n) { mark(px, py); ack(); }
}

/* building placement: top-left tile for the pointer at map pixel (px,py) (pointer = centre) */
static void place_tile(int px, int py, int *tx, int *ty)
{
	const w2_unit_type *t = w2_unit(g_place_type);
	int w = t ? t->tile_w : 1, h = t ? t->tile_h : 1;
	*tx = (px - (w - 1) * 16) >> 5; *ty = (py - (h - 1) * 16) >> 5;
	if (*tx > w2.map.w - w) *tx = w2.map.w - w;
	if (*ty > w2.map.h - h) *ty = w2.map.h - h;
	if (*tx < 0) *tx = 0;
	if (*ty < 0) *ty = 0;
}

static void try_place(int px, int py)
{
	int tx, ty;
	place_tile(px, py, &tx, &ty);
	if (!unit_ok(g_place_builder) || !is_own(g_place_builder)) { g_mode = MODE_NORMAL; return; }
	if (!econ_place_ok(me(), g_place_type, tx, ty, g_place_builder)) { ui_message("You cannot build there."); return; }
	cmd_build(g_place_builder, g_place_type, tx, ty);
	snd_unit(g_place_builder, 1);
	if (!g_shift) { g_mode = MODE_NORMAL; g_level = 0; }
}

/* ------------------------------------------------------------------ selection by mouse */
static void click_select(int sx, int sy)
{
	int px, py, s;
	unsigned long now = amigagfx_millis();
	if (!view_to_map(sx, sy, &px, &py)) return;
	s = pick_unit(px, py);
	if (s < 0) return;
	if ((g_ctrl || (s == g_click_unit && now - g_click_ms < 400)) && is_own(s) && !is_bld(s)) {
		select_same_type(w2.unit[s].type);
		g_click_unit = -1;
		selection_changed();
		return;
	}
	g_click_unit = s; g_click_ms = now;
	if (g_shift && is_own(s) && !is_bld(s) && sel_own() && !is_bld(sel_lead())) {
		if (sel_has(s)) sel_remove(s); else sel_add(s);
		if (w2.select_n) snd_unit(s, 0);
		g_level = 0;
		return;
	}
	sel_clear();
	sel_add(s);
	selection_changed();
}

static void box_select(int sx0, int sy0, int sx1, int sy1)
{
	static uint16_t buf[256];
	int ax, ay, bx, by, n, i, t, any = 0, keep;
	if (sx0 > sx1) { t = sx0; sx0 = sx1; sx1 = t; }
	if (sy0 > sy1) { t = sy0; sy0 = sy1; sy1 = t; }
	if (sx0 < view.scr_x) sx0 = view.scr_x;
	if (sy0 < view.scr_y) sy0 = view.scr_y;
	if (sx1 >= view.scr_x + view.scr_w) sx1 = view.scr_x + view.scr_w - 1;
	if (sy1 >= view.scr_y + view.scr_h) sy1 = view.scr_y + view.scr_h - 1;
	if (!view_to_map(sx0, sy0, &ax, &ay) || !view_to_map(sx1, sy1, &bx, &by)) return;
	n = units_in_rect(ax, ay, bx, by, buf, 256);
	/* own mobile units first; else one own building; else one visible other unit */
	for (i = 0; i < n; i++) if (unit_ok(buf[i]) && is_own(buf[i]) && !is_bld(buf[i])) { any = 1; break; }
	keep = g_shift && sel_own() && !is_bld(sel_lead()) && any;
	if (!keep) sel_clear();
	if (any) {
		for (i = 0; i < n && w2.select_n < W2_MAX_SELECT; i++) {
			int s = buf[i];
			const w2_unit_type *ty;
			if (!unit_ok(s) || !is_own(s) || is_bld(s)) continue;
			ty = w2_unit(w2.unit[s].type);
			if (ty && !(ty->xflags & W2UX_RECT_SELECT) && !(ty->flags & (W2UF_LAND | W2UF_AIR | W2UF_SEA))) continue;
			sel_add(s);
		}
	} else {
		for (i = 0; i < n; i++) if (unit_ok(buf[i]) && is_own(buf[i])) { sel_add(buf[i]); break; }
		if (!w2.select_n) for (i = 0; i < n; i++) if (unit_ok(buf[i]) && visible(buf[i])) { sel_add(buf[i]); break; }
	}
	selection_changed();
}

/* ------------------------------------------------------------------ control groups */
static void group_set(int g)
{
	int i;
	if (!sel_own()) return;
	g_grp_n[g] = 0;
	for (i = 0; i < w2.select_n; i++) g_grp[g][g_grp_n[g]++] = w2.select[i];
}

static void group_recall(int g, int add)
{
	int i, n = 0;
	unsigned long now = amigagfx_millis();
	if (!add) sel_clear();
	for (i = 0; i < g_grp_n[g]; i++) {
		int s = g_grp[g][i];
		if (unit_ok(s) && is_own(s)) { sel_add(s); g_grp[g][n++] = (uint16_t)s; }
	}
	g_grp_n[g] = (uint8_t)n;
	if (!w2.select_n) return;
	if (g_grp_last == g && now - g_grp_ms < 500) {
		int cx, cy;
		unit_center(w2.select[0], &cx, &cy);
		view_center_on(cx, cy);
	}
	g_grp_last = g; g_grp_ms = now;
	selection_changed();
}

/* ------------------------------------------------------------------ public: init / message / tick */
void ui_init(void)
{
	g_mode = MODE_NORMAL; g_level = 0; g_tgt = NULL;
	g_drag = 0; g_mm_drag = 0; g_btn_press = -1; g_menu_press = 0; g_gm_press = -1;
	g_shift = g_ctrl = g_alt = 0;
	game_draws_minimap = 0;     /* ui_draw draws the minimap frame and then the minimap itself */
	g_menu = 0; g_quit = 0; g_mark_t = 0; g_edge_dir = -1;
	memset(g_msg_ttl, 0, sizeof g_msg_ttl);
	memset(g_grp_n, 0, sizeof g_grp_n);
	g_grp_last = -1; g_click_unit = -1;
	g_status[0] = 0;
	g_pen_era = -2;
}

int ui_menu_active(void) { return g_menu; }
int ui_wants_quit(void) { return g_quit; }

void ui_message(const char *t)
{
	int i;
	if (!t) return;
	for (i = 0; i < MSG_LINES - 1; i++) {
		memcpy(g_msg[i], g_msg[i + 1], sizeof g_msg[i]);
		g_msg_ttl[i] = g_msg_ttl[i + 1];
	}
	snprintf(g_msg[MSG_LINES - 1], sizeof g_msg[0], "%s", t);
	g_msg_ttl[MSG_LINES - 1] = MSG_TTL;
}

/* edge direction 0..7 (N clockwise) or -1 */
static int edge_dir(void)
{
	int dx = 0, dy = 0;
	if (!g_mouse_seen || g_menu || g_drag == 2 || g_mm_drag) return -1;
	if (g_mx < SCROLL_EDGE) dx = -1; else if (g_mx >= 640 - SCROLL_EDGE) dx = 1;
	if (g_my < SCROLL_EDGE) dy = -1; else if (g_my >= 480 - SCROLL_EDGE) dy = 1;
	if (!dx && !dy) return -1;
	if (!dx) return dy < 0 ? 0 : 4;
	if (!dy) return dx > 0 ? 2 : 6;
	if (dx > 0) return dy < 0 ? 1 : 3;
	return dy < 0 ? 7 : 5;
}

void ui_tick(void)
{
	int i;
	sel_prune();
	for (i = 0; i < MSG_LINES; i++) if (g_msg_ttl[i] > 0) g_msg_ttl[i]--;
	if (g_mark_t > 0) g_mark_t--;
	/* scrolling itself (edges, arrow keys) is game.c's scroll_tick; the UI only shows the arrow */
	g_edge_dir = edge_dir();
}

/* ------------------------------------------------------------------ drawing: info panel */
static void two_line_name(const char *name, int cx, int y)
{
	char a[48], *sp;
	int n;
	if (assets_text_width(FONT_GAME, name) <= 110 || (n = (int)strlen(name)) >= (int)sizeof a) { text_c(FONT_GAME, FC_WHITE, cx, y + 7, name); return; }
	memcpy(a, name, (size_t)n + 1);
	sp = strrchr(a, ' ');
	while (sp && sp > a && assets_text_width(FONT_GAME, a) > 110) {
		*sp = 0;
		if (assets_text_width(FONT_GAME, a) <= 110) break;
		{ char *q = strrchr(a, ' '); *sp = ' '; sp = q; }
	}
	if (!sp || sp <= a) { text_c(FONT_GAME, FC_WHITE, cx, y + 7, name); return; }
	*sp = 0;
	text_c(FONT_GAME, FC_WHITE, cx, y, a);
	text_c(FONT_GAME, FC_WHITE, cx, y + 15, sp + 1);
}

static void stat_line(int y, const char *label, const char *val)
{
	text_r(FONT_GAME, FC_WHITE, 102, y, label);
	text(FONT_GAME, FC_YELLOW, 105, y, val);
}

static int total_cycles(const W2Unit *u, int *icon)
{
	const w2_unit_type *t;
	*icon = -1;
	if (u->flags & W2_UF_UNDERCONST) { t = w2_unit(u->type); return t ? (int)w2_time_to_cycles(t->build_time) : 1; }
	if (u->order == W2_ORD_RESEARCH && u->queue_type < W2_NUM_UPGRADES) {
		*icon = w2_upgrades[u->queue_type].icon;
		return (int)w2_time_to_cycles(w2_upgrades[u->queue_type].time);
	}
	t = w2_unit(u->queue_type);
	if (!t) return 1;
	*icon = t->icon;
	return (int)w2_time_to_cycles(t->build_time);
}

static void info_single(int s)
{
	const W2Unit *u = &w2.unit[s];
	const w2_unit_type *t = w2_unit(u->type);
	int own = is_own(s), frame, y0 = INFO_Y, prod = 0;
	char b[48];
	if (!t) return;
	prod = own && ((u->flags & W2_UF_UNDERCONST) || busy(s));
	frame = !own ? 0 : prod ? 3 : (t->xflags & W2UX_MANA) ? 2 : 1;
	draw_gfu(g_art.info, g_art.info_n, frame, 0, INFO_Y, &g_full);
	draw_icon(t->icon, PORTRAIT_X, PORTRAIT_Y, u->owner, 0);
	two_line_name(t->name, 114, 170);
	if (u->owner != W2_NEUTRAL && !(t->xflags & W2UX_RESOURCE)) {
		hp_bar(PORTRAIT_X - 1, y0 + 51, 48, 6, u->hp, t->hp);
		snprintf(b, sizeof b, "%d/%d", u->hp, t->hp);
		text_c(FONT_SMALL, FC_WHITE, PORTRAIT_X + 23, y0 + 60, b);
	}
	if (prod) {
		int icon, tot = total_cycles(u, &icon), pct;
		pct = tot > 0 ? (int)((long)u->progress * 100 / tot) : 0;
		if (pct > 100) pct = 100;
		if (pct < 0) pct = 0;
		if (!(u->flags & W2_UF_UNDERCONST)) {
			text_r(FONT_GAME, FC_WHITE, PROD_ICON_X - 6, PROD_ICON_Y + 12,
			       u->order == W2_ORD_TRAIN ? "Training:" : u->order == W2_ORD_RESEARCH ? "Researching:" : "Upgrading:");
			if (icon >= 0) draw_icon(icon, PROD_ICON_X, PROD_ICON_Y, u->owner, 0);
		} else {
			const w2_unit_type *wk = w2_unit(t->race == W2_RACE_ORC ? W2U_PEON : W2U_PEASANT);
			text_r(FONT_GAME, FC_WHITE, PROD_ICON_X - 6, PROD_ICON_Y + 12, "Building:");
			if (wk) draw_icon(wk->icon, PROD_ICON_X, PROD_ICON_Y, u->owner, 0);
		}
		fill(12, y0 + 153, 152, 14, PEN_BARBG);
		fill(12, y0 + 153, 152 * pct / 100, 14, PEN_BAR);
		snprintf(b, sizeof b, "%d%% Complete", pct);
		text_c(FONT_GAME, FC_WHITE, 88, y0 + 154, b);
		return;
	}
	if (t->xflags & W2UX_RESOURCE) {
		snprintf(b, sizeof b, "%s Left: %lu", t->flags & W2UF_OIL_PATCH ? "Oil" : "Gold", (unsigned long)u->res_left);
		text_c(FONT_GAME, FC_WHITE, 88, STAT_Y(1), b);
		return;
	}
	if (t->flags & W2UF_OIL_PLATFORM) {
		snprintf(b, sizeof b, "Oil Left: %lu", (unsigned long)u->res_left);
		text_c(FONT_GAME, FC_WHITE, 88, STAT_Y(1), b);
		return;
	}
	if (own && is_bld(s) && t->food > 0 && !(t->xflags & W2UX_MAIN_HALL)) {
		const W2Player *pl = &w2.player[u->owner];
		text_c(FONT_GAME, FC_WHITE, 88, STAT_Y(0), "Food Usage:");
		snprintf(b, sizeof b, "%d", pl->food_supply);  stat_line(STAT_Y(1), "Grown:", b);
		snprintf(b, sizeof b, "%d", pl->food_used);    stat_line(STAT_Y(2), "Used:", b);
		return;
	}
	if (own && (t->xflags & W2UX_MAIN_HALL)) {
		int i, bonus[4] = { 0, 0, 0, 0 };
		for (i = 0; i < w2.unit_hi; i++) {
			const w2_unit_type *t2;
			if (!unit_ok(i) || w2.unit[i].owner != u->owner || (w2.unit[i].flags & W2_UF_UNDERCONST)) continue;
			t2 = w2_unit(w2.unit[i].type);
			if (t2 && t2->improve_res > 0 && t2->improve_res < 4 && t2->improve_pct > bonus[t2->improve_res]) bonus[t2->improve_res] = t2->improve_pct;
		}
		text_c(FONT_GAME, FC_WHITE, 88, STAT_Y(0), "Production");
		for (i = 1; i <= 3; i++) {
			if (bonus[i]) snprintf(b, sizeof b, "100+%d", bonus[i]); else snprintf(b, sizeof b, "100");
			stat_line(STAT_Y(i), i == 1 ? "Gold:" : i == 2 ? "Lumber:" : "Oil:", b);
		}
		return;
	}
	if (is_bld(s) && !(t->flags & W2UF_CAN_ATTACK)) {
		snprintf(b, sizeof b, "%d", t->armor); stat_line(STAT_Y(0), "Armor:", b);
		return;
	}
	if (u->owner == W2_NEUTRAL) return;
	{   /* combat stats */
		int arm = econ_stat(s, W2_STAT_ARMOR), pier = econ_stat(s, W2_STAT_PIERCING), sight = econ_stat(s, W2_STAT_SIGHT),
		    range = econ_stat(s, W2_STAT_RANGE), mn = t->piercing_damage / 2, mx = t->piercing_damage + t->basic_damage;
		arm -= t->armor; pier -= t->piercing_damage; sight -= t->sight; range -= t->max_range;
		if (arm > 0) snprintf(b, sizeof b, "%d+%d", t->armor, arm); else snprintf(b, sizeof b, "%d", t->armor);
		stat_line(STAT_Y(0), "Armor:", b);
		if (mx > 0) {
			if (pier > 0) snprintf(b, sizeof b, "%d-%d+%d", mn, mx, pier); else snprintf(b, sizeof b, "%d-%d", mn, mx);
			stat_line(STAT_Y(1), "Damage:", b);
			if (range > 0) snprintf(b, sizeof b, "%d+%d", t->max_range, range); else snprintf(b, sizeof b, "%d", t->max_range);
			stat_line(STAT_Y(2), "Range:", b);
		}
		if (sight > 0) snprintf(b, sizeof b, "%d+%d", t->sight, sight); else snprintf(b, sizeof b, "%d", t->sight);
		stat_line(STAT_Y(3), "Sight:", b);
		if (!is_bld(s)) { snprintf(b, sizeof b, "%d", t->speed); stat_line(STAT_Y(4), "Speed:", b); }
		if (own && (t->xflags & W2UX_MANA)) {
			text_r(FONT_GAME, FC_WHITE, 100, STAT_Y(5), "Magic:");
			fill(103, STAT_Y(5) - 1, 60, 14, PEN_BARBG);
			fill(103, STAT_Y(5) - 1, 60 * u->mana / 255, 14, PEN_BLUE);
			snprintf(b, sizeof b, "%d", u->mana);
			text_c(FONT_GAME, FC_WHITE, 133, STAT_Y(5), b);
		}
	}
}

static int sel_icon_at(int x, int y)
{
	int i;
	if (w2.select_n < 2) return in_rect(x, y, PORTRAIT_X, PORTRAIT_Y, BTN_W, BTN_H) && w2.select_n ? 0 : -1;
	for (i = 0; i < w2.select_n; i++)
		if (in_rect(x, y, g_sel_x[i % 3] - 4, g_sel_y[i / 3] - 4, CELL_W, 53)) return i;
	return -1;
}

static void draw_info(void)
{
	int i;
	if (w2.select_n == 1) { info_single(w2.select[0]); return; }
	draw_gfu(g_art.info, g_art.info_n, 0, 0, INFO_Y, &g_full);
	for (i = 0; i < w2.select_n; i++) {
		int s = w2.select[i], x = g_sel_x[i % 3], y = g_sel_y[i / 3];
		const w2_unit_type *t = w2_unit(w2.unit[s].type);
		if (!t) continue;
		draw_icon(t->icon, x, y, w2.unit[s].owner, 0);
		hp_bar(x - 1, y + BTN_H + 3, BTN_W + 2, 5, w2.unit[s].hp, t->hp);
	}
}

/* ------------------------------------------------------------------ drawing: bars, card, map overlays */
static void draw_resources(void)
{
	const W2Player *pl = &w2.player[me()];
	char b[32];
	long v[3];
	int i;
	v[0] = pl->gold; v[1] = pl->wood; v[2] = pl->oil;
	for (i = 0; i < 3; i++) {
		draw_gfu(g_art.res, g_art.res_n, i, g_res_x[i], 0, &g_full);
		snprintf(b, sizeof b, "%ld", v[i]);
		text(FONT_GAME, FC_WHITE, g_res_x[i] + 18, 2, b);
	}
	snprintf(b, sizeof b, "Food: %d/%d", pl->food_used, pl->food_supply);
	text(FONT_GAME, pl->food_used > pl->food_supply ? FC_RED : FC_WHITE, FOOD_X, 2, b);
}

static void draw_status(void)
{
	char h[80];
	int i = card_at(g_mx, g_my), g, w, o, x;
	h[0] = 0; g = w = o = 0;
	if (g_menu) return;
	if (i >= 0) {
		char *c;
		hint_of(g_card[i], h, sizeof h, &g, &w, &o);
		for (c = h; *c; c++) if (*c >= 'a' && *c <= 'z') *c = (char)(*c - 32);   /* original hints are upper case */
	} else if ((i = sel_icon_at(g_mx, g_my)) >= 0 && in_rect(g_mx, g_my, 0, INFO_Y, PANEL_W, 176)) {
		const w2_unit_type *t = w2_unit(w2.unit[w2.select[i]].type);
		if (t) snprintf(h, sizeof h, "%s", t->name);
	} else if (in_rect(g_mx, g_my, MENU_BX, MENU_BY, MENU_BW, MENU_BH)) snprintf(h, sizeof h, "MENU (F10)");
	snprintf(g_status, sizeof g_status, "%s", h);
	if (!h[0]) return;
	text(FONT_GAME, FC_WHITE, RES_X + 2, STATUS_Y + 2, h);
	x = FILL_X - 4;
	{   /* costs, right-aligned: icon + number */
		int v[3], r;
		char b[16];
		v[0] = g; v[1] = w; v[2] = o;
		for (r = 2; r >= 0; r--) {
			if (!v[r]) continue;
			snprintf(b, sizeof b, "%d", v[r]);
			x -= assets_text_width(FONT_GAME, b);
			text(FONT_GAME, FC_WHITE, x, STATUS_Y + 2, b);
			x -= 18;
			draw_gfu(g_art.res, g_art.res_n, r, x, STATUS_Y, &g_full);
			x -= 8;
		}
	}
}

static void draw_card(void)
{
	int i;
	for (i = 0; i < 9; i++) {
		if (!g_card[i]) continue;
		draw_icon(g_card[i]->icon, g_btn_x[i % 3], g_btn_y[i / 3], me(), g_btn_press == i && card_at(g_mx, g_my) == i);
	}
}

static void draw_messages(void)
{
	int i, n = 0, y;
	(void)n;
	y = view.scr_y + 10;
	for (i = 0; i < MSG_LINES; i++) {
		if (g_msg_ttl[i] <= 0) continue;
		text(FONT_GAME, FC_WHITE, view.scr_x + 10, y, g_msg[i]);
		y += 14;
	}
}

static int tile_bad(int tx, int ty, int builder)
{
	int f, o;
	if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 1;
	if (!fog_explored(tx, ty)) return 1;
	f = w2.map.flags[ty * w2.map.w + tx];
	if (f & (W2_TF_NOBUILD | W2_TF_BUILDING | W2_TF_TREE | W2_TF_ROCK | W2_TF_WALL | W2_TF_NOWALK)) return 1;
	o = w2.map.occ_ground[ty * w2.map.w + tx];
	return o != W2_NO_UNIT && o != builder;
}

static void draw_ghost(void)
{
	const w2_unit_type *t = w2_unit(g_place_type);
	const W2Spr *spr;
	int px, py, tx, ty, ok, x, y, any = 0, e = assets_era();
	if (!t || !in_map(g_mx, g_my) || !view_to_map(g_mx, g_my, &px, &py)) return;
	place_tile(px, py, &tx, &ty);
	ok = econ_place_ok(me(), g_place_type, tx, ty, g_place_builder);
	spr = assets_sprite(ASSET_DATA, t->gfx[e < 0 || e > 3 ? 0 : e]);
	if (spr)
		assets_draw_sprite(spr, 0, 0, B, P, map_sx(tx * 32 + (t->tile_w * 32 - spr->box_w) / 2),
		                   map_sy(ty * 32 + (t->tile_h * 32 - spr->box_h) / 2), &g_mapclip, assets_remap(me()));
	if (!ok) for (y = 0; y < t->tile_h; y++) for (x = 0; x < t->tile_w; x++) any |= tile_bad(tx + x, ty + y, g_place_builder);
	for (y = 0; y < t->tile_h; y++) for (x = 0; x < t->tile_w; x++) {
		int bad = !ok && (!any || tile_bad(tx + x, ty + y, g_place_builder));
		int sx = map_sx((tx + x) * 32) >> L, sy = map_sy((ty + y) * 32) >> L, n = 32 >> L, i, j;
		unsigned char c = g_pen[bad ? PEN_GRED : PEN_GGREEN];
		for (j = 0; j < n; j++) {
			int by = sy + j;
			if (by < g_mapclip.y0 || by >= g_mapclip.y1) continue;
			for (i = (j & 1); i < n; i += 2) {
				int bx = sx + i;
				if (bx >= g_mapclip.x0 && bx < g_mapclip.x1) B[by * P + bx] = c;
			}
		}
	}
}

static void draw_map_overlays(void)
{
	if (g_drag == 2) {
		int x0 = g_dx0 < g_mx ? g_dx0 : g_mx, y0 = g_dy0 < g_my ? g_dy0 : g_my;
		int w = (g_dx0 < g_mx ? g_mx - g_dx0 : g_dx0 - g_mx) + 1, h = (g_dy0 < g_my ? g_my - g_dy0 : g_dy0 - g_my) + 1;
		rect_c(x0, y0, w, h, PEN_BOX, &g_mapclip);
	}
	if (g_mode == MODE_PLACE) draw_ghost();
	if (g_mark_t > 0) {
		const W2Spr *s = assets_sprite(ASSET_DATA, 352);
		if (s) assets_draw_sprite(s, (12 - g_mark_t) / 3 % (s->count ? s->count : 1), 0, B, P,
		                          map_sx(g_mark_x) - 16, map_sy(g_mark_y) - 16, &g_mapclip, NULL);
	}
}

static void draw_menu_button(void)
{
	int pressed = g_menu_press && in_rect(g_mx, g_my, MENU_BX, MENU_BY, MENU_BW, MENU_BH);
	W2Frame f;
	if (g_art.widget && !w2_sprite_frame(g_art.widget, g_art.widget_n, pressed ? 5 : 4, &f))
		draw_gfu(g_art.widget, g_art.widget_n, pressed ? 5 : 4, MENU_BX - f.x, MENU_BY - f.y, &g_full);
	else bevel(MENU_BX, MENU_BY, MENU_BW, MENU_BH, pressed);
	text_c(FONT_GAME, g_menu_press ? FC_WHITE : FC_YELLOW, MENU_BX + MENU_BW / 2 + pressed * 2, MENU_BY + 4 + pressed * 2, "Menu (F10)");
}

static const char *const g_gm_label[GM_N] = { "Return to Game (Esc)", "Options (O)", "Objectives (B)", "Quit to Menu (Q)", "Exit Program (X)" };
static const char *const g_opt_label[OPT_N] = { "Sound Volume", "Music Volume", "Game Speed", "Scroll Speed" };
static const char *const g_opt_short[OPT_N] = { "Sound", "Music", "Speed", "Scroll" };
static int g_opt_drag = -1, g_opt_dirty;

static int *opt_val(int i, int *lo, int *hi)
{
	*lo = 0; *hi = 100;
	switch (i) {
	case 0: return &w2_cfg.sfx_volume;
	case 1: return &w2_cfg.music_volume;
	case 2: *lo = 1; *hi = 5; return &w2_cfg.game_speed;
	default: *lo = 1; *hi = 5; return &w2_cfg.scroll_speed;
	}
}

static void gm_button(int x, int y, int i, const char *label)
{
	W2Frame f;
	int pressed = g_gm_press == i && in_rect(g_mx, g_my, x, y, MENU_BW, MENU_BH);
	if (g_art.widget && !w2_sprite_frame(g_art.widget, g_art.widget_n, pressed ? 5 : 4, &f))
		draw_gfu(g_art.widget, g_art.widget_n, pressed ? 5 : 4, x - f.x, y - f.y, &g_full);
	else bevel(x, y, MENU_BW, MENU_BH, pressed);
	text_c(FONT_GAME, in_rect(g_mx, g_my, x, y, MENU_BW, MENU_BH) ? FC_WHITE : FC_YELLOW,
	       x + MENU_BW / 2 + pressed * 2, y + 4 + pressed * 2, label);
}

/* the scenario objectives page (g_menu == 3): War Strings 53 title, the level's objective text
 * (War Strings 54 via campaign.c), word-wrapped into the panel */
static void draw_objectives(void)
{
	const char *t = camp_objectives(), *title = assets_string(53, 0);
	char line[64];
	int y = GM_Y + 40, w = GM_W - 28;
	text_c(FONT_LARGE, FC_YELLOW, GM_X + GM_W / 2, GM_Y + 14, title && *title ? title : "Objectives");
	while (t && *t && y < OPT_OK_Y - 12) {
		int len = 0, brk = -1;
		while (t[len] && t[len] != '\n' && len < (int)sizeof line - 1) {
			line[len] = t[len]; line[len + 1] = 0;
			if (t[len] == ' ') brk = len;
			if (assets_text_width(FONT_GAME, line) > w && brk > 0) { len = brk; break; }
			len++;
		}
		line[len] = 0;
		text(FONT_GAME, FC_WHITE, GM_X + 14, y, line);
		y += assets_font_height(FONT_GAME) + 3;
		t += len;
		while (*t == ' ') t++;
		if (*t == '\n') t++;
	}
	gm_button(GM_BX, OPT_OK_Y, 0, "OK (Esc)");
}

static void draw_game_menu(void)
{
	int i;
	draw_gfu(g_art.info, g_art.info_n, 1, GM_X, GM_Y, &g_full);
	if (g_menu == 3) { draw_objectives(); return; }
	if (g_menu == 2) {
		static const char *const spd[5] = { "Slowest", "Slow", "Normal", "Fast", "Fastest" };
		text_c(FONT_LARGE, FC_YELLOW, GM_X + GM_W / 2, GM_Y + 14, "Options");
		for (i = 0; i < OPT_N; i++) {
			int lo, hi, *v = opt_val(i, &lo, &hi), y = OPT_Y0 + i * OPT_DY, w;
			char b[16];
			if (i < 2) snprintf(b, sizeof b, "%d%%", *v); else snprintf(b, sizeof b, "%s", spd[(*v - 1) % 5]);
			text(FONT_GAME, FC_YELLOW, OPT_BX, y, L ? g_opt_short[i] : g_opt_label[i]);   /* 3x5 font is wider */
			text_r(FONT_GAME, FC_WHITE, OPT_BX + OPT_BW, y, b);
			w = (OPT_BW - 4) * (*v - lo) / (hi - lo);
			fill(OPT_BX, y + 13, OPT_BW, OPT_BH, PEN_BARBG);
			if (w > 0) fill(OPT_BX + 2, y + 15, w, OPT_BH - 4, PEN_YELLOW);
			bevel(OPT_BX, y + 13, OPT_BW, OPT_BH, 1);
		}
		gm_button(GM_BX, OPT_OK_Y, 0, "OK (Esc)");
		return;
	}
	text_c(FONT_LARGE, FC_YELLOW, GM_X + GM_W / 2, GM_Y + 14, "Game Menu");
	for (i = 0; i < GM_N; i++) gm_button(GM_BX, g_gm_by[i], i, g_gm_label[i]);
}

static void update_cursor(void)
{
	int shape = UI_CUR_POINT, px, py, s;
	if (g_menu) shape = UI_CUR_POINT;
	else if (g_edge_dir >= 0) shape = UI_CUR_ARROW_N + g_edge_dir;
	else if (g_drag == 2) shape = UI_CUR_CROSS;
	else if (in_map(g_mx, g_my) && view_to_map(g_mx, g_my, &px, &py)) {
		if (g_mode == MODE_TARGET) {
			s = pick_unit(px, py);
			shape = s < 0 ? UI_CUR_TARGET : unit_is_enemy(s, sel_lead() >= 0 ? sel_lead() : s) || (w2.player[me()].enemies >> w2.unit[s].owner & 1)
			        ? UI_CUR_TARGET_RED : is_own(s) ? UI_CUR_TARGET_GREEN : UI_CUR_TARGET;
		} else if (g_mode == MODE_NORMAL && pick_unit(px, py) >= 0) shape = UI_CUR_GLASS;
	} else if (g_mode == MODE_TARGET && in_rect(g_mx, g_my, MINI_X, MINI_Y, MINI_S, MINI_S)) shape = UI_CUR_TARGET;
	ui_cursor_set(shape, local_race());
}

/* ------------------------------------------------------------------ panel dirty regions
 * The panel around the map is drawn (and converted by c2p / blitted) only where its content changed.
 * Each region has a signature of everything it shows; a region is redrawn when its signature changes,
 * on ui_panel_force(), and all of them every UI_REFRESH frames as a safety net for anything the
 * signatures miss. The map viewport is redrawn every frame by game.c. Rectangles in hires units. */
enum { R_MINI, R_INFO, R_TOP, R_BOT, R_FILL, R_COUNT };
static const short g_reg[R_COUNT][4] = {
	{ 0, 0, PANEL_W, INFO_Y },              /* menu button + minimap */
	{ 0, INFO_Y, PANEL_W, 480 - INFO_Y },   /* selection info + command card */
	{ RES_X, 0, FILL_X - RES_X, 16 },       /* resources and food */
	{ RES_X, STATUS_Y, FILL_X - RES_X, 16 },/* status line */
	{ FILL_X, 0, 640 - FILL_X, 480 }        /* right border (static) */
};
#define UI_REFRESH 50
static uint32_t g_reg_sig[R_COUNT];
static uint8_t  g_reg_drawn[R_COUNT];
static int g_reg_force = 1, g_reg_frames;

void ui_panel_force(void) { g_reg_force = 1; }

static uint32_t mix(uint32_t h, uint32_t v)
{
	h ^= v; h *= 16777619u; h ^= h >> 15;
	return h;
}

static void panel_sigs(uint32_t *sg)
{
	const W2Player *pl = &w2.player[me()];
	int i, c, si;
	uint32_t h;
	/* menu button + minimap: the minimap follows units/fog, refreshed every 8 cycles or when the view moves */
	h = mix(2166136261u, (uint32_t)g_menu_press | (uint32_t)in_rect(g_mx, g_my, MENU_BX, MENU_BY, MENU_BW, MENU_BH) << 1);
	h = mix(h, w2.cycle >> 3); h = mix(h, (uint32_t)view.map_x); h = mix(h, (uint32_t)view.map_y);
	sg[R_MINI] = mix(h, (uint32_t)g_mode);
	/* selection info + card (stats with upgrades are covered by the periodic refresh) */
	h = mix(2166136261u, w2.select_n);
	for (i = 0; i < w2.select_n; i++) {
		const W2Unit *u = &w2.unit[w2.select[i]];
		h = mix(h, (uint32_t)w2.select[i] | (uint32_t)u->type << 16 | (uint32_t)u->owner << 24);
		h = mix(h, (uint32_t)u->hp | (uint32_t)u->mana << 16 | (uint32_t)u->order << 24);
		h = mix(h, (uint32_t)u->progress | (uint32_t)u->queue_type << 16 | (uint32_t)(u->flags & W2_UF_UNDERCONST) << 24);
		h = mix(h, u->res_left);
	}
	for (i = 0; i < 9; i++) h = mix(h, (uint32_t)(uintptr_t)g_card[i]);
	c = card_at(g_mx, g_my);
	h = mix(h, (uint32_t)(g_btn_press + 1) | (uint32_t)(c + 1) << 8 | (uint32_t)g_level << 16 | (uint32_t)g_mode << 24);
	h = mix(h, (uint32_t)pl->food_used | (uint32_t)pl->food_supply << 16);
	sg[R_INFO] = h;
	/* resources */
	h = mix(2166136261u, (uint32_t)pl->gold); h = mix(h, (uint32_t)pl->wood); h = mix(h, (uint32_t)pl->oil);
	sg[R_TOP] = mix(h, (uint32_t)pl->food_used | (uint32_t)pl->food_supply << 16);
	/* status line: what the pointer is over */
	si = in_rect(g_mx, g_my, 0, INFO_Y, PANEL_W, 176) ? sel_icon_at(g_mx, g_my) : -1;
	h = mix(2166136261u, (uint32_t)(c + 1) | (uint32_t)(si + 1) << 8 | (uint32_t)g_menu << 16);
	h = mix(h, (uint32_t)(uintptr_t)(c >= 0 ? g_card[c] : NULL));
	h = mix(h, si >= 0 ? w2.unit[w2.select[si]].type : 0xFFFF);
	sg[R_BOT] = mix(h, (uint32_t)in_rect(g_mx, g_my, MENU_BX, MENU_BY, MENU_BW, MENU_BH));
	sg[R_FILL] = 0;
}

static void panel_draw(void)
{
	uint32_t sg[R_COUNT];
	int r, all = g_reg_force || ++g_reg_frames >= UI_REFRESH || !w2_dirty;
	if (all) { g_reg_force = 0; g_reg_frames = 0; }
	panel_sigs(sg);
	for (r = 0; r < R_COUNT; r++) {
		g_reg_drawn[r] = (uint8_t)(all || sg[r] != g_reg_sig[r]);
		g_reg_sig[r] = sg[r];
	}
	if (g_reg_drawn[R_MINI]) {
		draw_image(IMG_MENUBTN, 0, 0);
		draw_image(IMG_MINIMAP, 0, 24);
		draw_menu_button();
		minimap_draw(MINI_X, MINI_Y, MINI_S);
	}
	if (g_reg_drawn[R_INFO]) {
		draw_image(IMG_BUTTONS, 0, BTNP_Y);
		draw_info();
		draw_card();
	}
	if (g_reg_drawn[R_TOP]) { draw_image(IMG_RESOURCE, RES_X, 0); draw_resources(); }
	if (g_reg_drawn[R_BOT]) { draw_image(IMG_STATUS, RES_X, STATUS_Y); draw_status(); }
	if (g_reg_drawn[R_FILL]) draw_image(IMG_FILLER, FILL_X, 0);
}

int ui_dirty_rects(short (*out)[4], int max)
{
	int r, n = 0;
	for (r = 0; r < R_COUNT && n < max; r++) {
		if (!g_reg_drawn[r]) continue;
		out[n][0] = g_reg[r][0]; out[n][1] = g_reg[r][1]; out[n][2] = g_reg[r][2]; out[n][3] = g_reg[r][3];
		n++;
	}
	return n;
}

void ui_draw(void)
{
	B = view.buf; P = view.pitch;
	if (!B) return;
	L = assets_lores() ? 1 : 0;
	SW = 640 >> L; SH = 480 >> L;
	g_full.x0 = 0; g_full.y0 = 0; g_full.x1 = SW; g_full.y1 = SH;
	g_mapclip.x0 = view.scr_x >> L; g_mapclip.y0 = view.scr_y >> L;
	g_mapclip.x1 = (view.scr_x + view.scr_w) >> L; g_mapclip.y1 = (view.scr_y + view.scr_h) >> L;
	pens_update();
	art_load();
	sel_prune();
	card_build();

	draw_map_overlays();
	draw_messages();
	panel_draw();             /* the panel regions whose content changed (dirty rectangles) */
	if (g_menu) draw_game_menu();
	update_cursor();
}

/* ------------------------------------------------------------------ input */
static int key_char(int k)
{
	static const char r1[] = "qwertyuiop", r2[] = "asdfghjkl", r3[] = "zxcvbnm";
	if (k >= 0x10 && k <= 0x19) return r1[k - 0x10];
	if (k >= 0x20 && k <= 0x28) return r2[k - 0x20];
	if (k >= 0x31 && k <= 0x37) return r3[k - 0x31];
	if (k >= 0x01 && k <= 0x09) return '0' + k;
	if (k == 0x0A) return '0';
	if (k == K_ESC) return 27;
	return 0;
}

static void options_close(void)
{
	click_sound();
	g_menu = 1; g_gm_press = -1; g_opt_drag = -1;
	if (g_opt_dirty) {
		g_opt_dirty = 0;
		if (w2cfg_save(&w2_cfg, W2CFG_FILE)) w2_log("ui: could not write %s", W2CFG_FILE);
		else w2_log("ui: options saved (sfx %d music %d speed %d scroll %d)", w2_cfg.sfx_volume,
		            w2_cfg.music_volume, w2_cfg.game_speed, w2_cfg.scroll_speed);
	}
}

static void menu_choose(int i)
{
	click_sound();
	g_gm_press = -1;
	if (i == 1) { g_menu = 2; g_opt_drag = -1; return; }
	if (i == 2) { g_menu = 3; return; }
	g_menu = 0;
	if (i == 3) game_end(3);
	else if (i == 4) { g_quit = 1; game_quit(); }
}

/* set slider row i from the mouse x; values apply at once (snd/main loop read w2_cfg live) */
static void opt_set(int i)
{
	int lo, hi, *v = opt_val(i, &lo, &hi), nv, was = *v;
	nv = lo + ((g_mx - OPT_BX - 2) * (hi - lo) + (OPT_BW - 4) / 2) / (OPT_BW - 4);
	if (nv < lo) nv = lo;
	if (nv > hi) nv = hi;
	if (nv == was) return;
	*v = nv; g_opt_dirty = 1;
	if (i == 1 && was == 0 && nv > 0 && w2_cfg.music && !snd_music_active())
		snd_music_play(SND_MUS_GAME, local_race());
	if (i == 0 && hi == 100 && (nv / 10) != (was / 10)) click_sound();
}

static int opt_row_at(int x, int y)
{
	int i;
	for (i = 0; i < OPT_N; i++)
		if (in_rect(x, y, OPT_BX - 4, OPT_Y0 + i * OPT_DY + 9, OPT_BW + 8, OPT_BH + 8)) return i;
	return -1;
}

static int opt_event(const AmigaGfxEvent *e)
{
	int r, on_ok = in_rect(g_mx, g_my, GM_BX, OPT_OK_Y, MENU_BW, MENU_BH);
	if (e->type == AMIGAGFX_EV_KEY) {
		int k = e->code & 0x7F, c = key_char(k);
		if (e->code & 0x80) return 1;
		if (k == K_ESC || k == K_F10 || c == 'o' || k == 0x44 /* Return */) options_close();
		return 1;
	}
	if (e->type == AMIGAGFX_EV_MOUSEDOWN && e->code == AMIGAGFX_BUTTON_LEFT) {
		if (on_ok) g_gm_press = 0;
		else if ((r = opt_row_at(g_mx, g_my)) >= 0) { g_opt_drag = r; opt_set(r); }
	} else if (e->type == AMIGAGFX_EV_MOUSEMOVE) {
		if (g_opt_drag >= 0) opt_set(g_opt_drag);
	} else if (e->type == AMIGAGFX_EV_MOUSEUP && e->code == AMIGAGFX_BUTTON_LEFT) {
		if (g_gm_press == 0 && on_ok) options_close();
		g_gm_press = -1; g_opt_drag = -1;
	}
	return 1;
}

/* objectives page: OK / Esc / Return back to the game menu */
static int obj_event(const AmigaGfxEvent *e)
{
	int on_ok = in_rect(g_mx, g_my, GM_BX, OPT_OK_Y, MENU_BW, MENU_BH);
	if (e->type == AMIGAGFX_EV_KEY) {
		int k = e->code & 0x7F;
		if (e->code & 0x80) return 1;
		if (k == K_ESC || k == K_F10 || k == 0x44 || key_char(k) == 'b') { click_sound(); g_menu = 1; g_gm_press = -1; }
		return 1;
	}
	if (e->type == AMIGAGFX_EV_MOUSEDOWN && e->code == AMIGAGFX_BUTTON_LEFT && on_ok) g_gm_press = 0;
	else if (e->type == AMIGAGFX_EV_MOUSEUP && e->code == AMIGAGFX_BUTTON_LEFT) {
		if (g_gm_press == 0 && on_ok) { click_sound(); g_menu = 1; }
		g_gm_press = -1;
	}
	return 1;
}

static int gm_event(const AmigaGfxEvent *e)
{
	int i;
	if (g_menu == 2) return opt_event(e);
	if (g_menu == 3) return obj_event(e);
	if (e->type == AMIGAGFX_EV_KEY) {
		int k = e->code & 0x7F, c = key_char(k);
		if (e->code & 0x80) return 1;
		if (k == K_ESC || k == K_F10 || c == 'r') menu_choose(0);
		else if (c == 'o') menu_choose(1);
		else if (c == 'b') menu_choose(2);
		else if (c == 'q') menu_choose(3);
		else if (c == 'x') menu_choose(4);
		return 1;
	}
	for (i = 0; i < GM_N; i++) if (in_rect(g_mx, g_my, GM_BX, g_gm_by[i], MENU_BW, MENU_BH)) break;
	if (e->type == AMIGAGFX_EV_MOUSEDOWN && e->code == AMIGAGFX_BUTTON_LEFT) g_gm_press = i < GM_N ? i : -1;
	else if (e->type == AMIGAGFX_EV_MOUSEUP && e->code == AMIGAGFX_BUTTON_LEFT) {
		if (g_gm_press >= 0 && g_gm_press == i) menu_choose(i);
		g_gm_press = -1;
	}
	return 1;
}

static void open_menu(void)
{
	click_sound();
	g_menu = 1; g_gm_press = -1; g_drag = 0; g_mm_drag = 0; g_btn_press = -1;
}

static void minimap_at(int sx, int sy, int center)
{
	int tx, ty;
	if (!minimap_to_map(sx, sy, &tx, &ty)) return;
	if (center) view_center_on(tx * 32 + 16, ty * 32 + 16);
	else if (sel_own()) {
		int i;
		for (i = 0; i < w2.select_n; i++) if (!is_bld(w2.select[i])) cmd_move(w2.select[i], tx, ty);
		mark(tx * 32 + 16, ty * 32 + 16);
		ack();
	}
}

static int mouse_down_left(void)
{
	int i, px, py;
	if (in_rect(g_mx, g_my, MENU_BX, MENU_BY, MENU_BW, MENU_BH)) { g_menu_press = 1; return 1; }
	if (in_rect(g_mx, g_my, MINI_X, MINI_Y, MINI_S, MINI_S)) {
		int tx, ty;
		if (g_mode == MODE_TARGET && minimap_to_map(g_mx, g_my, &tx, &ty)) { do_target(tx * 32 + 16, ty * 32 + 16, -1); return 1; }
		g_mm_drag = 1; minimap_at(g_mx, g_my, 1);
		return 1;
	}
	card_build();
	if ((i = card_at(g_mx, g_my)) >= 0) { g_btn_press = i; return 1; }
	if (in_rect(g_mx, g_my, 0, INFO_Y, PANEL_W, 176)) {
		i = sel_icon_at(g_mx, g_my);
		if (i >= 0 && w2.select_n > 1) {
			int s = w2.select[i];
			if (g_shift) sel_remove(s); else { sel_clear(); sel_add(s); selection_changed(); }
		} else if (i == 0) {
			int cx, cy; unit_center(w2.select[0], &cx, &cy); view_center_on(cx, cy);
		}
		return 1;
	}
	if (in_map(g_mx, g_my) && view_to_map(g_mx, g_my, &px, &py)) {
		if (g_mode == MODE_PLACE) { try_place(px, py); return 1; }
		if (g_mode == MODE_TARGET) { do_target(px, py, pick_unit(px, py)); return 1; }
		g_drag = 1; g_dx0 = g_mx; g_dy0 = g_my;
	}
	return 1;
}

static int mouse_up_left(void)
{
	if (g_menu_press) {
		g_menu_press = 0;
		if (in_rect(g_mx, g_my, MENU_BX, MENU_BY, MENU_BW, MENU_BH)) open_menu();
		return 1;
	}
	g_mm_drag = 0;
	if (g_btn_press >= 0) {
		int i = g_btn_press;
		g_btn_press = -1;
		card_build();
		if (card_at(g_mx, g_my) == i) do_button(g_card[i]);
		return 1;
	}
	if (g_drag == 2) box_select(g_dx0, g_dy0, g_mx, g_my);
	else if (g_drag == 1) click_select(g_dx0, g_dy0);
	g_drag = 0;
	return 1;
}

static int mouse_down_right(void)
{
	int px, py;
	if (g_mode != MODE_NORMAL) { g_mode = MODE_NORMAL; return 1; }
	if (in_rect(g_mx, g_my, MINI_X, MINI_Y, MINI_S, MINI_S)) { minimap_at(g_mx, g_my, 0); return 1; }
	if (in_map(g_mx, g_my) && view_to_map(g_mx, g_my, &px, &py)) smart(px, py, pick_unit(px, py));
	return 1;
}

static int key_event(int code)
{
	int up = code & 0x80, k = code & 0x7F, c, i;
	switch (k) {
	case K_LSHIFT: case K_RSHIFT: g_shift = !up; return 1;
	case K_CTRL: g_ctrl = !up; return 1;
	case K_LALT: case K_RALT: g_alt = !up; return 1;
	case K_UP: case K_DOWN: case K_LEFT: case K_RIGHT: return 0;   /* game.c scrolls */
	}
	if (up) return 0;
	if (k == K_F10) { open_menu(); return 1; }
	c = key_char(k);
	if (!c) return 0;
	if (c >= '1' && c <= '9') {
		if (g_ctrl) group_set(c - '0'); else group_recall(c - '0', g_shift);
		return 1;
	}
	card_build();
	for (i = 0; i < 9; i++) if (g_card[i] && g_card[i]->key == c) { do_button(g_card[i]); return 1; }
	if (c == 27) {
		if (g_mode != MODE_NORMAL || g_level) { g_mode = MODE_NORMAL; g_level = 0; }
		else open_menu();
		return 1;
	}
	return 0;
}

int ui_event(const void *ev)
{
	const AmigaGfxEvent *e = (const AmigaGfxEvent *)ev;
	int sh = assets_lores() ? 1 : 0;
	if (!e) return 0;
	if (e->type == AMIGAGFX_EV_MOUSEMOVE || e->type == AMIGAGFX_EV_MOUSEDOWN || e->type == AMIGAGFX_EV_MOUSEUP) {
		g_mx = e->x << sh; g_my = e->y << sh; g_mouse_seen = 1;
	}
	if (g_menu && (e->type == AMIGAGFX_EV_KEY || e->type == AMIGAGFX_EV_MOUSEDOWN || e->type == AMIGAGFX_EV_MOUSEUP || e->type == AMIGAGFX_EV_MOUSEMOVE))
		return gm_event(e);
	switch (e->type) {
	case AMIGAGFX_EV_MOUSEMOVE:
		if (g_mm_drag) minimap_at(g_mx, g_my, 1);
		if (g_drag == 1 && (abs(g_mx - g_dx0) > 4 || abs(g_my - g_dy0) > 4)) g_drag = 2;
		return 1;
	case AMIGAGFX_EV_MOUSEDOWN:
		return e->code == AMIGAGFX_BUTTON_LEFT ? mouse_down_left() : mouse_down_right();
	case AMIGAGFX_EV_MOUSEUP:
		return e->code == AMIGAGFX_BUTTON_LEFT ? mouse_up_left() : 1;
	case AMIGAGFX_EV_KEY:
		return key_event(e->code);
	}
	return 0;
}

/* ------------------------------------------------------------------ debug */
const char *ui_debug_state(void)
{
	static char s[400];
	int i, n;
	card_build();
	n = snprintf(s, sizeof s, "mode=%s level=%d menu=%d sel=%d",
	             g_mode == MODE_TARGET ? "target" : g_mode == MODE_PLACE ? "place" : "normal", g_level, g_menu, w2.select_n);
	for (i = 0; i < w2.select_n && n < (int)sizeof s - 20; i++) n += snprintf(s + n, sizeof s - n, "%s%02x", i ? "," : "[", w2.unit[w2.select[i]].type);
	if (w2.select_n && n < (int)sizeof s - 2) n += snprintf(s + n, sizeof s - n, "]");
	if (n < (int)sizeof s - 8) n += snprintf(s + n, sizeof s - n, " card=");
	for (i = 0; i < 9 && n < (int)sizeof s - 16; i++)
		n += g_card[i] ? snprintf(s + n, sizeof s - n, "%d:%d/%c ", i + 1, g_card[i]->icon, g_card[i]->key == 27 ? '^' : g_card[i]->key)
		               : snprintf(s + n, sizeof s - n, "%d:- ", i + 1);
	if (g_status[0] && n < (int)sizeof s - 4) snprintf(s + n, sizeof s - n, "status='%s'", g_status);
	return s;
}
