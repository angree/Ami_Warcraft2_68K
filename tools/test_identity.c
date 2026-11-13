/*
 * test_identity.c - host test of unit identity after slot reuse and of whole building footprints.
 *
 * Usage: test_identity <data dir with "War Data"> [dir with *.pud files]
 *
 * Reuses the stand-ins and helpers of tools/test_combat.c (included below with its main renamed).
 *  1. a missile whose target dies mid-flight does not hit the newcomer in the reused slot;
 *  2. a missile whose shooter dies mid-flight does not credit the newcomer in the shooter's slot;
 *  3. an attack order does not carry over to a newcomer in the target's slot;
 *  4. splash damage never hits the attacker itself (Mac SplashDamageUnit 0x00129C54);
 *  5. unit_create of a building fails when a footprint tile is held, nothing allocated;
 *  6. every map unit record (minus start locations / nobody players) appears on the real maps.
 */
#define main test_combat_main
#include "test_combat.c"
#undef main
#include "w2gfx_decode.h"

/* hooks the old test_combat.c stand-in does not provide */
void move_release(int s) { (void)s; }
void move_tick(int s) { (void)s; }
void amigagfx_pointer_colours(const unsigned char *rgb) { (void)rgb; }
void amigagfx_set_ehb_palette(const unsigned char *rgb) { (void)rgb; }
unsigned long amigagfx_millis(void) { return 0; }

static int alive_count(void)
{
	int s, n = 0;
	for (s = 0; s < w2.unit_hi; s++) if (w2.unit[s].flags & W2_UF_ALIVE) n++;
	return n;
}

static void fly_all(void)
{
	int c;
	for (c = 0; c < 400 && missile_count() > 0; c++) missile_tick_all();
}

static void test_missile_identity(void)
{
	int a, t, n, sx, sy, dx, dy, hp;
	/* control: the aimed-at target is hit */
	reset_world(7);
	a = unit_create(W2U_ARCHER, 0, LX, LY); t = unit_create(W2U_PEON, 1, LX + 4, LY);
	unit_center(a, &sx, &sy); unit_center(t, &dx, &dy);
	hp = w2.unit[t].hp;
	missile_fire(a, W2M_ARROW, sx, sy, dx, dy, t);
	fly_all();
	CHECK(w2.unit[t].hp < hp, "control: arrow hit its target (hp %d -> %d)", hp, w2.unit[t].hp);
	printf("  control: target hp %d -> %d\n", hp, w2.unit[t].hp);

	/* target dies mid-flight, an own footman takes the slot at the same tile */
	reset_world(7);
	a = unit_create(W2U_ARCHER, 0, LX, LY); t = unit_create(W2U_PEON, 1, LX + 4, LY);
	unit_center(a, &sx, &sy); unit_center(t, &dx, &dy);
	missile_fire(a, W2M_ARROW, sx, sy, dx, dy, t);
	missile_tick_all();
	unit_remove(t);
	n = unit_create(W2U_FOOTMAN, 0, LX + 4, LY);
	CHECK(n == t, "newcomer reuses the target slot (%d vs %d)", n, t);
	hp = w2.unit[n].hp;
	fly_all();
	CHECK(w2.unit[n].hp == hp, "newcomer in the target slot was hit (hp %d -> %d)", hp, w2.unit[n].hp);
	printf("  target slot %d reused mid-flight: newcomer hp %d -> %d\n", t, hp, w2.unit[n].hp);

	/* shooter dies mid-flight, a player-2 grunt takes the shooter slot: no kill credit */
	reset_world(7);
	a = unit_create(W2U_ARCHER, 0, LX, LY); t = unit_create(W2U_PEON, 1, LX + 4, LY);
	w2.unit[t].hp = 1;
	unit_center(a, &sx, &sy); unit_center(t, &dx, &dy);
	missile_fire(a, W2M_ARROW, sx, sy, dx, dy, t);
	missile_tick_all();
	unit_remove(a);
	n = unit_create(W2U_GRUNT, 2, LX, LY);
	CHECK(n == a, "newcomer reuses the shooter slot (%d vs %d)", n, a);
	fly_all();
	CHECK(w2.unit[t].hp == 0, "target still killed by the orphan arrow");
	CHECK(w2.unit[n].kills == 0 && combat_kills(2) == 0, "newcomer in the shooter slot credited (kills %d, p2 %d)",
	      w2.unit[n].kills, combat_kills(2));
	printf("  shooter slot %d reused mid-flight: newcomer kills %d, player 2 kills %d, target hp %d\n",
	       a, w2.unit[n].kills, combat_kills(2), w2.unit[t].hp);
}

