/* w2_stubs.c - WEAK placeholder definitions of every module function in w2.h, so each module can be
 * built and tested before its neighbours exist. A real (strong) definition in the owner file wins.
 * Host builds only (gcc ELF weak symbols); the Amiga build excludes this file once all modules exist.
 * Do not put game_* hooks here (build scripts detect game_init by grep). */
#include "w2.h"
#include "pud.h"

#define WEAK __attribute__((weak))

WEAK W2World w2;
WEAK W2View view;
WEAK int game_state;
WEAK int w2_think_period = 1;

WEAK int  game_start(const W2Setup *s) { (void)s; return -1; }
WEAK void game_end(int r) { (void)r; }

WEAK void mapdraw_draw(void) {}
WEAK void view_scroll(int dx, int dy) { (void)dx; (void)dy; }
WEAK void view_center_on(int px, int py) { (void)px; (void)py; }
WEAK int  view_to_map(int sx, int sy, int *px, int *py) { *px = sx; *py = sy; return 1; }
WEAK void fog_update(void) {}
WEAK int  fog_visible(int tx, int ty) { (void)tx; (void)ty; return 1; }
WEAK int  fog_explored(int tx, int ty) { (void)tx; (void)ty; return 1; }
WEAK void minimap_draw(int sx, int sy, int size) { (void)sx; (void)sy; (void)size; }
WEAK int  minimap_to_map(int sx, int sy, int *tx, int *ty) { (void)sx; (void)sy; *tx = 0; *ty = 0; return 0; }

WEAK int  unit_create(int type, int owner, int tx, int ty) { (void)type; (void)owner; (void)tx; (void)ty; return -1; }
WEAK void unit_remove(int slot) { (void)slot; }
WEAK void unit_tick_all(void) {}
WEAK void unit_set_anim(int slot, int k) { (void)slot; (void)k; }
WEAK int  unit_anim_event(int slot) { (void)slot; return 0; }
WEAK void unit_center(int slot, int *px, int *py) { *px = w2.unit[slot].tx * 32 + 16; *py = w2.unit[slot].ty * 32 + 16; }
WEAK int  unit_at_pixel(int px, int py) { (void)px; (void)py; return -1; }
WEAK int  units_in_rect(int x0, int y0, int x1, int y1, uint16_t *out, int max) { (void)x0; (void)y0; (void)x1; (void)y1; (void)out; (void)max; return 0; }
WEAK int  unit_dist_tiles(int a, int b) { int dx = w2.unit[a].tx - w2.unit[b].tx, dy = w2.unit[a].ty - w2.unit[b].ty; if (dx < 0) dx = -dx; if (dy < 0) dy = -dy; return dx > dy ? dx : dy; }
WEAK int  unit_dist_to_tile(int a, int tx, int ty) { int dx = w2.unit[a].tx - tx, dy = w2.unit[a].ty - ty; if (dx < 0) dx = -dx; if (dy < 0) dy = -dy; return dx > dy ? dx : dy; }
WEAK int  unit_is_enemy(int a, int b) { return (w2.player[w2.unit[a].owner].enemies >> w2.unit[b].owner) & 1; }
WEAK int  unit_owner_sees(int player, int slot) { (void)player; (void)slot; return 1; }
WEAK void unit_place(int slot, int tx, int ty) { w2.unit[slot].tx = (uint8_t)tx; w2.unit[slot].ty = (uint8_t)ty; }
WEAK void unit_unplace(int slot) { (void)slot; }
WEAK void unit_set_order(int slot, int order, int target, int tx, int ty) { W2Unit *u = &w2.unit[slot]; u->order = (uint8_t)order; u->order_state = 0; u->target = (uint16_t)target; u->goal_tx = (uint8_t)tx; u->goal_ty = (uint8_t)ty; }
WEAK void unit_set_dir_to(int slot, int px, int py) { (void)slot; (void)px; (void)py; }
WEAK const struct w2_unit_type *unit_type(int slot) { return w2_unit(w2.unit[slot].type); }
WEAK void unitdraw_draw(void) {}

