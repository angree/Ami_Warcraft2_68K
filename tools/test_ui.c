/* test_ui.c - host test of the in-game interface (src/ui.c, src/ui_cursor.c).
 *
 * Runs the real game shell (game.c + unit.c + unitdraw.c + mapdraw.c) on built-in skirmish map 220,
 * drives the UI with synthetic AmigaGfxEvents through game_event() and programmatic selections, and
 * writes PNG shots (host_shot) into W2_SHOTDIR. The current hardware pointer shape is composited onto
 * each shot (the host backend does not draw pointers). Env: W2_TEST_DATA (data dir), W2_TEST_LORES=1,
 * W2_TEST_RACE=0/1. Economy is not written yet: the econ_* functions the UI needs are test stand-ins
 * below (dependencies from w2_dependencies, simple placement test, food recount).
 * Exit code = number of failed checks. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "w2.h"
#include "ui.h"
#include "assets.h"
#include "w2cfg.h"
#include "game.h"
#include "unit.h"
#include "amiga_gfx.h"
#include "w2_sys.h"

int host_shot(const char *name);

static unsigned long g_frames;
static int W, H, L, g_fail, g_mx, g_my;

void w2_log(const char *fmt, ...)
{
	va_list a;
	va_start(a, fmt); vfprintf(stdout, fmt, a); va_end(a);
	fputc('\n', stdout); fflush(stdout);
}
unsigned long w2_tick_ms(void) { return 40; }
unsigned long w2_frame_count(void) { return g_frames; }

#define CHECK(c, ...) do { if (c) { printf("ok   "); } else { printf("FAIL "); g_fail++; } printf(__VA_ARGS__); printf("\n"); } while (0)

/* ---------------------------------------------------------------- economy stand-ins (test only) */
static int t_owns(int p, int type)
{
	int i;
	for (i = 0; i < w2.unit_hi; i++)
		if ((w2.unit[i].flags & W2_UF_ALIVE) && !(w2.unit[i].flags & W2_UF_UNDERCONST) && w2.unit[i].owner == p && w2.unit[i].type == type) return 1;
	return 0;
}
static int t_has_upg(int p, int u) { return u < 32 ? (int)((w2.player[p].upgrades >> u) & 1) : w2.player[p].upgrade_level[u - 32] != 0; }
static int t_deps(int p, int kind, int id)
{
	int alt, any = 0, i;
	for (alt = 0; alt < 8; alt++) {
		int n = 0, ok = 1;
		for (i = 0; i < w2_num_dependencies; i++) {
			const w2_dependency *d = &w2_dependencies[i];
			if (d->kind != kind || d->id != id || d->alt != alt) continue;
			n++; any = 1;
			if (d->req_kind == W2_DEP_UNIT ? !t_owns(p, d->req_id) : !t_has_upg(p, d->req_id)) ok = 0;
		}
		if (n && ok) return 1;
	}
	return !any;
}
/* WEAK: when src/economy.c exists its versions win; test_ui.o is linked before w2_stubs.o so these
 * beat the placeholders otherwise. */
#define TWEAK __attribute__((weak))
TWEAK int econ_allowed_unit(int p, int type) { return t_deps(p, W2_DEP_UNIT, type); }
TWEAK int econ_allowed_upgrade(int p, int upg) { return t_deps(p, W2_DEP_UPGRADE, upg); }
static int t_place_ok(int p, int type, int tx, int ty, int b)
{
	const w2_unit_type *t = w2_unit(type);
	int x, y;
	(void)p;
	if (!t) return 0;
	for (y = ty; y < ty + t->tile_h; y++) for (x = tx; x < tx + t->tile_w; x++) {
		int f, o;
		if (x < 0 || y < 0 || x >= w2.map.w || y >= w2.map.h) return 0;
		f = w2.map.flags[y * w2.map.w + x];
		o = w2.map.occ_ground[y * w2.map.w + x];
		if (!(f & W2_TF_LAND) || (f & (W2_TF_NOBUILD | W2_TF_BUILDING | W2_TF_TREE | W2_TF_ROCK | W2_TF_WALL | W2_TF_NOWALK | W2_TF_WATER))) return 0;
		if (o != W2_NO_UNIT && o != b) return 0;
	}
	return 1;
}
TWEAK int econ_place_ok(int p, int type, int tx, int ty, int b) { return t_place_ok(p, type, tx, ty, b); }
TWEAK void econ_recount(int p)
{
	int i, sup = 0, used = 0;
	for (i = 0; i < w2.unit_hi; i++) {
		const w2_unit_type *t;
		if (!(w2.unit[i].flags & W2_UF_ALIVE) || w2.unit[i].owner != p) continue;
		t = w2_unit(w2.unit[i].type);
		if (!t) continue;
		if (t->food > 0 && !(w2.unit[i].flags & W2_UF_UNDERCONST)) sup += t->food;
		if (t->food < 0) used -= t->food;
	}
	w2.player[p].food_supply = (int16_t)sup; w2.player[p].food_used = (int16_t)used;
}

