/* unitdraw.c - draws units, buildings, corpses/rubble and selection rectangles into view.buf.
 * Order: rubble, dead bodies, buildings + land/sea units sorted by footprint bottom, flyers on top.
 * Everything in hires map pixels; assets_draw_sprite() halves for lores, raw pixels use VX()/VY(). */
#include <string.h>
#include "w2.h"
#include "unit.h"
#include "assets.h"

#define CORPSE_GFX 120   /* War Data: neutral/units/corpses (72x72, 5-direction columns) */
/* destroyed site sprites per era (summer, winter, wasteland): large 64x64 (4 frames), small 32x32 */
static const uint16_t g_site_large[3] = { 121, 163, 191 };
static const uint16_t g_site_small[3] = { 189, 190, 188 };

static uint16_t g_ground[W2_MAX_UNITS], g_air[W2_MAX_UNITS];
static int32_t  g_gkey[W2_MAX_UNITS], g_akey[W2_MAX_UNITS];
static W2Clip   g_clip;

static int g_col_era = -2;
static const unsigned char *g_col_pal;
static unsigned char g_col_own, g_col_enemy, g_col_neutral;

static int era_index(void)
{
	int e = w2.map.era;
	if (e < 0 || e > 3) e = 0;
	return e;
}

static unsigned char nearest(const unsigned char *pal, int r, int g, int b)
{
	int i, best = 1; long bd = 0x7FFFFFFFL;
	for (i = 1; i < 256; i++) {
		long dr, dg, db, d;
		if (i >= 17 && i <= 19 && !assets_ehb()) continue;   /* hardware pointer pens (EHB: shared, real colours) */
		dr = pal[i * 3] - r; dg = pal[i * 3 + 1] - g; db = pal[i * 3 + 2] - b;
		d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
		if (d < bd) { bd = d; best = i; }
	}
	return (unsigned char)best;
}

static void sel_colours(void)
{
	const unsigned char *pal = assets_palette();
	if (pal == g_col_pal && g_col_era == assets_era()) return;
	g_col_pal = pal; g_col_era = assets_era();
	g_col_own = nearest(pal, 0, 252, 0);
	g_col_enemy = nearest(pal, 252, 0, 0);
	g_col_neutral = nearest(pal, 252, 252, 0);
}

/* map pixel -> hires screen pixel */
static int scr_x(int mx) { return view.scr_x + mx - view.map_x; }
static int scr_y(int my) { return view.scr_y + my - view.map_y; }

static int in_view(int x, int y, int w, int h)
{
	return x + w > view.map_x && x < view.map_x + view.scr_w && y + h > view.map_y && y < view.map_y + view.scr_h;
}

static void draw_at(const W2Spr *s, int frame, int mirror, int mx, int my, const unsigned char *remap)
{
	if (!s || frame < 0 || frame >= s->count) return;
	assets_draw_sprite(s, frame, mirror, view.buf, view.pitch, scr_x(mx), scr_y(my), &g_clip, remap);
}

/* ------------------------------------------------------------------------------------------ corpses */

static int tile_seen(int tx, int ty, int need_visible)
{
	if (tx < 0 || ty < 0 || tx >= w2.map.w || ty >= w2.map.h) return 0;
	return need_visible ? fog_visible(tx, ty) : fog_explored(tx, ty);
}

