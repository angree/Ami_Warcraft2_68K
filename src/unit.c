/* unit.c - unit life cycle, occupancy, animation engine, order dispatch, queries (see unit.h). */
#include <string.h>
#include "amiga_uclock.h"
#include "w2.h"
#include "spells.h"
#include "unit.h"
#include "move.h"

/* module-private per-slot state */
#define AF_RESTART 0x01   /* step 0 of the animation is entered at the next tick */
#define AF_DONE    0x02   /* animation wrapped / ended this cycle */
#define AF_BUSY    0x04   /* inside UNBREAK_BEGIN..END */
#define AF_PLACED  0x08   /* occupancy written */
static uint8_t a_flags[W2_MAX_UNITS];
static uint8_t a_ev[W2_MAX_UNITS];
static uint8_t a_px[W2_MAX_UNITS];

W2Corpse w2_corpses[W2_MAX_CORPSES];
static int corpse_next;

/* ------------------------------------------------------------------------------------------ helpers */

static int valid(int slot) { return slot >= 0 && slot < W2_MAX_UNITS && w2.unit[slot].flags != 0; }

static int is_directional(const W2Unit *u)
{
	const w2_unit_type *t = w2_unit(u->type);
	return t && (t->kind == W2_KIND_LAND || t->kind == W2_KIND_AIR || t->kind == W2_KIND_SEA);
}

static void count_unit(const W2Unit *u, int delta)
{
	W2Player *p;
	if (u->owner >= W2_MAX_PLAYERS) return;
	p = &w2.player[u->owner];
	if (u->flags & W2_UF_BUILDING) p->building_count = (uint16_t)(p->building_count + delta);
	else p->unit_count = (uint16_t)(p->unit_count + delta);
}

static void deselect(int slot)
{
	int i, j = 0, n = w2.select_n > W2_MAX_SELECT ? W2_MAX_SELECT : w2.select_n;
	for (i = 0; i < n; i++) if (w2.select[i] != slot) w2.select[j++] = w2.select[i];
	w2.select_n = (uint8_t)j;
	w2.unit[slot].flags &= (uint16_t)~W2_UF_SELECTED;
}

/* generation of each slot: bumped whenever a slot is freed, so a stored (slot, gen) pair
 * no longer matches a NEW unit that reuses the slot */
static uint16_t a_gen[W2_MAX_UNITS];

uint16_t unit_gen(int slot) { return (slot >= 0 && slot < W2_MAX_UNITS) ? a_gen[slot] : 0; }
int unit_same(int slot, uint16_t gen) { return slot >= 0 && slot < W2_MAX_UNITS && a_gen[slot] == gen && (w2.unit[slot].flags & W2_UF_ALIVE); }

/* generation of the order target, taken when the target is set (unit_set_order / unit_set_target);
 * a_tslot = the target value it belongs to (a direct write of u->target re-syncs on the next check) */
static uint16_t a_tslot[W2_MAX_UNITS], a_tgen[W2_MAX_UNITS];

void unit_set_target(int slot, int target)
{
	uint16_t t = (uint16_t)(target < 0 || target >= W2_MAX_UNITS ? W2_NO_UNIT : target);
	if (slot < 0 || slot >= W2_MAX_UNITS) return;
	w2.unit[slot].target = t;
	a_tslot[slot] = t;
	a_tgen[slot] = t == W2_NO_UNIT ? 0 : a_gen[t];
}

int unit_target_same(int slot)
{
	unsigned t;
	if (slot < 0 || slot >= W2_MAX_UNITS) return 0;
	t = w2.unit[slot].target;
	if (t >= W2_MAX_UNITS) return 0;
	if (a_tslot[slot] != t) { a_tslot[slot] = (uint16_t)t; a_tgen[slot] = a_gen[t]; }
	return a_gen[t] == a_tgen[slot] && (w2.unit[t].flags & W2_UF_ALIVE);
}

