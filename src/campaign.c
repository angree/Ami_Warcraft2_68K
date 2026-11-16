/* campaign.c - single-player campaigns: level table, progress, objective rules, rescue, circle of
 * power, briefing data and the speech stream. See campaign.h.
 *
 * Game facts from the Mac 1.23 binary (prose in docs/RE_NOTES.md, "campaigns"):
 *  - level index i = 2 * (mission - 1) + race; map = War Data 192 + i (f9_149C: A5+6A86 - 1192).
 *  - per level: objective code A5+18BC[i] and its count A5+1884[i] (FUN_0013800e -> A5+6D5E/6D60),
 *    allowance masks A5+1814 (units), A5+1734 (spells), A5+17A4 (upgrades) for all 16 players
 *    (f8_45F0), briefing picture table A5-0x7BA0, voice lists A5-0x7ADC, act screens and movies
 *    FUN_00142874, final movie FUN_00136e7e.
 *  - the objective rule runs every 200 cycles (Game_Every200 -> the function chosen by f14_32E6);
 *    the shared part is FUN_00172e88, the per-code parts f14_2FBC .. f14_321A.
 *  - rescue FUN_0016ccc8/FUN_0016cc14, circle of power f6_2624 (every 50 cycles), unit marks set
 *    for PUD owners 6/7 at load (f8_3EDE), diplomacy FUN_0014c04e, controller remap f9_401E.
 * Nothing here is a copy of the original's code or text; the numbers are entry indices and masks.
 *
 * 68k: the objective rule is one pass over the units every 200 cycles, rescue looks at the marked
 * units only (every 8 cycles), circles every 50 cycles. No floats, no per-frame scans.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w2.h"
#include "w2cfg.h"
#include "assets.h"
#include "campaign.h"
#include "economy.h"
#include "unit.h"
#include "snd.h"
#include "amiga_audio.h"
#include "amiga_gfx.h"

/* ------------------------------------------------------------------------------------------ level table */

/* Objective codes (A5+6D5E). Bits 0x100 "destroy every other player", 0x200 "bring marked units to
 * a circle of power" (count in 'need', +0x1000 = one of them must be a hero). 0..10 = level rules. */
enum {
	OBJ_FARMS = 0,        /* 4 farms + a barracks (completed) */
	OBJ_PLATFORMS = 1,    /* 4 oil platforms */
	OBJ_REFINERIES = 2,   /* no other player owns a refinery */
	OBJ_CASTLE_RIOT = 3,  /* player 4's workers turn into attack workers and become enemies; then destroy all + own a castle */
	OBJ_PRISONERS = 4,    /* player 4's workers turn into attack workers (still rescuable); then the circle rule */
	OBJ_RESCUE_FIRST = 5, /* rescue anything, then destroy all */
	OBJ_NAVY = 6,         /* no other player owns transports, oil platforms or shipyards */
	OBJ_HERO_LOST = 7,    /* player 3's hero (Lothar) is no longer rescuable; then destroy all */
	OBJ_PORTAL = 8,       /* the neutral dark portal is gone */
	OBJ_RUNESTONE = 9,    /* own the runestone and no other player owns a castle; nobody owns one = defeat */
	OBJ_ISLAND = 10,      /* a fortress and a shipyard of the local player on the circle's island */
	OBJ_ALL = 0x100,
	OBJ_CIRCLE = 0x200
};

typedef struct CampLevel {
	uint16_t obj, need;
	uint32_t units, spells, upgrades;
	uint16_t picture;                  /* War Data entry of the briefing picture */
	uint8_t  voice, nvoice;            /* War Music entries voice .. voice + nvoice - 1 */
} CampLevel;