static void draw_corpses(int bodies)
{
	int i, era = era_index(); if (era > 2) era = 2;
	for (i = 0; i < W2_MAX_CORPSES; i++) {
		const W2Corpse *c = &w2_corpses[i];
		const W2Spr *s; int body, bw, bh, x, y, frame, mirror = 0;
		if (!c->used) continue;
		body = c->kind <= W2_CORPSE_DEAD_SEA_BODY;
		if (body != bodies) continue;
		if (!tile_seen(c->tx, c->ty, body)) continue;
		if (body) {
			s = assets_sprite(ASSET_DATA, CORPSE_GFX); bw = bh = 72;
			frame = (int)w2_dir_frame(c->frame, c->dir, &mirror);
		} else if (c->tw <= 1) {
			s = assets_sprite(ASSET_DATA, g_site_small[era]); bw = bh = 32; frame = c->frame & 1;
		} else {
			s = assets_sprite(ASSET_DATA, g_site_large[era]); bw = bh = 64; frame = c->frame;
		}
		x = c->tx * W2_TILE + c->ox + (c->tw * W2_TILE - bw) / 2;
		y = c->ty * W2_TILE + c->oy + (c->th * W2_TILE - bh) / 2;
		if (!in_view(x, y, bw, bh)) continue;
		draw_at(s, frame, mirror, x, y, body ? assets_remap(c->owner) : NULL);
	}
}

/* ------------------------------------------------------------------------------------------ units */

/* sprite, frame, mirror and box (hires) of a unit as it must be drawn now */
static const W2Spr *unit_gfx(int slot, int *frame, int *mirror, int *bw, int *bh)
{
	const W2Unit *u = &w2.unit[slot];
	const w2_unit_type *t = w2_unit(u->type);
	int era = era_index(), entry;
	const W2Spr *s;
	*mirror = 0;
	if (!t) return NULL;
	entry = t->gfx[era] ? t->gfx[era] : t->gfx[0];
	*bw = t->frame_w; *bh = t->frame_h;
	if ((u->flags & W2_UF_BUILDING) && (u->flags & W2_UF_UNDERCONST) && t->construction != W2_NONE
	    && t->construction < W2_NUM_CONSTRUCTIONS) {
		const w2_construction *c = &w2_constructions[t->construction];
		unsigned total = w2_time_to_cycles(t->build_time), pct = 100;
		int i, st = 0;
		if (total) pct = (unsigned)u->progress * 100u / total;
		for (i = 1; i < c->nstages && i < 3; i++) if (pct >= c->stage[i].percent) st = i;
		*frame = c->stage[st].frame;
		if (!c->stage[st].use_building_sprite) {
			int ce = c->gfx[era] ? c->gfx[era] : c->gfx[0];
			*bw = c->w; *bh = c->h;
			return assets_sprite(ASSET_DATA, ce);
		}
		return assets_sprite(ASSET_DATA, entry);
	}
	*frame = unit_sprite_frame(slot, mirror);
	if (u->res_kind && !(u->flags & W2_UF_BUILDING)) {
		const w2_harvest *h = w2_unit_harvest(u->type, u->res_kind);
		/* worker "carrying" sheets hold only the walk frames (0..24) */
		if (h && h->gfx_loaded && (!(t->flags & W2UF_WORKER) || u->frame < 25)) {
			s = assets_sprite(ASSET_DATA, h->gfx_loaded);
			if (s && *frame < s->count) return s;
		}
	}
	return assets_sprite(ASSET_DATA, entry);
}

static void hline(int x0, int x1, int y, unsigned char c)
{
	int x;
	if (y < g_clip.y0 || y >= g_clip.y1) return;
	if (x0 < g_clip.x0) x0 = g_clip.x0;
	if (x1 >= g_clip.x1) x1 = g_clip.x1 - 1;
	for (x = x0; x <= x1; x++) view.buf[y * view.pitch + x] = c;
}

static void vline(int x, int y0, int y1, unsigned char c)
{
	int y;
	if (x < g_clip.x0 || x >= g_clip.x1) return;
	if (y0 < g_clip.y0) y0 = g_clip.y0;
	if (y1 >= g_clip.y1) y1 = g_clip.y1 - 1;
	for (y = y0; y <= y1; y++) view.buf[y * view.pitch + x] = c;
}

