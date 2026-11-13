/* mapdraw.c - terrain of the viewport, fog of war, minimap, view scrolling. See w2.h [mapdraw.c] and
 * mapdraw.h.
 *
 * Drawing works in BUFFER pixels: hires 1:1 (32x32 tiles), lores halved (16x16 tiles from
 * assets_megatile(), which already returns the halved art). view.* is in hires; >> view.lores converts.
 * The terrain is one block copy per visible tile (32-bit moves when aligned), cut at the viewport edges.
 *
 * Fog: per tile, for the local player. Unexplored = black, explored but not seen = dithered (every other
 * pixel black, anchored to the map so it does not shimmer while scrolling) or palette-shaded, seen = as is.
 * fog_update keeps the visible/explored bits of all eight players (AI and targeting use them).
 *
 * Minimap: terrain image (average colour of each megatile, computed once per era) cached until the map
 * changes; the fogged copy is rebuilt only after a fog_update; per frame: copy + unit dots + view frame.
 * Plain C, no floats, no per-pixel divisions, no per-frame allocation. */
#include <stdlib.h>
#include <string.h>
#include "w2.h"
#include "assets.h"
#include "w2cfg.h"
#include "mapdraw.h"

int mapdraw_fog_mode = MAPDRAW_FOG_DITHER;
int mapdraw_reveal = 0;

/* ---------------------------------------------------------------- era tables (palette dependent) */
static int g_era_c = -2, g_lo_c = -1;
static unsigned char g_shade[256];
static int g_black, g_white, g_yellow, g_grey;
static unsigned char *g_mega_col; static int g_mega_col_n;
static unsigned g_fog_serial = 1;

static int pal_nearest(const unsigned char *pal, int r, int g, int b)
{
	int i, best = 0; long bd = 0x7fffffffL;
	for (i = 0; i < 256; i++) {
		long dr, dg, db, d;
		if (i >= 17 && i <= 19 && !assets_ehb()) continue;   /* the hardware pointer's pens (EHB: shared, real colours) */
		if (i >= 208 && i <= 239) continue;        /* player colour ramps (remapped on sprites) */
		dr = pal[i * 3] - r; dg = pal[i * 3 + 1] - g; db = pal[i * 3 + 2] - b;
		d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
		if (d < bd) { bd = d; best = i; }
	}
	return best;
}

static void build_era_tables(void)
{
	const unsigned char *pal = assets_palette();
	int i, n = assets_megatile_count(), ts = assets_tile_size(), np = ts * ts;
	for (i = 0; i < 256; i++) g_shade[i] = (unsigned char)pal_nearest(pal, pal[i * 3] >> 1, pal[i * 3 + 1] >> 1, pal[i * 3 + 2] >> 1);
	g_black  = pal_nearest(pal, 0, 0, 0);
	g_white  = pal_nearest(pal, 255, 255, 255);
	g_yellow = pal_nearest(pal, 252, 220, 0);
	g_grey   = pal_nearest(pal, 150, 150, 150);
	if (n > g_mega_col_n) {
		unsigned char *p = (unsigned char *)realloc(g_mega_col, (size_t)n);
		if (p) { g_mega_col = p; g_mega_col_n = n; }
	}
	for (i = 0; i < n && i < g_mega_col_n; i++) {
		const unsigned char *t = assets_megatile(i);
		long r = 0, g = 0, b = 0; int k;
		if (!t) { g_mega_col[i] = (unsigned char)g_black; continue; }
		for (k = 0; k < np; k++) { const unsigned char *c = pal + t[k] * 3; r += c[0]; g += c[1]; b += c[2]; }
		g_mega_col[i] = (unsigned char)pal_nearest(pal, (int)(r / np), (int)(g / np), (int)(b / np));
	}
	g_era_c = assets_era();
	g_lo_c = assets_lores();
	minimap_invalidate();
}

static void ensure_era(void)
{
	if (g_era_c != assets_era() || g_lo_c != assets_lores()) build_era_tables();
}

