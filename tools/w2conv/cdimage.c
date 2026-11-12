/* cdimage.c - CUE/BIN/ISO reader for w2conv (see cdimage.h). Our own code. */
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "cdimage.h"

int w2c_seek(FILE *f, int64_t off)
{
#ifdef _WIN32
	return _fseeki64(f, off, SEEK_SET);
#else
	return fseeko(f, (off_t)off, SEEK_SET);
#endif
}

int64_t w2c_fsize(FILE *f)
{
	int64_t n;
#ifdef _WIN32
	if (_fseeki64(f, 0, SEEK_END)) return -1;
	n = _ftelli64(f);
#else
	if (fseeko(f, 0, SEEK_END)) return -1;
	n = (int64_t)ftello(f);
#endif
	w2c_seek(f, 0);
	return n;
}

static int ieq_prefix(const char *s, const char *p)
{
	while (*p) { if (toupper((unsigned char)*s) != toupper((unsigned char)*p)) return 0; s++; p++; }
	return 1;
}

static const char *skipws(const char *s) { while (*s == ' ' || *s == '\t') s++; return s; }

static int ends_with_ci(const char *s, const char *suf)
{
	size_t a = strlen(s), b = strlen(suf);
	return a >= b && ieq_prefix(s + a - b, suf);
}

/* "mm:ss:ff" -> frames */
static int64_t msf(const char *s)
{
	int m = 0, sec = 0, fr = 0;
	if (sscanf(s, "%d:%d:%d", &m, &sec, &fr) != 3) return -1;
	return ((int64_t)m * 60 + sec) * 75 + fr;
}

static FILE *open_rel(const char *cuepath, const char *name, char *full, size_t cap)
{
	const char *slash = strrchr(cuepath, '/');
	const char *bs = strrchr(cuepath, '\\');
	FILE *f;
	size_t dl;
	if (bs && (!slash || bs > slash)) slash = bs;
	dl = slash ? (size_t)(slash - cuepath + 1) : 0;
	if (dl + strlen(name) + 1 > cap) return NULL;
	memcpy(full, cuepath, dl);
	strcpy(full + dl, name);
	f = fopen(full, "rb");
	if (!f) {
		/* case-insensitive retry: try upper and lower case variants of the file name only */
		char *p;
		for (p = full + dl; *p; p++) *p = (char)toupper((unsigned char)*p);
		f = fopen(full, "rb");
		if (!f) { for (p = full + dl; *p; p++) *p = (char)tolower((unsigned char)*p); f = fopen(full, "rb"); }
	}
	return f;
}

static int parse_cue(CdImage *cd, const char *path)
{
	FILE *c = fopen(path, "rb");
	char line[1024], full[2048];
	int cur_file = -1, big = 0;
	CdTrack *t = NULL;
	if (!c) { fprintf(stderr, "w2conv: cannot open %s\n", path); return -1; }
	while (fgets(line, sizeof line, c)) {
		const char *s = skipws(line);
		size_t n = strlen(line);
		while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
		if (ieq_prefix(s, "FILE")) {
			char name[1024];
			const char *q = skipws(s + 4), *e;
			if (*q == '"') { q++; e = strchr(q, '"'); }
			else { e = q; while (*e && *e != ' ' && *e != '\t') e++; }
			if (!e || (size_t)(e - q) >= sizeof name) continue;
			memcpy(name, q, (size_t)(e - q)); name[e - q] = 0;
			if (cd->nfiles >= CD_MAX_FILES) break;
			cd->f[cd->nfiles] = open_rel(path, name, full, sizeof full);
			if (!cd->f[cd->nfiles]) { fprintf(stderr, "w2conv: %s: cannot open track file \"%s\"\n", path, name); fclose(c); return -1; }
			cd->fsize[cd->nfiles] = w2c_fsize(cd->f[cd->nfiles]);
			big = strstr(e, "MOTOROLA") != NULL;
			cur_file = cd->nfiles++;
		} else if (ieq_prefix(s, "TRACK")) {
			int num = 0;
			char type[64] = "";
			if (cur_file < 0 || cd->ntracks >= CD_MAX_TRACKS) continue;
			sscanf(s + 5, "%d %63s", &num, type);
			t = &cd->t[cd->ntracks++];
			memset(t, 0, sizeof *t);
			t->number = num; t->file = cur_file; t->start = -1; t->index0 = -1; t->big_endian = big;
			if (ieq_prefix(type, "AUDIO")) { t->audio = 1; t->sector_size = 2352; t->data_off = 0; }
			else if (ieq_prefix(type, "MODE1/2048")) { t->sector_size = 2048; t->data_off = 0; }
			else if (ieq_prefix(type, "MODE1/2352")) { t->sector_size = 2352; t->data_off = 16; }
			else if (ieq_prefix(type, "MODE2/2352")) { t->sector_size = 2352; t->data_off = 24; }
			else if (ieq_prefix(type, "MODE2/2336")) { t->sector_size = 2336; t->data_off = 8; }
			else { fprintf(stderr, "w2conv: track %d: unsupported type %s\n", num, type); t->sector_size = 2352; t->data_off = 16; }
		} else if (ieq_prefix(s, "INDEX") && t) {
			int idx = -1;
			char tm[32] = "";
			sscanf(s + 5, "%d %31s", &idx, tm);
			if (idx == 1) t->start = msf(tm);
			else if (idx == 0) t->index0 = msf(tm);
		}
	}
	fclose(c);
	return cd->ntracks > 0 ? 0 : -1;
}

