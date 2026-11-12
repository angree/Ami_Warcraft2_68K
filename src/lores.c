/* lores.c - halving the game's art for 320x240 (see lores.h). Plain C, integers only, no floats.
 * The method is AmiSC's (core/amisc_lores.h, user's own code), re-written in C for W2 data. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w2.h"
#include "assets.h"
#include "w2cfg.h"
#include "w2gfx_decode.h"
#include "amiga_gfx.h"
#include "lores.h"
#include "ehb.h"

LoresStats lores_stats;
void lores_stats_reset(void) { memset(&lores_stats, 0, sizeof lores_stats); }

/* ================================================================ the filter */

/* Lanczos-3 for 2:1, 16.16, sum 65536: output x covers source 2x,2x+1; tap 0 is at 2x-5.
 * t = (k - 0.5) / 2, w = 3 sin(pi t) sin(pi t / 3) / (pi t)^2, normalised (computed once on a PC). */
#define TAPS 12
#define FIRST (-5)
#define PAD 6
#define ROT32(s, k) ((((s) << (k)) | (((s) & 0xFFFFFFFFUL) >> (32 - (k)))) & 0xFFFFFFFFUL)   /* same on 32/64-bit hosts */
static const short k_w[TAPS] = { 242, 987, -2228, -4367, 8880, 29254, 29254, 8880, -4367, -2228, 987, 242 };
static long g_tab[TAPS][256];        /* k_w[i] * v: no multiply in the horizontal pass */
static int g_tab_ok;
static int g_sq[511];                /* g_sq[255 + d] = d * d */

static void tab_init(void)
{
	int i, v;
	if (g_tab_ok) return;
	for (i = 0; i < TAPS; i++) for (v = 0; v < 256; v++) g_tab[i][v] = (long)k_w[i] * v;
	for (v = -255; v <= 255; v++) g_sq[255 + v] = v * v;
	g_tab_ok = 1;
}

/* weighted distance (r 3, g 4, b 2 - as assets.c nearest()) */
#define DIST(pal, i, r, g, b) (3L * g_sq[255 + (r) - (pal)[(i) * 3]] + 4L * g_sq[255 + (g) - (pal)[(i) * 3 + 1]] + \
                               2L * g_sq[255 + (b) - (pal)[(i) * 3 + 2]])

/* ---------------------------------------------------------------- quantiser */

/* colour-cycled ranges per era (wargus tileset scripts - NIEPEWNE vs the original, harmless if wrong:
 * those indices are only kept out of RGB averages and halved by vote) */
static const unsigned char k_cyc[3][4][2] = {
	{ { 38, 47 }, { 48, 56 }, { 240, 244 }, { 0, 0 } },        /* summer */
	{ { 40, 47 }, { 48, 54 }, { 205, 207 }, { 240, 244 } },    /* winter */
	{ { 38, 47 }, { 64, 70 }, { 240, 244 }, { 0, 0 } },        /* wasteland */
};

static void add_ramp(LoresQ *q, int lo, int hi, int avg)
{
	int i, k;
	if (q->nramps >= LORES_MAX_RAMPS || lo > hi) return;
	k = ++q->nramps;
	q->lo[k] = (unsigned char)lo; q->hi[k] = (unsigned char)hi; q->avg[k] = (unsigned char)avg;
	for (i = lo; i <= hi; i++) if (!q->cls[i]) q->cls[i] = (unsigned char)k;
}

/* Exact 15-bit table, pruned (AmiSC glue_lores.h build_quantiser): the cube is cut into 8x8x8 blocks of
 * 4x4x4 cells; only entries whose nearest possible distance to a block does not exceed the best
 * guaranteed distance of any entry can win inside it. */
static void build_lut(LoresQ *q)
{
	static long mind[256];
	static unsigned char cand[256];
	int lv[32], br, bg, bb, i, c;
	const unsigned char *pal = q->dpal;
	for (c = 0; c < 32; c++) lv[c] = c * 255 / 31;
	for (br = 0; br < 8; br++)
	for (bg = 0; bg < 8; bg++)
	for (bb = 0; bb < 8; bb++) {
		int r0 = lv[br * 4], r1 = lv[br * 4 + 3], g0 = lv[bg * 4], g1 = lv[bg * 4 + 3];
		int b0 = lv[bb * 4], b1 = lv[bb * 4 + 3], nc = 0, fr, fg, fb;
		long best_max = 0x7fffffffL;
		for (i = 0; i < 256; i++) {
			int pr = pal[i * 3], pg = pal[i * 3 + 1], pb = pal[i * 3 + 2];
			int nr, ng, nb, xr, xg, xb; long mx;
			mind[i] = -1;
			if (!q->allow[i]) continue;
			nr = pr < r0 ? r0 - pr : pr > r1 ? pr - r1 : 0;
			ng = pg < g0 ? g0 - pg : pg > g1 ? pg - g1 : 0;
			nb = pb < b0 ? b0 - pb : pb > b1 ? pb - b1 : 0;
			xr = pr - r0 > r1 - pr ? pr - r0 : r1 - pr;
			xg = pg - g0 > g1 - pg ? pg - g0 : g1 - pg;
			xb = pb - b0 > b1 - pb ? pb - b0 : b1 - pb;
			mind[i] = 3L * g_sq[255 + nr] + 4L * g_sq[255 + ng] + 2L * g_sq[255 + nb];
			mx = 3L * g_sq[255 + xr] + 4L * g_sq[255 + xg] + 2L * g_sq[255 + xb];
			if (mx < best_max) best_max = mx;
		}
		for (i = 0; i < 256; i++) if (mind[i] >= 0 && mind[i] <= best_max) cand[nc++] = (unsigned char)i;
		for (fr = br * 4; fr < br * 4 + 4; fr++)
		for (fg = bg * 4; fg < bg * 4 + 4; fg++)
		for (fb = bb * 4; fb < bb * 4 + 4; fb++) {
			int r = lv[fr], g = lv[fg], b = lv[fb], k, best = 0;
			long bd = 0x7fffffffL;
			for (k = 0; k < nc; k++) {
				long d = DIST(pal, cand[k], r, g, b);
				if (d < bd) { bd = d; best = cand[k]; }
			}
			q->lut[(fr << 10) | (fg << 5) | fb] = (unsigned char)best;
		}
	}
}

/* destination palette (EHB) of the quantisers built from now on */
static unsigned char g_dst[192];
static int g_dst_on;

void lores_set_dst(const unsigned char *e192)
{
	g_dst_on = e192 != NULL;
	if (e192) memcpy(g_dst, e192, 192);
}

static int q_dst_same(const LoresQ *q)
{
	if (q->dst != g_dst_on) return 0;
	return !g_dst_on || !memcmp(q->dpal, g_dst, 192);
}

