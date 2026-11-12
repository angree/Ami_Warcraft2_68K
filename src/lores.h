/* lores.h - the 320x240 mode's art: everything halved ONCE with a real filter, drawn 1:1.
 *
 * Method (AmiSC core/amisc_lores.h, docs/AMISC_METHODS.md section 1), plain C, integers only:
 *  - Lanczos-3 2:1 (one constant 12-tap weight vector, multiply table, ring of 12 rows), anti-ringing
 *    clamp to the range of the inputs, averaging in RGB, result quantised through a 15-bit RGB -> index
 *    table (built exactly with a pruned search, never answers a reserved index).
 *  - Transparency is a weight: an output pixel is opaque when >= 2 of its 2x2 source pixels are.
 *    Edge pixels (2x2 block not fully opaque) get the plain 2x2 average, never Lanczos.
 *  - Index classes: the player-colour ramp 208..211 is averaged by RAMP POSITION among itself only (so
 *    it keeps remapping per player); colour-cycled ranges of the era are halved by vote. The quantiser
 *    never produces 17..19 (hardware pointer), 208..211 or a cycled index from an RGB average.
 *  - Sprites are re-encoded into the SAME RLE/raw format w2_sprite_draw reads, so lores drawing is the
 *    normal sprite drawer on smaller data.
 *  - Per era a pack file <data_dir>/lores-<era>.pack (terrain 16x16 + every sprite of War Data) is
 *    built once (progress through w2_log or lores_set_progress) and loaded on later starts. The pack
 *    location can be overridden with the w2.cfg key "lores_pack_dir".
 *  - EHB (assets_ehb(), ehb.h): the same halving with a DIFFERENT DESTINATION palette. Source pixels
 *    are read through the 256-colour source palette, the result is quantised to the 64-colour EHB
 *    palette given with lores_set_dst() and every output index is 0..63; the player ramp becomes the
 *    placeholder pair EHB_PH / EHB_PH + 32. The packs are then <data_dir>/ehb-<era>.pack (same format,
 *    plus the era's 64 colours at the end: "EHBP" + 64 x RGB).
 * Used by assets.c only; other modules keep calling assets_*().
 */
#ifndef W2_LORES_H
#define W2_LORES_H

#define LORES_MAX_RAMPS 6

/* Quantiser + index classes for ONE palette (about 33 KB - never on the stack). */
typedef struct LoresQ {
	unsigned long sum;                        /* checksum of palette + classes (cache key) */
	unsigned char pal[768];                   /* 8-bit RGB */
	unsigned char cls[256];                   /* 0 = colour, k = ramp class k (1..nramps) */
	unsigned char lo[LORES_MAX_RAMPS + 1], hi[LORES_MAX_RAMPS + 1];
	unsigned char avg[LORES_MAX_RAMPS + 1];   /* 1: average ramp position (player colour), 0: vote */
	int nramps;
	unsigned char allow[256];                 /* 1 = an RGB average may land on this (destination) index */
	unsigned char lut[32768];                 /* 15-bit RGB -> destination index */
	int dst;                                  /* 1: destination palette != source (EHB) */
	unsigned char dpal[768];                  /* destination palette (== pal when !dst) */
	unsigned char omap[256];                  /* source index -> output index where an index is kept (identity when !dst) */
} LoresQ;

/* era 0..2: era palette with player-colour + colour-cycling classes; era < 0: a picture palette
 * (only 17..19 kept out of the answers). */
void lores_q_build(LoresQ *q, const unsigned char *pal768, int era);
/* The destination palette of every quantiser built from now on: 64 x RGB (ehb.h), copied; NULL = the
 * source palette itself (normal lores). Set by assets.c whenever the EHB palette changes. */
void lores_set_dst(const unsigned char *e192);
/* the stamp an EHB pack of this source palette carries (does not need the EHB palette itself) */
unsigned long lores_ehb_sum(const unsigned char *pal768, int era);
/* the quantiser for pal768 (era classes when it is the current era palette), cached; never NULL */
const LoresQ *lores_q_for(const unsigned char *pal768);

/* Halve w x h (pitch) -> ((w+1)/2) x ((h+1)/2). mask == NULL: solid image (edges repeated);
 * else mask[i] != 0 = opaque (outside = transparent) and dmask receives the output opacity. */
void lores_halve(const unsigned char *src, const unsigned char *mask, int w, int h, int pitch,
                 unsigned char *dst, unsigned char *dmask, int dpitch, const LoresQ *q);

/* Halved copies (malloc'd, free()) in the source's own format; NULL on error. */
unsigned char *lores_sprite_halve(const unsigned char *spr, unsigned long size, int type,
                                  const LoresQ *q, unsigned long *out_size);
unsigned char *lores_image_halve(const unsigned char *img, unsigned long size, const LoresQ *q,
                                 unsigned long *out_size);
unsigned char *lores_tiles_halve(const unsigned char *mega32, int n, const LoresQ *q);   /* n * 256 */

/* ---- per-era pack (used by assets_set_era / assets_sprite in lores) ---- */
/* Load (or build + save) the pack of 'era' for the given hires megatiles and 8-bit era palette.
 * 0 = ok. Afterwards lores_era_tiles/sprite answer from it until lores_era_end(). */
int  lores_era_begin(int era, const unsigned char *mega32, int n, const unsigned char *pal768);
void lores_era_end(void);
const unsigned char *lores_era_tiles(void);                                  /* n * 256 or NULL */
/* EHB: the era's 64-colour palette (64 x RGB) stored at the end of the loaded ehb pack, or NULL */
const unsigned char *lores_era_ehb(void);
/* EHB: E for an era from its megatiles and the pack's sprites (what the pack builder does) */
void lores_ehb_era_palette(int era, const unsigned char *pal768, const unsigned char *mega32, int n, unsigned char *e192);
/* halved sprite entry (its type W2_SPR_*, and the HIRES box of the source) or NULL */
const unsigned char *lores_era_sprite(int which, int entry, unsigned long *size, int *type, int *box_w, int *box_h);

/* progress of a pack build: done/total work units; NULL = log every 10% through w2_log */
void lores_set_progress(void (*cb)(int era, int done, int total));

/* statistics of the last build (host test / log) */
typedef struct LoresStats {
	long out_px, lanczos_px, edge_px, ramp_px, clear_px;   /* output pixels by path */
	long htaps, hcols;                                     /* horizontal taps evaluated (opaque colour), columns */
	long sprites, frames, bytes_in, bytes_out;            /* sprite entries in the pack */
	unsigned long ms;                                      /* build time (amigagfx_millis) */
} LoresStats;
extern LoresStats lores_stats;
void lores_stats_reset(void);

#endif
