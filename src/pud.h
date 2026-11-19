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

/* Destructible terrain as on the Mac (DamageWallTile 0x001320A8, f13_1EA2, f13_1C3C; RE_NOTES §5). Walls are megatiles 16..101 in all
 * eras: 16..33 human / 34..51 orc intact, 52..87 damaged ("cracked", still a wall), 88..101 rubble
 * (plain land: walkable, buildable). The next stage of each wall megatile comes from War Data 6 / 22 / 14
 * (summer / winter / wasteland), the rock transitions from War Data 8 / 24 / 16. */
int  pud_wall_at(int tx, int ty);                        /* intact or damaged wall (W2_TF_WALL) */
/* DamageWallTile 0x001320A8: d = (dmg & 255) >> 2, at least 1, added to the tile's damage counter
 * (kept in w2.map.wood[] of wall tiles); 20 or more = the next stage (counter starts again at 0). */
void pud_wall_hit(int tx, int ty, int dmg);
/* f13_1EA2 0x00169EA2: one stage further (intact -> damaged -> rubble). A wall turning into rubble
 * also takes every INTACT orthogonal neighbour wall one stage further (it becomes damaged). */
void pud_wall_stage(int tx, int ty);
/* rock at (tx,ty) blown away (Mac FUN_00169b0a + f13_19FE, only demolition does it): the 3x3 tiles
 * around it are re-shaped by the era's rock table, the centre becomes buildable land. */
void pud_rock_remove(int tx, int ty);
/* 1 = the map's UNIT record had a non-zero parameter on this non-resource unit (Mac: "passive":
 * not counted in the AI forces, not picked for AI attacks, a combat unit of a computer player guards
 * its position). 0 for a unit created later in the slot (generation checked). */
int  pud_unit_passive(int slot);
void pud_describe(const uint8_t *data, uint32_t size, char *out, int cap, int *w, int *h);
#endif
