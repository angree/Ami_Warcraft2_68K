/*
 * war.c - Warcraft II data archive reader. See war.h and docs/DATA_FORMAT.md.
 *
 * Compression (flags 0x20), our description of the scheme:
 *   The stream is a sequence of groups: one control byte followed by 8 items.
 *   Control bits are consumed LSB first. Bit = 1: one literal byte follows.
 *   Bit = 0: a u16 little-endian reference follows: low 12 bits = ABSOLUTE
 *   position in a 4096-byte history ring, high 4 bits + 3 = copy length (3..18).
 *   The ring starts zero-filled with the write position at 0; every output byte
 *   (literal or copied) is appended to the ring. Decoding stops as soon as
 *   'size' bytes have been produced (possibly in the middle of a group / copy).
 *
 * We do not keep a separate ring: ring slot s always holds the most recent
 * output byte at position k < pos with (k & 0xFFF) == s, or 0 if none, so the
 * source is read straight from the output buffer.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "war.h"

#define WAR_MAGIC 0x19UL

struct WarArchive {
    FILE *fp;
    int count;
    int id;
    unsigned long filesize;
    unsigned long *offset;   /* count entries */
    unsigned long *avail;    /* bytes until next entry start / EOF */
};

static int war_err;

int war_last_error(void) { return war_err; }

/* ---- small buffered reader for the compressed stream ---- */
#define RDBUF 1024
typedef struct {
    FILE *fp;
    unsigned long left;      /* compressed bytes still allowed */
    unsigned int pos, len;
    unsigned char buf[RDBUF];
} Reader;

static int rd_byte(Reader *r)
{
    if (r->pos >= r->len) {
        unsigned long want = r->left < RDBUF ? r->left : RDBUF;
        size_t got;
        if (want == 0)
            return -1;
        got = fread(r->buf, 1, (size_t)want, r->fp);
        if (got == 0)
            return -1;
        r->left -= got;
        r->len = (unsigned int)got;
        r->pos = 0;
    }
    return r->buf[r->pos++];
}

static int cmp_ulong(const void *a, const void *b)
{
    unsigned long x = *(const unsigned long *)a, y = *(const unsigned long *)b;
    return x < y ? -1 : (x > y ? 1 : 0);
}

WarArchive *war_open(const char *path)
{
    unsigned char hdr[8];
    unsigned char *tab = NULL;
    unsigned long *sorted = NULL;
    WarArchive *a;
    int i;
    long fs;

    war_err = WAR_OK;
    a = (WarArchive *)calloc(1, sizeof(*a));
    if (!a) {
        war_err = WAR_ERR_MEMORY;
        return NULL;
    }
    a->fp = fopen(path, "rb");
    if (!a->fp) {
        war_err = WAR_ERR_OPEN;
        free(a);
        return NULL;
    }
    if (fseek(a->fp, 0, SEEK_END) != 0 || (fs = ftell(a->fp)) < 8 ||
        fseek(a->fp, 0, SEEK_SET) != 0 || fread(hdr, 1, 8, a->fp) != 8 ||
        W2_LE32(hdr) != WAR_MAGIC) {
        war_err = WAR_ERR_FORMAT;
        goto fail;
    }
    a->filesize = (unsigned long)fs;
    a->count = (int)W2_LE16(hdr + 4);
    a->id = (int)W2_LE16(hdr + 6);
    if (8UL + 4UL * (unsigned long)a->count > a->filesize) {
        war_err = WAR_ERR_FORMAT;
        goto fail;
    }
    a->offset = (unsigned long *)malloc(sizeof(unsigned long) * (a->count + 1));
    a->avail = (unsigned long *)malloc(sizeof(unsigned long) * (a->count + 1));
    sorted = (unsigned long *)malloc(sizeof(unsigned long) * (a->count + 1));
    tab = (unsigned char *)malloc(4 * (a->count + 1));
    if (!a->offset || !a->avail || !sorted || !tab) {
        war_err = WAR_ERR_MEMORY;
        goto fail;
    }
    if (a->count && fread(tab, 4, (size_t)a->count, a->fp) != (size_t)a->count) {
        war_err = WAR_ERR_FORMAT;
        goto fail;
    }
    for (i = 0; i < a->count; i++) {
        a->offset[i] = W2_LE32(tab + 4 * i);
        sorted[i] = a->offset[i];
    }
    sorted[a->count] = a->filesize;
    qsort(sorted, (size_t)a->count + 1, sizeof(unsigned long), cmp_ulong);
    for (i = 0; i < a->count; i++) {
        /* binary search first sorted value > offset */
        int lo = 0, hi = a->count + 1;
        unsigned long o = a->offset[i];
        while (lo < hi) {
            int mid = (lo + hi) >> 1;
            if (sorted[mid] <= o)
                lo = mid + 1;
            else
                hi = mid;
        }
        if (o >= a->filesize)
            a->avail[i] = 0;
        else
            a->avail[i] = (lo <= a->count ? sorted[lo] : a->filesize) - o;
    }
    free(tab);
    free(sorted);
    return a;
fail:
    free(tab);
    free(sorted);
    war_close(a);
    return NULL;
}

