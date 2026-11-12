/* adpcm_enc.c - polyphase windowed-sinc resampler + IMA ADPCM encoder (see adpcm_enc.h).
 * PC-side tool: floating point is fine here (it never runs on the Amiga). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "adpcm_enc.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static long gcd(long a, long b) { while (b) { long t = a % b; a = b; b = t; } return a; }

int16_t *adpe_resample(const int16_t *in, long n, long in_rate, long out_rate, long *out_n)
{
	long g = gcd(in_rate, out_rate), L = out_rate / g, M = in_rate / g;
	const int K = 48;                 /* taps per phase */
	long N, D, i, outn;
	double fc, sum;
	float *h;
	int16_t *out;
	*out_n = 0;
	if (L > 512) { fprintf(stderr, "w2conv: resample %ld -> %ld Hz not supported\n", in_rate, out_rate); return NULL; }
	outn = (long)((double)n * out_rate / in_rate);
	out = (int16_t *)malloc(sizeof(int16_t) * (size_t)(outn ? outn : 1));
	if (!out) return NULL;
	if (L == 1 && M == 1) { memcpy(out, in, sizeof(int16_t) * (size_t)n); *out_n = n; return out; }
	N = (long)K * L;
	h = (float *)malloc(sizeof(float) * (size_t)N);
	if (!h) { free(out); return NULL; }
	/* cutoff: 90% of the lower Nyquist, relative to the upsampled rate in_rate*L */
	fc = 0.45 * (double)(in_rate < out_rate ? in_rate : out_rate) / ((double)in_rate * L);
	sum = 0;
	for (i = 0; i < N; i++) {
		double x = (double)i - (double)(N - 1) / 2.0;
		double s = x == 0 ? 2 * fc : sin(2 * M_PI * fc * x) / (M_PI * x);
		double w = 0.42 - 0.5 * cos(2 * M_PI * i / (N - 1)) + 0.08 * cos(4 * M_PI * i / (N - 1));   /* Blackman */
		h[i] = (float)(s * w);
		sum += s * w;
	}
	for (i = 0; i < N; i++) h[i] = (float)(h[i] * L / sum);
	D = (N - 1) / 2;
	for (i = 0; i < outn; i++) {
		long m = i * M + D, p = m % L, base = m / L, j;
		double acc = 0;
		for (j = 0; j < K; j++) {
			long xi = base - j;
			if (xi < 0) break;
			if (xi < n) acc += h[p + j * L] * in[xi];
		}
		if (acc > 32767) acc = 32767;
		if (acc < -32768) acc = -32768;
		out[i] = (int16_t)lrint(acc);
	}
	free(h);
	*out_n = outn;
	return out;
}

static const int ima_index[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
static const int ima_step[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37,
	41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173,
	190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
	724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
	7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
	18500, 20350, 22385, 24623, 27086, 29794, 32767
};

/* encode one sample; updates pred and index exactly like the decoder in native/amiga_adpcm.c */
static int enc_sample(int x, int *pred, int *index)
{
	int step = ima_step[*index], diff = x - *pred, nib = 0, d;
	if (diff < 0) { nib = 8; diff = -diff; }
	if (diff >= step) { nib |= 4; diff -= step; }
	if (diff >= step >> 1) { nib |= 2; diff -= step >> 1; }
	if (diff >= step >> 2) { nib |= 1; }
	/* reconstruct with the decoder's formula */
	d = step >> 3;
	if (nib & 1) d += step >> 2;
	if (nib & 2) d += step >> 1;
	if (nib & 4) d += step;
	if (nib & 8) *pred -= d; else *pred += d;
	if (*pred > 32767) *pred = 32767;
	if (*pred < -32768) *pred = -32768;
	*index += ima_index[nib];
	if (*index < 0) *index = 0;
	if (*index > 88) *index = 88;
	return nib;
}

static void p16(unsigned char *p, unsigned v) { p[0] = (unsigned char)v; p[1] = (unsigned char)(v >> 8); }
static void p32(unsigned char *p, unsigned long v) { p16(p, (unsigned)(v & 0xFFFF)); p16(p + 2, (unsigned)(v >> 16)); }

long adpe_write_wav(const char *path, const int16_t *s, long n, long rate)
{
	FILE *f = fopen(path, "wb");
	unsigned char h[60], blk[ADPE_BLOCK_ALIGN];
	long nblocks = (n + ADPE_SPB - 1) / ADPE_SPB, b, total;
	int pred = 0, index = 0;
	if (!f) { fprintf(stderr, "w2conv: cannot write %s\n", path); return -1; }
	if (nblocks < 1) nblocks = 1;
	total = 60 + nblocks * ADPE_BLOCK_ALIGN;
	memcpy(h, "RIFF", 4); p32(h + 4, (unsigned long)(total - 8)); memcpy(h + 8, "WAVE", 4);
	memcpy(h + 12, "fmt ", 4); p32(h + 16, 20);
	p16(h + 20, 0x11); p16(h + 22, 1); p32(h + 24, (unsigned long)rate);
	p32(h + 28, (unsigned long)(rate * ADPE_BLOCK_ALIGN / ADPE_SPB)); p16(h + 32, ADPE_BLOCK_ALIGN); p16(h + 34, 4);
	p16(h + 36, 2); p16(h + 38, ADPE_SPB);
	memcpy(h + 40, "fact", 4); p32(h + 44, 4); p32(h + 48, (unsigned long)n);
	memcpy(h + 52, "data", 4); p32(h + 56, (unsigned long)(nblocks * ADPE_BLOCK_ALIGN));
	fwrite(h, 1, 60, f);
	for (b = 0; b < nblocks; b++) {
		long at = b * ADPE_SPB, i;
		int last = n ? s[n - 1] : 0;
		int x0 = at < n ? s[at] : last;
		pred = x0;
		p16(blk, (unsigned)(x0 & 0xFFFF)); blk[2] = (unsigned char)index; blk[3] = 0;
		for (i = 0; i < ADPE_SPB - 1; i += 2) {
			long k = at + 1 + i;
			int lo = enc_sample(k < n ? s[k] : last, &pred, &index);
			int hi = enc_sample(k + 1 < n ? s[k + 1] : last, &pred, &index);
			blk[4 + i / 2] = (unsigned char)(lo | hi << 4);
		}
		if (fwrite(blk, 1, ADPE_BLOCK_ALIGN, f) != ADPE_BLOCK_ALIGN) { fclose(f); return -1; }
	}
	if (fclose(f)) return -1;
	return total;
}

void adpe_stats(const int16_t *s, long n, int *peak, int *rms)
{
	long i;
	double e = 0;
	int pk = 0;
	for (i = 0; i < n; i++) {
		int v = s[i] < 0 ? -s[i] : s[i];
		if (v > pk) pk = v;
		e += (double)s[i] * s[i];
	}
	*peak = pk;
	*rms = n ? (int)sqrt(e / n) : 0;
}