static const CampLevel k_lv[CAMP_LEVELS] = {
	/* obj        need    units        spells      upgrades  pic  voice */
	{ OBJ_FARMS,      0,  0x04030003u, 0x000000u, 0x00000u, 369,  4, 2 },   /* human 1 */
	{ OBJ_FARMS,      0,  0x04030003u, 0x000000u, 0x00000u, 374, 27, 1 },   /* orc 1 */
	{ OBJ_CIRCLE,     1,  0x04070013u, 0x000000u, 0x00001u, 369,  6, 2 },   /* human 2 */
	{ OBJ_CIRCLE, 0x1001, 0x04070013u, 0x000000u, 0x00001u, 374, 28, 1 },   /* orc 2 */
	{ OBJ_PLATFORMS,  0,  0x440700D3u, 0x000000u, 0x00003u, 369,  8, 2 },   /* human 3 */
	{ OBJ_PLATFORMS,  0,  0x440700D3u, 0x000000u, 0x00003u, 374, 29, 2 },   /* orc 3 */
	{ OBJ_ALL,        0,  0x466701D3u, 0x000000u, 0x00143u, 369, 10, 2 },   /* human 4 */
	{ OBJ_ALL,        0,  0x466701D3u, 0x000000u, 0x00143u, 374, 31, 1 },   /* orc 4 */
	{ OBJ_ALL,        0,  0x6EE709D7u, 0x000000u, 0x01157u, 377, 12, 2 },   /* human 5 */
	{ OBJ_ALL,        0,  0x6EE709D7u, 0x000000u, 0x01157u, 370, 32, 1 },   /* orc 5 */
	{ OBJ_ALL,        0,  0x6EEF09DFu, 0x000000u, 0xF317Fu, 377, 14, 1 },   /* human 6 */
	{ OBJ_CIRCLE, 0x1001, 0x00000000u, 0x000000u, 0x00000u, 370, 33, 1 },   /* orc 6 */
	{ OBJ_REFINERIES, 0,  0x6EEF0BDFu, 0x000000u, 0xF33FFu, 377, 15, 2 },   /* human 7 */
	{ OBJ_ALL,        0,  0x6EEF09DFu, 0x000000u, 0xF317Fu, 370, 34, 2 },   /* orc 7 */
	{ OBJ_CASTLE_RIOT,0,  0x7FEF2BDFu, 0x100003u, 0xF33FFu, 375, 17, 2 },   /* human 8 */
	{ OBJ_RUNESTONE,  0,  0x6EEF0BDFu, 0x000000u, 0xF33FFu, 373, 36, 1 },   /* orc 8 */
	{ OBJ_CIRCLE, 0x1001, 0x00000000u, 0x100003u, 0x00000u, 375, 19, 1 },   /* human 9 */
	{ OBJ_ISLAND,     0,  0x7FEF2BDFu, 0x100C00u, 0xF33FFu, 373, 37, 1 },   /* orc 9 */
	{ OBJ_PRISONERS,  4,  0x7FEF2BDFu, 0x10000Bu, 0xF33FFu, 375, 20, 1 },   /* human 10 */
	{ OBJ_ALL,        0,  0x7FEF6BDFu, 0x140C00u, 0xF33FFu, 373, 38, 1 },   /* orc 10 */
	{ OBJ_RESCUE_FIRST,0, 0x7FFF6BFFu, 0x10005Bu, 0xF33FFu, 375, 21, 2 },   /* human 11 */
	{ OBJ_ALL,        0,  0x7FFF6BFFu, 0x152C00u, 0xF33FFu, 373, 39, 2 },   /* orc 11 */
	{ OBJ_NAVY,       0,  0x7FFF6FFFu, 0x1000DBu, 0xF33FFu, 376, 23, 1 },   /* human 12 */
	{ OBJ_ALL,        0,  0x7FFF6FFFu, 0x15AC00u, 0xF33FFu, 372, 41, 2 },   /* orc 12 */
	{ OBJ_HERO_LOST,  0,  0x7FFFFFFFu, 0x1003DBu, 0xF33FFu, 376, 24, 2 },   /* human 13 */
	{ OBJ_ALL,        0,  0x7FFFFFFFu, 0x1FAC00u, 0xF33FFu, 372, 43, 2 },   /* orc 13 */
	{ OBJ_PORTAL,     0,  0x7FFFFFFFu, 0x1003DBu, 0xF33FFu, 376, 26, 1 },   /* human 14 */
	{ OBJ_ALL,        0,  0x7FFFFFFFu, 0x1FAC00u, 0xF33FFu, 372, 45, 1 },   /* orc 14 */
};

#define CAMP_FIRST_MAP   192     /* War Data */
#define BRIEF_PAL_HUMAN  367     /* War Data palettes of the briefing pictures */
#define BRIEF_PAL_ORC    368
#define STR_OBJ_TITLE    54      /* War Strings: objectives 0..27, generic 34, titles 35..62 */
#define STR_OBJ_GENERIC  34
#define STR_TITLE_BASE   35
#define STR_BRIEF_BASE   65      /* War Strings 65 + index: briefing text (table of one string) */
#define STR_ACTS         62      /* War Strings: act number / act name pairs, orc from 8 */
#define STR_VICTORY      93      /* War Strings: final text, string 0 human, 1 orc */
#define ACT_PIC_BASE     19      /* War Resources 19 + 2 * (act - 1) + race, palette 17 */

/* ------------------------------------------------------------------------------------------ state */

static int g_on, g_race, g_mission = 1;
static int g_prog[2] = { 1, 1 }, g_prog_loaded;

static int lvl(void) { return 2 * (g_mission - 1) + g_race; }

/* per player (0..15) */
static uint16_t g_arrived[W2_MAX_PLAYERS];    /* A5+6D64: marked units that reached a circle (+0x1000 hero) */
static uint16_t g_rescued[W2_MAX_PLAYERS];    /* A5+6D84 */
static uint16_t g_obj, g_need;                /* A5+6D5E / 6D60 of the running game */
static int      g_obj_wait;                   /* cycles to the next objective check (A5+5386) */
static int      g_result;                     /* 1 victory / 2 defeat once decided */

/* marked (rescuable / objective) units */
#define MAX_MARKS 320
static uint8_t  g_mark[W2_MAX_UNITS];
static uint16_t g_mark_gen[W2_MAX_UNITS];
static uint16_t g_ml[MAX_MARKS];
static int      g_mn;

/* circles of power */
#define MAX_CIRCLES 8
static uint16_t g_circ[MAX_CIRCLES], g_circ_gen[MAX_CIRCLES];
static int      g_ncirc;

/* the island of the circle (objective 10): one bit per tile */
static uint8_t *g_region;

