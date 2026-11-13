/* unit.h - extras of the unit module (unit.c / unitdraw.c) beyond the shared contract in w2.h.
 *
 * Life cycle (owned by unit.c):
 *  - a slot is FREE when w2.unit[slot].flags == 0. unit_create() takes the lowest free slot.
 *    unit_create() of a BUILDING fails (-1, nothing allocated) when any footprint tile is held by
 *    another unit/building on the ground layer (a whole footprint or nothing).
 *  - a live unit has W2_UF_ALIVE. Death: any module sets order W2_ORD_DIE (unit_kill() or
 *    unit_set_order(slot, W2_ORD_DIE, ...)); a live non-indestructible unit whose hp reached 0 is
 *    killed automatically. At the next unit_tick_all() unit.c clears W2_UF_ALIVE/SELECTED, sets
 *    W2_UF_DYING, removes it from the occupancy grids, the player counts and w2.select[], starts
 *    the DEATH animation and calls ord_die(slot) ONCE as a hook (combat.c: death sound, explosion
 *    missile, score; it must NOT remove the unit). When the animation reaches its W2AF_DIE step
 *    (or at once when the type has no DEATH animation) the slot is freed and the type's corpse
 *    (w2_unit_type.corpse) is added to a module-private corpse list that decays by itself.
 *  - dying units still occupy their slot (flags == W2_UF_DYING...) and are drawn, but are not
 *    ALIVE: every target/selection query must test W2_UF_ALIVE.
 *
 * Animation (unit.c): unit_set_anim() sets the first frame at once; step 0 is ENTERED (its flags
 * reported, its pixels moved) at the next unit_tick_all(), so a handler that starts an animation
 * sees its first events one cycle later. Entering a step plays its W2AF_SOUND (if the local player
 * sees the unit) and performs W2AF_IDLE_TURN. STILL/MOVE/ATTACK/... loop; DEATH stops at its end.
 * A looping MOVE/ATTACK/HARVEST/REPAIR/SPELLCAST animation falls back to STILL when it wraps while
 * the order is STILL/STAND/NONE.
 */
#ifndef W2_UNIT_H
#define W2_UNIT_H

#include "w2.h"

/* pixels of movement of the MOVE-animation steps entered this cycle (0 if none) - move.c applies them */
int  unit_anim_move_pixels(int slot);
/* 1 while the animation is inside a W2AF_UNBREAK_BEGIN..END section (defer new orders) */
int  unit_anim_busy(int slot);
/* 1 when the current (non-looping or looping) animation has just wrapped/ended this cycle */
int  unit_anim_done(int slot);
/* kill a unit (sets order W2_ORD_DIE; the death is processed at the next unit_tick_all) */
void unit_kill(int slot);
/* sprite frame to draw for a unit (direction column applied) and whether to mirror it */
int  unit_sprite_frame(int slot, int *mirror);
/* the local player may see/click it: own units always; buildings once a footprint tile is explored,
 * other units while a footprint tile is visible (fog_explored / fog_visible) */
int  unit_visible_local(int slot);
/* slot generation: changes when the slot is freed. Keep (slot, unit_gen(slot)) to refer to ONE unit;
 * unit_same(slot, gen) is 1 while that very unit is alive (not a newcomer in a reused slot) */
uint16_t unit_gen(int slot);
int      unit_same(int slot, uint16_t gen);
/* order target with identity: unit_set_order() and unit_set_target() remember the target's generation;
 * unit_target_same(slot) is 1 while w2.unit[slot].target is that very unit and alive (0 for W2_NO_UNIT
 * or a newcomer that reused the target's slot). A direct write of .target is taken as-is at the next
 * unit_target_same() call - use unit_set_target() instead. */
void     unit_set_target(int slot, int target);
int      unit_target_same(int slot);
/* sprite box (hires map pixels, top-left) of a unit as drawn: centred on its footprint + ox/oy */
void unit_box(int slot, int *x, int *y, int *w, int *h);

/* ---- corpses / rubble (module-private list) ---- */
#define W2_MAX_CORPSES 256
typedef struct W2Corpse {
	uint8_t  used;
	uint8_t  kind;          /* enum w2_corpse */
	uint8_t  owner;         /* for the player colour of dead bodies */
	uint8_t  dir;
	uint8_t  tx, ty, tw, th;/* footprint */
	int8_t   ox, oy;
	uint8_t  step, wait;    /* animation state (set W2A_*_DEAD_BODY / DESTROYED_PLACE ...) */
	uint8_t  frame;
	uint8_t  set;           /* w2_anim_set id */
} W2Corpse;
extern W2Corpse w2_corpses[W2_MAX_CORPSES];
int  unit_corpse_add(int kind, int owner, int tx, int ty, int tw, int th, int ox, int oy, int dir);
int  unit_corpse_at(int tx, int ty);           /* a dead body (not rubble) on that tile, -1 (raise dead) */
void unit_corpse_remove(int idx);
void unit_corpses_clear(void);

#endif
