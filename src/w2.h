/* w2.h - shared contract of the AmiW2 engine (owned by the orchestrator; modules do not edit it).
 *
 * Conventions
 *  - plain C (gnu99), no floats, no sprintf, endian-neutral file reads.
 *  - one module per .c file; module-private per-unit data lives in that module's own arrays
 *    indexed by unit slot (0..W2_MAX_UNITS-1), never in new fields here.
 *  - coordinates: tiles are 32x32 pixels in "map pixels" (hires). Lores halves only at draw time
 *    (view.scale_shift = 1); simulation is identical in both modes.
 *  - directions: 0 = north, clockwise, 8 directions (0..7).
 *  - one logic cycle = one game cycle of the original; render runs as fast as it can.
 */
#ifndef W2_H
#define W2_H

#include <stdint.h>
#include "w2_tables.h"

#define W2_TILE        32
#define W2_MAP_MAX     128
#define W2_MAX_UNITS   1024
#define W2_MAX_PLAYERS 16          /* PUD owners: 0..7 players, 15 = neutral */
#define W2_NEUTRAL     15
#define W2_MAX_SELECT  9
#define W2_NO_UNIT     0xFFFF

/* ---- map --------------------------------------------------------------------------------- */
/* tile flags (W2_TF_*) - derived from the tileset's tile type per megatile + dynamic state */
#define W2_TF_LAND      0x0001
#define W2_TF_WATER     0x0002
#define W2_TF_COAST     0x0004
#define W2_TF_TREE      0x0008
#define W2_TF_ROCK      0x0010
#define W2_TF_WALL      0x0020
#define W2_TF_BUILDING  0x0040   /* occupied by a building (dynamic) */
#define W2_TF_NOBUILD   0x0080   /* not buildable terrain */
#define W2_TF_NOWALK    0x0100   /* static: impassable for land units */

/* eras: enum w2_era in w2_tables.h (FOREST/summer, WINTER, WASTELAND; no swamp on the original CD) */

typedef struct W2Map {
	int w, h;                                  /* tiles */
	int era;                                   /* W2_ERA_* */
	uint16_t tile[W2_MAP_MAX * W2_MAP_MAX];     /* PUD MTXM value (tileset tile id) */
	uint16_t mega[W2_MAP_MAX * W2_MAP_MAX];     /* resolved megatile index for drawing */
	uint16_t flags[W2_MAP_MAX * W2_MAP_MAX];    /* W2_TF_* */
	uint8_t  wood[W2_MAP_MAX * W2_MAP_MAX];     /* remaining chops on a tree tile */
	uint8_t  explored[W2_MAP_MAX * W2_MAP_MAX]; /* bit p = explored by player p (0..7) */
	uint8_t  visible[W2_MAP_MAX * W2_MAP_MAX];  /* bit p = currently seen by player p */
	uint16_t occ_ground[W2_MAP_MAX * W2_MAP_MAX];/* unit slot standing on tile (land+sea), W2_NO_UNIT */
	uint16_t occ_air[W2_MAP_MAX * W2_MAP_MAX];
	uint32_t version;                          /* bumped on any terrain change (trees cut, walls) */
} W2Map;

/* ---- players ----------------------------------------------------------------------------- */
enum { W2_CTRL_NOBODY, W2_CTRL_HUMAN, W2_CTRL_COMPUTER, W2_CTRL_RESCUE_PASSIVE, W2_CTRL_RESCUE_ACTIVE, W2_CTRL_NEUTRAL };

typedef struct W2Player {
	uint8_t  control;      /* W2_CTRL_* */
	uint8_t  race;         /* 0 human, 1 orc */
	uint8_t  colour;       /* index into the player-colour table */
	uint8_t  ai_type;
	int32_t  gold, wood, oil;
	int16_t  food_supply, food_used;
	uint16_t start_tx, start_ty;
	uint32_t upgrades;     /* bitmask of researched upgrades (see w2_tables.h) */
	uint8_t  upgrade_level[32];
	uint32_t allowed_units, allowed_spells;
	uint16_t enemies;      /* bit mask of hostile players */
	uint16_t unit_count, building_count;
	uint8_t  defeated;
} W2Player;

