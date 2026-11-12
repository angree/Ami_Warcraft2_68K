/* test_unit.c - host test of src/unit.c + src/unitdraw.c (unit life cycle, animation, drawing).
 *
 * Env: W2_DATA    directory with "War Data" etc. (default: the gamedata dir outside the repo)
 *      W2_PUDDIR  directory with extra *.pud maps
 *      W2_SHOTDIR where PNGs go (OUTSIDE the repository: they contain copyrighted art)
 * Writes PNG sheets (every unit type x 8 directions x player colours, animation strips, buildings
 * and construction stages, life cycle death -> corpse -> decay, fog, lores) for summer/winter/wasteland
 * maps, and checks occupancy, counts, queries, animation timing. Exit code = number of failures.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "w2.h"
#include "unit.h"
#include "pud.h"
#include "assets.h"
#include "w2cfg.h"

/* ---------------------------------------------------------------- platform / neighbour stand-ins */
void w2_log(const char *fmt, ...)
{
	va_list ap; va_start(ap, fmt); printf("  [log] "); vprintf(fmt, ap); printf("\n"); va_end(ap);
}
void amigagfx_set_palette(const unsigned char *rgb, int first, int count) { (void)rgb; (void)first; (void)count; }

static int g_fog_on, g_die_hooks, g_sounds;
int fog_visible(int tx, int ty) { return !g_fog_on || ((w2.map.visible[ty * w2.map.w + tx] >> w2.local_player) & 1); }
int fog_explored(int tx, int ty) { return !g_fog_on || ((w2.map.explored[ty * w2.map.w + tx] >> w2.local_player) & 1); }
void ord_die(int slot) { (void)slot; g_die_hooks++; }               /* combat's hook: must not free */
void snd_play(int id, int px, int py) { (void)id; (void)px; (void)py; g_sounds++; }

static int g_fail, g_checks;
#define CHECK(c, ...) do { g_checks++; if (!(c)) { g_fail++; printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static const char *g_shot = "/mnt/d/GITHUB/_AmiW2-poza-repo/scratch/unit";

/* ---------------------------------------------------------------- PNG (indexed, stored deflate) */
static unsigned long g_crctab[256];
static unsigned long crc_up(unsigned long c, const unsigned char *b, size_t n)
{
	size_t i;
	if (!g_crctab[1]) {
		unsigned long k, v; int j;
		for (k = 0; k < 256; k++) { for (v = k, j = 0; j < 8; j++) v = (v & 1) ? 0xEDB88320UL ^ (v >> 1) : v >> 1; g_crctab[k] = v; }
	}
	for (i = 0; i < n; i++) c = g_crctab[(c ^ b[i]) & 0xFF] ^ (c >> 8);
	return c;
}
static void be32(unsigned char *p, unsigned long v) { p[0] = (unsigned char)(v >> 24); p[1] = (unsigned char)(v >> 16); p[2] = (unsigned char)(v >> 8); p[3] = (unsigned char)v; }
static void chunk(FILE *f, const char *t, const unsigned char *d, size_t n)
{
	unsigned char h[8], c4[4]; unsigned long c;
	be32(h, (unsigned long)n); memcpy(h + 4, t, 4); fwrite(h, 1, 8, f);
	if (n) fwrite(d, 1, n, f);
	c = crc_up(0xFFFFFFFFUL, h + 4, 4); c = crc_up(c, d, n) ^ 0xFFFFFFFFUL; be32(c4, c); fwrite(c4, 1, 4, f);
}
static void png_indexed(const char *name, const unsigned char *px, int w, int h, int pitch)
{
	static const unsigned char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	char path[512]; FILE *f; unsigned char ihdr[13], *raw, *z, *q; size_t rn = (size_t)h * (w + 1), pos; int y;
	unsigned long a = 1, b = 0;
	snprintf(path, sizeof path, "%s/%s.png", g_shot, name);
	raw = malloc(rn); z = malloc(rn + rn / 65535 * 5 + 64);
	for (y = 0; y < h; y++) { raw[(size_t)y * (w + 1)] = 0; memcpy(raw + (size_t)y * (w + 1) + 1, px + (size_t)y * pitch, (size_t)w); }
	q = z; *q++ = 0x78; *q++ = 0x01;
	for (pos = 0; pos < rn;) {
		size_t n = rn - pos > 65535 ? 65535 : rn - pos;
		*q++ = (unsigned char)(pos + n == rn); *q++ = (unsigned char)n; *q++ = (unsigned char)(n >> 8);
		*q++ = (unsigned char)~n; *q++ = (unsigned char)(~n >> 8);
		memcpy(q, raw + pos, n); q += n; pos += n;
	}
	for (pos = 0; pos < rn; pos++) { a = (a + raw[pos]) % 65521; b = (b + a) % 65521; }
	be32(q, (b << 16) | a); q += 4;
	f = fopen(path, "wb");
	if (!f) { printf("cannot write %s\n", path); g_fail++; free(raw); free(z); return; }
	fwrite(sig, 1, 8, f);
	be32(ihdr, (unsigned long)w); be32(ihdr + 4, (unsigned long)h);
	ihdr[8] = 8; ihdr[9] = 3; ihdr[10] = ihdr[11] = ihdr[12] = 0;
	chunk(f, "IHDR", ihdr, 13);
	chunk(f, "PLTE", assets_palette(), 768);
	chunk(f, "IDAT", z, (size_t)(q - z));
	chunk(f, "IEND", NULL, 0);
	fclose(f); free(raw); free(z);
	printf("  shot %s (%dx%d)\n", path, w, h);
}