void war_close(WarArchive *a)
{
    if (!a)
        return;
    if (a->fp)
        fclose(a->fp);
    free(a->offset);
    free(a->avail);
    free(a);
}

int war_count(const WarArchive *a) { return a ? a->count : 0; }
int war_id(const WarArchive *a) { return a ? a->id : 0; }

/* Read entry header word. Returns 1 ok, 0 placeholder, -1 error. */
static int entry_header(WarArchive *a, int index, unsigned long *size, int *flags)
{
    unsigned char h[4];
    if (!a || index < 0 || index >= a->count) {
        war_err = WAR_ERR_INDEX;
        return -1;
    }
    if (a->avail[index] < 4) {
        *size = 0;
        *flags = 0xFF;
        return 0;
    }
    if (fseek(a->fp, (long)a->offset[index], SEEK_SET) != 0 || fread(h, 1, 4, a->fp) != 4) {
        war_err = WAR_ERR_READ;
        return -1;
    }
    *size = W2_LE32(h) & 0xFFFFFFUL;
    *flags = h[3];
    if (*flags != 0x00 && *flags != 0x20) {
        war_err = WAR_ERR_FLAGS;
        return -1;
    }
    if (*flags == 0x00 && *size > a->avail[index] - 4) {
        war_err = WAR_ERR_READ;
        return -1;
    }
    return 1;
}

long war_entry_size(WarArchive *a, int index)
{
    unsigned long size;
    int flags, r;
    war_err = WAR_OK;
    r = entry_header(a, index, &size, &flags);
    if (r < 0)
        return -1;
    return (long)size;
}

int war_entry_flags(WarArchive *a, int index)
{
    unsigned long size;
    int flags, r;
    war_err = WAR_OK;
    r = entry_header(a, index, &size, &flags);
    if (r < 0)
        return -1;
    return flags;
}

static int decompress(WarArchive *a, int index, unsigned char *out, unsigned long size)
{
    Reader rd;
    unsigned long pos = 0;

    rd.fp = a->fp;
    rd.left = a->avail[index] - 4;
    rd.pos = rd.len = 0;
    while (pos < size) {
        int ctrl = rd_byte(&rd);
        int bit;
        if (ctrl < 0)
            return -1;
        for (bit = 0; bit < 8 && pos < size; bit++, ctrl >>= 1) {
            if (ctrl & 1) {
                int c = rd_byte(&rd);
                if (c < 0)
                    return -1;
                out[pos++] = (unsigned char)c;
            } else {
                int lo = rd_byte(&rd), hi = rd_byte(&rd);
                unsigned int ref, n;
                long k;
                if (lo < 0 || hi < 0)
                    return -1;
                ref = (unsigned int)lo | ((unsigned int)hi << 8);
                n = (ref >> 12) + 3;
                /* most recent output position with (k & 0xFFF) == slot */
                k = (long)((pos & ~0xFFFUL) | (ref & 0xFFFU));
                if (k >= (long)pos)
                    k -= 4096;
                while (n-- && pos < size) {
                    out[pos++] = (k < 0) ? 0 : out[k];
                    k++;
                }
            }
        }
    }
    return 0;
}

long war_load_into(WarArchive *a, int index, unsigned char *dst, unsigned long cap)
{
    unsigned long size;
    int flags, r;

    war_err = WAR_OK;
    r = entry_header(a, index, &size, &flags);
    if (r <= 0)
        return r;
    if (size > cap) {
        war_err = WAR_ERR_MEMORY;
        return -1;
    }
    if (flags == 0x00) {
        if (size && fread(dst, 1, (size_t)size, a->fp) != (size_t)size) {
            war_err = WAR_ERR_READ;
            return -1;
        }
    } else if (decompress(a, index, dst, size) != 0) {
        war_err = WAR_ERR_READ;
        return -1;
    }
    return (long)size;
}

unsigned char *war_load(WarArchive *a, int index, unsigned long *size)
{
    unsigned long sz;
    int flags, r;
    unsigned char *buf;

    if (size)
        *size = 0;
    war_err = WAR_OK;
    r = entry_header(a, index, &sz, &flags);
    if (r <= 0)
        return NULL;
    buf = (unsigned char *)malloc(sz ? sz : 1);
    if (!buf) {
        war_err = WAR_ERR_MEMORY;
        return NULL;
    }
    if (war_load_into(a, index, buf, sz) < 0) {
        free(buf);
        return NULL;
    }
    if (size)
        *size = sz;
    return buf;
}

void war_free(void *p)
{
    free(p);
}
