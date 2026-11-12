/* smk.c - Smacker (SMK2) decoder, plain C. See smk.h.
 *
 * Ported from AmiSC amiga/glue_smk.h (same author). The video path, the tree
 * builder and the lookup tables are a line-for-line translation of that file;
 * the audio path (Huffman DPCM, raw PCM) is new and follows the same public
 * description of the format:
 *
 *   chunk  = u32 unpacked size, then a bit stream (LSB first):
 *            1 bit  data present
 *            1 bit  stereo
 *            1 bit  16-bit
 *            (1 << (stereo + 16bit)) byte trees, same format as the video's
 *            byte trees: 8-bit L [R], 16-bit Llo Lhi [Rlo Rhi]
 *            first sample of each channel raw (R first, then L; 8 or 16 bits,
 *            16-bit values high byte first), then deltas for the rest.
 *
 * No floats, no allocation per frame once the buffers have grown.
 */
#include "smk.h"

#include <stdlib.h>
#include <string.h>

#ifndef SMK_TABLE_BITS
#define SMK_TABLE_BITS 10
#endif
#ifndef SMK_SUB_BITS
#define SMK_SUB_BITS 6
#endif
#ifndef SMK_TABLE_BUDGET
#define SMK_TABLE_BUDGET 8192
#endif
/* Deepest big tree accepted. AmiSC used 64, which was enough for the StarCraft
 * menu videos but rejects 2 of the 12 Warcraft II movies (their 'full' trees
 * are deeper); 500 is the limit FFmpeg uses. Recursion is that deep at most. */
#ifndef SMK_BIG_MAX_DEPTH
#define SMK_BIG_MAX_DEPTH 500
#endif

/* ------------------------------------------------------------------------- */
/* little helpers                                                            */
/* ------------------------------------------------------------------------- */
static uint32_t rd32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int i32_reserve(SmkI32 *a, uint32_t need)
{
	if (need <= a->cap) return 0;
	{
		uint32_t nc = a->cap ? a->cap : 64;
		int32_t *np;
		while (nc < need) nc *= 2;
		np = (int32_t *)realloc(a->p, nc * sizeof(int32_t));
		if (!np) return -1;
		a->p = np;
		a->cap = nc;
	}
	return 0;
}

static int i32_push(SmkI32 *a, int32_t v)
{
	if (a->n == a->cap && i32_reserve(a, a->n + 1)) return -1;
	a->p[a->n++] = v;
	return 0;
}

static int u32_resize(SmkU32 *a, uint32_t n)
{
	if (n > a->cap) {
		uint32_t nc = a->cap ? a->cap : 64;
		uint32_t *np;
		while (nc < n) nc *= 2;
		np = (uint32_t *)realloc(a->p, nc * sizeof(uint32_t));
		if (!np) return -1;
		a->p = np;
		a->cap = nc;
	}
	a->n = n;
	return 0;
}

static void tree_free(SmkTree *t)
{
	free(t->t.p);
	free(t->tab.p);
	memset(t, 0, sizeof *t);
}

/* ------------------------------------------------------------------------- */
/* bit reader: LSB first, refilled a byte at a time (endian-neutral)         */
/* ------------------------------------------------------------------------- */
typedef struct SmkBits {
	const uint8_t *p, *end;
	uint32_t acc;      /* bits not yet taken, least significant first */
	int have;          /* bits of acc filled */
	int32_t left;      /* bits of the stream not yet taken; < 0 once past its end */
} SmkBits;

static void bs_start(SmkBits *b, const uint8_t *from, const uint8_t *to)
{
	b->p = from;
	b->end = to;
	b->acc = 0;
	b->have = 0;
	b->left = to > from ? (int32_t)(to - from) * 8 : 0;
}

static void bs_fill(SmkBits *b)
{
	while (b->have <= 24) {
		uint32_t c = b->p < b->end ? *b->p++ : 0;
		b->acc |= c << b->have;
		b->have += 8;
	}
}

static uint32_t bs_peek(SmkBits *b, int n)
{
	if (b->have < n) bs_fill(b);
	return b->acc;
}

