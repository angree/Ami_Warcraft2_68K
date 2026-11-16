/* transport.h - transports (src/transport.c): extras beyond the w2.h contract (ord_load, ord_unload,
 * transport_smart). Used by ai.c (the computer's transport tactic), move.c (passenger check) and,
 * for the cargo display, ui.c.
 *
 * Model (Mac 1.23, see transport.c for the addresses):
 *  - a transport carries up to TR_CAP land units in fixed slots; passengers are W2_UF_INSIDE, off the
 *    map, order W2_ORD_LOAD with order_state TRS_INSIDE, and die when the transport dies;
 *  - a transport docks by sailing onto a shallow coast tile (SQM 0x02) in docking mode; boarding needs
 *    the transport docked-waiting and the unit within 2 tiles on each axis; unloading needs the
 *    transport standing on a shallow tile and puts every passenger on a free land tile around it;
 *  - land regions: connected walkable land (trees, rocks, walls, water, shallows excluded; buildings
 *    ignored), relabelled at most every TR_REGION_PERIOD cycles after the terrain changed.
 */
#ifndef W2_TRANSPORT_H
#define W2_TRANSPORT_H

#include "w2.h"

#define TR_CAP 6                 /* passengers per transport (Mac: 6 slots at unit+0x70) */

/* order_state while the order is W2_ORD_LOAD / W2_ORD_UNLOAD */
enum {
	TRS_START = 0,               /* transport: choose the dock point; passenger: walking to the transport */
	TRS_DOCKING,                 /* transport: sailing to the dock point in docking mode */
	TRS_WAIT,                    /* transport: docked, waiting (Mac order 32) - passengers may board */
	TRS_INSIDE                   /* passenger: carried */
};

/* land region of a tile (0 = not walkable land); a shallow tile has no region of its own */
int  transport_region(int tx, int ty);
/* 1 when shallow tile (tx,ty) borders walkable land of region r (r = 0: any land) */
int  transport_coast_of(int tx, int ty, int r);

int  transport_is(int slot);                    /* the unit is a transport */
int  transport_count(int t);                    /* passengers aboard */
int  transport_list(int t, uint16_t *out);      /* passenger slots (at most TR_CAP), returns the count */
int  transport_of(int passenger);               /* the transport carrying it, -1 */
int  transport_docked(int t);                   /* standing on a shallow tile */
int  transport_waiting(int t);                  /* docked-waiting order (accepts passengers) */
int  transport_dock_goal(int t, int *tx, int *ty);   /* where a docking transport is heading, 0 none */

/* commands (as the right-click / buttons issue them) */
void transport_cmd_board(int unit, int t);      /* the unit boards t; t comes to dock near it */
/* t docks near land/shallow tile (tx,ty) and waits (unload = 0) or unloads there (unload = 1; 2 = the
 * same, but a usable dock tile (tx,ty) is taken as it is even when t is docked on that coast already).
 * 0 = no dock point within reach of that tile (nothing changed) */
int  transport_cmd_dock(int t, int tx, int ty, int unload);
/* the original's landing search for the computer (AI_RandomDirSearch 0x00141D06): from (tx,ty) walk
 * straight lines, starting in a random direction, to the first shallow tile of the region of
 * (tx,ty) (ours: on transport t's sea). 1 = found (*ox,*oy) */
int  transport_find_landing(int t, int tx, int ty, int *ox, int *oy);

/* move.c: called every cycle for a carried unit (keeps it inside / kills it with its transport) */
void transport_inside_tick(int slot);
void transport_reset(void);                     /* new game (called lazily on a world restart too) */

#endif
