# SPEC_MECHANICS — game rules of the original (Warcraft II Mac 1.23, 68k)

Behaviour specification written from the decompiled Mac binary (outside the repo,
`D:\GITHUB\_AmiW2-poza-repo\re68k\w2_all.c`). Prose and constant tables only; addresses are image
addresses as in `docs/RE_NOTES.md` (image base 0x00100000, A5 = 0x00210000, globals written
`A5+xxxx`). This file **extends** RE_NOTES: the main loop, cycle timing per speed, RNG, damage
formula, splash, pathfinding and animation interpreter are documented there and only referenced here.

Confidence tags: **CONFIRMED** = read directly in the code; **LIKELY** = code read, but the meaning of a
field/caller is inferred; **UNKNOWN** = not found. "Wargus≠" marks a contradiction with Wargus/Stratagus
(or with what `docs/SPEC_RULES.md` took from it).

All times are in **game cycles** (one call of `Game_SimCycle` 0x001389DA). At the default speed 5 the
original runs 37.5 cycles/s (RE_NOTES §3).

---

## 0. Vocabulary: unit record fields used below

Additions to the RE_NOTES §7 table. Field names in quotes come from the original's own sync-debug dump
(0x00133C9A, strings "attacker", "bldOrder", ...).

| off | meaning | conf. |
|---|---|---|
| +0x1C bit 0x10 | building is producing (train/research/upgrade in progress) | CONFIRMED (0x0012438C) |
| +0x1C bit 0x20 | production cancel requested | CONFIRMED (0x001242C2) |
| +0x1E bit 0x02 | dying / dead | CONFIRMED (0x0016C29C) |
| +0x1E bit 0x08 | hidden: inside a building/mine/transport, or removed | CONFIRMED (0x0013BB80) |
| +0x1E bit 0x80 | building is **complete** (counts for food, bonuses, prerequisites) | CONFIRMED (0x0016B866, 0x001314CE) |
| +0x1E bit 0x100 | building under construction | CONFIRMED (0x0016C6CA) |
| +0x26 | mana (casters) / production progress 0..255 (buildings) / remaining lifetime (decaying units) | CONFIRMED |
| +0x2F | queued next order (0x3C = none) | CONFIRMED (0x0016D1B6, 0x0016D38A) |
| +0x54 | "attacker": last enemy that hit this unit (retaliation memory) | CONFIRMED (0x00120E86) |
| +0x6C..+0x72 | buildings: "bldOrder" (kind 0 train, 1 research, 2 upgrade level, 3 hall upgrade), "bldParm" (what), "bldCurr" progress, "bldTotal" target, "bldAIparm" | CONFIRMED (0x00133C9A, 0x0012438C) |
| +0x6C/+0x70 | workers: remembered resource tile / mine for the next trip | CONFIRMED (0x0013BC28) |
| +0x74 | workers: chop counter; casters: mana regen counter; decaying units: decay counter | CONFIRMED |
| +0x75 bits | worker cargo: 0x80 gold job, 0x40 lumber job, 0x20 carrying a load, 0x08 return to remembered resource, 0x04 delivered at a hall | CONFIRMED (0x0013B8AC) |
| +0x7A | attack cooldown counter (decremented each cycle) | LIKELY (0x0016CEB8, 0x001215A4) |
| +0x81 | gold mine / oil platform: number of workers inside | CONFIRMED (0x0013BC28, 0x0013BA1E) |
| +0x82 | gold mine / oil patch / platform: remaining **loads** (trips) | CONFIRMED (0x0013BC28, PUD unit loader 0x00143EDE) |
| +0x84 / +0x88 / +0x8A | buildings under construction: elapsed cycles / HP accumulator / HP increment | CONFIRMED (0x0016B866, 0x0016C6CA) |

