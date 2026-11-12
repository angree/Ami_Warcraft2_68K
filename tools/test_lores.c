/* test_lores.c - host test of the lores art (src/lores.c + assets.c lores paths).
 * Built by tools/build_test_lores.sh (host gcc, every src .c except main_amiga.c/setup_amiga.c + host layer).
 *
 * 1. builds the three era packs from scratch (lores_prepare), reports time + filter statistics and an
 *    instruction-count estimate for a 68030; then a second lores_prepare (must be a header check only);
 * 2. maps 220 / 223 / 230 (summer / winter / wasteland): the same scene (terrain, units of several types
 *    and players, buildings, a UI panel image, icons, text) drawn in hires and in lores; PNG = hires
 *    reference box-downscaled | our lores, at 2x; plus a 4x crop of the units;
 * 3. a menu picture under its own palette (lazy image halving + cache), assets_restore_era_palette;
 * 4. checks: no pointer pens 17..19 in any lores frame, player colours 208..211 survive halving.
 * Env: W2_TEST_DATA (game data dir), W2_SHOTDIR (PNG dir). PNGs show copyrighted art: never in the repo. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include "w2.h"
#include "pud.h"
#include "assets.h"
#include "w2cfg.h"
#include "w2_tables.h"
#include "lores.h"

void w2_log(const char *fmt, ...)
{
	va_list a;
	va_start(a, fmt);
	vfprintf(stderr, fmt, a);
	va_end(a);
	fputc('\n', stderr);
}
unsigned long w2_tick_ms(void) { return 50; }
unsigned long w2_frame_count(void) { return 0; }

static int g_fail;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "CHECK FAILED %s:%d: %s\n", __FILE__, __LINE__, #c); g_fail++; } } while (0)

static long ms_now(void) { return (long)(clock() / (CLOCKS_PER_SEC / 1000)); }

/* ---------------------------------------------------------------- PNG (stored deflate) */
static unsigned long crc_tab[256];
static unsigned long crc(unsigned long c, const unsigned char *p, size_t n)
{
	size_t i; int k;
	if (!crc_tab[1]) for (i = 0; i < 256; i++) { unsigned long v = (unsigned long)i; for (k = 0; k < 8; k++) v = (v & 1) ? 0xEDB88320UL ^ (v >> 1) : v >> 1; crc_tab[i] = v; }
	c ^= 0xFFFFFFFFUL;
	for (i = 0; i < n; i++) c = crc_tab[(c ^ p[i]) & 0xFF] ^ (c >> 8);
	return c ^ 0xFFFFFFFFUL;
}
static void be32(unsigned char *p, unsigned long v) { p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16); p[2] = (unsigned char)(v >> 8); p[3] = (unsigned char)v; }
static void chunk(FILE *f, const char *t, const unsigned char *d, size_t n)
{
	unsigned char h[8]; unsigned long c;
	be32(h, (unsigned long)n); memcpy(h + 4, t, 4);
	fwrite(h, 1, 8, f); if (n) fwrite(d, 1, n, f);
	c = crc(crc(0, (const unsigned char *)t, 4), d, n);
	be32(h, c); fwrite(h, 1, 4, f);
}
static int png_rgb(const char *path, const unsigned char *rgb, int w, int h)
{
	size_t raw = (size_t)h * (w * 3 + 1), blocks = (raw + 65534) / 65535, zn = 2 + raw + blocks * 5 + 4, i, q = 0, o = 0;
	unsigned char *r = malloc(raw), *z = malloc(zn), ihdr[13];
	unsigned long a = 1, b = 0; int y;
	FILE *f = fopen(path, "wb");
	if (!f || !r || !z) { if (f) fclose(f); free(r); free(z); return -1; }
	for (y = 0; y < h; y++) { r[o++] = 0; memcpy(r + o, rgb + (size_t)y * w * 3, (size_t)w * 3); o += (size_t)w * 3; }
	z[q++] = 0x78; z[q++] = 0x01;
	for (i = 0; i < raw; i += 65535) {
		size_t n = raw - i > 65535 ? 65535 : raw - i;
		z[q++] = (unsigned char)(i + n >= raw); z[q++] = (unsigned char)n; z[q++] = (unsigned char)(n >> 8);
		z[q++] = (unsigned char)~n; z[q++] = (unsigned char)(~n >> 8);
		memcpy(z + q, r + i, n); q += n;
	}
	for (i = 0; i < raw; i++) { a = (a + r[i]) % 65521; b = (b + a) % 65521; }
	be32(z + q, (b << 16) | a); q += 4;
	fwrite("\x89PNG\r\n\x1a\n", 1, 8, f);
	be32(ihdr, (unsigned long)w); be32(ihdr + 4, (unsigned long)h); ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = ihdr[11] = ihdr[12] = 0;
	chunk(f, "IHDR", ihdr, 13); chunk(f, "IDAT", z, q); chunk(f, "IEND", NULL, 0);
	fclose(f); free(r); free(z);
	return 0;
}

