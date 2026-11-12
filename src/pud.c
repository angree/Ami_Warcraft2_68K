/* pud.c - load a PUD map into the world (map, players, units). Format: docs/DATA_FORMAT.md section 12. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "w2.h"
#include "pud.h"
#include "assets.h"
#include "w2cfg.h"

/* classify one tile from its MTXM value and the era's tile map group flags */
static unsigned tile_flags(unsigned v)
{
	/* tile-map group flags == the PUD SQM movement map (verified identical on the built-in maps):
	 * 0x40 water, 0x01 land, 0x02 land that cannot be built on, 0x10 coast, 0x80 obstacle,
	 * 0x08 wall (0x89 orc / 0x8D human). Forest AND rock groups both carry 0x81, and parts of the
	 * water/coast boundary (0x02xx) carry 0x82, so tree/rock is decided by the tile number:
	 * solid 0x007x forest, 0x008x rock; boundary 0x07xx forest/ground, 0x04xx rock/coast. */
	unsigned gf = assets_tile_group_flags(v), f = 0;
	unsigned hi = v >> 8, sg = (v >> 4) & 0x0F;
	if (gf & 0x40) f |= W2_TF_WATER | W2_TF_NOWALK | W2_TF_NOBUILD;
	else f |= W2_TF_LAND;
	if (gf & 0x10) f |= W2_TF_COAST;
	if (gf & 0x02) f |= W2_TF_NOBUILD;
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

int pud_load_mem(const uint8_t *d, uint32_t size)
{
	unsigned long len; const unsigned char *s; int i, x, y, n, w, h;
	memset(&w2.map, 0, sizeof w2.map);
	memset(w2.player, 0, sizeof w2.player);
	memset(w2.unit, 0, sizeof w2.unit);
	w2.unit_hi = 0; w2.cycle = 0; w2.select_n = 0; w2.game_over = 0;

	s = w2_pud_section(d, size, "DIM ", &len);
	if (!s || len < 4) { w2_log("pud: no DIM"); return -1; }
	w = W2_LE16(s); h = W2_LE16(s + 2);
	if (w < 1 || h < 1 || w > W2_MAP_MAX || h > W2_MAP_MAX) { w2_log("pud: bad size %dx%d", w, h); return -1; }
	w2.map.w = w; w2.map.h = h;

	s = w2_pud_section(d, size, "ERAX", &len);
	if (!s) s = w2_pud_section(d, size, "ERA ", &len);
	w2.map.era = s ? W2_LE16(s) : 0;
	if (assets_set_era(w2.map.era)) return -1;
	w2.map.era = assets_era();

	s = w2_pud_section(d, size, "OWNR", &len);
	for (i = 0; i < W2_MAX_PLAYERS; i++) {
		W2Player *p = &w2.player[i];
		p->control = (uint8_t)(s && (unsigned long)i < len ? ctrl_from_ownr(s[i]) : (i == W2_NEUTRAL ? W2_CTRL_NEUTRAL : W2_CTRL_NOBODY));
		p->colour = (uint8_t)(i & 7);
		p->start_tx = p->start_ty = 0xFFFF;
		p->allowed_units = 0xFFFFFFFFu; p->allowed_spells = 0xFFFFFFFFu;
	}
	s = w2_pud_section(d, size, "SIDE", &len);
	for (i = 0; s && i < W2_MAX_PLAYERS && (unsigned long)i < len; i++) w2.player[i].race = s[i] > 1 ? 0 : s[i];
	s = w2_pud_section(d, size, "SGLD", &len);
	for (i = 0; s && i < W2_MAX_PLAYERS && (unsigned long)(i * 2 + 2) <= len; i++) w2.player[i].gold = (int32_t)W2_LE16(s + i * 2);
	s = w2_pud_section(d, size, "SLBR", &len);
	for (i = 0; s && i < W2_MAX_PLAYERS && (unsigned long)(i * 2 + 2) <= len; i++) w2.player[i].wood = (int32_t)W2_LE16(s + i * 2);
	s = w2_pud_section(d, size, "SOIL", &len);
	for (i = 0; s && i < W2_MAX_PLAYERS && (unsigned long)(i * 2 + 2) <= len; i++) w2.player[i].oil = (int32_t)W2_LE16(s + i * 2);
	s = w2_pud_section(d, size, "AIPL", &len);
	for (i = 0; s && i < W2_MAX_PLAYERS && (unsigned long)i < len; i++) w2.player[i].ai_type = s[i];
	for (i = 0; i < 8; i++) w2.player[i].enemies = (uint16_t)(0x00FF & ~(1u << i));   /* free-for-all */

	s = w2_pud_section(d, size, "MTXM", &len);
	if (!s || len < (unsigned long)w * h * 2) { w2_log("pud: no MTXM"); return -1; }
	for (y = 0; y < h; y++)
		for (x = 0; x < w; x++) {
			i = y * w + x;
			w2.map.tile[i] = (uint16_t)W2_LE16(s + i * 2);
			w2.map.occ_ground[i] = W2_NO_UNIT;
			w2.map.occ_air[i] = W2_NO_UNIT;
			w2.map.wood[i] = 1;
			pud_reclassify_tile(x, y);
		}

	/* units */
	s = w2_pud_section(d, size, "UNIT", &len);
	n = s ? (int)(len / 8) : 0;
	for (i = 0; i < n; i++) {
		const unsigned char *u = s + i * 8;
		int ux = W2_LE16(u), uy = W2_LE16(u + 2), type = u[4], owner = u[5], data = W2_LE16(u + 6), slot;
		if (owner >= W2_MAX_PLAYERS) owner = W2_NEUTRAL;
		if (type == W2U_HUMAN_START_LOCATION || type == W2U_ORC_START_LOCATION) {
			w2.player[owner].start_tx = (uint16_t)ux; w2.player[owner].start_ty = (uint16_t)uy;
			continue;
		}
		if (w2.player[owner].control == W2_CTRL_NOBODY) continue;
		slot = unit_create(type, owner, ux, uy);
		if (slot < 0) continue;
		if (type == W2U_GOLD_MINE || type == W2U_OIL_PATCH || type == W2U_HUMAN_OIL_PLATFORM || type == W2U_ORC_OIL_PLATFORM)
			w2.unit[slot].res_left = data ? (uint32_t)data * 2500u : 100000u;   /* SPEC_RULES: 0 = default */
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
	unsigned long len; const unsigned char *s = w2_pud_section(d, size, "DESC", &len); int i;
	if (cap <= 0) return;
	out[0] = 0;
	for (i = 0; s && i < cap - 1 && (unsigned long)i < len && s[i]; i++) out[i] = (char)s[i];
	if (s) out[i] = 0;
	s = w2_pud_section(d, size, "DIM ", &len);
	if (w) *w = s ? (int)W2_LE16(s) : 0;
	if (h) *h = s ? (int)W2_LE16(s + 2) : 0;
}