Per-player counters (16 × u16 arrays, cleared by 0x001316C4): `A5+2628[type]` gives, per unit type,
the address of the count array for that type (only **complete** units are counted, 0x001314CE /
0x001315A4). Examples: town hall A5+61EE, keep A5+620E, castle A5+622E, lumber mill A5+62AE,
refinery A5+63CE, farm A5+630E, skeleton/daemon/critter A5+664E, tankers A5+662E, transports
A5+652E, flying machine/zeppelin A5+65AE. `A5+646E[p]` = number of non-building units,
`A5+61AE[p]` = number of buildings. Player stock: gold `A5+6C4A[p]`, lumber `A5+6C0A[p]`, oil
`A5+6C8A[p]` (s32). `A5+6D4A[p]` = controller: 0 human, 1 computer, 3 passive/rescuable (LIKELY).

Unit flag bits (UDTA flags `A5+519E`, consistent with all 110 types in `re68k\unit_types.csv`):
0x1 land, 0x2 air, 0x4 explodes on death, 0x8 naval, 0x10 critter, 0x20 building, 0x40 submarine,
0x80 sees submarines, 0x100 worker, 0x200 tanker, 0x400 transport, 0x800 oil platform, 0x1000 stores
gold (hall tiers), 0x2000 corpse/vanishing, 0x4000 ground attack, 0x8000 undead, 0x10000 shore
building, 0x20000 caster, 0x40000 stores lumber, 0x80000 can attack, 0x100000 tower, 0x200000 oil
patch, 0x400000 gold mine, 0x800000 hero, 0x1000000 stores oil, 0x8000000 organic. (LIKELY for the
names; the bit tests below are CONFIRMED.)

---

## 1. Orders (unit+0x2E) and the order loop

Per cycle each live unit steps its animation; **order logic runs only when the animation yields**
(opcode 00, see RE_NOTES §8), via the action table `A5+1F7C[order]` (0x0016CEB8). Order→animation
kind table `A5+1D56`, order flag words `A5+1C9E`, attack-event hook `A5+1D94`. A new order is queued
in +0x2F (0x0016D38A) and taken over at the next yield (0x0016D1B6, which also sets the anim wait to
3 cycles). CONFIRMED.

| order | handler | meaning | conf. |
|---|---|---|---|
| 0 | 0x00132416 | free/none | LIKELY |
| 1 | 0x0013246A | die (corpse / rubble, §9) | CONFIRMED |
| 2 | 0x001215A4 | idle / stand (auto-acquire, fidget) | CONFIRMED |
| 9, 11–17 | 0x001215A4 | attack-type orders (attack, attack-move, patrol, stand ground ...; individual mapping UNKNOWN) | LIKELY |
| 14 | 0x00121540 | tower firing (missile launched directly from the order handler) | LIKELY |
| 20, 21 | 0x00121468 | demolition (dwarves/sappers explode: damage to the 8 neighbouring tiles, then die) | LIKELY |
| 22 | 0x00132AB4 | hold (forgets attacker, anim wait 255) | LIKELY |
| 23 | 0x0013B7A2 | harvest (go to mine / chop tree) | CONFIRMED |
| 24 | 0x0013B820 | return goods to depot | CONFIRMED |
| 25 | 0x0013BC28 | enter mine/platform/depot with goods | CONFIRMED |
| 26 | 0x0013BA1E | leave building, continue the harvest cycle | CONFIRMED |
| 27 | 0x00132CC8 | repair | CONFIRMED |
| 28 | 0x00132BFE | build (walk to site, place) | CONFIRMED |
| 29 | 0x00132A4C | unload transport | LIKELY |
| 33 | 0x0013272E | building idle (computer production hook) | LIKELY |
| 37 | 0x001242C2 | building production tick (train / research / upgrade) | CONFIRMED |
| 38–56 | 0x001604D4 / 0x001604F8 / 0x00160542 + spell routines 0x00160BB6.. | spell casts | CONFIRMED (structure) |
| 59 | 0x00132624 | circle of power (rescue check, every 50 cycles) | LIKELY |

---

## 2. Combat

### 2.1 Damage, splash, walls
See RE_NOTES §5 (CONFIRMED there). Summary: `d = max(basic'−armor',0) + piercing'`, `dmg = h +
Random()%(h+1)` with `h=(d+1)>>1`; d = 0 deals nothing. **Wargus≠** (SPEC_RULES §5 uses
max(…,1) and a 50–100 % roll).

