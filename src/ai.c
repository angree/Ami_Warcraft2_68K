/* ai.c - computer opponents ("land attack" style skirmish AI).
 *
 * Behaviour modelled on the original's default skirmish AI as documented by Wargus
 * scripts/ai/land_attack.lua and docs/SPEC_RULES.md section 14 (read as documentation only):
 *  - economy: keep workers busy (about 2/3 on gold), farms ahead of the food cap, workers up to a
 *    target that grows with the build order, barracks -> lumber mill -> blacksmith -> 2nd barracks ->
 *    keep -> stables/ogre mound -> castle, weapon/armour/missile research when affordable;
 *  - military: a melee/ranged/cavalry/siege mix gathered at a rally point near home, attack waves of
 *    growing size (attack-move to the nearest known enemy building, else an enemy start location),
 *    home defence (idle home army attacks enemies near home or whoever hurts our units there).
 *
 * With the original's data present, each computer player runs its PUD AIPL profile program
 * (aiprog.c, War Data 277) instead of the fixed plan for: worker target, build list (buildings,
 * research, hall upgrades), unit counts per class, and when/how many units attack (one-shot
 * land/sea/air triggers). Farms, worker jobs, hall rebuild/expansion, rally, defence and targets
 * stay ours. No data / bad profile / w2.cfg ai_programs=0: the fixed plan above.
 * Enemy units are seen only when visible to the player (or attacking one of its units).
 *
 * Transports (Mac 1.23 role 3, f8_2010 0x00142010, every 50 cycles there, in our think here): a land
 * attacker whose target is on another land region "needs transport" (Mac unit+0x5F bit 0, destination
 * region +0x5C, set by the role assigner 0x00141224) and waits. A transport that is idle or docked:
 *  - if units assigned to it are still walking to it: it waits (Mac 0x00141F5A; ours gives up after
 *    AI_TR_PATIENCE thinks and sails with what is aboard);
 *  - empty: up to 6 waiting units with no valid transport and one destination region are assigned and
 *    ordered to board (the first one's order makes the transport come and dock near it); nobody to
 *    carry -> it docks near the home hall (Mac f10_2E0A nearest town hall);
 *  - loaded: landing point = the passengers' target, with the program field +0x0C set a coast tile of
 *    the target's region found by straight lines in a random direction (AI_RandomDirSearch
 *    0x00141D06); escorts follow (Mac role 7; the original reads the escort count from +0x0F of the
 *    context of player 0,1,2.. - ours uses its own +0x0F); then it docks there and unloads (+0x1C
 *    bit 0x40). Landed units are attackers in their target's region again and attack-move on.
 *
 * Only the cmd_* command API and read-only world state are used. Each computer player thinks once
 * every AI_THINK_PERIOD cycles (players staggered); one think scans the unit array exactly once.
 * No allocation, no floats, randomness only through w2_rand().
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "w2.h"
#include "w2cfg.h"
#include "ai.h"
#include "aiprog.h"
#include "unit.h"
#include "pud.h"
#include "transport.h"
#include "move.h"

/* m68k-amigaos-gcc 6.5 -O1 crashes (ivopts ICE) when these per-unit helpers are inlined into scan() */
#define W2_AI_NOINLINE __attribute__((noinline))

#define AI_MAX_P     8
#define AI_MAX_PEND  3
#define AI_MAX_WORK  160
#define AI_MAX_ARMY  256
#define AI_MAX_ENEMY 128
#define AI_MAX_MINE  24
#define AI_MAX_HURT  8
#define AI_MAX_PROD  8
#define AI_SITE_R    18      /* build-site search radius (tiles) */
#define AI_HOME_R    12      /* enemies this close to home are a threat */
#define AI_ENEMY_R   24      /* enemies further than this from home are not tracked */
#define AI_ARMY_CAP  40
#define AI_PEND_TTL  3000    /* cycles a build order may take until the building appears */
#define AI_MAX_TRANS 8
#define AI_MAX_OIL   24
#define AI_SHORE_R   24      /* shore building site search radius around home (tiles) */
#define AI_TR_PATIENCE 8     /* thinks a transport waits for units walking to it (ours) */
#define AI_NTR_WAIT  50      /* thinks a unit waits for a transport that cannot come (no transport possible) */

enum { J_IDLE, J_GOLD, J_WOOD, J_BUSY };
enum { ROLE_HOME, ROLE_ATTACK };
enum { S_WORKERS, S_BUILD, S_HALL, S_RESEARCH };
enum { B_BARRACKS, B_MILL, B_SMITH, B_STABLES };
enum { U_W1, U_W2, U_A1, U_A2, U_R1, U_R2, U_S1, U_S2 };

/* ---- per race facts ------------------------------------------------------------------------- */
typedef struct AiRace {
	uint8_t hall[4];          /* [1] town hall, [2] keep, [3] castle */
	uint8_t farm, worker;
	uint8_t bld[4];           /* B_* */
	uint8_t melee, ranged, cavalry, siege;
	uint8_t upg[8];           /* U_* */
	uint8_t race_shipyard;
} AiRace;

static const AiRace ai_races[2] = {
	{ { 0, W2U_TOWN_HALL, W2U_KEEP, W2U_CASTLE }, W2U_FARM, W2U_PEASANT,
	  { W2U_HUMAN_BARRACKS, W2U_ELVEN_LUMBER_MILL, W2U_HUMAN_BLACKSMITH, W2U_STABLES },
	  W2U_FOOTMAN, W2U_ARCHER, W2U_KNIGHT, W2U_BALLISTA,
	  { W2UP_SWORD1, W2UP_SWORD2, W2UP_HUMAN_SHIELD1, W2UP_HUMAN_SHIELD2, W2UP_ARROW1, W2UP_ARROW2,
	    W2UP_BALLISTA1, W2UP_BALLISTA2 }, W2U_HUMAN_SHIPYARD },
	{ { 0, W2U_GREAT_HALL, W2U_STRONGHOLD, W2U_FORTRESS }, W2U_PIG_FARM, W2U_PEON,
	  { W2U_ORC_BARRACKS, W2U_TROLL_LUMBER_MILL, W2U_ORC_BLACKSMITH, W2U_OGRE_MOUND },
	  W2U_GRUNT, W2U_AXETHROWER, W2U_OGRE, W2U_CATAPULT,
	  { W2UP_BATTLE_AXE1, W2UP_BATTLE_AXE2, W2UP_ORC_SHIELD1, W2UP_ORC_SHIELD2, W2UP_THROWING_AXE1,
	    W2UP_THROWING_AXE2, W2UP_CATAPULT1, W2UP_CATAPULT2 }, W2U_ORC_SHIPYARD },
};

/* ---- build order (land attack). Steps are re-evaluated from the top on every think, so a lost
 * building is rebuilt. S_BUILD / S_HALL block the steps below them while unfinished (and reserve
 * their cost); S_WORKERS sets the worker target; S_RESEARCH is tried when affordable. ---------- */
typedef struct AiStep { uint8_t kind, what, n; } AiStep;
static const AiStep ai_order[] = {
	{ S_WORKERS, 0, 5 },
	{ S_BUILD, B_BARRACKS, 1 },
	{ S_WORKERS, 0, 8 },
	{ S_BUILD, B_MILL, 1 },
	{ S_WORKERS, 0, 10 },
	{ S_BUILD, B_SMITH, 1 },
	{ S_RESEARCH, U_W1, 0 },
	{ S_RESEARCH, U_A1, 0 },
	{ S_RESEARCH, U_R1, 0 },
	{ S_WORKERS, 0, 13 },
	{ S_BUILD, B_BARRACKS, 2 },
	{ S_HALL, 0, 2 },
	{ S_WORKERS, 0, 16 },
	{ S_BUILD, B_STABLES, 1 },
	{ S_RESEARCH, U_W2, 0 },
	{ S_RESEARCH, U_A2, 0 },
	{ S_RESEARCH, U_R2, 0 },
	{ S_WORKERS, 0, 20 },
	{ S_HALL, 0, 3 },
	{ S_RESEARCH, U_S1, 0 },
	{ S_RESEARCH, U_S2, 0 },
	{ S_BUILD, B_BARRACKS, 3 },
	{ S_WORKERS, 0, 25 },
};
#define AI_NSTEPS ((int)(sizeof ai_order / sizeof ai_order[0]))

static const uint8_t ai_waves[] = { 3, 5, 8, 12, 16, 20, 24 };

/* ---- persistent state ---------------------------------------------------------------------- */
typedef struct AiPend { int16_t builder; uint8_t type, tx, ty; uint32_t t; } AiPend;
typedef struct AiState {
	uint8_t active;
	uint8_t race;
	uint8_t wave, wave_extra;
	uint8_t visited;            /* bit q: enemy q's start location was visited and found empty */
	uint8_t site_fail;
	uint32_t shore_cool;        /* no shore site found: do not search again before this cycle */
	uint8_t prog;               /* 1 = driven by its profile program (aiprog.c) */
	uint32_t prog_cycle;        /* game cycle the program was last advanced to */
	int16_t home_tx, home_ty;
	int16_t att_tx, att_ty;     /* -1: no attack target */
	int16_t tree_tx, tree_ty;   /* cached wood tile, -1 none */
	AiPend  pend[AI_MAX_PEND];
	AiInfo  info;
} AiState;

static AiState  ai_st[AI_MAX_P];
static uint16_t ai_gen[W2_MAX_UNITS];   /* unit_gen() of the unit the per-slot data below belongs to */
static uint8_t  ai_own[W2_MAX_UNITS];   /* its owner + 1 (0 = nothing recorded) */
static uint16_t ai_hp[W2_MAX_UNITS];    /* hp at the owner's last think */
static uint8_t  ai_role[W2_MAX_UNITS];
static uint8_t  ai_retry[W2_MAX_UNITS];
static uint8_t  ai_ntr[W2_MAX_UNITS];    /* needs transport: destination land region, 0 = no (Mac +0x5F bit 0, +0x5C) */
static int16_t  ai_ntx[W2_MAX_UNITS], ai_nty[W2_MAX_UNITS];   /* its target point */
static uint16_t ai_tslot[W2_MAX_UNITS], ai_tgen[W2_MAX_UNITS];  /* transport assigned to it (Mac +0x64) */
static uint8_t  ai_twait[W2_MAX_UNITS];  /* transport: thinks waiting for walkers (Mac +0x60); unit: thinks waiting */
static uint8_t  ai_tfail[W2_MAX_UNITS];  /* transport: unloads that left passengers aboard (no room on the coast) */
static int      g_log;
static int      g_prog_on = 1;          /* w2.cfg ai_programs=0: always the fixed plan */
static uint8_t  ai_isprod[W2_NUM_UNIT_TYPES];   /* building trains one of the program's unit classes */