/* ---------------------------------------------------------------- canvas: terrain + units */
static unsigned char *g_buf;

/* render map pixels (mx,my,w,h) hires into a fresh buffer and save it */
static void render(const char *name, int mx, int my, int w, int h)
{
	int lo = assets_lores(), ts = assets_tile_size(), bw = w >> lo, bh = h >> lo, tx, ty;
	free(g_buf);
	g_buf = calloc((size_t)bw * bh, 1);
	view.buf = g_buf; view.pitch = bw; view.lores = lo;
	view.scr_x = 0; view.scr_y = 0; view.scr_w = w; view.scr_h = h;
	view.map_x = mx; view.map_y = my;
	for (ty = my / 32; ty * 32 < my + h && ty < w2.map.h; ty++)
		for (tx = mx / 32; tx * 32 < mx + w && tx < w2.map.w; tx++) {
			int i = ty * w2.map.w + tx, x, y, ex, vis;
			const unsigned char *m = assets_megatile(w2.map.mega[i]);
			if (!m) continue;
			ex = fog_explored(tx, ty); vis = fog_visible(tx, ty);
			if (!ex) continue;
			for (y = 0; y < ts; y++) {
				int dy = ((ty * 32 - my) >> lo) + y;
				if (dy < 0 || dy >= bh) continue;
				for (x = 0; x < ts; x++) {
					int dx = ((tx * 32 - mx) >> lo) + x;
					if (dx < 0 || dx >= bw) continue;
					g_buf[dy * bw + dx] = (!vis && ((x + y) & 1)) ? 0 : m[y * ts + x];
				}
			}
		}
	unitdraw_draw();
	png_indexed(name, g_buf, bw, bh, bw);
}

static void tick(int n) { while (n-- > 0) { unit_tick_all(); w2.cycle++; } }

static void wipe(void)
{
	int s;
	for (s = W2_MAX_UNITS - 1; s >= 0; s--) if (w2.unit[s].flags) unit_remove(s);
	unit_corpses_clear();
	w2.cycle = 0; w2.select_n = 0;
	CHECK(w2.unit_hi == 0, "unit_hi %d after wipe", w2.unit_hi);
}

/* occupancy grids and building flags must match the live units exactly */
static int check_occ(const char *where)
{
	int i, s, err = 0, n = w2.map.w * w2.map.h;
	static uint16_t want_g[W2_MAP_MAX * W2_MAP_MAX], want_a[W2_MAP_MAX * W2_MAP_MAX];
	for (i = 0; i < n; i++) want_g[i] = want_a[i] = W2_NO_UNIT;
	for (s = 0; s < w2.unit_hi; s++) {
		const W2Unit *u = &w2.unit[s]; const w2_unit_type *t = w2_unit(u->type); int x, y;
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & W2_UF_INSIDE)) continue;
		for (y = u->ty; y < u->ty + t->tile_h; y++)
			for (x = u->tx; x < u->tx + t->tile_w; x++) {
				uint16_t *g = (u->flags & W2_UF_AIR) ? want_a : want_g;
				if (g[y * w2.map.w + x] == W2_NO_UNIT) g[y * w2.map.w + x] = (uint16_t)s;
			}
	}
	for (i = 0; i < n; i++) {
		int bld = want_g[i] != W2_NO_UNIT && (w2.unit[want_g[i]].flags & W2_UF_BUILDING);
		if (w2.map.occ_ground[i] != want_g[i] || w2.map.occ_air[i] != want_a[i] ||
		    (((w2.map.flags[i] & W2_TF_BUILDING) != 0) != (bld != 0))) {
			if (err < 5) printf("  occ mismatch at %d,%d: ground %d want %d, air %d want %d, bflag %d\n", i % w2.map.w, i / w2.map.w,
			                    w2.map.occ_ground[i], want_g[i], w2.map.occ_air[i], want_a[i], (w2.map.flags[i] & W2_TF_BUILDING) != 0);
			err++;
		}
	}
	CHECK(err == 0, "%s: %d occupancy mismatches", where, err);
	return err;
}