static void bs_skip(SmkBits *b, int n)
{
	b->acc >>= n;
	b->have -= n;
	b->left -= n;
}

static int bs_bit(SmkBits *b)
{
	int r;
	if (!b->have) bs_fill(b);
	r = (int)(b->acc & 1);
	b->acc >>= 1;
	--b->have;
	--b->left;
	return r;
}

static int bs_bits(SmkBits *b, int n)   /* n <= 16 */
{
	int v = (int)(bs_peek(b, n) & ((1u << n) - 1));
	bs_skip(b, n);
	return v;
}

/* ------------------------------------------------------------------------- */
/* lookup tables (see AmiSC glue_smk.h for the entry encoding)               */
/*   (leaf index << 5) | length                     leaf `length` bits down  */
/*   0x80000000 | (table << 9) | (w << 5) | length  continue in sub-table    */
/*   0x80000000 | (node << 9) | length              walk on bit by bit       */
/* ------------------------------------------------------------------------- */
typedef struct SmkPend { uint32_t pos, node, d; } SmkPend;
typedef struct SmkPendQ { SmkPend *p; uint32_t n, cap; } SmkPendQ;

static int pend_push(SmkPendQ *q, uint32_t pos, uint32_t node, uint32_t d)
{
	if (q->n == q->cap) {
		uint32_t nc = q->cap ? q->cap * 2 : 64;
		SmkPend *np = (SmkPend *)realloc(q->p, nc * sizeof(SmkPend));
		if (!np) return -1;
		q->p = np;
		q->cap = nc;
	}
	q->p[q->n].pos = pos;
	q->p[q->n].node = node;
	q->p[q->n].d = d;
	++q->n;
	return 0;
}

static int fill_from(SmkTree *tr, const int32_t *t, uint32_t base, int w,
                     uint32_t i, int d, uint32_t code, SmkPendQ *q)
{
	if (t[i] >= 0) {
		uint32_t e = (i << 5) | (uint32_t)d;
		uint32_t k;
		for (k = code; k < (1u << w); k += 1u << d) tr->tab.p[base + k] = e;
		return 0;
	}
	if (d == w) return pend_push(q, base + code, i, (uint32_t)d);
	if (fill_from(tr, t, base, w, i + 1, d + 1, code, q)) return -1;
	return fill_from(tr, t, base, w, i + 1 + (uint32_t)(t[i] & 0x7fffffff), d + 1,
	                 code | (1u << d), q);
}

static int lookup_build(SmkTree *tr)
{
	const int32_t *t = tr->t.p;
	uint32_t n = tr->t.n, i, k;
	uint16_t *depth;
	SmkPendQ q = { 0, 0, 0 };
	int rc = 0;

	if (!n) return -1;
	depth = (uint16_t *)malloc(n * sizeof(uint16_t));
	if (!depth) return -1;
	/* children always follow their node: one backward pass gives depths */
	for (i = n; i-- > 0;) {
		uint32_t l, r;
		int dl, dr;
		if (t[i] >= 0) { depth[i] = 0; continue; }
		l = i + 1;
		r = i + 1 + (uint32_t)(t[i] & 0x7fffffff);
		dl = depth[l];
		dr = depth[r];
		depth[i] = (uint16_t)(1 + (dl > dr ? dl : dr));
	}
	tr->tb = depth[0] < SMK_TABLE_BITS ? depth[0] : SMK_TABLE_BITS;
	tr->mask = (1u << tr->tb) - 1;
	if (u32_resize(&tr->tab, 1u << tr->tb)) { free(depth); return -1; }
	memset(tr->tab.p, 0, tr->tab.n * sizeof(uint32_t));
	if (fill_from(tr, t, 0, tr->tb, 0, 0, 0, &q)) { rc = -1; goto out; }
	for (k = 0; k < q.n; ++k) {          /* q grows while read: breadth first */
		SmkPend p = q.p[k];
		int sw = depth[p.node] < SMK_SUB_BITS ? depth[p.node] : SMK_SUB_BITS;
		uint32_t sub = tr->tab.n;
		if (sub + (1u << sw) <= (uint32_t)SMK_TABLE_BUDGET) {
			if (u32_resize(&tr->tab, sub + (1u << sw))) { rc = -1; goto out; }
			if (fill_from(tr, t, sub, sw, p.node, 0, 0, &q)) { rc = -1; goto out; }
			tr->tab.p[p.pos] = 0x80000000u | (sub << 9) | ((uint32_t)sw << 5) | p.d;
		} else {
			tr->tab.p[p.pos] = 0x80000000u | (p.node << 9) | p.d;
		}
	}
out:
	free(q.p);
	free(depth);
	return rc;
}

