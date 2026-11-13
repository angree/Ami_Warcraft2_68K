/* aiprog.c - interpreter for the original's computer-player profile programs (see aiprog.h).
 *
 * Data: War Data entry 277 (9237 bytes in Mac 1.23, the same bytes as the PC data). Layout:
 *   32 x u16 LE offsets (profile 0..31) -> 4-byte header { u16 build list, u16 producer intervals }
 *   followed by the profile's first instruction. All offsets are from the start of the entry, jumps
 *   are absolute and programs may jump into shared code before their own header.
 * Instructions (no alignment, operands little-endian):
 *   00 ff vv        field ff = vv
 *   01 lo hi        jump
 *   02 b0 b1 b2 b3  sleep: counter = value (0 = go on in the same step)
 *   03 cc           wait for condition cc: not met -> stay on this instruction; met -> next. Both
 *                   end the step with counter = 1.
 * One "call" per game cycle: counter > 0 -> counter-- and nothing else; counter == 0 -> execute
 * until something sets the counter. We are called every AI decision with the number of elapsed
 * cycles and replay that many calls in one go (a sleep is skipped arithmetically; a wait that fails
 * ends the batch since the world snapshot cannot change inside it).
 * The data is read once (~9 KB, ordinary memory) and kept for the whole run.
 */
#include <string.h>

#include "w2.h"
#include "w2cfg.h"
#include "assets.h"
#include "aiprog.h"

#define AIP_ENTRY      277
#define AIP_PROFILES   32
#define AIP_MAX_OPS    256     /* instructions per call before a program counts as runaway */
#define AIP_TRACE_MAX  40      /* logged instructions per player and game */

typedef struct AipState {
	uint8_t  active, profile, traced;
	uint32_t counter;
	uint16_t pc, blist;
	uint8_t  blen;                         /* build list length (to 0xff, max 64) */
	uint8_t  f[AIP_NFIELDS];
} AipState;

static AipState aip[8];
static unsigned char *g_data;
static unsigned long g_size;
static int g_tried, g_trace;

/* research class 0x80.. -> upgrade id (human, orc); Mac table at A5+2C2E..+2ECA */
static const uint8_t aip_res[24][2] = {
	{ 4, 6 }, { 5, 7 }, { 24, 28 }, { 27, 31 }, { 25, 29 }, { 26, 30 },     /* 80 missile, rangers/berserkers */
	{ 0, 2 }, { 1, 3 }, { 8, 10 }, { 9, 11 }, { 22, 20 }, { 23, 21 },       /* 86 weapons, armour, siege */
	{ 12, 14 }, { 13, 15 }, { 16, 18 }, { 17, 19 },                         /* 8c ship cannon / armour */
	{ 33, 32 }, { 35, 44 }, { 36, 50 },                                      /* 90 paladin/ogre mage, church/altar */
	{ 37, 45 }, { 39, 48 }, { 40, 47 }, { 41, 49 }, { 42, 51 },              /* 93 mage tower / temple */
};

static unsigned rd16(unsigned o) { return (unsigned)g_data[o] | ((unsigned)g_data[o + 1] << 8); }
static uint32_t rd32(unsigned o)
{
	return (uint32_t)g_data[o] | ((uint32_t)g_data[o + 1] << 8) | ((uint32_t)g_data[o + 2] << 16) | ((uint32_t)g_data[o + 3] << 24);
}

static int load(void)
{
	if (g_data) return 1;
	if (g_tried || !assets_war(ASSET_DATA)) return 0;   /* archives not open yet: try again later */
	g_tried = 1;
	g_data = assets_load(ASSET_DATA, AIP_ENTRY, &g_size);
	if (g_data && g_size < AIP_PROFILES * 2 + 8) { war_free(g_data); g_data = NULL; }
	w2_log("aiprog: War Data %d %s (%lu bytes)", AIP_ENTRY, g_data ? "loaded" : "missing - fixed AI plan", g_size);
	return g_data != NULL;
}

void aiprog_set_trace(int on) { g_trace = on; }

static void trace(AipState *s, int p, const char *what, int a, long b)
{
	if (!g_trace || s->traced >= AIP_TRACE_MAX) return;
	s->traced++;
	w2_log("aiprog p%d c%lu prof %d: %s %d %ld", p, (unsigned long)w2.cycle, s->profile, what, a, b);
}

static int cond(int p, int c, const AiProgWorld *w)
{
	const AipState *s = &aip[p];
	switch (c) {
	case AIP_C_SHIPYARD: return w->shipyards != 0;
	case AIP_C_KEEP:     return w->keeps != 0;
	case AIP_C_CASTLE:   return w->castles != 0;
	case AIP_C_WORKERS:  return s->f[AIP_WORKERS] <= w->workers;
	case AIP_C_LAND:     return (unsigned)s->f[AIP_LAND_SIZE] * s->f[AIP_LAND_CNT] <= w->land;
	case AIP_C_SEA:      return (unsigned)s->f[AIP_SEA_SIZE] * s->f[AIP_SEA_CNT] <= w->sea;
	case AIP_C_AIR:      return (unsigned)s->f[AIP_AIR_SIZE] * s->f[AIP_AIR_CNT] <= w->air;
	case AIP_C_ENEMY_WORKER: return w->enemy_worker != 0;
	}
	return 0;
}