/* ---- per-think scan results (static: small stack) ------------------------------------------- */
static uint16_t s_done[W2_NUM_UNIT_TYPES];   /* complete units / buildings of each type */
static uint16_t s_cons[W2_NUM_UNIT_TYPES];   /* buildings under construction / hall upgrades in progress */
static uint16_t s_train[W2_NUM_UNIT_TYPES];  /* units being trained */
static int16_t  s_idle[W2_NUM_UNIT_TYPES];   /* an idle complete building of that type, -1 */
static uint32_t s_research[2];              /* upgrades in progress (bit per id) */
static uint16_t s_work[AI_MAX_WORK];  static uint8_t s_job[AI_MAX_WORK]; static int s_nwork;
static uint16_t s_army[AI_MAX_ARMY];  static int s_narmy;
static uint16_t s_enemy[AI_MAX_ENEMY]; static int s_nenemy;
static uint16_t s_mine[AI_MAX_MINE];  static int s_nmine;
static uint16_t s_hurt[AI_MAX_HURT];  static int s_nhurt;
static uint16_t s_prod[AI_MAX_PROD];  static int s_nprod;    /* idle barracks */
static uint16_t s_prodx[AI_MAX_PROD]; static int s_nprodx;   /* idle producers of any program class */
static uint16_t s_navy[AI_MAX_PROD * 2]; static int s_nnavy; /* own attack-capable ships */
static uint16_t s_trans[AI_MAX_TRANS]; static int s_ntrans;  /* own transports on the map */
static uint16_t s_tank[AI_MAX_TRANS * 2]; static int s_ntank;  /* own tankers on the map */
static uint16_t s_plat[AI_MAX_OIL]; static int s_nplat;        /* own complete oil platforms with oil */
static uint16_t s_oil[AI_MAX_OIL];  static int s_noil;         /* oil patches */
static uint16_t s_land, s_sea, s_air;  /* complete attack units per domain (the program's conditions) */
static uint16_t s_ewk;                 /* bit q: enemy q has a worker */
static uint16_t s_alive[W2_MAX_PLAYERS];
static uint16_t s_halls[8]; static int s_nhalls;
static int s_p, s_hall, s_hall_tier, s_upgrading, s_main_mine;
static int s_food_sup, s_food_used;
static int s_ebld, s_ebld_d;           /* nearest explored enemy building to home */
static int s_ngold, s_nwood;
static int32_t b_gold, b_wood, b_oil;  /* budget left this think */
static int32_t r_gold, r_wood, r_oil;  /* reserved for the blocking build-order item */

/* ---- helpers --------------------------------------------------------------------------------- */
static void ai_log(int p, const char *fmt, ...)
{
	char buf[160];
	va_list ap;
	if (!g_log) return;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	w2_log("ai p%d c%lu: %s", p, (unsigned long)w2.cycle, buf);
}

static const char *tname(int type)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	return t ? t->ident : "?";
}

static int iabs(int v) { return v < 0 ? -v : v; }
static int cheb(int ax, int ay, int bx, int by) { int dx = iabs(ax - bx), dy = iabs(ay - by); return dx > dy ? dx : dy; }

/* centre tile of a unit's footprint */
static void ucentre(int slot, int *x, int *y)
{
	const W2Unit *u = &w2.unit[slot];
	const w2_unit_type *t = w2_unit(u->type);
	int w = t ? t->tile_w : 1, h = t ? t->tile_h : 1;
	*x = u->tx + w / 2; *y = u->ty + h / 2;
}

static int udist_tile(int slot, int x, int y) { int cx, cy; ucentre(slot, &cx, &cy); return cheb(cx, cy, x, y); }

static int is_idle(const W2Unit *u) { return u->order == W2_ORD_NONE || u->order == W2_ORD_STILL; }

static int hall_tier_of(int type)
{
	switch (type) {
	case W2U_TOWN_HALL: case W2U_GREAT_HALL: return 1;
	case W2U_KEEP: case W2U_STRONGHOLD: return 2;
	case W2U_CASTLE: case W2U_FORTRESS: return 3;
	}
	return 0;
}

/* same storage rule as economy.c (bits for 0..31, upgrade_level[] above; free spells always known);
 * read directly so the AI test can link without economy.c */
static int researched(int p, int upg)
{
	if (upg < 0 || upg >= (int)W2_NUM_UPGRADES) return 0;
	if (w2_upgrades[upg].researched_at == W2_NONE) return 1;
	if (upg < 32) return (w2.player[p].upgrades >> upg) & 1u;
	return w2.player[p].upgrade_level[upg - 32] != 0;
}
static int researching(int upg) { return (s_research[upg >> 5] >> (upg & 31)) & 1u; }

static int pend_count(int type)
{
	int i, n = 0;
	for (i = 0; i < AI_MAX_PEND; i++)
		if (ai_st[s_p].pend[i].builder >= 0 && (type < 0 || ai_st[s_p].pend[i].type == type)) n++;
	return n;
}
static int total_of(int type) { return s_done[type] + s_cons[type] + s_train[type] + pend_count(type); }

/* spend from this think's budget; 'urgent' ignores the reservation of the blocking item */
static int can_spend(int g, int w, int o, int urgent)
{
	if (urgent) return b_gold >= g && b_wood >= w && b_oil >= o;
	return b_gold - r_gold >= g && b_wood - r_wood >= w && b_oil - r_oil >= o;
}
static void spend(int g, int w, int o) { b_gold -= g; b_wood -= w; b_oil -= o; }
static void reserve(int g, int w, int o) { if (!r_gold && !r_wood && !r_oil) { r_gold = g; r_wood = w; r_oil = o; } }

/* AI-side tech requirements (complete buildings), plus the economy's own check */
static int req_ok(int p, int type)
{
	const AiRace *r = &ai_races[ai_st[p].race];
	int ok = 1;
	if (type == r->ranged) ok = s_done[r->bld[B_MILL]] > 0;
	else if (type == r->siege) ok = s_done[r->bld[B_MILL]] > 0 && s_done[r->bld[B_SMITH]] > 0;
	else if (type == r->cavalry) ok = s_done[r->bld[B_STABLES]] > 0 && s_done[r->bld[B_SMITH]] > 0;
	else if (type == r->bld[B_BARRACKS] || type == r->bld[B_MILL] || type == r->bld[B_SMITH]) ok = s_hall_tier >= 1;
	else if (type == r->bld[B_STABLES]) ok = s_hall_tier >= 2;
	else if (type == r->hall[2]) ok = s_hall_tier == 1 && s_done[r->bld[B_BARRACKS]] > 0;
	else if (type == r->hall[3]) ok = s_hall_tier == 2 && s_done[r->bld[B_STABLES]] > 0 &&
	                                  s_done[r->bld[B_SMITH]] > 0 && s_done[r->bld[B_MILL]] > 0;
	return ok && econ_allowed_unit(p, type);
}

/* ---- the scan (split in small functions: keeps m68k gcc 6.5 -O1 happy) --------------------------- */
static void scan_own_building(int i, const W2Unit *u, const w2_unit_type *t, const AiRace *r)
{
	int tier = hall_tier_of(u->type);
	if (u->flags & W2_UF_UNDERCONST) { s_cons[u->type]++; return; }
	s_done[u->type]++;
	if ((t->flags & W2UF_OIL_PLATFORM) && u->res_left > 0 && s_nplat < AI_MAX_OIL) s_plat[s_nplat++] = (uint16_t)i;
	if (t->food > 0) s_food_sup += t->food;
	if (tier) {
		if (tier > s_hall_tier) s_hall_tier = tier;
		if (s_nhalls < 8) s_halls[s_nhalls++] = (uint16_t)i;
	}
	switch (u->order) {
	case W2_ORD_TRAIN:
		if (u->queue_type < W2_NUM_UNIT_TYPES) { s_train[u->queue_type]++; s_food_used++; }
		break;
	case W2_ORD_RESEARCH:
		if (u->queue_type < 64) s_research[u->queue_type >> 5] |= 1u << (u->queue_type & 31);
		break;
	case W2_ORD_UPGRADE_TO:
		s_upgrading = u->queue_type;
		if (u->queue_type < W2_NUM_UNIT_TYPES) s_cons[u->queue_type]++;
		break;
	default:
		if (is_idle(u)) {
			if (s_idle[u->type] < 0) s_idle[u->type] = (int16_t)i;
			if (u->type == r->bld[B_BARRACKS] && s_nprod < AI_MAX_PROD) s_prod[s_nprod++] = (uint16_t)i;
			if (ai_isprod[u->type] && s_nprodx < AI_MAX_PROD) s_prodx[s_nprodx++] = (uint16_t)i;
		}
	}
}

static int worker_job(const W2Unit *u)
{
	if (u->flags & W2_UF_INSIDE) return (u->res_kind == W2_RES_WOOD) ? J_WOOD : J_GOLD;
	if (is_idle(u)) return J_IDLE;
	if (u->order == W2_ORD_HARVEST) {
		if (u->target != W2_NO_UNIT && u->target < W2_MAX_UNITS && w2.unit[u->target].type == W2U_GOLD_MINE) return J_GOLD;
		return J_WOOD;
	}
	if (u->order == W2_ORD_RETURN) return (u->res_kind == W2_RES_WOOD) ? J_WOOD : J_GOLD;
	return J_BUSY;
}

