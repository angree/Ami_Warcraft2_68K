/* w2conv - the AmiW2 player's data converter (PC side: Linux, Windows).
 *
 * Makes the Amiga "data" folder from the player's OWN Warcraft II:
 *   - a Mac CD image (.cue + .bin, or a plain .iso/.bin): Apple Partition Map + HFS read by our own
 *     code; copies Data/War Data, War Resources, War Strings, War Sounds, War Music, War Movies,
 *     Title Song and Custom PUDs; CD audio tracks -> music/trackNN.adp (IMA ADPCM, mono);
 *   - a PC CD image (ISO 9660 data track): DATA/MAINDAT.WAR etc. + its CD audio;
 *   - a folder with the archives (Mac names or PC names); --cue adds CD audio from an image.
 * Output: <outdir>/data/ (archives under their own names, music/, pud/, conversion.txt).
 * Nothing original ends up anywhere but the folder you name.
 */
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#define MKDIR(p) mkdir(p, 0777)
#endif
#include "cdimage.h"
#include "hfs.h"
#include "adpcm_enc.h"
#include "lores_pack.h"

#define W2CONV_VERSION    "1.0"
#define CONVERSION_STAMP  "AmiW2 data conversion 1"

typedef struct Archive { const char *mac, *pc; int id; int required; } Archive;
static const Archive g_arch[] = {
	{ "War Data",      "MAINDAT.WAR", 1000, 1 },
	{ "War Resources", "REZDAT.WAR",  3000, 1 },
	{ "War Strings",   "STRDAT.WAR",  4000, 1 },
	{ "War Sounds",    "SFXDAT.SUD",  5000, 1 },
	{ "War Music",     "SNDDAT.WAR",  2000, 0 },
	{ "War Movies",    "MUDDAT.CUD",  6000, 0 },
};
#define NARCH ((int)(sizeof g_arch / sizeof g_arch[0]))

static char g_out[1024];        /* <outdir>/data */
static FILE *g_stamp;
static int g_rate = 14700, g_music = 1, g_lores = 1, g_ehb = 1, g_errors;
static int g_have_title;