static void free_slot(int slot)
{
	a_gen[slot]++;
	memset(&w2.unit[slot], 0, sizeof w2.unit[slot]);
	a_flags[slot] = 0; a_ev[slot] = 0; a_px[slot] = 0;
	while (w2.unit_hi > 0 && w2.unit[w2.unit_hi - 1].flags == 0) w2.unit_hi--;
}

const struct w2_unit_type *unit_type(int slot)
{
	if (slot < 0 || slot >= W2_MAX_UNITS) return NULL;
	return w2_unit(w2.unit[slot].type);
}

static void footprint(const W2Unit *u, int *w, int *h)
{
	const w2_unit_type *t = w2_unit(u->type);
	*w = (t && t->tile_w) ? t->tile_w : 1;
	*h = (t && t->tile_h) ? t->tile_h : 1;
}

/* ------------------------------------------------------------------------------------------ occupancy */

void unit_unplace(int slot)
{
	W2Unit *u; uint16_t *occ; int w, h, x, y;
	if (!valid(slot) || !(a_flags[slot] & AF_PLACED)) return;
	u = &w2.unit[slot];
	occ = (u->flags & W2_UF_AIR) ? w2.map.occ_air : w2.map.occ_ground;
	footprint(u, &w, &h);
	for (y = u->ty; y < u->ty + h && y < w2.map.h; y++)
		for (x = u->tx; x < u->tx + w && x < w2.map.w; x++) {
			int i = y * w2.map.w + x;
			if (occ[i] != slot) continue;
			occ[i] = W2_NO_UNIT;
			if (u->flags & W2_UF_BUILDING) w2.map.flags[i] &= (uint16_t)~W2_TF_BUILDING;
		}
	a_flags[slot] &= (uint8_t)~AF_PLACED;
}

void unit_place(int slot, int tx, int ty)
{
	W2Unit *u; uint16_t *occ; int w, h, x, y;
	if (!valid(slot)) return;
	u = &w2.unit[slot];
	unit_unplace(slot);
	footprint(u, &w, &h);
	if (tx < 0) tx = 0;
	if (ty < 0) ty = 0;
	if (tx > w2.map.w - w) tx = w2.map.w - w;
	if (ty > w2.map.h - h) ty = w2.map.h - h;
	if (tx < 0 || ty < 0) return;
	u->tx = (uint8_t)tx; u->ty = (uint8_t)ty;
	if (!(u->flags & (W2_UF_ALIVE))) return;     /* dying units do not block */
	occ = (u->flags & W2_UF_AIR) ? w2.map.occ_air : w2.map.occ_ground;
	for (y = ty; y < ty + h; y++)
		for (x = tx; x < tx + w; x++) {
			int i = y * w2.map.w + x;
			if (occ[i] != W2_NO_UNIT) continue;   /* never steal another unit's tile */
			occ[i] = (uint16_t)slot;
			if (u->flags & W2_UF_BUILDING) w2.map.flags[i] |= W2_TF_BUILDING;
		}
	a_flags[slot] |= AF_PLACED;
}

/* ------------------------------------------------------------------------------------------ create / remove */