static W2_AI_NOINLINE void scan_own(int p, int i, const W2Unit *u, const w2_unit_type *t, const AiRace *r)
{
	/* a new unit in a reused slot (or a captured one) starts with fresh AI data, never an old role */
	if (ai_own[i] != p + 1 || !unit_same(i, ai_gen[i])) {
		ai_own[i] = (uint8_t)(p + 1); ai_gen[i] = unit_gen(i);
		ai_hp[i] = u->hp; ai_role[i] = ROLE_HOME; ai_retry[i] = 0;
		ai_ntr[i] = 0; ai_tslot[i] = W2_NO_UNIT; ai_twait[i] = 0; ai_tfail[i] = 0;
	}
	if (u->hp < ai_hp[i] && s_nhurt < AI_MAX_HURT) s_hurt[s_nhurt++] = (uint16_t)i;
	ai_hp[i] = u->hp;
	if (u->flags & W2_UF_BUILDING) { scan_own_building(i, u, t, r); return; }
	if (t->food < 0) s_food_used += -t->food;
	/* a "passive" unit of the map (PUD UNIT parameter, Mac unit+0x5F bit 1) guards its own spot: not in
	 * the AI's army (no waves, rally or defence orders) nor in the class / force counters (Mac f6_1358) */
	if (!(t->flags & W2UF_WORKER) && pud_unit_passive(i)) return;
	s_done[u->type]++;
	if (t->flags & W2UF_WORKER) {
		int job = worker_job(u);
		if (job == J_GOLD) s_ngold++;
		if (job == J_WOOD) s_nwood++;
		if (s_nwork < AI_MAX_WORK) { s_work[s_nwork] = (uint16_t)i; s_job[s_nwork++] = (uint8_t)job; }
	} else if ((t->flags & W2UF_CAN_ATTACK) && (t->kind == W2_KIND_LAND || t->kind == W2_KIND_AIR) &&
	           !(t->flags & W2UF_VOLATILE)) {
		if (s_narmy < AI_MAX_ARMY) s_army[s_narmy++] = (uint16_t)i;
	} else if ((t->flags & W2UF_CAN_ATTACK) && t->kind == W2_KIND_SEA && !(u->flags & W2_UF_INSIDE)) {
		if (s_nnavy < AI_MAX_PROD * 2) s_navy[s_nnavy++] = (uint16_t)i;
	} else if ((t->flags & W2UF_TRANSPORT) && t->kind == W2_KIND_SEA && !(u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN))) {
		if (s_ntrans < AI_MAX_TRANS) s_trans[s_ntrans++] = (uint16_t)i;
	} else if ((t->flags & W2UF_TANKER) && !(u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN))) {
		if (s_ntank < AI_MAX_TRANS * 2) s_tank[s_ntank++] = (uint16_t)i;
	}
	/* the original's domain counters (Mac f6_1358): attack-capable, by movement class */
	if ((t->flags & W2UF_CAN_ATTACK) && (t->flags & 0x1Fu)) {
		if (t->flags & W2UF_SEA) s_sea++;
		else if (t->flags & W2UF_AIR) s_air++;
		else if (!(t->flags & W2UF_WORKER)) s_land++;
	}
}

static W2_AI_NOINLINE void scan_enemy(int p, int i, const W2Unit *u, int home_tx, int home_ty)
{
	if (u->flags & W2_UF_BUILDING) {
		int ix = u->ty * w2.map.w + u->tx, d;
		if (ix >= 0 && ix < W2_MAP_MAX * W2_MAP_MAX && ((w2.map.explored[ix] >> p) & 1u)) {
			d = udist_tile(i, home_tx, home_ty);
			if (d < s_ebld_d) { s_ebld_d = d; s_ebld = i; }
		}
	} else {
		const w2_unit_type *t = w2_unit(u->type);
		if (t && (t->flags & W2UF_WORKER)) s_ewk |= (uint16_t)(1u << u->owner);
		if (!(u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN)) && s_nenemy < AI_MAX_ENEMY &&
		    udist_tile(i, home_tx, home_ty) <= AI_ENEMY_R) {
			/* only what this player sees now, or a unit that is attacking one of ours (it just hit us) */
			int ix = u->ty * w2.map.w + u->tx, tg = u->target;
			if (((w2.map.visible[ix] >> p) & 1u) ||
			    (u->order == W2_ORD_ATTACK && tg < W2_MAX_UNITS && w2.unit[tg].owner == p))
				s_enemy[s_nenemy++] = (uint16_t)i;
		}
	}
}

/* home hall: the best tier among halls with a gold mine close by, else the best tier;
 * main mine: the nearest one to home */
static void scan_home(AiState *st)
{
	int i, j, best = 0x7FFF;
	for (j = 0; j < s_nhalls; j++) {
		int h = s_halls[j], hx, hy, md = 0x7FFF, score;
		ucentre(h, &hx, &hy);
		for (i = 0; i < s_nmine; i++) { int d = udist_tile(s_mine[i], hx, hy); if (d < md) md = d; }
		score = (md <= 10 ? 0 : 100) - 10 * hall_tier_of(w2.unit[h].type) + (md < 50 ? md : 50) / 10;
		if (score < best) { best = score; s_hall = h; }
	}
	if (s_hall >= 0) {
		int hx, hy;
		ucentre(s_hall, &hx, &hy);
		st->home_tx = (int16_t)hx; st->home_ty = (int16_t)hy;
	}
	best = 0x7FFF;
	{   /* a mine on another island cannot be walked to (Mac 5.7: the site must be in the worker's region) */
		int hr = transport_region(st->home_tx, st->home_ty);
		for (i = 0; i < s_nmine; i++) {
			const W2Unit *m = &w2.unit[s_mine[i]];
			int d = udist_tile(s_mine[i], st->home_tx, st->home_ty), r = 0, x, y;
			for (y = m->ty - 1; y <= m->ty + 3 && !r; y++)
				for (x = m->tx - 1; x <= m->tx + 3 && !r; x++)
					if (transport_region(x, y) == hr) r = 1;
			if (hr && !r) continue;
			if (d < best) { best = d; s_main_mine = s_mine[i]; }
		}
	}
}

static void scan(int p)
{
	AiState *st = &ai_st[p];
	const AiRace *r = &ai_races[st->race];
	int i, htx = st->home_tx, hty = st->home_ty;
	uint16_t emask = w2.player[p].enemies;

	s_p = p;
	memset(s_done, 0, sizeof s_done); memset(s_cons, 0, sizeof s_cons); memset(s_train, 0, sizeof s_train);
	memset(s_alive, 0, sizeof s_alive);
	for (i = 0; i < (int)W2_NUM_UNIT_TYPES; i++) s_idle[i] = -1;
	s_research[0] = s_research[1] = 0;
	s_nwork = s_narmy = s_nenemy = s_nmine = s_nhurt = s_nprod = s_nhalls = 0;
	s_nprodx = s_nnavy = s_ntrans = s_ntank = s_nplat = s_noil = 0; s_land = s_sea = s_air = 0; s_ewk = 0;
	s_hall = -1; s_hall_tier = 0; s_upgrading = 0; s_main_mine = -1;
	s_food_sup = s_food_used = 0; s_ebld = -1; s_ebld_d = 0x7FFF; s_ngold = s_nwood = 0;

	for (i = 0; i < w2.unit_hi; i++) {
		const W2Unit *u = &w2.unit[i];
		const w2_unit_type *t;
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & W2_UF_DYING)) continue;
		t = w2_unit(u->type);
		if (!t) continue;
		if (u->owner < W2_MAX_PLAYERS && u->owner != W2_NEUTRAL) s_alive[u->owner]++;
		if (u->type == W2U_GOLD_MINE) {
			if (u->res_left > 0 && s_nmine < AI_MAX_MINE) s_mine[s_nmine++] = (uint16_t)i;
		} else if (u->type == W2U_OIL_PATCH) {
			if (s_noil < AI_MAX_OIL) s_oil[s_noil++] = (uint16_t)i;
		} else if (u->owner == p) {
			scan_own(p, i, u, t, r);
		} else if (u->owner < W2_MAX_PLAYERS && ((emask >> u->owner) & 1u)) {
			scan_enemy(p, i, u, htx, hty);
		}
	}
	scan_home(st);
}

/* ---- build sites ------------------------------------------------------------------------------ */
static int tile_free_for_building(int p, int x, int y)
{
	int ix = y * w2.map.w + x;
	uint16_t f = w2.map.flags[ix], o;
	if (!(f & W2_TF_LAND)) return 0;
	if (f & (W2_TF_WATER | W2_TF_COAST | W2_TF_TREE | W2_TF_ROCK | W2_TF_WALL | W2_TF_BUILDING | W2_TF_NOBUILD | W2_TF_NOWALK)) return 0;
	o = w2.map.occ_ground[ix];
	if (o != W2_NO_UNIT && o < W2_MAX_UNITS && (w2.unit[o].flags & W2_UF_ALIVE) &&
	    ((w2.unit[o].flags & W2_UF_BUILDING) || w2.unit[o].owner != p)) return 0;
	return 1;
}

/* tiles strictly between two rectangles (chebyshev gap); 0 = touching or overlapping */
static int rect_gap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
	int gx = 0, gy = 0;
	if (bx >= ax + aw) gx = bx - (ax + aw); else if (ax >= bx + bw) gx = ax - (bx + bw); else gx = -1;
	if (by >= ay + ah) gy = by - (ay + ah); else if (ay >= by + bh) gy = ay - (by + bh); else gy = -1;
	if (gx < 0 && gy < 0) return -1;
	return gx > gy ? gx : gy;
}

int ai_site_ok(int p, int type, int tx, int ty)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	int w, h, x, y, i, hall;
	if (!t) return 0;
	w = t->tile_w; h = t->tile_h;
	hall = (t->flags & W2UF_STORE_HALL) != 0;
	if (tx < 1 || ty < 1 || tx + w > w2.map.w - 1 || ty + h > w2.map.h - 1) return 0;
	for (y = ty; y < ty + h; y++)
		for (x = tx; x < tx + w; x++)
			if (!tile_free_for_building(p, x, y)) return 0;
	/* keep a one-tile lane around every building */
	for (x = tx - 1; x <= tx + w; x++) {
		if (w2.map.flags[(ty - 1) * w2.map.w + x] & W2_TF_BUILDING) return 0;
		if (w2.map.flags[(ty + h) * w2.map.w + x] & W2_TF_BUILDING) return 0;
	}
	for (y = ty; y < ty + h; y++) {
		if (w2.map.flags[y * w2.map.w + tx - 1] & W2_TF_BUILDING) return 0;
		if (w2.map.flags[y * w2.map.w + tx + w] & W2_TF_BUILDING) return 0;
	}
	/* away from gold mines (halls: the original's 3-tile rule) */
	for (i = 0; i < s_nmine; i++) {
		const W2Unit *m = &w2.unit[s_mine[i]];
		const w2_unit_type *mt = w2_unit(m->type);
		if (rect_gap(tx, ty, w, h, m->tx, m->ty, mt->tile_w, mt->tile_h) < (hall ? 3 : 2)) return 0;
	}
	/* never in the lane between the home hall and its mine */
	if (!hall && s_hall >= 0 && s_main_mine >= 0) {
		const W2Unit *a = &w2.unit[s_hall], *m = &w2.unit[s_main_mine];
		const w2_unit_type *at = w2_unit(a->type), *mt = w2_unit(m->type);
		int x0 = a->tx < m->tx ? a->tx : m->tx, y0 = a->ty < m->ty ? a->ty : m->ty;
		int x1 = a->tx + at->tile_w > m->tx + mt->tile_w ? a->tx + at->tile_w : m->tx + mt->tile_w;
		int y1 = a->ty + at->tile_h > m->ty + mt->tile_h ? a->ty + at->tile_h : m->ty + mt->tile_h;
		if (cheb(a->tx, a->ty, m->tx, m->ty) <= 12 && rect_gap(tx, ty, w, h, x0, y0, x1 - x0, y1 - y0) < 0) return 0;
	}
	return 1;
}

