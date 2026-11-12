/*
 * test_war.c - host test for src/war.c + src/w2gfx_decode.c
 *
 * Usage: test_war <data dir with "War Data" etc.> <scratch out dir>
 *
 * - opens every archive, loads + decompresses every entry, CRC32 per archive
 * - classifies entries exactly like tools/w2data.py `guess`
 * - decodes every frame of every sprite (normal and flipped, clipped) and
 *   CRCs the pixels  -> compare with `python tools/w2data.py selftest <dir>`
 * - writes a few PPMs (tileset sheet, unit sheet, title, campaign map with
 *   units) into the scratch dir (NEVER into the repository - copyrighted data).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/war.h"
#include "../src/w2gfx_decode.h"

/* ---------- crc32 (same as zlib.crc32) ---------- */
static unsigned long crc_tab[256];
static void crc_init(void)
{
    unsigned long c;
    int n, k;
    for (n = 0; n < 256; n++) {
        c = (unsigned long)n;
        for (k = 0; k < 8; k++)
            c = (c & 1) ? 0xEDB88320UL ^ (c >> 1) : c >> 1;
        crc_tab[n] = c;
    }
}
static unsigned long crc32_upd(unsigned long crc, const unsigned char *p, unsigned long n)
{
    crc = ~crc & 0xFFFFFFFFUL;
    while (n--)
        crc = crc_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    return ~crc & 0xFFFFFFFFUL;
}

/* ---------- classification (mirror of w2data.guess) ---------- */
enum { T_EMPTY, T_WAV, T_PUD, T_FONT, T_SMK, T_IFF, T_PAL, T_TILEMAP, T_IMAGE,
       T_CURSOR, T_SPRITE, T_GFU, T_OTHER, T_NTYPES };
static const char *tname[T_NTYPES] = { "empty", "wav", "pud", "font", "smacker", "iff",
    "palette", "tilemap", "image", "cursor", "sprite", "gfu", "other" };

static int classify(const unsigned char *b, unsigned long n)
{
    unsigned long i;
    int t;
    if (n == 0) return T_EMPTY;
    if (n >= 4 && !memcmp(b, "RIFF", 4)) return T_WAV;
    if (n >= 16 && !memcmp(b, "TYPE", 4) && !memcmp(b + 8, "WAR2 MAP", 8)) return T_PUD;
    if (n >= 5 && !memcmp(b, "FONT ", 5)) return T_FONT;
    if (n >= 4 && (!memcmp(b, "SMK2", 4) || !memcmp(b, "SMK4", 4))) return T_SMK;
    if (n >= 4 && !memcmp(b, "FORM", 4)) return T_IFF;
    if (n == 768) {
        for (i = 0; i < n && b[i] < 64; i++)
            ;
        if (i == n) return T_PAL;
    }
    if (n == W2_TILEMAP_GROUPS * W2_TILEMAP_RECSIZE) return T_TILEMAP;
    if (w2_image_probe(b, n, NULL, NULL)) return T_IMAGE;
    {
        int w, h;
        if (w2_cursor_probe(b, n, NULL, NULL, &w, &h) && w <= 64 && h <= 64) return T_CURSOR;
    }
    t = w2_sprite_probe(b, n, NULL, NULL, NULL);
    if (t == W2_SPR_RLE) return T_SPRITE;
    if (t == W2_SPR_RAW) return T_GFU;
    return T_OTHER;
}