const unsigned char *mapdraw_shade_table(void) { ensure_era(); return g_shade; }
int mapdraw_black(void) { ensure_era(); return g_black; }
int mapdraw_white(void) { ensure_era(); return g_white; }

/* ---------------------------------------------------------------- view */
static void view_defaults(void)
{
	if (view.scr_w <= 0 || view.scr_h <= 0) {
		view.scr_x = 176; view.scr_y = 16; view.scr_w = 448; view.scr_h = 448;
	}
}

static void view_clamp(void)
{
	int mx = w2.map.w * W2_TILE - view.scr_w, my = w2.map.h * W2_TILE - view.scr_h;
	if (mx < 0) mx = 0;
	if (my < 0) my = 0;
	if (view.map_x > mx) view.map_x = mx;
	if (view.map_y > my) view.map_y = my;
	if (view.map_x < 0) view.map_x = 0;
	if (view.map_y < 0) view.map_y = 0;
}

void view_scroll(int dx, int dy)
{
	view_defaults();
	view.map_x += dx; view.map_y += dy;
	view_clamp();
}

void view_center_on(int px, int py)
{
	view_defaults();
	view.map_x = px - view.scr_w / 2;
	view.map_y = py - view.scr_h / 2;
	view_clamp();
}

int view_to_map(int sx, int sy, int *px, int *py)
{
	int in = sx >= view.scr_x && sy >= view.scr_y && sx < view.scr_x + view.scr_w && sy < view.scr_y + view.scr_h;
	*px = view.map_x + sx - view.scr_x;
	*py = view.map_y + sy - view.scr_y;
	return in;
}

static void read_cfg(void)
{
	const char *f = w2cfg_extra("fog");
	mapdraw_fog_mode = MAPDRAW_FOG_DITHER;
	if (f) {
		if (!strcmp(f, "shade")) mapdraw_fog_mode = MAPDRAW_FOG_SHADE;
		else if (!strcmp(f, "off")) mapdraw_fog_mode = MAPDRAW_FOG_OFF;
	}
	mapdraw_reveal = w2cfg_extra_int("reveal", 0) ? 1 : 0;
}

void mapdraw_reset(void)
{
	read_cfg();
	build_era_tables();
	view_defaults();
	view.map_x = view.map_y = 0;
	view_clamp();
	g_fog_serial++;
}

/* ---------------------------------------------------------------- fog */
static unsigned char g_span[MAPDRAW_SIGHT_MAX + 1][MAPDRAW_SIGHT_MAX + 1];
static int g_span_ok;

static void build_span(void)
{
	int r, dy, s;
	for (r = 0; r <= MAPDRAW_SIGHT_MAX; r++)
		for (dy = 0; dy <= MAPDRAW_SIGHT_MAX; dy++) {
			if (dy > r) { g_span[r][dy] = 0; continue; }
			/* circle of radius r + 1/2: s*s + dy*dy <= r*r + r */
			for (s = r; s > 0 && s * s + dy * dy > r * r + r; s--) ;
			g_span[r][dy] = (unsigned char)s;
		}
	g_span_ok = 1;
}

int mapdraw_sight_span(int r, int dy)
{
	if (!g_span_ok) build_span();
	if (r < 0 || dy < 0 || r > MAPDRAW_SIGHT_MAX || dy > r) return -1;
	return g_span[r][dy];
}

