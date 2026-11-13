/* missile.c - missiles in flight, impacts, explosions (w2_missiles[] classes).
 * Static pool of W2_MAX_MISSILES; positions in map pixels (hires); integer maths only. */
#include <string.h>
#include "w2.h"
#include "assets.h"
#include "combat.h"
#include "unit.h"

typedef struct Missile {
	uint8_t  used, type, cls, anim_n;   /* anim_n: animation frames (frames/5 for 9-dir sprites) */
	uint8_t  frame, wait, dir, bounces;
	uint8_t  layer, stype, sowner, pad;
	int16_t  x, y, x0, y0, x1, y1;      /* current position, flight segment */
	uint16_t k, n;                      /* step k of n */
	int16_t  shooter;                   /* unit slot or -1 */
	uint16_t target;                    /* unit slot or W2_NO_UNIT (ground) */
	uint8_t  mode, pad2;                /* DM_* */
	uint16_t dmg;                       /* DM_DIRECT: damage rolled at launch; DM_AREA*: rating */
} Missile;

enum { DM_NONE, DM_DIRECT, DM_AREA, DM_AREA_FIXED };

/* missiles with the original's splash routine (RE_NOTES §5: big cannon, rock, bolt, small cannon)
 * plus the bouncing ones (gryphon hammer, dragon breath, fireball: same geometry, NIEPEWNE) */
static int area_type(int type)
{
	return type == W2M_BIG_CANNON || type == W2M_CATAPULT_ROCK || type == W2M_BALLISTA_BOLT ||
	       type == W2M_SMALL_CANNON || w2_missiles[type].cls == W2MC_POINT_TO_POINT_BOUNCE;
}

/* flight speed px/cycle: the original's values where known (RE_NOTES §7), else the table */
static int missile_speed(int type)
{
	switch (type) {
	case W2M_ARROW: case W2M_AXE: return 12;
	case W2M_BALLISTA_BOLT: case W2M_CATAPULT_ROCK: return 8;
	case W2M_BIG_CANNON: case W2M_SMALL_CANNON: case W2M_LIGHTNING:
	case W2M_SUBMARINE_MISSILE: case W2M_TURTLE_MISSILE: return 16;
	default: return w2_missiles[type].speed ? w2_missiles[type].speed : 16;
	}
}

static Missile g_m[W2_MAX_MISSILES];
/* identity of the stored shooter / target slots (unit_gen at launch): a unit that dies during the
 * flight and a newcomer in its slot must not be hit or credited (unit_same at impact) */
static uint16_t g_sgen[W2_MAX_MISSILES], g_tgen[W2_MAX_MISSILES];
static int g_hi;                       /* highest used index + 1 */
static int g_count;