/* ---------------------------------------------------------------- sheets */
typedef struct Item { int type, owner, dir, frame, progress, res, sel, flags; } Item;
static Item g_items[1024];
static int g_nitems;
static void add(int type, int owner, int dir, int frame, int progress, int res, int sel, int flags)
{
	Item *it = &g_items[g_nitems++];
	it->type = type; it->owner = owner; it->dir = dir; it->frame = frame; it->progress = progress;
	it->res = res; it->sel = sel; it->flags = flags;
}

/* place items on a grid of cell x cell tiles, cols per row; one PNG per page that fits the map */
static void sheet(const char *name, int cell, int cols)
{
	int rows_fit = (w2.map.h - 1) / cell, i = 0, page = 0;
	if (cols * cell > w2.map.w) cols = w2.map.w / cell;
	while (i < g_nitems) {
		int k, r = 0, c = 0; char nm[128];
		wipe();
		for (k = i; k < g_nitems; k++) {
			Item *it = &g_items[k]; int s; W2Unit *u;
			if (r >= rows_fit) break;
			s = unit_create(it->type, it->owner, c * cell + 1, r * cell + 1);
			CHECK(s >= 0, "create type %02x", it->type);
			if (s >= 0) {
				u = &w2.unit[s];
				if (!(u->flags & W2_UF_BUILDING)) u->dir = (uint8_t)it->dir;
				if (it->frame >= 0) u->frame = (uint16_t)it->frame;
				u->progress = (uint16_t)it->progress; u->res_kind = (uint8_t)it->res;
				u->flags |= (uint16_t)it->flags;
				if (it->sel) { u->flags |= W2_UF_SELECTED; if (w2.select_n < W2_MAX_SELECT) w2.select[w2.select_n++] = (uint16_t)s; }
			}
			if (++c == cols) { c = 0; r++; }
		}
		check_occ(name);
		snprintf(nm, sizeof nm, "%s_%d", name, page++);
		render(nm, 0, 0, cols * cell * 32 + 32, (r + (c ? 1 : 0)) * cell * 32 + 32);
		i = k;
	}
	g_nitems = 0;
}

static int is_mobile(int t)
{
	const w2_unit_type *u = w2_unit(t);
	return u && !(u->xflags & W2UX_UNUSED) && (u->kind == W2_KIND_LAND || u->kind == W2_KIND_AIR || u->kind == W2_KIND_SEA) && u->gfx[0];
}

static int anim_frame(int type, int kind, int want_flag, int last)
{
	unsigned n, i; const w2_anim_step *st = w2_unit_anim(type, kind, &n);
	if (!st || !n) return -1;
	if (last) return st[n - 1].frame;
	for (i = 0; i < n; i++) if (st[i].flags & want_flag) return st[i].frame;
	return st[n > 3 ? 3 : 0].frame;
}

/* every mobile unit of a race: 8 directions (owner = direction -> all 8 player colours),
 * then walk, attack, death-end frames facing SW (mirrored), and carrying gold/wood/oil */
static void unit_sheets(const char *era)
{
	int race, t; char nm[64];
	for (race = 0; race < 3; race++) {
		for (t = 0; t < (int)W2_NUM_UNIT_TYPES; t++) {
			int d;
			if (!is_mobile(t) || w2_unit(t)->race != race) continue;
			for (d = 0; d < 8; d++) add(t, d, d, -1, 0, 0, 0, 0);
			add(t, 0, 5, anim_frame(t, W2_ANIM_MOVE, 0, 0), 0, 0, 0, 0);
			add(t, 1, 5, anim_frame(t, W2_ANIM_ATTACK, W2AF_ATTACK, 0), 0, 0, 0, 0);
			add(t, 2, 5, anim_frame(t, W2_ANIM_DEATH, 0, 1), 0, 0, 0, 0);
			if (w2_unit_harvest(t, 1)) add(t, 3, 3, -1, 0, 1, 0, 0); else add(t, 3, 3, -1, 0, w2_unit_harvest(t, 3) ? 3 : 0, 0, 0);
			if (w2_unit_harvest(t, 2)) add(t, 4, 6, anim_frame(t, W2_ANIM_MOVE, 0, 0), 0, 2, 0, 0); else add(t, 4, 6, -1, 0, 0, 1, 0);
		}
		snprintf(nm, sizeof nm, "%s_units_%s", era, race == 0 ? "human" : race == 1 ? "orc" : "neutral");
		sheet(nm, 3, 13);
	}
}