/* ------------------------------------------------------------------ crc32 / helpers */
static unsigned long g_crctab[256];
static void crc_init(void)
{
	unsigned long c; int n, k;
	for (n = 0; n < 256; n++) { c = (unsigned long)n; for (k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320UL ^ (c >> 1) : c >> 1; g_crctab[n] = c; }
}
static unsigned long crc_upd(unsigned long crc, const unsigned char *p, size_t n)
{
	crc = ~crc & 0xFFFFFFFFUL;
	while (n--) crc = g_crctab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
	return ~crc & 0xFFFFFFFFUL;
}

static void join(char *dst, size_t cap, const char *a, const char *b)
{
	size_t n = strlen(a);
#ifdef _WIN32
	const char *sep = "\\";
#else
	const char *sep = "/";
#endif
	if (n && (a[n - 1] == '/' || a[n - 1] == '\\' || a[n - 1] == ':')) sep = "";
	snprintf(dst, cap, "%s%s%s", a, sep, b);
}

static int is_dir(const char *p) { struct stat st; return stat(p, &st) == 0 && S_ISDIR(st.st_mode); }

static void mkdirs(const char *path)
{
	char tmp[1024]; size_t i;
	snprintf(tmp, sizeof tmp, "%s", path);
	for (i = 1; tmp[i]; i++) if (tmp[i] == '/' || tmp[i] == '\\') { char c = tmp[i]; tmp[i] = 0; if (!is_dir(tmp)) MKDIR(tmp); tmp[i] = c; }
	if (!is_dir(tmp)) MKDIR(tmp);
}

static const char *base_name(const char *p)
{
	const char *a = strrchr(p, '/'), *b = strrchr(p, '\\');
	if (b > a) a = b;
	return a ? a + 1 : p;
}

/* case-insensitive lookup of 'name' inside directory 'dir'; full path into out */
static int find_ci(const char *dir, const char *name, char *out, size_t cap)
{
	DIR *d = opendir(dir);
	struct dirent *e;
	if (!d) return 0;
	while ((e = readdir(d)) != NULL) {
		const char *a = e->d_name, *b = name;
		while (*a && *b && toupper((unsigned char)*a) == toupper((unsigned char)*b)) { a++; b++; }
		if (!*a && !*b) { join(out, cap, dir, e->d_name); closedir(d); return 1; }
	}
	closedir(d);
	return 0;
}

/* check an archive header: magic 0x19, archive id */
static int check_header(const unsigned char *h, int want_id)
{
	unsigned long magic = (unsigned long)h[0] | (unsigned long)h[1] << 8 | (unsigned long)h[2] << 16 | (unsigned long)h[3] << 24;
	int id = h[6] | h[7] << 8;
	return magic == 0x19 && id == want_id;
}

/* ------------------------------------------------------------------ output of one archive */
typedef struct Sink { FILE *f; unsigned long crc; long size; unsigned char head[8]; } Sink;

static void sink_put(Sink *s, const unsigned char *p, size_t n)
{
	size_t i;
	for (i = 0; i < n && s->size + (long)i < 8; i++) s->head[s->size + i] = p[i];
	s->crc = crc_upd(s->crc, p, n);
	s->size += (long)n;
	if (fwrite(p, 1, n, s->f) != n) g_errors++;
}

static void stamp_archive(const Archive *a, const char *name, const Sink *s)
{
	int ok = s->size >= 8 && check_header(s->head, a->id);
	printf("  %-14s %10ld bytes  crc32 %08lx  %s\n", name, s->size, s->crc, ok ? "ok" : "BAD HEADER");
	if (g_stamp) fprintf(g_stamp, "file %-14s %10ld crc32 %08lx id %d%s\n", name, s->size, s->crc, a->id, ok ? "" : " BAD");
	if (!ok) g_errors++;
}

static int copy_plain(const char *src, const char *dstname, const Archive *a)
{
	char dst[1200];
	static unsigned char buf[1 << 16];
	FILE *in = fopen(src, "rb");
	Sink s;
	size_t n;
	if (!in) return -1;
	join(dst, sizeof dst, g_out, dstname);
	memset(&s, 0, sizeof s);
	s.f = fopen(dst, "wb");
	if (!s.f) { fclose(in); fprintf(stderr, "w2conv: cannot write %s\n", dst); g_errors++; return -1; }
	while ((n = fread(buf, 1, sizeof buf, in)) > 0) sink_put(&s, buf, n);
	fclose(in);
	if (fclose(s.f)) g_errors++;
	if (a) stamp_archive(a, dstname, &s);
	else {
		printf("  %-14s %10ld bytes\n", dstname, s.size);
		if (g_stamp) fprintf(g_stamp, "file %-14s %10ld crc32 %08lx\n", dstname, s.size, s.crc);
	}
	return 0;
}

/* ------------------------------------------------------------------ music */
static void music_from_pcm(const int16_t *mono, long n, long rate, const char *outname, const char *what)
{
	char dir[1200], path[1300];
	long rn = 0, size;
	int16_t *rs;
	int peak, rms;
	join(dir, sizeof dir, g_out, "music");
	mkdirs(dir);
	join(path, sizeof path, dir, outname);
	rs = adpe_resample(mono, n, rate, g_rate, &rn);
	if (!rs) { g_errors++; return; }
	adpe_stats(rs, rn, &peak, &rms);
	size = adpe_write_wav(path, rs, rn, g_rate);
	free(rs);
	if (size < 0) { g_errors++; return; }
	printf("  music/%-12s %4ld.%01ld s  peak %5d rms %5d  %9ld bytes  (%s)\n", outname, rn / g_rate, (rn % g_rate) * 10 / g_rate,
	       peak, rms, size, what);
	if (g_stamp) fprintf(g_stamp, "music %s %ld samples %d Hz %ld bytes (%s)\n", outname, rn, g_rate, size, what);
}

/* Byte order of CD-DA samples: the right one gives a much smoother waveform. */
static void detect_audio_order(CdImage *cd, int ti)
{
	static int16_t buf[75 * 1176];
	double d0 = 0, d1 = 0;
	int64_t at = cd->t[ti].length / 2;
	int got, i, save = cd->t[ti].big_endian;
	cd->t[ti].big_endian = 0;
	got = cd_read_audio(cd, ti, at, 75, buf);
	for (i = 2; i < got * 1176; i += 2) {
		int a = buf[i], b = buf[i - 2];
		int sa = (int16_t)(((unsigned)buf[i] & 0xFF) << 8 | ((unsigned)buf[i] >> 8 & 0xFF));
		int sb = (int16_t)(((unsigned)buf[i - 2] & 0xFF) << 8 | ((unsigned)buf[i - 2] >> 8 & 0xFF));
		d0 += abs(a - b); d1 += abs(sa - sb);
	}
	if (got == 0) { cd->t[ti].big_endian = save; return; }
	cd->t[ti].big_endian = d1 < d0 * 0.5 ? 1 : 0;
	if (cd->t[ti].big_endian) printf("  (track %d: big-endian audio samples)\n", cd->t[ti].number);
}

static void convert_cd_audio(CdImage *cd)
{
	int i, n = 0;
	for (i = 0; i < cd->ntracks; i++) if (cd->t[i].audio) n++;
	if (!n) { printf("No CD audio tracks in the image - no music.\n"); return; }
	printf("Converting %d CD audio tracks to %d Hz mono IMA ADPCM...\n", n, g_rate);
	for (i = 0; i < cd->ntracks; i++) {
		CdTrack *t = &cd->t[i];
		int16_t *st, *mono;
		long frames, k, got;
		char name[32], what[64];
		if (!t->audio || t->length <= 0) continue;
		detect_audio_order(cd, i);
		frames = (long)t->length;
		st = (int16_t *)malloc(sizeof(int16_t) * 1176 * (size_t)frames);
		mono = (int16_t *)malloc(sizeof(int16_t) * 588 * (size_t)frames);
		if (!st || !mono) { free(st); free(mono); fprintf(stderr, "w2conv: out of memory\n"); g_errors++; return; }
		got = cd_read_audio(cd, i, 0, (int)frames, st);
		for (k = 0; k < got * 588; k++) mono[k] = (int16_t)(((int)st[2 * k] + st[2 * k + 1]) / 2);
		free(st);
		snprintf(name, sizeof name, "track%02d.adp", t->number);
		snprintf(what, sizeof what, "CD track %d, %ld:%02ld", t->number, got / 75 / 60, got / 75 % 60);
		music_from_pcm(mono, got * 588, 44100, name, what);
		free(mono);
	}
}

/* plain RIFF PCM WAV (8/16-bit, mono/stereo) in memory -> mono 16-bit */
static int16_t *wav_to_mono(const unsigned char *b, long len, long *n, long *rate)
{
	long p = 12, data = -1, dlen = 0;
	int ch = 0, bits = 0, tag = 0;
	int16_t *m;
	long i, cnt;
	*n = 0;
	if (len < 12 || memcmp(b, "RIFF", 4) || memcmp(b + 8, "WAVE", 4)) return NULL;
	while (p + 8 <= len) {
		long cl = (long)((unsigned long)b[p + 4] | (unsigned long)b[p + 5] << 8 | (unsigned long)b[p + 6] << 16 | (unsigned long)b[p + 7] << 24);
		if (!memcmp(b + p, "fmt ", 4) && cl >= 16) {
			tag = b[p + 8] | b[p + 9] << 8; ch = b[p + 10] | b[p + 11] << 8;
			*rate = (long)((unsigned long)b[p + 12] | (unsigned long)b[p + 13] << 8 | (unsigned long)b[p + 14] << 16);
			bits = b[p + 22] | b[p + 23] << 8;
		} else if (!memcmp(b + p, "data", 4)) { data = p + 8; dlen = cl; if (data + dlen > len) dlen = len - data; break; }
		p += 8 + cl + (cl & 1);
	}
	if (data < 0 || tag != 1 || ch < 1 || ch > 2 || (bits != 8 && bits != 16)) return NULL;
	cnt = dlen / (ch * bits / 8);
	m = (int16_t *)malloc(sizeof(int16_t) * (size_t)(cnt ? cnt : 1));
	if (!m) return NULL;
	for (i = 0; i < cnt; i++) {
		int c, acc = 0;
		for (c = 0; c < ch; c++) {
			const unsigned char *s = b + data + (i * ch + c) * (bits / 8);
			acc += bits == 8 ? ((int)s[0] - 128) << 8 : (int16_t)(s[0] | s[1] << 8);
		}
		m[i] = (int16_t)(acc / ch);
	}
	*n = cnt;
	return m;
}

static void title_song(const char *path)
{
	FILE *f = fopen(path, "rb");
	unsigned char *b;
	long len, n = 0, rate = 0;
	int16_t *m;
	if (!f) return;
	len = (long)w2c_fsize(f);
	b = (unsigned char *)malloc((size_t)(len > 0 ? len : 1));
	if (!b || fread(b, 1, (size_t)len, f) != (size_t)len) { fclose(f); free(b); return; }
	fclose(f);
	m = wav_to_mono(b, len, &n, &rate);
	free(b);
	if (!m) { printf("  Title Song: not a PCM WAV - skipped\n"); return; }
	music_from_pcm(m, n, rate, "title.adp", "Title Song");
	free(m);
	g_have_title = 1;
}

/* ------------------------------------------------------------------ Mac CD (HFS) */
typedef struct HfsUd { CdImage *cd; int ti; } HfsUd;
static int hfs_rd(void *ud, int64_t off, void *dst, size_t len) { HfsUd *u = (HfsUd *)ud; return cd_read(u->cd, u->ti, off, dst, len); }


static int extract_hfs_file(const Hfs *h, const HfsFile *hf, const char *dstname, const Archive *a)
{
	char dst[1200];
	Sink s;
	FILE *tmp;
	long r;
	static unsigned char buf[1 << 16];
	size_t n;
	join(dst, sizeof dst, g_out, dstname);
	tmp = fopen(dst, "wb+");
	if (!tmp) { fprintf(stderr, "w2conv: cannot write %s\n", dst); g_errors++; return -1; }
	r = hfs_extract(h, hf, tmp);
	if (r < 0) { fclose(tmp); fprintf(stderr, "w2conv: reading %s from the CD failed\n", hf->name); g_errors++; return -1; }
	/* checksum pass over what was written */
	memset(&s, 0, sizeof s);
	rewind(tmp);
	while ((n = fread(buf, 1, sizeof buf, tmp)) > 0) {
		size_t i;
		for (i = 0; i < n && s.size + (long)i < 8; i++) s.head[s.size + i] = buf[i];
		s.crc = crc_upd(s.crc, buf, n); s.size += (long)n;
	}
	if (fclose(tmp)) g_errors++;
	if (a) stamp_archive(a, dstname, &s);
	else {
		printf("  %-14s %10ld bytes\n", dstname, s.size);
		if (g_stamp) fprintf(g_stamp, "file %-14s %10ld crc32 %08lx\n", dstname, s.size, s.crc);
	}
	return 0;
}

static int convert_mac(CdImage *cd, int ti, int list_only)
{
	Hfs h;
	HfsUd ud;
	int i, missing = 0;
	char p[300];
	ud.cd = cd; ud.ti = ti;
	if (hfs_open(&h, hfs_rd, &ud, cd_track_bytes(cd, ti))) return -1;
	printf("Mac HFS volume \"%s\": %d files, %d folders\n", h.volname, h.nfiles, h.ndirs);
	if (list_only) {
		for (i = 0; i < h.nfiles; i++) {
			hfs_path(&h, &h.files[i], p, sizeof p);
			printf("%10lu %8lu %-4s %-4s %s\n", (unsigned long)h.files[i].data_len, (unsigned long)h.files[i].rsrc_len,
			       h.files[i].type, h.files[i].creator, p);
		}
		hfs_close(&h);
		return 0;
	}
	if (g_stamp) fprintf(g_stamp, "source mac-cd volume \"%s\"\n", h.volname);
	printf("Copying the game archives...\n");
	for (i = 0; i < NARCH; i++) {
		const HfsFile *f;
		snprintf(p, sizeof p, "Data/%s", g_arch[i].mac);
		f = hfs_find(&h, p);
		if (!f) { printf("  %-14s not on this CD%s\n", g_arch[i].mac, g_arch[i].required ? " - REQUIRED" : ""); if (g_arch[i].required) missing++; continue; }
		extract_hfs_file(&h, f, g_arch[i].mac, &g_arch[i]);
	}
	{
		const HfsFile *ts = hfs_find(&h, "Data/Title Song");
		if (ts) {
			char tp[1200];
			extract_hfs_file(&h, ts, "Title Song", NULL);
			join(tp, sizeof tp, g_out, "Title Song");
			if (g_music) title_song(tp);
		}
	}
	/* custom maps */
	{
		char pd[1200];
		int npud = 0;
		join(pd, sizeof pd, g_out, "pud");
		for (i = 0; i < h.nfiles; i++) {
			char full[300], dn[300];
			size_t l = strlen(h.files[i].name);
			hfs_path(&h, &h.files[i], full, sizeof full);
			if (strncmp(full, "Custom PUDs/", 12) != 0 || l < 5) continue;
			if (!npud++) { mkdirs(pd); printf("Copying the custom maps...\n"); }
			snprintf(dn, sizeof dn, "pud/%s", h.files[i].name);
			extract_hfs_file(&h, &h.files[i], dn, NULL);
		}
	}
	hfs_close(&h);
	return missing ? -2 : 0;
}

/* ------------------------------------------------------------------ PC CD (ISO 9660) */
static uint32_t le32(const unsigned char *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }

/* look up name in the directory at extent/size; returns 1 and the entry's extent/size/isdir */
static int iso_lookup(CdImage *cd, int ti, uint32_t ext, uint32_t size, const char *name, uint32_t *oext, uint32_t *osize, int *odir)
{
	static unsigned char sec[2048];
	uint32_t off;
	for (off = 0; off < size; off += 2048) {
		int p = 0;
		if (cd_read(cd, ti, (int64_t)(ext + off / 2048) * 2048, sec, 2048)) return 0;
		while (p < 2048 && sec[p]) {
			int rl = sec[p], nl = sec[p + 32], k;
			const char *nm = (const char *)sec + p + 33;
			int match = 1;
			if (rl < 34 || p + rl > 2048) break;
			for (k = 0; name[k]; k++) if (k >= nl || toupper((unsigned char)nm[k]) != toupper((unsigned char)name[k])) { match = 0; break; }
			if (match && (k == nl || nm[k] == ';' || (nm[k] == '.' && k + 1 < nl && nm[k + 1] == ';'))) {
				*oext = le32(sec + p + 2); *osize = le32(sec + p + 10); *odir = (sec[p + 25] & 2) != 0;
				return 1;
			}
			p += rl;
		}
	}
	return 0;
}

static int convert_pc_iso(CdImage *cd, int ti)
{
	unsigned char pvd[2048];
	uint32_t rext, rsize, dext, dsize;
	int isdir, i, missing = 0;
	if (cd_read(cd, ti, 16 * 2048, pvd, 2048) || pvd[0] != 1 || memcmp(pvd + 1, "CD001", 5)) return -1;
	rext = le32(pvd + 156 + 2); rsize = le32(pvd + 156 + 10);
	printf("ISO 9660 volume (PC CD)\n");
	if (g_stamp) fprintf(g_stamp, "source pc-cd\n");
	if (!iso_lookup(cd, ti, rext, rsize, "DATA", &dext, &dsize, &isdir) || !isdir) { dext = rext; dsize = rsize; }
	printf("Copying the game archives...\n");
	for (i = 0; i < NARCH; i++) {
		uint32_t fe, fs;
		char dst[1200];
		Sink s;
		static unsigned char buf[1 << 16];
		uint32_t done = 0;
		if (!iso_lookup(cd, ti, dext, dsize, g_arch[i].pc, &fe, &fs, &isdir) && !iso_lookup(cd, ti, rext, rsize, g_arch[i].pc, &fe, &fs, &isdir)) {
			printf("  %-14s not on this CD%s\n", g_arch[i].pc, g_arch[i].required ? " - REQUIRED" : "");
			if (g_arch[i].required) missing++;
			continue;
		}
		join(dst, sizeof dst, g_out, g_arch[i].pc);
		memset(&s, 0, sizeof s);
		s.f = fopen(dst, "wb");
		if (!s.f) { g_errors++; continue; }
		while (done < fs) {
			uint32_t n = fs - done > sizeof buf ? (uint32_t)sizeof buf : fs - done;
			if (cd_read(cd, ti, (int64_t)fe * 2048 + done, buf, n)) { g_errors++; break; }
			sink_put(&s, buf, n);
			done += n;
		}
		if (fclose(s.f)) g_errors++;
		stamp_archive(&g_arch[i], g_arch[i].pc, &s);
	}
	return missing ? -2 : 0;
}

/* ------------------------------------------------------------------ folder source */
static int convert_folder(const char *dir)
{
	int i, missing = 0;
	static const char *const sub[] = { "", "Data", "DATA" };
	if (g_stamp) fprintf(g_stamp, "source folder\n");
	printf("Copying the game archives from the folder...\n");
	for (i = 0; i < NARCH; i++) {
		char src[1200], d[1200];
		int s, found = 0;
		for (s = 0; s < 3 && !found; s++) {
			if (*sub[s]) { if (!find_ci(dir, sub[s], d, sizeof d)) continue; }
			else snprintf(d, sizeof d, "%s", dir);
			if (find_ci(d, g_arch[i].mac, src, sizeof src)) { copy_plain(src, g_arch[i].mac, &g_arch[i]); found = 1; }
			else if (find_ci(d, g_arch[i].pc, src, sizeof src)) { copy_plain(src, g_arch[i].pc, &g_arch[i]); found = 1; }
			if (found && i == 0) {   /* a Mac folder may also hold the Title Song */
				char ts[1200];
				if (find_ci(d, "Title Song", ts, sizeof ts)) { copy_plain(ts, "Title Song", NULL); if (g_music) title_song(ts); }
			}
		}
		if (!found) {
			printf("  %-14s (%s) not found%s\n", g_arch[i].mac, g_arch[i].pc, g_arch[i].required ? " - REQUIRED" : "");
			if (g_arch[i].required) missing++;
		}
	}
	return missing ? -2 : 0;
}

/* a .cue next to / inside a folder */
static int find_cue_in(const char *dir, char *out, size_t cap)
{
	DIR *d = opendir(dir);
	struct dirent *e;
	if (!d) return 0;
	while ((e = readdir(d)) != NULL) {
		size_t l = strlen(e->d_name);
		if (l > 4 && (!strcmp(e->d_name + l - 4, ".cue") || !strcmp(e->d_name + l - 4, ".CUE"))) { join(out, cap, dir, e->d_name); closedir(d); return 1; }
	}
	closedir(d);
	return 0;
}

static int convert_image(const char *path, int list_only)
{
	CdImage *cd = (CdImage *)calloc(1, sizeof(CdImage));
	int ti, r;
	if (!cd) return -1;
	if (cd_open(cd, path)) { free(cd); return -1; }
	{
		int i, na = 0;
		for (i = 0; i < cd->ntracks; i++) na += cd->t[i].audio;
		printf("CD image %s: %d tracks (%d audio)\n", base_name(path), cd->ntracks, na);
	}
	ti = cd_first_data_track(cd);
	if (ti < 0) { r = -1; printf("No data track.\n"); }
	else {
		r = convert_mac(cd, ti, list_only);
		if (r == -1 && !list_only) r = convert_pc_iso(cd, ti);
		if (r == -1) fprintf(stderr, "w2conv: the data track is neither a Mac HFS nor a PC ISO 9660 volume\n");
	}
	if (r >= 0 && !list_only && g_music) convert_cd_audio(cd);
	cd_close(cd);
	free(cd);
	return r;
}

static void usage(void)
{
	printf("w2conv %s - makes the AmiW2 data folder from your own Warcraft II\n\n"
	       "usage: w2conv [options] <source> <outdir>\n"
	       "  <source>  Mac or PC CD image (.cue with its .bin, or .iso), or a folder with the\n"
	       "            game files (\"War Data\" ... or MAINDAT.WAR ...)\n"
	       "  <outdir>  the folder \"data\" is created inside it\n"
	       "options:\n"
	       "  --cue <file.cue>  CD image to take the music (CD audio) from, for a folder source\n"
	       "  --rate <hz>       music sample rate: 11025, 14700 (default) or 22050\n"
	       "  --no-music        skip the music\n"
	       "  --no-lores        skip the lores (320x240) art packs\n"
	       "  --no-ehb          skip the ECS Extra-Half-Brite (64 colours) art packs\n"
	       "  --lores-only      only (re)build the lores and EHB packs of an existing <outdir>/data:\n"
	       "                    w2conv --lores-only <outdir>\n"
	       "  --list            list the files of a Mac CD image and stop\n", W2CONV_VERSION);
}

/* the game's lores art packs, or (ehb) the same art for the ECS Extra-Half-Brite screen (built with
 * the game's own src/lores.c + src/ehb.c); stamped into conversion.txt */
static int lores_step(const char *out, int ehb)
{
	static const char *const pks[2][3] = { { "lores-summer.pack", "lores-winter.pack", "lores-wasteland.pack" },
	                                       { "ehb-summer.pack", "ehb-winter.pack", "ehb-wasteland.pack" } };
	const char *const *pk = pks[ehb ? 1 : 0];
	static unsigned char buf[1 << 16];
	int i, r;
	if (ehb) printf("Building the ECS Extra-Half-Brite (320x240, 64 colours) art packs...\n");
	else printf("Building the lores (320x240) art packs with the game's own filter...\n");
	fflush(stdout);
	r = lores_packs_build(out, ehb);
	for (i = 0; i < 3; i++) {
		char p[1200];
		FILE *f;
		unsigned long crc = 0;
		long size = 0;
		size_t n;
		join(p, sizeof p, g_out, pk[i]);
		f = fopen(p, "rb");
		if (!f) { printf("  %-20s MISSING\n", pk[i]); r = -1; continue; }
		while ((n = fread(buf, 1, sizeof buf, f)) > 0) { crc = crc_upd(crc, buf, n); size += (long)n; }
		fclose(f);
		printf("  %-20s %9ld bytes  crc32 %08lx\n", pk[i], size, crc);
		if (g_stamp) fprintf(g_stamp, "%-5s %-20s %9ld crc32 %08lx\n", ehb ? "ehb" : "lores", pk[i], size, crc);
	}
	if (r) fprintf(stderr, "w2conv: building the %s packs failed\n", ehb ? "EHB" : "lores");
	return r;
}

int main(int argc, char **argv)
{
	const char *src = NULL, *out = NULL, *cue = NULL;
	char stamp[1200], incue[1200];
	int i, list = 0, lores_only = 0, r;
	crc_init();
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--cue") && i + 1 < argc) cue = argv[++i];
		else if (!strcmp(argv[i], "--rate") && i + 1 < argc) g_rate = atoi(argv[++i]);
		else if (!strcmp(argv[i], "--no-music")) g_music = 0;
		else if (!strcmp(argv[i], "--no-lores")) g_lores = 0;
		else if (!strcmp(argv[i], "--no-ehb")) g_ehb = 0;
		else if (!strcmp(argv[i], "--lores-only")) lores_only = 1;
		else if (!strcmp(argv[i], "--list")) list = 1;
		else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) { usage(); return 0; }
		else if (!src) src = argv[i];
		else if (!out) out = argv[i];
		else { usage(); return 2; }
	}
	if (g_rate != 11025 && g_rate != 14700 && g_rate != 22050) { fprintf(stderr, "w2conv: --rate must be 11025, 14700 or 22050\n"); return 2; }
	if (!src || (!out && !list && !lores_only)) { usage(); return 2; }
	if (list) return convert_image(src, 1) < 0 ? 1 : 0;
	{   /* "C:\dir\." (extract.bat: a trailing backslash would escape the quote) -> "C:\dir\" */
		static char o2[1024], s2[1024];
		size_t l;
		if (out) { snprintf(o2, sizeof o2, "%s", out); l = strlen(o2); if (l > 2 && o2[l - 1] == '.' && (o2[l - 2] == '\\' || o2[l - 2] == '/')) o2[l - 1] = 0; out = o2; }
		snprintf(s2, sizeof s2, "%s", src); l = strlen(s2); if (lores_only && l > 2 && s2[l - 1] == '.' && (s2[l - 2] == '\\' || s2[l - 2] == '/')) s2[l - 1] = 0; src = s2;
	}
	if (lores_only) {   /* <source> is the folder that holds data/ */
		join(g_out, sizeof g_out, src, "data");
		r = 0;
		if (g_lores && lores_step(src, 0)) r = 1;
		if (g_ehb && lores_step(src, 1)) r = 1;
		return r;
	}

	join(g_out, sizeof g_out, out, "data");
	mkdirs(g_out);
	join(stamp, sizeof stamp, g_out, "conversion.txt");
	g_stamp = fopen(stamp, "wb");   /* same bytes (LF) on every host */
	if (!g_stamp) { fprintf(stderr, "w2conv: cannot write into %s\n", g_out); return 1; }
	fprintf(g_stamp, "%s\nconverter w2conv %s\n", CONVERSION_STAMP, W2CONV_VERSION);
	printf("Output: %s\n", g_out);

	if (is_dir(src)) {
		r = convert_folder(src);
		if (r == -2 && find_cue_in(src, incue, sizeof incue)) {   /* a folder holding the CD image */
			printf("No archives in the folder, but %s - using the CD image.\n", base_name(incue));
			r = convert_image(incue, 0);
			cue = NULL;
		}
		if (r >= 0 && g_music && cue) {
			CdImage *cd = (CdImage *)calloc(1, sizeof(CdImage));
			if (cd && cd_open(cd, cue) == 0) { convert_cd_audio(cd); cd_close(cd); }
			else g_errors++;
			free(cd);
		}
	} else r = convert_image(src, 0);

	if (r == -2) fprintf(stderr, "w2conv: required game files are missing\n");
	if (r >= 0 && !g_errors && g_lores && lores_step(out, 0)) g_errors++;
	if (r >= 0 && !g_errors && g_ehb && lores_step(out, 1)) g_errors++;
	fprintf(g_stamp, "music_rate %d\n", g_music ? g_rate : 0);
	fprintf(g_stamp, "result %s\n", (r < 0 || g_errors) ? "FAILED" : "ok");
	fclose(g_stamp);
	if (r < 0 || g_errors) { fprintf(stderr, "w2conv: conversion FAILED (%d errors)\n", g_errors); return 1; }
	printf("Done. Copy the folder \"%s\" to the Amiga, into the AmiW2 drawer, as \"data\".\n", g_out);
	return 0;
}
