/* assets.c - see assets.h. Plain C, no floats. Lores art comes from lores.c (halved once, cached). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assets.h"
#include "w2cfg.h"
#include "w2_tables.h"
#include "font35.h"
#include "amiga_gfx.h"
#include "lores.h"
#include "ehb.h"

static WarArchive *g_war[ASSET_COUNT];
static int g_era = -1;
static unsigned char g_pal[768];
static unsigned long g_palsum;   /* checksum of g_pal (keys the lores caches) */
static unsigned char *g_tilemap; static unsigned long g_tilemap_size;
static unsigned char *g_mega;    /* hires megatiles, 1024 bytes each (freed in lores once halved) */
static unsigned char *g_mega_lo; /* lores 16x16 = 256 bytes each */
static int g_mega_lo_owned;      /* 0: points into the lores pack */
static int g_mega_n;
static unsigned char g_base[256];
static unsigned char g_remap[8][256];
/* EHB (ehb.h): g_src = the source palette S, g_e = the 64 colours E, g_pal = the virtual palette */
static int g_ehb;
static unsigned char g_src[768], g_e[192], g_srcmap[256];
static unsigned char g_era_e[192]; static int g_era_e_for = -1;   /* E of an era (needs its megatiles) */
static void ehb_font_colours(void);
static const unsigned long *g_pic_hist;      /* EHB: pixel counts of the picture a palette is loaded for */

/* Mac names first, then PC names. AmigaDOS matches case-insensitively. */
static const char *const g_names[ASSET_COUNT][2] = {
	{ "War Data",      "MAINDAT.WAR" },
	{ "War Resources", "REZDAT.WAR" },
	{ "War Strings",   "STRDAT.WAR" },
	{ "War Sounds",    "SFXDAT.SUD" },
	{ "War Music",     "SNDDAT.WAR" },
	{ "War Movies",    "MUDDAT.CUD" },
};

static void images_flush(void);
static void mega_free(void);

int assets_lores(void) { return g_ehb || w2_cfg.resolution == W2_RES_LORES; }
int assets_ehb(void) { return g_ehb; }
void assets_set_ehb(int on) { g_ehb = on != 0; if (!g_ehb) lores_set_dst(NULL); }
/* the source palette the art is read through */
static const unsigned char *src_pal(void) { return g_ehb ? g_src : g_pal; }

static WarArchive *open_one(const char *dir, const char *name)
{
	char path[300];
	size_t n = strlen(dir);
	if (n && dir[n - 1] != ':' && dir[n - 1] != '/') snprintf(path, sizeof path, "%s/%s", dir, name);
	else snprintf(path, sizeof path, "%s%s", dir, name);
	return war_open(path);
}

int assets_init(void)
{
	int i, j;
	const char *dir = w2_cfg.data_dir[0] ? w2_cfg.data_dir : "PROGDIR:data";
	for (i = 0; i < ASSET_COUNT; i++) {
		for (j = 0; j < 2 && !g_war[i]; j++) g_war[i] = open_one(dir, g_names[i][j]);
		w2_log("assets: %s -> %s", g_names[i][0], g_war[i] ? "ok" : "MISSING");
	}
	if (!g_war[ASSET_DATA] || !g_war[ASSET_STRINGS]) {
		w2_log("assets: the game data was not found in %s", dir);
		return -1;
	}
	for (i = 0; i < 256; i++) g_base[i] = (unsigned char)i;
	return 0;
}

void assets_shutdown(void)
{
	int i;
	assets_flush_sprites();
	images_flush();
	mega_free();
	lores_era_end();
	war_free(g_tilemap); g_tilemap = NULL;
	g_era = -1;
	for (i = 0; i < ASSET_COUNT; i++) if (g_war[i]) { war_close(g_war[i]); g_war[i] = NULL; }
}

WarArchive *assets_war(int which) { return (which >= 0 && which < ASSET_COUNT) ? g_war[which] : NULL; }

unsigned char *assets_load(int which, int entry, unsigned long *size)
{
	if (size) *size = 0;
	if (!assets_war(which)) return NULL;
	return war_load(g_war[which], entry, size);
}

/* ---------------------------------------------------------------- palette & colours */