static void test_order_identity(void)
{
	int f, g, n, c, hp, hits = 0;
	reset_world(9);
	f = unit_create(W2U_FOOTMAN, 0, LX, LY); g = unit_create(W2U_PEON, 1, LX + 1, LY);
	w2.player[1].control = W2_CTRL_HUMAN;
	cmd_attack(f, g);
	game_cycle();
	unit_remove(g);
	n = unit_create(W2U_PEASANT, 0, LX + 1, LY);   /* own unit in the same slot */
	CHECK(n == g, "newcomer reuses the attack target slot");
	hp = w2.unit[n].hp;
	/* a swing already under way is finished (never cut) but strikes nothing; after it, no attack */
	for (c = 0; c < 200; c++) { game_cycle(); if (c >= 60 && w2.unit[f].order == W2_ORD_ATTACK && w2.unit[f].target == n) hits++; }
	CHECK(w2.unit[n].hp == hp, "own newcomer in the attack target slot attacked (hp %d -> %d)", hp, w2.unit[n].hp);
	CHECK(hits == 0, "attack order still on the newcomer for %d cycles", hits);
	printf("  attack target slot %d reused: newcomer hp %d -> %d, cycles still attacking %d, footman order %d\n",
	       g, hp, w2.unit[n].hp, hits, w2.unit[f].order);
}

static void test_splash_self(void)
{
	int f, g, px, py, hf, hg;
	reset_world(3);
	f = unit_create(W2U_FOOTMAN, 0, LX, LY); g = unit_create(W2U_GRUNT, 1, LX + 1, LY);
	hf = w2.unit[f].hp; hg = w2.unit[g].hp;
	unit_center(f, &px, &py);
	combat_area_damage(px + 16, py, 60, f, W2_TGT_LAND | W2_TGT_SEA, 0);
	CHECK(w2.unit[f].hp == hf, "splash hit its own attacker (hp %d -> %d)", hf, w2.unit[f].hp);
	CHECK(w2.unit[g].hp < hg, "splash missed the neighbour");
	printf("  splash by slot %d: attacker hp %d -> %d, neighbour hp %d -> %d\n", f, hf, w2.unit[f].hp, hg, w2.unit[g].hp);
}

static int occ_tiles(int s)
{
	int x, y, n = 0;
	for (y = 0; y < w2.map.h; y++) for (x = 0; x < w2.map.w; x++) if (w2.map.occ_ground[y * w2.map.w + x] == s) n++;
	return n;
}

static void test_footprint(void)
{
	int f, b, hi, cnt;
	reset_world(5);
	f = unit_create(W2U_FOOTMAN, 0, LX + 2, LY + 2);
	hi = w2.unit_hi; cnt = w2.player[0].unit_count;
	b = unit_create(W2U_HUMAN_BARRACKS, 0, LX, LY);   /* 3x3 over the footman */
	CHECK(b < 0, "barracks over a footman was created (slot %d)", b);
	CHECK(w2.unit_hi == hi && w2.player[0].unit_count == cnt && w2.unit[f + 1].flags == 0, "failed create allocated something");
	CHECK(w2.map.occ_ground[(LY + 2) * w2.map.w + LX + 2] == f, "footman lost its tile");
	printf("  barracks on an occupied 3x3: slot %d (unit_hi %d -> %d)\n", b, hi, w2.unit_hi);
	b = unit_create(W2U_HUMAN_BARRACKS, 0, LX + 4, LY);   /* free spot */
	CHECK(b >= 0 && occ_tiles(b) == 9, "barracks on a free spot: slot %d, %d tiles", b, b >= 0 ? occ_tiles(b) : 0);
	printf("  barracks on a free 3x3: slot %d owns %d tiles\n", b, b >= 0 ? occ_tiles(b) : 0);
	unit_remove(f);
	b = unit_create(W2U_HUMAN_BARRACKS, 0, LX, LY);
	CHECK(b >= 0 && occ_tiles(b) == 9, "barracks after the footman left: slot %d", b);
	printf("  barracks after the footman left: slot %d owns %d tiles\n", b, b >= 0 ? occ_tiles(b) : 0);
}