### 2.2 Attack rate / cooldown
- The attack rate **is** the attack animation loop; the hit happens on anim opcode 0x0B
  (`AnimAttackEvent` 0x001201CC → `Order_AttackHit` 0x0012016A). CONFIRMED.
- In addition a per-unit cooldown +0x7A is loaded from a per-type table `A5+59EC[type]` (filled at
  load time from the animation data by 0x0015A2C6) when an attack starts, and decremented every cycle
  (0x0016CEB8). While it is larger than the current anim wait, a ground unit idles instead of starting a
  new swing; **air units skip this check** (0x001215A4). LIKELY (exact table meaning).
- Attacking clears invisibility (+0x44 := 0, 0x001201CC). CONFIRMED.
- Towers (order 14) fire straight from the order handler at the best target (0x00121540). LIKELY.

### 2.3 Who can hit whom (0x0012112C)
| target | attacker needs |
|---|---|
| flying (+0x1C bit 4) | can_target bit 4 (air) |
| attacker is a submarine (flag 0x40) | target must be naval, an oil platform or a shore building (flags 0x10808) — ignores `can_target` |
| transport standing on a water tile | can_target bit 1 or 2 |
| naval mover (+0x1C bit 8) | can_target bit 2 (sea) |
| anything else | can_target bit 1 (land) |
CONFIRMED.

