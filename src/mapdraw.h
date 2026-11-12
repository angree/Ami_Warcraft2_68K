/* mapdraw.h - extras of mapdraw.c beyond the [mapdraw.c] block of w2.h (terrain, fog, minimap, view).
 * Owned by mapdraw.c. */
#ifndef W2_MAPDRAW_H
#define W2_MAPDRAW_H

/* Fog look for explored-but-not-visible tiles:
 *   MAPDRAW_FOG_DITHER  every other pixel black (the original's look; one 32-bit AND per 4 pixels)
 *   MAPDRAW_FOG_SHADE   palette darkened through a 256-byte shade table (one lookup per pixel)
 *   MAPDRAW_FOG_OFF     no fog at all (everything drawn as visible; the map is still black where unexplored
 *                       unless mapdraw_reveal is set)
 * Default from the w2.cfg extra key "fog=dither|shade|off" (dither). */
enum { MAPDRAW_FOG_DITHER, MAPDRAW_FOG_SHADE, MAPDRAW_FOG_OFF };
extern int mapdraw_fog_mode;

/* 1 = the whole map counts as explored and visible for the local player (cheat / debug; w2.cfg extra
 * key "reveal=1"). fog_update still maintains the real per-player bits for AI and targeting. */
extern int mapdraw_reveal;

/* Called by game_start after a map is loaded: reads the cfg keys, forgets the cached minimap and era
 * tables, resets the view to the map origin. */
void mapdraw_reset(void);

/* Drop the cached minimap terrain (done automatically when w2.map.version or the era changes). */
void minimap_invalidate(void);

/* The darkening table of the current era palette (256 bytes), and the black / white pen indices used
 * by the fog and the minimap frame (never 17..19, the pointer's pens). */
const unsigned char *mapdraw_shade_table(void);
int mapdraw_black(void);
int mapdraw_white(void);

/* The sight-circle half width: tiles to either side at row distance dy (0..r) from the edge of a
 * footprint for sight radius r (0..MAPDRAW_SIGHT_MAX). */
#define MAPDRAW_SIGHT_MAX 16
int mapdraw_sight_span(int r, int dy);

#endif