/* ------------------------------------------------------------------------------------------ progress */

#define PROG_FILE "PROGDIR:campaign.cfg"

void camp_progress_load(void)
{
	FILE *f;
	char line[64];
	g_prog_loaded = 1;
	g_prog[0] = g_prog[1] = 1;
	f = fopen(PROG_FILE, "r");
	if (!f) return;
	while (fgets(line, sizeof line, f)) {
		int r = -1;
		const char *v = NULL;
		if (!strncmp(line, "human=", 6)) { r = 0; v = line + 6; }
		else if (!strncmp(line, "orc=", 4)) { r = 1; v = line + 4; }
		if (r >= 0) {
			int n = atoi(v);
			if (n >= 1 && n <= CAMP_MISSIONS) g_prog[r] = n;
		}
	}
	fclose(f);
	w2_log("camp: progress human %d orc %d", g_prog[0], g_prog[1]);
}

static void progress_save(void)
{
	FILE *f = fopen(PROG_FILE, "w");
	if (!f) { w2_log("camp: cannot write %s", PROG_FILE); return; }
	fprintf(f, "# Warcraft II 68K campaign progress: furthest mission reached per race\nhuman=%d\norc=%d\n", g_prog[0], g_prog[1]);
	fclose(f);
}

int camp_progress(int race)
{
	if (!g_prog_loaded) camp_progress_load();
	return g_prog[race ? 1 : 0];
}

void camp_progress_reach(int race, int mission)
{
	race = race ? 1 : 0;
	if (!g_prog_loaded) camp_progress_load();
	if (mission < 1 || mission > CAMP_MISSIONS || mission <= g_prog[race]) return;
	g_prog[race] = mission;
	progress_save();
}

/* ------------------------------------------------------------------------------------------ selection */

int  camp_active(void) { return g_on; }
void camp_stop(void) { g_on = 0; }
int  camp_race(void) { return g_race; }
int  camp_mission(void) { return g_mission; }
int  camp_index(void) { return lvl(); }
int  camp_map_entry(void) { return CAMP_FIRST_MAP + lvl(); }
int  camp_objective_kind(void) { return g_obj; }

void camp_select(int race, int mission)
{
	g_race = race ? 1 : 0;
	if (mission < 1) mission = 1;
	if (mission > CAMP_MISSIONS) mission = CAMP_MISSIONS;
	g_mission = mission;
	g_on = 1;
	w2_log("camp: %s mission %d (level %d, map War Data %d)", g_race ? "orc" : "human", g_mission, lvl(), camp_map_entry());
}

int camp_start_game(void)
{
	W2Setup s;
	if (!g_on) return -1;
	memset(&s, 0, sizeof s);
	s.builtin_pud = camp_map_entry();
	s.local_player = -1;      /* the PUD's human slot */
	s.local_race = -1;        /* and its SIDE */
	return game_start(&s);
}

int camp_after_end(int result)
{
	if (!g_on) return 0;
	if (result != 1) return 0;                 /* defeat: the same mission again (Mac FUN_00143204) */
	if (g_mission >= CAMP_MISSIONS) { camp_progress_reach(g_race, CAMP_MISSIONS); return 1; }
	g_mission++;
	camp_progress_reach(g_race, g_mission);
	return 0;
}

/* ------------------------------------------------------------------------------------------ presentation */

static int act_of_mission(int m) { return m == 1 ? 1 : m == 5 ? 2 : m == 8 ? 3 : m == 12 ? 4 : 0; }

int camp_act_picture(void)
{
	int a = act_of_mission(g_mission);
	return a ? ACT_PIC_BASE + 2 * (a - 1) + g_race : -1;
}

const char *camp_act_number(void)
{
	int a = act_of_mission(g_mission);
	return a ? assets_string(STR_ACTS, g_race * 8 + 2 * (a - 1)) : "";
}

const char *camp_act_name(void)
{
	int a = act_of_mission(g_mission);
	return a ? assets_string(STR_ACTS, g_race * 8 + 2 * (a - 1) + 1) : "";
}

int camp_movie_before(void)
{
	/* War Movies: act II human-1 / orc-1, act III human-2 / orc-2, act IV human-3 / orc-3 */
	static const signed char k[2][3] = { { 11, 14, 10 }, { 0, 12, 2 } };
	int a = act_of_mission(g_mission);
	return a >= 2 ? k[g_race][a - 2] : -1;
}

int camp_movie_final(void) { return g_race ? 8 : 3; }

int camp_brief_picture(int *palette)
{
	if (palette) *palette = g_race ? BRIEF_PAL_ORC : BRIEF_PAL_HUMAN;
	return k_lv[lvl()].picture;
}

const char *camp_title(void) { return assets_string(STR_OBJ_TITLE, STR_TITLE_BASE + lvl()); }
const char *camp_title_of(int race, int mission)
{
	if (mission < 1 || mission > CAMP_MISSIONS) return "";
	return assets_string(STR_OBJ_TITLE, STR_TITLE_BASE + 2 * (mission - 1) + (race ? 1 : 0));
}
const char *camp_objectives(void) { return assets_string(STR_OBJ_TITLE, g_on ? lvl() : STR_OBJ_GENERIC); }
const char *camp_briefing(void) { return assets_string(STR_BRIEF_BASE + lvl(), 0); }
int camp_voice_count(void) { return k_lv[lvl()].nvoice; }
int camp_voice_entry(int k) { return (k >= 0 && k < k_lv[lvl()].nvoice) ? k_lv[lvl()].voice + k : -1; }