static int nearest(const unsigned char *pal, int r, int g, int b, int skip_lo, int skip_hi)
{
	int i, best = 0; long bd = 0x7fffffffL;
	for (i = 1; i < 256; i++) {
		long dr, dg, db, d;
		if (i >= skip_lo && i <= skip_hi) continue;
		dr = pal[i * 3] - r; dg = pal[i * 3 + 1] - g; db = pal[i * 3 + 2] - b;
		d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
		if (d < bd) { bd = d; best = i; }
	}
	return best;
}

static void build_remaps(void)
{
	int i, p, c;
	/* hardware pointer owns pens 17..19: game pixels using them go to the nearest other colour */
	for (i = 0; i < 256; i++) g_base[i] = (unsigned char)i;
	for (i = 17; i <= 19; i++) g_base[i] = (unsigned char)nearest(g_pal, g_pal[i * 3], g_pal[i * 3 + 1], g_pal[i * 3 + 2], 17, 19);
	for (p = 0; p < 8; p++) {
		memcpy(g_remap[p], g_base, 256);
		for (c = 0; c < 4; c++) {
			int to = (p == 7) ? 12 + c : 208 + p * 4 + c;
			g_remap[p][208 + c] = g_base[to];
		}
	}
}

static void ehb_log(const unsigned char *e192)
{
	int i;
	for (i = 0; i < 32; i += 8) {
		const unsigned char *e = e192 + i * 3;
		w2_log("assets: EHB reg %2d: %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x %02x%02x%02x", i,
		       e[0], e[1], e[2], e[3], e[4], e[5], e[6], e[7], e[8], e[9], e[10], e[11],
		       e[12], e[13], e[14], e[15], e[16], e[17], e[18], e[19], e[20], e[21], e[22], e[23]);
	}
}

/* EHB: everything that depends on S (g_src) and E (g_e). era_mode: E has the player registers. */
static void ehb_setup(int era_mode)
{
	int i, p, k, bf = ehb_bright_first(g_src);
	ehb_virtual(g_e, g_pal);
	for (i = 0; i < 256; i++) g_base[i] = (unsigned char)i;
	for (p = 0; p < 8; p++) memcpy(g_remap[p], g_base, 256);
	if (era_mode) {
		/* converted art carries the player colour as the placeholder pair; 208..211 stay mapped too
		 * (modules index a remap table with 209 for "this player's colour") */
		for (p = -1; p < 8; p++) {
			unsigned char *t = p < 0 ? g_base : g_remap[p];
			int reg = ehb_player_reg[p < 0 ? 0 : p];   /* no owner: the ramp as stored = player 0 (red) */
			t[EHB_PH] = (unsigned char)reg; t[EHB_PH + 32] = (unsigned char)(reg + 32);
			for (k = 0; k < 4; k++) t[208 + k] = (unsigned char)(((k < 2) == (bf != 0)) ? reg : reg + 32);
		}
	}
	for (i = 0; i < 256; i++) g_srcmap[i] = (unsigned char)ehb_nearest(g_e, g_src[i * 3], g_src[i * 3 + 1], g_src[i * 3 + 2], 0);
	if (era_mode) for (k = 0; k < 4; k++) g_srcmap[208 + k] = g_base[208 + k];
	ehb_font_colours();
	lores_set_dst(g_e);
}

/* era >= 0: this is that era's palette (EHB: use the era's own E when it is known) */
static void set_pal6(const unsigned char *p6, int era)
{
	int i; unsigned long s = 0x9E3779B9UL;
	unsigned char *pal = g_ehb ? g_src : g_pal;
	for (i = 0; i < 768; i++) { int v = p6[i] & 63; pal[i] = (unsigned char)((v << 2) | (v >> 4)); }
	for (i = 0; i < 768; i++) s = ((s << 5) | (s >> 27)) ^ pal[i];
	if (!g_ehb) {
		g_palsum = s ? s : 1;
		build_remaps();
		return;
	}
	if (era >= 0 && era == g_era_e_for) memcpy(g_e, g_era_e, 192);
	else {
		/* a picture palette: its E is cached (the menus switch between two or three palettes) */
		static unsigned char cs[3][768], ce[3][192]; static int cn, cnext;
		era = -1;
		for (i = 0; i < cn; i++) if (!memcmp(cs[i], g_src, 768)) break;
		if (i < cn) memcpy(g_e, ce[i], 192);
		else {
			ehb_build(g_src, g_pic_hist, NULL, 0, g_e);
			w2_log("assets: EHB picture palette (%s): mean error %lu after seeding, %lu final (%lu passes)",
			       g_pic_hist ? "weighted by its art" : "unweighted", ehb_err_init, ehb_err_final, ehb_evals);
			ehb_log(g_e);
			i = cnext; cnext = (cnext + 1) % 3; if (cn < 3) cn++;
			memcpy(cs[i], g_src, 768); memcpy(ce[i], g_e, 192);
		}
	}
	for (i = 0; i < 192; i++) s = ((s << 5) | (s >> 27)) ^ g_e[i];   /* art converted under another E is stale */
	g_palsum = s ? s : 1;
	ehb_setup(era >= 0);
}

