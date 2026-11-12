/* test_smk.c - host test of src/smk.c on the player's "War Movies" archive.
 *
 *   test_smk <War Movies> <outdir> [ppm_movies]
 *
 * Walks the archive (u32 0x19, u16 count, u16 id, u32 offsets[count]; an
 * entry is u32 size with the flags in the top byte, then the data), opens
 * every SMK2 entry, decodes EVERY frame (video + audio) and prints a line per
 * movie: size, frames, frame time, audio format, decode time, audio errors,
 * and an audio continuity check (mean step across chunk borders vs. inside
 * chunks - a wrong predictor start would make the border step huge).
 *
 * Writes into <outdir> (which must be OUTSIDE the repository - original
 * content never enters it):
 *   movNN_fFFFF.ppm   the middle frame of each of the first `ppm_movies`
 *                     movies (default 4), plus frames 0 and last of movie 0
 *   movNN.wav         track 0 audio of the first movie that has any
 *
 * Plain C, host only.
 */
#include "../src/smk.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint32_t le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint8_t *load_file(const char *path, uint32_t *n)
{
	FILE *f = fopen(path, "rb");
	long sz;
	uint8_t *b;
	if (!f) return 0;
	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	b = (uint8_t *)malloc((size_t)sz);
	if (b && fread(b, 1, (size_t)sz, f) != (size_t)sz) { free(b); b = 0; }
	fclose(f);
	*n = (uint32_t)sz;
	return b;
}

static void write_ppm(const char *path, const SmkVideo *v)
{
	int ydup = (v->flags & (SMK_FLAG_Y_DOUBLE | SMK_FLAG_Y_INTERLACE)) ? 2 : 1;
	FILE *f = fopen(path, "wb");
	int x, y, k;
	uint8_t *line;
	if (!f) { printf("  cannot write %s\n", path); return; }
	fprintf(f, "P6\n%d %d\n255\n", v->w, v->h * ydup);
	line = (uint8_t *)malloc((size_t)v->w * 3);
	for (y = 0; y < v->h; ++y) {
		const uint8_t *s = v->pixels + (size_t)y * v->w;
		for (x = 0; x < v->w; ++x) memcpy(line + 3 * x, v->palette + 3 * s[x], 3);
		for (k = 0; k < ydup; ++k) {
			if (k && (v->flags & SMK_FLAG_Y_INTERLACE)) memset(line, 0, (size_t)v->w * 3);
			fwrite(line, 1, (size_t)v->w * 3, f);
		}
	}
	free(line);
	fclose(f);
	printf("  wrote %s\n", path);
}

static void put16(FILE *f, unsigned v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); }
static void put32(FILE *f, uint32_t v) { put16(f, v & 0xFFFF); put16(f, v >> 16); }

typedef struct Wav { FILE *f; uint32_t bytes; int bits, ch; uint32_t rate; } Wav;

static void wav_open(Wav *w, const char *path, const SmkTrack *t)
{
	w->f = fopen(path, "wb");
	w->bytes = 0;
	w->bits = t->bits;
	w->ch = t->channels;
	w->rate = t->rate;
	if (!w->f) return;
	fwrite("RIFF\0\0\0\0WAVEfmt ", 1, 16, w->f);
	put32(w->f, 16);
	put16(w->f, 1);
	put16(w->f, (unsigned)w->ch);
	put32(w->f, w->rate);
	put32(w->f, w->rate * (uint32_t)w->ch * (uint32_t)(w->bits / 8));
	put16(w->f, (unsigned)(w->ch * w->bits / 8));
	put16(w->f, (unsigned)w->bits);
	fwrite("data\0\0\0\0", 1, 8, w->f);
}

static void wav_add(Wav *w, const uint8_t *p, uint32_t n)
{
	uint32_t i;
	if (!w->f) return;
	if (w->bits == 16) {
		const int16_t *s = (const int16_t *)p;
		for (i = 0; i < n / 2; ++i) put16(w->f, (uint16_t)s[i]);
	} else {
		fwrite(p, 1, n, w->f);
	}
	w->bytes += n;
}

static void wav_close(Wav *w, const char *path)
{
	if (!w->f) return;
	fseek(w->f, 4, SEEK_SET);
	put32(w->f, 36 + w->bytes);
	fseek(w->f, 40, SEEK_SET);
	put32(w->f, w->bytes);
	fclose(w->f);
	w->f = 0;
	printf("  wrote %s (%lu bytes)\n", path, (unsigned long)w->bytes);
}

static int sample_at(const SmkTrack *t, const uint8_t *p, uint32_t i)
{
	if (t->bits == 16) return ((const int16_t *)p)[i];
	return (int)p[i] - 128;
}