int camp_final_picture(int *palette)
{
	/* War Resources 29 (human, palette 27) / 30 (orc, palette 32) */
	if (palette) *palette = g_race ? 32 : 27;
	return g_race ? 30 : 29;
}
const char *camp_final_text(void) { return assets_string(STR_VICTORY, g_race); }
int camp_final_voice(void) { return g_race ? 3 : 2; }

/* ------------------------------------------------------------------------------------------ speech */

static unsigned char *g_sp;          /* the whole WAV entry */
static unsigned long  g_sp_pos, g_sp_end;
static int            g_sp_bits, g_sp_on;

static int sp_refill(void *ud, signed char *dst, int max)
{
	int n = 0;
	(void)ud;
	if (!g_sp) return 0;
	if (g_sp_bits == 8) {
		while (n < max && g_sp_pos < g_sp_end) dst[n++] = (signed char)(g_sp[g_sp_pos++] - 128);
	} else {
		while (n < max && g_sp_pos + 1 < g_sp_end) { dst[n++] = (signed char)g_sp[g_sp_pos + 1]; g_sp_pos += 2; }
	}
	return n;
}

static unsigned rd16(const unsigned char *p) { return (unsigned)p[0] | (unsigned)p[1] << 8; }
static unsigned long rd32(const unsigned char *p) { return (unsigned long)rd16(p) | (unsigned long)rd16(p + 2) << 16; }

void camp_speech_stop(void)
{
	if (g_sp_on) AmigaAudio_MusicStop();
	g_sp_on = 0;
	if (g_sp) { war_free(g_sp); g_sp = NULL; }
}

int camp_speech_play(int entry)
{
	unsigned long size = 0, p, rate = 0, per;
	unsigned tag = 0, ch = 0;
	camp_speech_stop();
	if (entry < 0 || w2_cfg.sfx_volume <= 0) return 0;
	snd_music(0);                                         /* the stream uses the music channels */
	g_sp = assets_load(ASSET_SPEECH, entry, &size);
	if (!g_sp) { w2_log("camp: voice %d missing", entry); return 0; }
	g_sp_end = 0;
	if (size >= 12 && !memcmp(g_sp, "RIFF", 4) && !memcmp(g_sp + 8, "WAVE", 4)) {
		for (p = 12; p + 8 <= size; ) {
			unsigned long cl = rd32(g_sp + p + 4);
			if (!memcmp(g_sp + p, "fmt ", 4) && cl >= 16 && p + 24 <= size) {
				tag = rd16(g_sp + p + 8); ch = rd16(g_sp + p + 10); rate = rd32(g_sp + p + 12); g_sp_bits = (int)rd16(g_sp + p + 22);
			} else if (!memcmp(g_sp + p, "data", 4)) {
				g_sp_pos = p + 8;
				g_sp_end = g_sp_pos + (cl < size - g_sp_pos ? cl : size - g_sp_pos);
				break;
			}
			if (cl > size - p - 8) break;
			p += 8 + cl + (cl & 1);
		}
	}
	if (!g_sp_end || tag != 1 || ch != 1 || (g_sp_bits != 8 && g_sp_bits != 16) || rate < 4000) {
		w2_log("camp: voice %d not a mono PCM WAV", entry);
		camp_speech_stop();
		return 0;
	}
	per = (amigagfx_paula_clock() + rate / 2) / rate;
	if (per < 124) per = 124;
	AmigaAudio_MusicSetVolume(w2_cfg.sfx_volume * 64 / 100);
	if (!AmigaAudio_MusicStart((int)per, 4096, sp_refill, NULL)) { camp_speech_stop(); return 0; }
	g_sp_on = 1;
	w2_log("camp: voice War Music %d, %lu bytes at %lu Hz", entry, g_sp_end - g_sp_pos, rate);
	return 1;
}

int camp_speech_active(void) { return g_sp_on; }

void camp_speech_service(void)
{
	if (!g_sp_on) return;
	if (AmigaAudio_MusicFinished() || !AmigaAudio_MusicActive()) camp_speech_stop();
}

/* ------------------------------------------------------------------------------------------ helpers */

static int alive(int s) { return s >= 0 && s < W2_MAX_UNITS && (w2.unit[s].flags & W2_UF_ALIVE) && !(w2.unit[s].flags & W2_UF_DYING); }
static int ctrl(int p) { return p >= 0 && p < W2_MAX_PLAYERS ? w2.player[p].control : W2_CTRL_NOBODY; }

static int marked(int s) { return alive(s) && g_mark[s] && unit_same(s, g_mark_gen[s]); }

static void mark(int s)
{
	if (!alive(s) || g_mark[s]) return;
	if (g_mn >= MAX_MARKS) { w2_log("camp: more than %d rescuable units", MAX_MARKS); return; }
	g_mark[s] = 1; g_mark_gen[s] = unit_gen(s);
	g_ml[g_mn++] = (uint16_t)s;
}