static const int8_t dir_dx[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static const int8_t dir_dy[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

void missile_reset(void) { memset(g_m, 0, sizeof g_m); g_hi = 0; g_count = 0; }
int  missile_count(void) { return g_count; }

static int iabs(int v) { return v < 0 ? -v : v; }

/* 8-way heading of a vector (0 = N clockwise), sectors of 45 degrees */
static int heading(int dx, int dy)
{
	int ax = iabs(dx), ay = iabs(dy);
	if (!dx && !dy) return 4;
	if (ax * 12 < ay * 5) return dy < 0 ? 0 : 4;      /* |dx| < 0.414 |dy| */
	if (ay * 12 < ax * 5) return dx < 0 ? 6 : 2;
	if (dx > 0) return dy < 0 ? 1 : 3;
	return dy < 0 ? 7 : 5;
}

/* octagonal approximation of the euclidean length */
static int vlen(int dx, int dy)
{
	int ax = iabs(dx), ay = iabs(dy);
	return ax > ay ? ax + (ay * 3 >> 3) : ay + (ax * 3 >> 3);
}

static void set_segment(Missile *m, int x0, int y0, int x1, int y1)
{
	int len = vlen(x1 - x0, y1 - y0), sp = missile_speed(m->type);
	m->x0 = (int16_t)x0; m->y0 = (int16_t)y0; m->x1 = (int16_t)x1; m->y1 = (int16_t)y1;
	m->x = (int16_t)x0; m->y = (int16_t)y0;
	m->k = 0;
	m->n = (uint16_t)((len + sp - 1) / sp);
	if (m->n < 1) m->n = 1;
	if (x1 != x0 || y1 != y0) m->dir = (uint8_t)heading(x1 - x0, y1 - y0);
}

static int flies(int cls)
{
	return cls == W2MC_POINT_TO_POINT || cls == W2MC_POINT_TO_POINT_WITH_HIT || cls == W2MC_POINT_TO_POINT_BOUNCE ||
	       cls == W2MC_PARABOLIC || cls == W2MC_DEATH_COIL;
}

static Missile *spawn(int type, int sx, int sy, int dx, int dy)
{
	int i;
	Missile *m;
	const w2_missile *d;
	if (type < 0 || type >= W2_NUM_MISSILES || type == W2M_NONE) return NULL;
	for (i = 0; i < W2_MAX_MISSILES && g_m[i].used; i++) ;
	if (i >= W2_MAX_MISSILES) return NULL;          /* pool full: drop (visual loss only for effects) */
	m = &g_m[i];
	memset(m, 0, sizeof *m);
	d = &w2_missiles[type];
	m->used = 1; m->type = (uint8_t)type; m->cls = d->cls;
	m->anim_n = (uint8_t)(d->dirs == 9 ? d->frames / 5 : d->frames);
	if (!m->anim_n) m->anim_n = 1;
	m->wait = d->sleep ? d->sleep : 1;
	m->bounces = d->bounces;
	m->shooter = -1; m->target = W2_NO_UNIT;
	m->dir = 4;
	set_segment(m, sx, sy, dx, dy);
	if (!flies(m->cls)) { m->x = (int16_t)dx; m->y = (int16_t)dy; }
	if (i >= g_hi) g_hi = i + 1;
	g_count++;
	return m;
}

static void free_missile(Missile *m)
{
	m->used = 0;
	g_count--;
	while (g_hi > 0 && !g_m[g_hi - 1].used) g_hi--;
}

static void launch(int shooter, int type, int sx, int sy, int dx, int dy, int target, int fixed, int weapon)
{
	Missile *m;
	int tvalid = target >= 0 && target < W2_MAX_UNITS && (w2.unit[target].flags & W2_UF_ALIVE);
	if (weapon) {   /* original launch 0x001295DE: target jitter (r & 7) - 3 per axis */
		dx += (int)(w2_rand() & 7u) - 3;
		dy += (int)(w2_rand() & 7u) - 3;
	}
	m = spawn(type, sx, sy, dx, dy);
	if (!m) return;
	if (shooter >= 0 && shooter < W2_MAX_UNITS && (w2.unit[shooter].flags & W2_UF_ALIVE)) {
		m->shooter = (int16_t)shooter;
		g_sgen[m - g_m] = unit_gen(shooter);
		m->stype = w2.unit[shooter].type; m->sowner = w2.unit[shooter].owner;
	}
	if (tvalid) {
		const w2_unit_type *t = w2_unit(w2.unit[target].type);
		m->target = (uint16_t)target;
		g_tgen[m - g_m] = unit_gen(target);
		m->layer = (uint8_t)(t && t->kind == W2_KIND_AIR ? W2_TGT_AIR : (W2_TGT_LAND | W2_TGT_SEA));
	} else m->layer = W2_TGT_LAND | W2_TGT_SEA;
	if (fixed > 0) {
		m->dmg = (uint16_t)fixed;
		m->mode = (uint8_t)(area_type(type) || !tvalid ? DM_AREA_FIXED : DM_DIRECT);
	} else if (weapon && m->shooter >= 0) {
		int b, p;
		combat_weapon(shooter, &b, &p);
		if (area_type(type) || !tvalid) {
			m->mode = DM_AREA;                     /* attack rating, armor applied per victim */
			m->dmg = (uint16_t)(b + p > 255 ? 255 : b + p);
		} else {
			m->mode = DM_DIRECT;                   /* CalcDamage at launch */
			m->dmg = (uint16_t)combat_roll(b, p, combat_armor(target));
		}
	}
}

void missile_fire(int shooter, int type, int sx, int sy, int dx, int dy, int target)
{
	launch(shooter, type, sx, sy, dx, dy, target, 0, 1);
}

void missile_fire_dmg(int shooter, int type, int sx, int sy, int dx, int dy, int target, int dmg)
{
	launch(shooter, type, sx, sy, dx, dy, target, dmg > 0 ? dmg : 0, 0);
}

/* missile reached (x1,y1) */
static void impact(Missile *m)
{
	const w2_missile *d = &w2_missiles[m->type];
	int attacker = -1, x = m->x1, y = m->y1;
	int mi = (int)(m - g_m);
	/* shooter / target by (slot, generation): a newcomer in a reused slot is neither credited nor hit */
	if (m->shooter >= 0 && unit_same(m->shooter, g_sgen[mi])) attacker = m->shooter;
	if (m->mode == DM_DIRECT && m->target != W2_NO_UNIT) {
		if (m->dmg && unit_same(m->target, g_tgen[mi])) unit_damage(m->target, m->dmg, attacker);
	} else if (m->mode == DM_AREA || m->mode == DM_AREA_FIXED) {
		combat_area_damage(x, y, m->dmg, attacker, m->layer, m->mode == DM_AREA_FIXED);
	}
	if (d->impact_missile != W2_NONE) spawn(d->impact_missile, x, y, x, y);
	if (d->impact_sound != W2_NONE) snd_play(d->impact_sound, x, y);
	if (m->cls == W2MC_POINT_TO_POINT_BOUNCE && m->bounces > 0) {
		int nx = x + dir_dx[m->dir] * W2_TILE, ny = y + dir_dy[m->dir] * W2_TILE;
		int mw = w2.map.w * W2_TILE, mh = w2.map.h * W2_TILE;
		m->bounces--;
		m->target = W2_NO_UNIT;
		if (m->mode == DM_DIRECT) m->mode = DM_AREA_FIXED;
		if (nx >= 0 && ny >= 0 && nx < mw && ny < mh) { set_segment(m, x, y, nx, ny); return; }
	}
	free_missile(m);
}

void missile_tick_all(void)
{
	int i;
	for (i = 0; i < g_hi; i++) {
		Missile *m = &g_m[i];
		const w2_missile *d;
		if (!m->used) continue;
		if (m->wait > 1) { m->wait--; continue; }
		d = &w2_missiles[m->type];
		m->wait = d->sleep ? d->sleep : 1;
		if (flies(m->cls)) {
			m->k++;
			m->x = (int16_t)(m->x0 + (int)(m->x1 - m->x0) * (int)m->k / (int)m->n);
			m->y = (int16_t)(m->y0 + (int)(m->y1 - m->y0) * (int)m->k / (int)m->n);
			if (++m->frame >= m->anim_n) m->frame = 0;
			if (m->k >= m->n) impact(m);
		} else {
			/* stay / cycle-once / fire / spell effects: play the frames once */
			if (++m->frame >= m->anim_n) free_missile(m);
		}
	}
}

void missile_draw_all(void)
{
	int i;
	W2Clip clip;
	if (!view.buf) return;
	clip.x0 = VX(view.scr_x); clip.y0 = VY(view.scr_y);
	clip.x1 = VX(view.scr_x + view.scr_w); clip.y1 = VY(view.scr_y + view.scr_h);
	for (i = 0; i < g_hi; i++) {
		const Missile *m = &g_m[i];
		const w2_missile *d;
		const W2Spr *spr;
		int f, mirror = 0, px, py, tx, ty;
		if (!m->used) continue;
		d = &w2_missiles[m->type];
		if (!d->gfx) continue;
		px = m->x - view.map_x; py = m->y - view.map_y;
		if (m->cls == W2MC_PARABOLIC && m->n > 1)     /* arc: height n/2 steps * 8 px at the top */
			py -= (int)m->k * (int)(m->n - m->k) * 32 / (int)m->n;
		if (px + d->w / 2 < 0 || py + d->h / 2 < 0 || px - d->w / 2 >= view.scr_w || py - d->h / 2 >= view.scr_h) continue;
		tx = m->x >> 5; ty = m->y >> 5;
		if (tx >= 0 && ty >= 0 && tx < w2.map.w && ty < w2.map.h && !fog_visible(tx, ty)) continue;
		spr = assets_sprite(ASSET_DATA, d->gfx);
		if (!spr) continue;
		if (d->dirs == 9) f = (int)w2_dir_frame((unsigned)m->frame * 5u, m->dir, &mirror);
		else f = m->frame;
		if (f >= spr->count) f = spr->count - 1;
		assets_draw_sprite(spr, f, mirror, view.buf, view.pitch,
		                   view.scr_x + px - d->w / 2, view.scr_y + py - d->h / 2, &clip, NULL);
	}
}