void fog_update(void)
{
	int w = w2.map.w, h = w2.map.h, n = w * h, i, slot;
	uint8_t *vis = w2.map.visible, *exp = w2.map.explored;
	if (!g_span_ok) build_span();
	if (n <= 0) return;
	memset(vis, 0, (size_t)n);
	for (slot = 0; slot < w2.unit_hi; slot++) {
		const W2Unit *u = &w2.unit[slot];
		const w2_unit_type *t;
		int r, x0, y0, x1, y1, y, bit;
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN)) || u->owner >= 8) continue;
		t = w2_unit(u->type);
		if (!t) continue;
		r = econ_stat(slot, W2_STAT_SIGHT);      /* with researched bonuses (Scouting, Longbow) */
		if (r <= 0) r = t->sight;                /* economy not linked (module tests) */
		if (u->flags & W2_UF_UNDERCONST) r = 1;   /* NIEPEWNE: a site under construction sees its surroundings only */
		if (r < 1) r = 1;
		if (r > MAPDRAW_SIGHT_MAX) r = MAPDRAW_SIGHT_MAX;
		x0 = u->tx; y0 = u->ty;
		x1 = x0 + (t->tile_w ? t->tile_w : 1) - 1;
		y1 = y0 + (t->tile_h ? t->tile_h : 1) - 1;
		bit = 1 << u->owner;
		for (y = y0 - r; y <= y1 + r; y++) {
			int dy, s, xa, xb;
			uint8_t *row;
			if (y < 0) continue;
			if (y >= h) break;
			dy = y < y0 ? y0 - y : y > y1 ? y - y1 : 0;
			s = g_span[r][dy];
			xa = x0 - s; xb = x1 + s;
			if (xa < 0) xa = 0;
			if (xb >= w) xb = w - 1;
			row = vis + y * w;
			for (; xa <= xb; xa++) row[xa] |= (uint8_t)bit;
		}
	}
	for (i = 0; i < n; i++) exp[i] |= vis[i];
	g_fog_serial++;
}

static int local_bit(void) { return w2.local_player < 8 ? 1 << w2.local_player : 0; }

int fog_visible(int tx, int ty)
{
	if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 0;
	if (mapdraw_reveal) return 1;
	return (w2.map.visible[ty * w2.map.w + tx] & local_bit()) != 0;
}

int fog_explored(int tx, int ty)
{
	if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 0;
	if (mapdraw_reveal) return 1;
	return (w2.map.explored[ty * w2.map.w + tx] & local_bit()) != 0;
}

/* 0 unexplored, 1 explored (fogged), 2 visible - for the local player */
static int tile_state(int i, int bit)
{
	if (mapdraw_reveal) return 2;
	if (w2.map.visible[i] & bit) return 2;
	if (w2.map.explored[i] & bit) return mapdraw_fog_mode == MAPDRAW_FOG_OFF ? 2 : 1;
	return 0;
}

/* ---------------------------------------------------------------- terrain */
typedef uint32_t __attribute__((may_alias)) u32a;

static void copy_rect(uint8_t *d, int pitch, const uint8_t *s, int spitch, int w, int h)
{
	if (!(((uintptr_t)d | (uintptr_t)s | (unsigned)w | (unsigned)pitch | (unsigned)spitch) & 3)) {
		int n4 = w >> 2;
		for (; h > 0; h--, d += pitch, s += spitch) {
			u32a *dd = (u32a *)d; const u32a *ss = (const u32a *)s; int k;
			for (k = 0; k < n4; k++) dd[k] = ss[k];
		}
	} else {
		for (; h > 0; h--, d += pitch, s += spitch) memcpy(d, s, (size_t)w);
	}
}

static void fill_rect(uint8_t *d, int pitch, int w, int h, int c)
{
	for (; h > 0; h--, d += pitch) memset(d, c, (size_t)w);
}

/* fogged copy, dither: pixel (map x + map y) even -> black. phase = parity of the first pixel. */
static void copy_dither(uint8_t *d, int pitch, const uint8_t *s, int spitch, int w, int h, int phase)
{
	int x;
	uint8_t b = (uint8_t)g_black;
	for (; h > 0; h--, d += pitch, s += spitch, phase ^= 1) {
		memcpy(d, s, (size_t)w);
		for (x = phase; x < w; x += 2) d[x] = b;
	}
}