int ai_find_site(int p, int type, int cx, int cy, int builder, int *otx, int *oty)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	int r, k, w, h, skip, fb = 0, fbx = 0, fby = 0;
	if (!t) return 0;
	w = t->tile_w; h = t->tile_h;
	skip = (p < AI_MAX_P) ? (ai_st[p].site_fail & 7) : 0;
	for (r = 0; r <= AI_SITE_R; r++) {
		int n = r ? 8 * r : 1;
		for (k = 0; k < n; k++) {
			int dx, dy, tx, ty;
			/* walk the ring: top row, right column, bottom row, left column */
			if (!r) { dx = 0; dy = 0; }
			else if (k < 2 * r) { dx = -r + k; dy = -r; }
			else if (k < 4 * r) { dx = r; dy = -r + (k - 2 * r); }
			else if (k < 6 * r) { dx = r - (k - 4 * r); dy = r; }
			else { dx = -r; dy = r - (k - 6 * r); }
			tx = cx + dx - w / 2; ty = cy + dy - h / 2;
			if (!ai_site_ok(p, type, tx, ty)) continue;
			if (skip > 0) { skip--; continue; }
			if (econ_place_ok(p, type, tx, ty, builder)) { *otx = tx; *oty = ty; return 1; }
			if (!fb) { fb = 1; fbx = tx; fby = ty; }
		}
	}
	if (fb) { *otx = fbx; *oty = fby; return 1; }   /* economy refused everything: own terrain test */
	return 0;
}

/* nearest worker to (x,y) that may be pulled off its job to build */
static int pick_builder(int x, int y)
{
	int i, best = -1, bd = 0x7FFF, pass;
	for (pass = 0; pass < 3 && best < 0; pass++)
		for (i = 0; i < s_nwork; i++) {
			int s = s_work[i], d;
			const W2Unit *u = &w2.unit[s];
			if (u->flags & W2_UF_INSIDE) continue;
			if (pass == 0 && s_job[i] != J_IDLE) continue;
			if (pass == 1 && (s_job[i] != J_WOOD || u->res_amount)) continue;
			if (pass == 2 && (s_job[i] != J_GOLD || u->res_amount)) continue;
			d = udist_tile(s, x, y);
			if (d < bd) { bd = d; best = i; }
		}
	return best;
}

static int issue_build(int p, int type, int wi, int tx, int ty);

/* order a building at a site near (cx,cy). 1 = issued, 0 = not possible now */
static int try_build(int p, int type, int cx, int cy, int urgent)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	int wi, b, tx, ty;
	if (!t || !req_ok(p, type)) return 0;
	if (!can_spend(t->gold, t->wood, t->oil, urgent)) { reserve(t->gold, t->wood, t->oil); return 0; }
	if (pend_count(-1) >= 2) return 0;
	wi = pick_builder(cx, cy);
	if (wi < 0) return 0;
	b = s_work[wi];
	if (!ai_find_site(p, type, cx, cy, b, &tx, &ty)) { ai_log(p, "no site for %s near %d,%d", tname(type), cx, cy); return 0; }
	return issue_build(p, type, wi, tx, ty);
}

/* worker s_work[wi] builds 'type' at (tx,ty). 1 = issued */
static int issue_build(int p, int type, int wi, int tx, int ty)
{
	AiState *st = &ai_st[p];
	const w2_unit_type *t = w2_unit((unsigned)type);
	int b = s_work[wi], i;
	cmd_build(b, type, tx, ty);
	if (w2.unit[b].order != W2_ORD_BUILD) {
		st->site_fail++;
		ai_log(p, "build %s at %d,%d by #%d REJECTED", tname(type), tx, ty, b);
		return 0;
	}
	for (i = 0; i < AI_MAX_PEND; i++)
		if (st->pend[i].builder < 0) {
			st->pend[i].builder = (int16_t)b; st->pend[i].type = (uint8_t)type;
			st->pend[i].tx = (uint8_t)tx; st->pend[i].ty = (uint8_t)ty; st->pend[i].t = w2.cycle;
			break;
		}
	if (s_job[wi] == J_GOLD) s_ngold--;
	if (s_job[wi] == J_WOOD) s_nwood--;
	s_job[wi] = J_BUSY;
	spend(t->gold, t->wood, t->oil);
	ai_log(p, "build %s at %d,%d by #%d", tname(type), tx, ty, b);
	return 1;
}

/* shore building (shipyard, foundry, refinery: water / coast boundary tiles, at least one of them a
 * coast of our land) near home: rings around home, our own fast terrain test, then the economy's */
static int shore_site(int p, int type, int builder, int *otx, int *oty)
{
	AiState *st = &ai_st[p];
	const w2_unit_type *t = w2_unit((unsigned)type);
	int w = t->tile_w, h = t->tile_h, cx = st->home_tx, cy = st->home_ty, hr = transport_region(cx, cy), r, k;
	for (r = 0; r <= AI_SHORE_R; r++) {
		int n = r ? 8 * r : 1;
		for (k = 0; k < n; k++) {
			int dx, dy, tx, ty, x, y, ok = 1, coast = 0;
			if (!r) { dx = 0; dy = 0; }
			else if (k < 2 * r) { dx = -r + k; dy = -r; }
			else if (k < 4 * r) { dx = r; dy = -r + (k - 2 * r); }
			else if (k < 6 * r) { dx = r - (k - 4 * r); dy = r; }
			else { dx = -r; dy = r - (k - 6 * r); }
			tx = cx + dx - w / 2; ty = cy + dy - h / 2;
			if (tx < 1 || ty < 1 || tx + w > w2.map.w - 1 || ty + h > w2.map.h - 1) continue;
			for (y = ty; y < ty + h && ok; y++)
				for (x = tx; x < tx + w && ok; x++) {
					int mt = move_terrain(x, y), i = y * w2.map.w + x;
					if ((mt & MT_LAND) || !(mt & (MT_WATER | MT_SHALLOW)) || (w2.map.flags[i] & W2_TF_BUILDING) ||
					    w2.map.occ_ground[i] != W2_NO_UNIT) ok = 0;
					else if ((mt & MT_SHALLOW) && transport_coast_of(x, y, hr)) coast = 1;
				}
			if (!ok || !coast) continue;
			if (econ_place_ok(p, type, tx, ty, builder)) { *otx = tx; *oty = ty; return 1; }
		}
	}
	return 0;
}

static int try_shore(int p, int type)
{
	AiState *st = &ai_st[p];
	const w2_unit_type *t = w2_unit((unsigned)type);
	int wi, tx, ty;
	if (!t || !req_ok(p, type)) return 0;
	if (!can_spend(t->gold, t->wood, t->oil, 1)) { reserve(t->gold, t->wood, t->oil); return 0; }
	if (pend_count(-1) >= 2 || w2.cycle < st->shore_cool) return 0;
	wi = pick_builder(st->home_tx, st->home_ty);
	if (wi < 0) return 0;
	if (!shore_site(p, type, s_work[wi], &tx, &ty)) {
		st->shore_cool = w2.cycle + 800;
		ai_log(p, "no shore site for %s", tname(type));
		return 0;
	}
	return issue_build(p, type, wi, tx, ty);
}

/* tankers (Mac 0x00152952): loaded -> back to a depot; empty -> the own platform with oil nearest to
 * home; none -> build a platform on the oil patch nearest to home */
static void tankers(int p)
{
	AiState *st = &ai_st[p];
	int k, building = 0, i;
	for (k = 0; k < s_ntank; k++) if (w2.unit[s_tank[k]].order == W2_ORD_BUILD) building = 1;
	for (k = 0; k < s_ntank; k++) {
		int s = s_tank[k], best = -1, bd = 0x7FFF;
		const W2Unit *u = &w2.unit[s];
		if (!is_idle(u)) continue;
		if (u->res_amount) { cmd_return(s); continue; }
		for (i = 0; i < s_nplat; i++) {
			int d = udist_tile(s_plat[i], st->home_tx, st->home_ty) + unit_dist_tiles(s_plat[i], s) / 2;
			if (d < bd) { bd = d; best = s_plat[i]; }
		}
		if (best >= 0) { cmd_harvest(s, best, w2.unit[best].tx, w2.unit[best].ty); continue; }
		if (building) continue;
		{
			int pt = st->race ? W2U_ORC_OIL_PLATFORM : W2U_HUMAN_OIL_PLATFORM;
			const w2_unit_type *t = w2_unit(pt);
			for (i = 0; i < s_noil; i++) {
				int d = udist_tile(s_oil[i], st->home_tx, st->home_ty);
				if (d < bd) { bd = d; best = s_oil[i]; }
			}
			if (best < 0 || !t || !can_spend(t->gold, t->wood, t->oil, 0)) continue;
			cmd_build(s, pt, w2.unit[best].tx, w2.unit[best].ty);
			if (w2.unit[s].order == W2_ORD_BUILD) {
				spend(t->gold, t->wood, t->oil);
				building = 1;
				ai_log(p, "tanker #%d builds an oil platform at %d,%d", s, w2.unit[best].tx, w2.unit[best].ty);
			}
		}
	}
}

static void check_pending(int p)
{
	AiState *st = &ai_st[p];
	int i;
	for (i = 0; i < AI_MAX_PEND; i++) {
		AiPend *pd = &st->pend[i];
		const W2Unit *u;
		const w2_unit_type *t;
		int ix, o, keep = 1;
		if (pd->builder < 0) continue;
		u = &w2.unit[pd->builder];
		ix = pd->ty * w2.map.w + pd->tx;
		o = w2.map.occ_ground[ix];
		if (!(u->flags & W2_UF_ALIVE) || u->owner != p || u->order != W2_ORD_BUILD) keep = 0;
		if (o != W2_NO_UNIT && o < W2_MAX_UNITS && w2.unit[o].type == pd->type && (w2.unit[o].flags & W2_UF_BUILDING)) keep = 0;
		if (w2.cycle - pd->t > AI_PEND_TTL) {
			keep = 0;
			if ((u->flags & W2_UF_ALIVE) && u->owner == p && u->order == W2_ORD_BUILD) cmd_stop(pd->builder);
			ai_log(p, "build %s at %d,%d timed out", tname(pd->type), pd->tx, pd->ty);
		}
		if (!keep) pd->builder = -1;
		else if ((t = w2_unit(pd->type)) != NULL) spend(t->gold, t->wood, t->oil);   /* not paid yet */
	}
}