const unsigned char *assets_palette(void) { return g_pal; }

void assets_apply_palette(void)
{
	if (!g_ehb) { amigagfx_set_palette(g_pal, 0, 256); return; }
	/* 32 registers; 32..63 are the hardware's halves. The pointer takes 17..19 from the same registers. */
	amigagfx_pointer_colours(g_e + 17 * 3);
	amigagfx_set_ehb_palette(g_e);
	amigagfx_set_palette(g_e, 0, 32);
}

void assets_load_palette(int which, int entry)
{
	unsigned long n; unsigned char *p = assets_load(which, entry, &n);
	if (p && n >= 768) set_pal6(p, (which == ASSET_DATA && g_era >= 0 && g_era <= 2 && entry == assets_era_entry(g_era, 0)) ? g_era : -1);
	war_free(p);
}

/* pixel counts of one piece of art (an image entry, or a raw sprite sheet without its pen 0) */
static void art_hist(int which, int entry, unsigned long *h)
{
	unsigned long n, k; int w, hh, cnt, i, j;
	unsigned char *d = assets_load(which, entry, &n);
	const unsigned char *px = d ? w2_image_probe(d, n, &w, &hh) : NULL;
	memset(h, 0, 256 * sizeof *h);
	if (px) for (k = 0; k < (unsigned long)w * hh; k++) h[px[k]]++;
	else if (d && w2_sprite_probe(d, n, &cnt, NULL, NULL) == W2_SPR_RAW) {
		for (i = 0; i < cnt; i++) {
			W2Frame f, g;
			if (w2_sprite_frame(d, n, i, &f) || f.offset + (unsigned long)f.w * f.h > n) continue;
			for (j = 0; j < i; j++) if (!w2_sprite_frame(d, n, j, &g) && g.offset == f.offset) break;
			if (j < i) continue;
			for (k = 0; k < (unsigned long)f.w * f.h; k++) if (d[f.offset + k]) h[d[f.offset + k]]++;
		}
	}
	war_free(d);
}

void assets_load_palette_for(int which, int entry, const short *art)
{
	static unsigned long hist[256], one[256];
	if (g_ehb && art) {
		int i, k;
		memset(hist, 0, sizeof hist);
		for (k = 0; art[k] >= 0 && art[k + 1] >= 0; k += 2) {
			unsigned long total = 0, unit;
			art_hist(art[k], art[k + 1], one);
			for (i = 0; i < 256; i++) total += one[i];
			unit = total / (k ? 1024 : 4096) + 1;       /* the picture 4096, every other piece 1024 */
			for (i = 0; i < 256; i++) hist[i] += one[i] / unit;
		}
		g_pic_hist = hist;
	}
	assets_load_palette(which, entry);
	g_pic_hist = NULL;
}

const unsigned char *assets_remap(int player) { return (player >= 0 && player < 8) ? g_remap[player] : g_base; }
const unsigned char *assets_base_remap(void) { return g_base; }
const unsigned char *assets_src_map(void) { return g_ehb ? g_srcmap : g_base; }

/* ---------------------------------------------------------------- terrain */

static const int g_era_entry[3][4] = { { 2, 3, 4, 5 }, { 18, 19, 20, 21 }, { 10, 11, 12, 13 } };

int assets_era(void) { return g_era; }
int assets_era_entry(int era, int what)
{
	if (era < 0 || era > 2 || what < 0 || what > 3) return -1;
	return g_era_entry[era][what];
}