### 2.4 Target acquisition (0x0012062C, scoring 0x00120476)
- **Search box** (Chebyshev, around the unit's tile): computer-owned unit → `react_range_computer`
  (A5+4A84); human unit with an order whose flag word has bit 4 and not bit 0x400 → `react_range_human`
  (A5+4AF2); otherwise the unit's own attack range (`f13_5576`). Towers get +1 on the far edges.
  Uses a y-sorted unit list (`A5+560E`, count `A5+560A`, binary search 0x0012022E). CONFIRMED.
- **Minimum range**: missile `A5+294C[missile]` − 1 tiles; closer candidates score 0 (ballista,
  catapult, cannon tower cannot hit adjacent units). CONFIRMED.
- **Candidate filter**: enemy per diplomacy matrix `A5+6F3A[me*16+them]`, alive, not invisible to me
  (target+0x28 player bit), passes 0x00170F58 (submarine visibility, LIKELY), passes §2.3. An air
  attacker ignores air targets that cannot attack. CONFIRMED.
- **Score** (highest wins, ties → first in list): `priority(A5+4C3C) − dist²/4` (min 1), plus a band
  bonus if the target can attack (flag 0x80000): +0x10000 if it is outside my attack range, +0x20000 if
  inside, +0x30000 if inside and it is an air unit or submarine. So *armed enemies in range always win*,
  then armed out of range, then unarmed by priority. The remembered attacker (+0x54) is scored first and
  must be beaten strictly. CONFIRMED. **Wargus≠** (Wargus: plain priority/cost heuristics).
- Mage/death knight (types 10/11) auto-attack only unarmed or worker-class targets with their basic
  attack (0x00120D14). LIKELY.
- An invisible unit (+0x44 ≠ 0) does not auto-acquire. CONFIRMED (0x00120D14).

### 2.5 Retaliation and calls for help
- When hit (0x00120E86, called from `DamageUnit` 0x00132148): if the victim has no remembered attacker
  and the attacker is an enemy, store it in +0x54. **Human-owned victims ignore hits from buildings**
  (towers). CONFIRMED.
- **Call for help** (0x00120A12): owner's units within a box around the victim's footprint — computer
  owner **13 tiles**, human owner **2 tiles (4 if the victim is naval)** — that are idle (order 2 or 7,
  nothing queued), can attack, are not workers/tankers/towers, have no attacker memory and can target
  the attacker, get an attack order on it. A computer victim attacked by a submarine first triggers an
  AI routine (0x001412DA). CONFIRMED.
- **Workers / tankers under attack** (0x00120930): a worker hit by a worker fights back; otherwise a
  human-owned worker that is idle, harvesting without a target, or repairing **flees** to a random tile
  ≈4 tiles away in the attacker's facing direction (`(Random&7)−2` jitter per axis, 0x00120832).
  Computer-owned workers flee only when idle. Passive players (`A5+6D4A`=3) never retaliate. CONFIRMED.

### 2.6 Spells (mana and effects)
Mana cost per order, u16 table at A5−0x739C (`0x00208C64[order]`), deducted when the cast happens
(0x00160596); insufficient mana cancels the order. CONFIRMED values; spell→order mapping LIKELY:

| order | 38 | 39 | 40 | 41 | 42 | 43 | 44 | 45 | 46 | 47 | 48 | 49 | 50 | 51 | 52 | 53 | 54 | 55 | 56 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| mana | 70 | 6 | 5 | 4 | 80 | 100 | 50 | 200 | 200 | 25 | 70 | 50 | 50 | 100 | 100 | 50 | 100 | 200 | 25 |

Likely: 38 holy vision, 39 healing, 41 exorcism, 42 flame shield, 43 fireball, 44 slow, 45
invisibility, 46 polymorph, 47 blizzard, 48 eye of kilrogg, 49 bloodlust, 50 raise dead, 51 death
coil, 52 whirlwind, 53 haste, 54 unholy armor, 55 runes, 56 death and decay (order 40 cost 5 unknown).

Effects found (CONFIRMED unless noted; timers decrement once per cycle in 0x0016BBFA):
- Bloodlust: +0x48 = 1000 cycles (doubles basic and piercing, RE_NOTES §5).
- Haste: +0x4A = 1000; if the target was slowed, +1000 is added instead (cancels slow). Slow: +0x4A =
  −1000 (or −1000 more). Haste/slow change anim waits (RE_NOTES §8).
- Invisibility: +0x44 = 2000. Unholy armor: +0x46 = 500 and HP halved (if > 1). Dwarves/sappers that
  become invisible or unholy die immediately (0x0016BBFA). Unholy armor makes `DamageUnit` ignore hits.
- Flame shield (LIKELY): +0x4E = 500 on a non-air target plus the orbiting missile (0x00160BB6).
- Polymorph: target removed, a critter (type 0x39) placed on its tile.
- Raise dead: every dead body (type 0x69) within distance² < 37 of the target tile becomes a skeleton,
  each costing the spell's mana again while mana lasts (0x00160E5E).
- Death coil (0x00160F42): 5×5 tiles around the target, victims sorted by HP ascending; a 50-HP budget
  is spent killing/damaging them in that order; the caster heals by the amount dealt (capped at max HP).
- Blizzard / death and decay: 5 random area missiles per cast step (0x00160D62).
- Runes: up to 50 rune slots (`A5+716E` x, `A5+71A0` y, `A5+71D2` timer); a unit entering a rune tile
  takes 50 damage (unless unholy armored) and the rune is consumed (0x00160656).
- Eye of kilrogg: summoned unit type 0x2D; lifetime via decay (§8).

---

## 3. Harvesting

### 3.1 Cycle
1. **Harvest order 23**: worker walks to the mine / tree. Gold/oil: enters the building (order 25).
2. **Chopping** (0x0013B66C): each order call (= one swing of the worker's attack animation, 25 cycles
   for peasant/peon: 1+3+3+3+5+3+7) adds 1 to +0x74; at **50** the tree tile is cut (0x0016982C) and the
   worker carries a load → about **1250 cycles per lumber load** (affected by haste/slow). A cheat flag
   (`A5+6878` bit 0x800) makes it instant. The chopping sound uses the C-library `rand()`, not the game
   RNG (0x0013A78C). CONFIRMED. **Wargus≠** (Wargus 24-cycle chops; total ≈ the same).
3. **Entering a building with goods (order 25, 0x0013BC28)**: the worker steps in, is hidden
   (0x0013BB80) and the anim wait is set to **150 cycles**, then queued order 26. On entry:
   - depot → goods credited at once (§3.2);
   - gold mine / oil platform → occupancy +0x81 + 1, sprite frame "active" (mine 1, platform 2), if the
     remaining loads +0x82 ≠ 0 the worker gets a load and +0x82 − 1; at 0 the mine/platform is destroyed
     (0x0016C452). CONFIRMED.
   So **150 cycles inside a mine/platform and 150 inside a depot**, for peasants and tankers alike.
   **Wargus≠** for oil (SPEC_RULES: 100).
4. **Leaving (order 26, 0x0013BA1E)**: occupancy − 1 (sprite back to idle at 0); the worker is placed
   next to the building with anim wait 25; computer workers are re-tasked by the AI; human workers
   carrying goods go to the nearest depot, empty ones return to the remembered mine/tree (+0x6C/+0x70).
   CONFIRMED.
- **No exclusivity check** for mine occupancy was found: several workers can be inside one mine at the
  same time. LIKELY. **Wargus≠** (Stratagus: one at a time).

### 3.2 Amount per trip (0x0013B8AC)
| resource | base | with bonus | bonus condition | delivered to |
|---|---|---|---|---|
| gold | 100 | 110 / **120** | player owns ≥1 complete keep/stronghold → 110; ≥1 castle/fortress → 120 (castle checked first) | hall tiers only (flag 0x1000) |
| lumber | 100 | 125 | player owns ≥1 complete lumber mill | hall tiers or lumber mill (0x40000) |
| oil | 100 | 125 | player owns ≥1 complete refinery | any oil depot (tanker to non-platform: shipyard, refinery) |
CONFIRMED amounts and the counters used; the bonus is **player-wide**, not tied to the depot used.
The mine is always decremented by one load regardless of the bonus. Delivered amounts are also added to
per-player harvest statistics (`A5+6E7A` gold, `A5+6EBA` lumber, `A5+6EFA` oil).

### 3.3 Depot and mine choice
- Nearest suitable depot: land workers scan the owner's unit list with a filter (0x00151A84) and keep the
  best by distance (0x001517AE); ships use 0x001519EA. CONFIRMED structure, exact metric LIKELY Chebyshev.
- Computer workers choose a mine minimising `dist(depot, mine) + dist(mine, worker)/2` over all mines
  with loads left (0x00151C22). CONFIRMED.

### 3.4 Mine and oil amounts
PUD unit loader (0x00143EDE): for gold mines / oil patches
`loads = PUD_value × 2500 / 100` (= value × 25 trips of 100); a value of 0 defaults to **120 → 3000
loads (300 000 gold)**. CONFIRMED. **Wargus≠** (default 100 000). An oil platform takes over the patch's
loads when built and gives them back to a new oil patch when it dies (0x0016C29C, 0x0016C6CA). CONFIRMED.

---

## 4. Production (train, research, upgrade, build)

### 4.1 Starting (0x0012438C) and costs
- Cost/time is computed by `A5+2EF6[kind]`: units/hall upgrades 0x00123F64 (UDTA gold/lumber/oil
  ×10, time = **UDTA build_time × 2** steps), research/upgrades 0x00123EF8 (UGRD tables at A5+3A0C time,
  A5+3A40/3AA8/3B10 costs, time × 2 steps). Resources are **deducted at start** (0x00123E6E).
  CONFIRMED.
- Training a unit (type < 0x3A) is refused when `food_supply ≤ units − food_exempt` (§5). CONFIRMED.
- Hall upgrades and units increment `A5+6DB6[p]` (units in production); research sets a pending bit in
  `A5+6B4A[p]`, weapon/armor levels a pending bit in `A5+6BCA[p]`. CONFIRMED.

### 4.2 Progress (0x001242C2, order 37)
- +1 step per order call; the building's work animation is "wait 10" once, then "yield, wait 4" loop
  (`War Data` 278), i.e. **one step every 5 cycles**. Done after `2 × time` steps → **about 10 cycles
  per time unit** (footman 60 → ≈600 cycles = 16 s at speed 5). Cheat `A5+6878` bit 2 adds 10 steps per
  call. CONFIRMED (code), the 5-cycle cadence LIKELY (anim data). **Wargus≠** (6 cycles per unit).
- Progress bar byte +0x26 = 255·cur/total. CONFIRMED.
- **Cancel** refunds **100 %** (0x001240B2 / 0x00124156 / 0x00124204 with done = 0). CONFIRMED.
- Completion: unit placed next to the building (0x0016C606); if no room, cost refunded and a message
  shown. Research sets `A5+6ACA[p]` bit (0x00124156); upgrade levels increment the per-player level byte
  via `A5+2F06[group]` (0x00124204); hall upgrade changes type and adds `maxHP(new) − maxHP(old)` to
  current HP (0x00124016). CONFIRMED.

### 4.3 Construction (0x0016B866, called every cycle from 0x0016BAE2)
- New building starts with **HP = maxHP/10, capped at 40** (0x0016C6CA); increment = maxHP − start HP;
  accumulator starts at build_time. CONFIRMED.
- Elapsed counter +0x84 + 1 per cycle; every **12th** cycle one step: accumulator −= increment, and
  while ≤ 0: HP + 1, accumulator += build_time (≈ increment/build_time HP per step). Finished when
  elapsed = **(build_time + 1) × 12 cycles** (town hall 255 → 3072 cycles ≈ 82 s at speed 5; farm 100 →
  1212). Cheat bit 2 adds 12 per cycle. HP is clamped to max. CONFIRMED. **Wargus≠** (6 cycles/unit).
- Progress byte = min(HP fraction, time fraction) × 255. CONFIRMED.
- Only **one** builder: the worker is hidden inside and released when the building completes
  (0x0016B866 loop). No speed-up from extra workers was found. CONFIRMED / LIKELY.
- Payment: human player pays when the worker places the building; computer pays when ordering and is
  refunded if placement fails (0x00132BFE). CONFIRMED.
- Cancel / destruction of an unfinished building refunds **75 %** of gold and lumber (`cost/10 × 30 >> 2`,
  no oil) and ejects workers inside (0x0016C452). LIKELY (exact trigger).

---

## 5. Food (0x001312AA, 0x00131306, 0x00123F64)

| source | supply |
|---|---|
| complete farm / pig farm | +4 |
| complete town hall, keep, castle (and orc tiers) | +1 each |
- Use = number of non-building units **minus** skeletons, daemons and critters (counter A5+664E).
  Transports, tankers, flyers, eyes of kilrogg **do** count. CONFIRMED.
- No hard unit cap was found in the training check. UNKNOWN whether one exists elsewhere.
  **Wargus≠?** (Wargus limit 200).

---

## 6. Repair (order 27, 0x00132CC8)

- Each order call (= one swing of the worker's attack animation, 25 cycles for peasant/peon): **+4 HP**,
  clamped to max. Every **second** swing costs **1 gold + 1 lumber** (counter in the target's +0x84).
  No oil cost. Stops when the target is at full HP or when gold or lumber is below 1 (message for the
  local player). CONFIRMED. **Wargus≠** (oil cost, ratio).
- Works on any damaged own unit the worker can reach (buildings, ships). LIKELY.

---

## 7. Movement basics

- Speed comes only from the move animation (cycles per tile, RE_NOTES §8); haste halves / slow
  doubles the wait opcodes (07/09; opcode 08 halves for both — original bug). CONFIRMED (RE_NOTES).
- Naval units and flyers live on even tile coordinates (positions masked to 64 px on creation,
  0x0016C6CA; random destinations rounded to even, 0x00120832). CONFIRMED.
- Blocked movement, replanning: RE_NOTES §6.
- Idle fidget (0x0012130E): per idle call `Random()&0xFF`: 0 → turn left, 1..3 → turn right; critters
  wander one tile for values < 0x33. CONFIRMED.

---

## 8. Hit points, mana, regeneration, decay (0x0016BBFA, once per cycle)

| rule | value | conf. |
|---|---|---|
| natural HP regeneration | none | CONFIRMED |
| berserker regeneration (type 0x13, research flag `A5+715E[p]`) | +1 HP every **240** cycles (counter +0x26) | CONFIRMED; **Wargus≠** (1 HP / 2 s) |
| caster start mana (flag 0x20000) | **85** | CONFIRMED (0x0016C6CA, 0x0016D3A8) |
| mana regeneration | +1 every **40** cycles, max 255 | CONFIRMED |
| decaying units (UDTA decay rate ≠ 0) | lifetime counter starts at 255, −1 every `decay` cycles; at 0 the unit loses 5 HP per period and dies below 5 | CONFIRMED |
| → skeleton (decay 100) | 25 500 cycles, then drains | derived |
| → eye of kilrogg (decay 3) | 765 cycles, then drains | derived (matches Wargus 765) |

---

## 9. Death, corpses, rubble (order 1, 0x0013246A; kill 0x0016C29C)

- Units: if `A5+21A8[type]` is set the unit becomes a **dead body** (type 0x69) playing its death
  animation; transports on water and units with the table entry 0 vanish. Units with flag 0x4 (siege)
  and the zeppelin spawn an explosion missile (type 23). A dying transport kills its passengers.
  A dying worker drops its cargo. CONFIRMED.
- Buildings: replaced by rubble of the footprint size (`destroyed_1x1..4x4` = types 106..109); a dying
  oil platform with oil left re-creates an oil patch (type 0x5D) with the remaining loads. CONFIRMED.
- Corpse decay timing: from the corpse animations (`War Data` 278). UNKNOWN in detail.

---

## 10. Burning buildings (0x0016BAE2)

Complete buildings below **75 %** HP get a small fire, below **50 %** a big fire (missile slots 0/1
attached at +0x7C). Purely visual: **no HP loss** in this routine. CONFIRMED.

---

## 11. Sight and fog

- UDTA sight 0..9 is replaced at load (0x00138602) by one of 10 reveal routines (`A5+18F4`,
  0x00144C40..0x00144D40) that stamp a **(2s+1)×(2s+1) square** of fog codes onto the player's fog map
  (`f13_07EC` with combine callback 0x00144B40). Sight 0 behaves as 1. CONFIRMED.
- Stamp shape: the outer ring is a soft edge (boundary shapes 1..15), the inside is clear. Corners are
  trimmed: for s ≤ 4 the four corner cells stay covered and the next diagonal cells are soft; for s ≥ 5
  three cells per corner stay covered. I.e. fully visible ≈ Chebyshev radius s−1 with clipped corners —
  **not** a circle. CONFIRMED (data). **Wargus≠** (circle).
- Fog snapshot / redraw every 100 cycles: RE_NOTES §9.

---

## 12. Victory and defeat (0x00172D16, 0x00172D90)

- A player is **out** when it has no buildings and all its remaining units are transports, tankers or
  flying machines/zeppelins; human players must also still be connected (`A5+68AE[p*0x24]`). CONFIRMED.
  **Wargus≠** (any unit keeps a player alive).
- The local player loses when out. Victory needs **no computer player alive** and all remaining human
  players mutually allied (both directions of `A5+6F3A`). CONFIRMED.
- Campaign objectives use a separate check (0x00172E88, flags `A5+6D5E`). Not decoded.

---

## 13. Open questions

1. Exact name of each order 3..21, 30..36, 57..63 (only handlers known).
2. Meaning of the per-type value `A5+59EC` used as attack cooldown and its relation to the attack anim.
3. Depot-choice metric (0x00151A84 / 0x001517AE) and naval variant 0x001519EA.
4. Whether a unit cap (like Wargus 200) exists anywhere.
5. Trigger of the 75 % refund (cancel only, or also destruction of an unfinished building).
6. Order 40 (mana 5) identity; flame shield vs fireball naming of 0x00160BB6.
7. Multi-tile units: where the sight stamp is centred.
8. Demolition damage value (argument dropped by Ghidra in 0x001213C0) and dwarf damage against walls.
9. Corpse/rubble lifetime and when the dead-body unit is freed.