static void fail(int p, const char *why)
{
	w2_log("aiprog p%d: profile %d %s at %u - fixed AI plan from now", p, aip[p].profile, why, aip[p].pc);
	aip[p].active = 0;
}

/* one call; returns 0 when a wait was not met (the caller ends the batch) */
static int step(int p, const AiProgWorld *w)
{
	AipState *s = &aip[p];
	int n = 0;
	if (s->counter) { s->counter--; return 1; }
	while (s->counter == 0) {
		unsigned pc = s->pc, op;
		if (++n > AIP_MAX_OPS) { s->counter = 1; trace(s, p, "runaway at", (int)pc, 0); return 1; }
		if (pc >= g_size) { fail(p, "ran off the data"); return 0; }
		op = g_data[pc];
		switch (op) {
		case 0:
			if (pc + 3 > g_size) { fail(p, "truncated"); return 0; }
			if (g_data[pc + 1] >= 8 && g_data[pc + 1] < AIP_NFIELDS) s->f[g_data[pc + 1]] = g_data[pc + 2];
			trace(s, p, "set", g_data[pc + 1], g_data[pc + 2]);
			s->pc = (uint16_t)(pc + 3);
			break;
		case 1:
			if (pc + 3 > g_size) { fail(p, "truncated"); return 0; }
			s->pc = (uint16_t)rd16(pc + 1);
			break;
		case 2:
			if (pc + 5 > g_size) { fail(p, "truncated"); return 0; }
			s->counter = rd32(pc + 1);
			trace(s, p, "sleep", 0, (long)s->counter);
			s->pc = (uint16_t)(pc + 5);
			break;
		case 3:
			if (pc + 2 > g_size) { fail(p, "truncated"); return 0; }
			s->counter = 1;
			if (!cond(p, g_data[pc + 1], w)) return 0;
			trace(s, p, "wait passed", g_data[pc + 1], 0);
			s->pc = (uint16_t)(pc + 2);
			break;
		default:
			fail(p, "bad opcode");
			return 0;
		}
	}
	return 1;
}

int aiprog_start(int p, int profile, const AiProgWorld *w)
{
	AipState *s;
	unsigned h, i;
	if (p < 0 || p >= 8) return 0;
	s = &aip[p];
	memset(s, 0, sizeof *s);
	if (profile < 0 || profile >= AIP_PROFILES || !load()) return 0;
	h = rd16((unsigned)profile * 2);
	if (h + 4 > g_size) return 0;
	s->profile = (uint8_t)profile;
	s->blist = (uint16_t)rd16(h);
	s->pc = (uint16_t)(h + 4);
	s->f[AIP_BUILD_LIMIT] = 0xFF;          /* as the original's init; programs set it first thing */
	for (i = 0; i < 64 && s->blist + i < g_size && g_data[s->blist + i] != 0xFF; i++) ;
	s->blen = (uint8_t)i;
	s->active = 1;
	s->traced = AIP_TRACE_MAX;             /* the start block only sets defaults: one summary line */
	step(p, w);
	s->traced = 0;
	w2_log("aiprog p%d: profile %d, build list %u (%u entries), after start: pc %u workers %d limit %d",
	       p, profile, s->blist, i, s->pc, s->f[AIP_WORKERS], s->f[AIP_BUILD_LIMIT]);
	return s->active;
}

void aiprog_stop(int p) { if (p >= 0 && p < 8) aip[p].active = 0; }
int  aiprog_active(int p) { return p >= 0 && p < 8 && aip[p].active; }
int  aiprog_profile(int p) { return (p >= 0 && p < 8) ? aip[p].profile : 0; }
uint16_t aiprog_pc(int p) { return (p >= 0 && p < 8) ? aip[p].pc : 0; }

void aiprog_run(int p, uint32_t calls, const AiProgWorld *w)
{
	AipState *s;
	if (!aiprog_active(p)) return;
	s = &aip[p];
	while (calls > 0 && s->active) {
		if (s->counter) {
			uint32_t k = s->counter < calls ? s->counter : calls;
			s->counter -= k; calls -= k;
			continue;
		}
		calls--;
		if (!step(p, w)) break;
	}
}

int aiprog_get(int p, int field)
{
	if (p < 0 || p >= 8 || field < 0 || field >= AIP_NFIELDS) return 0;
	return aip[p].f[field];
}

void aiprog_set(int p, int field, int value)
{
	if (p < 0 || p >= 8 || field < 8 || field >= AIP_NFIELDS) return;
	aip[p].f[field] = (uint8_t)value;
}

int aiprog_build_list(int p, const uint8_t **list)
{
	int n;
	if (!aiprog_active(p)) { *list = NULL; return 0; }
	n = aip[p].blen;
	if (aip[p].f[AIP_BUILD_LIMIT] < n) n = aip[p].f[AIP_BUILD_LIMIT];
	*list = g_data + aip[p].blist;
	return n;
}

int aiprog_research(int entry, int race)
{
	if (entry == 0x98) return -2;
	if (entry == 0x99) return -3;
	if (entry < 0x80 || entry >= 0x80 + 24) return -1;
	return aip_res[entry - 0x80][race & 1];
}