static void copy_shade(uint8_t *d, int pitch, const uint8_t *s, int spitch, int w, int h)
{
	int x;
	for (; h > 0; h--, d += pitch, s += spitch)
		for (x = 0; x < w; x++) d[x] = g_shade[s[x]];
}

void mapdraw_draw(void)
{
	int lo = view.lores ? 1 : 0, ts = lo ? 16 : 32, sh = lo ? 4 : 5;
	int bx0, by0, bw, bh, mx, my, tx0, ty0, tx1, ty1, tx, ty, bit, mw = w2.map.w, mh = w2.map.h;
	uint8_t *buf = view.buf;
	int pitch = view.pitch;
	if (!buf) return;
	ensure_era();
	view_defaults();
	bx0 = view.scr_x >> lo; by0 = view.scr_y >> lo;
	bw = view.scr_w >> lo;  bh = view.scr_h >> lo;
	mx = view.map_x >> lo;  my = view.map_y >> lo;
	if (bw <= 0 || bh <= 0) return;
	tx0 = mx >> sh; ty0 = my >> sh;
	tx1 = (mx + bw - 1) >> sh; ty1 = (my + bh - 1) >> sh;
	bit = local_bit();
	for (ty = ty0; ty <= ty1; ty++) {
		int ys = (ty << sh) - my, sy0 = 0, hh = ts;      /* tile top in viewport rows */
		if (ys < 0) { sy0 = -ys; hh += ys; ys = 0; }
		if (ys + hh > bh) hh = bh - ys;
		if (hh <= 0) continue;
		for (tx = tx0; tx <= tx1; tx++) {
			int xs = (tx << sh) - mx, sx0 = 0, ww = ts, i, st;
			uint8_t *d;
			const uint8_t *src;
			if (xs < 0) { sx0 = -xs; ww += xs; xs = 0; }
			if (xs + ww > bw) ww = bw - xs;
			if (ww <= 0) continue;
			d = buf + (long)(by0 + ys) * pitch + bx0 + xs;
			if (tx >= mw || ty >= mh) { fill_rect(d, pitch, ww, hh, g_black); continue; }
			i = ty * mw + tx;
			st = tile_state(i, bit);
			if (!st) { fill_rect(d, pitch, ww, hh, g_black); continue; }
			src = assets_megatile(w2.map.mega[i]);
			if (!src) { fill_rect(d, pitch, ww, hh, g_black); continue; }
			src += sy0 * ts + sx0;
			if (st == 2) copy_rect(d, pitch, src, ts, ww, hh);
			else if (mapdraw_fog_mode == MAPDRAW_FOG_SHADE) copy_shade(d, pitch, src, ts, ww, hh);
			else copy_dither(d, pitch, src, ts, ww, hh, ((mx + xs) + (my + ys) + 1) & 1);
		}
	}
}

/* ---------------------------------------------------------------- minimap */
static unsigned char g_mm[128 * 128];        /* terrain, buffer pixels, size g_mm_size^2 */
static unsigned char g_mm_fog[128 * 128];    /* terrain with fog */
static unsigned char g_mm_tx[128], g_mm_ty[128];   /* minimap pixel -> map tile */
static short g_mm_px[W2_MAP_MAX + 1];        /* map tile -> minimap pixel (from the drawn area origin) */
static int g_mm_size = -1, g_mm_w, g_mm_h, g_mm_ox, g_mm_oy, g_mm_maxd;
static uint32_t g_mm_ver = 0xFFFFFFFFu;
static unsigned g_mm_fog_serial;
static int g_mm_mapw = -1, g_mm_maph = -1, g_mm_reveal = -1, g_mm_local = -1, g_mm_fogmode = -1;
static int g_mm_scr_x, g_mm_scr_y, g_mm_scr_size = 128;   /* last drawn, hires */

void minimap_invalidate(void) { g_mm_size = -1; g_mm_fog_serial = 0; }