/* The index of the leaf the next code leads to. */
static uint32_t tree_find(const SmkTree *tr, SmkBits *b)
{
	const uint32_t *T = tr->tab.p;
	const int32_t *t = tr->t.p;
	uint32_t e = T[bs_peek(b, tr->tb) & tr->mask];
	while (e & 0x80000000u) {
		int w;
		uint32_t at;
		bs_skip(b, (int)(e & 31));
		w = (int)((e >> 5) & 15);
		at = (e >> 9) & 0x3fffff;
		if (!w) {
			while (t[at] < 0) {
				if (bs_bit(b)) at += (uint32_t)(t[at] & 0x7fffffff);
				++at;
			}
			return at;
		}
		e = T[at + (bs_peek(b, w) & ((1u << w) - 1))];
	}
	bs_skip(b, (int)(e & 31));
	return e >> 5;
}

/* ------------------------------------------------------------------------- */
/* byte trees                                                                */
/* ------------------------------------------------------------------------- */
static int small_build(SmkBits *b, SmkTree *tr, int depth)
{
	uint32_t node;
	if (depth > 32 || tr->t.n > 600) return -1;
	if (!bs_bit(b)) return i32_push(&tr->t, bs_bits(b, 8));
	node = tr->t.n;
	if (i32_push(&tr->t, 0)) return -1;
	if (small_build(b, tr, depth + 1)) return -1;
	tr->t.p[node] = (int32_t)(0x80000000u | (tr->t.n - node - 1));
	return small_build(b, tr, depth + 1);
}

static int small_read(SmkBits *b, SmkTree *tr)
{
	tr->t.n = 0;
	tr->present = bs_bit(b);
	if (!tr->present) return 0;
	if (small_build(b, tr, 0)) return -1;
	bs_bit(b);                           /* closing zero */
	if (b->left < 0) return -1;
	return lookup_build(tr);
}

static int small_decode(const SmkTree *tr, SmkBits *b)
{
	if (!tr->present) return 0;
	return tr->t.p[tree_find(tr, b)];
}

/* ------------------------------------------------------------------------- */
/* big trees: 16-bit values, three escape leaves = cache of recent values     */
/* ------------------------------------------------------------------------- */
static int big_build(SmkBits *b, SmkTree *tr, const SmkTree *lo, const SmkTree *hi, int depth)
{
	uint32_t node;
	if (depth > SMK_BIG_MAX_DEPTH || tr->t.n > 0x20000) return -1;
	if (!bs_bit(b)) {
		/* Two statements on purpose: in `lo(b) | (hi(b) << 8)` C leaves the
		 * order of the two reads unspecified, and gcc -O2 really does read the
		 * high byte first - which garbled 2 of the 12 W2 movies' trees. */
		int v = small_decode(lo, b);
		v |= small_decode(hi, b) << 8;
		if (v == tr->esc[0]) { tr->last[0] = (int)tr->t.n; v = 0; }
		else if (v == tr->esc[1]) { tr->last[1] = (int)tr->t.n; v = 0; }
		else if (v == tr->esc[2]) { tr->last[2] = (int)tr->t.n; v = 0; }
		return i32_push(&tr->t, v);
	}
	node = tr->t.n;
	if (i32_push(&tr->t, 0)) return -1;
	if (big_build(b, tr, lo, hi, depth + 1)) return -1;
	tr->t.p[node] = (int32_t)(0x80000000u | (tr->t.n - node - 1));
	return big_build(b, tr, lo, hi, depth + 1);
}

