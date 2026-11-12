/* ui_cursor.c - the game's cursors (War Data 301..322) as the HARDWARE pointer.
 *
 * Method (docs/AMISC_METHODS.md section 4): the pointer is Intuition's sprite with 3 colours =
 * palette registers 17..19 (assets.c keeps game pixels off them) + transparent. Each cursor is
 * kept 1:1 in hires (amiga_gfx runs AGA hires sprites at 70 ns: a sprite pixel = a screen pixel; RTG is 1:1)
 * and reduced 2:1 both ways in lores (sprites at their lores default there),
 * so it keeps its on-screen size. Cells are aligned so the hot spot pixel starts its cell; the
 * BRIGHTEST pixel of a cell speaks for it (averaging turns thin outlines into blobs). Then 3 colours:
 * the darkest colour is reserved for the edge, the rest is reduced to 2 by a small integer k-means.
 * One pointer per (shape, race, resolution) is cached; the cache is rebuilt when the era palette
 * changes (pointer objects are freed by amigagfx_close).
 */
#include <string.h>
#include <stdlib.h>
#include "w2.h"
#include "ui.h"
#include "assets.h"
#include "amiga_gfx.h"

#define PW 32          /* max reduced width (AGA sprites 32 wide; the RTG path in amiga_gfx shrinks to 16) */
#define PH 40          /* max reduced height */

typedef struct {
	void *ptr;
	unsigned char pix[PW * PH];
	unsigned char rgb[9];
	unsigned char w, h, hx, hy;
	unsigned char built;
} CurEnt;

static CurEnt g_cur[UI_CUR_COUNT][2][2];   /* shape, race, lores */
static int g_cache_era = -2;
static int g_now_shape = -1, g_now_race = -1, g_now_lores = -1;

static int entry_of(int shape, int race)
{
	race = race ? 1 : 0;
	switch (shape) {
	case UI_CUR_POINT:        return 301 + race;
	case UI_CUR_NO:           return 303 + race;
	case UI_CUR_TARGET:       return 305 + race;
	case UI_CUR_TARGET_RED:   return 307 + race;
	case UI_CUR_TARGET_GREEN: return 309 + race;
	case UI_CUR_GLASS:        return 311;
	case UI_CUR_CROSS:        return 312;
	case UI_CUR_WAIT:         return 313;
	default:
		if (shape >= UI_CUR_ARROW_N && shape <= UI_CUR_ARROW_NW) return 315 + (shape - UI_CUR_ARROW_N);
	}
	return 301 + race;
}

static int lum(const unsigned char *pal, int c)
{
	return pal[c * 3] * 3 + pal[c * 3 + 1] * 6 + pal[c * 3 + 2];   /* *10 */
}

static long dist3(const unsigned char *a, int r, int g, int b)
{
	long dr = a[0] - r, dg = a[1] - g, db = a[2] - b;
	return dr * dr * 3 + dg * dg * 4 + db * db * 2;
}

/* reduced pixel buffer of palette indices (0 = transparent) */
static unsigned char g_red[PW * 2 * PH];

/* The colours of the cursor pixels: the game palette (War Data 2, summer; the cursor entries use the
 * era-independent part), NOT whatever is on screen - in the menus that is a picture's palette and the
 * pointer came out black. Loaded once; the pointer has its own registers 17..19 anyway. */
static const unsigned char *cursor_palette(void)
{
	static unsigned char pal8[768];
	static int have;
	if (!have) {
		unsigned long n = 0;
		unsigned char *d = assets_load(ASSET_DATA, 2, &n);
		int i;
		if (!d || n < 768) { if (d) war_free(d); return assets_palette(); }
		for (i = 0; i < 768; i++) pal8[i] = (unsigned char)((d[i] & 63) << 2 | (d[i] & 63) >> 4);   /* 6 -> 8 bit */
		war_free(d);
		have = 1;
	}
	return pal8;
}