int unit_create(int type, int owner, int tx, int ty)
{
	const w2_unit_type *t = w2_unit((unsigned)type);
	W2Unit *u; int slot;
	if (!t || (t->xflags & W2UX_UNUSED) || t->kind == W2_KIND_NONE || t->kind == W2_KIND_MARKER) return -1;
	if (owner < 0 || owner >= W2_MAX_PLAYERS) owner = W2_NEUTRAL;
	if (type == W2U_GOLD_MINE || type == W2U_OIL_PATCH) owner = W2_NEUTRAL;
	if (tx < 0 || ty < 0 || tx + t->tile_w > w2.map.w || ty + t->tile_h > w2.map.h) return -1;
	/* a building takes its whole footprint or nothing: any tile held by another unit/building -> fail
	 * (a builder going inside has already been unplaced by economy.c; air units use occ_air) */
	if (t->kind == W2_KIND_BUILDING) {
		int x, y, w = t->tile_w ? t->tile_w : 1, h = t->tile_h ? t->tile_h : 1;
		for (y = ty; y < ty + h; y++)
			for (x = tx; x < tx + w; x++)
				if (w2.map.occ_ground[y * w2.map.w + x] != W2_NO_UNIT) return -1;
	}
	/* a freshly reset world (pud_load_mem memsets the units): drop the corpses of the old one */
	if (w2.unit_hi == 0 && w2.cycle == 0) unit_corpses_clear();
	for (slot = 0; slot < W2_MAX_UNITS; slot++) if (w2.unit[slot].flags == 0) break;
	if (slot >= W2_MAX_UNITS) return -1;
	u = &w2.unit[slot];
	memset(u, 0, sizeof *u);
	a_flags[slot] = 0; a_ev[slot] = 0; a_px[slot] = 0;
	u->type = (uint8_t)type;
	u->owner = (uint8_t)owner;
	u->flags = W2_UF_ALIVE;
	if (t->kind == W2_KIND_BUILDING) u->flags |= W2_UF_BUILDING;
	else if (t->kind == W2_KIND_AIR) u->flags |= W2_UF_AIR;
	else if (t->kind == W2_KIND_SEA) u->flags |= W2_UF_SEA;
	u->hp = t->hp;
	u->mana = (uint8_t)((t->xflags & W2UX_MANA) ? W2_MANA_START : 0);
	u->dir = (uint8_t)((u->flags & W2_UF_BUILDING) ? 0 : (w2_rand() & 7));
	u->order = W2_ORD_STILL;
	u->target = W2_NO_UNIT;
	u->tx = (uint8_t)tx; u->ty = (uint8_t)ty;
	if (slot >= w2.unit_hi) w2.unit_hi = (uint16_t)(slot + 1);
	unit_set_anim(slot, W2_ANIM_STILL);
	unit_place(slot, tx, ty);
	count_unit(u, 1);
	econ_recount(owner);
	return slot;
}

void unit_remove(int slot)
{
	W2Unit *u; int owner, alive;
	if (!valid(slot)) return;
	u = &w2.unit[slot];
	owner = u->owner; alive = (u->flags & W2_UF_ALIVE) != 0;
	move_release(slot);
	unit_unplace(slot);
	deselect(slot);
	if (alive) count_unit(u, -1);
	free_slot(slot);
	if (alive) econ_recount(owner);
}

/* ------------------------------------------------------------------------------------------ animation */

static const w2_anim_step *cur_steps(const W2Unit *u, unsigned *n)
{
	return w2_unit_anim(u->type, u->anim_kind, n);
}

static int falls_back_to_still(const W2Unit *u)
{
	int k = u->anim_kind;
	if (u->flags & W2_UF_BUILDING) return 0;
	if (k != W2_ANIM_MOVE && k != W2_ANIM_ATTACK && k != W2_ANIM_HARVEST && k != W2_ANIM_REPAIR && k != W2_ANIM_SPELLCAST) return 0;
	return u->order == W2_ORD_STILL || u->order == W2_ORD_STAND || u->order == W2_ORD_NONE;
}

static void play_step_sound(int slot, int id)
{
	int px, py;
	if (!unit_visible_local(slot)) return;
	unit_center(slot, &px, &py);
	snd_play(id, px, py);
}

