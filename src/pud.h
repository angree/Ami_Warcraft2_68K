#ifndef W2_PUD_H
#define W2_PUD_H
#include <stdint.h>
/* Built-in PUDs in "War Data": campaigns 192..219 (human even, orc odd), multiplayer/skirmish maps
 * 220..247 (the Mac lists 28 of them: f6_6300 0x00136300, map i = entry 220 + i). 248..251 are
 * small untitled 32x32 PUDs, not in the Mac's list. */
#define PUD_BUILTIN_SKIRMISH_FIRST 220
#define PUD_BUILTIN_SKIRMISH_LAST  247
int  pud_load_mem(const uint8_t *data, uint32_t size);   /* resets the world, loads map/players/units; a PUD without valid DIM/MTXM fails (-1) before the world is touched */
int  pud_load_file(const char *path);
int  pud_load_builtin(int war_data_entry);
void pud_reclassify_tile(int tx, int ty);                /* after changing w2.map.tile[] (tree cut, wall) */
/* 1 = the map's UNIT record had a non-zero parameter on this non-resource unit (Mac: "passive":
 * not counted in the AI forces, not picked for AI attacks, a combat unit of a computer player guards
 * its position). 0 for a unit created later in the slot (generation checked). */
int  pud_unit_passive(int slot);
void pud_describe(const uint8_t *data, uint32_t size, char *out, int cap, int *w, int *h);
#endif
