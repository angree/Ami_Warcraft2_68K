/*
 * w2gfx_decode.c - Warcraft II graphics decoders. See w2gfx_decode.h.
 * Plain C (gnu99), no floats, no allocation except w2_tileset_build().
 */
#include <stdlib.h>
#include <string.h>

#include "war.h"
#include "w2gfx_decode.h"

/* ------------------------------------------------------------------------- */
/* sprites                                                                   */
/* ------------------------------------------------------------------------- */

static int read_frame(const unsigned char *spr, int frame, W2Frame *f)
{
    const unsigned char *p = spr + 6 + 8 * frame;
    unsigned long off = W2_LE32(p + 4);
    f->x = p[0];
    f->y = p[1];
    f->w = p[2];
    f->h = p[3];
    if (off & 0x80000000UL) {
        f->w += 256;
        off &= 0x7FFFFFFFUL;
    }
    f->offset = off;
    return 0;
}

int w2_sprite_probe(const unsigned char *spr, unsigned long size,
                    int *count, int *box_w, int *box_h)
{
    int n, bw, bh, i, rle_ok = 1, raw_ok = 1, nonempty = 0;

    if (!spr || size < 6)
        return 0;
    n = (int)W2_LE16(spr);
    bw = (int)W2_LE16(spr + 2);
    bh = (int)W2_LE16(spr + 4);
    if (n <= 0 || bw <= 0 || bh <= 0 || 6UL + 8UL * (unsigned long)n > size)
        return 0;
    for (i = 0; i < n; i++) {
        W2Frame f;
        read_frame(spr, i, &f);
        if (f.x + f.w > bw || f.y + f.h > bh || f.offset > size)
            return 0;
        if (f.w == 0 || f.h == 0)
            continue;
        nonempty++;
        if (f.offset + 2UL * (unsigned long)f.h > size ||
            W2_LE16(spr + f.offset) != (unsigned int)(2 * f.h))
            rle_ok = 0;
        if (f.offset + (unsigned long)f.w * (unsigned long)f.h > size)
            raw_ok = 0;
    }
    if (count)
        *count = n;
    if (box_w)
        *box_w = bw;
    if (box_h)
        *box_h = bh;
    if (nonempty && rle_ok)
        return W2_SPR_RLE;
    if (raw_ok)
        return W2_SPR_RAW;
    return 0;
}

int w2_sprite_frame(const unsigned char *spr, unsigned long size, int frame, W2Frame *f)
{
    int n;
    if (!spr || size < 6)
        return -1;
    n = (int)W2_LE16(spr);
    if (frame < 0 || frame >= n || 6UL + 8UL * (unsigned long)n > size)
        return -1;
    return read_frame(spr, frame, f);
}

/* write n pixels of a run (src != NULL: literal bytes, else value v) to
 * columns [c, c+n) of the frame row, honouring the visible column window
 * [cmin, cmax). */
static void put_run(unsigned char *row, int base, int flip, int c, int n,
                    const unsigned char *src, int v, int cmin, int cmax,
                    const unsigned char *remap)
{
    int a = c, b = c + n, k;
    if (a < cmin)
        a = cmin;
    if (b > cmax)
        b = cmax;
    if (a >= b)
        return;
    if (!flip) {
        unsigned char *d = row + base + a;
        if (src) {
            src += a - c;
            if (remap)
                for (k = a; k < b; k++)
                    *d++ = remap[*src++];
            else
                memcpy(d, src, (size_t)(b - a));
        } else {
            memset(d, remap ? remap[v] : v, (size_t)(b - a));
        }
    } else {
        unsigned char *d = row + base - a;     /* base = x of column 0, mirrored */
        if (src) {
            src += a - c;
            for (k = a; k < b; k++)
                *d-- = remap ? remap[*src++] : *src++;
        } else {
            unsigned char px = (unsigned char)(remap ? remap[v] : v);
            for (k = a; k < b; k++)
                *d-- = px;
        }
    }
}