/* ---- economy ---------------------------------------------------------------------------------- */
static int find_tree(int p)
{
	AiState *st = &ai_st[p];
	int r, k;
	if (st->tree_tx >= 0 && (w2.map.flags[st->tree_ty * w2.map.w + st->tree_tx] & W2_TF_TREE)) return 1;
	st->tree_tx = st->tree_ty = -1;
	for (r = 2; r <= 32; r++)
		for (k = 0; k < 8 * r; k++) {
			int dx, dy, x, y, j;
			if (k < 2 * r) { dx = -r + k; dy = -r; }
			else if (k < 4 * r) { dx = r; dy = -r + (k - 2 * r); }
			else if (k < 6 * r) { dx = r - (k - 4 * r); dy = r; }
			else { dx = -r; dy = r - (k - 6 * r); }
			x = st->home_tx + dx; y = st->home_ty + dy;
			if (x < 1 || y < 1 || x >= w2.map.w - 1 || y >= w2.map.h - 1) continue;
			if (!(w2.map.flags[y * w2.map.w + x] & W2_TF_TREE)) continue;
			for (j = 0; j < 8; j++) {   /* needs a walkable neighbour */
				static const int8_t nx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 }, ny[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
				uint16_t f = w2.map.flags[(y + ny[j]) * w2.map.w + x + nx[j]];
				if ((f & W2_TF_LAND) && !(f & (W2_TF_TREE | W2_TF_ROCK | W2_TF_WALL | W2_TF_WATER | W2_TF_BUILDING | W2_TF_NOWALK))) {
					st->tree_tx = (int16_t)x; st->tree_ty = (int16_t)y;
					return 1;
				}
			}
		}
	return 0;
}

static void assign_workers(int p)
{
	AiState *st = &ai_st[p];
	int i, nidle = 0, total, want_gold, have_tree, have_mine = s_main_mine >= 0 && s_hall >= 0;
	const w2_unit_type *mt;
	if (s_hall < 0) return;   /* nothing to return goods to */
	have_tree = find_tree(p);
	for (i = 0; i < s_nwork; i++) if (s_job[i] == J_IDLE) nidle++;
	total = s_ngold + s_nwood + nidle;
	want_gold = (total * 2 + 2) / 3;
	if (w2.player[p].wood < 300 && w2.player[p].gold > 1200) want_gold = total / 2;
	if (w2.player[p].wood > 1500 && w2.player[p].gold < 600) want_gold = (total * 4 + 4) / 5;
	if (w2.player[p].wood > 4000 && w2.player[p].wood > 4 * w2.player[p].gold) want_gold = total - 1 - total / 10;
	if (!have_tree) want_gold = total;
	if (!have_mine) want_gold = 0;
	mt = have_mine ? w2_unit(w2.unit[s_main_mine].type) : NULL;
	for (i = 0; i < s_nwork; i++) {
		int s = s_work[i];
		W2Unit *u = &w2.unit[s];
		if (s_job[i] != J_IDLE) continue;
		if (u->res_amount && u->res_kind) { cmd_return(s); s_job[i] = J_BUSY; continue; }
		if (have_mine && s_ngold < want_gold) {
			cmd_harvest(s, s_main_mine, w2.unit[s_main_mine].tx + mt->tile_w / 2, w2.unit[s_main_mine].ty + mt->tile_h / 2);
			s_job[i] = J_GOLD; s_ngold++;
			ai_log(p, "worker #%d -> gold mine #%d", s, s_main_mine);
		} else if (have_tree) {
			cmd_harvest(s, -1, st->tree_tx, st->tree_ty);
			s_job[i] = J_WOOD; s_nwood++;
			ai_log(p, "worker #%d -> wood %d,%d", s, st->tree_tx, st->tree_ty);
		}
	}
	/* rebalance one worker per think */
	if (have_tree && s_ngold > want_gold + 1) {
		for (i = 0; i < s_nwork; i++) {
			const W2Unit *u = &w2.unit[s_work[i]];
			if (s_job[i] == J_GOLD && u->order == W2_ORD_HARVEST && !(u->flags & W2_UF_INSIDE) && !u->res_amount) {
				cmd_harvest(s_work[i], -1, st->tree_tx, st->tree_ty);
				s_job[i] = J_WOOD; s_ngold--; s_nwood++;
				ai_log(p, "rebalance #%d gold -> wood", s_work[i]);
				break;
			}
		}
	} else if (have_mine && s_ngold + 1 < want_gold) {
		for (i = 0; i < s_nwork; i++) {
			const W2Unit *u = &w2.unit[s_work[i]];
			if (s_job[i] == J_WOOD && u->order == W2_ORD_HARVEST && !(u->flags & W2_UF_INSIDE) && !u->res_amount) {
				cmd_harvest(s_work[i], s_main_mine, w2.unit[s_main_mine].tx + mt->tile_w / 2, w2.unit[s_main_mine].ty + mt->tile_h / 2);
				s_job[i] = J_GOLD; s_ngold++; s_nwood--;
				ai_log(p, "rebalance #%d wood -> gold", s_work[i]);
				break;
			}
		}
	}
}

static int food_free(void) { return s_food_sup - s_food_used; }

static void try_research(int p, int upg)
{
	const w2_upgrade *g = &w2_upgrades[upg];
	int b, lvl1 = -1, i;
	const AiRace *r = &ai_races[ai_st[p].race];
	if (researched(p, upg) || researching(upg)) return;
	for (i = 0; i < 8; i += 2) if (r->upg[i + 1] == upg) lvl1 = r->upg[i];
	if (lvl1 >= 0 && !researched(p, lvl1)) return;
	if (g->researched_at >= W2_NUM_UNIT_TYPES) return;
	b = s_idle[g->researched_at];
	if (b < 0 || !econ_allowed_upgrade(p, upg)) return;
	if (!can_spend(g->gold, g->wood, g->oil, 0)) return;
	cmd_research(b, upg);
	spend(g->gold, g->wood, g->oil);
	s_idle[g->researched_at] = -1;
	s_research[upg >> 5] |= 1u << (upg & 31);
	ai_log(p, "research %s at #%d", g->name, b);
}

/* upgrade a hall of the previous tier to 'type' (keep / castle) */
static void hall_upgrade(int p, int type)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	/* the hall to upgrade is one of the previous tier (the home hall may be an expansion's
	 * lower-tier hall, or already upgraded); cmd_upgrade_to refuses anything else silently */
	int h = -1, i;
	if (!t) return;
	if (s_hall >= 0 && w2.unit[s_hall].type == t->upgrade_from && is_idle(&w2.unit[s_hall])) h = s_hall;
	for (i = 0; h < 0 && i < w2.unit_hi; i++) {
		const W2Unit *v = &w2.unit[i];
		if ((v->flags & W2_UF_ALIVE) && !(v->flags & W2_UF_UNDERCONST) && v->owner == p &&
		    v->type == t->upgrade_from && is_idle(v)) h = i;
	}
	if (h >= 0 && can_spend(t->gold, t->wood, t->oil, 1)) {
		cmd_upgrade_to(h, type);
		if (w2.unit[h].order == W2_ORD_UPGRADE_TO) {
			spend(t->gold, t->wood, t->oil);
			ai_log(p, "upgrade hall #%d -> %s", h, tname(type));
		}
	} else reserve(t->gold, t->wood, t->oil);
}

/* walk the build order; returns the worker target, *blocked = step index or -1 */
static int run_build_order(int p, int *blocked)
{
	AiState *st = &ai_st[p];
	const AiRace *r = &ai_races[st->race];
	int k, wt = 4;
	*blocked = -1;
	for (k = 0; k < AI_NSTEPS; k++) {
		const AiStep *s = &ai_order[k];
		switch (s->kind) {
		case S_WORKERS:   /* the build order waits until the workers are there (or in training) */
			wt = s->n;
			if (s_done[r->worker] + s_train[r->worker] < s->n) { *blocked = k; return wt; }
			break;
		case S_RESEARCH:
			try_research(p, r->upg[s->what]);
			break;
		case S_BUILD: {
			int type = r->bld[s->what];
			if (total_of(type) >= s->n) break;
			if (!req_ok(p, type)) break;              /* cannot yet: do not block on it */
			*blocked = k;
			try_build(p, type, st->home_tx, st->home_ty, 1);
			return wt;
		}
		case S_HALL:
			if (s_hall_tier >= s->n || s_upgrading) break;
			if (!req_ok(p, r->hall[s->n])) break;
			*blocked = k;
			hall_upgrade(p, r->hall[s->n]);
			return wt;
		}
	}
	return wt;
}

/* the program's build list (aiprog_build_list): entries are walked from the top like the original
 * (Mac FUN_00152546): a building entry is met while the player owns as many of that building as the
 * list has asked for so far (lost ones are rebuilt); the first unmet one that can be built now is
 * ordered and blocks the rest; a research entry blocks until it is done. Shore buildings go to a
 * coast near home (skipped while there is none); oil platforms are left to the tankers; a second town
 * hall (the expansion rule does that) or anything not allowed: skipped. */
static void run_prog_build(int p, int *blocked)
{
	static uint8_t need[W2_NUM_UNIT_TYPES];
	AiState *st = &ai_st[p];
	const AiRace *r = &ai_races[st->race];
	const uint8_t *bl;
	int k, n = aiprog_build_list(p, &bl);
	*blocked = -1;
	memset(need, 0, sizeof need);
	for (k = 0; k < n; k++) {
		int e = bl[k];
		if (e < 0x80) {
			int type = (e & 0xFE) | st->race, have;
			const w2_unit_type *t = w2_unit((unsigned)type);
			if (!t || !(t->flags & W2UF_BUILDING)) continue;
			need[type]++;
			if (hall_tier_of(type) == 1) {
				if (need[type] > 1) continue;
				have = s_nhalls + s_cons[r->hall[1]] + pend_count(r->hall[1]);
			} else have = total_of(type);
			if (have >= need[type]) continue;
			if (t->flags & W2UF_OIL_PLATFORM) continue;           /* the tankers build those */
			if (!req_ok(p, type)) continue;
			if (t->flags & W2UF_SHORE_BUILDING) {                /* no coast near home: skip it */
				if (w2.cycle < st->shore_cool) continue;
				*blocked = k;
				try_shore(p, type);
				return;
			}
			*blocked = k;
			try_build(p, type, st->home_tx, st->home_ty, 1);
			return;
		} else {
			int upg = aiprog_research(e, st->race);
			if (upg == -2 || upg == -3) {          /* town hall -> keep, keep -> castle */
				int tier = -upg;
				if (s_hall_tier >= tier) continue;
				*blocked = k;
				if (!s_upgrading && req_ok(p, r->hall[tier])) hall_upgrade(p, r->hall[tier]);
				return;
			}
			if (upg < 0 || researched(p, upg)) continue;
			if (!econ_allowed_upgrade(p, upg) && !researching(upg)) continue;   /* cannot: do not stall */
			*blocked = k;
			if (!researching(upg)) try_research(p, upg);
			return;
		}
	}
}