static int big_read(SmkBits *b, SmkTree *tr)
{
	int i, rc;
	SmkTree lo, hi;
	tr->t.n = 0;
	tr->last[0] = tr->last[1] = tr->last[2] = -1;
	tr->present = 1;
	if (!bs_bit(b)) {
		/* no tree: every lookup yields 0 */
		if (i32_push(&tr->t, 0)) return -1;
		for (i = 0; i < 3; ++i) {
			tr->last[i] = (int)tr->t.n;
			if (i32_push(&tr->t, 0)) return -1;
		}
		return lookup_build(tr);
	}
	memset(&lo, 0, sizeof lo);
	memset(&hi, 0, sizeof hi);
	rc = -1;
	if (small_read(b, &lo) || small_read(b, &hi)) goto out;
	for (i = 0; i < 3; ++i) tr->esc[i] = bs_bits(b, 16);
	if (big_build(b, tr, &lo, &hi, 0)) goto out;
	bs_bit(b);                           /* closing zero */
	for (i = 0; i < 3; ++i)
		if (tr->last[i] < 0) {
			tr->last[i] = (int)tr->t.n;
			if (i32_push(&tr->t, 0)) goto out;
		}
	rc = lookup_build(tr);
out:
	tree_free(&lo);
	tree_free(&hi);
	return rc;
}

static void big_reset(SmkTree *tr)
{
	int32_t *T = tr->t.p;
	T[tr->last[0]] = 0;
	T[tr->last[1]] = 0;
	T[tr->last[2]] = 0;
}

static int big_get(SmkTree *tr, SmkBits *b)
{
	int32_t *T = tr->t.p;
	int32_t v = T[tree_find(tr, b)];
	int32_t *c0 = T + tr->last[0];
	if (v != *c0) {
		int32_t *c1 = T + tr->last[1];
		int32_t *c2 = T + tr->last[2];
		*c2 = *c1;
		*c1 = *c0;
		*c0 = v;
	}
	return (int)v;
}

/* ------------------------------------------------------------------------- */
/* open / close                                                              */
/* ------------------------------------------------------------------------- */
void smk_close(SmkVideo *v)
{
	int i;
	if (!v) return;
	free(v->frame_size);
	free(v->frame_type);
	free(v->frame_at);
	free(v->pixels);
	tree_free(&v->mmap);
	tree_free(&v->mclr);
	tree_free(&v->full);
	tree_free(&v->type);
	for (i = 0; i < 4; ++i) tree_free(&v->atree[i]);
	for (i = 0; i < SMK_MAX_TRACKS; ++i) free(v->track[i].buf);
	memset(v, 0, sizeof *v);
	v->cur = -1;
}

static int track_reserve(SmkTrack *t, uint32_t need)
{
	uint8_t *np;
	if (need <= t->cap) return 0;
	np = (uint8_t *)realloc(t->buf, need);
	if (!np) return -1;
	t->buf = np;
	t->cap = need;
	return 0;
}

