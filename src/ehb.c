/* ehb.c - see ehb.h. Plain C, integers only (32-bit long is enough), no floats: the same bytes on the
 * PC converter and on the Amiga. */
#include <string.h>
#include "ehb.h"

/* red, blue, green, violet, orange, black, white, yellow (the order of the ramps 208.., 12..15) */
const unsigned char ehb_player_reg[8] = { 1, 2, 3, 4, 5, 17, 19, 6 };

unsigned long ehb_err_init, ehb_err_final, ehb_evals;

#define W_SHARE 4096           /* total weight of one histogram */
#define W_FLOOR 1              /* every colour's weight on top (art that was not counted: UI, fonts) */
#define W_FLAT 16              /* no histogram at all: every colour the same */
#define LLOYD_MAX 8            /* k-means passes per round (it stops as soon as a pass does not help) */
#define SWAP_MAX 16            /* rounds of "drop the least useful register, re-seed it at the worst colour" */
#define SWAP_FAILS 3           /* ... given up after this many rounds in a row that did not help */

/* weighted squared distance (r 3, g 4, b 2 - as everywhere in the game) from tables: no multiply */
static long g_d3[511], g_d4[511], g_d2[511];
static int g_d_ok;
#define DIST(c, r, g, b) (g_d3[255 + (c)[0] - (r)] + g_d4[255 + (c)[1] - (g)] + g_d2[255 + (c)[2] - (b)])

static void dist_init(void)
{
	int v;
	if (g_d_ok) return;
	for (v = -255; v <= 255; v++) { long q = (long)v * v; g_d3[255 + v] = 3 * q; g_d4[255 + v] = 4 * q; g_d2[255 + v] = 2 * q; }
	g_d_ok = 1;
}

/* distance x weight in 32 bits (weights reach 8193: the largest distances are clamped) */
static unsigned long wmul(long d, unsigned w)
{
	if (d > 500000L) d = 500000L;
	return (unsigned long)d * w;
}

/* register r = the 12-bit colour nearest to R,G,B; its half is what the hardware shows */
static void set_reg(unsigned char *e, int r, int R, int G, int B)
{
	int v[3], k;
	v[0] = R; v[1] = G; v[2] = B;
	for (k = 0; k < 3; k++) {
		int n = (v[k] + 8) / 17;
		if (n < 0) n = 0; else if (n > 15) n = 15;
		e[r * 3 + k] = (unsigned char)(n * 17);
		e[(r + 32) * 3 + k] = (unsigned char)((n >> 1) * 17);
	}
}

/* register r seeded by the source colour p: itself, or - a dark colour - the register whose HALF is
 * nearest to it (even nibbles: the hardware drops the low bit) */
static void seed_reg(unsigned char *e, int r, const unsigned char *p)
{
	int k;
	if (p[0] > 127 || p[1] > 127 || p[2] > 127) { set_reg(e, r, p[0], p[1], p[2]); return; }
	for (k = 0; k < 3; k++) {
		int h = (p[k] + 8) / 17;
		e[r * 3 + k] = (unsigned char)(2 * h * 17);
		e[(r + 32) * 3 + k] = (unsigned char)(h * 17);
	}
}

/* The best 4-bit gun for a register whose cluster has full-brightness points (sum sf, weight wf) and
 * half-brightness points (sh, wh): minimises wf (mf - 17n)^2 + wh (mh - 17 (n >> 1))^2 exactly. */
static int best_gun(long sf, long wf, long sh, long wh)
{
	int n, bn = 0; long bc = 0x7fffffffL;
	for (n = 0; n < 16; n++) {
		long f = 17L * n, h = 17L * (n >> 1);
		long c = wf * f * f - 2 * f * sf + wh * h * h - 2 * h * sh;
		if (c < bc) { bc = c; bn = n; }
	}
	return bn;
}

int ehb_bright_first(const unsigned char *s)
{
	int a = 0, b = 0, k;
	for (k = 0; k < 6; k++) { a += s[208 * 3 + k]; b += s[210 * 3 + k]; }
	return a >= b;
}

int ehb_nearest(const unsigned char *e, int r, int g, int b, int deny_ph)
{
	int i, best = 0; long bd = 0x7fffffffL;
	dist_init();
	for (i = 0; i < 64; i++) {
		long d;
		if (deny_ph && (i & 31) == EHB_PH) continue;
		d = DIST(e + i * 3, r, g, b);
		if (d < bd) { bd = d; best = i; }
	}
	return best;
}