/* ---- units ------------------------------------------------------------------------------- */
#define W2_UF_ALIVE       0x0001
#define W2_UF_SELECTED    0x0002
#define W2_UF_BUILDING    0x0004  /* type is a building */
#define W2_UF_UNDERCONST  0x0008  /* building under construction */
#define W2_UF_INSIDE      0x0010  /* inside a mine / transport / building (not drawn) */
#define W2_UF_DYING       0x0020
#define W2_UF_AIR         0x0040
#define W2_UF_SEA         0x0080
#define W2_UF_HIDDEN      0x0100  /* not on map (removed) */

/* orders */
enum {
	W2_ORD_NONE, W2_ORD_STILL, W2_ORD_STAND, W2_ORD_MOVE, W2_ORD_ATTACK, W2_ORD_ATTACK_GROUND,
	W2_ORD_ATTACK_MOVE, W2_ORD_PATROL, W2_ORD_FOLLOW, W2_ORD_HARVEST, W2_ORD_RETURN, W2_ORD_BUILD,
	W2_ORD_BUILT /* building in progress (on the building) */, W2_ORD_TRAIN, W2_ORD_RESEARCH,
	W2_ORD_UPGRADE_TO, W2_ORD_REPAIR, W2_ORD_DIE, W2_ORD_LOAD, W2_ORD_UNLOAD, W2_ORD_CAST,
	W2_ORD_DEMOLISH, W2_ORD_COUNT
};

typedef struct W2Unit {
	uint16_t flags;        /* W2_UF_* */
	uint8_t  type;         /* PUD unit id, see w2_tables.h */
	uint8_t  owner;
	uint8_t  tx, ty;       /* tile (top-left tile for buildings) */
	int8_t   ox, oy;       /* pixel offset from the tile origin while moving (-31..31) */
	uint8_t  dir;          /* 0..7 */
	uint8_t  order, order_state;
	uint16_t hp;
	uint8_t  mana;
	uint8_t  res_kind;     /* carried resource 0 none 1 gold 2 wood 3 oil */
	uint16_t res_amount;   /* carried amount, or for mines/oil patches: remaining /100 */
	uint32_t res_left;     /* gold mine / oil patch remaining */
	uint16_t target;       /* unit slot of the order target or W2_NO_UNIT */
	uint8_t  goal_tx, goal_ty;
	uint16_t timer;        /* generic order timer (cycles) */
	uint16_t progress;     /* build/train/research progress (cycles) */
	uint8_t  queue_type;   /* what is being trained/researched/built (type id or upgrade id) */
	uint8_t  anim_kind;    /* W2_ANIM_* currently playing */
	uint8_t  anim_step;    /* index in the animation sequence */
	uint8_t  anim_wait;    /* cycles left in this step */
	uint16_t frame;        /* sprite frame to draw (before direction mirroring) */
	uint16_t kills;
	uint16_t ttl;          /* summoned units, corpses */
	uint8_t  armor_bonus, damage_bonus;
	uint8_t  group;        /* control group bits */
} W2Unit;

/* ---- the world --------------------------------------------------------------------------- */
typedef struct W2World {
	W2Map    map;
	W2Player player[W2_MAX_PLAYERS];
	W2Unit   unit[W2_MAX_UNITS];
	uint16_t unit_hi;              /* highest used slot + 1 */
	uint32_t cycle;                /* logic cycles since start */
	uint32_t rng;                  /* game RNG state (algorithm: see rng.c / RE_NOTES) */
	uint8_t  local_player;
	uint8_t  game_over;            /* 0 running, 1 victory, 2 defeat */
	uint16_t select[W2_MAX_SELECT];
	uint8_t  select_n;
} W2World;

extern W2World w2;

/* =====================================================================================================
 * MODULE API. Owner file in [brackets]. Everything uses HIRES map pixels (tile*32). Slots are unit
 * indices into w2.unit[]. Functions return 0/-1 or a boolean unless noted.
 * ===================================================================================================== */

/* [rng.c] */
uint32_t w2_rand(void);                 /* deterministic game RNG, 0..0x7FFF */

/* [pud.c] see pud.h */

/* [game.c] the game shell: owns w2, view, the game_* hooks (w2cfg.h), the state machine
 * menu <-> play <-> end screen, the logic-cycle driver and the logic frameskip. */
