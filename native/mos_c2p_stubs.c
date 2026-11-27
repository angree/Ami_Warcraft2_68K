/* mos_c2p_stubs.c - native MorphOS PPC build only: the chunky-to-planar routines are 68k assembler (Kalms).
 * They are never called there - MorphOS always takes the RTG (deep screen) path - but amiga_gfx.c refers to them. */
struct C2PArgs; struct C2P6Args;
void c2p_rect_asm(struct C2PArgs *a) { (void)a; }
void c2p6_bm_asm(struct C2P6Args *a) { (void)a; }