int smk_open(SmkVideo *v, const uint8_t *d, uint32_t n)
{
	int32_t rate;
	uint32_t trees, at;
	int i, total;
	SmkBits b;

	memset(v, 0, sizeof *v);
	v->cur = -1;
	if (!d || n < 104 || memcmp(d, "SMK2", 4) != 0) return -1;
	v->data = d;
	v->size = n;
	v->w = (int)rd32(d + 4);
	v->h = (int)rd32(d + 8);
	v->frames = (int)rd32(d + 12);
	rate = (int32_t)rd32(d + 16);
	v->frame_us = rate > 0 ? (uint32_t)rate * 1000u : rate < 0 ? (uint32_t)(-rate) * 10u : 100000u;
	if (!v->frame_us) v->frame_us = 100000u;
	v->flags = rd32(d + 20);
	trees = rd32(d + 52);
	if (v->w <= 0 || v->h <= 0 || v->w > 2048 || v->h > 2048 || (v->w & 3) || (v->h & 3)
	    || v->frames <= 0 || v->frames > 100000)
		goto bad;
	total = v->frames + ((v->flags & SMK_FLAG_RING) ? 1 : 0);
	v->total = total;
	at = 104;
	if ((uint32_t)total * 5u + trees > n - at) goto bad;

	for (i = 0; i < SMK_MAX_TRACKS; ++i) {
		uint32_t r = rd32(d + 72 + 4 * i);
		SmkTrack *t = &v->track[i];
		t->present = (r >> 30) & 1;
		t->compressed = (r >> 31) & 1;
		t->bits = (r & (1u << 29)) ? 16 : 8;
		t->channels = (r & (1u << 28)) ? 2 : 1;
		t->rate = r & 0xFFFFFFu;
		if (t->present && ((r >> 26) & 3)) t->present = 0;   /* Bink audio: not SMK2 */
		if (t->present && track_reserve(t, rd32(d + 24 + 4 * i) + 16)) goto bad;
	}

	v->frame_size = (uint32_t *)malloc((size_t)total * sizeof(uint32_t));
	v->frame_type = (uint8_t *)malloc((size_t)total);
	v->frame_at = (uint32_t *)malloc((size_t)total * sizeof(uint32_t));
	v->pixels = (uint8_t *)calloc((size_t)v->w * v->h, 1);
	if (!v->frame_size || !v->frame_type || !v->frame_at || !v->pixels) goto bad;
	for (i = 0; i < total; ++i) v->frame_size[i] = rd32(d + at + 4 * i);
	at += 4u * (uint32_t)total;
	for (i = 0; i < total; ++i) v->frame_type[i] = d[at + i];
	at += (uint32_t)total;

	bs_start(&b, d + at, d + at + trees);
	if (big_read(&b, &v->mmap) || big_read(&b, &v->mclr)
	    || big_read(&b, &v->full) || big_read(&b, &v->type))
		goto bad;
	at += trees;
	for (i = 0; i < total; ++i) {
		v->frame_at[i] = at;
		if ((v->frame_size[i] & ~3u) > n - at) goto bad;
		at += v->frame_size[i] & ~3u;
	}
	v->ok = 1;
	return 0;
bad:
	smk_close(v);
	return -1;
}

/* ------------------------------------------------------------------------- */
/* palette                                                                   */
/* ------------------------------------------------------------------------- */
static void read_palette(SmkVideo *v, const uint8_t *p, const uint8_t *end)
{
	static const uint8_t level[64] = {
		0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
		0x41, 0x45, 0x49, 0x4D, 0x51, 0x55, 0x59, 0x5D, 0x61, 0x65, 0x69, 0x6D, 0x71, 0x75, 0x79, 0x7D,
		0x82, 0x86, 0x8A, 0x8E, 0x92, 0x96, 0x9A, 0x9E, 0xA2, 0xA6, 0xAA, 0xAE, 0xB2, 0xB6, 0xBA, 0xBE,
		0xC3, 0xC7, 0xCB, 0xCF, 0xD3, 0xD7, 0xDB, 0xDF, 0xE3, 0xE7, 0xEB, 0xEF, 0xF3, 0xF7, 0xFB, 0xFF };
	uint8_t old[768];
	uint8_t *pal = v->palette;
	int i = 0;
	memcpy(old, pal, 768);
	while (i < 256 && p < end) {
		int t = *p++;
		if (t & 0x80) {
			int k = (t & 0x7f) + 1;
			for (; k && i < 256; --k, ++i) memcpy(pal + 3 * i, old + 3 * i, 3);
		} else if (t & 0x40) {
			int k = (t & 0x3f) + 1, from;
			if (p >= end) break;
			from = *p++;
			for (; k && i < 256 && from < 256; --k, ++i, ++from) memcpy(pal + 3 * i, old + 3 * from, 3);
		} else {
			if (end - p < 2) break;
			pal[3 * i + 0] = level[t & 0x3f];
			pal[3 * i + 1] = level[p[0] & 0x3f];
			pal[3 * i + 2] = level[p[1] & 0x3f];
			p += 2;
			++i;
		}
	}
	v->palette_changed = 1;
}