void assets_restore_era_palette(void)
{
	if (g_era < 0 || g_era > 2) return;
	assets_load_palette(ASSET_DATA, g_era_entry[g_era][0]);
	assets_apply_palette();
}

static void mega_free(void)
{
	free(g_mega); g_mega = NULL;
	if (g_mega_lo_owned) free(g_mega_lo);
	g_mega_lo = NULL; g_mega_lo_owned = 0;
	g_mega_n = 0;
}

int assets_set_era(int era)
{
	unsigned long ms, ns, ps; unsigned char *mega, *mini, *pal; int i, n;
	if (era < 0 || era > 3) era = 0;
	if (era == 3) era = 2;                      /* swamp is expansion-only: wasteland */
	if (era == g_era && (assets_lores() ? g_mega_lo != NULL : g_mega != NULL)) {
		/* cached - but a menu may have loaded a picture palette since: the era's palette again */
		assets_restore_era_palette();
		return 0;
	}
	assets_flush_sprites();
	images_flush();
	mega_free();
	lores_era_end();
	war_free(g_tilemap); g_tilemap = NULL;
	g_era = -1;
	pal  = assets_load(ASSET_DATA, g_era_entry[era][0], &ps);
	mega = assets_load(ASSET_DATA, g_era_entry[era][1], &ms);
	mini = assets_load(ASSET_DATA, g_era_entry[era][2], &ns);
	g_tilemap = assets_load(ASSET_DATA, g_era_entry[era][3], &g_tilemap_size);
	if (!pal || ps < 768 || !mega || !mini || !g_tilemap) { war_free(pal); war_free(mega); war_free(mini); w2_log("assets: era %d missing", era); return -1; }
	g_mega = w2_tileset_build(mega, ms, mini, ns, &n);
	if (g_mega && g_ehb) {
		/* the era's EHB palette lives in its pack (built now if missing); without a pack: computed here */
		unsigned char pal8[768];
		for (i = 0; i < 768; i++) { int v = pal[i] & 63; pal8[i] = (unsigned char)((v << 2) | (v >> 4)); }
		if (!lores_era_begin(era, g_mega, n, pal8) && lores_era_ehb()) memcpy(g_era_e, lores_era_ehb(), 192);
		else lores_ehb_era_palette(era, pal8, g_mega, n, g_era_e);
		g_era_e_for = era;
		w2_log("assets: EHB palette of era %d", era);
		ehb_log(g_era_e);
	}
	set_pal6(pal, era);
	war_free(pal); war_free(mega); war_free(mini);
	if (!g_mega) return -1;
	g_mega_n = n;
	if (!g_ehb) for (i = 0; i < n * 1024; i++) g_mega[i] = g_base[g_mega[i]];   /* EHB: source indices are converted by the halving */
	if (assets_lores()) {
		/* halved art: from the era's pack (built once, see lores.c), else filtered here */
		if (!lores_era_begin(era, g_mega, n, src_pal())) g_mega_lo = (unsigned char *)lores_era_tiles();
		if (!g_mega_lo) {
			g_mega_lo = lores_tiles_halve(g_mega, n, lores_q_for(src_pal()));
			g_mega_lo_owned = g_mega_lo != NULL;
		}
		if (g_mega_lo) { free(g_mega); g_mega = NULL; }   /* hires tiles are not drawn in lores */
	}
	g_era = era;
	assets_apply_palette();
	w2_log("assets: era %d, %d megatiles%s", era, n, g_ehb ? " (EHB)" : assets_lores() ? " (lores)" : "");
	return 0;
}

int assets_tile_size(void) { return assets_lores() ? 16 : 32; }
int assets_megatile_count(void) { return g_mega_n; }

const unsigned char *assets_megatile(int m)
{
	if (m < 0 || m >= g_mega_n) return NULL;
	if (g_mega_lo && assets_lores()) return g_mega_lo + m * 256;
	return g_mega ? g_mega + m * 1024 : NULL;
}

int assets_tile_to_mega(unsigned tile) { return g_tilemap ? w2_tile_to_megatile(g_tilemap, g_tilemap_size, tile) : -1; }
unsigned assets_tile_group_flags(unsigned tile) { return g_tilemap ? w2_tile_flags(g_tilemap, g_tilemap_size, tile) : 0; }

