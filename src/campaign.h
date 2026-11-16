/* campaign.h - single-player campaigns (Tides of Darkness, human and orc, 14 missions each).
 *
 * Everything the player reads, sees or hears comes from the player's own data at run time:
 * titles and objectives War Strings 54, briefings War Strings 65.., act names War Strings 62,
 * briefing pictures War Data 369..378, act pictures War Resources 19..26, voices War Music 4..45,
 * movies War Movies. campaign.c only holds game facts (which map, which objective rule, which
 * allowance masks, which entry numbers) taken from the Mac 1.23 binary (docs/RE_NOTES.md).
 *
 * Level index i = 2 * (mission - 1) + race (0 human, 1 orc), map War Data 192 + i.
 *
 * Rescue and circle of power work in every single-player game (the Mac does it for any map):
 * units of PUD owners "rescue passive" (6) / "rescue active" (7) are marked at the start; a marked
 * unit of a non-human player that touches a unit of a human player changes owner; a marked unit of
 * a human player standing on a circle of power counts as "arrived". Rescue-active players are run by
 * the computer AI.
 */
#ifndef W2_CAMPAIGN_H
#define W2_CAMPAIGN_H

#include <stdint.h>

#define CAMP_MISSIONS 14
#define CAMP_LEVELS   28

/* ---- progress (PROGDIR:campaign.cfg, our own two-line text format) ---- */
void camp_progress_load(void);
int  camp_progress(int race);              /* furthest mission reached, 1..14 */
void camp_progress_reach(int race, int mission);   /* raises it and saves */

/* ---- the running campaign ---- */
int  camp_active(void);                    /* a campaign mission is being played / briefed */
void camp_select(int race, int mission);   /* make this mission current (camp_active() = 1) */
void camp_stop(void);                      /* custom game / main menu: no campaign */
int  camp_race(void);
int  camp_mission(void);                   /* 1..14 */
int  camp_index(void);                     /* 0..27 */
int  camp_map_entry(void);                 /* War Data entry of the current mission's map */
int  camp_start_game(void);                /* game_start() of the current mission; 0 = ok */
/* After the end screen: result 1 victory (advances; 1 = the campaign is finished), 2 defeat (same
 * mission again). Returns 0 normally, 1 when the last mission was won. */
int  camp_after_end(int result);

/* ---- pre-mission presentation (Mac FUN_00142874 / f9_149C) ---- */
/* Act title before the first mission of an act: War Resources picture (palette War Resources 17)
 * and the act strings (War Strings 62); -1 if this mission opens no act. */
int  camp_act_picture(void);
const char *camp_act_number(void);         /* "Act I" ... from the player's data, "" if none */
const char *camp_act_name(void);
/* War Movies entry played before this mission's briefing, -1 none. */
int  camp_movie_before(void);
/* War Movies entry played after the last mission was won (race's ending), -1 none. */
int  camp_movie_final(void);
/* Briefing picture (War Data) and its palette (War Data). */
int  camp_brief_picture(int *palette);
const char *camp_title(void);              /* War Strings 54 */
const char *camp_title_of(int race, int mission);
const char *camp_objectives(void);         /* War Strings 54 (custom games: the generic line) */
const char *camp_briefing(void);           /* War Strings 65 + index */
int  camp_voice_count(void);
int  camp_voice_entry(int k);              /* War Music entry of voice part k */
/* the final screen after the last mission: War Resources picture + palette, text, voice */
int  camp_final_picture(int *palette);
const char *camp_final_text(void);
int  camp_final_voice(void);

/* ---- speech (War Music WAVs, streamed on the music channels; music stops meanwhile) ---- */
int  camp_speech_play(int entry);          /* 1 = started */
void camp_speech_stop(void);
int  camp_speech_active(void);             /* still playing (call camp_speech_service per frame) */
void camp_speech_service(void);

/* ---- in-game (game.c) ---- */
/* After the map is loaded and the local player chosen, before the AI starts: rescue marks, the
 * controller of rescue-active players, circles of power; in a campaign also the diplomacy, the
 * allowances and the objective rule of the level. */
void camp_world_setup(void);
/* Once per logic cycle (cheap: real work every 8 / 50 / 200 cycles). Returns 0 running, 1 victory,
 * 2 defeat (campaign objective rule; 0 always in custom games - game.c keeps its own end check). */
int  camp_tick(void);

/* ---- test / debug ---- */
int  camp_rescued(int player);             /* units this player rescued */
int  camp_arrived(int player);             /* marked units that reached a circle (low 12 bits) */
int  camp_objective_kind(void);            /* Mac objective code of the current level (A5+6D5E) */
const char *camp_debug_state(void);

#endif