static void build(CurEnt *e, int shape, int race, int lores)
{
	unsigned long n; int hotx, hoty, w, h, fx = lores ? 2 : 1, fy = lores ? 2 : 1, offx, offy, rw, rh, x, y, hx, hy;
	int left, right, start, i, k, nc;
	const unsigned char *pal = cursor_palette(), *src;
	unsigned char *data;
	static unsigned short count[256];
	static unsigned char used[256];
	int cent[3][3];
	memset(e, 0, sizeof *e);
	e->built = 1;
	data = assets_load(ASSET_DATA, entry_of(shape, race), &n);
	if (!data) return;
	src = w2_cursor_probe(data, n, &hotx, &hoty, &w, &h);
	if (!src || w <= 0 || h <= 0) { war_free(data); return; }
	offx = (fx - hotx % fx) % fx; offy = (fy - hoty % fy) % fy;
	rw = (w + offx + fx - 1) / fx; rh = (h + offy + fy - 1) / fy;
	if (rw > PW * 2) rw = PW * 2;
	if (rh > PH) rh = PH;
	hx = (hotx + offx) / fx; hy = (hoty + offy) / fy;
	memset(g_red, 0, sizeof g_red);
	for (y = 0; y < h; y++) {
		int cy = (y + offy) / fy;
		if (cy >= rh) break;
		for (x = 0; x < w; x++) {
			int cx = (x + offx) / fx, c = src[y * w + x], o;
			if (!c || cx >= rw) continue;
			o = g_red[cy * (PW * 2) + cx];
			if (!o || lum(pal, c) > lum(pal, o)) g_red[cy * (PW * 2) + cx] = (unsigned char)c;
		}
	}
	/* crop to 16 columns, keeping the hot spot */
	left = rw; right = -1;
	for (y = 0; y < rh; y++) for (x = 0; x < rw; x++) if (g_red[y * (PW * 2) + x]) { if (x < left) left = x; if (x > right) right = x; }
	if (right < 0) { left = 0; right = 0; }
	start = left < hx ? left : hx;
	if (right - start + 1 > PW && hx - start >= PW) start = hx - PW + 1;
	if (right - start + 1 < PW) rw = right - start + 1; else rw = PW;
	if (rw < 1) rw = 1;
	/* colours: histogram */
	memset(count, 0, sizeof count); memset(used, 0, sizeof used);
	nc = 0;
	for (y = 0; y < rh; y++) for (x = 0; x < rw; x++) {
		int c = g_red[y * (PW * 2) + start + x];
		if (c) { if (!count[c]) used[nc++] = (unsigned char)c; if (count[c] < 60000) count[c]++; }
	}
	if (nc <= 3) {
		for (k = 0; k < 3; k++) {
			int c = k < nc ? used[k] : (nc ? used[0] : 0);
			cent[k][0] = pal[c * 3]; cent[k][1] = pal[c * 3 + 1]; cent[k][2] = pal[c * 3 + 2];
		}
	} else {
		/* darkest colour = the edge register; the rest -> 2 centroids (k-means, integer) */
		int dark = used[0], lo = -1, hi = -1, it;
		for (i = 1; i < nc; i++) if (lum(pal, used[i]) < lum(pal, dark)) dark = used[i];
		for (i = 0; i < nc; i++) {
			int c = used[i];
			if (c == dark) continue;
			if (lo < 0 || lum(pal, c) < lum(pal, lo)) lo = c;
			if (hi < 0 || lum(pal, c) > lum(pal, hi)) hi = c;
		}
		cent[0][0] = pal[dark * 3]; cent[0][1] = pal[dark * 3 + 1]; cent[0][2] = pal[dark * 3 + 2];
		for (k = 0; k < 3; k++) { cent[1][k] = pal[lo * 3 + k]; cent[2][k] = pal[hi * 3 + k]; }
		for (it = 0; it < 8; it++) {
			long sum[2][3], cnt[2];
			memset(sum, 0, sizeof sum); cnt[0] = cnt[1] = 0;
			for (i = 0; i < nc; i++) {
				int c = used[i], j;
				const unsigned char *p = pal + c * 3;
				if (c == dark) continue;
				j = dist3(p, cent[1][0], cent[1][1], cent[1][2]) <= dist3(p, cent[2][0], cent[2][1], cent[2][2]) ? 0 : 1;
				sum[j][0] += (long)p[0] * count[c]; sum[j][1] += (long)p[1] * count[c]; sum[j][2] += (long)p[2] * count[c];
				cnt[j] += count[c];
			}
			for (k = 0; k < 2; k++) if (cnt[k]) {
				cent[k + 1][0] = (int)(sum[k][0] / cnt[k]); cent[k + 1][1] = (int)(sum[k][1] / cnt[k]); cent[k + 1][2] = (int)(sum[k][2] / cnt[k]);
			}
		}
	}
	if (assets_ehb()) {
		/* EHB (OCS/ECS): the pointer's colours ARE playfield registers 17..19 and stay what the EHB
		 * palette put there (dark / grey / bright) - the cursor is drawn with those three */
		const unsigned char *ep = assets_palette() + 17 * 3;
		for (k = 0; k < 3; k++) { cent[k][0] = ep[k * 3]; cent[k][1] = ep[k * 3 + 1]; cent[k][2] = ep[k * 3 + 2]; }
	}
	for (k = 0; k < 3; k++) { e->rgb[k * 3] = (unsigned char)cent[k][0]; e->rgb[k * 3 + 1] = (unsigned char)cent[k][1]; e->rgb[k * 3 + 2] = (unsigned char)cent[k][2]; }
	for (y = 0; y < rh; y++) for (x = 0; x < rw; x++) {
		int c = g_red[y * (PW * 2) + start + x], best = 0; long bd = 0x7fffffffL;
		if (!c) { e->pix[y * rw + x] = 0; continue; }
		for (k = 0; k < 3; k++) {
			long d = dist3(pal + c * 3, cent[k][0], cent[k][1], cent[k][2]);
			if (d < bd) { bd = d; best = k; }
		}
		e->pix[y * rw + x] = (unsigned char)(best + 1);
	}
	hx -= start;
	if (hx < 0) hx = 0;
	if (hx >= rw) hx = rw - 1;
	e->w = (unsigned char)rw; e->h = (unsigned char)rh; e->hx = (unsigned char)hx; e->hy = (unsigned char)hy;
	e->ptr = amigagfx_pointer_new(e->pix, rw, rh, hx, hy);
	war_free(data);
}