static void building_sheets(const char *era)
{
	int t, i = 0; char nm[64];
	for (t = 0; t < (int)W2_NUM_UNIT_TYPES; t++) {
		const w2_unit_type *u = w2_unit(t);
		if (u->kind != W2_KIND_BUILDING || !u->gfx[0]) continue;
		add(t, i % 8, 0, -1, 0, 0, (i % 5) == 0, 0); i++;
	}
	add(W2U_GOLD_MINE, W2_NEUTRAL, 0, 1, 0, 0, 1, 0);           /* mine with a miner inside (frame 1) */
	add(W2U_HUMAN_OIL_PLATFORM, 0, 0, 2, 0, 0, 0, 0);
	snprintf(nm, sizeof nm, "%s_buildings", era);
	sheet(nm, 5, 8);
	/* construction stages 0 %, 30 %, 60 % for every construction kind */
	for (t = 0; t < (int)W2_NUM_UNIT_TYPES; t++) {
		const w2_unit_type *u = w2_unit(t); int p;
		if (u->kind != W2_KIND_BUILDING || u->construction == W2_NONE || u->race == W2_RACE_NEUTRAL) continue;
		if (!(t == W2U_FARM || t == W2U_PIG_FARM || t == W2U_TOWN_HALL || t == W2U_GREAT_HALL || t == W2U_HUMAN_SHIPYARD ||
		      t == W2U_ORC_SHIPYARD || t == W2U_HUMAN_OIL_PLATFORM || t == W2U_ORC_OIL_PLATFORM || t == W2U_HUMAN_REFINERY ||
		      t == W2U_ORC_REFINERY || t == W2U_HUMAN_FOUNDRY || t == W2U_ORC_FOUNDRY || t == W2U_HUMAN_WALL ||
		      t == W2U_HUMAN_GUARD_TOWER || t == W2U_ORC_BARRACKS)) continue;
		for (p = 0; p < 3; p++)
			add(t, u->race, 0, -1, (int)(w2_time_to_cycles(u->build_time) * (unsigned)(p * 30) / 100u), 0, 0, W2_UF_UNDERCONST);
		add(t, u->race, 0, -1, 0, 0, 0, 0);
	}
	snprintf(nm, sizeof nm, "%s_construction", era);
	sheet(nm, 5, 8);
}

/* play MOVE/ATTACK/DEATH through the real engine, record every frame change, draw the strips */
static const int g_anim_types[] = { W2U_FOOTMAN, W2U_GRUNT, W2U_PEASANT, W2U_KNIGHT, W2U_ARCHER, W2U_AXETHROWER,
	W2U_MAGE, W2U_DEATH_KNIGHT, W2U_DWARVES, W2U_CATAPULT, W2U_GRYPHON_RIDER, W2U_DRAGON, W2U_BALLOON,
	W2U_HUMAN_DESTROYER, W2U_OGRE_JUGGERNAUGHT, W2U_ORC_SUBMARINE, W2U_SKELETON, W2U_DAEMON, W2U_CRITTER, W2U_EYE_OF_VISION };

