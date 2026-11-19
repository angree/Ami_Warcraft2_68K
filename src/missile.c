/* missile.c - missiles in flight, impacts, explosions (w2_missiles[] classes).
 * Static pool of W2_MAX_MISSILES; positions in map pixels (hires); integer maths only. */
#include <string.h>
#include "w2.h"
#include "assets.h"
#include "combat.h"
#include "unit.h"
#include "pud.h"

typedef struct Missile {
	uint8_t  used, type, cls, anim_n;   /* anim_n: animation frames (frames/5 for 9-dir sprites) */
	uint8_t  frame, wait, dir, bounces;
	uint8_t  layer, stype, sowner, wsplash;   /* wsplash: the Mac's splash missile (walls hit too) */
	int16_t  x, y, x0, y0, x1, y1;      /* current position, flight segment */
	uint16_t k, n;                      /* step k of n */
	int16_t  shooter;                   /* unit slot or -1 */
	uint16_t target;                    /* unit slot or W2_NO_UNIT (ground) */
	uint8_t  mode, spell;               /* DM_*; MSP_* spell behaviour (0 = weapon/plain effect) */
	uint16_t dmg;                       /* DM_DIRECT: damage rolled at launch; DM_AREA*: rating */
	/* spell missiles (MSP_*): 16.16 position + direction per pixel of the major axis (the original's
	 * Line_Init/Line_Step walks the Chebyshev distance), remaining distance, counters */
	int32_t  fx, fy, vx, vy;
	int16_t  rem, tx, ty;               /* remaining distance (px); target point */
	uint16_t ttl, count;                /* lifetime / repeat counters (meaning per kind) */
	uint8_t  phase, fwait, fend, pad3;  /* orbit index; frame timer; last frame + 1 (effects) */
} Missile;

enum { DM_NONE, DM_DIRECT, DM_AREA, DM_AREA_FIXED, DM_WALL };   /* DM_WALL: no target unit, dmg -> wall tile */