static CurEnt *get(int shape, int race, int lores)
{
	CurEnt *e;
	if (shape < 0 || shape >= UI_CUR_COUNT) shape = UI_CUR_POINT;
	race = race ? 1 : 0; lores = lores ? 1 : 0;
	if (g_cache_era != assets_era()) {
		memset(g_cur, 0, sizeof g_cur);
		g_cache_era = assets_era();
		g_now_shape = -1;
	}
	e = &g_cur[shape][race][lores];
	if (!e->built) build(e, shape, race, lores);
	return e;
}

void ui_cursor_set(int shape, int race)
{
	int lores = assets_lores();
	CurEnt *e;
	if (g_cache_era == assets_era() && shape == g_now_shape && race == g_now_race && lores == g_now_lores) return;
	e = get(shape, race, lores);
	g_now_shape = shape; g_now_race = race; g_now_lores = lores;
	if (!e->ptr) return;
	if (!assets_ehb()) amigagfx_pointer_colours(e->rgb);   /* EHB: registers 17..19 belong to the picture */
	amigagfx_pointer_use(e->ptr);
}

int ui_cursor_shape(void) { return g_now_shape; }

void ui_cursor_flush(void)
{
	memset(g_cur, 0, sizeof g_cur);
	g_cache_era = -2;
	g_now_shape = -1;
}

int ui_cursor_debug(int shape, int race, const unsigned char **pix, int *w, int *h, int *hotx, int *hoty, unsigned char rgb9[9])
{
	CurEnt *e = get(shape, race, assets_lores());
	if (!e->w) return -1;
	*pix = e->pix; *w = e->w; *h = e->h; *hotx = e->hx; *hoty = e->hy;
	memcpy(rgb9, e->rgb, 9);
	return 0;
}
