#ifndef W2_PUD_H
#define W2_PUD_H
#include <stdint.h>
/* Built-in PUDs in "War Data": campaigns 192..219 (human even, orc odd), skirmish maps 220..240+. */
#define PUD_BUILTIN_SKIRMISH_FIRST 220
#define PUD_BUILTIN_SKIRMISH_LAST  240
int  pud_load_mem(const uint8_t *data, uint32_t size);   /* resets the world, loads map/players/units */
int  pud_load_file(const char *path);
int  pud_load_builtin(int war_data_entry);
void pud_reclassify_tile(int tx, int ty);                /* after changing w2.map.tile[] (tree cut, wall) */
void pud_describe(const uint8_t *data, uint32_t size, char *out, int cap, int *w, int *h);
#endif