/* the original's splash flag A5+2988 (MissileSplash 0x00129D52: walls on the centre + 4 neighbours) */
static int mac_splash(int type)
{
	return type == W2M_BIG_CANNON || type == W2M_CATAPULT_ROCK || type == W2M_BALLISTA_BOLT || type == W2M_SMALL_CANNON;
}

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
		m->wsplash = (uint8_t)mac_splash(type);
		if (!tvalid && !m->wsplash) {
			/* Missile_Spawn 0x001295DE: a non-splash missile carries CalcDamage(u, NULL) (no armor);
			 * with no target unit the impact only hits a wall tile (MissileState6_Impact 0x00129EEA) */
			m->mode = DM_WALL;
			m->dmg = (uint16_t)combat_roll(b, p, 0);
		} else if (area_type(type) || !tvalid) {
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

/* ==== spell missiles: the original's missile states 7..12 (CODE 5, 0x0012A03A..0x0012A9BE) ========== */
/* flame shield orbit around the unit centre: 51 positions (A5+2AAE x, A5+29E2 y) */
static const int8_t k_orbit_x[51] = { 1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 30, 31, 32, 32, 32, 31, 30, 28, 25, 22, 19, 16, 13,
	10, 7, 4, 1, -2, -5, -8, -11, -14, -17, -20, -23, -25, -27, -28, -28, -28, -27, -26, -24, -22, -19, -16, -13, -10, -7, -4, -1 };
static const int8_t k_orbit_y[51] = { -34, -34, -34, -33, -32, -31, -30, -28, -26, -23, -20, -17, -14, -11, -8, -5, -2, 1, 4, 6, 8,
	9, 10, 11, 12, 12, 12, 12, 11, 11, 10, 8, 7, 5, 2, -1, -4, -7, -10, -13, -16, -19, -22, -25, -27, -29, -31, -32, -33, -34, -34 };
#define SPL_ALL (W2_TGT_LAND | W2_TGT_SEA | W2_TGT_AIR)

static void set_shooter(Missile *m, int s)
{
	if (s >= 0 && s < W2_MAX_UNITS && (w2.unit[s].flags & W2_UF_ALIVE)) {
		m->shooter = (int16_t)s;
		g_sgen[m - g_m] = unit_gen(s);
		m->stype = w2.unit[s].type; m->sowner = w2.unit[s].owner;
	}
}

static int shooter_now(const Missile *m)
{
	return (m->shooter >= 0 && unit_same(m->shooter, g_sgen[m - g_m])) ? m->shooter : -1;
}

/* straight line, whole-pixel speed along the major axis (the original's Line_Init / Line_Step) */
static void line_to(Missile *m, int x0, int y0, int x1, int y1)
{
	int dx = x1 - x0, dy = y1 - y0, d = iabs(dx) > iabs(dy) ? iabs(dx) : iabs(dy);
	m->fx = (int32_t)x0 * 65536; m->fy = (int32_t)y0 * 65536;
	m->x = (int16_t)x0; m->y = (int16_t)y0;
	m->tx = (int16_t)x1; m->ty = (int16_t)y1;
	if (d) {
		m->vx = (int32_t)dx * 65536 / d;
		m->vy = (int32_t)dy * 65536 / d;
		m->dir = (uint8_t)heading(dx, dy);
	} else m->vx = m->vy = 0;
	m->rem = (int16_t)d;
}

static void line_step(Missile *m, int sp)
{
	m->fx += m->vx * sp; m->fy += m->vy * sp;
	m->x = (int16_t)(m->fx >> 16); m->y = (int16_t)(m->fy >> 16);
}

static int off_map(const Missile *m)
{
	return m->x < -256 || m->y < -256 || m->x >= w2.map.w * W2_TILE + 256 || m->y >= w2.map.h * W2_TILE + 256;
}

/* splash of a spell missile at its position: the original's MissileSplash(m, 1) - ground and air; these
 * missile types ignore armor (A5+296A != 0); the shooter itself is never hit */
static void spell_splash(Missile *m, int rating)
{
	combat_area_damage(m->x, m->y, rating, shooter_now(m), SPL_ALL, 1);
}

static Missile *spell_spawn(int type, int kind, int x, int y)
{
	Missile *m = spawn(type, x, y, x, y);
	if (!m) return NULL;
	m->spell = (uint8_t)kind;
	m->x = (int16_t)x; m->y = (int16_t)y;
	m->fx = (int32_t)x * 65536; m->fy = (int32_t)y * 65536;
	return m;
}

/* play the frames [first, end) once, 'wait' cycles each */
static void as_effect(Missile *m, int first, int end, int wait)
{
	m->spell = MSP_EFFECT;
	m->frame = (uint8_t)first; m->fend = (uint8_t)end;
	m->pad3 = (uint8_t)(wait < 1 ? 1 : wait); m->fwait = m->pad3;
}

void missile_effect(int type, int x, int y)
{
	const w2_missile *d;
	Missile *m;
	if (type < 0 || type >= W2_NUM_MISSILES || type == W2M_NONE) return;
	d = &w2_missiles[type];
	m = spell_spawn(type, MSP_EFFECT, x, y);
	if (!m) return;
	as_effect(m, 0, d->dirs == 9 ? m->anim_n : (d->frames ? d->frames : 1), type == W2M_EXPLOSION ? 2 : 3);
}

/* FUN_0012A03A: a shard starts 110 px left / 170 px above the field point (+-11 px jitter), rating 10 */
static void blizzard_shard(int shooter, int x, int y, int count)
{
	int r1 = (int)(w2_rand() % 3u), r2 = (int)(w2_rand() % 3u);
	Missile *m = spell_spawn(W2M_BLIZZARD, MSP_BLIZZARD, x, y);
	if (!m) return;
	set_shooter(m, shooter);
	m->dmg = 10; m->count = (uint16_t)count;
	line_to(m, x - 110 - (1 - r1) * 11, y - 170 - (1 - r2) * 11, x, y);
	m->fwait = 3;
}

void missile_spell(int kind, int shooter, int target, int x, int y, int arg)
{
	Missile *m;
	int sx = x, sy = y, i;
	if (shooter >= 0 && shooter < W2_MAX_UNITS && (w2.unit[shooter].flags & W2_UF_ALIVE)) unit_center(shooter, &sx, &sy);
	else shooter = -1;
	switch (kind) {
	case MSP_BREATH:        /* fireball spell: LaunchBreathMissile 0x0012A242 (missile 2, rating 40) */
		m = spell_spawn(W2M_DRAGON_BREATH, kind, sx, sy);
		if (!m) return;
		set_shooter(m, shooter);
		m->dmg = (uint16_t)(arg > 0 ? arg : 40);
		line_to(m, sx, sy, x, y);
		if (!m->rem && shooter >= 0) {        /* cast onto its own tile: roll on in the facing direction */
			m->vx = (int32_t)dir_dx[w2.unit[shooter].dir] * 65536;
			m->vy = (int32_t)dir_dy[w2.unit[shooter].dir] * 65536;
		}
		break;
	case MSP_SHIELD_GO:     /* flame shield: LaunchFireballMissile 0x0012A372 (missile 3) to the target unit */
		if (target < 0 || target >= W2_MAX_UNITS || !(w2.unit[target].flags & W2_UF_ALIVE)) return;
		m = spell_spawn(W2M_FIREBALL, kind, sx, sy);
		if (!m) return;
		set_shooter(m, shooter);
		m->target = (uint16_t)target; g_tgen[m - g_m] = unit_gen(target);
		unit_center(target, &x, &y);
		line_to(m, sx, sy, x, y);
		break;
	case MSP_SHIELD:        /* the 5 flames (FUN_0012A572): orbit index 0/10/20/30/40, ttl 512 + 3*f */
		if (target < 0 || target >= W2_MAX_UNITS || !(w2.unit[target].flags & W2_UF_ALIVE)) return;
		for (i = 0; i < 5; i++) {
			static const uint8_t f0[5] = { 0, 4, 2, 3, 5 };
			int cx, cy;
			unit_center(target, &cx, &cy);
			m = spell_spawn(W2M_FLAME_SHIELD, kind, cx + k_orbit_x[i * 10], cy + k_orbit_y[i * 10]);
			if (!m) return;
			set_shooter(m, target);           /* the shielded unit: never hit by its own flames */
			m->phase = (uint8_t)(i * 10); m->frame = f0[i]; m->ttl = (uint16_t)(0x200 + 3 * f0[i]);
			m->dmg = 4; m->fwait = 2;
		}
		break;
	case MSP_BLIZZARD:      /* one field (Missile_SpawnAreaRandom): the shard, 'arg' more follow */
		blizzard_shard(shooter, x, y, arg);
		break;
	case MSP_DND: {         /* Missile_AreaRandom 0x0012A1AE: point +-2 tiles, 'arg' splashes of 10 */
		int ox = ((int)(w2_rand() % 5u) - 2) * W2_TILE, oy;
		oy = ((int)(w2_rand() % 5u) - 2) * W2_TILE;
		m = spell_spawn(W2M_DEATH_AND_DECAY, kind, x + ox, y + oy);
		if (!m) return;
		set_shooter(m, shooter);
		m->dmg = 10; m->count = (uint16_t)(arg > 0 ? arg : 10); m->ttl = 0; m->frame = 1;
		break; }
	case MSP_WHIRL: {       /* Missile_WhirlwindSpawn 0x0012A8C0: +-64 px around the point, 800 steps, rating 4 */
		int ox = (int)(w2_rand() & 0x7Fu) - 0x40, oy;
		oy = (int)(w2_rand() & 0x7Fu) - 0x40;
		m = spell_spawn(W2M_WHIRLWIND, kind, x + ox, y + oy);
		if (!m) return;
		set_shooter(m, shooter);
		m->dmg = 4; m->ttl = (uint16_t)(arg > 0 ? arg : 800); m->fwait = 3;
		line_to(m, x + ox, y + oy, x, y);
		break; }
	default: break;
	}
}

/* one cycle of a spell missile */
static void spell_tick(Missile *m)
{
	const w2_missile *d = &w2_missiles[m->type];
	switch (m->spell) {
	case MSP_EFFECT:
		if (--m->fwait) return;
		m->fwait = m->pad3;
		if (++m->frame >= m->fend) free_missile(m);
		return;
	case MSP_BREATH: {      /* state 7: half speed in the last 50 px, then rolls on: splash every 8 steps */
		int sp = 12, lim = m->dmg == 40 ? 40 : 25;
		(void)w2_rand();                  /* the original calls Random() here and drops the result */
		if (m->rem < 50) sp >>= 1;
		line_step(m, sp);
		m->rem = (int16_t)(m->rem - sp);
		if (m->rem < 50) {
			if (m->count > lim) { free_missile(m); return; }
			m->count++;
			if (!(m->count & 7)) {
				spell_splash(m, m->dmg);
				missile_effect(W2M_EXPLOSION, m->x, m->y);
			}
		}
		if (off_map(m)) free_missile(m);
		return; }
	case MSP_SHIELD_GO:     /* state 8: fly, then the 5 flames around the target */
		(void)w2_rand();
		if (!m->count) {
			line_step(m, 12);
			m->rem = (int16_t)(m->rem - 12);
			if (m->rem < 1) m->count = 1;
			return;
		}
		if (m->target != W2_NO_UNIT && unit_same(m->target, g_tgen[m - g_m]))
			missile_spell(MSP_SHIELD, -1, m->target, 0, 0, 0);
		free_missile(m);
		return;
	case MSP_SHIELD: {      /* state 9: orbit while the unit lives, splash 4 when (ttl & 15) == 1 */
		int cx, cy;
		if (shooter_now(m) < 0 || --m->ttl == 0) { free_missile(m); return; }
		if (++m->phase >= 51) m->phase = 0;
		unit_center(m->shooter, &cx, &cy);
		m->x = (int16_t)(cx + k_orbit_x[m->phase]); m->y = (int16_t)(cy + k_orbit_y[m->phase]);
		if ((m->ttl & 15) == 1) spell_splash(m, m->dmg);
		if (--m->fwait == 0) { m->fwait = 2; if (++m->frame >= (d->frames ? d->frames : 1)) m->frame = 0; }
		return; }
	case MSP_BLIZZARD:      /* state 10: fall onto the point, splash 10, then the next shard of the field */
		line_step(m, 12);
		m->rem = (int16_t)(m->rem - 12);
		if (m->rem < 0) {
			int sh = shooter_now(m);
			spell_splash(m, m->dmg);
			if (m->count) blizzard_shard(sh, m->tx, m->ty, m->count - 1);
			as_effect(m, 2, 4, 2);            /* impact frames */
			return;
		}
		(void)w2_rand();
		if (--m->fwait == 0) { m->fwait = 3; m->frame ^= 1; }
		return;
	case MSP_DND: {         /* state 11: one splash per animation loop (27 cycles), 'count' times */
		static const uint8_t fr[18] = { 1, 1, 1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7, 7, 7 };
		int t = m->ttl % 27;
		m->ttl++;
		m->frame = (uint8_t)(t < 18 ? fr[t] : 0);
		if (t == 18) {
			if (!m->count) { free_missile(m); return; }
			m->count--;
			spell_splash(m, m->dmg);
		}
		return; }
	case MSP_WHIRL:         /* state 12: one step every 3 cycles: splash on odd ttl, 1 px, wander +-64 px */
		if (--m->fwait) return;
		m->fwait = 3;
		if (m->ttl & 1) spell_splash(m, m->dmg);
		if (--m->ttl == 0) { free_missile(m); return; }
		if (++m->frame >= (d->frames ? d->frames : 1)) m->frame = 0;
		line_step(m, 1);
		if (--m->rem <= 0) {
			int nx = m->tx + (int)(w2_rand() & 0x7Fu) - 0x40, ny = m->ty + (int)(w2_rand() & 0x7Fu) - 0x40;
			int mw = w2.map.w * W2_TILE - 1, mh = w2.map.h * W2_TILE - 1;
			if (nx < 0) nx = 0;
			if (ny < 0) ny = 0;
			if (nx > mw) nx = mw;
			if (ny > mh) ny = mh;
			line_to(m, m->tx, m->ty, nx, ny);
		}
		return;
	default:
		free_missile(m);
		return;
	}
}

int missile_count_kind(int kind)
{
	int i, n = 0;
	for (i = 0; i < g_hi; i++) if (g_m[i].used && g_m[i].spell == kind) n++;
	return n;
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
	} else if (m->mode == DM_WALL) {
		pud_wall_hit(x >> 5, y >> 5, m->dmg);
		m->mode = DM_NONE;
	}
	if (m->wsplash && m->mode == DM_AREA) {
		/* MissileSplash: wall damage = the missile's rating on the centre tile, rating/4 on the four
		 * orthogonal neighbours (each then >> 2, at least 1, by DamageWallTile) */
		int tx = x >> 5, ty = y >> 5, q = m->dmg / 4;
		pud_wall_hit(tx, ty, m->dmg);
		pud_wall_hit(tx + 1, ty, q);
		pud_wall_hit(tx - 1, ty, q);
		pud_wall_hit(tx, ty + 1, q);
		pud_wall_hit(tx, ty - 1, q);
		m->wsplash = 0;
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
		if (m->spell) { spell_tick(m); continue; }
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