enum { GS_MENU, GS_PLAY, GS_END };
extern int game_state;
typedef struct W2Setup {
	int  builtin_pud;          /* War Data entry, or 0 to use pud_path */
	char pud_path[128];
	int  local_player;         /* -1 = first human slot of the map */
	int  local_race;           /* 0 human, 1 orc, -1 map default */
	int  opponents;            /* 0 = map default */
	int  resources;            /* 0 map default, 1 low, 2 medium, 3 high */
	int  units;                /* 0 map default, 1 one peasant only */
} W2Setup;
int  game_start(const W2Setup *s);     /* load + enter GS_PLAY; 0 = ok */
void game_end(int result);             /* 1 victory 2 defeat 3 quit to menu */
extern int w2_think_period;            /* logic frameskip: units think when ((cycle + slot) % period) == 0 */
static inline int unit_thinks(int slot) { return w2_think_period <= 1 || ((w2.cycle + (uint32_t)slot) % (uint32_t)w2_think_period) == 0; }

/* view (owned by game.c / mapdraw.c) */
typedef struct W2View {
	int map_x, map_y;          /* top-left of the viewport in HIRES map pixels */
	int scr_x, scr_y, scr_w, scr_h;   /* viewport rectangle on screen in HIRES screen pixels (640x480 space) */
	int lores;                 /* 1: everything on screen is drawn at half size (320x240 buffer) */
	uint8_t *buf; int pitch;   /* chunky target for this frame */
} W2View;
extern W2View view;
/* helpers: hires screen coordinate -> buffer coordinate */
#define VX(x) ((x) >> view.lores)
#define VY(y) ((y) >> view.lores)

/* [mapdraw.c] terrain, fog, minimap, scrolling */
void mapdraw_draw(void);                       /* terrain + fog of the viewport into view.buf */
void view_scroll(int dx, int dy);              /* clamps to the map */
void view_center_on(int px, int py);
int  view_to_map(int sx, int sy, int *px, int *py);   /* hires screen -> map pixels; 0 if outside viewport */
void fog_update(void);                         /* recompute w2.map.visible/explored (call every few cycles) */
int  fog_visible(int tx, int ty);              /* for the local player */
int  fog_explored(int tx, int ty);
void minimap_draw(int sx, int sy, int size);   /* at hires screen pos, size 128 hires px */
int  minimap_to_map(int sx, int sy, int *tx, int *ty);

/* [unit.c] unit life cycle, occupancy, animation, order dispatch, selection-independent queries */
int  unit_create(int type, int owner, int tx, int ty);   /* slot or -1; places it (finished building) */
void unit_remove(int slot);                    /* off the map and freed (no corpse) */
void unit_tick_all(void);                      /* one logic cycle: animations, orders, deaths */
void unit_set_anim(int slot, int anim_kind);   /* W2_ANIM_* from w2_tables.h, restarts it */
int  unit_anim_event(int slot);                /* flags of the anim step entered this cycle (W2AF_*), 0 if none */
void unit_center(int slot, int *px, int *py);  /* centre in map pixels */
int  unit_at_pixel(int px, int py);            /* topmost unit under a map pixel, -1 */
int  units_in_rect(int x0, int y0, int x1, int y1, uint16_t *out, int max);  /* map pixels */
int  unit_dist_tiles(int a, int b);            /* chebyshev distance between footprints */
int  unit_dist_to_tile(int a, int tx, int ty);
int  unit_is_enemy(int a, int b);
int  unit_owner_sees(int player, int slot);
void unit_place(int slot, int tx, int ty);     /* occupancy bookkeeping when moving tile to tile */
void unit_unplace(int slot);
void unit_set_order(int slot, int order, int target, int tx, int ty);   /* clears state, order_state=0 */
void unit_set_dir_to(int slot, int px, int py);
const struct w2_unit_type *unit_type(int slot);
/* [unitdraw.c] */
void unitdraw_draw(void);                      /* all units/buildings/corpses in the viewport (+selection) */

/* [move.c + path.c] movement */
int  path_find(int slot, int gx, int gy, int range, uint8_t *dirs, int max);   /* steps found, -1 none */
/* Drive a unit toward (gx,gy) until within 'range' tiles of it (0 = onto it). Call every cycle from
 * an order handler. Returns 0 still walking, 1 arrived, -1 unreachable (gave up). */
