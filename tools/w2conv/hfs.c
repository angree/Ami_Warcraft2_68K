/* hfs.c - read-only Apple Partition Map + HFS reader for w2conv (see hfs.h). Our own code. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "hfs.h"

static uint32_t be16(const unsigned char *p) { return (uint32_t)p[0] << 8 | p[1]; }
static uint32_t be32(const unsigned char *p) { return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3]; }

static int is_hfs_at(Hfs *h, int64_t off)
{
	unsigned char m[2];
	if (off < 0 || h->rd(h->ud, off + 1024, m, 2)) return 0;
	return m[0] == 'B' && m[1] == 'D';
}

static int find_partition(Hfs *h, int64_t total)
{
	unsigned char b[512];
	int stride_i, k;
	static const int strides[2] = { 512, 2048 };
	if (h->rd(h->ud, 0, b, 512) == 0 && b[0] == 'E' && b[1] == 'R') {
		for (stride_i = 0; stride_i < 2; stride_i++) {
			int stride = strides[stride_i], count = 1;
			for (k = 1; k <= count && k < 64; k++) {
				uint32_t start, units[2];
				int u;
				if ((int64_t)k * stride + 512 > total) break;
				if (h->rd(h->ud, (int64_t)k * stride, b, 512) || b[0] != 'P' || b[1] != 'M') break;
				count = (int)be32(b + 4);
				if (strncmp((const char *)b + 48, "Apple_HFS", 9) != 0) continue;
				start = be32(b + 8);
				units[0] = 512; units[1] = (uint32_t)stride;
				for (u = 0; u < 2; u++)
					if (is_hfs_at(h, (int64_t)start * units[u])) { h->part = (int64_t)start * units[u]; return 0; }
			}
		}
	}
	if (is_hfs_at(h, 0)) { h->part = 0; return 0; }
	return -1;
}

/* read a fork given up to 3 extents into a malloc'd buffer of len bytes */
static unsigned char *read_fork(Hfs *h, const unsigned char *extrec, uint32_t len)
{
	unsigned char *buf = (unsigned char *)malloc(len ? len : 1);
	uint32_t done = 0;
	int i;
	if (!buf) return NULL;
	for (i = 0; i < 3 && done < len; i++) {
		uint32_t st = be16(extrec + i * 4), n = be16(extrec + i * 4 + 2);
		uint64_t bytes = (uint64_t)n * h->blk_size;
		if (bytes > len - done) bytes = len - done;
		if (h->rd(h->ud, h->blk0 + (int64_t)st * h->blk_size, buf + done, (size_t)bytes)) { free(buf); return NULL; }
		done += (uint32_t)bytes;
	}
	if (done < len) { fprintf(stderr, "w2conv: HFS: system file fragmented beyond 3 extents\n"); free(buf); return NULL; }
	return buf;
}

typedef void (*LeafFn)(Hfs *h, const unsigned char *key, const unsigned char *rec, const unsigned char *end);

static int walk_leaves(Hfs *h, const unsigned char *tree, uint32_t tlen, LeafFn fn)
{
	uint32_t nsize, node, guard = 0;
	if (tlen < 512) return -1;
	nsize = be16(tree + 14 + 18);
	node = be32(tree + 14 + 10);            /* first leaf */
	if (nsize < 512 || nsize > 32768) return -1;
	while (node && (uint64_t)(node + 1) * nsize <= tlen && guard++ < 1000000) {
		const unsigned char *n = tree + (size_t)node * nsize;
		int nrec = (int)be16(n + 10), r;
		if ((signed char)n[8] != -1) break;  /* not a leaf */
		for (r = 0; r < nrec; r++) {
			uint32_t o = be16(n + nsize - 2 - 2 * r), o2 = be16(n + nsize - 4 - 2 * r);
			const unsigned char *key = n + o;
			uint32_t klen = key[0];
			uint32_t ro = o + 1 + klen;
			if (o >= nsize || o2 > nsize) break;
			if (ro & 1) ro++;
			if (ro >= nsize) continue;
			fn(h, key, n + ro, n + (o2 > ro ? o2 : nsize));
		}
		node = be32(n);                      /* fLink */
	}
	return 0;
}