/* ------------------------------------------------------------------------- */
/* audio                                                                     */
/* ------------------------------------------------------------------------- */
static int audio_chunk(SmkVideo *v, SmkTrack *tr, const uint8_t *p, uint32_t n)
{
	SmkBits b;
	uint32_t unp, count, i;
	int stereo, is16, ch, k;
	int pred[2];

	tr->len = 0;
	if (!tr->compressed) {
		/* raw PCM: 8-bit unsigned as is, 16-bit little-endian -> host order */
		if (track_reserve(tr, n)) return -1;
		if (tr->bits == 16) {
			int16_t *o = (int16_t *)tr->buf;
			for (i = 0; i + 1 < n; i += 2) *o++ = (int16_t)(uint16_t)(p[i] | (p[i + 1] << 8));
			tr->len = n & ~1u;
		} else {
			memcpy(tr->buf, p, n);
			tr->len = n;
		}
		tr->total += tr->len;
		return 0;
	}
	if (n < 4) return -1;
	unp = rd32(p);
	p += 4;
	n -= 4;
	if (unp > 0x1000000u) return -1;
	bs_start(&b, p, p + n);
	if (!bs_bit(&b)) return 0;           /* chunk carries no data */
	stereo = bs_bit(&b);
	is16 = bs_bit(&b);
	if (stereo != (tr->channels == 2) || is16 != (tr->bits == 16)) return -1;
	ch = stereo + 1;
	for (k = 0; k < (1 << (stereo + is16)); ++k)
		if (small_read(&b, &v->atree[k])) return -1;
	count = unp / (uint32_t)(ch * (is16 + 1));   /* sample frames */
	count *= (uint32_t)ch;                       /* sample values */
	if (count < (uint32_t)ch) return 0;
	if (track_reserve(tr, count * (uint32_t)(is16 + 1))) return -1;

	if (is16) {
		int16_t *o = (int16_t *)tr->buf;
		for (k = stereo; k >= 0; --k) {
			int hi = bs_bits(&b, 8);         /* high byte first */
			int lo = bs_bits(&b, 8);
			pred[k] = (int16_t)(uint16_t)((hi << 8) | lo);
		}
		for (k = 0; k < ch; ++k) o[k] = (int16_t)pred[k];
		for (i = (uint32_t)ch; i < count; ++i) {
			int c = (int)(i & (uint32_t)stereo);
			int lo = small_decode(&v->atree[c * 2], &b);
			int hi = small_decode(&v->atree[c * 2 + 1], &b);
			pred[c] = (int16_t)(uint16_t)(pred[c] + (int16_t)(uint16_t)(lo | (hi << 8)));
			o[i] = (int16_t)pred[c];
		}
		tr->len = count * 2;
	} else {
		uint8_t *o = tr->buf;
		for (k = stereo; k >= 0; --k) pred[k] = bs_bits(&b, 8);
		for (k = 0; k < ch; ++k) o[k] = (uint8_t)pred[k];
		for (i = (uint32_t)ch; i < count; ++i) {
			int c = (int)(i & (uint32_t)stereo);
			pred[c] = (pred[c] + small_decode(&v->atree[c], &b)) & 0xFF;
			o[i] = (uint8_t)pred[c];
		}
		tr->len = count;
	}
	tr->total += tr->len;
	return b.left < 0 ? -1 : 0;
}

const uint8_t *smk_audio(const SmkVideo *v, int t, uint32_t *bytes)
{
	if (t < 0 || t >= SMK_MAX_TRACKS || !v->track[t].len) {
		if (bytes) *bytes = 0;
		return 0;
	}
	if (bytes) *bytes = v->track[t].len;
	return v->track[t].buf;
}