static void economy(int p, int *wt_out)
{
	AiState *st = &ai_st[p];
	const AiRace *r = &ai_races[st->race];
	int wt, blocked, i, workers, nhalls = 0, farm_soon, need;

	/* 1. no town hall at all: build one near the nearest gold mine (everything else waits) */
	if (s_hall < 0) {
		int hall = r->hall[1];
		*wt_out = 1;
		st->info.step = -1;
		if (s_cons[hall] + s_cons[r->hall[2]] + s_cons[r->hall[3]] + pend_count(hall) == 0) {
			int cx = st->home_tx, cy = st->home_ty;
			if (s_main_mine >= 0) ucentre(s_main_mine, &cx, &cy);
			try_build(p, hall, cx, cy, 1);
		}
		return;
	}

	/* 2. home mine exhausted / far away: a new town hall at the nearest mine with gold left */
	if (s_main_mine >= 0 && udist_tile(s_main_mine, st->home_tx, st->home_ty) > 10 &&
	    s_cons[r->hall[1]] + pend_count(r->hall[1]) == 0 && s_done[r->worker] >= 4) {
		int cx, cy;
		ucentre(s_main_mine, &cx, &cy);
		if (try_build(p, r->hall[1], cx, cy, 1)) ai_log(p, "expanding to gold mine #%d", s_main_mine);
	}

	/* 3. farms ahead of the food cap */
	need = 3 + s_nprod + 2 * (s_done[r->bld[B_BARRACKS]] > 0);
	farm_soon = (s_cons[r->farm] + pend_count(r->farm)) * 4;
	if (food_free() + farm_soon < need && s_food_sup < 200 && pend_count(r->farm) + s_cons[r->farm] < 2)
		try_build(p, r->farm, st->home_tx, st->home_ty, 1);

	/* 4. build order (buildings, hall upgrades, research): the profile program's list and worker
	 * target, else the fixed plan */
	if (st->prog && aiprog_active(p)) {
		run_prog_build(p, &blocked);
		wt = aiprog_get(p, AIP_WORKERS);
	} else wt = run_build_order(p, &blocked);
	st->info.step = blocked;
	*wt_out = wt;

	/* 5. workers */
	workers = s_done[r->worker] + s_train[r->worker];
	for (i = 1; i <= 3; i++) {
		int h = s_idle[r->hall[i]];
		const w2_unit_type *t = w2_unit(r->worker);
		if (h < 0) continue;
		nhalls++;
		if (workers >= wt || food_free() <= 0) break;
		if (!can_spend(t->gold, t->wood, t->oil, 1)) break;   /* workers are the income: never starved */
		cmd_train(h, r->worker);
		spend(t->gold, t->wood, t->oil);
		s_food_used++; workers++;
		ai_log(p, "train %s at #%d (%d/%d)", tname(r->worker), h, workers, wt);
	}
	(void)nhalls;
}

/* ---- military --------------------------------------------------------------------------------- */
/* the program's unit classes: field, trained type (human, orc), counted variant (human, orc) */
typedef struct AiClass { uint8_t field, type[2], alt[2]; } AiClass;
static const AiClass ai_classes[] = {
	{ AIP_INFANTRY,    { W2U_FOOTMAN, W2U_GRUNT },                       { W2U_FOOTMAN, W2U_GRUNT } },
	{ AIP_ARCHERS,     { W2U_ARCHER, W2U_AXETHROWER },                   { W2U_RANGER, W2U_BERSERKER } },
	{ AIP_SIEGE,       { W2U_BALLISTA, W2U_CATAPULT },                   { W2U_BALLISTA, W2U_CATAPULT } },
	{ AIP_CAVALRY,     { W2U_KNIGHT, W2U_OGRE },                         { W2U_PALADIN, W2U_OGRE_MAGE } },
	{ AIP_TANKERS,     { W2U_HUMAN_OIL_TANKER, W2U_ORC_OIL_TANKER },     { W2U_HUMAN_OIL_TANKER, W2U_ORC_OIL_TANKER } },
	{ AIP_DESTROYERS,  { W2U_HUMAN_DESTROYER, W2U_ORC_DESTROYER },       { W2U_HUMAN_DESTROYER, W2U_ORC_DESTROYER } },
	{ AIP_TRANSPORTS,  { W2U_HUMAN_TRANSPORT, W2U_ORC_TRANSPORT },       { W2U_HUMAN_TRANSPORT, W2U_ORC_TRANSPORT } },
	{ AIP_BATTLESHIPS, { W2U_BATTLESHIP, W2U_OGRE_JUGGERNAUGHT },        { W2U_BATTLESHIP, W2U_OGRE_JUGGERNAUGHT } },
	{ AIP_SUBS,        { W2U_HUMAN_SUBMARINE, W2U_ORC_SUBMARINE },       { W2U_HUMAN_SUBMARINE, W2U_ORC_SUBMARINE } },
	{ AIP_CASTERS,     { W2U_MAGE, W2U_DEATH_KNIGHT },                   { W2U_MAGE, W2U_DEATH_KNIGHT } },
	{ AIP_FLYERS,      { W2U_BALLOON, W2U_ZEPPELIN },                    { W2U_BALLOON, W2U_ZEPPELIN } },
	{ AIP_SAPPERS,     { W2U_DWARVES, W2U_GOBLIN_SAPPERS },              { W2U_DWARVES, W2U_GOBLIN_SAPPERS } },
	{ AIP_DRAGONS,     { W2U_GRYPHON_RIDER, W2U_DRAGON },                { W2U_GRYPHON_RIDER, W2U_DRAGON } },
};
#define AI_NCLASSES ((int)(sizeof ai_classes / sizeof ai_classes[0]))

static int class_have(const AiClass *c, int race)
{
	int a = c->type[race], b = c->alt[race];
	return s_done[a] + s_train[a] + (b != a ? s_done[b] + s_train[b] : 0);
}

/* every idle producer trains the class furthest below the program's request (Mac: per-building AI
 * compares the class counter with the field, FUN_001284d4..) */
static void train_prog(int p)
{
	int race = ai_st[p].race, i, k;
	for (i = 0; i < s_nprodx; i++) {
		int b = s_prodx[i], best = -1, bdef = 0;
		const w2_unit_type *t;
		if (s_narmy >= AI_ARMY_CAP || food_free() <= 0) return;
		for (k = 0; k < AI_NCLASSES; k++) {
			const AiClass *c = &ai_classes[k];
			int ty = c->type[race], def = aiprog_get(p, c->field) - class_have(c, race);
			const w2_unit_type *tt = w2_unit((unsigned)ty);
			if (def <= bdef || !tt || tt->built_by != w2.unit[b].type) continue;
			if (!req_ok(p, ty) || !can_spend(tt->gold, tt->wood, tt->oil, 0)) continue;
			bdef = def; best = ty;
		}
		if (best < 0) continue;
		t = w2_unit((unsigned)best);
		cmd_train(b, best);
		if (w2.unit[b].order != W2_ORD_TRAIN) continue;
		spend(t->gold, t->wood, t->oil);
		s_train[best]++; s_food_used++;
		ai_log(p, "train %s at #%d (program wants %d)", tname(best), b, bdef);
	}
}

static void train_army(int p)
{
	const AiRace *r = &ai_races[ai_st[p].race];
	static const uint8_t weight[4] = { 3, 2, 4, 1 };
	int i, k, army = s_narmy;
	for (i = 0; i < s_nprod; i++) {
		int types[4], best = -1, bscore = 0x7FFFFFFF;
		const w2_unit_type *t;
		if (army >= AI_ARMY_CAP || food_free() <= 0) return;
		types[0] = r->melee; types[1] = r->ranged; types[2] = r->cavalry; types[3] = r->siege;
		for (k = 0; k < 4; k++) {
			int ty = types[k], score;
			const w2_unit_type *tt = w2_unit((unsigned)ty);
			if (!req_ok(p, ty)) continue;
			if (k == 3 && army < 6) continue;
			if (!can_spend(tt->gold, tt->wood, tt->oil, 0)) continue;
			score = (s_done[ty] + s_train[ty]) * 100 / weight[k];
			if (score < bscore) { bscore = score; best = ty; }
		}
		if (best < 0) return;
		t = w2_unit((unsigned)best);
		cmd_train(s_prod[i], best);
		spend(t->gold, t->wood, t->oil);
		s_train[best]++; s_food_used++; army++;
		ai_log(p, "train %s at #%d (army %d)", tname(best), s_prod[i], army);
	}
}

/* target for an attack: nearest explored enemy building, else the nearest unvisited enemy start */
static int pick_target(int p, int *tx, int *ty)
{
	AiState *st = &ai_st[p];
	int q, bd = 0x7FFF, found = 0;
	if (s_ebld >= 0) { ucentre(s_ebld, tx, ty); return 1; }
	for (q = 0; q < AI_MAX_P; q++) {
		const W2Player *e = &w2.player[q];
		int d;
		if (q == p || !((w2.player[p].enemies >> q) & 1u) || e->control == W2_CTRL_NOBODY || e->defeated) continue;
		if (!s_alive[q] || e->start_tx == 0xFFFF || ((st->visited >> q) & 1u)) continue;
		d = cheb(st->home_tx, st->home_ty, e->start_tx, e->start_ty);
		if (d < bd) { bd = d; *tx = e->start_tx; *ty = e->start_ty; found = 1; }
	}
	return found;
}

int ai_wave_size(int n)
{
	int last = (int)sizeof ai_waves - 1;
	return ai_waves[n < last ? n : last];
}
static int wave_reserve(int wave) { return wave >= 2 ? 2 : 0; }

static int find_threat(int p)
{
	AiState *st = &ai_st[p];
	int i, j, best = -1, bd = 0x7FFF;
	for (i = 0; i < s_nenemy; i++) {
		int e = s_enemy[i], d;
		const w2_unit_type *t = w2_unit(w2.unit[e].type);
		if (!(t->flags & W2UF_CAN_ATTACK) || (t->flags & W2UF_WORKER)) continue;
		d = udist_tile(e, st->home_tx, st->home_ty);
		if (d <= AI_HOME_R && d < bd) { bd = d; best = e; }
	}
	if (best >= 0) return best;
	for (j = 0; j < s_nhurt; j++) {   /* whoever hurts our units near home */
		int hx, hy;
		ucentre(s_hurt[j], &hx, &hy);
		if (cheb(hx, hy, st->home_tx, st->home_ty) > AI_ENEMY_R) continue;
		for (i = 0; i < s_nenemy; i++) {
			int d = udist_tile(s_enemy[i], hx, hy);
			if (d <= 8 && d < bd) { bd = d; best = s_enemy[i]; }
		}
	}
	return best;
}

/* the program's one-shot attack triggers (Mac AI_Tick50a, every 50 cycles there): each sends 'count'
 * groups of 'size' units of its domain that are not attacking yet, then the trigger is cleared. Kept
 * while home is under threat (our addition: defence first). Returns the land/air units sent. */
/* ---- transports ------------------------------------------------------------------------------ */
static int land_kind(int s) { const w2_unit_type *t = w2_unit(w2.unit[s].type); return t && t->kind == W2_KIND_LAND; }