unsigned long lores_ehb_sum(const unsigned char *pal768, int era)
{
	unsigned long s = 0x12345u; int i;
	for (i = 0; i < 768; i++) s = ROT32(s, 5) ^ pal768[i];
	s = ROT32(s, 9) ^ 0x45484200UL ^ (unsigned long)EHB_VERSION ^ ((unsigned long)(era >= 0 && era < 3 ? era + 1 : 0) << 8)
	    ^ ((unsigned long)EHB_PH << 12);
	for (i = 0; i < 8; i++) s = ROT32(s, 3) ^ ehb_player_reg[i];
	return s & 0xFFFFFFFFUL;
}

/* EHB: source palette pal768 -> the 64 colours of g_dst. Classes: only the player ramp (era), which
 * becomes the placeholder pair; nothing is colour-cycled on an EHB screen. */
static void q_build_dst(LoresQ *q, const unsigned char *pal768, int era)
{
	int i, isera = era >= 0 && era < 3, bf = ehb_bright_first(pal768);
	q->dst = 1;
	memset(q->dpal, 0, sizeof q->dpal);
	memcpy(q->dpal, g_dst, 192);
	if (isera) add_ramp(q, 208, 211, 1);
	for (i = 0; i < 256; i++) q->allow[i] = (unsigned char)(i < 64 && !(isera && (i & 31) == EHB_PH));
	build_lut(q);
	for (i = 0; i < 256; i++) {
		if (q->cls[i]) q->omap[i] = (unsigned char)(((i - 208 < 2) == (bf != 0)) ? EHB_PH : EHB_PH + 32);
		else q->omap[i] = (unsigned char)ehb_nearest(g_dst, pal768[i * 3], pal768[i * 3 + 1], pal768[i * 3 + 2], isera);
	}
	q->sum = lores_ehb_sum(pal768, era);
}

void lores_q_build(LoresQ *q, const unsigned char *pal768, int era)
{
	int i; unsigned long s = 0x12345u;
	tab_init();
	memcpy(q->pal, pal768, 768);
	memset(q->cls, 0, sizeof q->cls);
	q->nramps = 0;
	if (g_dst_on) { q_build_dst(q, pal768, era); return; }
	q->dst = 0;
	memcpy(q->dpal, pal768, 768);
	for (i = 0; i < 256; i++) q->omap[i] = (unsigned char)i;
	if (era >= 0 && era < 3) {
		add_ramp(q, 208, 211, 1);                          /* player colour: ramp position */
		for (i = 0; i < 4; i++) if (k_cyc[era][i][1]) add_ramp(q, k_cyc[era][i][0], k_cyc[era][i][1], 0);
	}
	for (i = 0; i < 256; i++) q->allow[i] = (unsigned char)(!q->cls[i] && !(i >= 17 && i <= 19));
	build_lut(q);
	for (i = 0; i < 768; i++) s = ROT32(s, 5) ^ pal768[i];
	for (i = 0; i < 256; i++) s = ROT32(s, 3) ^ q->cls[i];
	q->sum = s & 0xFFFFFFFFUL;   /* 32 bits on every host (stored in the pack) */
}

/* nearest allowed index to (r,g,b): the table's answer, refined against up to 4 candidates (the source
 * pixels of the block) with the exact distance, so a flat colour keeps its own index. */