/* enter step idx (chaining zero-wait steps) */
static void anim_enter(int slot, unsigned idx)
{
	W2Unit *u = &w2.unit[slot];
	unsigned n, guard;
	const w2_anim_step *st = cur_steps(u, &n);
	if (!st || !n) { u->anim_wait = 0; return; }
	for (guard = 0; guard < 2u * n + 2u; guard++) {
		const w2_anim_step *s;
		if (idx >= n) {
			a_flags[slot] |= AF_DONE;
			if (u->anim_kind == W2_ANIM_DEATH) { u->anim_step = (uint8_t)(n - 1); u->anim_wait = 0; return; }
			idx = 0;
			if (falls_back_to_still(u)) {
				const w2_anim_step *s2; unsigned n2;
				s2 = w2_unit_anim(u->type, W2_ANIM_STILL, &n2);
				if (s2 && n2) { u->anim_kind = W2_ANIM_STILL; st = s2; n = n2; a_flags[slot] &= (uint8_t)~AF_BUSY; }
			}
		}
		s = &st[idx];
		u->anim_step = (uint8_t)idx;
		u->frame = s->frame;
		a_ev[slot] |= s->flags;
		if (s->flags & W2AF_UNBREAK_BEGIN) a_flags[slot] |= AF_BUSY;
		if (s->flags & W2AF_UNBREAK_END) a_flags[slot] &= (uint8_t)~AF_BUSY;
		if (s->flags & W2AF_SOUND) play_step_sound(slot, s->arg);
		else if (s->flags & W2AF_IDLE_TURN) {
			if (!(u->flags & W2_UF_BUILDING) && s->arg && w2_rand() % 100u == 0) {
				int turn = (w2_rand() & 1) ? s->arg : 8 - s->arg;
				u->dir = (uint8_t)((u->dir + turn) & 7);
			}
		} else if (u->anim_kind == W2_ANIM_MOVE) {
			int px = a_px[slot] + s->arg;
			a_px[slot] = (uint8_t)(px > 255 ? 255 : px);
		}
		if (s->wait) { u->anim_wait = s->wait; return; }
		idx++;
	}
	u->anim_wait = 1;   /* only zero-wait steps: hold one cycle */
}

void unit_set_anim(int slot, int kind)
{
	W2Unit *u; unsigned n; const w2_anim_step *st;
	if (!valid(slot)) return;
	u = &w2.unit[slot];
	if (kind < 0 || kind >= W2_NUM_ANIM_KINDS) kind = W2_ANIM_STILL;
	st = w2_unit_anim(u->type, (unsigned)kind, &n);
	if ((!st || !n) && kind != W2_ANIM_STILL) { kind = W2_ANIM_STILL; st = w2_unit_anim(u->type, W2_ANIM_STILL, &n); }
	u->anim_kind = (uint8_t)kind;
	u->anim_step = 0;
	u->anim_wait = 0;
	a_flags[slot] &= (uint8_t)~(AF_BUSY | AF_DONE);
	if (st && n) { u->frame = st[0].frame; a_flags[slot] |= AF_RESTART; }
	else { u->frame = 0; a_flags[slot] &= (uint8_t)~AF_RESTART; }
}

static void anim_advance(int slot)
{
	W2Unit *u = &w2.unit[slot];
	if (a_flags[slot] & AF_RESTART) { a_flags[slot] &= (uint8_t)~AF_RESTART; anim_enter(slot, 0); return; }
	if (u->anim_wait == 0) return;              /* ended (DEATH) or no animation */
	if (u->anim_wait > 1) { u->anim_wait--; return; }
	anim_enter(slot, (unsigned)u->anim_step + 1u);
}

int unit_anim_event(int slot) { return (slot >= 0 && slot < W2_MAX_UNITS) ? a_ev[slot] : 0; }
int unit_anim_move_pixels(int slot) { return (slot >= 0 && slot < W2_MAX_UNITS) ? a_px[slot] : 0; }
int unit_anim_busy(int slot) { return (slot >= 0 && slot < W2_MAX_UNITS) ? (a_flags[slot] & AF_BUSY) != 0 : 0; }
int unit_anim_done(int slot) { return (slot >= 0 && slot < W2_MAX_UNITS) ? (a_flags[slot] & AF_DONE) != 0 : 0; }

int unit_sprite_frame(int slot, int *mirror)
{
	const W2Unit *u = &w2.unit[slot];
	if (mirror) *mirror = 0;
	if (!is_directional(u)) return u->frame;
	return (int)w2_dir_frame(u->frame, u->dir, mirror);
}

/* ------------------------------------------------------------------------------------------ corpses */

static unsigned corpse_set(int kind)
{
	switch (kind) {
	case W2_CORPSE_HUMAN_DEAD_BODY: return W2A_HUMAN_DEAD_BODY;
	case W2_CORPSE_ORC_DEAD_BODY: return W2A_ORC_DEAD_BODY;
	case W2_CORPSE_DEAD_SEA_BODY: return W2A_DEAD_SEA_BODY;
	case W2_CORPSE_DESTROYED_3X3_PLACE_WATER: return W2A_DESTROYED_PLACE_WATER;
	default: return W2A_DESTROYED_PLACE;
	}
}