/* ---------- PPM ---------- */
static void write_ppm(const char *path, const unsigned char *img, int w, int h,
                      const unsigned char *pal6)
{
    FILE *f = fopen(path, "wb");
    long i;
    if (!f) {
        printf("cannot write %s\n", path);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (i = 0; i < (long)w * h; i++) {
        unsigned char rgb[3];
        int c;
        for (c = 0; c < 3; c++) {
            unsigned v = pal6[img[i] * 3 + c] & 63;
            rgb[c] = (unsigned char)((v << 2) | (v >> 4));
        }
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
    printf("  wrote %s (%dx%d)\n", path, w, h);
}

static const char *archives[] = { "War Data", "War Resources", "War Strings",
                                  "War Sounds", "War Music", "War Movies" };

static char g_data[512], g_out[512];

static unsigned char *load(const char *arch, int idx, unsigned long *n)
{
    char path[600];
    WarArchive *a;
    unsigned char *b;
    snprintf(path, sizeof(path), "%s/%s", g_data, arch);
    a = war_open(path);
    if (!a)
        return NULL;
    b = war_load(a, idx, n);
    war_close(a);
    return b;
}

/* summer sprite entry for a unit type (War Data), -1 if none */
static int summer_sprite(int t)
{
    static const short units[0x3A] = {
        45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 58, 51, 52, 33, 34,
        47, 48, 53, 54, -1, -1, -1, -1, -1, -1, 59, 60, 39, 40, 61, 62,
        41, 42, -1, -1, -1, -1, 43, 44, 38, 63, 35, 36, -1, 37, -1, -1,
        -1, 52, 51, 58, 51, 54, -1, 69, 70, 64 };
    static const short bld[0x68 - 0x3A] = {
        92, 93, 94, 95, 96, 97, 98, 99, 104, 105, 90, 91, 88, 89, 108, 109,
        100, 101, 102, 103, 110, 111, 84, 85, 106, 107, 112, 113, 114, 115, 86, 87,
        116, 117, 119, 118, 164, 165, 80, 81, 82, 83, 166, 167, 181, 168 };
    if (t >= 0 && t < 0x3A)
        return units[t];
    if (t >= 0x3A && t < 0x68)
        return bld[t - 0x3A];
    return -1;
}

static void render_campaign_map(int pudidx)
{
    unsigned long npud, nmega, nmini, nmap, nudta, len;
    unsigned char *pud = load("War Data", pudidx, &npud);
    unsigned char *pal, *mega, *mini, *map, *udta, *img;
    const unsigned char *mtxm, *dim, *unit, *era;
    int w, h, x, y, base = 2, i;
    unsigned char remap[8][256];
    char path[600];
    W2Clip clip;

    if (!pud) return;
    dim = w2_pud_section(pud, npud, "DIM ", NULL);
    mtxm = w2_pud_section(pud, npud, "MTXM", NULL);
    era = w2_pud_section(pud, npud, "ERA ", NULL);
    if (!dim || !mtxm || !era) { war_free(pud); return; }
    w = (int)W2_LE16(dim);
    h = (int)W2_LE16(dim + 2);
    base = era[0] == 1 ? 18 : (era[0] == 2 || era[0] == 3) ? 10 : 2;
    pal = load("War Data", base, NULL);
    mega = load("War Data", base + 1, &nmega);
    mini = load("War Data", base + 2, &nmini);
    map = load("War Data", base + 3, &nmap);
    udta = load("War Data", 0, &nudta);
    img = (unsigned char *)calloc((size_t)w * 32, (size_t)h * 32);
    if (!pal || !mega || !mini || !map || !img || !udta) goto out;
    for (y = 0; y < h; y++)
        for (x = 0; x < w; x++) {
            int m = w2_tile_to_megatile(map, nmap, W2_LE16(mtxm + 2 * (y * w + x)));
            if (m >= 0)
                w2_megatile_draw(mega, nmega, mini, nmini, m,
                                 img + (long)y * 32 * w * 32 + x * 32, w * 32);
        }
    /* player colour remaps: players 0..6 = 208+4p, player 7 = 12..15 */
    for (i = 0; i < 8; i++) {
        int k;
        for (k = 0; k < 256; k++) remap[i][k] = (unsigned char)k;
        for (k = 0; k < 4; k++) remap[i][208 + k] = (unsigned char)(i < 7 ? 208 + 4 * i + k : 12 + k);
    }
    clip.x0 = 0; clip.y0 = 0; clip.x1 = w * 32; clip.y1 = h * 32;
    unit = w2_pud_section(pud, npud, "UNIT", &len);
    for (i = 0; unit && i < (int)(len / 8); i++) {
        const unsigned char *u = unit + 8 * i;
        int ux = (int)W2_LE16(u), uy = (int)W2_LE16(u + 2), t = u[4], pl = u[5];
        int e = summer_sprite(t), cnt, bw, bh, typ, tw = 1, th = 1, frame;
        unsigned long nspr;
        unsigned char *spr;
        if (e < 0) continue;
        if (t < 110 && nudta >= 2446 + 4 * 110) {   /* UDTA size dword (no flag word) */
            tw = (int)W2_LE16(udta + 2446 + 4 * t);
            th = (int)W2_LE16(udta + 2446 + 4 * t + 2);
        }
        spr = load("War Data", e, &nspr);
        if (!spr) continue;
        typ = w2_sprite_probe(spr, nspr, &cnt, &bw, &bh);
        frame = (t < 0x3A) ? 2 + 2 * (i & 1) : 0;     /* units: E or S facing, buildings: frame 0 */
        if (frame >= cnt) frame = 0;
        w2_sprite_draw(spr, nspr, typ, frame, img, w * 32,
                       ux * 32 + (tw * 32 - bw) / 2, uy * 32 + (th * 32 - bh) / 2,
                       &clip, (t < 0x3A && (i & 2)) ? W2_DRAW_FLIPX : 0, remap[pl & 7]);
        war_free(spr);
    }
    snprintf(path, sizeof(path), "%s/c_map_%d.ppm", g_out, pudidx);
    write_ppm(path, img, w * 32, h * 32, pal);
out:
    free(img);
    war_free(pud); war_free(pal); war_free(mega); war_free(mini); war_free(map); war_free(udta);
}

static void write_samples(void)
{
    unsigned long n1, n2, n3, n;
    unsigned char *pal = load("War Data", 2, NULL);
    unsigned char *mega = load("War Data", 3, &n1);
    unsigned char *mini = load("War Data", 4, &n2);
    unsigned char *spr, *img, *tp;
    char path[600];
    int cnt, bw, bh, typ, i, nt;

    if (pal && mega && mini) {
        unsigned char *tiles = w2_tileset_build(mega, n1, mini, n2, &nt);
        int rows = (nt + 15) / 16, y;
        img = (unsigned char *)calloc(512, (size_t)rows * 32);
        for (i = 0; tiles && img && i < nt; i++)
            for (y = 0; y < 32; y++)
                memcpy(img + ((long)(i / 16) * 32 + y) * 512 + (i % 16) * 32, tiles + 1024L * i + 32 * y, 32);
        snprintf(path, sizeof(path), "%s/c_tileset_summer.ppm", g_out);
        if (img) write_ppm(path, img, 512, rows * 32, pal);
        free(img);
        free(tiles);
    }
    /* footman: 5 facings x 2 rows (normal, then flipped), clipped 8px border */
    spr = load("War Data", 45, &n3);
    if (spr && pal) {
        W2Clip c;
        typ = w2_sprite_probe(spr, n3, &cnt, &bw, &bh);
        img = (unsigned char *)calloc((size_t)bw * 5, (size_t)bh * 2);
        c.x0 = 0; c.y0 = 0; c.x1 = bw * 5; c.y1 = bh * 2;
        for (i = 0; img && i < 5; i++) {
            w2_sprite_draw(spr, n3, typ, i, img, bw * 5, i * bw, 0, &c, 0, NULL);
            w2_sprite_draw(spr, n3, typ, i, img, bw * 5, i * bw, bh, &c, W2_DRAW_FLIPX, NULL);
        }
        snprintf(path, sizeof(path), "%s/c_footman.ppm", g_out);
        if (img) write_ppm(path, img, bw * 5, bh * 2, pal);
        free(img);
    }
    war_free(spr);
    /* title screen with its own palette */
    tp = load("War Data", 300, NULL);
    spr = load("War Data", 299, &n);
    if (tp && spr) {
        int w, h;
        if (w2_image_probe(spr, n, &w, &h)) {
            img = (unsigned char *)malloc((size_t)w * h);
            if (img) {
                w2_image_draw(spr, n, img, w, 0, 0, NULL);
                snprintf(path, sizeof(path), "%s/c_title.ppm", g_out);
                write_ppm(path, img, w, h, tp);
                free(img);
            }
        }
    }
    war_free(tp);
    war_free(spr);
    /* game font */
    spr = load("War Data", 282, &n);
    if (spr && pal) {
        static const unsigned char cols[8] = { 0, 228, 239, 0, 230, 200, 232, 233 };
        int mw, mh, ng = w2_font_probe(spr, n, &mw, &mh), x = 0;
        static const char msg[] = "Warcraft II: Tides of Darkness - 0123456789";
        img = (unsigned char *)calloc((size_t)(sizeof(msg) * mw), (size_t)mh);
        for (i = 0; img && ng && msg[i]; i++) {
            int adv = w2_font_draw(spr, n, (unsigned char)msg[i], img, (int)sizeof(msg) * mw, x, 0, NULL, cols);
            x += adv > 0 ? adv + 1 : mw / 2;
        }
        snprintf(path, sizeof(path), "%s/c_font.ppm", g_out);
        if (img) write_ppm(path, img, x, mh, pal);
        free(img);
    }
    war_free(spr);
    war_free(pal); war_free(mega); war_free(mini);
    render_campaign_map(192);
}

int main(int argc, char **argv)
{
    int ai, failures = 0;
    unsigned long total_sprites = 0, total_frames = 0;

    if (argc < 3) {
        printf("usage: %s <data dir> <scratch out dir>\n", argv[0]);
        return 2;
    }
    snprintf(g_data, sizeof(g_data), "%s", argv[1]);
    snprintf(g_out, sizeof(g_out), "%s", argv[2]);
    crc_init();

    for (ai = 0; ai < (int)(sizeof(archives) / sizeof(*archives)); ai++) {
        char path[600];
        WarArchive *a;
        int i, hist[T_NTYPES];
        unsigned long crc = 0, bytes = 0, pixcrc = 0, frames = 0, sprites = 0;
        unsigned char *buf = NULL;
        unsigned long bufcap = 0;

        snprintf(path, sizeof(path), "%s/%s", g_data, archives[ai]);
        a = war_open(path);
        if (!a) {
            printf("%-14s: cannot open (err %d)\n", archives[ai], war_last_error());
            failures++;
            continue;
        }
        memset(hist, 0, sizeof(hist));
        for (i = 0; i < war_count(a); i++) {
            unsigned long n;
            unsigned char *b = war_load(a, i, &n);
            int t;
            if (!b && war_last_error() != WAR_OK) {
                printf("  %s #%d: load error %d\n", archives[ai], i, war_last_error());
                failures++;
                continue;
            }
            /* war_load_into must agree with war_load */
            if (b && n) {
                long r;
                unsigned char *b2 = (unsigned char *)malloc(n);
                if (b2) {
                    r = war_load_into(a, i, b2, n);
                    if (r != (long)n || memcmp(b, b2, n) != 0) {
                        printf("  %s #%d: war_load_into mismatch\n", archives[ai], i);
                        failures++;
                    }
                    free(b2);
                }
            }
            crc = crc32_upd(crc, b, n);
            bytes += n;
            t = classify(b, n);
            hist[t]++;
            if (t == T_SPRITE || t == T_GFU) {
                int cnt, bw, bh, f, typ = w2_sprite_probe(b, n, &cnt, &bw, &bh);
                unsigned long need = (unsigned long)bw * bh;
                sprites++;
                if (need > bufcap) {
                    free(buf);
                    buf = (unsigned char *)malloc(need);
                    bufcap = buf ? need : 0;
                }
                for (f = 0; buf && f < cnt; f++) {
                    W2Clip c;
                    memset(buf, 0, need);
                    if (w2_sprite_draw(b, n, typ, f, buf, bw, 0, 0, NULL, 0, NULL) < 0) {
                        printf("  %s #%d frame %d: decode error\n", archives[ai], i, f);
                        failures++;
                    }
                    pixcrc = crc32_upd(pixcrc, buf, need);
                    /* flipped + clipped to the inner box (2px border) */
                    memset(buf, 0, need);
                    c.x0 = 2; c.y0 = 2; c.x1 = bw - 2; c.y1 = bh - 2;
                    w2_sprite_draw(b, n, typ, f, buf, bw, 0, 0, &c, W2_DRAW_FLIPX, NULL);
                    pixcrc = crc32_upd(pixcrc, buf, need);
                    frames++;
                }
            }
            war_free(b);
        }
        free(buf);
        total_sprites += sprites;
        total_frames += frames;
        printf("%-14s id %4d entries %3d bytes %9lu crc %08lx sprites %3lu frames %5lu pixcrc %08lx\n",
               archives[ai], war_id(a), war_count(a), bytes, crc, sprites, frames, pixcrc);
        printf("               types:");
        for (i = 0; i < T_NTYPES; i++)
            if (hist[i])
                printf(" %s=%d", tname[i], hist[i]);
        printf("\n");
        war_close(a);
    }
    printf("total sprites %lu frames %lu, failures %d\n", total_sprites, total_frames, failures);
    write_samples();
    return failures ? 1 : 0;
}