static void anim_strips(const char *era)
{
	int ti, k; char nm[64];
	for (ti = 0; ti < (int)(sizeof g_anim_types / sizeof g_anim_types[0]); ti++) {
		int type = g_anim_types[ti];
		for (k = 0; k < 3; k++) {
			static const int kinds[3] = { W2_ANIM_MOVE, W2_ANIM_ATTACK, W2_ANIM_DEATH };
			int s, cyc, px = 0, nrec = 0, last = -1, attacks = 0, ok = 0; int rec[12];
			unsigned n;
			if (!w2_unit_anim(type, kinds[k], &n) || !n) continue;
			wipe();
			s = unit_create(type, ti & 7, 5, 5);
			if (s < 0) continue;
			w2.unit[s].dir = 5;
			w2.unit[s].order = W2_ORD_CAST;   /* no handler in the contract: the animation is not reverted to STILL */
			unit_set_anim(s, kinds[k]);
			for (cyc = 1; cyc <= 1000; cyc++) {
				tick(1);
				if (!w2.unit[s].flags) break;
				if (unit_anim_done(s)) { ok = 1; cyc--; break; }   /* wrapped: this cycle starts the next loop */
				px += unit_anim_move_pixels(s);
				if (unit_anim_event(s) & W2AF_ATTACK) attacks++;
				if (w2.unit[s].frame != last && nrec < 12) { rec[nrec++] = w2.unit[s].frame; last = w2.unit[s].frame; }
			}
			CHECK(ok, "%s anim %d did not finish", w2_unit(type)->ident, kinds[k]);
			if (kinds[k] == W2_ANIM_MOVE) CHECK(px == 32, "%s MOVE moved %d px (want 32)", w2_unit(type)->ident, px);
			if (kinds[k] == W2_ANIM_ATTACK)
				CHECK(attacks == 1 || !(w2_unit(type)->basic_damage + w2_unit(type)->piercing_damage), "%s ATTACK fired %d times", w2_unit(type)->ident, attacks);
			if (!strcmp(era, "summer"))
				printf("  anim %-18s %-6s %2u steps %3d cycles %2d px %d attack\n", w2_unit(type)->ident,
				       k == 0 ? "MOVE" : k == 1 ? "ATTACK" : "DEATH", n, cyc, px, attacks);
			for (cyc = 0; cyc < nrec; cyc++) add(type, ti & 7, 5, rec[cyc], 0, 0, 0, 0);
			for (; cyc < 12; cyc++) add(W2U_CRITTER, W2_NEUTRAL, 0, -1, 0, 0, 0, W2_UF_HIDDEN);   /* pad the row */
		}
	}
	snprintf(nm, sizeof nm, "%s_anim", era);
	sheet(nm, 3, 12);
}

/* ---------------------------------------------------------------- life cycle on real terrain */
static int find_land_area(int w, int h, int *ox, int *oy)
{
	int x, y, i, j;
	for (y = 2; y + h < w2.map.h; y++)
		for (x = 2; x + w < w2.map.w; x++) {
			int ok = 1;
			for (j = 0; j < h && ok; j++) for (i = 0; i < w && ok; i++)
				if (w2.map.flags[(y + j) * w2.map.w + x + i] & (W2_TF_NOWALK | W2_TF_WATER)) ok = 0;
			if (ok) { *ox = x; *oy = y; return 1; }
		}
	return 0;
}