int w2_sprite_draw(const unsigned char *spr, unsigned long size, int type, int frame,
                   unsigned char *dst, int pitch, int dx, int dy,
                   const W2Clip *clip, int flags, const unsigned char *remap)
{
    W2Frame f;
    int bw, r, flip = flags & W2_DRAW_FLIPX, base, cmin, cmax;

    if (w2_sprite_frame(spr, size, frame, &f) < 0)
        return -1;
    if (f.w == 0 || f.h == 0)
        return 0;
    bw = (int)W2_LE16(spr + 2);
    /* destination x of frame column c:  !flip: base + c   flip: base - c */
    if (!flip)
        base = dx + f.x;
    else
        base = dx + bw - 1 - f.x;
    cmin = 0;
    cmax = f.w;
    if (clip) {
        if (!flip) {
            if (clip->x0 - base > cmin) cmin = clip->x0 - base;
            if (clip->x1 - base < cmax) cmax = clip->x1 - base;
        } else {
            if (base - clip->x1 + 1 > cmin) cmin = base - clip->x1 + 1;
            if (base - clip->x0 + 1 < cmax) cmax = base - clip->x0 + 1;
        }
        if (cmin >= cmax)
            return 0;
    }
    for (r = 0; r < f.h; r++) {
        int oy = dy + f.y + r;
        unsigned char *row;
        if (clip && (oy < clip->y0 || oy >= clip->y1))
            continue;
        row = dst + (long)oy * pitch;
        if (type == W2_SPR_RAW) {
            unsigned long p = f.offset + (unsigned long)r * f.w;
            if (p + f.w > size)
                return -1;
            put_run(row, base, flip, 0, f.w, spr + p, 0, cmin, cmax, remap);
        } else {
            unsigned long p, ro = f.offset + 2UL * r;
            int col = 0;
            if (ro + 2 > size)
                return -1;
            p = f.offset + W2_LE16(spr + ro);
            while (col < f.w) {
                int c, n;
                if (p >= size)
                    return -1;
                c = spr[p++];
                if (c & 0x80) {
                    n = c & 0x7F;
                    if (n > f.w - col)
                        n = f.w - col;
                    if (flags & W2_DRAW_OPAQUE)
                        put_run(row, base, flip, col, n, NULL, 0, cmin, cmax, NULL);
                } else if (c & 0x40) {
                    n = c & 0x3F;
                    if (p >= size)
                        return -1;
                    if (n > f.w - col)
                        n = f.w - col;
                    put_run(row, base, flip, col, n, NULL, spr[p], cmin, cmax, remap);
                    p++;
                } else {
                    n = c & 0x3F;
                    if (p + (unsigned long)n > size)
                        return -1;
                    if (n > f.w - col) {
                        put_run(row, base, flip, col, f.w - col, spr + p, 0, cmin, cmax, remap);
                        p += n;
                        n = f.w - col;
                    } else {
                        put_run(row, base, flip, col, n, spr + p, 0, cmin, cmax, remap);
                        p += n;
                    }
                }
                if (n == 0)        /* corrupt: zero-length run would loop forever */
                    return -1;
                col += n;
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------------------- */
/* images / cursors                                                          */
/* ------------------------------------------------------------------------- */

const unsigned char *w2_image_probe(const unsigned char *img, unsigned long size, int *w, int *h)
{
    unsigned long iw, ih;
    if (!img || size < 4)
        return NULL;
    iw = W2_LE16(img);
    ih = W2_LE16(img + 2);
    if (!iw || !ih || 4UL + iw * ih != size)
        return NULL;
    if (w)
        *w = (int)iw;
    if (h)
        *h = (int)ih;
    return img + 4;
}

int w2_image_draw(const unsigned char *img, unsigned long size,
                  unsigned char *dst, int pitch, int dx, int dy, const W2Clip *clip)
{
    int w, h, x0 = 0, y0 = 0, x1, y1, y;
    const unsigned char *px = w2_image_probe(img, size, &w, &h);
    if (!px)
        return -1;
    x1 = w;
    y1 = h;
    if (clip) {
        if (clip->x0 - dx > x0) x0 = clip->x0 - dx;
        if (clip->y0 - dy > y0) y0 = clip->y0 - dy;
        if (clip->x1 - dx < x1) x1 = clip->x1 - dx;
        if (clip->y1 - dy < y1) y1 = clip->y1 - dy;
    }
    for (y = y0; y < y1 && x0 < x1; y++)
        memcpy(dst + (long)(dy + y) * pitch + dx + x0, px + (long)y * w + x0, (size_t)(x1 - x0));
    return 0;
}

const unsigned char *w2_cursor_probe(const unsigned char *cur, unsigned long size,
                                     int *hotx, int *hoty, int *w, int *h)
{
    unsigned long cw, ch;
    if (!cur || size < 8)
        return NULL;
    cw = W2_LE16(cur + 4);
    ch = W2_LE16(cur + 6);
    if (!cw || !ch || 8UL + cw * ch != size)
        return NULL;
    if (hotx) *hotx = (int)W2_LE16(cur);
    if (hoty) *hoty = (int)W2_LE16(cur + 2);
    if (w) *w = (int)cw;
    if (h) *h = (int)ch;
    return cur + 8;
}

/* ------------------------------------------------------------------------- */
/* fonts                                                                     */
/* ------------------------------------------------------------------------- */

int w2_font_probe(const unsigned char *fnt, unsigned long size, int *maxw, int *maxh)
{
    int n;
    if (!fnt || size < 8 || memcmp(fnt, "FONT ", 5) != 0 || fnt[5] < 32)
        return 0;
    n = fnt[5] - 32 + 1;
    if (8UL + 4UL * n > size)
        return 0;
    if (maxw) *maxw = fnt[6];
    if (maxh) *maxh = fnt[7];
    return n;
}

int w2_font_draw(const unsigned char *fnt, unsigned long size, int ch,
                 unsigned char *dst, int pitch, int dx, int dy,
                 const W2Clip *clip, const unsigned char colors[8])
{
    int n = w2_font_probe(fnt, size, NULL, NULL);
    unsigned long o, p;
    int w, h, xo, yo, pos, total;

    if (!n)
        return -1;
    if (ch < 32 || ch >= 32 + n)
        return 0;
    o = W2_LE32(fnt + 8 + 4 * (ch - 32));
    if (!o)
        return 0;
    if (o + 4 > size)
        return -1;
    w = fnt[o];
    h = fnt[o + 1];
    xo = fnt[o + 2];
    yo = fnt[o + 3];
    p = o + 4;
    total = w * h;
    pos = 0;
    while (pos < total) {
        int c, x, y;
        if (p >= size)
            return -1;
        c = fnt[p++];
        pos += c >> 3;
        if (pos >= total)
            break;
        x = dx + xo + pos % w;
        y = dy + yo + pos / w;
        if ((c & 7) && (!clip || (x >= clip->x0 && x < clip->x1 && y >= clip->y0 && y < clip->y1)))
            dst[(long)y * pitch + x] = colors[c & 7];
        pos++;
    }
    return w;
}

/* ------------------------------------------------------------------------- */
/* tilesets                                                                  */
/* ------------------------------------------------------------------------- */

int w2_tileset_count(unsigned long megasize)
{
    return (int)(megasize / 32);
}

int w2_megatile_draw(const unsigned char *mega, unsigned long megasize,
                     const unsigned char *mini, unsigned long minisize,
                     int index, unsigned char *dst, int pitch)
{
    int m;
    if (index < 0 || (unsigned long)index >= megasize / 32)
        return -1;
    for (m = 0; m < 16; m++) {
        unsigned int ref = W2_LE16(mega + 32UL * index + 2 * m);
        unsigned long src = (unsigned long)(ref >> 2) * 64;
        unsigned char *d = dst + (long)(m >> 2) * 8 * pitch + (m & 3) * 8;
        int xf = ref & 2, yf = ref & 1, y, x;
        if (src + 64 > minisize)
            return -1;
        for (y = 0; y < 8; y++) {
            const unsigned char *s = mini + src + (yf ? 7 - y : y) * 8;
            unsigned char *dd = d + (long)y * pitch;
            if (!xf)
                memcpy(dd, s, 8);
            else
                for (x = 0; x < 8; x++)
                    dd[x] = s[7 - x];
        }
    }
    return 0;
}

unsigned char *w2_tileset_build(const unsigned char *mega, unsigned long megasize,
                                const unsigned char *mini, unsigned long minisize, int *n)
{
    int cnt = w2_tileset_count(megasize), i;
    unsigned char *out;
    if (n)
        *n = 0;
    if (cnt <= 0)
        return NULL;
    out = (unsigned char *)malloc((size_t)cnt * 1024);
    if (!out)
        return NULL;
    for (i = 0; i < cnt; i++)
        if (w2_megatile_draw(mega, megasize, mini, minisize, i, out + 1024L * i, 32) < 0)
            memset(out + 1024L * i, 0, 1024);
    if (n)
        *n = cnt;
    return out;
}

int w2_tile_to_megatile(const unsigned char *tilemap, unsigned long size, unsigned int tile)
{
    unsigned int g = tile >> 4, j = tile & 15;
    if (g >= W2_TILEMAP_GROUPS || (unsigned long)(g + 1) * W2_TILEMAP_RECSIZE > size)
        return -1;
    return (int)W2_LE16(tilemap + g * W2_TILEMAP_RECSIZE + 2 * j);
}

unsigned int w2_tile_flags(const unsigned char *tilemap, unsigned long size, unsigned int tile)
{
    unsigned int g = tile >> 4;
    if (g >= W2_TILEMAP_GROUPS || (unsigned long)(g + 1) * W2_TILEMAP_RECSIZE > size)
        return 0;
    return W2_LE16(tilemap + g * W2_TILEMAP_RECSIZE + 40);
}

/* ------------------------------------------------------------------------- */
/* PUD                                                                       */
/* ------------------------------------------------------------------------- */

const unsigned char *w2_pud_section(const unsigned char *pud, unsigned long size,
                                    const char tag[4], unsigned long *len)
{
    unsigned long p = 0;
    while (p + 8 <= size) {
        unsigned long n = W2_LE32(pud + p + 4);
        if (memcmp(pud + p, tag, 4) == 0) {
            if (n > size - p - 8)          /* overflow-safe: p + 8 <= size holds here */
                return NULL;
            if (len)
                *len = n;
            return pud + p + 8;
        }
        if (n > size - p - 8)
            break;
        p += 8 + n;
    }
    return NULL;
}