/* attack-move an attacker to (tx,ty), or mark it "needs transport" when the point is on another land
 * region (Mac role assigner 0x00141224); carried units are left alone until they land */
static void send_attacker(int s, int tx, int ty)
{
	const W2Unit *u = &w2.unit[s];
	int rt, ru;
	if (u->flags & W2_UF_INSIDE) return;
	if (land_kind(s)) {
		rt = transport_region(tx, ty); ru = transport_region(u->tx, u->ty);
		if (rt && ru && rt != ru) {
			if (ai_ntr[s] != rt) { ai_ntr[s] = (uint8_t)rt; ai_twait[s] = 0; ai_tslot[s] = W2_NO_UNIT; }
			ai_ntx[s] = (int16_t)tx; ai_nty[s] = (int16_t)ty;
			return;
		}
	}
	ai_ntr[s] = 0;
	cmd_attack_move(s, tx, ty);
}

/* this player can get a transport (has one, trains one, or its program asks for one) */
static int can_ship(int p)
{
	const AiRace *r = &ai_races[ai_st[p].race];
	int tt = r->race_shipyard == W2U_HUMAN_SHIPYARD ? W2U_HUMAN_TRANSPORT : W2U_ORC_TRANSPORT;
	if (s_ntrans || s_train[tt]) return 1;
	return ai_st[p].prog && aiprog_active(p) && aiprog_get(p, AIP_TRANSPORTS) > 0;
}

static int tr_assigned(int s)
{
	int t = ai_tslot[s];
	return t != W2_NO_UNIT && unit_same(t, ai_tgen[s]) && w2.unit[t].owner == w2.unit[s].owner;
}

static void transports(int p)
{
	AiState *st = &ai_st[p];
	int k;
	for (k = 0; k < s_ntrans; k++) {
		int t = s_trans[k], n, i, walk = -1, wd = 0x7FFF;
		const W2Unit *ut = &w2.unit[t];
		if (!(is_idle(ut) || transport_waiting(t))) continue;     /* Mac: only in order 2 or 32 */
		n = transport_count(t);
		/* units assigned to it that are not aboard yet (Mac 0x00141F5A) */
		for (i = 0; i < s_narmy; i++) {
			int s = s_army[i], d;
			if (ai_tslot[s] != t || !tr_assigned(s) || (w2.unit[s].flags & W2_UF_INSIDE) || !ai_ntr[s] ||
			    transport_region(w2.unit[s].tx, w2.unit[s].ty) == ai_ntr[s]) continue;
			d = unit_dist_tiles(s, t);
			if (d < wd) { wd = d; walk = s; }
		}
		if (walk >= 0) {
			if (ai_twait[t] < AI_TR_PATIENCE) {
				ai_twait[t]++;
				if (w2.unit[walk].order != W2_ORD_LOAD) transport_cmd_board(walk, t);
				continue;
			}
			for (i = 0; i < s_narmy; i++)          /* gave up on the stragglers */
				if (ai_tslot[s_army[i]] == t && !(w2.unit[s_army[i]].flags & W2_UF_INSIDE)) ai_tslot[s_army[i]] = W2_NO_UNIT;
			if (!n) continue;
		}
		ai_twait[t] = 0;
		if (!n) {
			int dest = 0, got = 0;
			for (i = 0; i < s_narmy && got < TR_CAP; i++) {
				int s = s_army[i];
				const W2Unit *u = &w2.unit[s];
				if (!ai_ntr[s] || (u->flags & W2_UF_INSIDE) || u->order == W2_ORD_LOAD || tr_assigned(s)) continue;
				if (!dest) dest = ai_ntr[s];
				else if (ai_ntr[s] != dest) continue;
				ai_tslot[s] = (uint16_t)t; ai_tgen[s] = unit_gen(t);
				transport_cmd_board(s, t);        /* the first one brings the transport to its coast */
				got++;
			}
			if (got) {
				ai_log(p, "transport #%d at %d,%d: %d units to board (region %d), it %s", t, ut->tx, ut->ty, got, dest,
				       ut->order == W2_ORD_LOAD ? "docks" : "cannot dock");
				continue;
			}
			if (s_hall >= 0) {                      /* nothing to carry: wait at the home coast */
				int hx = w2.unit[s_hall].tx, hy = w2.unit[s_hall].ty, hr = transport_region(hx, hy);
				if (!(transport_docked(t) && transport_coast_of(ut->tx, ut->ty, hr)) && is_idle(ut))
					transport_cmd_dock(t, hx, hy, 0);
			}
			continue;
		}
		{
			uint16_t list[TR_CAP];
			int m = transport_list(t, list), tx = -1, ty = -1, lx, ly, esc, e, mode = 1;
			for (i = 0; i < m && tx < 0; i++) if (ai_ntr[list[i]]) { tx = ai_ntx[list[i]]; ty = ai_nty[list[i]]; }
			if (tx < 0) { if (st->att_tx < 0) continue; tx = st->att_tx; ty = st->att_ty; }
			lx = tx; ly = ty;
			if (transport_docked(t) && transport_coast_of(ut->tx, ut->ty, transport_region(tx, ty))) {
				/* unloaded here already and some are still aboard: no room - once more here (the first
				 * ones have walked off by now), then another coast of that region (ours) */
				if (++ai_tfail[t] >= 2) {
					for (e = 0; e < 4; e++)
						if (transport_find_landing(t, tx, ty, &lx, &ly) && cheb(lx, ly, ut->tx, ut->ty) > 4) { mode = 2; break; }
					ai_tfail[t] = 0;
				}
			} else {
				ai_tfail[t] = 0;
				/* program field +0x0C (1 unless the program clears it): a random coast of the target region */
				if (!st->prog || !aiprog_active(p) || aiprog_get(p, AIP_STRATEGY)) transport_find_landing(t, tx, ty, &lx, &ly);
			}
			if (!transport_cmd_dock(t, lx, ly, mode) && !transport_cmd_dock(t, tx, ty, 1)) {
				ai_log(p, "transport #%d: no landing near %d,%d", t, tx, ty);
				continue;
			}
			if (ai_tfail[t]) continue;           /* unloading again where it is */
			esc = (st->prog && aiprog_active(p)) ? aiprog_get(p, AIP_SEA_SIZE) : 0;
			for (e = 0; e < s_nnavy && esc > 0; e++)
				if (is_idle(&w2.unit[s_navy[e]])) { cmd_follow(s_navy[e], t); esc--; }
			ai_log(p, "transport #%d at %d,%d%s: %d aboard (%s), sails to land at %d,%d (target %d,%d)", t, ut->tx, ut->ty,
			       transport_docked(t) ? " docked" : "", m, tname(w2.unit[list[0]].type), w2.unit[t].goal_tx, w2.unit[t].goal_ty, tx, ty);
		}
	}
}

static int prog_waves(int p, int threat, int have_t, int tx, int ty)
{
	static const uint8_t trig[3] = { AIP_ATT_LAND, AIP_ATT_SEA, AIP_ATT_AIR };
	static const char *const dname[3] = { "land", "sea", "air" };
	AiState *st = &ai_st[p];
	int d, sent = 0;
	for (d = 0; d < 3; d++) {
		int want, n = 0, i;
		if (!aiprog_get(p, trig[d])) continue;
		if (threat >= 0 && d != 1) continue;
		aiprog_set(p, trig[d], 0);
		want = aiprog_get(p, AIP_LAND_SIZE + 2 * d) * aiprog_get(p, AIP_LAND_CNT + 2 * d);
		if (!have_t || want <= 0) { ai_log(p, "program %s attack: nothing to send (%d) / no target", dname[d], want); continue; }
		if (d == 1) {
			for (i = 0; i < s_nnavy && n < want; i++)
				if (is_idle(&w2.unit[s_navy[i]])) { cmd_attack_move(s_navy[i], tx, ty); n++; }
		} else {
			for (i = 0; i < s_narmy && n < want; i++) {
				int s = s_army[i];
				const W2Unit *u = &w2.unit[s];
				const w2_unit_type *t = w2_unit(u->type);
				if (ai_role[s] != ROLE_HOME || !t || t->kind != (d == 0 ? W2_KIND_LAND : W2_KIND_AIR)) continue;
				if (u->order == W2_ORD_ATTACK && u->target != W2_NO_UNIT) continue;   /* fighting at home */
				ai_role[s] = ROLE_ATTACK; ai_retry[s] = 0;
				send_attacker(s, tx, ty);
				n++;
			}
			sent += n;
		}
		ai_log(p, "WAVE %d (program, %s): %d of %d units attack-move -> %d,%d", st->wave + 1, dname[d], n, want, tx, ty);
		if (n && d != 1) { st->wave++; st->att_tx = (int16_t)tx; st->att_ty = (int16_t)ty; }
	}
	return sent;
}