static void life_cycle(const char *era)
{
	int ox, oy, foot, grunt, crit, knight, gry, farm, barr, hall, mine, mage, skel, ship, i, alive, n, cx, cy, s;
	char nm[64]; uint16_t list[64];
	wipe();
	if (!find_land_area(15, 7, &ox, &oy)) { printf("  no land area\n"); ox = oy = 2; }
	w2.local_player = 0;
	w2.player[0].enemies = 0x00FE; w2.player[1].enemies = 0x00FD;
	hall = unit_create(W2U_TOWN_HALL, 0, ox, oy);
	farm = unit_create(W2U_PIG_FARM, 1, ox + 5, oy);
	barr = unit_create(W2U_HUMAN_BARRACKS, 0, ox + 8, oy);
	mine = unit_create(W2U_GOLD_MINE, 3, ox + 12, oy);
	foot = unit_create(W2U_FOOTMAN, 0, ox + 1, oy + 5);
	grunt = unit_create(W2U_GRUNT, 1, ox + 2, oy + 5);
	crit = unit_create(W2U_CRITTER, W2_NEUTRAL, ox + 4, oy + 5);
	knight = unit_create(W2U_KNIGHT, 0, ox + 6, oy + 5);
	gry = unit_create(W2U_GRYPHON_RIDER, 0, ox + 6, oy + 5);   /* flies over the knight */
	mage = unit_create(W2U_MAGE, 0, ox + 8, oy + 5);
	skel = unit_create(W2U_SKELETON, 1, ox + 10, oy + 5);
	ship = unit_create(W2U_ORC_DESTROYER, 1, ox + 12, oy + 5);
	CHECK(hall >= 0 && farm >= 0 && barr >= 0 && mine >= 0 && foot >= 0 && grunt >= 0 && crit >= 0 && knight >= 0 &&
	      gry >= 0 && mage >= 0 && skel >= 0 && ship >= 0, "life: create failed");
	CHECK(w2.unit[mine].owner == W2_NEUTRAL, "gold mine owner %d", w2.unit[mine].owner);
	CHECK(w2.unit[mage].mana == W2_MANA_START, "mage mana %d", w2.unit[mage].mana);
	CHECK(w2.unit[gry].flags & W2_UF_AIR, "gryphon not AIR");
	CHECK(w2.unit[ship].flags & W2_UF_SEA, "ship not SEA");
	CHECK(w2.map.occ_air[(oy + 5) * w2.map.w + ox + 6] == gry && w2.map.occ_ground[(oy + 5) * w2.map.w + ox + 6] == knight, "air/ground occ");
	CHECK(w2.player[0].building_count == 2 && w2.player[0].unit_count == 4, "p0 counts b%d u%d", w2.player[0].building_count, w2.player[0].unit_count);
	check_occ("life create");
	/* queries */
	unit_center(foot, &cx, &cy);
	CHECK(unit_at_pixel(cx, cy) == foot, "unit_at_pixel footman -> %d", unit_at_pixel(cx, cy));
	unit_center(knight, &cx, &cy);
	CHECK(unit_at_pixel(cx, cy) == gry, "unit_at_pixel over knight -> %d (want flyer %d)", unit_at_pixel(cx, cy), gry);
	unit_center(hall, &cx, &cy);
	CHECK(unit_at_pixel(cx, cy) == hall, "unit_at_pixel hall");
	CHECK(unit_at_pixel(ox * 32 + 3, (oy + 9) * 32) == -1, "unit_at_pixel empty");
	n = units_in_rect((ox + 1) * 32, (oy + 5) * 32, (ox + 7) * 32 + 31, (oy + 5) * 32 + 31, list, 64);
	CHECK(n == 5, "units_in_rect -> %d (want foot, grunt, critter, knight, gryphon)", n);
	CHECK(unit_dist_tiles(hall, foot) == 2, "dist hall-foot %d", unit_dist_tiles(hall, foot));
	CHECK(unit_dist_tiles(foot, knight) == 5, "dist foot-knight %d", unit_dist_tiles(foot, knight));
	CHECK(unit_dist_tiles(hall, barr) == 5, "dist hall-barracks %d", unit_dist_tiles(hall, barr));
	CHECK(unit_dist_to_tile(hall, ox + 3, oy + 3) == 0 && unit_dist_to_tile(hall, ox + 6, oy) == 3, "dist to tile");
	CHECK(unit_is_enemy(foot, grunt) && !unit_is_enemy(foot, knight) && !unit_is_enemy(foot, crit) && !unit_is_enemy(crit, foot), "is_enemy");
	unit_center(foot, &cx, &cy);
	{
		static const int vx[8] = { 0, 10, 10, 10, 0, -10, -10, -10 }, vy[8] = { -10, -10, 0, 10, 10, 10, 0, -10 };
		for (i = 0; i < 8; i++) { unit_set_dir_to(foot, cx + vx[i] * 7, cy + vy[i] * 7); CHECK(w2.unit[foot].dir == i, "dir_to %d -> %d", i, w2.unit[foot].dir); }
		unit_set_dir_to(foot, cx + 100, cy + 30); CHECK(w2.unit[foot].dir == 2, "dir_to shallow -> %d", w2.unit[foot].dir);
		unit_set_dir_to(foot, cx - 30, cy + 100); CHECK(w2.unit[foot].dir == 4, "dir_to steep -> %d", w2.unit[foot].dir);
	}
	w2.unit[foot].dir = 3; w2.unit[grunt].dir = 6; w2.unit[knight].dir = 4;
	/* selection colours: own green, enemy red, neutral yellow */
	w2.unit[foot].flags |= W2_UF_SELECTED; w2.unit[grunt].flags |= W2_UF_SELECTED; w2.unit[crit].flags |= W2_UF_SELECTED;
	w2.unit[barr].flags |= W2_UF_SELECTED; w2.unit[mine].flags |= W2_UF_SELECTED; w2.unit[gry].flags |= W2_UF_SELECTED;
	w2.select[0] = (uint16_t)foot; w2.select[1] = (uint16_t)barr; w2.select_n = 2;
	snprintf(nm, sizeof nm, "%s_life0_select", era); render(nm, (ox - 1) * 32, (oy - 1) * 32, 16 * 32, 9 * 32);

	/* mana and ttl */
	w2.unit[skel].ttl = 10;
	tick(11);
	CHECK(w2.unit[skel].flags & W2_UF_DYING || !w2.unit[skel].flags, "skeleton ttl did not kill it");
	tick(289);
	CHECK(w2.unit[mage].mana == W2_MANA_START + 10, "mage mana after 300 cycles %d", w2.unit[mage].mana);

	/* deaths */
	g_die_hooks = 0;
	unit_kill(foot); unit_kill(grunt); unit_kill(knight); unit_kill(ship); unit_kill(gry);
	w2.unit[farm].hp = 0;                  /* hp 0 -> automatic death */
	unit_kill(barr);
	unit_kill(mine);                       /* indestructible? the mine has hp; killing by order still works */
	tick(1);
	CHECK(g_die_hooks == 8, "ord_die hook called %d times (want 8)", g_die_hooks);
	CHECK((w2.unit[foot].flags & W2_UF_DYING) && !(w2.unit[foot].flags & W2_UF_ALIVE), "footman not dying");
	CHECK(w2.select_n == 0, "dying units still selected (%d)", w2.select_n);
	CHECK(w2.unit[barr].flags == 0 && w2.unit[farm].flags == 0, "buildings not freed at once");
	CHECK(w2.player[0].building_count == 1 && w2.player[0].unit_count == 1, "p0 counts after death b%d u%d", w2.player[0].building_count, w2.player[0].unit_count);
	check_occ("life dying");
	for (n = 0, i = 0; i < W2_MAX_CORPSES; i++) n += w2_corpses[i].used;
	CHECK(n == 3, "rubble %d (want 3: farm, barracks, mine)", n);
	tick(5);
	snprintf(nm, sizeof nm, "%s_life1_dying", era); render(nm, (ox - 1) * 32, (oy - 1) * 32, 16 * 32, 9 * 32);
	tick(110);
	for (n = 0, i = 0; i < W2_MAX_CORPSES; i++) n += w2_corpses[i].used;
	CHECK(n == 6 && (w2.unit[knight].flags & W2_UF_DYING), "corpses %d (want 6: footman, grunt, ship + 3 rubble; knight still dying)", n);
	snprintf(nm, sizeof nm, "%s_life2_bodies", era); render(nm, (ox - 1) * 32, (oy - 1) * 32, 16 * 32, 9 * 32);
	for (i = 0; i < 1000; i++) {
		tick(1);
		for (alive = 0, s = 0; s < w2.unit_hi; s++) if (w2.unit[s].flags & W2_UF_DYING) alive++;
		if (!alive) break;
	}
	printf("  deaths finished after %d cycles\n", i + 6);
	CHECK(!alive, "dying units left");
	for (n = 0, i = 0; i < W2_MAX_CORPSES; i++) n += w2_corpses[i].used;
	CHECK(n == 3, "corpses %d (want 3 bodies: footman, grunt, knight; sea body and rubble decayed)", n);
	CHECK(unit_corpse_at(ox + 1, oy + 5) >= 0, "no body under the footman");
	snprintf(nm, sizeof nm, "%s_life2b_corpses", era); render(nm, (ox - 1) * 32, (oy - 1) * 32, 16 * 32, 9 * 32);
	tick(450);
	snprintf(nm, sizeof nm, "%s_life3_decay", era); render(nm, (ox - 1) * 32, (oy - 1) * 32, 16 * 32, 9 * 32);
	tick(700);
	for (n = 0, i = 0; i < W2_MAX_CORPSES; i++) n += w2_corpses[i].used;
	CHECK(n == 0, "corpses left after decay: %d", n);
	check_occ("life end");
	CHECK(w2.unit[hall].flags & W2_UF_ALIVE && w2.unit[crit].flags & W2_UF_ALIVE, "survivors");

	/* fog: right half not visible (explored), units there hidden, buildings stay */
	foot = unit_create(W2U_FOOTMAN, 1, ox + 1, oy + 5);
	grunt = unit_create(W2U_GRUNT, 1, ox + 12, oy + 5);
	farm = unit_create(W2U_FARM, 1, ox + 12, oy + 7);
	for (i = 0; i < w2.map.w * w2.map.h; i++) { w2.map.explored[i] = 1; w2.map.visible[i] = (uint8_t)((i % w2.map.w) < ox + 8); }
	g_fog_on = 1;
	CHECK(unit_visible_local(foot) && !unit_visible_local(grunt) && unit_visible_local(farm), "fog visibility");
	CHECK(unit_owner_sees(0, foot) && !unit_owner_sees(0, grunt) && unit_owner_sees(1, grunt), "owner_sees");
	snprintf(nm, sizeof nm, "%s_life4_fog", era); render(nm, (ox - 1) * 32, (oy - 1) * 32, 16 * 32, 9 * 32);
	g_fog_on = 0;
	memset(w2.map.visible, 0, sizeof w2.map.visible);
}