int main(int argc, char **argv)
{
	uint32_t n, count, i;
	uint8_t *arc;
	int ppm_movies = 4, wav_done = 0, movies = 0, bad = 0;
	char path[1024], wavpath[1024];

	if (argc < 3) {
		fprintf(stderr, "usage: test_smk <War Movies> <outdir> [ppm_movies]\n");
		return 2;
	}
	if (argc > 3) ppm_movies = atoi(argv[3]);
	arc = load_file(argv[1], &n);
	if (!arc || n < 8) { fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }
	if (le32(arc) != 0x19) { fprintf(stderr, "bad archive magic %08lx\n", (unsigned long)le32(arc)); return 1; }
	count = arc[4] | (arc[5] << 8);
	printf("archive: %lu bytes, %lu entries, id %u\n", (unsigned long)n, (unsigned long)count,
	       (unsigned)(arc[6] | (arc[7] << 8)));

	for (i = 0; i < count; ++i) {
		uint32_t off = le32(arc + 8 + 4 * i), word, size, flags;
		const uint8_t *d;
		SmkVideo v;
		clock_t c0, c1;
		int f, t, mid, ok_frames = 0, at_track = -1;
		long border_sum = 0, border_n = 0, inner_sum = 0, inner_n = 0;
		int prev_last[2] = { 0, 0 }, have_prev = 0;
		Wav wav;
		memset(&wav, 0, sizeof wav);

		if (off == 0 || off + 4 > n) continue;
		word = le32(arc + off);
		size = word & 0xFFFFFFu;
		flags = word >> 24;
		d = arc + off + 4;
		if (flags & 0x20) { printf("entry %3lu: compressed (flags %02lx) - skipped\n", (unsigned long)i, (unsigned long)flags); continue; }
		if (size < 4 || off + 4 + size > n) { printf("entry %3lu: bad size\n", (unsigned long)i); continue; }
		if (memcmp(d, "SMK2", 4) != 0) {
			printf("entry %3lu: %lu bytes, not SMK2 (%02x %02x %02x %02x)\n", (unsigned long)i,
			       (unsigned long)size, d[0], d[1], d[2], d[3]);
			continue;
		}
		if (smk_open(&v, d, size)) { printf("entry %3lu: smk_open FAILED\n", (unsigned long)i); ++bad; continue; }
		++movies;
		printf("entry %3lu: %ux%u flags %lx, %d frames (+%d), %lu us/frame, %lu bytes\n",
		       (unsigned long)i, v.w, v.h, (unsigned long)v.flags, v.frames, v.total - v.frames,
		       (unsigned long)v.frame_us, (unsigned long)size);
		for (t = 0; t < SMK_MAX_TRACKS; ++t)
			if (v.track[t].present) {
				printf("  track %d: %lu Hz, %d-bit, %s, %s\n", t, (unsigned long)v.track[t].rate,
				       v.track[t].bits, v.track[t].channels == 2 ? "stereo" : "mono",
				       v.track[t].compressed ? "huffman dpcm" : "raw pcm");
				if (at_track < 0) at_track = t;
			}
		if (at_track >= 0 && !wav_done) {
			snprintf(wavpath, sizeof wavpath, "%s/mov%02lu.wav", argv[2], (unsigned long)i);
			wav_open(&wav, wavpath, &v.track[at_track]);
		}
		mid = v.frames / 2;
		c0 = clock();
		for (f = 0; f < v.frames; ++f) {
			if (smk_next_frame(&v, SMK_DECODE_ALL)) { printf("  frame %d: decode FAILED\n", f); ++bad; break; }
			++ok_frames;
			if (at_track >= 0) {
				const SmkTrack *tr = &v.track[at_track];
				uint32_t bytes, ns, k;
				const uint8_t *a = smk_audio(&v, at_track, &bytes);
				int ch = tr->channels;
				if (a) {
					ns = bytes / (uint32_t)(tr->bits / 8);
					if (have_prev && ns >= (uint32_t)ch) {
						for (k = 0; k < (uint32_t)ch; ++k) {
							border_sum += labs((long)sample_at(tr, a, k) - prev_last[k]);
							++border_n;
						}
					}
					for (k = (uint32_t)ch; k < ns; ++k) {
						inner_sum += labs((long)sample_at(tr, a, k) - sample_at(tr, a, k - (uint32_t)ch));
						++inner_n;
					}
					if (ns >= (uint32_t)ch) {
						for (k = 0; k < (uint32_t)ch; ++k) prev_last[k] = sample_at(tr, a, ns - (uint32_t)ch + k);
						have_prev = 1;
					}
					if (wav.f) wav_add(&wav, a, bytes);
				}
			}
			if ((int)movies <= ppm_movies && (f == mid || (movies == 1 && (f == 0 || f == v.frames - 1)))) {
				snprintf(path, sizeof path, "%s/mov%02lu_f%04d.ppm", argv[2], (unsigned long)i, f);
				write_ppm(path, &v);
			}
		}
		c1 = clock();
		printf("  decoded %d/%d frames in %ld ms host, audio errors %lu",
		       ok_frames, v.frames, (long)((c1 - c0) * 1000 / CLOCKS_PER_SEC), v.errors);
		if (at_track >= 0)
			printf(", audio %lu bytes, mean step border %ld / inner %ld",
			       (unsigned long)v.track[at_track].total,
			       border_n ? border_sum / border_n : -1, inner_n ? inner_sum / inner_n : -1);
		printf("\n");
		if (v.errors) ++bad;
		if (wav.f) { wav_close(&wav, wavpath); wav_done = 1; }
		smk_close(&v);
	}
	printf("SUMMARY: %d movies decoded, %d problems\n", movies, bad);
	free(arc);
	return bad ? 1 : 0;
}