void ehb_virtual(const unsigned char *e, unsigned char *pal)
{
	int i;
	for (i = 0; i < 256; i++) {
		int j = i & 63;
		if (i >= 64 && j >= 17 && j <= 19) j = 1;
		memcpy(pal + i * 3, e + j * 3, 3);
	}
}

/* ---------------------------------------------------------------- the optimiser's state */
static const unsigned char *S;             /* source palette */
static unsigned short W[256], WS[256];     /* weight; weight a colour may SEED a register with */
static unsigned char who[256];             /* serving entry 0..63 */
static long d1[256], d2[256];              /* distance to it; to the best entry of ANOTHER register */
static unsigned char isfree[32];
static int g_era;                          /* era palette: the placeholder pair serves nothing */

/* assign every colour (who, d1, and with second != 0 also d2); returns the weighted error in units of
 * 256 (the plain sum can need more than 32 bits) */
static unsigned long evaluate(const unsigned char *e, int second)
{
	unsigned long hi = 0, lo = 0;
	int i, k;
	ehb_evals++;
	for (i = 0; i < 256; i++) {
		const unsigned char *p = S + i * 3;
		long b1 = 0x7fffffffL, b2 = 0x7fffffffL; int w1 = 0;
		unsigned long x;
		who[i] = 0; d1[i] = 0; d2[i] = 0;
		if (!W[i]) continue;
		for (k = 0; k < 64; k++) {
			long d;
			if (g_era && (k & 31) == EHB_PH) continue;
			d = DIST(e + k * 3, p[0], p[1], p[2]);
			if (d < b1) {
				if (second && (w1 & 31) != (k & 31)) b2 = b1;
				b1 = d; w1 = k;
			} else if (second && d < b2 && (w1 & 31) != (k & 31)) b2 = d;
		}
		who[i] = (unsigned char)w1; d1[i] = b1; d2[i] = b2;
		x = wmul(b1, W[i]);
		hi += x >> 8; lo += x & 0xFF;
	}
	return hi + (lo >> 8);
}

/* one k-means pass: every free register moves to the best 12-bit colour for its cluster */
static void lloyd_pass(unsigned char *e)
{
	static long sf[32][3], sh[32][3], wf[32], wh[32];
	static unsigned char used[256];
	int i, k, r, r2;
	memset(used, 0, sizeof used);
	memset(sf, 0, sizeof sf); memset(sh, 0, sizeof sh); memset(wf, 0, sizeof wf); memset(wh, 0, sizeof wh);
	for (i = 0; i < 256; i++) {
		const unsigned char *p = S + i * 3;
		int c = who[i] & 31;
		if (!W[i]) continue;
		if (who[i] < 32) { for (k = 0; k < 3; k++) sf[c][k] += (long)p[k] * W[i]; wf[c] += W[i]; }
		else             { for (k = 0; k < 3; k++) sh[c][k] += (long)p[k] * W[i]; wh[c] += W[i]; }
	}
	for (r = 0; r < 32; r++) {
		unsigned char old[3];
		if (!isfree[r] || !(wf[r] + wh[r])) continue;
		memcpy(old, e + r * 3, 3);
		for (k = 0; k < 3; k++) {
			int n = best_gun(sf[r][k], wf[r], sh[r][k], wh[r]);
			e[r * 3 + k] = (unsigned char)(n * 17);
			e[(r + 32) * 3 + k] = (unsigned char)((n >> 1) * 17);
		}
		/* never onto another register's colour: that would waste one of the two */
		for (r2 = 0; r2 < 32; r2++) if (r2 != r && !memcmp(e + r2 * 3, e + r * 3, 3)) break;
		if (r2 < 32) set_reg(e, r, old[0], old[1], old[2]);
	}
	/* a register that serves nothing goes to the colour that is served worst (each colour once) */
	for (r = 0; r < 32; r++) {
		int tries;
		if (!isfree[r] || wf[r] + wh[r]) continue;
		for (tries = 0; tries < 8; tries++) {
			unsigned long bs = 0; int bi = -1;
			unsigned char old[6];
			for (i = 0; i < 256; i++) {
				unsigned long x;
				if (!WS[i] || used[i]) continue;
				x = wmul(d1[i], WS[i]);
				if (x > bs) { bs = x; bi = i; }
			}
			if (bi < 0) break;
			used[bi] = 1;
			memcpy(old, e + r * 3, 3); memcpy(old + 3, e + (r + 32) * 3, 3);
			seed_reg(e, r, S + bi * 3);
			for (r2 = 0; r2 < 32; r2++) if (r2 != r && !memcmp(e + r2 * 3, e + r * 3, 3)) break;
			if (r2 == 32) break;
			memcpy(e + r * 3, old, 3); memcpy(e + (r + 32) * 3, old + 3, 3);
		}
	}
}

