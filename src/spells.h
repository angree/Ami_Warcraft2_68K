/* spells.h - spells, mana, timed unit effects, runes, demolition (src/spells.c).
 * The w2.h contract has ord_cast / ord_demolish / spells_tick_all; this header is the rest.
 *
 * Behaviour follows the Mac 68k binary (docs/RE_NOTES.md conventions; spell hooks 0x00160954..0x0016133A,
 * unit timers 0x0016BBFA, runes 0x00160656, caster AI 0x0013D722). Summary:
 *  - casters = UDTA flag 0x20000 (W2UF_SPELLCASTER: mage, death knight, paladin, ogre-mage, heroes);
 *    start mana 85, +1 every 40 cycles, max 255 (spells.c owns regeneration);
 *  - a cast order walks into range (Chebyshev tiles from the caster's tile to the target tile), plays the
 *    ATTACK animation and casts on its hit step; mana is paid at that moment; blizzard / death and decay
 *    repeat while mana lasts, everything else casts once;
 *  - timers (cycles): invisibility 2000, unholy armor 500 (+ HP halved), bloodlust 1000 (x2 damage),
 *    haste +1000 / slow -1000 (one signed counter, they cancel), flame shield 500 (5 orbiting flames);
 *  - effects are module-private per-slot arrays guarded by the slot generation; only units with an
 *    effect are visited each cycle (small active list).
 */
#ifndef W2_SPELLS_H
#define W2_SPELLS_H

#include <stdint.h>

void spells_reset(void);                 /* new game: forget effects, runes, visions (combat_reset calls it) */

/* timed effects (0 = none; values are the remaining cycles) */
int  spells_invisible(int slot);
int  spells_unholy(int slot);            /* unholy armor: unit_damage ignores hits */
int  spells_bloodlust(int slot);         /* basic + piercing damage doubled */
int  spells_haste(int slot);             /* > 0 hasted, < 0 slowed */
int  spells_flame_shield(int slot);
void spells_break_invis(int slot);       /* the unit attacked or cast: invisibility ends */
/* 1 when 'player' must not see the unit (invisible and not its owner - Mac visibility mask +0x29) */
int  spells_hidden_from(int slot, int player);

/* status icons for the unit drawing: bit 0 bloodlust, 1 haste, 2 slow, 3 invisible, 4 unholy armor
 * (War Data 323 frames 0..4, drawn at the selection box corner + 0/16/16/32/48 px - Mac 0x00170E46) */
int  spells_status_icons(int slot);

/* the original's numbers per spell id (enum w2_spell_id); range 255 = unlimited */
int  spells_mana_cost(int spell);
int  spells_range(int spell);
int  spells_targets_unit(int spell);     /* 1 = needs a target unit, 0 = position (or self) */

/* commands (validated): 0 = refused. cmd_cast also checks mana and research. */
int  cmd_cast(int slot, int spell, int target, int tx, int ty);
int  cmd_demolish(int slot, int target, int tx, int ty);

/* holy vision: re-apply the temporary sight to w2.map.visible. spells_tick_all() calls it; fog_update()
 * (mapdraw.c) should call it at its end so a recompute does not hide the area for a frame. */
void spells_fog_stamp(void);

/* runes on the map (for tests / an optional overlay): number active, and 1 if one lies on the tile */
int  spells_rune_count(void);
int  spells_rune_at(int tx, int ty);

/* computer casters: casts done since the game start (tests / logs) */
extern uint32_t spells_ai_casts;
extern uint32_t spells_casts[19];

#endif