static unsigned char g_hi[640 * 480], g_lo[320 * 240];
static unsigned char g_out[1280 * 960 * 3];
static char g_shots[256];

/* left: hires box-downscaled to 320x240; right: our lores; each scaled by z; crop (cx,cy,cw,ch) in lores px */
static void save_pair(const char *name, int cx, int cy, int cw, int ch, int z)
{
	const unsigned char *pal = assets_palette();
	int W = cw * 2 * z, H = ch * z, x, y;
	char path[400];
	if ((long)W * H * 3 > (long)sizeof g_out) return;
	for (y = 0; y < H; y++) for (x = 0; x < W; x++) {
		int lx = cx + (x % (cw * z)) / z, ly = cy + y / z, c;
		unsigned char *o = g_out + ((size_t)y * W + x) * 3;
		if (x < cw * z) {
			int k, s = 0;
			for (c = 0; c < 3; c++) {
				s = 0;
				for (k = 0; k < 4; k++) s += pal[g_hi[(ly * 2 + (k >> 1)) * 640 + lx * 2 + (k & 1)] * 3 + c];
				o[c] = (unsigned char)((s + 2) / 4);
			}
		} else {
			for (c = 0; c < 3; c++) o[c] = pal[g_lo[ly * 320 + lx] * 3 + c];
		}
	}
	snprintf(path, sizeof path, "%s/%s.png", g_shots, name);
	png_rgb(path, g_out, W, H);
	fprintf(stderr, "test_lores: wrote %s\n", path);
}

/* ---------------------------------------------------------------- the scene (hires coordinates) */
static const int k_units[] = { W2U_FOOTMAN, W2U_GRUNT, W2U_PEASANT, W2U_KNIGHT, W2U_OGRE, W2U_ARCHER,
                               0x0A, 0x0B, 0x09, W2U_DRAGON, 0x2A, W2U_BATTLESHIP, 0x21, 0x04, 0x05, 0x03 };
static const int k_bld[] = { W2U_TOWN_HALL, W2U_GREAT_HALL, 0x3C, W2U_FARM, W2U_GOLD_MINE, 0x41, 0x52 };

static void draw_spr(unsigned char *buf, int pitch, const W2Clip *cl, int gfx, int frame, int flip, int x, int y, int player)
{
	const W2Spr *s = assets_sprite(ASSET_DATA, gfx);
	if (!s) { fprintf(stderr, "no sprite %d\n", gfx); g_fail++; return; }
	if (frame >= s->count) frame = s->count - 1;
	assets_draw_sprite(s, frame, flip, buf, pitch, x, y, cl, assets_remap(player));
}

static void scene(unsigned char *buf, int pitch, int W, int H, int mx, int my)
{
	int ts = assets_tile_size(), tx, ty, i, era = assets_era(), y, x;
	W2Clip cl; cl.x0 = 0; cl.y0 = 0; cl.x1 = W; cl.y1 = H;
	memset(buf, 0, (size_t)pitch * H);
	for (ty = 0; ty < 15; ty++) for (tx = 0; tx < 20; tx++) {
		int mtx = mx + tx, mty = my + ty;
		const unsigned char *t;
		if (mtx >= w2.map.w || mty >= w2.map.h) continue;
		t = assets_megatile(w2.map.mega[mty * w2.map.w + mtx]);
		if (!t) continue;
		for (y = 0; y < ts; y++) memcpy(buf + (ty * ts + y) * pitch + tx * ts, t + y * ts, (size_t)ts);
	}
	/* buildings along the top */
	for (i = 0, x = 8; i < (int)(sizeof k_bld / sizeof k_bld[0]); i++) {
		const w2_unit_type *t = w2_unit(k_bld[i]);
		if (!t || !t->gfx[era]) continue;
		draw_spr(buf, pitch, &cl, t->gfx[era], 0, 0, x, 8, k_bld[i] == W2U_GOLD_MINE ? -1 : i % 8);
		x += t->tile_w * 32 + 4;
		if (x > 560) break;
	}
	/* units: 16 types, players 0..7, directions N..NW (5 stored + 3 mirrored) */
	for (i = 0; i < 16; i++) {
		const w2_unit_type *t = w2_unit(k_units[i]);
		int dir = i & 7, mirror = 0, f;
		if (!t || !t->gfx[era]) continue;
		f = (int)w2_dir_frame(0, (unsigned)dir, &mirror);
		draw_spr(buf, pitch, &cl, t->gfx[era], f, mirror, 8 + (i % 8) * 76, 150 + (i / 8) * 80, i % 8);
	}
	/* a UI panel image with icons on it, and text */
	assets_draw_image(ASSET_DATA, 297, buf, pitch, 448, 320, &cl);
	{
		const W2Spr *ic = assets_sprite(ASSET_DATA, era == 1 ? 357 : era == 2 ? 358 : 356);
		CHECK(ic != NULL);
		for (i = 0; ic && i < 9; i++)
			assets_draw_sprite(ic, i * 7, 0, buf, pitch, 456 + (i % 3) * 54, 328 + (i / 3) * 44, &cl, assets_remap(i % 8));
	}
	assets_draw_image(ASSET_DATA, 291, buf, pitch, 0, 464, &cl);
	assets_text(FONT_GAME, FC_YELLOW, buf, pitch, &cl, 8, 320, "Footman: Ready for battle!");
	assets_text(FONT_SMALL, FC_WHITE, buf, pitch, &cl, 8, 340, "Gold 1200  Wood 800  Oil 500");
}

