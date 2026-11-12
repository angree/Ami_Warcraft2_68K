/* ai.h - computer opponents (owner: ai.c). The game calls ai_init()/ai_tick() from w2.h; the rest is
 * for tests and debugging (tools/test_ai.c). */
#ifndef W2_AI_H
#define W2_AI_H

#include "w2.h"

#define AI_THINK_PERIOD 40      /* cycles between two "thinks" of one computer player (staggered by player) */

/* Snapshot of one computer player's view after its last think. */
typedef struct AiInfo {
	int active;                 /* 1 = this player is run by the AI */
	int thinks;                 /* number of thinks so far */
	int workers, worker_target, gold_workers, wood_workers;
	int army, home_army, attackers;
	int wave;                   /* attack waves launched so far */
	int next_wave_size;         /* idle home army needed for the next wave (incl. home reserve) */
	int hall_tier;              /* 0 none, 1 town hall, 2 keep, 3 castle (complete) */
	int food_supply, food_used;
	int step;                   /* build-order step the AI is blocked on (-1 = all done) */
	int home_tx, home_ty;
	int target_tx, target_ty;   /* current attack target, -1 none */
	int threat;                 /* slot of the enemy being defended against, -1 none */
} AiInfo;

void ai_set_log(int on);                       /* decision log via w2_log (default: w2.cfg ai_log=1) */
void ai_think_now(int player);                 /* run one full think for this player immediately */
void ai_get_info(int player, AiInfo *out);
/* Scan the world for this player (what a think does first) without deciding anything; needed before
 * ai_find_site when called from outside a think. */
void ai_debug_scan(int player);
/* Build-site search around tile (cx,cy) for a building of 'type'. Uses econ_place_ok when it accepts a
 * site, else falls back to the AI's own terrain test. 1 = found (*tx,*ty top-left). */
int  ai_find_site(int player, int type, int cx, int cy, int builder, int *tx, int *ty);
/* The AI's own terrain/margin test for a footprint (the fallback used by ai_find_site). */
int  ai_site_ok(int player, int type, int tx, int ty);
/* Attack wave size for wave number n (0-based, without the home reserve). */
int  ai_wave_size(int n);

#endif