static int q_nearest(const LoresQ *q, int r, int g, int b, const unsigned char *cand, int nc)
{
	int best, k; long bd;
	if (r < 0) r = 0; else if (r > 255) r = 255;
	if (g < 0) g = 0; else if (g > 255) g = 255;
	if (b < 0) b = 0; else if (b > 255) b = 255;
	best = q->lut[((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)];
	bd = DIST(q->dpal, best, r, g, b);
	for (k = 0; k < nc; k++) {
		int c = q->omap[cand[k]]; long d;
		if (!q->allow[c] || c == best) continue;
		d = DIST(q->dpal, c, r, g, b);
		if (d < bd) { bd = d; best = c; }
	}
	return best;
}

/* ---------------------------------------------------------------- halving */

typedef struct { short r, g, b, w; unsigned char lr, lg, lb, hr, hg, hb; } Acc;
static Acc *g_ring; static long g_ring_cap;
static unsigned char *g_rowv, *g_rowr, *g_rowg, *g_rowb; static long g_row_cap;

static int work_reserve(int ow, int w)
{
	if ((long)ow * TAPS > g_ring_cap) {
		Acc *p = (Acc *)realloc(g_ring, sizeof(Acc) * (size_t)ow * TAPS);
		if (!p) return -1;
		g_ring = p; g_ring_cap = (long)ow * TAPS;
	}
	if (w + 2 * PAD > g_row_cap) {
		long n = w + 2 * PAD;
		unsigned char *p = (unsigned char *)realloc(g_rowv, (size_t)n * 4);
		if (!p) return -1;
		g_rowv = p; g_rowr = p + n; g_rowg = p + 2 * n; g_rowb = p + 3 * n; g_row_cap = n;
	}
	return 0;
}

/* horizontal pass of source row sy into ring slot 'out' */
static void hpass(const unsigned char *src, const unsigned char *mask, int w, int h, int pitch,
                  const LoresQ *q, int sy, Acc *out, int ow)
{
	int x, ox;
	const unsigned char *line, *ml = NULL;
	if (sy < 0 || sy >= h) {
		if (mask) {
			for (ox = 0; ox < ow; ox++) { Acc *a = &out[ox]; a->r = a->g = a->b = a->w = 0; a->lr = a->lg = a->lb = 255; a->hr = a->hg = a->hb = 0; }
			return;
		}
		sy = sy < 0 ? 0 : h - 1;
	}
	lores_stats.hcols += ow;
	line = src + (long)sy * pitch;
	if (mask) ml = mask + (long)sy * pitch;
	/* padded row: valid = opaque colour-class pixel */
	for (x = -PAD; x < w + PAD; x++) {
		int sx = x, idx, v;
		if (sx < 0 || sx >= w) {
			if (mask) { g_rowv[x + PAD] = 0; continue; }
			sx = sx < 0 ? 0 : w - 1;
		}
		idx = line[sx];
		v = (!ml || ml[sx]) && !q->cls[idx];
		g_rowv[x + PAD] = (unsigned char)v;
		g_rowr[x + PAD] = q->pal[idx * 3]; g_rowg[x + PAD] = q->pal[idx * 3 + 1]; g_rowb[x + PAD] = q->pal[idx * 3 + 2];
	}
	for (ox = 0; ox < ow; ox++) {
		long ar = 0, ag = 0, ab = 0, aw = 0;
		int lr = 255, lg = 255, lb = 255, hr = 0, hg = 0, hb = 0, i, base = ox * 2 + FIRST + PAD;
		Acc *a = &out[ox];
		for (i = 0; i < TAPS; i++) {
			int p = base + i, cr, cg, cb;
			if (!g_rowv[p]) continue;
			cr = g_rowr[p]; cg = g_rowg[p]; cb = g_rowb[p];
			ar += g_tab[i][cr]; ag += g_tab[i][cg]; ab += g_tab[i][cb]; aw += k_w[i];
			if (cr < lr) lr = cr;
			if (cr > hr) hr = cr;
			if (cg < lg) lg = cg;
			if (cg > hg) hg = cg;
			if (cb < lb) lb = cb;
			if (cb > hb) hb = cb;
			lores_stats.htaps++;
		}
		/* scale 2^16 -> 2^6 (>>10, not >>8: the overshoot of the negative lobes needs the headroom) */
		a->r = (short)(ar >> 10); a->g = (short)(ag >> 10); a->b = (short)(ab >> 10); a->w = (short)(aw >> 8);
		a->lr = (unsigned char)lr; a->lg = (unsigned char)lg; a->lb = (unsigned char)lb;
		a->hr = (unsigned char)hr; a->hg = (unsigned char)hg; a->hb = (unsigned char)hb;
	}
}

/* Dithered translucency (building shadows, some effects) is drawn as a checkerboard: every opaque pixel
 * has no opaque 4-neighbour. Such a block keeps the pattern at half size (opaque on even (ox+oy) only)
 * instead of turning into a solid black area. A 1-pixel diagonal line has only 2 diagonal neighbours
 * per pixel, so it is not taken for a dither: some pixel of the block needs >= 3. */
static int mask_at(const unsigned char *mask, int w, int h, int pitch, int x, int y)
{
	return x >= 0 && y >= 0 && x < w && y < h && mask[(long)y * pitch + x];
}

static int dither_block(const unsigned char *mask, int w, int h, int pitch, int bx0, int by0)
{
	int dx, dy, best = 0;
	for (dy = 0; dy < 2; dy++) for (dx = 0; dx < 2; dx++) {
		int x = bx0 + dx, y = by0 + dy, d;
		if (!mask_at(mask, w, h, pitch, x, y)) continue;
		if (mask_at(mask, w, h, pitch, x - 1, y) || mask_at(mask, w, h, pitch, x + 1, y) ||
		    mask_at(mask, w, h, pitch, x, y - 1) || mask_at(mask, w, h, pitch, x, y + 1)) return 0;
		d = mask_at(mask, w, h, pitch, x - 1, y - 1) + mask_at(mask, w, h, pitch, x + 1, y - 1) +
		    mask_at(mask, w, h, pitch, x - 1, y + 1) + mask_at(mask, w, h, pitch, x + 1, y + 1);
		if (d > best) best = d;
	}
	return best >= 3;
}

void lores_halve(const unsigned char *src, const unsigned char *mask, int w, int h, int pitch,
                 unsigned char *dst, unsigned char *dmask, int dpitch, const LoresQ *q)
{
	int ow = (w + 1) >> 1, oh = (h + 1) >> 1, ox, oy, t;
	long ring_row[TAPS];
	const Acc *rp[TAPS];
	if (w <= 0 || h <= 0) return;
	tab_init();
	if (work_reserve(ow, w)) {   /* out of memory: point sample */
		for (oy = 0; oy < oh; oy++) for (ox = 0; ox < ow; ox++) {
			long s = (long)(oy * 2) * pitch + ox * 2;
			dst[(long)oy * dpitch + ox] = q->omap[src[s]];
			if (dmask) dmask[(long)oy * dpitch + ox] = mask ? mask[s] : 1;
		}
		return;
	}
	for (t = 0; t < TAPS; t++) ring_row[t] = -0x40000000L;   /* a row no image has */
	for (oy = 0; oy < oh; oy++) {
		int need_rows = 0;
		unsigned char *drow = dst + (long)oy * dpitch, *mrow = dmask ? dmask + (long)oy * dpitch : NULL;
		for (ox = 0; ox < ow; ox++) {
			unsigned char blk[4]; int n_set = 0, n_col = 0, k, dx, dy, cnt[LORES_MAX_RAMPS + 1], best_c = 0, out;
			long sr = 0, sg = 0, sb = 0;
			/* the 2x2 block this pixel covers */
			for (dy = 0; dy < 2; dy++) for (dx = 0; dx < 2; dx++) {
				int bx = ox * 2 + dx, by = oy * 2 + dy;
				if (bx >= w || by >= h) {
					if (mask) continue;
					if (bx >= w) bx = w - 1;
					if (by >= h) by = h - 1;
				}
				if (mask && !mask[(long)by * pitch + bx]) continue;
				blk[n_set++] = src[(long)by * pitch + bx];
			}
			lores_stats.out_px++;
			if (mask && n_set > 0 && n_set <= 2 && dither_block(mask, w, h, pitch, ox * 2, oy * 2)) {
				int c = blk[0];
				lores_stats.ramp_px++;
				if ((ox + oy) & 1) { drow[ox] = 0; if (mrow) mrow[ox] = 0; continue; }   /* hole of the pattern */
				if (!q->cls[c] && !q->allow[q->omap[c]]) c = q_nearest(q, q->pal[c * 3], q->pal[c * 3 + 1], q->pal[c * 3 + 2], NULL, 0);
				else c = q->omap[c];
				drow[ox] = (unsigned char)c; if (mrow) mrow[ox] = 1;
				continue;
			}
			if (mask && n_set < 2) {     /* transparent: fewer than half of the block is opaque */
				drow[ox] = 0; if (mrow) mrow[ox] = 0;
				lores_stats.clear_px++;
				continue;
			}
			if (mrow) mrow[ox] = 1;
			for (k = 0; k <= q->nramps; k++) cnt[k] = 0;
			for (k = 0; k < n_set; k++) cnt[q->cls[blk[k]]]++;
			n_col = cnt[0];
			for (k = 1; k <= q->nramps; k++) if (cnt[k] > (best_c ? cnt[best_c] : 0)) best_c = k;
			if (best_c && (cnt[best_c] > n_col || (cnt[best_c] == n_col && q->avg[best_c]))) {
				int lo = q->lo[best_c];
				lores_stats.ramp_px++;
				if (q->avg[best_c]) {      /* player colour: average ramp position */
					int sum = 0, n = cnt[best_c];
					for (k = 0; k < n_set; k++) if (q->cls[blk[k]] == best_c) sum += blk[k] - lo;
					drow[ox] = q->omap[lo + (sum * 2 + n) / (n * 2)];
				} else {                   /* cycled colours: vote, ties to the first in reading order */
					int bn = 0, bv = blk[0], j;
					for (k = 0; k < n_set; k++) {
						int n = 0;
						if (q->cls[blk[k]] != best_c) continue;
						for (j = 0; j < n_set; j++) if (blk[j] == blk[k]) n++;
						if (n > bn) { bn = n; bv = blk[k]; }
					}
					drow[ox] = q->omap[bv];
				}
				continue;
			}
			/* colour class */
			if (n_col == 4 && blk[0] == blk[1] && blk[0] == blk[2] && blk[0] == blk[3] && q->allow[q->omap[blk[0]]]) {
				drow[ox] = q->omap[blk[0]];   /* flat block: keep the index */
				lores_stats.edge_px++;
				continue;
			}
			if (n_col == 4) {             /* interior: Lanczos */
				long vr = 0, vg = 0, vb = 0, vw = 0, den;
				int lr = 255, lg = 255, lb = 255, hr = 0, hg = 0, hb = 0, r, g, b;
				if (!need_rows) {
					for (t = 0; t < TAPS; t++) {
						int sy = oy * 2 + FIRST + t, slot = (sy + TAPS) % TAPS;
						if (ring_row[slot] == sy) continue;
						ring_row[slot] = sy;
						hpass(src, mask, w, h, pitch, q, sy, g_ring + (long)slot * ow, ow);
					}
					for (t = 0; t < TAPS; t++) rp[t] = g_ring + (long)((oy * 2 + FIRST + t + TAPS) % TAPS) * ow;
					need_rows = 1;
				}
				/* the weights are symmetric (w[t] == w[11-t]): add the mirrored rows first, 6 multiplies per sum */
				for (t = 0; t < TAPS / 2; t++) {
					const Acc *a = rp[t] + ox, *c = rp[TAPS - 1 - t] + ox;
					long wt = k_w[t];
					vr += wt * (a->r + c->r); vg += wt * (a->g + c->g); vb += wt * (a->b + c->b); vw += wt * (a->w + c->w);
					if (a->lr < lr) lr = a->lr;
					if (a->hr > hr) hr = a->hr;
					if (a->lg < lg) lg = a->lg;
					if (a->hg > hg) hg = a->hg;
					if (a->lb < lb) lb = a->lb;
					if (a->hb > hb) hb = a->hb;
					if (c->lr < lr) lr = c->lr;
					if (c->hr > hr) hr = c->hr;
					if (c->lg < lg) lg = c->lg;
					if (c->hg > hg) hg = c->hg;
					if (c->lb < lb) lb = c->lb;
					if (c->hb > hb) hb = c->hb;
				}
				/* vr: scale 2^22 * colour, vw: scale 2^24 for full weight */
				if (vw >= (1L << 23)) {
					den = vw >> 2;
					r = vr <= 0 ? 0 : (int)((vr + (den >> 1)) / den);
					g = vg <= 0 ? 0 : (int)((vg + (den >> 1)) / den);
					b = vb <= 0 ? 0 : (int)((vb + (den >> 1)) / den);
					if (r < lr) r = lr; else if (r > hr) r = hr;     /* anti-ringing */
					if (g < lg) g = lg; else if (g > hg) g = hg;
					if (b < lb) b = lb; else if (b > hb) b = hb;
					drow[ox] = (unsigned char)q_nearest(q, r, g, b, blk, 4);
					lores_stats.lanczos_px++;
					continue;
				}
			}
			/* edge (or too little opaque weight): plain average of the block's colour pixels */
			for (k = 0; k < n_set; k++) {
				int c = blk[k];
				if (q->cls[c]) continue;
				sr += q->pal[c * 3]; sg += q->pal[c * 3 + 1]; sb += q->pal[c * 3 + 2];
			}
			out = q_nearest(q, (int)((sr + n_col / 2) / n_col), (int)((sg + n_col / 2) / n_col),
			                (int)((sb + n_col / 2) / n_col), blk, n_set);
			drow[ox] = (unsigned char)out;
			lores_stats.edge_px++;
		}
	}
}

/* ---------------------------------------------------------------- growable buffer */

typedef struct { unsigned char *p; unsigned long n, cap; int err; } Buf;

static unsigned char *buf_grow(Buf *b, unsigned long more)
{
	if (b->err) return NULL;
	if (b->n + more > b->cap) {
		unsigned long nc = b->cap ? b->cap : 4096;
		unsigned char *p;
		while (nc < b->n + more) nc *= 2;
		p = (unsigned char *)realloc(b->p, nc);
		if (!p) { b->err = 1; return NULL; }
		b->p = p; b->cap = nc;
	}
	b->n += more;
	return b->p + b->n - more;
}
static void put16(unsigned char *p, unsigned long v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }
static void put32(unsigned char *p, unsigned long v) { put16(p, v & 0xFFFF); put16(p + 2, v >> 16); }
static void buf_byte(Buf *b, int v) { unsigned char *p = buf_grow(b, 1); if (p) *p = (unsigned char)v; }

/* ---------------------------------------------------------------- sprites */

static unsigned char *g_cv, *g_cm, *g_hv, *g_hm; static long g_cv_cap, g_hv_cap;
static unsigned char g_ones[256];

static int canvas_reserve(long n, long hn)
{
	if (n > g_cv_cap) {
		unsigned char *a = (unsigned char *)realloc(g_cv, (size_t)n * 2);
		if (!a) return -1;
		g_cv = a; g_cm = a + n; g_cv_cap = n;
	}
	if (hn > g_hv_cap) {
		unsigned char *a = (unsigned char *)realloc(g_hv, (size_t)hn * 2);
		if (!a) return -1;
		g_hv = a; g_hm = a + hn; g_hv_cap = hn;
	}
	return 0;
}

/* one RLE row (w pixels, m = opacity) in the w2 sprite format */
static void rle_row(Buf *b, const unsigned char *v, const unsigned char *m, int w)
{
	int c = 0;
	while (c < w) {
		int e = c;
		if (!m[c]) {
			while (e < w && !m[e] && e - c < 127) e++;
			buf_byte(b, 0x80 | (e - c));
			c = e;
			continue;
		}
		while (e < w && m[e]) e++;           /* opaque span [c,e) */
		while (c < e) {
			int r = c + 1, l;
			while (r < e && v[r] == v[c] && r - c < 63) r++;
			if (r - c >= 3) { buf_byte(b, 0x40 | (r - c)); buf_byte(b, v[c]); c = r; continue; }
			/* literal: until a run of 3 starts or 63 bytes */
			l = c;
			while (l < e && l - c < 63) {
				if (l + 2 < e && v[l] == v[l + 1] && v[l] == v[l + 2]) break;
				l++;
			}
			if (l == c) l = c + 1;
			buf_byte(b, l - c);
			{ unsigned char *p = buf_grow(b, (unsigned long)(l - c)); if (p) memcpy(p, v + c, (size_t)(l - c)); }
			c = l;
		}
	}
}

unsigned char *lores_sprite_halve(const unsigned char *spr, unsigned long size, int type,
                                  const LoresQ *q, unsigned long *out_size)
{
	Buf b; int n, bw, bh, i;
	if (out_size) *out_size = 0;
	if (!spr || size < 6 || (type != W2_SPR_RLE && type != W2_SPR_RAW)) return NULL;
	n = (int)W2_LE16(spr); bw = (int)W2_LE16(spr + 2); bh = (int)W2_LE16(spr + 4);
	if (n <= 0 || 6UL + 8UL * n > size) return NULL;
	if (!g_ones[0]) memset(g_ones, 1, sizeof g_ones);
	memset(&b, 0, sizeof b);
	{
		unsigned char *h = buf_grow(&b, 6UL + 8UL * n);
		if (!h) return NULL;
		memset(h, 0, 6UL + 8UL * n);
		put16(h, (unsigned long)n); put16(h + 2, (unsigned long)((bw + 1) >> 1)); put16(h + 4, (unsigned long)((bh + 1) >> 1));
	}
	for (i = 0; i < n; i++) {
		W2Frame f; int x0, y0, ow, oh, cw, ch, j, nx, ny, nw, nh, minx, maxx, miny, maxy, x, y;
		unsigned long off;
		unsigned char *rec;
		if (w2_sprite_frame(spr, size, i, &f) < 0) break;
		/* the same frame data used again (same geometry): share it */
		for (j = 0; j < i; j++) {
			W2Frame g;
			w2_sprite_frame(spr, size, j, &g);
			if (g.offset == f.offset && g.x == f.x && g.y == f.y && g.w == f.w && g.h == f.h) break;
		}
		if (j < i) { memcpy(b.p + 6 + 8 * i, b.p + 6 + 8 * j, 8); continue; }
		off = b.n;
		nx = f.x >> 1; ny = f.y >> 1; nw = 0; nh = 0;
		if (f.w > 0 && f.h > 0) {
			lores_stats.frames++;
			if (type == W2_SPR_RLE) {
				W2Clip cl;
				x0 = f.x >> 1; y0 = f.y >> 1;
				ow = ((f.x + f.w + 1) >> 1) - x0; oh = ((f.y + f.h + 1) >> 1) - y0;
				cw = ow * 2; ch = oh * 2;
				if (canvas_reserve((long)cw * ch, (long)ow * oh)) { b.err = 1; break; }
				memset(g_cv, 0, (size_t)cw * ch); memset(g_cm, 0, (size_t)cw * ch);
				cl.x0 = 0; cl.y0 = 0; cl.x1 = cw; cl.y1 = ch;
				/* box origin at (-2*x0, -2*y0): the frame lands at (f.x - 2*x0, f.y - 2*y0) of the canvas;
				 * drawn once with an all-ones remap (= opacity mask), once for the colours */
				w2_sprite_draw(spr, size, type, i, g_cm, cw, -2 * x0, -2 * y0, &cl, 0, g_ones);
				w2_sprite_draw(spr, size, type, i, g_cv, cw, -2 * x0, -2 * y0, &cl, 0, NULL);
				lores_halve(g_cv, g_cm, cw, ch, cw, g_hv, g_hm, ow, q);
				/* tight box of what stayed opaque */
				minx = ow; maxx = -1; miny = oh; maxy = -1;
				for (y = 0; y < oh; y++) for (x = 0; x < ow; x++) if (g_hm[y * ow + x]) {
					if (x < minx) minx = x;
					if (x > maxx) maxx = x;
					if (y < miny) miny = y;
					if (y > maxy) maxy = y;
				}
				if (maxx >= 0) {
					unsigned char *rt;
					nx = x0 + minx; ny = y0 + miny; nw = maxx - minx + 1; nh = maxy - miny + 1;
					rt = buf_grow(&b, 2UL * nh);
					if (!rt) break;
					for (y = 0; y < nh; y++) {
						put16(b.p + off + 2 * y, b.n - off);
						rle_row(&b, g_hv + (miny + y) * ow + minx, g_hm + (miny + y) * ow + minx, nw);
					}
					if (b.n - off > 0xFFFF) { w2_log("lores: frame %d too big for RLE", i); b.err = 1; break; }
				}
			} else {
				unsigned char *d;
				const unsigned char *px = spr + f.offset;
				if (f.offset + (unsigned long)f.w * f.h > size) break;
				ow = (f.w + 1) >> 1; oh = (f.h + 1) >> 1;
				if (canvas_reserve(1, (long)ow * oh)) { b.err = 1; break; }
				lores_halve(px, NULL, f.w, f.h, f.w, g_hv, NULL, ow, q);
				nw = ow; nh = oh;
				if (nx + nw > ((bw + 1) >> 1)) nw = ((bw + 1) >> 1) - nx;
				if (ny + nh > ((bh + 1) >> 1)) nh = ((bh + 1) >> 1) - ny;
				d = buf_grow(&b, (unsigned long)nw * nh);
				if (!d) break;
				for (y = 0; y < nh; y++) memcpy(d + y * nw, g_hv + y * ow, (size_t)nw);
			}
		}
		if (b.err) break;
		rec = b.p + 6 + 8 * i;
		rec[0] = (unsigned char)nx; rec[1] = (unsigned char)ny; rec[2] = (unsigned char)(nw & 0xFF); rec[3] = (unsigned char)nh;
		put32(rec + 4, (nw ? off : 0) | (nw >= 256 ? 0x80000000UL : 0));
		if (!nw || !nh) { rec[2] = rec[3] = 0; put32(rec + 4, off); }
	}
	if (b.err || i < n) { free(b.p); return NULL; }
	if (out_size) *out_size = b.n;
	return b.p;
}

unsigned char *lores_image_halve(const unsigned char *img, unsigned long size, const LoresQ *q, unsigned long *out_size)
{
	int w, h, ow, oh; unsigned char *o;
	const unsigned char *px = w2_image_probe(img, size, &w, &h);
	if (out_size) *out_size = 0;
	if (!px) return NULL;
	ow = (w + 1) >> 1; oh = (h + 1) >> 1;
	o = (unsigned char *)malloc(4 + (size_t)ow * oh);
	if (!o) return NULL;
	put16(o, (unsigned long)ow); put16(o + 2, (unsigned long)oh);
	lores_halve(px, NULL, w, h, w, o + 4, NULL, ow, q);
	if (out_size) *out_size = 4 + (unsigned long)ow * oh;
	return o;
}

unsigned char *lores_tiles_halve(const unsigned char *mega32, int n, const LoresQ *q)
{
	int t;
	unsigned char *o = (unsigned char *)malloc((size_t)n * 256 + 1);
	if (!o) return NULL;
	for (t = 0; t < n; t++) lores_halve(mega32 + (long)t * 1024, NULL, 32, 32, 32, o + (long)t * 256, NULL, 16, q);
	return o;
}

/* ================================================================ quantiser cache */

static LoresQ *g_era_q;           /* the current era's (with classes) */
static int g_era_q_era = -1;
static LoresQ *g_pic_q[2];        /* picture palettes */
static int g_pic_next;

const LoresQ *lores_q_for(const unsigned char *pal768)
{
	int i;
	if (g_era_q && g_era_q_era >= 0 && !memcmp(g_era_q->pal, pal768, 768) && q_dst_same(g_era_q)) return g_era_q;
	for (i = 0; i < 2; i++) if (g_pic_q[i] && !memcmp(g_pic_q[i]->pal, pal768, 768) && q_dst_same(g_pic_q[i])) return g_pic_q[i];
	i = g_pic_next; g_pic_next ^= 1;
	if (!g_pic_q[i]) g_pic_q[i] = (LoresQ *)malloc(sizeof(LoresQ));
	if (!g_pic_q[i]) {                 /* no memory: fall back to the era's table */
		if (!g_era_q) g_era_q = (LoresQ *)malloc(sizeof(LoresQ));
		if (!g_era_q) { w2_log("lores: out of memory"); exit(20); }
		lores_q_build(g_era_q, pal768, -1); g_era_q_era = -1;
		return g_era_q;
	}
	lores_q_build(g_pic_q[i], pal768, -1);
	return g_pic_q[i];
}

/* ================================================================ era packs */

#define PACK_FORMAT 1
#define PACK_FILTER 4            /* Lanczos-3 + 2x2 edge average + class vote + checker dither kept */
#define PACK_HDR 32

/* sprite entries of War Data in a pack; entries of the OTHER eras are left out (a missing one is
 * still halved on first use). */
static const short k_ranges[][2] = { { 33, 191 }, { 252, 276 }, { 323, 358 } };
static const short k_only_winter[] = { 66, 128, -163, 169, -172, 184, 186, 190, 262, -270, 273, 275, 357, 0 };
static const short k_only_waste[]  = { 65, 173, -180, 182, 183, 185, 188, 191, 271, 272, 274, 276, 358, 0 };
static const short k_only_summer[] = { 64, 356, 0 };

static int in_list(const short *l, int e)
{
	int i;
	for (i = 0; l[i]; i++) {
		if (l[i + 1] < 0) { if (e >= l[i] && e <= -l[i + 1]) return 1; i++; continue; }
		if (e == l[i]) return 1;
	}
	return 0;
}

static int pack_wants(int era, int e)
{
	if (era != 1 && in_list(k_only_winter, e)) return 0;
	if (era != 2 && in_list(k_only_waste, e)) return 0;
	if (era != 0 && in_list(k_only_summer, e)) return 0;
	return 1;
}

static const char *const k_era_name[3] = { "summer", "winter", "wasteland" };

/* ---- EHB: the era's palette is weighted by what the era really draws: megatiles + pack sprites ---- */
#define EHB_TRAILER 196          /* "EHBP" + 64 x RGB at the end of an ehb pack */

static void hist_sprite(const unsigned char *spr, unsigned long size, int type, unsigned long *hist)
{
	int n = (int)W2_LE16(spr), i, j;
	if (!g_ones[0]) memset(g_ones, 1, sizeof g_ones);
	for (i = 0; i < n; i++) {
		W2Frame f; long k, np;
		if (w2_sprite_frame(spr, size, i, &f) < 0) break;
		for (j = 0; j < i; j++) {
			W2Frame g;
			w2_sprite_frame(spr, size, j, &g);
			if (g.offset == f.offset && g.x == f.x && g.y == f.y && g.w == f.w && g.h == f.h) break;
		}
		if (j < i || f.w <= 0 || f.h <= 0) continue;
		np = (long)f.w * f.h;
		if (type == W2_SPR_RLE) {
			W2Clip cl;
			if (canvas_reserve(np, 1)) return;
			memset(g_cv, 0, (size_t)np); memset(g_cm, 0, (size_t)np);
			cl.x0 = 0; cl.y0 = 0; cl.x1 = f.w; cl.y1 = f.h;
			w2_sprite_draw(spr, size, type, i, g_cm, f.w, -f.x, -f.y, &cl, 0, g_ones);
			w2_sprite_draw(spr, size, type, i, g_cv, f.w, -f.x, -f.y, &cl, 0, NULL);
			for (k = 0; k < np; k++) if (g_cm[k]) hist[g_cv[k]]++;
		} else {
			const unsigned char *px = spr + f.offset;
			if (f.offset + (unsigned long)np > size) break;
			for (k = 0; k < np; k++) hist[px[k]]++;
		}
	}
}

void lores_ehb_era_palette(int era, const unsigned char *pal768, const unsigned char *mega32, int n, unsigned char *e192)
{
	static unsigned long ht[256], hs[256];
	long k, nb = (long)n * 1024;
	int r, e;
	memset(ht, 0, sizeof ht); memset(hs, 0, sizeof hs);
	for (k = 0; k < nb; k++) ht[mega32[k]]++;
	for (r = 0; r < 3; r++) for (e = k_ranges[r][0]; e <= k_ranges[r][1]; e++) {
		unsigned long sz; unsigned char *src; int type, cnt;
		if (!pack_wants(era, e)) continue;
		src = assets_load(ASSET_DATA, e, &sz);
		type = src ? w2_sprite_probe(src, sz, &cnt, NULL, NULL) : 0;
		if (type) hist_sprite(src, sz, type, hs);
		war_free(src);
	}
	ehb_build(pal768, ht, hs, 1, e192);
	w2_log("ehb: %s palette: mean error %lu after seeding, %lu final (%lu passes)", k_era_name[era], ehb_err_init, ehb_err_final, ehb_evals);
}

static void pack_path(char *out, int n, int era)
{
	const char *dir = w2cfg_extra("lores_pack_dir");
	size_t l;
	if (!dir || !*dir) dir = w2_cfg.data_dir[0] ? w2_cfg.data_dir : "PROGDIR:data";
	l = strlen(dir);
	snprintf(out, (size_t)n, "%s%s%s-%s.pack", dir, (l && dir[l - 1] != ':' && dir[l - 1] != '/') ? "/" : "",
	         assets_ehb() ? "ehb" : "lores", k_era_name[era]);
}

/* source stamp: which entries, their sizes, the terrain entry sizes */
static unsigned long src_stamp(int era, int n_mega)
{
	WarArchive *a = assets_war(ASSET_DATA);
	unsigned long s = 0x5EED0000UL ^ (unsigned long)n_mega;
	int r, e, k;
	if (!a) return 0;
	for (k = 0; k < 4; k++) s = ROT32(s, 7) ^ (unsigned long)war_entry_size(a, assets_era_entry(era, k));
	for (r = 0; r < 3; r++)
		for (e = k_ranges[r][0]; e <= k_ranges[r][1]; e++)
			if (pack_wants(era, e)) s = ROT32(s, 7) ^ ((unsigned long)war_entry_size(a, e) + (unsigned long)e);
	return s & 0xFFFFFFFFUL;
}

static unsigned char *g_pack;     /* the loaded pack blob */
static unsigned long g_pack_n;
static int g_pack_era = -1, g_pack_mega;
static unsigned long g_pack_dir_n;
static void (*g_progress)(int, int, int);

void lores_set_progress(void (*cb)(int era, int done, int total)) { g_progress = cb; }

static void progress(int era, int done, int total)
{
	static int last = -1;
	if (g_progress) { g_progress(era, done, total); return; }
	if (total <= 0) return;
	if (done == 0) last = -1;
	if (done * 10 / total != last) {
		last = done * 10 / total;
		w2_log("%s: building %s art %d%%", assets_ehb() ? "ehb" : "lores", k_era_name[era], done * 100 / total);
	}
}

/* parse + check a pack blob; 0 = good */
static int pack_check(const unsigned char *p, unsigned long n, unsigned long palsum, unsigned long srcsum, int n_mega)
{
	unsigned long nd, i, dir;
	if (n < PACK_HDR || memcmp(p, "W2LP", 4)) return -1;
	if (assets_ehb()) {            /* the era's EHB palette closes the file */
		if (n < PACK_HDR + EHB_TRAILER || memcmp(p + n - EHB_TRAILER, "EHBP", 4)) return -1;
		n -= EHB_TRAILER;
	}
	if (W2_LE32(p + 4) != PACK_FORMAT || W2_LE32(p + 8) != PACK_FILTER) return -1;
	if (W2_LE32(p + 12) != palsum || W2_LE32(p + 16) != srcsum || W2_LE32(p + 20) != (unsigned long)n_mega) return -1;
	nd = W2_LE32(p + 24);
	dir = PACK_HDR + (unsigned long)n_mega * 256;
	if (dir + nd * 16 > n) return -1;
	for (i = 0; i < nd; i++) {
		const unsigned char *d = p + dir + i * 16;
		if (W2_LE32(d + 4) > n || W2_LE32(d + 8) > n - W2_LE32(d + 4)) return -1;
	}
	return 0;
}

static int pack_adopt(unsigned char *p, unsigned long n, int era, int n_mega)
{
	lores_era_end();
	g_pack = p; g_pack_n = n; g_pack_era = era; g_pack_mega = n_mega;
	g_pack_dir_n = W2_LE32(p + 24);
	return 0;
}

static unsigned char *pack_read(const char *path, unsigned long *n)
{
	FILE *f = fopen(path, "rb");
	unsigned char hdr[PACK_HDR], *p;
	unsigned long len;
	*n = 0;
	if (!f) return NULL;
	if (fread(hdr, 1, PACK_HDR, f) != PACK_HDR || memcmp(hdr, "W2LP", 4)) { fclose(f); return NULL; }
	len = W2_LE32(hdr + 28);
	if (len < PACK_HDR || len > 64UL * 1024 * 1024) { fclose(f); return NULL; }
	p = (unsigned char *)malloc(len);
	if (!p) { fclose(f); return NULL; }
	memcpy(p, hdr, PACK_HDR);
	if (fread(p + PACK_HDR, 1, len - PACK_HDR, f) != len - PACK_HDR) { fclose(f); free(p); return NULL; }
	fclose(f);
	*n = len;
	return p;
}

/* build the pack of an era in memory (tiles from mega32), save it; returns the blob */
static unsigned char *pack_build(int era, const unsigned char *mega32, int n_mega, const LoresQ *q,
                                 unsigned long srcsum, unsigned long *out_n)
{
	Buf b, data; int r, e, total = 0, done = 0, nd = 0, k;
	unsigned char *h;
	unsigned long start = amigagfx_millis(), dirpos;
	char path[300];
	*out_n = 0;
	for (r = 0; r < 3; r++) for (e = k_ranges[r][0]; e <= k_ranges[r][1]; e++) if (pack_wants(era, e)) nd++;
	total = nd + (n_mega + 15) / 16;
	progress(era, 0, total);
	memset(&b, 0, sizeof b); memset(&data, 0, sizeof data);
	h = buf_grow(&b, PACK_HDR);
	if (!h) return NULL;
	memcpy(h, "W2LP", 4);
	put32(h + 4, PACK_FORMAT); put32(h + 8, PACK_FILTER); put32(h + 12, q->sum); put32(h + 16, srcsum);
	put32(h + 20, (unsigned long)n_mega); put32(h + 24, 0); put32(h + 28, 0);
	for (k = 0; k < n_mega; k += 16) {
		int m, cnt = n_mega - k < 16 ? n_mega - k : 16;
		unsigned char *t = buf_grow(&b, (unsigned long)cnt * 256);
		if (!t) break;
		for (m = 0; m < cnt; m++) lores_halve(mega32 + (long)(k + m) * 1024, NULL, 32, 32, 32, t + m * 256, NULL, 16, q);
		progress(era, ++done, total);
	}
	dirpos = b.n;
	nd = 0;
	for (r = 0; r < 3; r++) for (e = k_ranges[r][0]; e <= k_ranges[r][1]; e++) {
		unsigned long sz, hs; unsigned char *src, *half; int type, cnt, bw, bh;
		if (!pack_wants(era, e)) continue;
		src = assets_load(ASSET_DATA, e, &sz);
		type = src ? w2_sprite_probe(src, sz, &cnt, &bw, &bh) : 0;
		half = type ? lores_sprite_halve(src, sz, type, q, &hs) : NULL;
		if (half) {
			unsigned char *d = buf_grow(&data, hs), *de;
			if (d) memcpy(d, half, hs);
			de = buf_grow(&b, 16);
			if (de) { put16(de, (unsigned long)(ASSET_DATA | (type << 8))); put16(de + 2, (unsigned long)e); put32(de + 4, data.n - hs); put32(de + 8, hs); put16(de + 12, (unsigned long)bw); put16(de + 14, (unsigned long)bh); }
			nd++;
			lores_stats.sprites++; lores_stats.bytes_in += (long)sz; lores_stats.bytes_out += (long)hs;
		}
		free(half); war_free(src);
		progress(era, ++done, total);
	}
	if (b.err || data.err) { free(b.p); free(data.p); w2_log("lores: out of memory building %s", k_era_name[era]); return NULL; }
	/* data offsets become absolute */
	{
		unsigned long base = b.n, i;
		for (i = 0; i < (unsigned long)nd; i++) { unsigned char *de = b.p + dirpos + i * 16; put32(de + 4, W2_LE32(de + 4) + base); }
		h = buf_grow(&b, data.n);
		if (!h) { free(b.p); free(data.p); return NULL; }
		memcpy(h, data.p, data.n);
		free(data.p);
	}
	if (q->dst) {                  /* EHB: the palette this art was converted to */
		h = buf_grow(&b, EHB_TRAILER);
		if (!h) { free(b.p); return NULL; }
		memcpy(h, "EHBP", 4); memcpy(h + 4, q->dpal, 192);
	}
	put32(b.p + 24, (unsigned long)nd); put32(b.p + 28, b.n);
	lores_stats.ms += amigagfx_millis() - start;
	progress(era, total, total);
	pack_path(path, sizeof path, era);
	{
		FILE *f = fopen(path, "wb");
		int ok = f && fwrite(b.p, 1, b.n, f) == b.n;
		if (f && fclose(f)) ok = 0;
		if (!ok) { w2_log("lores: could not write %s (kept in memory only)", path); if (f) remove(path); }
		else w2_log("%s: wrote %s (%lu bytes, %d sprites, %lu ms)", assets_ehb() ? "ehb" : "lores", path, b.n, nd, amigagfx_millis() - start);
	}
	*out_n = b.n;
	return b.p;
}

static int era_q_ready(int era, const unsigned char *pal768)
{
	if (!g_era_q) g_era_q = (LoresQ *)malloc(sizeof(LoresQ));
	if (!g_era_q) return -1;
	if (g_era_q_era != era || memcmp(g_era_q->pal, pal768, 768) || !q_dst_same(g_era_q)) { lores_q_build(g_era_q, pal768, era); g_era_q_era = era; }
	return 0;
}

const unsigned char *lores_era_ehb(void)
{
	if (!g_pack || !assets_ehb() || g_pack_n < PACK_HDR + EHB_TRAILER) return NULL;
	return g_pack + g_pack_n - EHB_TRAILER + 4;
}

/* EHB: the pack is found by a stamp that needs no palette; the palette comes out of the pack (or is
 * built with it) and becomes the destination of the era's quantiser */
static int era_begin_ehb(int era, const unsigned char *mega32, int n, const unsigned char *pal768)
{
	unsigned long srcsum, len, sum = lores_ehb_sum(pal768, era); unsigned char *p; char path[300];
	unsigned char e[192];
	if (!(g_pack && g_pack_era == era && g_pack_mega == n && W2_LE32(g_pack + 12) == sum)) {
		srcsum = src_stamp(era, n);
		pack_path(path, sizeof path, era);
		p = pack_read(path, &len);
		if (p && pack_check(p, len, sum, srcsum, n)) { w2_log("ehb: %s is stale - rebuilding", path); free(p); p = NULL; }
		if (!p) {
			lores_ehb_era_palette(era, pal768, mega32, n, e);
			lores_set_dst(e);
			if (era_q_ready(era, pal768)) return -1;
			p = pack_build(era, mega32, n, g_era_q, srcsum, &len);
			if (!p) return -1;
		}
		pack_adopt(p, len, era, n);
	}
	lores_set_dst(lores_era_ehb());
	return era_q_ready(era, pal768);
}

int lores_era_begin(int era, const unsigned char *mega32, int n, const unsigned char *pal768)
{
	unsigned long srcsum, len; unsigned char *p; char path[300];
	if (era < 0 || era > 2 || !mega32 || n <= 0) return -1;
	if (assets_ehb()) return era_begin_ehb(era, mega32, n, pal768);
	if (era_q_ready(era, pal768)) return -1;
	if (g_pack && g_pack_era == era && g_pack_mega == n && W2_LE32(g_pack + 12) == g_era_q->sum) return 0;
	srcsum = src_stamp(era, n);
	pack_path(path, sizeof path, era);
	p = pack_read(path, &len);
	if (p && !pack_check(p, len, g_era_q->sum, srcsum, n)) return pack_adopt(p, len, era, n);
	if (p) w2_log("lores: %s is stale - rebuilding", path);
	free(p);
	p = pack_build(era, mega32, n, g_era_q, srcsum, &len);
	if (!p) return -1;
	return pack_adopt(p, len, era, n);
}

void lores_era_end(void)
{
	free(g_pack); g_pack = NULL; g_pack_n = 0; g_pack_era = -1; g_pack_mega = 0; g_pack_dir_n = 0;
}

const unsigned char *lores_era_tiles(void) { return g_pack ? g_pack + PACK_HDR : NULL; }

const unsigned char *lores_era_sprite(int which, int entry, unsigned long *size, int *type, int *box_w, int *box_h)
{
	unsigned long i, dir;
	if (size) *size = 0;
	if (!g_pack) return NULL;
	dir = PACK_HDR + (unsigned long)g_pack_mega * 256;
	for (i = 0; i < g_pack_dir_n; i++) {
		const unsigned char *d = g_pack + dir + i * 16;
		if ((int)(W2_LE16(d) & 0xFF) == which && (int)W2_LE16(d + 2) == entry) {
			if (size) *size = W2_LE32(d + 8);
			if (type) *type = (int)(W2_LE16(d) >> 8);
			if (box_w) *box_w = (int)W2_LE16(d + 12);
			if (box_h) *box_h = (int)W2_LE16(d + 14);
			return g_pack + W2_LE32(d + 4);
		}
	}
	return NULL;
}

/* At game start in lores: make sure every era's pack exists and is current (build the missing ones
 * now, with progress, so that starting a map never waits). Packs are loaded later by assets_set_era. */
int lores_prepare(void)
{
	int era, bad = 0;
	if (!assets_lores()) return 0;
	for (era = 0; era < 3; era++) {
		unsigned long ps, ms, ns, len, srcsum, qsum; int n = 0, i;
		unsigned char *pal6 = assets_load(ASSET_DATA, assets_era_entry(era, 0), &ps);
		unsigned char *mega, *mini, *tiles, *p, pal[768];
		char path[300];
		if (!pal6 || ps < 768) { war_free(pal6); bad++; continue; }
		for (i = 0; i < 768; i++) { int v = pal6[i] & 63; pal[i] = (unsigned char)((v << 2) | (v >> 4)); }
		war_free(pal6);
		/* EHB: the stamp does not need the EHB palette (which needs the megatiles) */
		if (assets_ehb()) qsum = lores_ehb_sum(pal, era);
		else { if (era_q_ready(era, pal)) return -1; qsum = g_era_q->sum; }
		ms = (unsigned long)war_entry_size(assets_war(ASSET_DATA), assets_era_entry(era, 1));
		n = w2_tileset_count(ms);
		srcsum = src_stamp(era, n);
		pack_path(path, sizeof path, era);
		/* header-only check */
		{
			FILE *f = fopen(path, "rb");
			unsigned char hdr[PACK_HDR];
			int ok = f && fread(hdr, 1, PACK_HDR, f) == PACK_HDR && !memcmp(hdr, "W2LP", 4) &&
			         W2_LE32(hdr + 4) == PACK_FORMAT && W2_LE32(hdr + 8) == PACK_FILTER &&
			         W2_LE32(hdr + 12) == qsum && W2_LE32(hdr + 16) == srcsum && W2_LE32(hdr + 20) == (unsigned long)n;
			if (f) fclose(f);
			if (ok) { w2_log("%s: %s ok", assets_ehb() ? "ehb" : "lores", path); continue; }
		}
		mega = assets_load(ASSET_DATA, assets_era_entry(era, 1), &ms);
		mini = assets_load(ASSET_DATA, assets_era_entry(era, 2), &ns);
		tiles = (mega && mini) ? w2_tileset_build(mega, ms, mini, ns, &n) : NULL;
		war_free(mega); war_free(mini);
		if (!tiles) { bad++; continue; }
		if (assets_ehb()) {
			unsigned char e[192];
			lores_ehb_era_palette(era, pal, tiles, n, e);
			lores_set_dst(e);
			if (era_q_ready(era, pal)) { free(tiles); return -1; }
		}
		p = pack_build(era, tiles, n, g_era_q, srcsum, &len);
		free(tiles);
		if (!p) { bad++; continue; }
		free(p);
	}
	return bad ? -1 : 0;
}
