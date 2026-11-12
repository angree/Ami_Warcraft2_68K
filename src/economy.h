/* economy.h - extra API of economy.c beyond the [economy.c] block of w2.h.
 *
 * Researched upgrades: W2Player has only 32 bits in 'upgrades' but there are 52 upgrades, so
 * economy.c stores upgrade u < 32 as bit u of player.upgrades and u >= 32 as
 * player.upgrade_level[u - 32] != 0. Always ask econ_has_upgrade() instead of reading those fields.
 * Free spells (w2_upgrades[u].researched_at == W2_NONE: holy vision, fireball, eye, death coil)
 * count as researched from the start.
 */
#ifndef W2_ECONOMY_H
#define W2_ECONOMY_H

#include "w2.h"

/* Forget all module-private state (call after a new map is loaded; economy.c also does it by
 * itself when w2.cycle goes backwards). */
void econ_reset(void);

int  econ_has_upgrade(int player, int upg);
int  econ_research_in_progress(int player, int upg);   /* some building of the player researches it */
/* 0..100 progress of the building's current job: construction, training, research or upgrade;
 * -1 when it has none. */
int  econ_progress_percent(int slot);
/* Why econ_place_ok() refuses: 0 = ok, otherwise the War Strings (table 1) index of the original
 * message (390 off the map, 391 cannot build there, 392 coast, 393 explore first, 394 oil
 * platform over oil, 395 town hall near gold mine, 396 near oil patch). */
int  econ_place_reason(int player, int type, int tx, int ty, int builder);
int  econ_food_free(int player);       /* supply - used - reserved by units in training */
/* Chop the tree at (tx,ty) off the map: stump tile + forest edges of the neighbours re-picked.
 * Also usable by demolition/explosions. */
void econ_clear_tree(int tx, int ty);
/* Nearest choppable tree to (tx,ty) within 'radius' tiles; 1 = found (*ox,*oy). */
int  econ_find_tree(int tx, int ty, int radius, int *ox, int *oy);
/* Nearest completed depot of 'player' accepting resource kind (W2_RES_*) from unit 'slot'; -1 none. */
int  econ_find_depot(int slot, int kind);
/* Number of workers currently inside a gold mine (0 for anything else). */
int  econ_mine_workers(int mine);

/* Messages: War Strings table 1 indices of the original texts. */
#define ECON_STR_NO_RETURN   384
#define ECON_STR_FOOD        386
#define ECON_STR_GOLD        387
#define ECON_STR_LUMBER      388
#define ECON_STR_OIL         389

#endif
