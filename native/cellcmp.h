#ifndef CELLCMP_H
#define CELLCMP_H
/* changed-cell conversion (see cellcmp.c). 'shadow' = what the screen shows, same layout as 'chunky'
 * (pitch, screen size sw x sh). conv() converts one rectangle to the screen. */
void cellcmp_blit(const unsigned char *chunky, unsigned char *shadow, int pitch, int sw, int sh,
                  int x, int y, int w, int h, void (*conv)(int x, int y, int w, int h));
/* copy a rectangle (snapped to 32-pixel columns) of chunky into the shadow */
void cellcmp_store(const unsigned char *chunky, unsigned char *shadow, int pitch, int sw, int sh,
                   int x, int y, int x2, int y2);
/* cells converted / compared since the last call (resets) */
void cellcmp_stats(unsigned long *conv, unsigned long *all);
#endif
