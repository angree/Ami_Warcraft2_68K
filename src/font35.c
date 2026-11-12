/* font35.c - the 3x5 pixel font (CC0 1.0, see font35.h).
 *
 * Glyph encoding: a 5-digit OCTAL literal per glyph, one octal digit per row
 * (an octal digit is exactly three bits), top row first, left pixel = the
 * digit's highest bit. So 075557 is
 *     ###   7
 *     # #   5
 *     # #   5
 *     # #   5
 *     ###   7
 * Shapes identical to AmiSC's amiga/glue_font_3x5.h (same author, CC0).
 */
#include "font35.h"

#define F35_FIRST 32
#define F35_LAST  126

static const unsigned short font35_tab[F35_LAST - F35_FIRST + 1] = {
	000000, /* 32 space */
	022202, /* ! */
	055000, /* " */
	057575, /* # */
	036236, /* $ */
	051245, /* % */
	025253, /* & */
	022000, /* ' */
	012221, /* ( */
	042224, /* ) */
	005250, /* * */
	002720, /* + */
	000024, /* , */
	000700, /* - */
	000002, /* . */
	011244, /* / */
	075557, /* 0 */
	026227, /* 1 */
	061247, /* 2 */
	061216, /* 3 */
	055711, /* 4 */
	074616, /* 5 */
	034757, /* 6 */
	071222, /* 7 */
	075757, /* 8 */
	075716, /* 9 */
	002020, /* : */
	002024, /* ; */
	012421, /* < */
	007070, /* = */
	042124, /* > */
	061202, /* ? */
	025743, /* @ */
	025755, /* A */
	065656, /* B */
	034443, /* C */
	065556, /* D */
	074647, /* E */
	074644, /* F */
	034553, /* G */
	055755, /* H */
	072227, /* I */
	011152, /* J */
	055655, /* K */
	044447, /* L */
	057755, /* M */
	065555, /* N */
	025552, /* O */
	065644, /* P */
	025573, /* Q */
	065655, /* R */
	034216, /* S */
	072222, /* T */
	055557, /* U */
	055552, /* V */
	055775, /* W */
	055255, /* X */
	055222, /* Y */
	071247, /* Z */
	064446, /* [ */
	044211, /* backslash */
	031113, /* ] */
	025000, /* ^ */
	000007, /* _ */
	042000, /* ` */
	/* a..z: folded to the uppercase shapes in font35_glyph() */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	032623, /* { */
	022222, /* | */
	062326, /* } */
	006300  /* ~ */
};

/* ISO-8859-2 Polish letters and the German letters (same codes in ISO-8859-1/
 * CP1252) folded to their plain letter; anything else outside 32..126 is '?'. */
static int font35_fold(int c)
{
	if (c >= F35_FIRST && c <= F35_LAST) return c;
	switch (c) {
	case 0xC4: case 0xE4: return 'A';
	case 0xD6: case 0xF6: return 'O';
	case 0xDC: case 0xFC: return 'U';
	case 0xDF: return 'S';
	case 0xA1: case 0xB1: return 'A';
	case 0xC6: case 0xE6: return 'C';
	case 0xCA: case 0xEA: return 'E';
	case 0xA3: case 0xB3: return 'L';
	case 0xD1: case 0xF1: return 'N';
	case 0xD3: case 0xF3: return 'O';
	case 0xA6: case 0xB6: return 'S';
	case 0xAC: case 0xBC: case 0xAF: case 0xBF: return 'Z';
	default: return '?';
	}
}

unsigned short font35_glyph(int c)
{
	c = font35_fold(c & 0xFF);
	if (c >= 'a' && c <= 'z') c -= 32;
	return font35_tab[c - F35_FIRST];
}

int font35_width(const char *s)
{
	int best = 0, cur = 0;
	if (!s) return 0;
	for (; *s; ++s) {
		if (*s == '\n') {
			if (cur > best) best = cur;
			cur = 0;
			continue;
		}
		cur += FONT35_ADVANCE;
	}
	if (cur > best) best = cur;
	return best ? best - (FONT35_ADVANCE - FONT35_CELL_W) : 0;
}

int font35_height(const char *s)
{
	int lines = 1;
	if (!s || !*s) return 0;
	for (; *s; ++s)
		if (*s == '\n') ++lines;
	return lines * FONT35_LINE_H - (FONT35_LINE_H - FONT35_CELL_H);
}

static void font35_put(unsigned char *dst, int pitch, int clipw, int cliph,
                       int x, int y, unsigned short g, unsigned char colour)
{
	int row, col;
	for (row = 0; row < FONT35_CELL_H; ++row) {
		int yy = y + row;
		unsigned bits;
		unsigned char *line;
		if (yy < 0 || yy >= cliph) continue;
		bits = (g >> (12 - row * 3)) & 7u;
		if (!bits) continue;
		line = dst + (long)yy * pitch;
		for (col = 0; col < FONT35_CELL_W; ++col) {
			int xx = x + col;
			if (!(bits & (4u >> col))) continue;
			if (xx < 0 || xx >= clipw) continue;
			line[xx] = colour;
		}
	}
}

static int font35_run(unsigned char *dst, int pitch, int clipw, int cliph,
                      int x, int y, const char *s, unsigned char colour)
{
	int cx = x, cy = y;
	if (!dst || !s) return x;
	for (; *s; ++s) {
		unsigned char c = (unsigned char)*s;
		if (c == '\n') {
			cx = x;
			cy += FONT35_LINE_H;
			continue;
		}
		/* whole cell outside: skip without touching pixels */
		if (cx < clipw && cx + FONT35_CELL_W > 0 && cy < cliph && cy + FONT35_CELL_H > 0) {
			unsigned short g = font35_glyph(c);
			if (g) font35_put(dst, pitch, clipw, cliph, cx, cy, g, colour);
		}
		cx += FONT35_ADVANCE;
	}
	return cx;
}

int font35_draw(unsigned char *dst, int pitch, int clipw, int cliph,
                int x, int y, const char *s, unsigned char colour)
{
	return font35_run(dst, pitch, clipw, cliph, x, y, s, colour);
}

int font35_draw_shadow(unsigned char *dst, int pitch, int clipw, int cliph,
                       int x, int y, const char *s,
                       unsigned char colour, unsigned char shadow)
{
	font35_run(dst, pitch, clipw, cliph, x + 1, y + 1, s, shadow);
	return font35_run(dst, pitch, clipw, cliph, x, y, s, colour);
}