/* ---------------------------------------------------------------- maps */
static int load_map(int entry, const char *file)
{
	int r = file ? pud_load_file(file) : pud_load_builtin(entry);
	if (r == 0) w2.local_player = 0;
	return r;
}

int main(void)
{
	const char *data = getenv("W2_DATA"), *pd = getenv("W2_PUDDIR"), *sd = getenv("W2_SHOTDIR");
	static const char *const era_name[3] = { "summer", "winter", "wasteland" };
	static const char *const puds[] = { "Alamo.pud", "Channel.pud", "Death.pud", "Dragon.pud", "Icebrdge.pud", "Islands.pud", "Land_Sea.pud", "Mutton.pud" };
	char path[512]; int e, i, s, found[3] = { -1, -1, -1 }; const char *found_file[3] = { 0, 0, 0 };
	if (!data) data = "/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/data";
	if (!pd) pd = "/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata";
	if (sd) g_shot = sd;
	w2cfg_defaults(&w2_cfg);
	snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", data);
	if (assets_init()) { printf("assets_init failed (%s)\n", data); return 100; }

	/* 1. built-in map 220 as loaded by pud.c -> unit_create */
	CHECK(load_map(220, NULL) == 0, "load 220");
	{
		int mines = 0, bld = 0, units = 0, x0 = 0, y0 = 0;
		for (s = 0; s < w2.unit_hi; s++) {
			const W2Unit *u = &w2.unit[s];
			if (!(u->flags & W2_UF_ALIVE)) continue;
			if (u->type == W2U_GOLD_MINE) { mines++; CHECK(u->owner == W2_NEUTRAL, "mine owner"); if (!x0) { x0 = u->tx; y0 = u->ty; } }
			if (u->flags & W2_UF_BUILDING) bld++; else units++;
		}
		printf("map 220: %dx%d era %d, %d units, %d buildings (%d gold mines), p0 units %d\n", w2.map.w, w2.map.h, w2.map.era,
		       units, bld, mines, w2.player[0].unit_count);
		check_occ("map 220");
		for (s = 0; s < w2.unit_hi; s++) if ((w2.unit[s].flags & W2_UF_ALIVE) && w2.unit[s].owner == 0) { x0 = w2.unit[s].tx; y0 = w2.unit[s].ty; break; }
		tick(90);
		x0 = x0 * 32 - 320; y0 = y0 * 32 - 240; if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
		render("map220_start", x0, y0, 960, 640);
	}

	/* 2. find one map per era */
	for (i = 220; i <= 240; i++) {
		if (load_map(i, NULL)) continue;
		e = w2.map.era; if (e > 2) e = 2;
		if (found[e] < 0 && w2.map.w >= 64 && w2.map.h >= 64) found[e] = i;
	}
	for (i = 0; i < (int)(sizeof puds / sizeof puds[0]); i++) {
		snprintf(path, sizeof path, "%s/%s", pd, puds[i]);
		if (load_map(0, path)) continue;
		e = w2.map.era; if (e > 2) e = 2;
		printf("  %s: %dx%d era %d, %d units\n", puds[i], w2.map.w, w2.map.h, w2.map.era, w2.unit_hi);
		check_occ(puds[i]);
		if (found[e] < 0 && w2.map.w >= 64 && w2.map.h >= 64) { found[e] = 0; found_file[e] = puds[i]; }
	}
	/* 3. sheets per era */
	for (e = 0; e < 3; e++) {
		if (found[e] < 0) { printf("no %s map found\n", era_name[e]); g_fail++; continue; }
		if (found_file[e]) { snprintf(path, sizeof path, "%s/%s", pd, found_file[e]); load_map(0, path); }
		else load_map(found[e], NULL);
		printf("era %s: map %s %dx%d\n", era_name[e], found_file[e] ? found_file[e] : "builtin", w2.map.w, w2.map.h);
		if (found_file[e]) printf("  (%s)\n", found_file[e]); else printf("  (War Data entry %d)\n", found[e]);
		unit_sheets(era_name[e]);
		building_sheets(era_name[e]);
		anim_strips(era_name[e]);
		if (found_file[e]) load_map(0, path); else load_map(found[e], NULL);   /* fresh terrain */
		life_cycle(era_name[e]);
	}
	/* 4. lores: switch resolution and load a map of a different era so the tiles are rebuilt halved */
	w2_cfg.resolution = W2_RES_LORES;
	for (e = 0; e < 3; e++) if (found[e] >= 0 && e != assets_era()) break;
	if (e < 3) {
		if (found_file[e]) { snprintf(path, sizeof path, "%s/%s", pd, found_file[e]); load_map(0, path); } else load_map(found[e], NULL);
		life_cycle("lores");
	}
	w2_cfg.resolution = W2_RES_HIRES;

	printf("sounds played by animation steps: %d\n", g_sounds);
	printf("test_unit: %d checks, %d failures\n", g_checks, g_fail);
	free(g_buf);
	assets_shutdown();
	return g_fail;
}
