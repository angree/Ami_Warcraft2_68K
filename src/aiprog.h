/* aiprog.h - the original's computer-player profile programs (owner: aiprog.c, used by ai.c).
 *
 * War Data entry 277 holds 32 small byte-code programs (PUD AIPL = profile number). Each one sets
 * the per-player AI parameters below over time: worker target, requested unit counts per class,
 * attack triggers with group size/count, and how far down its build list the AI may go.
 * Facts (Mac 1.23 binary, see docs/RE_NOTES.md): the interpreter steps once per game cycle for each
 * computer player; op 0 = set field (2 bytes), 1 = jump (abs16 LE), 2 = sleep (u32 LE),
 * 3 = wait condition (1 byte; not met: stays, met: next; both sleep 1).
 */
#ifndef W2_AIPROG_H
#define W2_AIPROG_H

#include <stdint.h>

/* field ids = byte offsets in the original's per-player record */
enum {
	AIP_ATT_LAND = 9, AIP_ATT_SEA = 10, AIP_ATT_AIR = 11,   /* one-shot attack triggers */
	AIP_STRATEGY = 12,
	AIP_LAND_SIZE = 13, AIP_LAND_CNT = 14, AIP_SEA_SIZE = 15, AIP_SEA_CNT = 16,
	AIP_AIR_SIZE = 17, AIP_AIR_CNT = 18,
	AIP_WORKERS = 19, AIP_INFANTRY = 20, AIP_ARCHERS = 21, AIP_SIEGE = 22, AIP_CAVALRY = 23,
	AIP_TANKERS = 24, AIP_DESTROYERS = 25, AIP_TRANSPORTS = 26, AIP_BATTLESHIPS = 27, AIP_SUBS = 28,
	AIP_CASTERS = 29, AIP_FLYERS = 30, AIP_SAPPERS = 31, AIP_DRAGONS = 32,
	AIP_AGGRESSION = 33, AIP_BUILD_LIMIT = 34,
	AIP_NFIELDS = 36
};

/* wait conditions */
enum { AIP_C_SHIPYARD, AIP_C_KEEP, AIP_C_CASTLE, AIP_C_WORKERS, AIP_C_LAND, AIP_C_SEA, AIP_C_AIR, AIP_C_ENEMY_WORKER };

/* what the conditions look at (filled by ai.c from its scan) */
typedef struct AiProgWorld {
	uint16_t shipyards, keeps, castles;    /* complete buildings of that kind */
	uint16_t workers;                      /* complete workers */
	uint16_t land, sea, air;               /* complete attack-capable units per domain (incl. attackers) */
	uint8_t  enemy_worker;                 /* some enemy player has a worker */
} AiProgWorld;

void aiprog_set_trace(int on);              /* log executed instructions (first lines per player) */
/* Start player p on profile 'profile' (PUD AIPL) and run its first step at once (as the original
 * does at game start). 0 = no program (data missing / bad profile): use the fixed plan. */
int  aiprog_start(int p, int profile, const AiProgWorld *w);
void aiprog_stop(int p);
int  aiprog_active(int p);
/* Advance by 'calls' interpreter steps (= game cycles elapsed) with this world snapshot. */
void aiprog_run(int p, uint32_t calls, const AiProgWorld *w);
int  aiprog_get(int p, int field);
void aiprog_set(int p, int field, int value);
/* The usable start of the build list: *list, returns min(limit, length, 64). Entries < 0x80 are
 * unit types (human id; | race bit 1 for orcs), 0x80.. are research classes (aiprog_research). */
int  aiprog_build_list(int p, const uint8_t **list);
/* research class entry (0x80..) -> upgrade id for race 0/1; 0x98/0x99 -> -2/-3 (hall to keep /
 * keep to castle); -1 = unknown */
int  aiprog_research(int entry, int race);
int  aiprog_profile(int p);
uint16_t aiprog_pc(int p);

#endif