static int count_pens(const unsigned char *b, long n, int lo, int hi)
{
	long i; int c = 0;
	for (i = 0; i < n; i++) if (b[i] >= lo && b[i] <= hi) c++;
	return c;
}

static long file_size(const char *path)
{
	FILE *f = fopen(path, "rb"); long n;
	if (!f) return -1;
	fseek(f, 0, SEEK_END); n = ftell(f); fclose(f);
	return n;
}

static long g_era_t0[3], g_era_ms[3];
static void prog(int era, int done, int total)
{
	if (done == 0) g_era_t0[era] = ms_now();
	if (done == total) g_era_ms[era] = ms_now() - g_era_t0[era];
}

/* pixels of every frame of every pack sprite: no 17..19; player colour present where the source had it */
static void check_pack_sprites(int era)
{
	int e, bad = 0, team_src = 0, team_lo = 0, n = 0;
	for (e = 33; e <= 358; e++) {
		unsigned long sz, hs; int type, bw, bh, cnt, f;
		const unsigned char *h = lores_era_sprite(ASSET_DATA, e, &hs, &type, &bw, &bh);
		unsigned char *src;
		if (!h) continue;
		n++;
		CHECK(w2_sprite_probe(h, hs, &cnt, NULL, NULL) != 0);
		/* draw every frame onto a cleared canvas and look at the pens */
		for (f = 0; f < cnt; f++) {
			static unsigned char cv[160 * 160];
			W2Clip c; c.x0 = 0; c.y0 = 0; c.x1 = 160; c.y1 = 160;
			memset(cv, 0, sizeof cv);
			if (w2_sprite_draw(h, hs, type, f, cv, 160, 0, 0, &c, 0, NULL) < 0) { bad++; continue; }
			if (count_pens(cv, sizeof cv, 17, 19)) bad++;
			team_lo += count_pens(cv, sizeof cv, 208, 211) > 0;
		}
		src = assets_load(ASSET_DATA, e, &sz);
		if (src) {
			int t2 = w2_sprite_probe(src, sz, &cnt, NULL, NULL);
			for (f = 0; t2 && f < cnt; f++) {
				static unsigned char cv[320 * 320];
				W2Clip c; c.x0 = 0; c.y0 = 0; c.x1 = 320; c.y1 = 320;
				memset(cv, 0, sizeof cv);
				w2_sprite_draw(src, sz, t2, f, cv, 320, 0, 0, &c, 0, NULL);
				team_src += count_pens(cv, sizeof cv, 208, 211) > 3;
			}
			war_free(src);
		}
	}
	fprintf(stderr, "test_lores: era %d pack: %d sprites, frames with pointer pens: %d, frames with player colour: hires %d lores %d\n",
	        era, n, bad, team_src, team_lo);
	CHECK(bad == 0);
	CHECK(team_lo >= team_src * 9 / 10);
}