static void compute_lengths(CdImage *cd)
{
	int i;
	for (i = 0; i < cd->ntracks; i++) {
		CdTrack *t = &cd->t[i];
		int64_t end;
		if (t->start < 0) t->start = t->index0 >= 0 ? t->index0 : 0;
		if (i + 1 < cd->ntracks && cd->t[i + 1].file == t->file)
			end = cd->t[i + 1].index0 >= 0 ? cd->t[i + 1].index0 : cd->t[i + 1].start;
		else
			end = cd->fsize[t->file] / t->sector_size;
		t->length = end > t->start ? end - t->start : 0;
	}
}

int cd_open(CdImage *cd, const char *path)
{
	memset(cd, 0, sizeof *cd);
	cd->cache_track = -1;
	if (ends_with_ci(path, ".cue")) {
		if (parse_cue(cd, path)) { cd_close(cd); return -1; }
	} else {
		/* single file: raw 2352 with the sync pattern, else cooked 2048 */
		static const unsigned char sync[12] = { 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0 };
		unsigned char h[16];
		CdTrack *t;
		cd->f[0] = fopen(path, "rb");
		if (!cd->f[0]) { fprintf(stderr, "w2conv: cannot open %s\n", path); return -1; }
		cd->fsize[0] = w2c_fsize(cd->f[0]);
		cd->nfiles = 1;
		t = &cd->t[0];
		cd->ntracks = 1;
		t->number = 1; t->file = 0; t->start = 0; t->index0 = -1;
		if (fread(h, 1, 16, cd->f[0]) == 16 && !memcmp(h, sync, 12) && cd->fsize[0] % 2352 == 0) {
			t->sector_size = 2352; t->data_off = (h[15] == 2) ? 24 : 16;
		} else { t->sector_size = 2048; t->data_off = 0; }
	}
	compute_lengths(cd);
	return 0;
}

void cd_close(CdImage *cd)
{
	int i;
	for (i = 0; i < cd->nfiles; i++) if (cd->f[i]) fclose(cd->f[i]);
	memset(cd, 0, sizeof *cd);
	cd->cache_track = -1;
}

int cd_first_data_track(const CdImage *cd)
{
	int i;
	for (i = 0; i < cd->ntracks; i++) if (!cd->t[i].audio) return i;
	return -1;
}

int64_t cd_track_bytes(const CdImage *cd, int ti)
{
	return cd->t[ti].length * 2048;
}

static int load_sector(CdImage *cd, int ti, int64_t sec)
{
	CdTrack *t = &cd->t[ti];
	if (cd->cache_track == ti && cd->cache_sector == sec) return 0;
	if (sec < 0 || sec >= t->length) return -1;
	if (w2c_seek(cd->f[t->file], (t->start + sec) * t->sector_size + t->data_off)) return -1;
	if (fread(cd->cache, 1, 2048, cd->f[t->file]) != 2048) return -1;
	cd->cache_track = ti; cd->cache_sector = sec;
	return 0;
}

int cd_read(CdImage *cd, int ti, int64_t off, void *dst, size_t len)
{
	unsigned char *d = (unsigned char *)dst;
	CdTrack *t = &cd->t[ti];
	if (t->sector_size == 2048) {   /* cooked: read straight through */
		if (off + (int64_t)len > t->length * 2048) return -1;
		if (w2c_seek(cd->f[t->file], t->start * 2048 + off)) return -1;
		return fread(d, 1, len, cd->f[t->file]) == len ? 0 : -1;
	}
	while (len) {
		int64_t sec = off / 2048;
		size_t in = (size_t)(off % 2048), n = 2048 - in;
		if (n > len) n = len;
		if (load_sector(cd, ti, sec)) return -1;
		memcpy(d, cd->cache + in, n);
		d += n; off += (int64_t)n; len -= n;
	}
	return 0;
}

int cd_read_audio(CdImage *cd, int ti, int64_t first, int frames, int16_t *dst)
{
	CdTrack *t = &cd->t[ti];
	static unsigned char buf[2352 * 75];
	int done = 0;
	if (first >= t->length) return 0;
	if (first + frames > t->length) frames = (int)(t->length - first);
	if (w2c_seek(cd->f[t->file], (t->start + first) * 2352)) return 0;
	while (done < frames) {
		int n = frames - done, i, got;
		if (n > 75) n = 75;
		got = (int)(fread(buf, 2352, (size_t)n, cd->f[t->file]));
		if (got <= 0) break;
		for (i = 0; i < got * 1176; i++) {
			const unsigned char *p = buf + i * 2;
			dst[(size_t)done * 1176 + i] = (int16_t)(t->big_endian ? (p[0] << 8 | p[1]) : (p[1] << 8 | p[0]));
		}
		done += got;
		if (got < n) break;
	}
	return done;
}