static void corpse_enter(W2Corpse *c, unsigned idx)
{
	unsigned n, guard; const w2_anim_step *st = w2_anim(c->set, W2_ANIM_DEATH, &n);
	if (!st || !n) { c->used = 0; return; }
	for (guard = 0; guard <= n; guard++) {
		if (idx >= n) { c->used = 0; return; }
		c->step = (uint8_t)idx; c->frame = st[idx].frame;
		if (st[idx].flags & W2AF_DIE) { c->used = 0; return; }   /* decayed */
		if (st[idx].wait) { c->wait = st[idx].wait; return; }
		idx++;
	}
	c->used = 0;
}

int unit_corpse_add(int kind, int owner, int tx, int ty, int tw, int th, int ox, int oy, int dir)
{
	int i, k; W2Corpse *c;
	if (kind <= W2_CORPSE_NONE || kind > W2_CORPSE_DESTROYED_4X4_PLACE) return -1;
	for (i = 0, k = -1; i < W2_MAX_CORPSES; i++) if (!w2_corpses[i].used) { k = i; break; }
	if (k < 0) { k = corpse_next; corpse_next = (corpse_next + 1) % W2_MAX_CORPSES; }   /* recycle */
	c = &w2_corpses[k];
	memset(c, 0, sizeof *c);
	c->used = 1; c->kind = (uint8_t)kind; c->owner = (uint8_t)owner; c->dir = (uint8_t)(dir & 7);
	c->tx = (uint8_t)tx; c->ty = (uint8_t)ty; c->tw = (uint8_t)tw; c->th = (uint8_t)th;
	c->ox = (int8_t)ox; c->oy = (int8_t)oy;
	c->set = (uint8_t)corpse_set(kind);
	corpse_enter(c, 0);
	return c->used ? k : -1;
}

int unit_corpse_at(int tx, int ty)
{
	int i;
	for (i = 0; i < W2_MAX_CORPSES; i++) {
		const W2Corpse *c = &w2_corpses[i];
		if (c->used && c->kind <= W2_CORPSE_DEAD_SEA_BODY && c->tx == tx && c->ty == ty) return i;
	}
	return -1;
}

void unit_corpse_remove(int idx) { if (idx >= 0 && idx < W2_MAX_CORPSES) w2_corpses[idx].used = 0; }
void unit_corpses_clear(void) { memset(w2_corpses, 0, sizeof w2_corpses); corpse_next = 0; }

static void corpses_tick(void)
{
	int i;
	for (i = 0; i < W2_MAX_CORPSES; i++) {
		W2Corpse *c = &w2_corpses[i];
		if (!c->used) continue;
		if (c->wait > 1) { c->wait--; continue; }
		corpse_enter(c, (unsigned)c->step + 1u);
	}
}

/* ------------------------------------------------------------------------------------------ death */

void unit_kill(int slot)
{
	if (!valid(slot) || !(w2.unit[slot].flags & W2_UF_ALIVE)) return;
	w2.unit[slot].order = W2_ORD_DIE;
	w2.unit[slot].order_state = 0;
}

static void finish_death(int slot)
{
	W2Unit *u = &w2.unit[slot];
	const w2_unit_type *t = w2_unit(u->type);
	if (t && t->corpse && !(u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN))) {
		int w, h;
		footprint(u, &w, &h);
		unit_corpse_add(t->corpse, u->owner, u->tx, u->ty, w, h, u->ox, u->oy, u->dir);
	}
	unit_unplace(slot);
	free_slot(slot);
}