/* ------------------------------------------------------------------------- */
/* frames                                                                    */
/* ------------------------------------------------------------------------- */
static void decode_video(SmkVideo *v, const uint8_t *p, const uint8_t *end)
{
	static const int runs[64] = {
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
		49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 128, 256, 512, 1024, 2048 };
	const int bw = v->w / 4, blocks = bw * (v->h / 4), stride = v->w, rowstep = 4 * v->w;
	uint8_t *const pix = v->pixels;
	int blk = 0, col = 0;
	uint8_t *row = pix;
	SmkBits b;

	big_reset(&v->mmap);
	big_reset(&v->mclr);
	big_reset(&v->full);
	big_reset(&v->type);
	bs_start(&b, p, end);
	while (blk < blocks && b.left >= 0) {
		int t = big_get(&v->type, &b);
		int run = runs[(t >> 2) & 0x3f];
		switch (t & 3) {
		case 0:   /* two colours and a 16-bit mask */
			while (run-- && blk < blocks) {
				int clr = big_get(&v->mclr, &b), map = big_get(&v->mmap, &b);
				uint8_t hi = (uint8_t)(clr >> 8), lo = (uint8_t)clr;
				uint8_t *o = row + col * 4;
				int r;
				for (r = 0; r < 4; ++r, o += stride, map >>= 4) {
					o[0] = (map & 1) ? hi : lo;
					o[1] = (map & 2) ? hi : lo;
					o[2] = (map & 4) ? hi : lo;
					o[3] = (map & 8) ? hi : lo;
				}
				++blk;
				if (++col == bw) { col = 0; row += rowstep; }
			}
			break;
		case 1:   /* every pixel given, two per code */
			while (run-- && blk < blocks) {
				uint8_t *o = row + col * 4;
				int r;
				for (r = 0; r < 4; ++r, o += stride) {
					int px = big_get(&v->full, &b);
					o[2] = (uint8_t)px; o[3] = (uint8_t)(px >> 8);
					px = big_get(&v->full, &b);
					o[0] = (uint8_t)px; o[1] = (uint8_t)(px >> 8);
				}
				++blk;
				if (++col == bw) { col = 0; row += rowstep; }
			}
			break;
		case 2:   /* unchanged */
			blk += run;
			if (blk < blocks) {
				col = blk % bw;
				row = pix + (blk / bw) * rowstep;
			}
			break;
		default: { /* 3: one colour */
			uint8_t c = (uint8_t)(t >> 8);
			while (run-- && blk < blocks) {
				uint8_t *o = row + col * 4;
				int r;
				for (r = 0; r < 4; ++r, o += stride) o[0] = o[1] = o[2] = o[3] = c;
				++blk;
				if (++col == bw) { col = 0; row += rowstep; }
			}
			break;
		}
		}
	}
}

int smk_decode(SmkVideo *v, int f, int what)
{
	const uint8_t *p, *end;
	int t;
	if (!v->ok || f < 0 || f >= v->total) return -1;
	p = v->data + v->frame_at[f];
	end = p + (v->frame_size[f] & ~3u);
	v->palette_changed = 0;
	for (t = 0; t < SMK_MAX_TRACKS; ++t) v->track[t].len = 0;
	if (v->frame_type[f] & 1) {
		uint32_t len;
		if (p >= end) return -1;
		len = (uint32_t)*p * 4u;
		if (len == 0 || len > (uint32_t)(end - p)) return -1;
		read_palette(v, p + 1, p + len);
		p += len;
	}
	for (t = 0; t < SMK_MAX_TRACKS; ++t) {
		uint32_t len;
		if (!(v->frame_type[f] & (2 << t))) continue;
		if (end - p < 4) return -1;
		len = rd32(p);
		if (len < 4 || len > (uint32_t)(end - p)) return -1;
		if ((what & SMK_DECODE_AUDIO) && v->track[t].present)
			if (audio_chunk(v, &v->track[t], p + 4, len - 4)) ++v->errors;
		p += len;
	}
	if (what & SMK_DECODE_VIDEO) decode_video(v, p, end);
	v->cur = f;
	return 0;
}

int smk_next_frame(SmkVideo *v, int what)
{
	int f = v->cur + 1;
	if (f >= v->total) f = (v->flags & SMK_FLAG_RING) ? 1 : 0;
	if (f == 0 && v->pixels) memset(v->pixels, 0, (size_t)v->w * v->h);
	return smk_decode(v, f, what);
}