int main(void)
{
	const char *data = getenv("W2_TEST_DATA"), *shots = getenv("W2_SHOTDIR");
	int maps[3] = { 0, 0, 0 };   /* the first built-in skirmish map of each era */
	long t; int m, era;
	char path[300];

	w2cfg_defaults(&w2_cfg);
	w2cfg_load(&w2_cfg, "PROGDIR:w2.cfg");          /* lores_pack_dir=PROGDIR:packs */
	snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", data ? data : "PROGDIR:data");
	snprintf(g_shots, sizeof g_shots, "%s", shots ? shots : ".");
	w2_cfg.resolution = W2_RES_LORES;
	if (assets_init()) { fprintf(stderr, "assets_init failed\n"); return 1; }

	/* ---- 1. build all packs ---- */
	lores_set_progress(prog);
	lores_stats_reset();
	t = ms_now();
	CHECK(lores_prepare() == 0);
	t = ms_now() - t;
	fprintf(stderr, "test_lores: lores_prepare (build) %ld ms: summer %ld, winter %ld, wasteland %ld ms\n", t, g_era_ms[0], g_era_ms[1], g_era_ms[2]);
	fprintf(stderr, "test_lores: out px %ld (lanczos %ld, edge/flat %ld, ramp %ld, clear %ld), htaps %ld, hcols %ld\n",
	        lores_stats.out_px, lores_stats.lanczos_px, lores_stats.edge_px, lores_stats.ramp_px, lores_stats.clear_px,
	        lores_stats.htaps, lores_stats.hcols);
	fprintf(stderr, "test_lores: sprites %ld entries, %ld frames, %ld -> %ld bytes\n",
	        lores_stats.sprites, lores_stats.frames, lores_stats.bytes_in, lores_stats.bytes_out);
	{
		/* 68030 estimate in cycles: per opaque horizontal tap 40, per horizontal column 60, per Lanczos
		 * pixel 2000 (6 x 4 MULS.L + 24 min/max + 3 DIVS.L + quantiser), per edge/flat pixel 250, per clear/ramp
		 * pixel 120, + decode/encode ~30 per source pixel (4 per output pixel) */
		long long cyc = (long long)lores_stats.htaps * 40 + (long long)lores_stats.hcols * 60 + (long long)lores_stats.lanczos_px * 2000 +
		                (long long)lores_stats.edge_px * 250 + (long long)(lores_stats.clear_px + lores_stats.ramp_px) * 120 +
		                (long long)lores_stats.out_px * 4 * 30;
		fprintf(stderr, "test_lores: 68030 estimate %lld Mcycles = %lld s at 50 MHz (3 eras)\n", cyc / 1000000, cyc / 50000000);
	}
	lores_set_progress(NULL);
	t = ms_now();
	CHECK(lores_prepare() == 0);
	fprintf(stderr, "test_lores: lores_prepare (packs present) %ld ms\n", ms_now() - t);
	for (era = 0; era < 3; era++) {
		static const char *nm[3] = { "summer", "winter", "wasteland" };
		snprintf(path, sizeof path, "PROGDIR:packs/lores-%s.pack", nm[era]);
		fprintf(stderr, "test_lores: %s = %ld bytes\n", path, file_size(path));
	}

	/* ---- 2. scenes ---- */
	w2_cfg.resolution = W2_RES_HIRES;
	for (m = 220; m <= 247; m++) if (!pud_load_builtin(m) && w2.map.era >= 0 && w2.map.era < 3 && !maps[w2.map.era]) maps[w2.map.era] = m;
	fprintf(stderr, "test_lores: maps summer %d winter %d wasteland %d\n", maps[0], maps[1], maps[2]);
	for (m = 0; m < 3; m++) {
		int mx, my; char name[64];
		w2_cfg.resolution = W2_RES_LORES;
		t = ms_now();
		if (pud_load_builtin(maps[m])) { fprintf(stderr, "pud %d failed\n", maps[m]); g_fail++; continue; }
		fprintf(stderr, "test_lores: map %d era %d loaded in lores (pack load) %ld ms\n", maps[m], w2.map.era, ms_now() - t);
		era = w2.map.era;
		check_pack_sprites(era);
		mx = w2.map.w / 2 - 10; my = w2.map.h / 2 - 7;
		for (era = 0; era < 8; era++) if (w2.player[era].start_tx < w2.map.w) { mx = w2.player[era].start_tx - 10; my = w2.player[era].start_ty - 7; break; }
		era = w2.map.era;
		if (mx < 0) mx = 0;
		if (my < 0) my = 0;
		t = ms_now();
		scene(g_lo, 320, 320, 240, mx, my);
		fprintf(stderr, "test_lores: lores scene (first, lazy entries included) %ld ms\n", ms_now() - t);
		t = ms_now();
		scene(g_lo, 320, 320, 240, mx, my);
		fprintf(stderr, "test_lores: lores scene (cached) %ld ms\n", ms_now() - t);
		CHECK(count_pens(g_lo, sizeof g_lo, 17, 19) == 0);
		w2_cfg.resolution = W2_RES_HIRES;
		CHECK(assets_set_era(era) == 0);
		scene(g_hi, 640, 640, 480, mx, my);
		snprintf(name, sizeof name, "map%d_pair", maps[m]);
		save_pair(name, 0, 0, 320, 240, 2);
		snprintf(name, sizeof name, "map%d_units4x", maps[m]);
		save_pair(name, 0, 70, 160, 90, 4);
		snprintf(name, sizeof name, "map%d_units4x_b", maps[m]);
		save_pair(name, 160, 70, 160, 90, 4);
		snprintf(name, sizeof name, "map%d_ui4x", maps[m]);
		save_pair(name, 160, 150, 160, 90, 4);
		snprintf(name, sizeof name, "map%d_bld4x", maps[m]);
		save_pair(name, 0, 0, 160, 75, 4);
	}

	/* ---- 3. menu picture under its own palette ---- */
	{
		W2Clip c;
		w2_cfg.resolution = W2_RES_LORES;
		CHECK(assets_set_era(0) == 0);
		assets_load_palette(ASSET_RESOURCES, 14);
		c.x0 = 0; c.y0 = 0; c.x1 = 320; c.y1 = 240;
		t = ms_now();
		assets_draw_image(ASSET_RESOURCES, 13, g_lo, 320, 0, 0, &c);
		fprintf(stderr, "test_lores: menu picture 640x480 halved in %ld ms (incl. quantiser table)\n", ms_now() - t);
		t = ms_now();
		assets_draw_image(ASSET_RESOURCES, 13, g_lo, 320, 0, 0, &c);
		fprintf(stderr, "test_lores: menu picture again (cached) %ld ms\n", ms_now() - t);
		{
			const W2Spr *wd = assets_sprite_any(ASSET_RESOURCES, 0);
			CHECK(wd != NULL);
			if (wd) assets_draw_sprite(wd, 3, 0, g_lo, 320, 40, 40, &c, NULL);
		}
		CHECK(count_pens(g_lo, sizeof g_lo, 17, 19) == 0);
		w2_cfg.resolution = W2_RES_HIRES;
		c.x1 = 640; c.y1 = 480;
		assets_set_era(0);
		assets_load_palette(ASSET_RESOURCES, 14);
		assets_draw_image(ASSET_RESOURCES, 13, g_hi, 640, 0, 0, &c);
		{
			const W2Spr *wd = assets_sprite_any(ASSET_RESOURCES, 0);
			if (wd) assets_draw_sprite(wd, 3, 0, g_hi, 640, 40, 40, &c, NULL);
		}
		save_pair("menu_pair", 0, 0, 320, 240, 2);
		save_pair("menu4x", 80, 60, 160, 90, 4);
		/* restore the era palette */
		{
			unsigned long n; unsigned char *p6 = assets_load(ASSET_DATA, 2, &n);
			assets_restore_era_palette();
			CHECK(p6 && assets_palette()[300] == (unsigned char)(((p6[300] & 63) << 2) | ((p6[300] & 63) >> 4)));
			war_free(p6);
		}
	}
	/* ---- strings: table sizes (bound check for assets_string) ---- */
	{
		static const int ents[] = { 1, 54, 65, 93 };
		int i;
		for (i = 0; i < 4; i++) {
			unsigned long n; unsigned char *s = assets_load(ASSET_STRINGS, ents[i], &n);
			if (s && n >= 4) {
				unsigned cnt = W2_LE16(s), last = W2_LE16(s + 2 + (cnt - 1) * 2);
				fprintf(stderr, "test_lores: strings %d: n=%u size=%lu off[0]=%u off[n-1]=%u '%.30s' first='%.20s'\n", ents[i], cnt, n,
				        W2_LE16(s + 2), last, last < n ? (const char *)s + last : "(out)", (const char *)s + W2_LE16(s + 2));
			}
			war_free(s);
		}
		fprintf(stderr, "test_lores: assets_string(1,1)='%s' (65,0)='%.30s'\n", assets_string(1, 1), assets_string(65, 0));
	}
	assets_shutdown();
	fprintf(stderr, "test_lores: %s (%d failures)\n", g_fail ? "FAILED" : "OK", g_fail);
	return g_fail ? 1 : 0;
}