static void begin_death(int slot)
{
	W2Unit *u = &w2.unit[slot];
	unsigned n; const w2_anim_step *st;
	int owner = u->owner;
	move_release(slot);
	unit_unplace(slot);
	deselect(slot);
	count_unit(u, -1);
	u->flags = (uint16_t)((u->flags & ~(W2_UF_ALIVE | W2_UF_SELECTED)) | W2_UF_DYING);
	{   /* a destroyed oil platform gives its oil patch back with the oil left (Wargus ReplaceOnDie) */
		const w2_unit_type *pt = w2_unit(u->type);
		if (pt && (pt->flags & W2UF_OIL_PLATFORM) && !(u->flags & W2_UF_INSIDE)) {
			uint32_t left = u->res_left;
			int p = unit_create(W2U_OIL_PATCH, W2_NEUTRAL, u->tx, u->ty);
			if (p >= 0) { w2.unit[p].res_left = left; w2.unit[p].res_amount = (uint16_t)(left / 100u); }
		}
	}
	u->order = W2_ORD_DIE; u->order_state = 0; u->target = W2_NO_UNIT;
	u->hp = 0;
	econ_recount(owner);
	st = w2_unit_anim(u->type, W2_ANIM_DEATH, &n);
	if (st && n && !(u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN))) {
		u->anim_kind = W2_ANIM_DEATH; u->anim_step = 0; u->anim_wait = 0; u->frame = st[0].frame;
		a_flags[slot] = (uint8_t)((a_flags[slot] & AF_PLACED) | AF_RESTART);
	} else {
		u->anim_kind = W2_ANIM_DEATH; u->anim_wait = 0;
		a_flags[slot] = (uint8_t)((a_flags[slot] & AF_PLACED) | AF_DONE);   /* finishes this cycle */
	}
	ord_die(slot);   /* hook: sound, explosion, score (must not free the slot; if it does, fine) */
}

/* ------------------------------------------------------------------------------------------ tick */

static void dispatch(int slot)
{
	switch (w2.unit[slot].order) {
	case W2_ORD_STILL:         if (unit_thinks(slot)) ord_still(slot); break;
	case W2_ORD_STAND:         ord_stand(slot); break;
	case W2_ORD_MOVE:          ord_move(slot); break;
	case W2_ORD_ATTACK:        ord_attack(slot); break;
	case W2_ORD_ATTACK_GROUND: ord_attack_ground(slot); break;
	case W2_ORD_ATTACK_MOVE:   ord_attack_move(slot); break;
	case W2_ORD_PATROL:        ord_patrol(slot); break;
	case W2_ORD_FOLLOW:        ord_follow(slot); break;
	case W2_ORD_HARVEST:       ord_harvest(slot); break;
	case W2_ORD_RETURN:        ord_return(slot); break;
	case W2_ORD_BUILD:         ord_build(slot); break;
	case W2_ORD_BUILT:         ord_built(slot); break;
	case W2_ORD_TRAIN:         ord_train(slot); break;
	case W2_ORD_RESEARCH:      ord_research(slot); break;
	case W2_ORD_UPGRADE_TO:    ord_upgrade_to(slot); break;
	case W2_ORD_REPAIR:        ord_repair(slot); break;
	case W2_ORD_LOAD:          ord_load(slot); break;
	case W2_ORD_UNLOAD:        ord_unload(slot); break;
	case W2_ORD_CAST:          ord_cast(slot); break;
	case W2_ORD_DEMOLISH:      ord_demolish(slot); break;
	default: break;
	}
}

