/* move.h - movement + pathfinding module (src/move.c, src/path.c): extras beyond the w2.h contract.
 *
 * Movement model (like the original): a unit stands on whole tiles. A step to a neighbouring tile
 * (8 directions) reserves the destination tile in w2.map.occ_ground / occ_air at once; during the step
 * the unit holds BOTH tiles (tx,ty = source, ox/oy = pixel offset toward the destination). The step is
 * driven by the unit type's MOVE animation, played by unit.c (unit_set_anim(W2_ANIM_MOVE) restarts it
 * at each step): the offset is the sum of `arg` of the steps entered so far in this loop (both axes on
 * diagonals) - derived from the animation state, so a cycle without a move_* call loses nothing. At
 * 32 px the unit arrives: tx/ty = destination, ox/oy = 0, source released, and the next step may start
 * in the same cycle (16 cycles per tile for a footman).
 *
 * Ships: a 2-tile step (PATH_STEP2, see path.c) reserves only the far tile and arrives after 64 px of
 * the MOVE animation - two loops of our 32-px ship loop (the original's ship loop is one of 64 px in
 * 32 cycles, re68k anim_move_speed.csv: the same 2 px per cycle).
 *
 * Integration (works with today's unit.c; the first two would make it tighter):
 *  - unit.c should call move_tick(slot) every cycle for every unit on the map (idempotent). Today the
 *    offset is updated only when a move_* function / ord_still runs for the unit; a step interrupted by
 *    an order that never calls them stays between tiles until it does;
 *  - unit.c should call move_release(slot) when a unit leaves the map (death, removal, mine, transport).
 *    Without it a once-per-cycle sweep frees the reserved tile one cycle later;
 *  - game.c calls move_reset_all() after loading a map.
 */
#ifndef W2_MOVE_H
#define W2_MOVE_H

#include <stdint.h>

enum { MC_LAND, MC_SEA, MC_AIR, MC_DOCK };   /* MC_DOCK: a transport docking (Mac movement type 3) */

/* SQM terrain bits of a tile (cached from the tileset group flags; see path.c) */
#define MT_LAND    1     /* SQM 0x01: blocks ships and docking ships */
#define MT_SHALLOW 2     /* SQM 0x02: coast boundary; blocks land units and ships, a docking ship enters */
#define MT_WATER   4     /* SQM 0x40 */
int  move_terrain(int tx, int ty);          /* MT_* bits, 0 off the map */
/* docking mode of a transport (only while its order is W2_ORD_LOAD / W2_ORD_UNLOAD): may enter
 * shallow tiles, single steps unless start and goal are even (transport.c sets/clears it) */
void move_set_dock(int slot, int on);
int  move_get_dock(int slot);
void path_terrain_reset(void);             /* new map (move_reset_all calls it) */
/* path directions: bit 3 = a 2-tile step (ships on the even grid) */
#define PATH_STEP2 8

extern const int8_t w2_dir_dx[8];   /* 0=N clockwise */
extern const int8_t w2_dir_dy[8];

/* tunables */
extern int path_node_budget;        /* A* expansions per search (default 4096) */
extern int move_search_quota;       /* new path searches per logic cycle (default 3; FIFO for the rest) */

typedef struct PathStats {
	uint32_t searches, expansions, max_expansions, partial, nopath, deferred;
} PathStats;
extern PathStats path_stats;

/* path_search flags */
#define PATH_COMPLETE   1   /* the returned steps reach the goal range */
#define PATH_EXHAUSTED  2   /* the whole reachable area was searched (goal truly unreachable) */
#define PATH_MOVERBLOCK 4   /* an adjacent moving unit was treated as an obstacle */

/* Footprint-aware search: goal rectangle (gx,gy,gw,gh), stop within `range` tiles (chebyshev) of it.
 * Writes at most `max` directions (0..7) into dirs. Returns steps (0 = already in range), -1 when no
 * step brings the unit closer. *flags gets PATH_*. Static memory only, not reentrant. */
int  path_search(int slot, int gx, int gy, int gw, int gh, int range, uint8_t *dirs, int max, int *flags);

int  move_class(int slot);                 /* MC_* */
int  move_tile_ok(int mc, int tx, int ty); /* static terrain/building passability for a class */
int  move_diag_ok(int mc, int tx, int ty, int dir);   /* no corner cutting from (tx,ty) along dir */
int  move_is_active(int slot);             /* has a movement goal (walking or waiting to walk) */
int  move_is_stepping(int slot);           /* inside a tile step (holds two tiles) */
int  move_is_progressing(int slot);        /* started a step within the last few cycles */
void move_tick(int slot);
void move_release(int slot);
int  move_reserved(int slot, int *tx, int *ty);   /* 1 + destination tile while the step still holds it */
int  move_path_get(int slot, uint8_t *dirs, int max);   /* remaining planned steps (for debug overlays) */
void move_reset_all(void);

#endif