WEAK int  path_find(int slot, int gx, int gy, int range, uint8_t *dirs, int max) { (void)slot; (void)gx; (void)gy; (void)range; (void)dirs; (void)max; return -1; }
WEAK int  move_toward(int slot, int gx, int gy, int range) { (void)slot; (void)gx; (void)gy; (void)range; return 1; }
WEAK int  move_toward_unit(int slot, int t, int range) { (void)slot; (void)t; (void)range; return 1; }
WEAK void move_stop(int slot) { (void)slot; }
WEAK void ord_move(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_follow(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_stand(int slot) { (void)slot; }
WEAK void ord_still(int slot) { (void)slot; }

WEAK void ord_attack(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_attack_ground(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_attack_move(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_patrol(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_die(int slot) { unit_remove(slot); }
WEAK int  combat_acquire(int slot) { (void)slot; return -1; }
WEAK void unit_damage(int v, int amount, int a) { (void)a; if (w2.unit[v].hp > amount) w2.unit[v].hp = (uint16_t)(w2.unit[v].hp - amount); else w2.unit[v].hp = 0; }
WEAK void missile_fire(int s, int m, int sx, int sy, int dx, int dy, int t) { (void)s; (void)m; (void)sx; (void)sy; (void)dx; (void)dy; (void)t; }
WEAK void missile_tick_all(void) {}
WEAK void missile_draw_all(void) {}

WEAK void ord_harvest(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_return(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_build(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_built(int slot) { w2.unit[slot].order = W2_ORD_STILL; w2.unit[slot].flags &= (uint16_t)~W2_UF_UNDERCONST; }
WEAK void ord_train(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_research(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_upgrade_to(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK void ord_repair(int slot) { w2.unit[slot].order = W2_ORD_STILL; }
WEAK int  econ_can_afford(int p, int g, int w, int o) { return w2.player[p].gold >= g && w2.player[p].wood >= w && w2.player[p].oil >= o; }
WEAK void econ_pay(int p, int g, int w, int o) { w2.player[p].gold -= g; w2.player[p].wood -= w; w2.player[p].oil -= o; }
WEAK int  econ_allowed_unit(int p, int t) { (void)p; (void)t; return 1; }
WEAK int  econ_allowed_upgrade(int p, int u) { (void)p; (void)u; return 1; }
WEAK int  econ_place_ok(int p, int t, int tx, int ty, int b) { (void)p; (void)t; (void)tx; (void)ty; (void)b; return 0; }
WEAK void econ_recount(int p) { (void)p; }
WEAK int  econ_stat(int slot, int stat) { (void)slot; (void)stat; return 0; }

WEAK void cmd_stop(int s) { unit_set_order(s, W2_ORD_STILL, W2_NO_UNIT, 0, 0); }
WEAK void cmd_stand(int s) { unit_set_order(s, W2_ORD_STAND, W2_NO_UNIT, 0, 0); }
WEAK void cmd_move(int s, int tx, int ty) { unit_set_order(s, W2_ORD_MOVE, W2_NO_UNIT, tx, ty); }
WEAK void cmd_attack(int s, int t) { unit_set_order(s, W2_ORD_ATTACK, t, 0, 0); }
WEAK void cmd_attack_ground(int s, int tx, int ty) { unit_set_order(s, W2_ORD_ATTACK_GROUND, W2_NO_UNIT, tx, ty); }
WEAK void cmd_attack_move(int s, int tx, int ty) { unit_set_order(s, W2_ORD_ATTACK_MOVE, W2_NO_UNIT, tx, ty); }
WEAK void cmd_patrol(int s, int tx, int ty) { unit_set_order(s, W2_ORD_PATROL, W2_NO_UNIT, tx, ty); }
WEAK void cmd_follow(int s, int t) { unit_set_order(s, W2_ORD_FOLLOW, t, 0, 0); }
WEAK void cmd_harvest(int s, int t, int tx, int ty) { unit_set_order(s, W2_ORD_HARVEST, t, tx, ty); }
WEAK void cmd_return(int s) { unit_set_order(s, W2_ORD_RETURN, W2_NO_UNIT, 0, 0); }
WEAK void cmd_build(int s, int type, int tx, int ty) { (void)type; unit_set_order(s, W2_ORD_BUILD, W2_NO_UNIT, tx, ty); }
WEAK void cmd_train(int b, int type) { (void)b; (void)type; }
WEAK void cmd_research(int b, int u) { (void)b; (void)u; }
WEAK void cmd_upgrade_to(int b, int type) { (void)b; (void)type; }
WEAK void cmd_cancel(int b) { (void)b; }
WEAK void cmd_repair(int s, int t) { unit_set_order(s, W2_ORD_REPAIR, t, 0, 0); }
WEAK void cmd_smart(int s, int tx, int ty, int t) { if (t >= 0 && t != W2_NO_UNIT) cmd_attack(s, t); else cmd_move(s, tx, ty); }

WEAK void ui_init(void) {}
WEAK void ui_draw(void) {}
WEAK int  ui_event(const void *ev) { (void)ev; return 0; }
WEAK void ui_message(const char *t) { (void)t; }
WEAK void ui_tick(void) {}
WEAK void ai_init(void) {}
WEAK void ai_tick(void) {}
WEAK int  snd_init(void) { return 0; }
WEAK void snd_shutdown(void) {}
WEAK void snd_play(int id, int px, int py) { (void)id; (void)px; (void)py; }
WEAK void snd_unit(int slot, int what) { (void)slot; (void)what; }
WEAK void snd_music(int on) { (void)on; }
WEAK void snd_service(void) {}
WEAK void menu_enter(void) {}
WEAK void menu_draw(void) {}
WEAK int  menu_event(const void *ev) { (void)ev; return 0; }
WEAK void menu_tick(void) {}
WEAK void endscreen_enter(int r) { (void)r; }
WEAK int  lores_prepare(void) { return 0; }