static void unmark(int s) { if (s >= 0 && s < W2_MAX_UNITS) g_mark[s] = 0; }

/* drop dead / reused entries from the list */
static void marks_compact(void)
{
	int i, j = 0;
	for (i = 0; i < g_mn; i++) {
		int s = g_ml[i];
		if (marked(s)) g_ml[j++] = (uint16_t)s;
		else g_mark[s] = 0;
	}
	g_mn = j;
}

static void set_enemy(int a, int b, int on)
{
	if (a < 0 || a >= 8 || b < 0 || b >= 8 || a == b) return;
	if (on) w2.player[a].enemies |= (uint16_t)(1u << b);
	else w2.player[a].enemies &= (uint16_t)~(1u << b);
}

/* f13_5434: every unit of 'player' of type from (either race) becomes type to; keep_mark 0 also
 * takes the rescue mark off a unit that became an attack worker */
static void convert_type(int player, int from, int to, int keep_mark)
{
	int s;
	for (s = 0; s < w2.unit_hi; s++) {
		W2Unit *u = &w2.unit[s];
		int nt;
		if (!alive(s) || u->owner != player || (u->type & ~1) != from) continue;
		nt = to | (u->type & 1);
		if (!w2_unit((unsigned)nt)) continue;
		u->type = (uint8_t)nt;
		if (u->hp > w2_unit((unsigned)nt)->hp) u->hp = w2_unit((unsigned)nt)->hp;
		unit_set_anim(s, W2_ANIM_STILL);
		if (!keep_mark) unmark(s);
	}
	econ_recount(player);
}

/* the island (land connected over walkable / forest / rock tiles, not across water or the shallow
 * coast boundary) the tile (x,y) belongs to, as a tile bitmap */
static int land_tile(int i)
{
	unsigned f = w2.map.flags[i];
	if (!(f & W2_TF_LAND) || (f & W2_TF_WATER)) return 0;
	return !(f & W2_TF_NOWALK) || (f & (W2_TF_TREE | W2_TF_ROCK | W2_TF_WALL | W2_TF_BUILDING));
}

static void region_build(int x, int y)
{
	int n = w2.map.w * w2.map.h, head = 0, tail = 0;
	uint16_t *q;
	free(g_region); g_region = NULL;
	if (x < 0 || y < 0 || x >= w2.map.w || y >= w2.map.h) return;
	g_region = (uint8_t *)calloc((size_t)(n + 7) / 8, 1);
	q = (uint16_t *)malloc((size_t)n * sizeof *q);
	if (!g_region || !q) { free(q); free(g_region); g_region = NULL; return; }
	g_region[(y * w2.map.w + x) >> 3] |= (uint8_t)(1u << ((y * w2.map.w + x) & 7));
	q[tail++] = (uint16_t)(y * w2.map.w + x);
	while (head < tail) {
		int i = q[head++], cx = i % w2.map.w, cy = i / w2.map.w, dx, dy;
		for (dy = -1; dy <= 1; dy++)
			for (dx = -1; dx <= 1; dx++) {
				int nx = cx + dx, ny = cy + dy, j;
				if (nx < 0 || ny < 0 || nx >= w2.map.w || ny >= w2.map.h) continue;
				j = ny * w2.map.w + nx;
				if (g_region[j >> 3] & (1u << (j & 7)) || !land_tile(j)) continue;
				g_region[j >> 3] |= (uint8_t)(1u << (j & 7));
				q[tail++] = (uint16_t)j;
			}
	}
	free(q);
	w2_log("camp: island of the circle at %d,%d: %d tiles", x, y, tail);
}

static int in_region(int s)
{
	const w2_unit_type *t = unit_type(s);
	int x, y, w, h;
	if (!g_region || !t) return 0;
	w = t->tile_w ? t->tile_w : 1; h = t->tile_h ? t->tile_h : 1;
	for (y = w2.unit[s].ty; y < w2.unit[s].ty + h && y < w2.map.h; y++)
		for (x = w2.unit[s].tx; x < w2.unit[s].tx + w && x < w2.map.w; x++) {
			int i = y * w2.map.w + x;
			if (g_region[i >> 3] & (1u << (i & 7))) return 1;
		}
	return 0;
}

/* ------------------------------------------------------------------------------------------ setup */