void unit_tick_all(void)
{
	int slot, m30 = (int)(w2.cycle % 30u), m60 = (int)(w2.cycle % 60u);
	corpses_tick();
	for (slot = 0; slot < w2.unit_hi; slot++, m30 = (m30 == 29 ? 0 : m30 + 1), m60 = (m60 == 59 ? 0 : m60 + 1)) {
		W2Unit *u = &w2.unit[slot];
		const w2_unit_type *t;
		if (!u->flags) continue;
		t = w2_unit(u->type);
		a_ev[slot] = 0; a_px[slot] = 0;
		a_flags[slot] &= (uint8_t)~AF_DONE;
		if (u->flags & W2_UF_ALIVE) {
			if (u->order == W2_ORD_DIE ||
			    (u->hp == 0 && t && t->hp > 0 && !(t->xflags & W2UX_INDESTRUCTIBLE))) {
				begin_death(slot);
				if (!u->flags) continue;       /* the hook removed it */
			}
		}
		anim_advance(slot);
		if (u->flags & W2_UF_DYING) {
			if ((a_ev[slot] & W2AF_DIE) || (a_flags[slot] & AF_DONE) || u->anim_wait == 0) finish_death(slot);
			continue;
		}
		if (!(u->flags & W2_UF_ALIVE)) continue;
		/* rates: phases spread over the slots so not every unit ticks in the same cycle */
		if (m60 == 0 && t && u->hp < t->hp && u->hp > 0 && econ_stat(slot, W2_STAT_REGEN) > 0) u->hp++;
		if (u->ttl && --u->ttl == 0) { unit_kill(slot); continue; }
		dispatch(slot);
		if (u->flags & W2_UF_ALIVE) move_tick(slot);   /* finishes a step whatever the order (idempotent) */
	}
}

/* ------------------------------------------------------------------------------------------ orders */

void unit_set_order(int slot, int order, int target, int tx, int ty)
{
	W2Unit *u;
	if (!valid(slot)) return;
	u = &w2.unit[slot];
	if (!(u->flags & W2_UF_ALIVE)) return;   /* dying: nothing more to do */
	u->order = (uint8_t)order;
	u->order_state = 0;
	unit_set_target(slot, target);
	u->goal_tx = (uint8_t)(tx < 0 ? 0 : tx);
	u->goal_ty = (uint8_t)(ty < 0 ? 0 : ty);
	u->timer = 0;
}

/* 8-way heading from a vector, no floats: tan(22.5) ~ 5/12 */
static int dir_from(int dx, int dy)
{
	int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
	if (ax * 12 <= ay * 5) return dy < 0 ? 0 : 4;          /* mostly vertical */
	if (ay * 12 <= ax * 5) return dx > 0 ? 2 : 6;          /* mostly horizontal */
	if (dx > 0) return dy < 0 ? 1 : 3;
	return dy < 0 ? 7 : 5;
}

void unit_set_dir_to(int slot, int px, int py)
{
	int cx, cy;
	if (!valid(slot) || (w2.unit[slot].flags & W2_UF_BUILDING)) return;
	unit_center(slot, &cx, &cy);
	if (px == cx && py == cy) return;
	w2.unit[slot].dir = (uint8_t)dir_from(px - cx, py - cy);
}

/* ------------------------------------------------------------------------------------------ queries */

void unit_center(int slot, int *px, int *py)
{
	const W2Unit *u = &w2.unit[slot]; int w, h;
	footprint(u, &w, &h);
	*px = u->tx * W2_TILE + u->ox + w * (W2_TILE / 2);
	*py = u->ty * W2_TILE + u->oy + h * (W2_TILE / 2);
}

void unit_box(int slot, int *x, int *y, int *w, int *h)
{
	const w2_unit_type *t = unit_type(slot);
	int cx, cy, bw = t && t->frame_w ? t->frame_w : 32, bh = t && t->frame_h ? t->frame_h : 32;
	unit_center(slot, &cx, &cy);
	*x = cx - bw / 2; *y = cy - bh / 2; *w = bw; *h = bh;
}

/* selection box (UDTA box size) centred on the unit */
static void sel_box(int slot, int *x0, int *y0, int *x1, int *y1)
{
	const w2_unit_type *t = unit_type(slot);
	int cx, cy, bw = t && t->box_w ? t->box_w : 31, bh = t && t->box_h ? t->box_h : 31;
	unit_center(slot, &cx, &cy);
	*x0 = cx - bw / 2; *y0 = cy - bh / 2; *x1 = *x0 + bw; *y1 = *y0 + bh;
}