int  move_toward(int slot, int gx, int gy, int range);
int  move_toward_unit(int slot, int target, int range);
void move_stop(int slot);                      /* stop walking at the next tile boundary */
void ord_move(int slot);                       /* W2_ORD_MOVE handler */
void ord_follow(int slot);
void ord_stand(int slot);
void ord_still(int slot);                      /* idle; calls combat_acquire for armed units */
/* [transport.c] transports (docking, load, unload) */
void ord_load(int slot);                       /* W2_ORD_LOAD: a passenger boards / a transport picks up */
void ord_unload(int slot);                     /* W2_ORD_UNLOAD: a transport lands its passengers */
int  transport_smart(int slot, int tx, int ty, int target);   /* right-click handled as board/land? 1 = done */
/* [spells.c] spells, mana, timed effects, demolish */
void ord_cast(int slot);                       /* W2_ORD_CAST */
void ord_demolish(int slot);                   /* W2_ORD_DEMOLISH */
void spells_tick_all(void);                    /* once per game cycle after the units: mana, timed effects */

/* [combat.c + missile.c] */
void ord_attack(int slot);
void ord_attack_ground(int slot);
void ord_attack_move(int slot);
void ord_patrol(int slot);
void ord_die(int slot);
int  combat_acquire(int slot);                 /* idle auto-target; returns target slot or -1 */
void unit_damage(int victim, int amount, int attacker);   /* applies hp loss, death, under-attack alerts */
void missile_fire(int shooter, int missile_type, int sx, int sy, int dx, int dy, int target);
void missile_tick_all(void);
void missile_draw_all(void);

/* [economy.c] gathering, building, training, research, food, costs */
void ord_harvest(int slot);
void ord_return(int slot);
void ord_build(int slot);                      /* worker going to build */
void ord_built(int slot);                      /* the building being constructed */
void ord_train(int slot);
void ord_research(int slot);
void ord_upgrade_to(int slot);
void ord_repair(int slot);
int  econ_can_afford(int player, int gold, int wood, int oil);
void econ_pay(int player, int gold, int wood, int oil);
int  econ_allowed_unit(int player, int type);  /* dependencies met */
int  econ_allowed_upgrade(int player, int upg);
int  econ_place_ok(int player, int type, int tx, int ty, int builder);
void econ_recount(int player);                 /* food, unit counts */
int  econ_stat(int slot, int stat);            /* effective stat with upgrades (enum w2_stat) */

/* commands: issued by UI or AI, validated, set the order. queue = shift-click append (may be ignored) */
void cmd_stop(int slot);
void cmd_stand(int slot);
void cmd_move(int slot, int tx, int ty);
void cmd_attack(int slot, int target);          /* target slot */
void cmd_attack_ground(int slot, int tx, int ty);
void cmd_attack_move(int slot, int tx, int ty);
void cmd_patrol(int slot, int tx, int ty);
void cmd_follow(int slot, int target);
void cmd_harvest(int slot, int target, int tx, int ty);   /* target mine/platform slot, or tree tile */
void cmd_return(int slot);
void cmd_build(int slot, int type, int tx, int ty);
void cmd_train(int bslot, int type);
void cmd_research(int bslot, int upg);
void cmd_upgrade_to(int bslot, int type);
void cmd_cancel(int bslot);
void cmd_repair(int slot, int target);
void cmd_smart(int slot, int tx, int ty, int target);     /* right-click default action */
/* [combat.c] owns cmd_stop/stand/attack/attack_ground/attack_move/patrol/smart;
 * [move.c] owns cmd_move/follow; [economy.c] owns the rest. */

/* [ui.c] in-game interface */
void ui_init(void);
void ui_draw(void);                             /* panel, resources, minimap frame, buttons, messages, cursor */
int  ui_event(const void *amiga_gfx_event);     /* AmigaGfxEvent*; 1 = consumed */
void ui_message(const char *text);
void ui_tick(void);
/* [ai.c] */
void ai_init(void);                             /* for every computer player */
void ai_tick(void);                             /* once per logic cycle */
/* [snd.c] */
int  snd_init(void);
void snd_shutdown(void);
void snd_play(int sound_id, int px, int py);    /* w2_sound id; px<0 = UI sound (no position) */
void snd_unit(int slot, int what);              /* 0 selected 1 acknowledge 2 ready 3 help 4 dead */
void snd_music(int on);                         /* start/stop game music */
void snd_service(void);                         /* once per drawn frame */
/* [menu.c] */
void menu_enter(void);
void menu_draw(void);
int  menu_event(const void *amiga_gfx_event);
void menu_tick(void);
void endscreen_enter(int result);
/* [lores.c] builds halved art used by assets.c in 320x240 */
int  lores_prepare(void);                       /* 0 = ok (may build a cache file the first time) */

#endif