void camp_world_setup(void)
{
	int p, q, s;
	memset(g_mark, 0, sizeof g_mark);
	memset(g_arrived, 0, sizeof g_arrived);
	memset(g_rescued, 0, sizeof g_rescued);
	g_mn = 0; g_ncirc = 0; g_result = 0;
	free(g_region); g_region = NULL;

	/* rescue marks: every unit of a rescue player at the start (f8_3EDE, owners 6 / 7) */
	for (s = 0; s < w2.unit_hi; s++) {
		int c;
		if (!alive(s)) continue;
		c = ctrl(w2.unit[s].owner);
		if (c == W2_CTRL_RESCUE_PASSIVE || c == W2_CTRL_RESCUE_ACTIVE) mark(s);
		if (w2.unit[s].type == W2U_CIRCLE_OF_POWER && g_ncirc < MAX_CIRCLES) {
			/* walkable: the circle leaves the occupancy grid (units stand on it) */
			unit_unplace(s);
			g_circ[g_ncirc] = (uint16_t)s; g_circ_gen[g_ncirc] = unit_gen(s); g_ncirc++;
		}
	}

	/* diplomacy (FUN_0014c04e) on the raw owner kinds, then the controller remap (f9_401E) */
	for (p = 0; p < 8; p++) {
		int cp = ctrl(p);
		for (q = 0; q < 8; q++) {
			int cq = ctrl(q), enemy;
			if (p == q) continue;
			if (cp == W2_CTRL_RESCUE_PASSIVE || cq == W2_CTRL_RESCUE_PASSIVE) enemy = 0;
			else if ((cp == W2_CTRL_HUMAN && cq == W2_CTRL_RESCUE_ACTIVE) || (cq == W2_CTRL_HUMAN && cp == W2_CTRL_RESCUE_ACTIVE)) enemy = 0;
			else if (cp == W2_CTRL_RESCUE_ACTIVE || cq == W2_CTRL_RESCUE_ACTIVE) enemy = 1;     /* vs the computer */
			else if (g_on && cp == W2_CTRL_COMPUTER && cq == W2_CTRL_COMPUTER) enemy = 0;       /* campaign: computers allied */
			else continue;                                                                     /* custom game: as loaded */
			set_enemy(p, q, enemy);
		}
	}
	for (p = 0; p < W2_MAX_PLAYERS; p++) if (w2.player[p].control == W2_CTRL_RESCUE_ACTIVE) w2.player[p].control = W2_CTRL_COMPUTER;

	g_obj = OBJ_ALL; g_need = 0;
	if (!g_on) { w2_log("camp: custom game, %d rescuable units, %d circles", g_mn, g_ncirc); return; }

	/* the level: allowances for all 16 players (f8_45F0), objective (FUN_0013800e / f14_32E6) */
	{
		const CampLevel *L = &k_lv[lvl()];
		for (p = 0; p < W2_MAX_PLAYERS; p++) { w2.player[p].allowed_units = L->units; w2.player[p].allowed_spells = L->spells; }
		econ_set_allowed_upgrades(-1, L->upgrades);
		g_obj = L->obj; g_need = L->need;
	}
	switch (g_obj) {
	case OBJ_CASTLE_RIOT:            /* f13_5434(4, worker -> attack worker, unmark); 4 and the local player at war */
		convert_type(4, W2U_PEASANT, W2U_ATTACK_PEASANT, 0);
		set_enemy(w2.local_player, 4, 1); set_enemy(4, w2.local_player, 1);
		break;
	case OBJ_PRISONERS:              /* the same, marks kept; then the circle rule */
		convert_type(4, W2U_PEASANT, W2U_ATTACK_PEASANT, 1);
		g_obj = OBJ_CIRCLE;
		break;
	case OBJ_HERO_LOST:              /* Lothar of player 3 cannot be rescued; then destroy all */
		for (s = 0; s < w2.unit_hi; s++)
			if (alive(s) && w2.unit[s].owner == 3 && w2.unit[s].type == W2U_WISE_MAN) unmark(s);
		marks_compact();
		g_obj = OBJ_ALL;
		break;
	case OBJ_ISLAND:
		if (g_ncirc) region_build(w2.unit[g_circ[0]].tx, w2.unit[g_circ[0]].ty);
		break;
	default: break;
	}
	g_obj_wait = 1;                  /* A5+5386 = 1: first check at the next cycle, then every 200 */
	w2_log("camp: level %d objective 0x%x need 0x%x, %d rescuable units, %d circles, local player %d",
	       lvl(), g_obj, g_need, g_mn, g_ncirc, w2.local_player);
}

/* ------------------------------------------------------------------------------------------ rescue */

/* FUN_0016cc14: 's' changes to the owner 'to' */
static void change_owner(int s, int to)
{
	W2Unit *u = &w2.unit[s];
	int from = u->owner;
	if (from == to) return;
	g_rescued[to]++;
	u->owner = (uint8_t)to;
	if (!(u->flags & W2_UF_BUILDING) && u->order != W2_ORD_STILL) unit_set_order(s, W2_ORD_STILL, -1, u->tx, u->ty);
	econ_recount(from);
	econ_recount(to);
}