/* ---------------------------------------------------------------- sprites */

#define SPR_CACHE 512
static struct {
	short which, entry;
	unsigned char owned;      /* data is ours (malloc/war_load), not the pack's */
	unsigned char lazy;       /* lores: halved here with the palette g_palsum had then */
	unsigned long palsum;
	W2Spr s;
} g_spr[SPR_CACHE];
static int g_spr_n;

void assets_flush_sprites(void)
{
	int i;
	for (i = 0; i < g_spr_n; i++) if (g_spr[i].owned) free(g_spr[i].s.data);   /* war_free == free */
	g_spr_n = 0;
}

/* fill s for (which, entry): hires as stored; lores from the pack, else halved now */
static void sprite_fetch(int i)
{
	W2Spr *s = &g_spr[i].s;
	int which = g_spr[i].which, entry = g_spr[i].entry, type = 0, bw = 0, bh = 0;
	unsigned long n;
	const unsigned char *pk;
	memset(s, 0, sizeof *s);
	g_spr[i].owned = 0; g_spr[i].lazy = 0; g_spr[i].palsum = g_palsum;
	if (assets_lores() && (pk = lores_era_sprite(which, entry, &n, &type, &bw, &bh)) != NULL) {
		int cnt;
		if (w2_sprite_probe(pk, n, &cnt, NULL, NULL)) {
			s->data = (unsigned char *)pk; s->size = n; s->type = type; s->count = cnt; s->box_w = bw; s->box_h = bh;
			return;
		}
	}
	s->data = assets_load(which, entry, &s->size);
	if (!s->data) return;
	s->type = w2_sprite_probe(s->data, s->size, &s->count, &s->box_w, &s->box_h);
	if (!s->type) { w2_log("assets: entry %d/%d is not a sprite", which, entry); war_free(s->data); s->data = NULL; return; }
	g_spr[i].owned = 1;
	if (assets_lores()) {
		unsigned long hn;
		unsigned char *h = lores_sprite_halve(s->data, s->size, s->type, lores_q_for(src_pal()), &hn);
		g_spr[i].lazy = 1;
		if (h) { war_free(s->data); s->data = h; s->size = hn; }
		else { w2_log("assets: could not halve %d/%d", which, entry); war_free(s->data); s->data = NULL; }
	}
}

const W2Spr *assets_sprite_any(int which, int entry)
{
	int i;
	if (entry < 0) return NULL;
	for (i = 0; i < g_spr_n; i++)
		if (g_spr[i].which == which && g_spr[i].entry == entry) {
			if (g_spr[i].lazy && g_spr[i].palsum != g_palsum) {   /* halved under another palette */
				if (g_spr[i].owned) free(g_spr[i].s.data);
				sprite_fetch(i);
			}
			return g_spr[i].s.data ? &g_spr[i].s : NULL;
		}
	if (g_spr_n >= SPR_CACHE) return NULL;
	i = g_spr_n++;
	g_spr[i].which = (short)which; g_spr[i].entry = (short)entry;
	sprite_fetch(i);
	return g_spr[i].s.data ? &g_spr[i].s : NULL;
}

const W2Spr *assets_sprite(int which, int entry)
{
	if (entry <= 0) return NULL;
	return assets_sprite_any(which, entry);
}

void assets_draw_sprite(const W2Spr *s, int frame, int flip, unsigned char *dst, int pitch,
                        int px, int py, const W2Clip *clip, const unsigned char *remap)
{
	if (!s) return;
	if (!remap) remap = g_base;
	if (assets_lores()) { px >>= 1; py >>= 1; }   /* the data is already halved */
	w2_sprite_draw(s->data, s->size, s->type, frame, dst, pitch, px, py, clip, flip ? W2_DRAW_FLIPX : 0, remap);
}

/* ---------------------------------------------------------------- images (cached) */

#define IMG_CACHE 24
#define IMG_BUDGET (1024UL * 1024UL)
static struct { short which, entry; unsigned long palsum, size, used; unsigned char *data; } g_img[IMG_CACHE];
static unsigned long g_img_bytes, g_img_clock;

static void images_flush(void)
{
	int i;
	for (i = 0; i < IMG_CACHE; i++) { free(g_img[i].data); g_img[i].data = NULL; }
	g_img_bytes = 0;
}