static void draw_selection(int slot)
{
	const W2Unit *u = &w2.unit[slot];
	const w2_unit_type *t = w2_unit(u->type);
	int cx, cy, bw, bh, x0, y0, x1, y1; unsigned char c;
	if (!t) return;
	bw = t->box_w ? t->box_w : 31; bh = t->box_h ? t->box_h : 31;
	unit_center(slot, &cx, &cy);
	if (u->owner == w2.local_player) c = g_col_own;
	else if (u->owner < W2_MAX_PLAYERS && ((w2.player[w2.local_player].enemies >> u->owner) & 1)) c = g_col_enemy;
	else c = g_col_neutral;
	x0 = VX(scr_x(cx - bw / 2)); y0 = VY(scr_y(cy - bh / 2));
	x1 = VX(scr_x(cx - bw / 2 + bw)); y1 = VY(scr_y(cy - bh / 2 + bh));
	hline(x0, x1, y0, c); hline(x0, x1, y1, c);
	vline(x0, y0, y1, c); vline(x1, y0, y1, c);
}

static void draw_unit(int slot)
{
	const W2Unit *u = &w2.unit[slot];
	const w2_unit_type *t = w2_unit(u->type);
	int frame = 0, mirror = 0, bw = 0, bh = 0, x, y;
	const W2Spr *s = unit_gfx(slot, &frame, &mirror, &bw, &bh);
	if (!t) return;
	if (!bw || !bh) { if (!s) return; bw = s->box_w; bh = s->box_h; }
	x = u->tx * W2_TILE + u->ox + (t->tile_w * W2_TILE - bw) / 2;
	y = u->ty * W2_TILE + u->oy + (t->tile_h * W2_TILE - bh) / 2;
	if (s && in_view(x, y, bw, bh))
		draw_at(s, frame, mirror, x, y, assets_remap(u->owner < 8 ? u->owner : -1));
	if ((u->flags & (W2_UF_SELECTED | W2_UF_ALIVE)) == (W2_UF_SELECTED | W2_UF_ALIVE)) draw_selection(slot);
}

static void sort_list(uint16_t *a, int32_t *k, int n)
{
	/* shell sort (stable enough; n = units in view) */
	int gap, i, j;
	for (gap = n / 2; gap > 0; gap /= 2)
		for (i = gap; i < n; i++) {
			int32_t kk = k[i]; uint16_t aa = a[i];
			for (j = i; j >= gap && k[j - gap] > kk; j -= gap) { k[j] = k[j - gap]; a[j] = a[j - gap]; }
			k[j] = kk; a[j] = aa;
		}
}

void unitdraw_draw(void)
{
	int slot, ng = 0, na = 0, i;
	if (!view.buf) return;
	g_clip.x0 = VX(view.scr_x); g_clip.y0 = VY(view.scr_y);
	g_clip.x1 = VX(view.scr_x + view.scr_w); g_clip.y1 = VY(view.scr_y + view.scr_h);
	sel_colours();

	draw_corpses(0);   /* rubble */
	draw_corpses(1);   /* dead bodies */

	for (slot = 0; slot < w2.unit_hi; slot++) {
		const W2Unit *u = &w2.unit[slot];
		const w2_unit_type *t;
		int x, y, w, h, key;
		if (!u->flags || (u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN))) continue;
		t = w2_unit(u->type);
		if (!t) continue;
		unit_box(slot, &x, &y, &w, &h);
		if (!in_view(x - 4, y - 4, w + 8, h + 8)) continue;   /* margin: selection box may exceed the frame */
		if (!unit_visible_local(slot)) continue;
		key = (u->ty + t->tile_h) * W2_TILE + u->oy;
		key = key * 2 + ((u->flags & W2_UF_BUILDING) ? 0 : 1);   /* same row: unit in front of building */
		if (u->flags & W2_UF_AIR) { g_air[na] = (uint16_t)slot; g_akey[na++] = key; }
		else { g_ground[ng] = (uint16_t)slot; g_gkey[ng++] = key; }
	}
	sort_list(g_ground, g_gkey, ng);
	sort_list(g_air, g_akey, na);
	for (i = 0; i < ng; i++) draw_unit(g_ground[i]);
	for (i = 0; i < na; i++) draw_unit(g_air[i]);
}