/* k-means until the palette stops moving (on the 12-bit grid a pass may make it a little worse before
 * it gets better); e ends as the best palette seen, with who/d1/d2 for it */
static unsigned long lloyd(unsigned char *e, unsigned long cur)
{
	unsigned char prev[192], best[192];
	int it;
	memcpy(best, e, 192);
	for (it = 0; it < LLOYD_MAX; it++) {
		unsigned long n;
		memcpy(prev, e, 192);
		lloyd_pass(e);
		if (!memcmp(prev, e, 192)) break;
		n = evaluate(e, 0);
		if (n < cur) { cur = n; memcpy(best, e, 192); }
	}
	memcpy(e, best, 192);
	return evaluate(e, 1);
}

void ehb_build(const unsigned char *s, const unsigned long *hist_a, const unsigned long *hist_b, int era_mode, unsigned char *e)
{
	static long dmin[256];
	static unsigned char tried[256];       /* colours whose seeding did not help */
	unsigned char def[32], keep[192];
	unsigned long wsum = 0, cur;
	int i, r, k, swap, guard, fails = 0;

	dist_init();
	S = s; g_era = era_mode;
	ehb_evals = 0;

	/* weights */
	for (i = 0; i < 256; i++) W[i] = 0;
	for (k = 0; k < 2; k++) {
		const unsigned long *h = k ? hist_b : hist_a;
		unsigned long total = 0, unit;
		if (!h) continue;
		for (i = 0; i < 256; i++) if (!(era_mode && i >= 208 && i <= 211)) total += h[i];
		unit = total / W_SHARE + 1;
		for (i = 0; i < 256; i++) W[i] = (unsigned short)(W[i] + h[i] / unit);
	}
	for (i = 0; i < 256; i++) {
		/* colours no counted art uses (the palettes carry pure test colours) still get the floor, so
		 * uncounted art is served somehow - but they never seed a register */
		if (hist_a || hist_b) { WS[i] = W[i]; W[i] = (unsigned short)(W[i] + W_FLOOR); }
		else WS[i] = W[i] = W_FLAT;
	}
	if (era_mode) for (i = 208; i <= 211; i++) W[i] = WS[i] = 0;   /* the placeholder ramp is never shown as itself */
	for (i = 0; i < 256; i++) wsum += W[i];

	/* fixed registers */
	memset(e, 0, 192);
	memset(def, 0, sizeof def);
	set_reg(e, 0, 0, 0, 0); def[0] = 1;
	set_reg(e, 18, 152, 152, 152); def[18] = 1;
	if (era_mode) {
		int bf = ehb_bright_first(s), p;
		for (p = 0; p < 8; p++) {
			const unsigned char *c0 = s + EHB_RAMP(p, bf ? 0 : 2) * 3, *c1 = s + EHB_RAMP(p, bf ? 1 : 3) * 3;
			const unsigned char *q0 = s + EHB_RAMP(p, bf ? 2 : 0) * 3, *q1 = s + EHB_RAMP(p, bf ? 3 : 1) * 3;
			int v[3];
			/* r ~ a (mean of the bright pair), r/2 ~ b (mean of the dark pair): r = (4a + 2b) / 5 */
			for (k = 0; k < 3; k++) v[k] = (2 * (c0[k] + c1[k]) + (q0[k] + q1[k]) + 2) / 5;
			set_reg(e, ehb_player_reg[p], v[0], v[1], v[2]);
			def[ehb_player_reg[p]] = 1;
		}
	} else {
		set_reg(e, 17, 40, 40, 40); def[17] = 1;
		set_reg(e, 19, 240, 240, 240); def[19] = 1;
	}
	for (r = 0; r < 32; r++) isfree[r] = (unsigned char)!def[r];

	/* farthest-point initialisation: the colour with the largest weighted error seeds the next free
	 * register (a dark colour seeds the register whose HALF it is) */
	for (i = 0; i < 256; i++) {
		long b = 0x7fffffffL;
		for (k = 0; k < 64; k++) {
			long d;
			if (!def[k & 31] || (era_mode && (k & 31) == EHB_PH)) continue;
			d = DIST(e + k * 3, s[i * 3], s[i * 3 + 1], s[i * 3 + 2]);
			if (d < b) b = d;
		}
		dmin[i] = b;
	}
	for (r = 0; r < 32; r++) {
		unsigned long bs = 0; int bi = -1;
		if (!isfree[r]) continue;
		for (i = 0; i < 256; i++) {
			unsigned long x = wmul(dmin[i], WS[i]);
			if (x > bs) { bs = x; bi = i; }
		}
		if (bi < 0) {
			/* every counted colour is exact already: a grey nobody has yet, never a duplicate */
			int v;
			for (v = 255; v > 0; v -= 17) {
				set_reg(e, r, v, v, v);
				for (k = 0; k < 32; k++) if (k != r && (def[k] || !isfree[k]) && !memcmp(e + k * 3, e + r * 3, 3)) break;
				if (k == 32) break;
			}
			def[r] = 1;
			continue;
		}
		seed_reg(e, r, s + bi * 3);
		def[r] = 1;
		for (i = 0; i < 256; i++) {
			long d = DIST(e + r * 3, s[i * 3], s[i * 3 + 1], s[i * 3 + 2]);
			if (d < dmin[i]) dmin[i] = d;
			d = DIST(e + (r + 32) * 3, s[i * 3], s[i * 3 + 1], s[i * 3 + 2]);
			if (d < dmin[i]) dmin[i] = d;
		}
	}

	cur = evaluate(e, 0);
	ehb_err_init = cur / (wsum / 256 ? wsum / 256 : 1);
	cur = lloyd(e, cur);

	/* k-means stops in the first local minimum. Rounds of: drop the free register whose loss costs
	 * least, seed it again at the colour that is served worst, k-means; keep the result if better. */
	memset(tried, 0, sizeof tried);
	for (swap = 0, guard = 0; swap < SWAP_MAX && fails < SWAP_FAILS && guard < 64; guard++) {
		static unsigned long rc_hi[32];
		unsigned long best_rc = 0xFFFFFFFFUL, n, bs = 0;
		int br = -1, bi = -1;
		unsigned char seed[6], tmp[192];
		memset(rc_hi, 0, sizeof rc_hi);
		for (i = 0; i < 256; i++) {
			if (!W[i] || d2[i] == 0x7fffffffL) continue;
			rc_hi[who[i] & 31] += wmul(d2[i] - d1[i], W[i]) >> 8;
		}
		for (r = 0; r < 32; r++) if (isfree[r] && rc_hi[r] < best_rc) { best_rc = rc_hi[r]; br = r; }
		if (br < 0) break;
		for (i = 0; i < 256; i++) {
			unsigned long x;
			if (!WS[i] || tried[i] || (who[i] & 31) == br) continue;
			x = wmul(d1[i], WS[i]);
			if (x > bs) { bs = x; bi = i; }
		}
		if (bi < 0) break;
		memcpy(tmp, e, 192);
		seed_reg(tmp, br, s + bi * 3);
		memcpy(seed, tmp + br * 3, 3); memcpy(seed + 3, tmp + (br + 32) * 3, 3);
		tried[bi] = 1;
		for (r = 0; r < 32; r++) if (r != br && !memcmp(tmp + r * 3, seed, 3)) break;
		if (r < 32) continue;                    /* this colour is as well served as the grid allows: the next */
		swap++;
		memcpy(keep, e, 192);
		memcpy(e, tmp, 192);
		n = lloyd(e, evaluate(e, 0));
		if (n >= cur) { memcpy(e, keep, 192); evaluate(e, 1); fails++; continue; }
		cur = n; fails = 0;
	}
	ehb_err_final = cur / (wsum / 256 ? wsum / 256 : 1);
}