/* FUN_0016ccc8: a marked unit of a non-human player touched by a human player's unit */
static void rescue_check(int s)
{
	const W2Unit *u = &w2.unit[s];
	const w2_unit_type *t = unit_type(s);
	int x0, y0, x1, y1, x, y, w, h;
	if (!t) return;
	w = t->tile_w ? t->tile_w : 1; h = t->tile_h ? t->tile_h : 1;
	x0 = u->tx - 1; y0 = u->ty - 1; x1 = u->tx + w + 1; y1 = u->ty + h + 1;
	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 > w2.map.w) x1 = w2.map.w;
	if (y1 > w2.map.h) y1 = w2.map.h;
	for (y = y0; y < y1; y++)
		for (x = x0; x < x1; x++) {
			int o = w2.map.occ_ground[y * w2.map.w + x], from, k;
			const w2_unit_type *ot;
			if (o == W2_NO_UNIT || o == s || !alive(o)) continue;
			ot = unit_type(o);
			if (!ot || !(ot->flags & (W2UF_LAND | W2UF_AIR | W2UF_EXPLODE_DEATH | W2UF_SEA | W2UF_CRITTER))) continue;
			if (ctrl(w2.unit[o].owner) != W2_CTRL_HUMAN) continue;
			from = u->owner;
			change_owner(s, w2.unit[o].owner);
			w2_log("camp: unit %d (type 0x%02x) of player %d rescued by player %d at cycle %lu", s, w2.unit[s].type, from, w2.unit[s].owner, (unsigned long)w2.cycle);
			if (t->flags & W2UF_STORE_HALL)              /* a hall: the whole player comes along */
				for (k = 0; k < w2.unit_hi; k++)
					if (alive(k) && w2.unit[k].owner == from) change_owner(k, w2.unit[s].owner);
			if (from == w2.local_player || w2.unit[s].owner == w2.local_player) ui_message("Rescued!");
			return;
		}
}

/* f6_2624: marked units of human players on a circle count as arrived */
static void circles_check(void)
{
	int c;
	for (c = 0; c < g_ncirc; c++) {
		int cs = g_circ[c], x, y, w, h;
		const w2_unit_type *t;
		if (!unit_same(cs, g_circ_gen[c])) continue;
		t = unit_type(cs);
		w = t && t->tile_w ? t->tile_w : 2; h = t && t->tile_h ? t->tile_h : 2;
		for (y = w2.unit[cs].ty; y < w2.unit[cs].ty + h && y < w2.map.h; y++)
			for (x = w2.unit[cs].tx; x < w2.unit[cs].tx + w && x < w2.map.w; x++) {
				int o = w2.map.occ_ground[y * w2.map.w + x], p;
				if (o == W2_NO_UNIT || !marked(o)) continue;
				p = w2.unit[o].owner;
				if (ctrl(p) != W2_CTRL_HUMAN) continue;
				if ((unit_type(o)->flags & W2UF_HERO) && (g_need & 0x1000)) g_arrived[p] = (uint16_t)(g_arrived[p] + 0x1000);
				g_arrived[p]++;
				unmark(o);
				w2_log("camp: unit %d (type 0x%02x) of player %d reached the circle (%d) at cycle %lu", o, w2.unit[o].type, p, g_arrived[p] & 0xFFF, (unsigned long)w2.cycle);
			}
	}
	marks_compact();
}

/* ------------------------------------------------------------------------------------------ objectives */

typedef struct Count {
	uint16_t units, buildings, ships_ok;    /* non-building units; all buildings; transports+tankers+flying machines */
	uint16_t farms, barracks, platforms, refineries, shipyards, castles, runestones, portals, transports;
	uint16_t producers, training, unfinished;
} Count;
static Count g_cnt[W2_MAX_PLAYERS];

static void count_all(void)
{
	int s;
	memset(g_cnt, 0, sizeof g_cnt);
	for (s = 0; s < w2.unit_hi; s++) {
		const W2Unit *u = &w2.unit[s];
		Count *c;
		int t, done;
		if (!alive(s) || u->owner >= W2_MAX_PLAYERS) continue;
		c = &g_cnt[u->owner];
		t = u->type;
		if (!(u->flags & W2_UF_BUILDING)) {
			c->units++;
			if ((t & ~1) == 0x1A || (t & ~1) == 0x1C || (t & ~1) == 0x28) c->ships_ok++;
			if ((t & ~1) == 0x1C) c->transports++;
			continue;
		}
		c->buildings++;
		done = !(u->flags & W2_UF_UNDERCONST);
		if (!done) { c->unfinished++; continue; }
		if (u->order == W2_ORD_TRAIN || u->order == W2_ORD_UPGRADE_TO) c->training++;
		if (t == W2U_DARK_PORTAL) { c->portals++; continue; }
		switch (t & ~1) {
		case 0x3A: c->farms++; break;
		case 0x3C: c->barracks++; c->producers++; break;
		case 0x56: c->platforms++; break;
		case 0x54: c->refineries++; break;
		case 0x48: c->shipyards++; c->producers++; break;
		case 0x4A: case 0x58: c->producers++; break;
		case 0x5A: c->castles++; c->producers++; break;
		case 0x60: case 0x62: c->producers++; break;
		case 0x66: c->runestones++; break;
		default: break;
		}
	}
}

/* FUN_00172e88: 0 nothing, 1 defeat, 2 victory */
static int base_rule(void)
{
	int p, me = w2.local_player;
	const Count *m = &g_cnt[me];
	if (m->training == 0 && m->unfinished == 0) {
		if (m->units == 0) {
			if (m->producers == 0 || w2.player[me].gold < 400) return 1;
		} else if (g_obj & OBJ_ALL) {
			for (p = 0; p < 8; p++) {
				if (p == me) continue;
				if (g_cnt[p].buildings || g_cnt[p].units != g_cnt[p].ships_ok) return 0;
			}
			return 2;
		}
	}
	if (g_obj & (OBJ_CIRCLE | 0x400)) {
		int alive_n = 0, hero = 0, i, have;
		if (g_arrived[me] >= g_need) return 2;                       /* FUN_00172cae */
		for (i = 0; i < g_mn; i++) {
			int s = g_ml[i];
			if (!marked(s)) continue;
			alive_n++;
			if (unit_type(s)->flags & W2UF_HERO) hero++;
		}
		have = alive_n + (g_arrived[me] & 0xEFFF) + ((hero || (g_arrived[me] & 0x1000)) ? 0x1000 : 0);
		if (have < g_need) return 1;                                  /* FUN_00172cc8 */
	}
	return 0;
}