static void military(int p)
{
	AiState *st = &ai_st[p];
	int i, threat, home_idle = 0, attackers = 0, tx = -1, ty = -1, have_t, rx, ry, need, landed = 0;

	threat = find_threat(p);
	st->info.threat = threat;
	if (threat >= 0) {
		int n = 0;
		for (i = 0; i < s_narmy; i++) {
			int s = s_army[i];
			const W2Unit *u = &w2.unit[s];
			if (ai_role[s] != ROLE_HOME) continue;
			if (u->order == W2_ORD_ATTACK && u->target != W2_NO_UNIT) continue;   /* already fighting */
			cmd_attack(s, threat);
			n++;
		}
		if (n) ai_log(p, "defend: %d units -> %s #%d at %d,%d", n, tname(w2.unit[threat].type), threat,
		              w2.unit[threat].tx, w2.unit[threat].ty);
	}

	have_t = pick_target(p, &tx, &ty);
	/* rally point: 5 tiles from home toward the enemy */
	rx = st->home_tx; ry = st->home_ty;
	if (have_t) { rx += (tx > rx) ? 5 : (tx < rx ? -5 : 0); ry += (ty > ry) ? 5 : (ty < ry ? -5 : 0); }
	if (rx < 1) rx = 1;
	if (ry < 1) ry = 1;
	if (rx > w2.map.w - 2) rx = w2.map.w - 2;
	if (ry > w2.map.h - 2) ry = w2.map.h - 2;
	{   /* on an island the point toward the enemy may be in the sea: back toward home until on our land */
		int hr = transport_region(st->home_tx, st->home_ty), k;
		for (k = 0; k < 6 && hr && transport_region(rx, ry) != hr; k++) {
			rx += (st->home_tx > rx) - (st->home_tx < rx);
			ry += (st->home_ty > ry) - (st->home_ty < ry);
		}
	}

	/* attackers: new target, or reached it, or stuck */
	if (have_t && (tx != st->att_tx || ty != st->att_ty) && st->att_tx >= 0) {
		ai_log(p, "attack target -> %d,%d", tx, ty);
	}
	for (i = 0; i < s_narmy; i++) {
		int s = s_army[i];
		const W2Unit *u = &w2.unit[s];
		if (ai_role[s] != ROLE_ATTACK) continue;
		attackers++;
		if (!is_idle(u)) continue;
		if (ai_ntr[s] && transport_region(u->tx, u->ty) == ai_ntr[s]) { landed++; ai_tslot[s] = W2_NO_UNIT; }
		if (ai_ntr[s] && land_kind(s) && transport_region(u->tx, u->ty) != ai_ntr[s]) {   /* waits for a transport */
			if (can_ship(p)) ai_twait[s] = 0;
			else if (++ai_twait[s] > AI_NTR_WAIT) { ai_ntr[s] = 0; ai_role[s] = ROLE_HOME; cmd_move(s, rx, ry); }
			continue;
		}
		if (st->att_tx >= 0 && udist_tile(s, st->att_tx, st->att_ty) <= 3 && s_ebld < 0) {
			int q;   /* reached an enemy start location with nothing known there */
			for (q = 0; q < AI_MAX_P; q++)
				if (w2.player[q].start_tx != 0xFFFF && cheb(w2.player[q].start_tx, w2.player[q].start_ty, st->att_tx, st->att_ty) <= 3 &&
				    !((st->visited >> q) & 1u)) {
					st->visited |= (uint8_t)(1u << q);
					ai_log(p, "start of player %d at %d,%d is empty", q, st->att_tx, st->att_ty);
				}
			have_t = pick_target(p, &tx, &ty);
		}
		if (!have_t || ai_retry[s] > 6) {
			int hr = transport_region(st->home_tx, st->home_ty);
			ai_retry[s] = 0;
			if (hr && land_kind(s) && transport_region(u->tx, u->ty) != hr) continue;   /* landed abroad: stays */
			ai_role[s] = ROLE_HOME; ai_ntr[s] = 0; ai_tslot[s] = W2_NO_UNIT;
			cmd_move(s, rx, ry);
			continue;
		}
		if (tx == st->att_tx && ty == st->att_ty) ai_retry[s]++;
		send_attacker(s, tx, ty);
	}
	if (landed) ai_log(p, "%d attackers landed on the target's island", landed);
	if (have_t && attackers) { st->att_tx = (int16_t)tx; st->att_ty = (int16_t)ty; }
	if (!attackers) st->att_tx = st->att_ty = -1;

	/* idle home army */
	for (i = 0; i < s_narmy; i++)
		if (ai_role[s_army[i]] == ROLE_HOME && is_idle(&w2.unit[s_army[i]])) home_idle++;

	/* launch a wave: the program's attack triggers, else the fixed wave sizes */
	need = ai_wave_size(st->wave) + st->wave_extra + wave_reserve(st->wave);
	if (st->prog && aiprog_active(p)) {
		int n = prog_waves(p, threat, have_t, tx, ty);
		attackers += n; home_idle -= n;
		if (home_idle < 0) home_idle = 0;
	} else if (threat < 0 && have_t && home_idle >= need) {
		int keep = wave_reserve(st->wave), n = 0;
		for (i = 0; i < s_narmy; i++) {
			int s = s_army[i];
			if (ai_role[s] != ROLE_HOME || !is_idle(&w2.unit[s])) continue;
			if (keep > 0) { keep--; continue; }
			ai_role[s] = ROLE_ATTACK; ai_retry[s] = 0;
			send_attacker(s, tx, ty);
			n++;
		}
		ai_log(p, "WAVE %d: %d units attack-move -> %d,%d", st->wave + 1, n, tx, ty);
		st->wave++;
		st->wave_extra = (uint8_t)(w2_rand() % 3);
		st->att_tx = (int16_t)tx; st->att_ty = (int16_t)ty;
		attackers += n; home_idle -= n;
	}
	/* the rest gathers at the rally point */
	if (threat < 0)
		for (i = 0; i < s_narmy; i++) {
			int s = s_army[i];
			if (ai_role[s] == ROLE_HOME && is_idle(&w2.unit[s]) && udist_tile(s, rx, ry) > 4) cmd_move(s, rx, ry);
		}
	transports(p);
	st->info.next_wave_size = ai_wave_size(st->wave) + st->wave_extra + wave_reserve(st->wave);
	st->info.attackers = attackers;
	st->info.home_army = home_idle;
	st->info.target_tx = st->att_tx; st->info.target_ty = st->att_ty;
}

/* ---- think ------------------------------------------------------------------------------------ */
/* what the program's wait conditions look at, from this think's scan */
static void prog_world(int p, AiProgWorld *w)
{
	const AiRace *r = &ai_races[ai_st[p].race];
	int q;
	w->shipyards = s_done[r->race_shipyard];
	w->keeps = s_done[r->hall[2]];
	w->castles = s_done[r->hall[3]];
	w->workers = s_done[r->worker];
	w->land = s_land; w->sea = s_sea; w->air = s_air;
	w->enemy_worker = 0;
	for (q = 0; q < AI_MAX_P; q++)
		if (q != p && ((w2.player[p].enemies >> q) & 1u) && ((s_ewk >> q) & 1u)) w->enemy_worker = 1;
}

static void think(int p)
{
	AiState *st = &ai_st[p];
	const AiRace *r = &ai_races[st->race];
	int wt = 0;
	if (w2.player[p].defeated) return;
	scan(p);
	if (st->prog && aiprog_active(p)) {
		/* the original steps the program every cycle: replay the cycles since the last think */
		AiProgWorld w;
		prog_world(p, &w);
		aiprog_run(p, w2.cycle - st->prog_cycle, &w);
		st->prog_cycle = w2.cycle;
	}
	b_gold = w2.player[p].gold; b_wood = w2.player[p].wood; b_oil = w2.player[p].oil;
	r_gold = r_wood = r_oil = 0;
	check_pending(p);
	economy(p, &wt);
	if (st->prog && aiprog_active(p)) train_prog(p); else train_army(p);
	assign_workers(p);
	tankers(p);
	military(p);
	st->info.active = 1;
	st->info.thinks++;
	st->info.workers = s_done[r->worker] + s_train[r->worker];
	st->info.worker_target = wt;
	st->info.gold_workers = s_ngold; st->info.wood_workers = s_nwood;
	st->info.army = s_narmy;
	st->info.wave = st->wave;
	st->info.hall_tier = s_hall_tier;
	st->info.food_supply = s_food_sup; st->info.food_used = s_food_used;
	st->info.home_tx = st->home_tx; st->info.home_ty = st->home_ty;
	st->info.prog = st->prog && aiprog_active(p);
	st->info.profile = st->info.prog ? aiprog_profile(p) : -1;
	st->info.prog_pc = st->info.prog ? aiprog_pc(p) : 0;
}

static void init_player(int p)
{
	AiState *st = &ai_st[p];
	int i;
	memset(st, 0, sizeof *st);
	st->active = 1;
	st->race = (uint8_t)(w2.player[p].race & 1);
	st->att_tx = st->att_ty = -1;
	st->tree_tx = st->tree_ty = -1;
	st->info.threat = -1; st->info.target_tx = st->info.target_ty = -1;
	for (i = 0; i < AI_MAX_PEND; i++) st->pend[i].builder = -1;
	/* home: start location, else the first own unit (the first think moves it to the hall) */
	st->home_tx = (int16_t)(w2.player[p].start_tx != 0xFFFF ? w2.player[p].start_tx : 0);
	st->home_ty = (int16_t)(w2.player[p].start_ty != 0xFFFF ? w2.player[p].start_ty : 0);
	if (w2.player[p].start_tx == 0xFFFF)
		for (i = 0; i < w2.unit_hi; i++)
			if ((w2.unit[i].flags & W2_UF_ALIVE) && w2.unit[i].owner == p) {
				st->home_tx = (int16_t)w2.unit[i].tx; st->home_ty = (int16_t)w2.unit[i].ty;
				break;
			}
}

/* ---- public ----------------------------------------------------------------------------------- */
void ai_set_log(int on) { g_log = on; aiprog_set_trace(on); }

void ai_init(void)
{
	int p;
	int k;
	g_log = w2cfg_extra_int("ai_log", 0);
	g_prog_on = w2cfg_extra_int("ai_programs", 1);
	aiprog_set_trace(g_log);
	memset(ai_own, 0, sizeof ai_own);
	memset(ai_role, 0, sizeof ai_role);
	memset(ai_retry, 0, sizeof ai_retry);
	memset(ai_ntr, 0, sizeof ai_ntr); memset(ai_twait, 0, sizeof ai_twait); memset(ai_tfail, 0, sizeof ai_tfail);
	for (k = 0; k < W2_MAX_UNITS; k++) ai_tslot[k] = W2_NO_UNIT;
	memset(ai_isprod, 0, sizeof ai_isprod);
	for (k = 0; k < AI_NCLASSES; k++)
		for (p = 0; p < 2; p++) {
			const w2_unit_type *t = w2_unit(ai_classes[k].type[p]);
			if (t && t->built_by < W2_NUM_UNIT_TYPES) ai_isprod[t->built_by] = 1;
		}
	for (p = 0; p < AI_MAX_P; p++) {
		ai_st[p].active = 0;
		aiprog_stop(p);
		if (w2.player[p].control == W2_CTRL_COMPUTER) {
			init_player(p);
			ai_log(p, "init: %s at %d,%d", ai_st[p].race ? "orc" : "human", ai_st[p].home_tx, ai_st[p].home_ty);
			if (g_prog_on) {
				/* the profile program from the map's AIPL; its first step runs now, as in the original */
				AiProgWorld w;
				scan(p);
				prog_world(p, &w);
				ai_st[p].prog = (uint8_t)aiprog_start(p, w2.player[p].ai_type, &w);
				ai_st[p].prog_cycle = w2.cycle;
			}
		}
	}
}

void ai_tick(void)
{
	int p;
	for (p = 0; p < AI_MAX_P; p++) {
		if (!ai_st[p].active || w2.player[p].control != W2_CTRL_COMPUTER) continue;
		if ((w2.cycle + (uint32_t)p * 5u) % AI_THINK_PERIOD) continue;
		think(p);
	}
}

void ai_think_now(int p)
{
	if (p < 0 || p >= AI_MAX_P) return;
	if (!ai_st[p].active) init_player(p);
	think(p);
}

void ai_debug_scan(int p)
{
	if (p < 0 || p >= AI_MAX_P) return;
	if (!ai_st[p].active) init_player(p);
	scan(p);
}

void ai_get_info(int p, AiInfo *out)
{
	if (p < 0 || p >= AI_MAX_P) { memset(out, 0, sizeof *out); return; }
	*out = ai_st[p].info;
	out->active = ai_st[p].active;
}