static void copy_name(char *dst, const unsigned char *pstr)
{
	int l = pstr[0] > 31 ? 31 : pstr[0];
	memcpy(dst, pstr + 1, (size_t)l);
	dst[l] = 0;
}

static void cat_leaf(Hfs *h, const unsigned char *key, const unsigned char *rec, const unsigned char *end)
{
	uint32_t parent = be32(key + 2);
	const unsigned char *name = key + 6;
	(void)end;
	if (rec[0] == 1) {               /* directory */
		HfsDir *d;
		HfsDir *nd = (HfsDir *)realloc(h->dirs, sizeof(HfsDir) * (size_t)(h->ndirs + 1));
		if (!nd) return;
		h->dirs = nd; d = &h->dirs[h->ndirs++];
		d->id = be32(rec + 6); d->parent = parent;
		copy_name(d->name, name);
	} else if (rec[0] == 2) {        /* file */
		HfsFile *f;
		HfsFile *nf = (HfsFile *)realloc(h->files, sizeof(HfsFile) * (size_t)(h->nfiles + 1));
		int i;
		if (!nf) return;
		h->files = nf; f = &h->files[h->nfiles++];
		memset(f, 0, sizeof *f);
		f->parent = parent;
		copy_name(f->name, name);
		memcpy(f->type, rec + 4, 4); memcpy(f->creator, rec + 8, 4);
		f->id = be32(rec + 20);
		f->data_len = be32(rec + 26);
		f->rsrc_len = be32(rec + 36);
		for (i = 0; i < 3; i++) { f->ext[i].start = be16(rec + 74 + i * 4); f->ext[i].count = be16(rec + 76 + i * 4); }
	}
}

int hfs_open(Hfs *h, HfsReadFn rd, void *ud, int64_t total)
{
	unsigned char m[512];
	memset(h, 0, sizeof *h);
	h->rd = rd; h->ud = ud;
	if (find_partition(h, total)) return -1;
	if (rd(ud, h->part + 1024, m, 512)) return -1;
	h->blk_size = be32(m + 20);
	h->blk0 = h->part + (int64_t)be16(m + 28) * 512;
	copy_name(h->volname, m + 36);
	if (!h->blk_size || (h->blk_size & 511)) return -1;
	h->ext_len = be32(m + 130);
	h->cat_len = be32(m + 146);
	h->ext = read_fork(h, m + 134, h->ext_len);
	h->cat = read_fork(h, m + 150, h->cat_len);
	if (!h->cat) { hfs_close(h); return -1; }
	if (walk_leaves(h, h->cat, h->cat_len, cat_leaf)) { hfs_close(h); return -1; }
	return 0;
}

void hfs_close(Hfs *h)
{
	free(h->cat); free(h->ext); free(h->files); free(h->dirs);
	h->cat = h->ext = NULL; h->files = NULL; h->dirs = NULL; h->nfiles = h->ndirs = 0;
}

static int name_eq(const char *a, const char *b, size_t blen)
{
	size_t i;
	for (i = 0; i < blen; i++) {
		if (!a[i] || toupper((unsigned char)a[i]) != toupper((unsigned char)b[i])) return 0;
	}
	return a[blen] == 0;
}

const HfsFile *hfs_find(const Hfs *h, const char *path)
{
	uint32_t dir = 2;   /* root folder id */
	const char *p = path;
	for (;;) {
		const char *slash = strchr(p, '/');
		size_t n = slash ? (size_t)(slash - p) : strlen(p);
		int i, found = 0;
		if (slash) {
			for (i = 0; i < h->ndirs; i++)
				if (h->dirs[i].parent == dir && name_eq(h->dirs[i].name, p, n)) { dir = h->dirs[i].id; found = 1; break; }
			if (!found) return NULL;
			p = slash + 1;
		} else {
			for (i = 0; i < h->nfiles; i++)
				if (h->files[i].parent == dir && name_eq(h->files[i].name, p, n)) return &h->files[i];
			return NULL;
		}
	}
}