static void img_drop(int i) { g_img_bytes -= g_img[i].size; free(g_img[i].data); g_img[i].data = NULL; }

/* the image as drawn: hires as stored, lores halved under the current palette */
static const unsigned char *image_get(int which, int entry, unsigned long *size, unsigned char **tmp)
{
	int i, slot = -1, lo = assets_lores();
	unsigned long key = lo ? g_palsum : 0, n;
	unsigned char *img;
	*tmp = NULL;
	for (i = 0; i < IMG_CACHE; i++)
		if (g_img[i].data && g_img[i].which == which && g_img[i].entry == entry && g_img[i].palsum == key) {
			g_img[i].used = ++g_img_clock; *size = g_img[i].size; return g_img[i].data;
		}
	img = assets_load(which, entry, &n);
	if (!img || !w2_image_probe(img, n, NULL, NULL)) { war_free(img); return NULL; }
	if (lo) {
		unsigned long hn;
		unsigned char *h = lores_image_halve(img, n, lores_q_for(src_pal()), &hn);
		war_free(img);
		if (!h) return NULL;
		img = h; n = hn;
	}
	if (n > IMG_BUDGET / 2) { *tmp = img; *size = n; return img; }   /* too big to keep */
	while (g_img_bytes + n > IMG_BUDGET) {      /* evict the least recently used */
		int old = -1;
		for (i = 0; i < IMG_CACHE; i++) if (g_img[i].data && (old < 0 || g_img[i].used < g_img[old].used)) old = i;
		if (old < 0) break;
		img_drop(old);
	}
	for (i = 0; i < IMG_CACHE; i++) if (!g_img[i].data) { slot = i; break; }
	if (slot < 0) {
		for (i = 0; i < IMG_CACHE; i++) if (slot < 0 || g_img[i].used < g_img[slot].used) slot = i;
		img_drop(slot);
	}
	g_img[slot].which = (short)which; g_img[slot].entry = (short)entry; g_img[slot].palsum = key;
	g_img[slot].data = img; g_img[slot].size = n; g_img[slot].used = ++g_img_clock;
	g_img_bytes += n;
	*size = n;
	return img;
}

void assets_draw_image(int which, int entry, unsigned char *dst, int pitch, int px, int py, const W2Clip *clip)
{
	unsigned long n; unsigned char *tmp;
	const unsigned char *img = image_get(which, entry, &n, &tmp);
	if (!img) return;
	if (assets_lores()) { px >>= 1; py >>= 1; }
	w2_image_draw(img, n, dst, pitch, px, py, clip);
	free(tmp);
}

/* ---------------------------------------------------------------- strings */

#define STR_CACHE 32
static struct { int entry; unsigned char *data; unsigned long size; } g_str[STR_CACHE];

const char *assets_string(int entry, int idx)
{
	int i, slot = -1; unsigned n, off;
	for (i = 0; i < STR_CACHE; i++) if (g_str[i].data && g_str[i].entry == entry) { slot = i; break; }
	if (slot < 0) {
		for (i = 0; i < STR_CACHE && g_str[i].data; i++) ;
		if (i == STR_CACHE) { war_free(g_str[0].data); g_str[0].data = NULL; i = 0; }
		g_str[i].data = assets_load(ASSET_STRINGS, entry, &g_str[i].size);
		g_str[i].entry = entry;
		if (!g_str[i].data) return "";
		slot = i;
	}
	if (g_str[slot].size < 2) return "";
	n = W2_LE16(g_str[slot].data);
	if (idx < 0 || (unsigned)idx >= n || 2 + (unsigned long)idx * 2 + 2 > g_str[slot].size) return "";
	off = W2_LE16(g_str[slot].data + 2 + idx * 2);
	if (off >= g_str[slot].size) return "";
	return (const char *)g_str[slot].data + off;
}

const char *assets_unit_name(int type) { return assets_string(1, type + 1); }

/* ---------------------------------------------------------------- fonts */

