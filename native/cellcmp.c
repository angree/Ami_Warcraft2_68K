/* cellcmp.c - changed-cell conversion, shared by the Amiga backend (c2p) and the host backend.
 *
 * AmiSC's dirty-cell method: 'shadow' holds what the screen shows. cellcmp_blit() compares the chunky
 * buffer with it in cells of 32x16 pixels (32 = the c2p column) using 32-bit XORs, converts only the
 * cells that differ - runs closer than 3 cells merged into one call of 'conv' - and copies the converted
 * cells into the shadow. Plain C, no Amiga headers, so the host test runs exactly this code.
 * Requirements: chunky and shadow rows 4-byte aligned, pitch a multiple of 4, width <= 64 cells. */
#include <string.h>
#include "cellcmp.h"

static unsigned long s_conv, s_all;

void cellcmp_store(const unsigned char *chunky, unsigned char *shadow, int pitch, int sw, int sh,
                   int x, int y, int x2, int y2)
{
	int r;
	x &= ~31; x2 = (x2 + 31) & ~31;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x2 > sw) x2 = sw;
	if (y2 > sh) y2 = sh;
	if (x2 <= x || y2 <= y) return;
	for (r = y; r < y2; r++) memcpy(shadow + (long)r * pitch + x, chunky + (long)r * pitch + x, (size_t)(x2 - x));
}

void cellcmp_blit(const unsigned char *chunky, unsigned char *shadow, int pitch, int sw, int sh,
                  int x, int y, int w, int h, void (*conv)(int x, int y, int w, int h))
{
	int x0, x2, y2, cy, ncol, c, lw = pitch >> 2;
	unsigned char chg[64];
	x0 = x & ~31; x2 = (x + w + 31) & ~31; y2 = y + h;
	if (x0 < 0) x0 = 0;
	if (y < 0) y = 0;
	if (x2 > sw) x2 = sw;
	if (y2 > sh) y2 = sh;
	if (x2 <= x0 || y2 <= y) return;
	ncol = (x2 - x0) >> 5;
	if (ncol > 64) { conv(x, y, w, h); cellcmp_store(chunky, shadow, pitch, sw, sh, x, y, x + w, y + h); return; }
	for (cy = y; cy < y2; cy += 16) {
		int hh = y2 - cy < 16 ? y2 - cy : 16;
		for (c = 0; c < ncol; c++) {
			const unsigned long *a = (const unsigned long *)(const void *)(chunky + (long)cy * pitch + x0 + (c << 5));
			const unsigned long *b = (const unsigned long *)(const void *)(shadow + (long)cy * pitch + x0 + (c << 5));
			unsigned long d = 0;
			int r;
			if (sizeof(unsigned long) == 4) {
				for (r = 0; r < hh && !d; r++, a += lw, b += lw)
					d = (a[0] ^ b[0]) | (a[1] ^ b[1]) | (a[2] ^ b[2]) | (a[3] ^ b[3]) |
					    (a[4] ^ b[4]) | (a[5] ^ b[5]) | (a[6] ^ b[6]) | (a[7] ^ b[7]);
			} else {                     /* 64-bit host: same test, byte compare of the cell rows */
				for (r = 0; r < hh && !d; r++)
					d = memcmp(chunky + (long)(cy + r) * pitch + x0 + (c << 5),
					           shadow + (long)(cy + r) * pitch + x0 + (c << 5), 32) != 0;
			}
			chg[c] = (unsigned char)(d != 0);
		}
		s_all += (unsigned long)ncol;
		for (c = 0; c < ncol; ) {
			int e, k;
			if (!chg[c]) { c++; continue; }
			e = c;
			for (;;) {                        /* extend the run over gaps shorter than 3 cells */
				int n = e + 1, gap = 0;
				while (n < ncol && !chg[n] && gap < 3) { n++; gap++; }
				if (n < ncol && chg[n] && gap < 3) e = n; else break;
			}
			for (k = c; k <= e; k++) s_conv += chg[k];
			conv(x0 + (c << 5), cy, (e - c + 1) << 5, hh);
			cellcmp_store(chunky, shadow, pitch, sw, sh, x0 + (c << 5), cy, x0 + ((e + 1) << 5), cy + hh);
			c = e + 1;
		}
	}
}

void cellcmp_stats(unsigned long *conv, unsigned long *all)
{
	*conv = s_conv; *all = s_all;
	s_conv = s_all = 0;
}
