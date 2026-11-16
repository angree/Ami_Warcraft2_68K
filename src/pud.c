/* pud.c - load a PUD map into the world (map, players, units). Format: docs/DATA_FORMAT.md section 12. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w2.h"
#include "pud.h"
#include "assets.h"
#include "w2cfg.h"
#include "unit.h"
#include "economy.h"

/* classify one tile from its MTXM value and the era's tile map group flags */
static unsigned tile_flags(unsigned v)
{
	/* tile-map group flags == the PUD SQM movement map (verified identical, low byte, on all 60 Mac
	 * War Data PUDs and the 8 CD PUDs). The Mac reads SQM straight into its map-flag words and tests:
	 * land move 0x09CE, ship move 0x0903, docking ship 0x0901, building 0x09DE, shore building
	 * 0x0911 (+ needs one 0x02 tile), oil platform 0x0913 (0x100 ground unit, 0x200 air, 0x800 bldg).
	 * So: 0x01 land (blocks ships), 0x02 shallow water/coast boundary (blocks land units AND ships,
	 * only a docking ship enters it; never built on except by shore buildings), 0x10 coast (walkable,
	 * not buildable), 0x40 water, 0x80 obstacle, 0x08 wall (0x89 orc / 0x8D human).
	 * Forest AND rock groups both carry 0x81, and parts of the water/coast boundary (0x02xx) carry
	 * 0x82, so tree/rock is decided by the tile number:
	 * solid 0x007x forest, 0x008x rock; boundary 0x07xx forest/ground, 0x04xx rock/coast. */
	unsigned gf = assets_tile_group_flags(v), f = 0;
	unsigned hi = v >> 8, sg = (v >> 4) & 0x0F;
	if (gf & 0x40) f |= W2_TF_WATER | W2_TF_NOWALK | W2_TF_NOBUILD;
	else f |= W2_TF_LAND;
	if (gf & 0x10) f |= W2_TF_COAST | W2_TF_NOBUILD;
	if (gf & 0x02) f |= W2_TF_NOWALK | W2_TF_NOBUILD;    /* shallow: no land unit, no ship (not WATER) */
	if (gf & 0x80) {
		f |= W2_TF_NOWALK | W2_TF_NOBUILD;
		if (gf & 0x08) f |= W2_TF_WALL;
		else if ((hi == 0 && sg == 7) || hi == 7) f |= W2_TF_TREE;
		else if ((hi == 0 && sg == 8) || hi == 4) f |= W2_TF_ROCK;
		/* else: impassable shoreline, neither harvestable nor destructible */
	}
	return f;
}

void pud_reclassify_tile(int tx, int ty)
{
	int i = ty * w2.map.w + tx;
	int m = assets_tile_to_mega(w2.map.tile[i]);
	w2.map.mega[i] = (uint16_t)(m < 0 ? 0 : m);
	w2.map.flags[i] = (uint16_t)((w2.map.flags[i] & W2_TF_BUILDING) | tile_flags(w2.map.tile[i]));
	if (!(w2.map.flags[i] & W2_TF_TREE)) w2.map.wood[i] = 0;
	w2.map.version++;
}

static int ctrl_from_ownr(int o)
{
	switch (o) {
	case 2: return W2_CTRL_NEUTRAL;
	case 3: return W2_CTRL_NOBODY;
	case 4: case 1: return W2_CTRL_COMPUTER;
	case 5: return W2_CTRL_HUMAN;
	case 6: return W2_CTRL_RESCUE_PASSIVE;
	case 7: return W2_CTRL_RESCUE_ACTIVE;
	default: return W2_CTRL_COMPUTER;
	}
}

/* a PUD section by tag: data pointer + length, NULL when absent or malformed. Overflow-safe on a
 * 32-bit unsigned long: a section must fit in the bytes left after its 8-byte header (the Mac's
 * reader f8_3994 / FUN_001443fc rejects a length larger than what remains in the same way). */
static const unsigned char *sec(const uint8_t *d, uint32_t size, const char *tag, unsigned long *len)
{
	uint32_t p = 0, n;
	while (size - p >= 8) {                        /* p <= size always holds here */
		n = W2_LE32(d + p + 4);
		if (n > size - p - 8) return NULL;         /* truncated / corrupt: stop */
		if (!memcmp(d + p, tag, 4)) { *len = n; return d + p + 8; }
		p += 8 + n;
	}
	return NULL;
}