static unsigned char *g_font[FONT_COUNT]; static unsigned long g_font_n[FONT_COUNT];
/* glyph shade -> palette index (wargus fonts.lua tables, NIEPEWNE vs the original) */
static const unsigned char g_fc[FC_COUNT][8] = {
	{ 246, 200, 199, 197, 192, 239, 104, 0 },  /* yellow */
	{ 0, 246, 246, 246, 104, 239, 0, 0 },      /* white  */
	{ 0, 111, 110, 109, 104, 239, 0, 0 },      /* grey   */
	{ 0, 208, 209, 210, 211, 239, 0, 0 },      /* red    */
	{ 0, 216, 216, 216, 216, 239, 0, 0 },      /* green  */
	{ 0, 1, 1, 1, 1, 239, 0, 0 },              /* blue   */
	{ 0, 228, 228, 228, 228, 239, 0, 0 },      /* black  */
};
static const unsigned char g_font35_col[FC_COUNT] = { 200, 246, 110, 208, 216, 1, 228 };
static unsigned char g_font35_ehb[FC_COUNT];

/* EHB: the 3x5 font's colours by RGB (taken once from the summer palette, War Data 2 - the font
 * indices mean the same colours in every game palette) -> nearest of the current 64 */
static void ehb_font_colours(void)
{
	static unsigned char rgb[FC_COUNT][3];
	static int have;
	int i;
	if (!have) {
		unsigned long n; unsigned char *p = assets_load(ASSET_DATA, 2, &n);
		static const unsigned char fb[FC_COUNT][3] = { {252,252,0}, {252,252,252}, {160,160,160}, {200,0,0}, {0,160,0}, {0,0,200}, {40,40,40} };
		for (i = 0; i < FC_COUNT; i++) {
			int c = g_font35_col[i], k;
			for (k = 0; k < 3; k++) {
				if (p && n >= 768) { int v = p[c * 3 + k] & 63; rgb[i][k] = (unsigned char)((v << 2) | (v >> 4)); }
				else rgb[i][k] = fb[i][k];
			}
		}
		war_free(p);
		have = 1;
	}
	for (i = 0; i < FC_COUNT; i++) g_font35_ehb[i] = (unsigned char)ehb_nearest(g_e, rgb[i][0], rgb[i][1], rgb[i][2], 0);
}

static const unsigned char *font_data(int font, unsigned long *n)
{
	if (font < 0 || font >= FONT_COUNT) return NULL;
	if (!g_font[font]) g_font[font] = assets_load(ASSET_DATA, 279 + font, &g_font_n[font]);
	*n = g_font_n[font];
	return g_font[font];
}

int assets_font_height(int font)
{
	unsigned long n; int mw, mh; const unsigned char *f;
	if (assets_lores()) return 12;   /* 3x5 font = 6 lores px = 12 hires px per line */
	f = font_data(font, &n);
	if (!f || !w2_font_probe(f, n, &mw, &mh)) return 10;
	return mh;
}

int assets_text_width(int font, const char *s)
{
	unsigned long n; int mw, mh, w = 0; const unsigned char *f;
	if (assets_lores()) return font35_width(s) * 2;
	f = font_data(font, &n);
	if (!f || !w2_font_probe(f, n, &mw, &mh)) return 0;
	for (; *s; s++) {
		int c = (unsigned char)*s, off;
		if (c < 32 || c > f[5]) continue;
		off = (int)W2_LE32(f + 8 + (c - 32) * 4);
		w += off ? f[off] + 1 : mw / 2;
	}
	return w;
}

int assets_text(int font, int colour, unsigned char *dst, int pitch, const W2Clip *clip, int px, int py, const char *s)
{
	unsigned long n; int mw, mh, x = px; const unsigned char *f;
	if (colour < 0 || colour >= FC_COUNT) colour = FC_YELLOW;
	if (assets_lores()) {
		int cw = clip ? clip->x1 : 320, ch = clip ? clip->y1 : 240;
		int end = font35_draw_shadow(dst, pitch, cw, ch, px >> 1, py >> 1, s, g_ehb ? g_font35_ehb[colour] : g_font35_col[colour], 0);
		return (end - (px >> 1)) * 2;
	}
	f = font_data(font, &n);
	if (!f || !w2_font_probe(f, n, &mw, &mh)) return 0;
	for (; *s; s++) {
		int c = (unsigned char)*s, a;
		if (c < 32) continue;
		a = w2_font_draw(f, n, c, dst, pitch, x, py, clip, g_fc[colour]);
		x += (a > 0) ? a + 1 : mw / 2;
	}
	return x - px;
}