/* expected = UNIT records that pud_load_mem would create (no start locations, no nobody players) */
static int map_check(const char *name, const uint8_t *d, unsigned long size)
{
	unsigned long len; const unsigned char *s; int i, n, exp = 0, got, bld = 0;
	if (pud_load_mem(d, (uint32_t)size)) return 0;
	s = w2_pud_section(d, size, "UNIT", &len);
	n = s ? (int)(len / 8) : 0;
	for (i = 0; i < n; i++) {
		int type = s[i * 8 + 4], owner = s[i * 8 + 5];
		const w2_unit_type *t = w2_unit((unsigned)type);
		if (owner >= W2_MAX_PLAYERS) owner = W2_NEUTRAL;
		if (type == W2U_HUMAN_START_LOCATION || type == W2U_ORC_START_LOCATION) continue;
		if (w2.player[owner].control == W2_CTRL_NOBODY) continue;
		if (!t || (t->xflags & W2UX_UNUSED) || t->kind == W2_KIND_NONE || t->kind == W2_KIND_MARKER) continue;
		exp++;
		if (t->kind == W2_KIND_BUILDING) bld++;
	}
	got = alive_count();
	if (got != exp) { g_fail++; printf("FAIL map %s: %d of %d units created\n", name, got, exp); }
	return got == exp ? 1 : 0;
}

static void test_maps(const char *pd)
{
	static const char *const puds[] = { "Alamo.pud", "Channel.pud", "Death.pud", "Dragon.pud", "Icebrdge.pud", "Islands.pud", "Land_Sea.pud", "Mutton.pud" };
	int e, i, maps = 0, ok = 0; long units = 0;
	for (e = 192; e <= 240; e++) {
		unsigned long n; unsigned char *p = assets_load(ASSET_DATA, e, &n); char nm[16];
		if (!p) continue;
		snprintf(nm, sizeof nm, "entry %d", e);
		if (n > 16 && !memcmp(p, "TYPE", 4)) { maps++; ok += map_check(nm, p, n); units += alive_count(); }
		war_free(p);
	}
	for (i = 0; pd && i < (int)(sizeof puds / sizeof puds[0]); i++) {
		char path[512]; FILE *f; long n; uint8_t *buf;
		snprintf(path, sizeof path, "%s/%s", pd, puds[i]);
		f = fopen(path, "rb"); if (!f) continue;
		fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
		buf = malloc((size_t)n);
		if (buf && fread(buf, 1, (size_t)n, f) == (size_t)n) { maps++; ok += map_check(puds[i], buf, (unsigned long)n); units += alive_count(); }
		free(buf); fclose(f);
	}
	printf("  maps: %d of %d with every unit record created (%ld units total)\n", ok, maps, units);
	CHECK(maps >= 20, "too few maps found (%d)", maps);
}

int main(int argc, char **argv)
{
	if (argc < 2) { printf("usage: test_identity <data dir> [pud dir] | --combat <data dir> <out dir>\n"); return 2; }
	/* the whole old combat test with these stand-ins (tools/build_test_combat.sh no longer links) */
	if (!strcmp(argv[1], "--combat")) return test_combat_main(argc - 1, argv + 1);
	w2cfg_defaults(&w2_cfg);
	w2_cfg.resolution = W2_RES_HIRES;
	snprintf(w2_cfg.data_dir, sizeof w2_cfg.data_dir, "%s", argv[1]);
	if (assets_init()) { printf("FATAL: assets_init\n"); return 2; }
	reset_world(1);
	if (!find_rect(16, 8, W2_TF_LAND, W2_TF_NOWALK | W2_TF_WATER, &LX, &LY)) { printf("FATAL: no land area\n"); return 2; }
	LX += 2; LY += 1;
	printf("missile identity:\n");  test_missile_identity();
	printf("attack order identity:\n"); test_order_identity();
	printf("splash:\n");            test_splash_self();
	printf("footprint:\n");         test_footprint();
	printf("map loads:\n");         test_maps(argc > 2 ? argv[2] : NULL);
	assets_shutdown();
	if (g_fail) printf("TEST_IDENTITY: %d FAILURES\n", g_fail);
	else printf("TEST_IDENTITY: ALL OK\n");
	return g_fail ? 1 : 0;
}
