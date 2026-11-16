/* combat.h - combat.c + missile.c: module-local extras beyond the w2.h contract.
 *
 * The contract functions (ord_attack, ord_attack_ground, ord_attack_move, ord_patrol, ord_die,
 * combat_acquire, unit_damage, missile_fire, missile_tick_all, missile_draw_all and the commands
 * cmd_stop/stand/attack/attack_ground/attack_move/patrol/smart) are declared in w2.h.
 *
 * Rules implemented (docs/RE_NOTES.md section 5 = the original's CalcDamage 0x131FFE):
 *  - damage:  d = max(basic - armor, 0) + piercing; d == 0 -> nothing; h = (d + 1) >> 1;
 *             dmg = h + w2_rand() % (h + 1), capped at 255 (ceil(d/2) .. 2*ceil(d/2)).
 *  - melee hits on the attack event; direct missiles roll CalcDamage at launch (target armor) and
 *    apply it on impact; splash missiles (rock, bolt, cannons; bouncers too) carry the attack
 *    rating min(255, basic+piercing) and hit a 7x7-tile area: <= ~22 px from a unit centre full,
 *    <= ~42 px rating/4, minus armor, then the same roll. Launch adds target jitter (r&7)-3 per
 *    axis (two w2_rand() calls). NOT mirrored yet: the discarded Random() calls of some spell /
 *    missile routines (RE_NOTES §4 list) - needs per-routine RE.
 *  - spells (spells.c): bloodlust doubles basic' and piercing' (combat_weapon); unholy armor makes
 *    unit_damage ignore the hit (the reaction still happens); an invisible unit does not auto-acquire,
 *    cannot be targeted by other players' units and becomes visible when it strikes.
 *    W2Unit.damage_bonus is added to piercing, armor_bonus to armor (unused by the original).
 *  - weapon upgrades add to BASIC damage (swords/axes, ship cannons, catapult/ballista), except
 *    archers/axethrowers/rangers/berserkers whose arrow/axe upgrades add to PIERCING; marksmanship
 *    turns a ranger's basic damage into piercing. Armor upgrades add to armor.
 *    Upgrade state: econ_stat(slot, stat) when it returns non-zero, otherwise w2_modifiers[] with
 *    "researched" = bit (upgrade id) of W2Player.upgrades (all combat upgrades have ids < 32).
 *  - range: Chebyshev distance between footprints (unit_dist_tiles), min_range..max_range tiles.
 *  - attack cadence = the ATTACK animation; the hit / missile launch happens on the step flagged
 *    W2AF_ATTACK (detected through unit_anim_event() and, as a fallback, through anim_step).
 *
 * Assumptions about neighbouring modules (documented contract interpretation):
 *  - unit.c (unit.h) advances the animation before dispatching the order every cycle; step 0 of a
 *    new animation is entered one tick after unit_set_anim. Swings are never cut: a swing starts
 *    with unit_set_anim(ATTACK), the hit comes on the W2AF_ATTACK step, and the next decision is
 *    taken when the animation wraps (unit_anim_done / anim_step going back).
 *  - death: unit_damage sets W2_ORD_DIE when hp runs out (kill credit + score there); unit.c does
 *    the death (DYING, animation, corpse, slot freeing) and calls ord_die once as a hook: death
 *    sound + W2M_EXPLOSION for buildings / W2UX_EXPLODES units; ord_die never removes the unit.
 *    A unit with order DIE or hp 0 is treated as dead by every combat query.
 *  - idle auto-attack: move.c's ord_still/ord_stand should do
 *        t = combat_acquire(slot); if (t >= 0) combat_engage(slot, t);
 *    cmd_attack(slot, t) right after combat_acquire() in the same cycle is treated the same way
 *    (keeps stand-ground units from chasing).
 */
#ifndef W2_COMBAT_H
#define W2_COMBAT_H

#include <stdint.h>

/* ---- combat.c ---- */
void combat_reset(void);                       /* new game: clear module state (+ missiles) */
/* auto engagement from an idle order: remembers the current order (STILL/STAND/ATTACK_MOVE/PATROL)
 * and returns to it when the target is gone; STAND units never leave their tile. */
void combat_engage(int slot, int target);
int  combat_can_attack(int slot);              /* armed, finished, alive */
int  combat_can_target(int slot, int target);  /* kind/visibility/enemy-independent check of the target mask */
int  combat_in_range(int slot, int target);
/* effective weapon of a unit with upgrades: *basic, *piercing; returns max range (tiles) */
int  combat_weapon(int slot, int *basic, int *piercing);
int  combat_armor(int slot);
int  combat_roll(int basic, int piercing, int armor);   /* one hit, uses w2_rand() */
/* burning state of a damaged building: 0 none, 1 small fire (< 75 % hp), 2 big fire (< 50 %).
 * unitdraw.c can draw the W2M_SMALL_FIRE / W2M_BIG_FIRE sprites from this. */
int  combat_burn_level(int slot);
/* score of a player (sum of w2_unit_type.points of the units/buildings it destroyed) and kill count */
uint32_t combat_score(int player);
uint16_t combat_kills(int player);

/* ---- missile.c ---- */
#define W2_MAX_MISSILES 256
void missile_reset(void);
int  missile_count(void);                      /* missiles alive (tests) */
/* like missile_fire, but with a fixed damage (spells): dmg applied as-is (no armor, no roll);
 * dmg 0 = visual only */
void missile_fire_dmg(int shooter, int missile_type, int sx, int sy, int dx, int dy, int target, int dmg);
/* splash damage of the original at a map pixel (missiles; usable by spells): rating = attack rating
 * min(255, basic'+piercing') (no armor); 7x7 tiles, by distance to each unit centre: <= ~22 px full,
 * <= ~42 px rating/4; then minus armor (unless ignore_armor) and the CalcDamage roll.
 * layer_mask = W2_TGT_* layers hit. The attacker itself is never hit (Mac SplashDamageUnit 0x00129C54). */
void combat_area_damage(int px, int py, int rating, int attacker, int layer_mask, int ignore_armor);

/* spell missiles (the original's missile states 7..12; used by spells.c). Positions in map pixels.
 *  MSP_EFFECT     visual only: missile_effect(type, x, y) plays the sprite's frames once (3 cycles each)
 *  MSP_BREATH     fireball spell: dragon-breath missile from the shooter toward (x,y), half speed in the last
 *                 50 px, rolls on and splashes 'arg' (40) every 8 steps, 5 times (no armor, air too)
 *  MSP_SHIELD_GO  flame shield: fireball sprite flies to 'target', then MSP_SHIELD: 5 flames orbit the target
 *                 for 512..527 cycles, each splashing 4 every 16 cycles (never the shielded unit)
 *  MSP_BLIZZARD   one blizzard field at (x,y): a shard falls from up-left, splash 10, then 'arg' more shards
 *  MSP_DND        death and decay at (x,y) +-2 tiles (random): 'arg' (10) splashes of 10, one per 27 cycles
 *  MSP_WHIRL      whirlwind near (x,y) (+-64 px random): 'arg' (800) steps of 3 cycles, 1 px each, splash 4
 *                 every other step, wanders to random points +-64 px around itself */
enum { MSP_NONE, MSP_EFFECT, MSP_BREATH, MSP_SHIELD_GO, MSP_SHIELD, MSP_BLIZZARD, MSP_DND, MSP_WHIRL };
void missile_effect(int type, int x, int y);
void missile_spell(int kind, int shooter, int target, int x, int y, int arg);
int  missile_count_kind(int kind);             /* live spell missiles of one kind (tests) */

#endif