void hfs_path(const Hfs *h, const HfsFile *f, char *buf, size_t cap)
{
	char tmp[512];
	uint32_t dir = f->parent;
	int guard = 0;
	snprintf(buf, cap, "%s", f->name);
	while (dir != 2 && dir > 1 && guard++ < 64) {
		int i, found = -1;
		for (i = 0; i < h->ndirs; i++) if (h->dirs[i].id == dir) { found = i; break; }
		if (found < 0) break;
		snprintf(tmp, sizeof tmp, "%s/%s", h->dirs[found].name, buf);
		snprintf(buf, cap, "%s", tmp);
		dir = h->dirs[found].parent;
	}
}

/* data-fork extents from the overflow tree, appended to list (sorted by start in fork) */
typedef struct { uint32_t id; uint32_t fork_start[64]; HfsExt e[64 * 3]; int n; } OvfQuery;
static OvfQuery g_q;

static void ext_leaf(Hfs *h, const unsigned char *key, const unsigned char *rec, const unsigned char *end)
{
	int i;
	(void)h; (void)end;
	if (key[0] < 7 || key[1] != 0 || be32(key + 2) != g_q.id || g_q.n >= 64) return;
	g_q.fork_start[g_q.n] = be16(key + 6);
	for (i = 0; i < 3; i++) { g_q.e[g_q.n * 3 + i].start = be16(rec + i * 4); g_q.e[g_q.n * 3 + i].count = be16(rec + i * 4 + 2); }
	g_q.n++;
}

long hfs_extract(const Hfs *h, const HfsFile *f, FILE *out)
{
	static unsigned char buf[1 << 16];
	uint32_t left = f->data_len, fblk = 0;
	long written = 0;
	int i, pass;
	g_q.n = 0;
	for (pass = 0; pass < 2 && left; pass++) {
		const HfsExt *ex; int nex;
		if (pass == 0) { ex = f->ext; nex = 3; }
		else {
			int j, k;
			if (!h->ext) return -1;
			g_q.id = f->id; g_q.n = 0;
			walk_leaves((Hfs *)h, h->ext, h->ext_len, ext_leaf);
			/* order records by their first fork block */
			for (j = 0; j < g_q.n; j++) for (k = j + 1; k < g_q.n; k++)
				if (g_q.fork_start[k] < g_q.fork_start[j]) {
					uint32_t t = g_q.fork_start[j]; HfsExt te[3];
					g_q.fork_start[j] = g_q.fork_start[k]; g_q.fork_start[k] = t;
					memcpy(te, &g_q.e[j * 3], sizeof te); memcpy(&g_q.e[j * 3], &g_q.e[k * 3], sizeof te); memcpy(&g_q.e[k * 3], te, sizeof te);
				}
			if (g_q.n && g_q.fork_start[0] != fblk) { fprintf(stderr, "w2conv: HFS: extents of %s do not continue\n", f->name); return -1; }
			ex = g_q.e; nex = g_q.n * 3;
		}
		for (i = 0; i < nex && left; i++) {
			uint64_t bytes = (uint64_t)ex[i].count * h->blk_size;
			int64_t off = h->blk0 + (int64_t)ex[i].start * h->blk_size;
			if (!ex[i].count) continue;
			if (bytes > left) bytes = left;
			fblk += ex[i].count;
			while (bytes) {
				size_t n = bytes > sizeof buf ? sizeof buf : (size_t)bytes;
				if (h->rd(h->ud, off, buf, n)) return -1;
				if (out && fwrite(buf, 1, n, out) != n) return -1;
				off += (int64_t)n; bytes -= n; left -= (uint32_t)n; written += (long)n;
			}
		}
	}
	return left ? -1 : written;
}