static void mm_build_terrain(int size)
{
	int x, y, w = w2.map.w, h = w2.map.h, maxd = w > h ? w : h;
	if (maxd <= 0) maxd = 1;
	g_mm_size = size; g_mm_maxd = maxd;
	g_mm_w = w * size / maxd; g_mm_h = h * size / maxd;
	if (g_mm_w < 1) g_mm_w = 1;
	if (g_mm_h < 1) g_mm_h = 1;
	g_mm_ox = (size - g_mm_w) / 2; g_mm_oy = (size - g_mm_h) / 2;
	for (x = 0; x < g_mm_w; x++) { int t = x * maxd / size; g_mm_tx[x] = (unsigned char)(t < w ? t : w - 1); }
	for (y = 0; y < g_mm_h; y++) { int t = y * maxd / size; g_mm_ty[y] = (unsigned char)(t < h ? t : h - 1); }
	for (x = 0; x <= maxd && x <= W2_MAP_MAX; x++) g_mm_px[x] = (short)(x * size / maxd);
	for (y = 0; y < g_mm_h; y++) {
		const uint16_t *mega = w2.map.mega + g_mm_ty[y] * w;
		unsigned char *d = g_mm + y * g_mm_w;
		for (x = 0; x < g_mm_w; x++) {
			int m = mega[g_mm_tx[x]];
			d[x] = (unsigned char)(m < g_mega_col_n ? g_mega_col[m] : g_black);
		}
	}
	g_mm_ver = w2.map.version; g_mm_mapw = w; g_mm_maph = h;
	g_mm_fog_serial = 0;
}

static void mm_build_fog(void)
{
	int x, y, w = w2.map.w, bit = local_bit();
	for (y = 0; y < g_mm_h; y++) {
		int row = g_mm_ty[y] * w;
		const unsigned char *s = g_mm + y * g_mm_w;
		unsigned char *d = g_mm_fog + y * g_mm_w;
		for (x = 0; x < g_mm_w; x++) {
			int st = tile_state(row + g_mm_tx[x], bit);
			d[x] = st == 2 ? s[x] : st == 0 ? (unsigned char)g_black :
			       mapdraw_fog_mode == MAPDRAW_FOG_SHADE ? g_shade[s[x]] : ((x + y) & 1) ? s[x] : (unsigned char)g_black;
		}
	}
	g_mm_fog_serial = g_fog_serial; g_mm_reveal = mapdraw_reveal; g_mm_local = w2.local_player; g_mm_fogmode = mapdraw_fog_mode;
}

static void mm_rect(uint8_t *b, int pitch, int x0, int y0, int x1, int y1, int c)
{
	/* filled, x1/y1 exclusive, clipped to the drawn area */
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > g_mm_w) x1 = g_mm_w;
	if (y1 > g_mm_h) y1 = g_mm_h;
	if (x1 <= x0) x1 = x0 + 1;
	if (y1 <= y0) y1 = y0 + 1;
	if (x0 >= g_mm_w || y0 >= g_mm_h) return;
	for (; y0 < y1; y0++) memset(b + (long)y0 * pitch + x0, c, (size_t)(x1 - x0));
}

static int unit_mm_visible(const W2Unit *u, const w2_unit_type *t)
{
	int i = u->ty * w2.map.w + u->tx;
	if (mapdraw_reveal || u->owner == w2.local_player) return 1;
	if (u->tx >= w2.map.w || u->ty >= w2.map.h) return 0;
	(void)t;
	if (u->owner >= 8 && (u->type == W2U_GOLD_MINE || u->type == W2U_OIL_PATCH))
		return (w2.map.explored[i] & local_bit()) != 0;    /* resources stay on the map once seen */
	return (w2.map.visible[i] & local_bit()) != 0;
}