/* PUD UNIT parameter of non-resource units on the Mac (f8_3EDE 0x00143EDE): a non-zero byte 6
 * marks the unit "passive" (unit+0x5F bit 1): taken out of the AI force counters (f6_1358 with -1),
 * skipped when the AI picks attackers, and a combat unit guards its own position (f8_1224 mode 1).
 * Kept per slot with the slot generation, so a unit that reuses the slot is not passive. */
static uint8_t  g_passive[W2_MAX_UNITS];
static uint16_t g_passive_gen[W2_MAX_UNITS];

int pud_unit_passive(int slot)
{
	if (slot < 0 || slot >= W2_MAX_UNITS || !g_passive[slot]) return 0;
	return unit_same(slot, g_passive_gen[slot]);
}

int pud_load_mem(const uint8_t *d, uint32_t size)
{
	unsigned long len, mlen; const unsigned char *s, *mt; int i, x, y, n, w, h, era;

	/* validate everything that can fail BEFORE the world is touched: a bad PUD leaves the old world */
	if (!d) return -1;
	s = sec(d, size, "DIM ", &len);
	if (!s || len < 4) { w2_log("pud: no DIM"); return -1; }
	w = W2_LE16(s); h = W2_LE16(s + 2);
	if (w < 1 || h < 1 || w > W2_MAP_MAX || h > W2_MAP_MAX) { w2_log("pud: bad size %dx%d", w, h); return -1; }
	mt = sec(d, size, "MTXM", &mlen);
	if (!mt || mlen < (unsigned long)w * h * 2) { w2_log("pud: no MTXM"); return -1; }
	s = sec(d, size, "ERAX", &len);
	if (!s || len < 2) s = sec(d, size, "ERA ", &len);
	era = s && len >= 2 ? W2_LE16(s) : 0;           /* Mac: ERA must be 2 bytes (f8_3D50) */
	if (assets_set_era(era)) return -1;

	memset(&w2.map, 0, sizeof w2.map);
	memset(w2.player, 0, sizeof w2.player);
	memset(w2.unit, 0, sizeof w2.unit);
	memset(g_passive, 0, sizeof g_passive);
	w2.unit_hi = 0; w2.cycle = 0; w2.select_n = 0; w2.game_over = 0;
	w2.map.w = w; w2.map.h = h;
	w2.map.era = assets_era();

	s = sec(d, size, "OWNR", &len);
	for (i = 0; i < W2_MAX_PLAYERS; i++) {
		W2Player *p = &w2.player[i];
		p->control = (uint8_t)(s && (unsigned long)i < len ? ctrl_from_ownr(s[i]) : (i == W2_NEUTRAL ? W2_CTRL_NEUTRAL : W2_CTRL_NOBODY));
		p->colour = (uint8_t)(i & 7);
		p->start_tx = p->start_ty = 0xFFFF;
		p->allowed_units = 0xFFFFFFFFu; p->allowed_spells = 0xFFFFFFFFu;
	}
	s = sec(d, size, "SIDE", &len);
	for (i = 0; s && i < W2_MAX_PLAYERS && (unsigned long)i < len; i++) w2.player[i].race = s[i] > 1 ? 0 : s[i];
	/* ALOW (Mac f8_3B04: exactly 0x180 bytes = 6 x 16 u32: units, spells at start, spells allowed,
	 * spells being researched, upgrades allowed, upgrades being researched). Absent = all allowed. */
	econ_set_allowed_upgrades(-1, 0xFFFFFFFFu);
	s = sec(d, size, "ALOW", &len);
	if (s && len == 0x180) {
		for (i = 0; i < W2_MAX_PLAYERS; i++) {
			w2.player[i].allowed_units = (uint32_t)W2_LE32(s + i * 4);
			w2.player[i].allowed_spells = (uint32_t)W2_LE32(s + 0x80 + i * 4);
			econ_set_allowed_upgrades(i, (uint32_t)W2_LE32(s + 0x100 + i * 4));
		}
	}
	s = sec(d, size, "SGLD", &len);
	for (i = 0; s && i < W2_MAX_PLAYERS && (unsigned long)(i * 2 + 2) <= len; i++) w2.player[i].gold = (int32_t)W2_LE16(s + i * 2);
	s = sec(d, size, "SLBR", &len);
	for (i = 0; s && i < W2_MAX_PLAYERS && (unsigned long)(i * 2 + 2) <= len; i++) w2.player[i].wood = (int32_t)W2_LE16(s + i * 2);
	s = sec(d, size, "SOIL", &len);
	for (i = 0; s && i < W2_MAX_PLAYERS && (unsigned long)(i * 2 + 2) <= len; i++) w2.player[i].oil = (int32_t)W2_LE16(s + i * 2);
	s = sec(d, size, "AIPL", &len);
	for (i = 0; s && i < W2_MAX_PLAYERS && (unsigned long)i < len; i++) w2.player[i].ai_type = s[i];
	for (i = 0; i < 8; i++) w2.player[i].enemies = (uint16_t)(0x00FF & ~(1u << i));   /* free-for-all */

	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			i = y * w + x;
			w2.map.tile[i] = (uint16_t)W2_LE16(mt + i * 2);
			w2.map.occ_ground[i] = W2_NO_UNIT;
			w2.map.occ_air[i] = W2_NO_UNIT;
			w2.map.wood[i] = 1;
			pud_reclassify_tile(x, y);
		}

	/* units: 8-byte records x16 y16 type owner param pad (Mac f8_3EDE reads byte 6 only, byte 7 is
	 * padding). Gold mine / oil patch / oil platform: param * 2500, param 0 = 120 (300000). */
	s = sec(d, size, "UNIT", &len);
	n = s ? (int)(len / 8) : 0;
	for (i = 0; i < n; i++) {
		const unsigned char *u = s + i * 8;
		int ux = W2_LE16(u), uy = W2_LE16(u + 2), type = u[4], owner = u[5], param = u[6], slot;
		if (owner >= W2_MAX_PLAYERS) owner = W2_NEUTRAL;
		if (type == W2U_HUMAN_START_LOCATION || type == W2U_ORC_START_LOCATION) {
			w2.player[owner].start_tx = (uint16_t)ux; w2.player[owner].start_ty = (uint16_t)uy;
			continue;
		}
		if (w2.player[owner].control == W2_CTRL_NOBODY) continue;
		/* the Mac converts every record to its owner's race (f8_3EDE: tables A5+0AE4 human /
		 * A5+0B52 orc, chosen by SIDE): a human unit of an orc player becomes its orc equivalent */
		if (owner < W2_NEUTRAL && w2.player[owner].race <= 1) {
			const w2_unit_type *ut = w2_unit((unsigned)type);
			if (ut && (ut->race == W2_RACE_HUMAN || ut->race == W2_RACE_ORC) && ut->race != (w2.player[owner].race ? W2_RACE_ORC : W2_RACE_HUMAN) &&
			    ut->equiv != W2_NONE && w2_unit(ut->equiv)) type = ut->equiv;
		}
		slot = unit_create(type, owner, ux, uy);
		if (slot < 0) continue;
		if (type == W2U_GOLD_MINE || type == W2U_OIL_PATCH || type == W2U_HUMAN_OIL_PLATFORM || type == W2U_ORC_OIL_PLATFORM)
			w2.unit[slot].res_left = (uint32_t)(param ? param : 120) * 2500u;
		else if (param) {
			g_passive[slot] = 1;
			g_passive_gen[slot] = unit_gen(slot);
		}
	}
	w2.map.version++;
	w2_log("pud: %dx%d era %d, %d unit records, %d units", w, h, w2.map.era, n, w2.unit_hi);
	return 0;
}