/* ---------------------------------------------------------------- command recorders (test only)
 * Linked with -Wl,--wrap=cmd_X (build_test_ui.sh): every command the UI issues is recorded, then
 * passed on to the real module (or the w2_stubs placeholder). */
static char g_cmd[160]; static int g_ncmd;
static void rec(const char *fmt, ...)
{
	va_list a;
	va_start(a, fmt); vsnprintf(g_cmd, sizeof g_cmd, fmt, a); va_end(a);
	g_ncmd++;
	printf("cmd  %s\n", g_cmd);
}
void __real_cmd_build(int s, int type, int tx, int ty);
void __real_cmd_smart(int s, int tx, int ty, int t);
void __real_cmd_move(int s, int tx, int ty);
void __real_cmd_attack(int s, int t);
void __real_cmd_attack_move(int s, int tx, int ty);
void __real_cmd_train(int b, int type);
void __real_cmd_stop(int s);
void __wrap_cmd_build(int s, int type, int tx, int ty);
void __wrap_cmd_smart(int s, int tx, int ty, int t);
void __wrap_cmd_move(int s, int tx, int ty);
void __wrap_cmd_attack(int s, int t);
void __wrap_cmd_attack_move(int s, int tx, int ty);
void __wrap_cmd_train(int b, int type);
void __wrap_cmd_stop(int s);
void __wrap_cmd_build(int s, int type, int tx, int ty) { rec("build %d type %02x at %d,%d", s, type, tx, ty); __real_cmd_build(s, type, tx, ty); }
static unsigned char g_smart[W2_MAX_UNITS];
void __wrap_cmd_smart(int s, int tx, int ty, int t) { if (s >= 0 && s < W2_MAX_UNITS) g_smart[s] = 1; rec("smart %d -> %d,%d target %d", s, tx, ty, t); __real_cmd_smart(s, tx, ty, t); }
void __wrap_cmd_move(int s, int tx, int ty) { rec("move %d -> %d,%d", s, tx, ty); __real_cmd_move(s, tx, ty); }
void __wrap_cmd_attack(int s, int t) { rec("attack %d -> unit %d", s, t); __real_cmd_attack(s, t); }
void __wrap_cmd_attack_move(int s, int tx, int ty) { rec("attack_move %d -> %d,%d", s, tx, ty); __real_cmd_attack_move(s, tx, ty); }
void __wrap_cmd_train(int b, int type) { rec("train %d type %02x", b, type); __real_cmd_train(b, type); }
void __wrap_cmd_stop(int s) { rec("stop %d", s); __real_cmd_stop(s); }

/* ---------------------------------------------------------------- helpers */
static int nearest(int r, int g, int b)
{
	const unsigned char *pal = assets_palette();
	int i, best = 0; long bd = 0x7fffffffL;
	for (i = 1; i < 256; i++) {
		long dr = pal[i * 3] - r, dg = pal[i * 3 + 1] - g, db = pal[i * 3 + 2] - b, d;
		if (i >= 17 && i <= 19) continue;
		d = dr * dr + dg * dg + db * db;
		if (d < bd) { bd = d; best = i; }
	}
	return best;
}

/* draw a pointer image like the hardware would: a pointer pixel = 2 hires px wide, 1 line high in
 * hires; 1x1 in lores (the shape is already halved vertically there) */