int unit_visible_local(int slot)
{
	const W2Unit *u; int w, h, x, y, bld;
	if (!valid(slot)) return 0;
	u = &w2.unit[slot];
	if (u->owner == w2.local_player) return 1;
	if (spells_hidden_from(slot, w2.local_player)) return 0;   /* invisible enemy */
	footprint(u, &w, &h);
	bld = (u->flags & W2_UF_BUILDING) != 0;
	for (y = u->ty; y < u->ty + h; y++)
		for (x = u->tx; x < u->tx + w; x++)
			if (bld ? fog_explored(x, y) : fog_visible(x, y)) return 1;
	return 0;
}

int unit_at_pixel(int px, int py)
{
	int slot, best = -1, best_key = -0x7FFFFFFF;
	for (slot = 0; slot < w2.unit_hi; slot++) {
		const W2Unit *u = &w2.unit[slot];
		int x0, y0, x1, y1, key;
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN))) continue;
		sel_box(slot, &x0, &y0, &x1, &y1);
		if (px < x0 || px > x1 || py < y0 || py > y1) continue;
		if (!unit_visible_local(slot)) continue;
		/* topmost as drawn: flyers above everything, then units above buildings, then lower on screen */
		key = ((u->flags & W2_UF_AIR) ? 0x400000 : 0) + ((u->flags & W2_UF_BUILDING) ? 0 : 0x200000) + y1;
		if (key > best_key) { best_key = key; best = slot; }
	}
	return best;
}

int units_in_rect(int x0, int y0, int x1, int y1, uint16_t *out, int max)
{
	int slot, n = 0, t;
	if (x0 > x1) { t = x0; x0 = x1; x1 = t; }
	if (y0 > y1) { t = y0; y0 = y1; y1 = t; }
	for (slot = 0; slot < w2.unit_hi && n < max; slot++) {
		const W2Unit *u = &w2.unit[slot];
		int bx0, by0, bx1, by1;
		if (!(u->flags & W2_UF_ALIVE) || (u->flags & (W2_UF_INSIDE | W2_UF_HIDDEN))) continue;
		sel_box(slot, &bx0, &by0, &bx1, &by1);
		if (bx1 < x0 || bx0 > x1 || by1 < y0 || by0 > y1) continue;
		out[n++] = (uint16_t)slot;
	}
	return n;
}

static int gap(int a0, int alen, int b0, int blen)
{
	if (b0 >= a0 + alen) return b0 - (a0 + alen - 1);
	if (a0 >= b0 + blen) return a0 - (b0 + blen - 1);
	return 0;
}

int unit_dist_tiles(int a, int b)
{
	const W2Unit *ua = &w2.unit[a], *ub = &w2.unit[b];
	int aw, ah, bw, bh, dx, dy;
	footprint(ua, &aw, &ah); footprint(ub, &bw, &bh);
	dx = gap(ua->tx, aw, ub->tx, bw); dy = gap(ua->ty, ah, ub->ty, bh);
	return dx > dy ? dx : dy;
}

int unit_dist_to_tile(int a, int tx, int ty)
{
	const W2Unit *ua = &w2.unit[a];
	int aw, ah, dx, dy;
	footprint(ua, &aw, &ah);
	dx = gap(ua->tx, aw, tx, 1); dy = gap(ua->ty, ah, ty, 1);
	return dx > dy ? dx : dy;
}

int unit_is_enemy(int a, int b)
{
	int pa = w2.unit[a].owner, pb = w2.unit[b].owner;
	if (pa == pb || pa >= W2_MAX_PLAYERS || pb >= W2_MAX_PLAYERS) return 0;
	return (w2.player[pa].enemies >> pb) & 1;
}

int unit_owner_sees(int player, int slot)
{
	const W2Unit *u; int w, h, x, y;
	if (!valid(slot)) return 0;
	u = &w2.unit[slot];
	if (u->owner == player) return 1;
	if (player < 0 || player >= 8) return 0;
	footprint(u, &w, &h);
	for (y = u->ty; y < u->ty + h && y < w2.map.h; y++)
		for (x = u->tx; x < u->tx + w && x < w2.map.w; x++) {
			if (player == w2.local_player) { if (fog_visible(x, y)) return 1; }
			else if (w2.map.visible[y * w2.map.w + x] & (1u << player)) return 1;
		}
	return 0;
}