int pud_load_file(const char *path)
{
	FILE *f = fopen(path, "rb"); long n; uint8_t *buf; int r;
	if (!f) { w2_log("pud: cannot open %s", path); return -1; }
	fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
	if (n <= 0) { fclose(f); w2_log("pud: empty %s", path); return -1; }
	buf = (uint8_t *)malloc((size_t)n);
	if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(buf); return -1; }
	fclose(f);
	r = pud_load_mem(buf, (uint32_t)n);
	free(buf);
	return r;
}

int pud_load_builtin(int entry)
{
	unsigned long n; unsigned char *p = assets_load(ASSET_DATA, entry, &n); int r;
	if (!p) return -1;
	r = pud_load_mem(p, (uint32_t)n);
	war_free(p);
	return r;
}

/* description string (32 bytes) of a PUD in memory, "" if none */
void pud_describe(const uint8_t *d, uint32_t size, char *out, int cap, int *w, int *h)
{
	unsigned long len = 0; const unsigned char *s = d ? sec(d, size, "DESC", &len) : NULL; int i;
	if (cap > 0) {
		for (i = 0; s && i < cap - 1 && (unsigned long)i < len && s[i]; i++) out[i] = (char)s[i];
		out[s ? i : 0] = 0;
	}
	s = d ? sec(d, size, "DIM ", &len) : NULL;
	if (s && len < 4) s = NULL;
	if (w) *w = s ? (int)W2_LE16(s) : 0;
	if (h) *h = s ? (int)W2_LE16(s + 2) : 0;
}
