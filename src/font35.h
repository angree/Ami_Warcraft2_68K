/* font35.h - the 3x5 pixel font for the 320x240 (lores) screens.
 *
 * LICENCE: CC0 1.0 Universal (public domain dedication). The glyph shapes were
 * drawn by hand for AmiSC 68K (amiga/glue_font_3x5.h in that project, by the
 * same author) and released there under CC0 1.0; no bitmap of any existing
 * font was copied. This C port carries the same dedication.
 *
 * Metrics, in screen pixels: cell 3x5, advance 4 (3 ink + 1 gap; the gap is
 * where an optional +1,+1 shadow falls), line height 6.
 * Covers ASCII 32..126; lowercase uses the uppercase shapes (a 3x5 cell has no
 * room for a readable lowercase). ISO-8859-2 Polish letters and the German
 * umlauts fold to their plain letter, anything else shows as '?'.
 *
 * Plain C (gnu99), no floats, no Amiga headers.
 */
#ifndef FONT35_H
#define FONT35_H

#ifdef __cplusplus
extern "C" {
#endif

#define FONT35_CELL_W  3
#define FONT35_CELL_H  5
#define FONT35_ADVANCE 4
#define FONT35_LINE_H  6

/* 15-bit shape of byte c: five rows of three bits, top row in bits 14..12,
 * the left pixel the highest bit of its row. */
unsigned short font35_glyph(int c);

/* Width in pixels of the widest line of s ('\n' starts a new line). The last
 * glyph's trailing gap is not counted, so "AB" is 7 pixels wide. 0 for "". */
int font35_width(const char *s);

/* Height in pixels of s: lines * FONT35_LINE_H - 1 (the last line's gap row
 * is not counted). 0 for "". */
int font35_height(const char *s);

/* Draws s into an 8-bit chunky buffer `dst` (pitch bytes per row), clipped to
 * the rectangle 0..clipw-1 x 0..cliph-1. (x, y) is the top-left of the first
 * cell; '\n' returns to x and moves down FONT35_LINE_H. Only ink pixels are
 * written (transparent background). Returns the x just past the last glyph
 * drawn on the last line. */
int font35_draw(unsigned char *dst, int pitch, int clipw, int cliph,
                int x, int y, const char *s, unsigned char colour);

/* The same with a one-pixel shadow at (+1,+1) in `shadow`, drawn first. This
 * is how AmiSC keeps the font legible over any background. */
int font35_draw_shadow(unsigned char *dst, int pitch, int clipw, int cliph,
                       int x, int y, const char *s,
                       unsigned char colour, unsigned char shadow);

#ifdef __cplusplus
}
#endif

#endif /* FONT35_H */