void minimap_draw(int sx, int sy, int size)
{
	int lo = view.lores ? 1 : 0, bs, slot, y, mw = w2.map.w;
	uint8_t *b;
	if (!view.buf || mw <= 0 || size <= 0) return;
	ensure_era();
	g_mm_scr_x = sx; g_mm_scr_y = sy; g_mm_scr_size = size;
	bs = size >> lo;
	if (bs > 128) bs = 128;
	if (bs != g_mm_size || g_mm_ver != w2.map.version || g_mm_mapw != mw || g_mm_maph != w2.map.h) mm_build_terrain(bs);
	if (g_mm_fog_serial != g_fog_serial || g_mm_reveal != mapdraw_reveal || g_mm_local != w2.local_player || g_mm_fogmode != mapdraw_fog_mode)
		mm_build_fog();
	b = view.buf + (long)(sy >> lo) * view.pitch + (sx >> lo);
	if (g_mm_w != bs || g_mm_h != bs) fill_rect(b, view.pitch, bs, bs, g_black);
	b += g_mm_oy * view.pitch + g_mm_ox;
	for (y = 0; y < g_mm_h; y++) memcpy(b + (long)y * view.pitch, g_mm_fog + y * g_mm_w, (size_t)g_mm_w);
	/* units */
	for (slot = 0; slot < w2.unit_hi; slot++) {
		const W2Unit *u = &w2.unit[slot];
		const w2_unit_type *t;
		int c, tw, th;
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN))) continue;
		t = w2_unit(u->type);
		if (!t || u->tx >= mw || u->ty >= w2.map.h || !unit_mm_visible(u, t)) continue;
		tw = t->tile_w ? t->tile_w : 1; th = t->tile_h ? t->tile_h : 1;
		if (u->tx + tw > g_mm_maxd) tw = g_mm_maxd - u->tx;
		if (u->ty + th > g_mm_maxd) th = g_mm_maxd - u->ty;
		if (u->owner < 8) c = assets_remap(u->owner)[209];
		else c = u->type == W2U_GOLD_MINE ? g_yellow : g_grey;
		mm_rect(b, view.pitch, g_mm_px[u->tx], g_mm_px[u->ty], g_mm_px[u->tx + tw], g_mm_px[u->ty + th], c);
	}
	/* the viewport frame */
	{
		int den = g_mm_maxd * W2_TILE;
		int x0 = view.map_x * g_mm_size / den, y0 = view.map_y * g_mm_size / den;
		int x1 = (view.map_x + view.scr_w) * g_mm_size / den - 1, y1 = (view.map_y + view.scr_h) * g_mm_size / den - 1;
		int k;
		if (x1 >= g_mm_w) x1 = g_mm_w - 1;
		if (y1 >= g_mm_h) y1 = g_mm_h - 1;
		if (x0 < 0) x0 = 0;
		if (y0 < 0) y0 = 0;
		if (x1 > x0 && y1 > y0) {
			memset(b + (long)y0 * view.pitch + x0, g_white, (size_t)(x1 - x0 + 1));
			memset(b + (long)y1 * view.pitch + x0, g_white, (size_t)(x1 - x0 + 1));
			for (k = y0; k <= y1; k++) { b[(long)k * view.pitch + x0] = (uint8_t)g_white; b[(long)k * view.pitch + x1] = (uint8_t)g_white; }
		}
	}
}

int minimap_to_map(int sx, int sy, int *tx, int *ty)
{
	/* in hires units (independent of the lores halving) */
	int w = w2.map.w, h = w2.map.h, maxd = w > h ? w : h, size = g_mm_scr_size;
	int dw, dh, x, y, rx, ry;
	*tx = 0; *ty = 0;
	if (maxd <= 0 || size <= 0) return 0;
	dw = (w * size) / maxd;
	dh = (h * size) / maxd;
	x = sx - g_mm_scr_x - ((size - dw) >> 1);
	y = sy - g_mm_scr_y - ((size - dh) >> 1);
	if (x < 0 || y < 0 || x >= dw || y >= dh) return 0;
	rx = (x * maxd) / size;
	ry = (y * maxd) / size;
	*tx = rx < w ? rx : w - 1;
	*ty = ry < h ? ry : h - 1;
	return 1;
}
