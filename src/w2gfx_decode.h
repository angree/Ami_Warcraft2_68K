/*
 * w2gfx_decode.h - decoders for Warcraft II graphics entries (8-bit chunky).
 *
 * All functions take the decompressed entry bytes (from war_load) and never
 * allocate, except w2_tileset_build(). All multi-byte fields are little-endian
 * and read byte-wise (safe on 68k: no unaligned access, no endianness issues).
 * Formats: docs/DATA_FORMAT.md.
 */
#ifndef W2_GFX_DECODE_H
#define W2_GFX_DECODE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Clip rectangle in destination coordinates, x1/y1 exclusive. */
typedef struct {
    int x0, y0, x1, y1;
} W2Clip;

/* ---------------- sprites ("gfx": RLE frames, units/buildings/missiles/icons) -------
 * Header: u16 count, u16 box_w, u16 box_h, then count * 8-byte frame records:
 *   u8 x, u8 y, u8 w, u8 h, u32 offset (from entry start; bit 31 = w += 256).
 * RLE frame data: u16 row_offset[h] (relative to frame data start), each row:
 *   c & 0x80 : skip (c & 0x7F) transparent pixels
 *   c & 0x40 : (c & 0x3F) copies of the next byte
 *   else     : (c & 0x3F) literal bytes follow
 * "gfu" (uncompressed) variant: same header, frame data = w*h raw bytes.     */

#define W2_SPR_RLE   1
#define W2_SPR_RAW   2

typedef struct {
    int x, y, w, h;          /* frame rectangle inside the box_w x box_h box */
    unsigned long offset;    /* data offset from entry start */
} W2Frame;

/* Validate header; returns W2_SPR_RLE, W2_SPR_RAW or 0 (not a sprite).
 * box_w/box_h/count may be NULL. */
int w2_sprite_probe(const unsigned char *spr, unsigned long size,
                    int *count, int *box_w, int *box_h);

/* Frame record; returns 0 on success, -1 if out of range. */
int w2_sprite_frame(const unsigned char *spr, unsigned long size, int frame, W2Frame *f);

/* Draw flags */
#define W2_DRAW_FLIPX   1   /* mirror horizontally inside the box (W/SW/NW facings) */
#define W2_DRAW_OPAQUE  2   /* also write transparent pixels with colour 0 */

/* Draw one frame. (dx,dy) = destination of the TOP-LEFT of the frame box
 * (box_w x box_h; the unit is centred in it). remap: optional 256-byte colour
 * table applied to every opaque pixel (player colours 208..211), may be NULL.
 * clip may be NULL (no clipping - caller guarantees the box is inside dst).
 * type: W2_SPR_RLE or W2_SPR_RAW (from w2_sprite_probe).
 * Returns 0, or -1 on corrupt data (drawing stops at the bad row). */
int w2_sprite_draw(const unsigned char *spr, unsigned long size, int type, int frame,
                   unsigned char *dst, int pitch, int dx, int dy,
                   const W2Clip *clip, int flags, const unsigned char *remap);

/* ---------------- images (u16 w, u16 h, w*h bytes) ---------------- */
/* Returns pointer to pixels or NULL if not an image. */
const unsigned char *w2_image_probe(const unsigned char *img, unsigned long size, int *w, int *h);
/* Blit (opaque) with clipping. Returns 0 or -1. */
int w2_image_draw(const unsigned char *img, unsigned long size,
                  unsigned char *dst, int pitch, int dx, int dy, const W2Clip *clip);

/* ---------------- cursors (u16 hotx, hoty, w, h, w*h bytes; 0 = transparent) ---- */
const unsigned char *w2_cursor_probe(const unsigned char *cur, unsigned long size,
                                     int *hotx, int *hoty, int *w, int *h);

/* ---------------- fonts ("FONT " u8 last, u8 maxw, u8 maxh, u32 off[last-31]) --- */
/* Returns number of glyph slots (chars 32..32+n-1) or 0. */
int w2_font_probe(const unsigned char *fnt, unsigned long size, int *maxw, int *maxh);
/* Draw character ch with its top-left of the maxw x maxh cell at (dx,dy).
 * colors[0..7] maps glyph colour indices to palette indices (index 0 of the
 * glyph colour is never drawn). Returns glyph advance (glyph w), 0 if none,
 * -1 on error. */
int w2_font_draw(const unsigned char *fnt, unsigned long size, int ch,
                 unsigned char *dst, int pitch, int dx, int dy,
                 const W2Clip *clip, const unsigned char colors[8]);

/* ---------------- tilesets ---------------------------------------------------
 * mega  entry: n * 16 u16 (4x4 minitile refs per 32x32 megatile, row-major)
 *              ref = (minitile << 2) | (xflip << 1) | yflip
 * mini  entry: 8x8 = 64 byte minitiles
 * map   entry (tile map, 6636 bytes): 0x9E records of 42 bytes:
 *              u16 megatile[16]; u8 terrain[8]; u16 flags
 *   PUD MTXM tile value v (0x000..0x9DF) -> megatile = record[v >> 4].megatile[v & 15]
 */
#define W2_TILEMAP_GROUPS   0x9E
#define W2_TILEMAP_RECSIZE  42

int w2_tileset_count(unsigned long megasize);   /* number of megatiles */

/* Render one 32x32 megatile into dst (pitch bytes per row), no clipping. */
int w2_megatile_draw(const unsigned char *mega, unsigned long megasize,
                     const unsigned char *mini, unsigned long minisize,
                     int index, unsigned char *dst, int pitch);

/* Build all megatiles as consecutive 32x32 blocks (1024 bytes each).
 * Returns malloc'd buffer (free()), count in *n. ~380 KB for summer. */
unsigned char *w2_tileset_build(const unsigned char *mega, unsigned long megasize,
                                const unsigned char *mini, unsigned long minisize, int *n);

/* PUD tile value -> megatile index, -1 if out of range. */
int w2_tile_to_megatile(const unsigned char *tilemap, unsigned long size, unsigned int tile);

/* Tile map group flags (u16 at +40 in each 42-byte record). */
unsigned int w2_tile_flags(const unsigned char *tilemap, unsigned long size, unsigned int tile);

/* ---------------- PUD helper ---------------- */
/* Find a section ("MTXM", "UNIT", ...). Returns pointer to its data, length in *len. */
const unsigned char *w2_pud_section(const unsigned char *pud, unsigned long size,
                                    const char tag[4], unsigned long *len);

#ifdef __cplusplus
}
#endif

#endif /* W2_GFX_DECODE_H */