static void draw_pointer(int shape, int bx, int by, int hot)
{
	const unsigned char *pix; int w, h, hx, hy, x, y, c[4], race = w2.player[w2.local_player].race;
	unsigned char rgb[9];
	unsigned char *buf = amigagfx_chunky(); int pitch = amigagfx_pitch(), sx = L ? 1 : 2;
	if (shape < 0 || ui_cursor_debug(shape, race, &pix, &w, &h, &hx, &hy, rgb)) return;
	for (x = 0; x < 3; x++) c[x + 1] = nearest(rgb[x * 3], rgb[x * 3 + 1], rgb[x * 3 + 2]);
	if (hot) { bx -= hx * sx; by -= hy; }
	for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
		int v = pix[y * w + x], i;
		if (!v) continue;
		for (i = 0; i < sx; i++) {
			int px = bx + x * sx + i, py = by + y;
			if (px >= 0 && py >= 0 && px < W && py < H) buf[py * pitch + px] = (unsigned char)c[v];
		}
	}
}

static void frame(const char *shot)
{
	unsigned char *buf = amigagfx_chunky();
	game_draw(buf, amigagfx_pitch(), W, H);
	g_frames++;
	if (shot) {
		char n[80];
		draw_pointer(ui_cursor_shape(), g_mx >> L, g_my >> L, 1);
		amigagfx_blit(0, 0, W, H);
		snprintf(n, sizeof n, "%s_%s", L ? "lo" : "hi", shot);
		host_shot(n);
		printf("STATE %-16s %s\n", shot, ui_debug_state());
	} else amigagfx_blit(0, 0, W, H);
}

static void ticks(int n) { while (n-- > 0) { game_tick(); w2_wait_tof(); } }

static void ev(int type, int x, int y, int code)
{
	AmigaGfxEvent e;
	e.type = type; e.x = x >> L; e.y = y >> L; e.code = code;
	if (type != AMIGAGFX_EV_KEY) { g_mx = x; g_my = y; }
	else { e.x = g_mx >> L; e.y = g_my >> L; }
	game_event(&e);
}
static void move(int x, int y) { ev(AMIGAGFX_EV_MOUSEMOVE, x, y, 0); }
static void click(int x, int y, int button)
{
	move(x, y);
	ev(AMIGAGFX_EV_MOUSEDOWN, x, y, button);
	ev(AMIGAGFX_EV_MOUSEUP, x, y, button);
	ticks(1);
}
static void key(int code) { ev(AMIGAGFX_EV_KEY, 0, 0, code); ev(AMIGAGFX_EV_KEY, 0, 0, code | 0x80); }

static void unit_screen(int s, int *x, int *y)
{
	int px, py;
	unit_center(s, &px, &py);
	*x = px - view.map_x + view.scr_x; *y = py - view.map_y + view.scr_y;
}
static void click_unit(int s)
{
	int x, y;
	unit_screen(s, &x, &y);
	if (x < view.scr_x + 8 || y < view.scr_y + 8 || x >= view.scr_x + view.scr_w - 8 || y >= view.scr_y + view.scr_h - 8) {
		int px, py; unit_center(s, &px, &py); view_center_on(px, py); unit_screen(s, &x, &y);
	}
	click(x, y, AMIGAGFX_BUTTON_LEFT);
	ticks(20);          /* no accidental double click with the next one */
}

static int find_own(int p, unsigned long flag, int type)
{
	int i;
	for (i = 0; i < w2.unit_hi; i++) {
		const w2_unit_type *t;
		if (!(w2.unit[i].flags & W2_UF_ALIVE) || w2.unit[i].owner != p) continue;
		t = w2_unit(w2.unit[i].type);
		if (type >= 0 ? w2.unit[i].type == type : (t && (t->flags & flag))) return i;
	}
	return -1;
}

static int free_tile(int cx, int cy, int w, int h, int *ox, int *oy)
{
	int r, x, y;
	for (r = 2; r < 14; r++)
		for (y = cy - r; y <= cy + r; y++) for (x = cx - r; x <= cx + r; x++)
			if (t_place_ok(0, w == 1 ? W2U_FOOTMAN : W2U_HUMAN_BARRACKS, x, y, -1)) {
				int a, b, ok = 1;
				for (b = y - 1; b <= y + h && ok; b++) for (a = x - 1; a <= x + w && ok; a++)
					if (a < 0 || b < 0 || a >= w2.map.w || b >= w2.map.h || w2.map.occ_ground[b * w2.map.w + a] != W2_NO_UNIT) ok = 0;
				if (ok) { *ox = x; *oy = y; return 0; }
			}
	return -1;
}

static int make(int type, int p, int cx, int cy)
{
	const w2_unit_type *t = w2_unit(type);
	int tx, ty, s;
	if (!t || free_tile(cx, cy, t->tile_w, t->tile_h, &tx, &ty)) return -1;
	s = unit_create(type, p, tx, ty);
	return s;
}