static int objective_rule(void)
{
	int me = w2.local_player, p, r;
	count_all();
	r = base_rule();
	if (r) return r;
	switch (g_obj) {
	case OBJ_FARMS:
		return (g_cnt[me].barracks && g_cnt[me].farms > 3) ? 2 : 0;
	case OBJ_PLATFORMS:
		return g_cnt[me].platforms > 3 ? 2 : 0;
	case OBJ_REFINERIES:
		for (p = 0; p < 8; p++) if (p != me && g_cnt[p].refineries) return 0;
		return 2;
	case OBJ_CASTLE_RIOT: {          /* f14_3066: destroy-all, and then a castle */
		uint16_t keep = g_obj;
		g_obj = OBJ_ALL; r = base_rule(); g_obj = keep;
		return (r == 2 && g_cnt[me].castles) ? 2 : 0;
	}
	case OBJ_RESCUE_FIRST:
		if (g_rescued[me]) { g_obj = OBJ_ALL; w2_log("camp: first rescue - now destroy all"); }
		return 0;
	case OBJ_NAVY:
		for (p = 0; p < 8; p++) if (p != me && (g_cnt[p].transports || g_cnt[p].platforms || g_cnt[p].shipyards)) return 0;
		return 2;
	case OBJ_PORTAL:
		return g_cnt[W2_NEUTRAL].portals == 0 ? 2 : 0;
	case OBJ_RUNESTONE: {
		int any = 0, other_castle = 0;
		for (p = 0; p < 8; p++) {
			if (g_cnt[p].runestones) any = 1;
			if (p != me && g_cnt[p].castles) other_castle = 1;
		}
		if (!any) return 1;
		return (!other_castle && g_cnt[me].runestones) ? 2 : 0;
	}
	case OBJ_ISLAND: {
		int s, fort = 0, yard = 0;
		if (!g_cnt[me].castles || !g_cnt[me].shipyards) return 0;
		for (s = 0; s < w2.unit_hi; s++) {
			const W2Unit *u = &w2.unit[s];
			if (!alive(s) || u->owner != me || (u->flags & W2_UF_UNDERCONST)) continue;
			if (u->type == W2U_FORTRESS && in_region(s)) fort = 1;
			if (u->type == W2U_ORC_SHIPYARD && in_region(s)) yard = 1;
		}
		return fort && yard ? 2 : 0;
	}
	default: return 0;
	}
}

int camp_tick(void)
{
	uint32_t c = w2.cycle;
	if ((c & 7) == 3) {                                   /* rescue: the marked units of non-human players */
		int i, n = g_mn;
		for (i = 0; i < n && i < g_mn; i++) {
			int s = g_ml[i];
			if (marked(s) && ctrl(w2.unit[s].owner) != W2_CTRL_HUMAN) rescue_check(s);
		}
	}
	if (g_ncirc && c % 50 == 17) circles_check();
	if (!g_on || g_result) return g_result;
	if (--g_obj_wait > 0) return 0;
	g_obj_wait = 201;                                     /* Game_Every200: counter reloaded with 200, runs at -1 */
	{
		int r = objective_rule();      /* 1 defeat, 2 victory (the Mac's order) */
		int force = w2cfg_extra_int("camp_force", 0);   /* test switch: 1 win / 2 lose at the first check */
		if ((force == 1 || force == 2) && w2.cycle >= (uint32_t)w2cfg_extra_int("camp_force_cycle", 0)) r = force == 1 ? 2 : 1;
		g_result = r == 2 ? 1 : r == 1 ? 2 : 0;
	}
	if (g_result) w2_log("camp: objective rule 0x%x -> %s at cycle %lu", g_obj, g_result == 1 ? "VICTORY" : "DEFEAT", (unsigned long)c);
	return g_result;
}

/* ------------------------------------------------------------------------------------------ debug */

int camp_rescued(int player) { return player >= 0 && player < W2_MAX_PLAYERS ? g_rescued[player] : 0; }
int camp_arrived(int player) { return player >= 0 && player < W2_MAX_PLAYERS ? (g_arrived[player] & 0xFFF) : 0; }

const char *camp_debug_state(void)
{
	static char b[160];
	int me = w2.local_player;
	snprintf(b, sizeof b, "camp %s m%d obj 0x%x need 0x%x marks %d arrived 0x%x rescued %d result %d",
	         g_on ? (g_race ? "orc" : "human") : "off", g_mission, g_obj, g_need, g_mn, g_arrived[me & 15], g_rescued[me & 15], g_result);
	return b;
}