int main(void)
{
	const char *data = getenv("W2_TEST_DATA");
	const char *lo = getenv("W2_TEST_LORES"), *race = getenv("W2_TEST_RACE");
	W2Setup st;
	int p, worker, hall, mine, i, s, x, y, army[8], na = 0, cx, cy, mage = -1, foot = -1, rax = -1, vx;
	w2cfg_defaults(&w2_cfg);
	snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", data ? data : "PROGDIR:data");
	L = lo && *lo == '1';
	w2_cfg.resolution = L ? W2_RES_LORES : W2_RES_HIRES;
	W = 640 >> L; H = 480 >> L;
	if (amigagfx_open(W, H, 0, AMIGAGFX_BACKEND_AGA)) { printf("cannot open screen\n"); return 99; }
	if (assets_init()) { printf("assets_init failed\n"); return 98; }
	memset(&st, 0, sizeof st);
	st.builtin_pud = 220; st.local_player = -1; st.local_race = race ? atoi(race) : -1;
	if (game_start(&st)) { printf("game_start failed\n"); return 97; }
	p = w2.local_player;
	w2.player[p].gold = 4500; w2.player[p].wood = 2300; w2.player[p].oil = 1200;
	worker = find_own(p, W2UF_WORKER, -1);
	hall = find_own(p, W2UF_STORE_HALL, -1);
	for (mine = 0; mine < w2.unit_hi; mine++) if ((w2.unit[mine].flags & W2_UF_ALIVE) && w2.unit[mine].type == W2U_GOLD_MINE) break;
	printf("local player %d race %d worker %d hall %d mine %d units %d era %d\n", p, w2.player[p].race, worker, hall, mine, w2.unit_hi, w2.map.era);
	CHECK(worker >= 0, "a worker of the local player exists");
	if (worker < 0) return 1;
	unit_center(worker, &cx, &cy); cx >>= 5; cy >>= 5;
	if (hall < 0) hall = make(w2.player[p].race ? W2U_GREAT_HALL : W2U_TOWN_HALL, p, cx, cy);
	{   /* a small army + buildings around the start */
		int hum = !w2.player[p].race;
		static const int th[] = { W2U_FOOTMAN, W2U_FOOTMAN, W2U_ARCHER, W2U_KNIGHT, W2U_MAGE, W2U_BALLISTA };
		static const int to[] = { W2U_GRUNT, W2U_GRUNT, W2U_AXETHROWER, W2U_OGRE, W2U_DEATH_KNIGHT, W2U_CATAPULT };
		for (i = 0; i < 6; i++) { s = make(hum ? th[i] : to[i], p, cx + 3, cy + 3); if (s >= 0) army[na++] = s; }
		foot = na > 0 ? army[0] : -1; mage = na > 4 ? army[4] : -1;
		rax = make(hum ? W2U_HUMAN_BARRACKS : W2U_ORC_BARRACKS, p, cx - 4, cy + 2);
		make(hum ? W2U_FARM : W2U_PIG_FARM, p, cx - 3, cy - 3);
		econ_recount(p);
		if (mage >= 0) w2.unit[mage].mana = 120;
	}
	printf("army %d units, barracks %d\n", na, rax);
	unit_center(worker, &x, &y); view_center_on(x, y);
	move(400, 240);
	ticks(2);
	frame("01_start");

	click_unit(worker);
	frame("02_worker");
	CHECK(w2.select_n == 1 && w2.select[0] == worker, "click selects the worker (sel=%d)", w2.select_n);

	key(0x35);  /* b */
	frame("03_build_basic");
	CHECK(strstr(ui_debug_state(), "level=1") != NULL, "'b' opens the basic build menu");

	key(0x23);  /* f = farm */
	CHECK(strstr(ui_debug_state(), "mode=place") != NULL, "'f' enters placement");
	{   /* ghost on free ground, then on the hall */
		int tx, ty, px2, py2;
		if (!free_tile(cx, cy, 2, 2, &tx, &ty)) {
			view_center_on(tx * 32 + 32, ty * 32 + 32);
			move(tx * 32 + 32 - view.map_x + view.scr_x, ty * 32 + 32 - view.map_y + view.scr_y);
			frame("04_ghost_ok");
		}
		if (hall >= 0) { unit_center(hall, &px2, &py2); view_center_on(px2, py2); unit_screen(hall, &px2, &py2); move(px2 + 20, py2 + 20); frame("05_ghost_bad"); }
		if (!free_tile(cx, cy, 2, 2, &tx, &ty)) {
			view_center_on(tx * 32 + 32, ty * 32 + 32);
			click(tx * 32 + 32 - view.map_x + view.scr_x, ty * 32 + 32 - view.map_y + view.scr_y, AMIGAGFX_BUTTON_LEFT);
			{ char want[80]; snprintf(want, sizeof want, "build %d type %02x at %d,%d", worker, w2.player[p].race ? W2U_PIG_FARM : W2U_FARM, tx, ty);
			  CHECK(!strcmp(g_cmd, want), "placing the farm issues '%s' (got '%s')", want, g_cmd); }
		}
	}
	key(0x45);  /* esc: nothing to cancel -> menu */
	CHECK(ui_menu_active(), "ESC with nothing to cancel opens the game menu");
	key(0x45);
	CHECK(!ui_menu_active(), "ESC closes the game menu");
	key(0x35); key(0x23); key(0x45);   /* b f esc: placement cancelled */
	CHECK(strstr(ui_debug_state(), "mode=normal level=0") != NULL, "ESC cancels placement");

	if (hall >= 0) {
		click_unit(hall);
		frame("06_hall");
		move(9 + 23, 340 + 19);
		frame("07_hint");
		CHECK(strstr(ui_debug_state(), "status='TRAIN") != NULL, "hovering train shows a hint");
		w2.unit[hall].order = W2_ORD_TRAIN; w2.unit[hall].queue_type = w2.player[p].race ? W2U_PEON : W2U_PEASANT;
		w2.unit[hall].progress = (uint16_t)(w2_time_to_cycles(w2_unit(w2.unit[hall].queue_type)->build_time) * 2 / 5);
		frame("08_training");
		CHECK(strstr(ui_debug_state(), "9:91/^") != NULL, "training building shows cancel");
		w2.unit[hall].order = W2_ORD_STILL;
	}
	if (rax >= 0) { click_unit(rax); frame("09_barracks"); }

	/* drag box over the army */
	if (na > 1) {
		int x0 = 9999, y0 = 9999, x1 = -1, y1 = -1;
		unit_center(army[0], &x, &y); view_center_on(x, y); ticks(1);
		for (i = 0; i < na; i++) {
			unit_screen(army[i], &x, &y);
			if (x < x0) x0 = x;
			if (y < y0) y0 = y;
			if (x > x1) x1 = x;
			if (y > y1) y1 = y;
		}
		x0 -= 20; y0 -= 20; x1 += 20; y1 += 20;
		if (x0 < view.scr_x) x0 = view.scr_x;
		if (y0 < view.scr_y) y0 = view.scr_y;
		move(x0, y0);
		ev(AMIGAGFX_EV_MOUSEDOWN, x0, y0, AMIGAGFX_BUTTON_LEFT);
		move((x0 + x1) / 2, (y0 + y1) / 2);
		move(x1, y1);
		frame("10_dragging");
		ev(AMIGAGFX_EV_MOUSEUP, x1, y1, AMIGAGFX_BUTTON_LEFT);
		frame("11_group");
		CHECK(w2.select_n >= na, "box selects the %d army units (got %d)", na, w2.select_n);
		na = w2.select_n;   /* the box may also catch the worker */
		/* ctrl+1 */
		ev(AMIGAGFX_EV_KEY, 0, 0, 0x63); key(0x01); ev(AMIGAGFX_EV_KEY, 0, 0, 0x63 | 0x80);
		/* right click on ground -> cmd_smart for all */
		{ int got = 0; memset(g_smart, 0, sizeof g_smart);
		  unit_screen(army[0], &x, &y); click(x + 96, y - 64, AMIGAGFX_BUTTON_RIGHT);
		  frame("12_smart");
		  for (i = 0; i < w2.select_n; i++) got += g_smart[w2.select[i]];
		  CHECK(got == na, "right click: smart command for %d of %d selected units", got, na); }
		/* shift-click removes one */
		ev(AMIGAGFX_EV_KEY, 0, 0, 0x60);
		unit_screen(army[1], &x, &y); click(x, y, AMIGAGFX_BUTTON_LEFT); ticks(20);
		ev(AMIGAGFX_EV_KEY, 0, 0, 0x60 | 0x80);
		CHECK(w2.select_n == na - 1, "shift-click removes a unit (%d)", w2.select_n);
	}
	if (mine < w2.unit_hi) {
		click_unit(mine);
		frame("13_goldmine");
		CHECK(w2.select_n == 1 && w2.select[0] == mine, "gold mine selected");
	}
	key(0x01);   /* recall group 1 */
	CHECK(w2.select_n == na, "group 1 recalled (%d units)", w2.select_n);
	vx = view.map_x;
	key(0x01);   /* double tap -> centre */
	printf("view after double tap %d -> %d\n", vx, view.map_x);

	for (i = 0; i < na && i < 6; i++) { int ax, ay; unit_center(army[i], &ax, &ay); printf("army[%d] slot %d type %02x at %d,%d pick %d\n", i, army[i], w2.unit[army[i]].type, ax, ay, unit_at_pixel(ax, ay)); }
	if (foot >= 0) {
		click_unit(foot);
		frame("14_footman");
		key(0x20);   /* a = attack -> targeting */
		CHECK(strstr(ui_debug_state(), "mode=target") != NULL, "'a' enters targeting");
		unit_screen(foot, &x, &y); move(x + 64, y + 40);
		frame("15_target");
		click(x + 64, y + 40, AMIGAGFX_BUTTON_LEFT);
		CHECK(!strncmp(g_cmd, "attack_move", 11), "attack at ground = attack-move ('%s')", g_cmd);
		/* double click = all footmen in view */
		ticks(30);
		unit_screen(foot, &x, &y);
		click(x, y, AMIGAGFX_BUTTON_LEFT); click(x, y, AMIGAGFX_BUTTON_LEFT);
		CHECK(w2.select_n >= 2, "double click selects same type in view (%d)", w2.select_n);
		ticks(20);
	}
	if (mage >= 0) { click_unit(mage); frame("16_caster"); }

	/* info panel click on a group icon */
	key(0x01); ticks(20);
	click(65 + 20, 169 + 20, AMIGAGFX_BUTTON_LEFT);
	CHECK(w2.select_n == 1, "clicking a group icon selects that unit");

	ui_message("Not enough gold... mine more gold.");
	ui_message("Your forces are under attack!");
	ui_message("Test message three");
	frame("17_messages");

	/* minimap click moves the view */
	vx = view.map_x;
	click(24 + 100, 26 + 100, AMIGAGFX_BUTTON_LEFT);
	printf("minimap click: view.map_x %d -> %d\n", vx, view.map_x);
	CHECK(view.map_x != vx, "minimap click moves the view");
	unit_center(worker, &x, &y); view_center_on(x, y);

	key(0x59);   /* F10 */
	move(400, 250);
	frame("18_menu");
	CHECK(ui_menu_active(), "F10 opens the game menu");
	key(0x45);

	/* the reduced pointer shapes, 4 per row, drawn big on the map area */
	{
		unsigned char *buf = amigagfx_chunky();
		game_draw(buf, amigagfx_pitch(), W, H);
		for (i = 0; i < UI_CUR_COUNT; i++) draw_pointer(i, ((190 + (i % 4) * 100) >> L), ((40 + (i / 4) * 90) >> L), 0);
		amigagfx_blit(0, 0, W, H);
		host_shot(L ? "lo_19_cursors" : "hi_19_cursors");
		for (i = 0; i < UI_CUR_COUNT; i++) {
			const unsigned char *pix; int w, h, hx, hy; unsigned char rgb[9];
			if (!ui_cursor_debug(i, w2.player[p].race, &pix, &w, &h, &hx, &hy, rgb))
				printf("cursor %2d: %2dx%2d hot %2d,%2d pens %02x%02x%02x %02x%02x%02x %02x%02x%02x\n", i, w, h, hx, hy,
				       rgb[0], rgb[1], rgb[2], rgb[3], rgb[4], rgb[5], rgb[6], rgb[7], rgb[8]);
			CHECK(!ui_cursor_debug(i, w2.player[p].race, &pix, &w, &h, &hx, &hy, rgb) && w <= 32, "cursor %d built, <= 32 wide", i);   /* ui_cursor.c PW */
		}
	}
	printf("test_ui: %d failed checks\n", g_fail);
	assets_shutdown();
	amigagfx_close();
	return g_fail;
}
