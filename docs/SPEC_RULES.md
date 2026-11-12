# Warcraft II — game rules spec (for our engine)

Companion to `src/w2_tables.h` / `src/w2_tables.c`. Everything here is a *fact about the game*
gathered from data; no code is taken from Wargus/Stratagus.

Markers: **NIEPEWNE** = not verified against the original game (needs RE of the 68k binary or
testing in the original). **WARGUS≠** = Wargus is known/suspected to deviate from the original.

## 1. Sources and their authority

| source | what it gives | authority |
|---|---|---|
| Mac `War Data` entry 0 (default UDTA, 5694 B) and entry 1 (default UGRD, 780 B) | HP, armor, damage, ranges, sight, costs, build time, footprint, box, flags, priority, missile, target mask, points, upgrade costs/times/icons | **authoritative** (the game's own defaults) |
| UDTA/UGRD sections in 8 Mac PUDs (`hfs/Custom PUDs`) | same structure | cross-check: 4 PUDs equal the default except 2 fields (dark portal box 128 vs 127, eye of kilrogg organic bit); 4 PUDs (Channel, Death, Dragon, Icebrdge) carry an *older/edited* table (e.g. mage 40 HP, gryphon 2000 gold, ballista 120 HP/100 dmg) — ignored |
| Mac `War Strings` entry 1 | unit/upgrade names, hotkey hints ("fTRAIN FOOTMAN"), status strings | authoritative for names |
| Wargus `scripts/*.lua` | animations, buttons, missiles, spells, sounds, constructions, dependencies, speeds, harvest timing, AI | documentation; WARGUS≠ where noted |
| Wargus `wartool.h` | archive entry numbers (War Data / War Sounds), name expansion `%N` | entry numbering == Mac (verified via `docs/ENTRY_MAP.md`) |

Wargus Lua was evaluated in a sandbox (all Define* calls recorded) with
`wc2.preferences.EnhancedEffects = false`, `wargus.extensions = false`, `wargus.expansion = false`
to get the closest-to-original variants. The generator (Python, kept out of the repo) only emits
numbers/names into our own struct layout.

Mac 1.2x is **Tides of Darkness only**: War Data has 438 entries (no expansion entries ≥ 438),
so no swamp tileset and no BDP heroes. Units 0x14–0x19, 0x23, 0x2C, 0x2E, 0x2F are flagged
`W2UX_EXPANSION` (stats from Wargus, `W2UX_STATS_WARGUS`); 0x22, 0x24, 0x25, 0x30, 0x36 are unused.

## 2. Unit types

Unit id == PUD unit id 0x00..0x68 (`W2_NUM_UNIT_TYPES` = 0x69; UDTA arrays have 110 slots).
`w2_unit_types[]` fields: see header. Values below are from the default UDTA.

| id | name | HP | arm | dmg b+p | range | sight | speed | time | gold/wood/oil |
|---:|---|---:|---:|---|---|---:|---:|---:|---|
| 00 | Footman | 60 | 2 | 6+3 | 1 | 4 | 10 | 60 | 600/0/0 |
| 01 | Grunt | 60 | 2 | 6+3 | 1 | 4 | 10 | 60 | 600/0/0 |
| 02 | Peasant | 30 | 0 | 3+2 | 1 | 4 | 10 | 45 | 400/0/0 |
| 03 | Peon | 30 | 0 | 3+2 | 1 | 4 | 10 | 45 | 400/0/0 |
| 04 | Ballista | 110 | 0 | 80+0 | 2-8 | 9 | 5 | 250 | 900/300/0 |
| 05 | Catapult | 110 | 0 | 80+0 | 2-8 | 9 | 5 | 250 | 900/300/0 |
| 06 | Knight | 90 | 4 | 8+4 | 1 | 4 | 13 | 90 | 800/100/0 |
| 07 | Ogre | 90 | 4 | 8+4 | 1 | 4 | 13 | 90 | 800/100/0 |
| 08 | Elven Archer | 40 | 0 | 3+6 | 4 | 5 | 10 | 70 | 500/50/0 |
| 09 | Troll Axethrower | 40 | 0 | 3+6 | 4 | 5 | 10 | 70 | 500/50/0 |
| 0A | Mage | 60 | 0 | 0+9 | 2 | 9 | 8 | 120 | 1200/0/0 |
| 0B | Death Knight | 60 | 0 | 0+9 | 3 | 9 | 8 | 120 | 1200/0/0 |
| 0C | Paladin | 90 | 4 | 8+4 | 1 | 5 | 13 | 90 | 800/100/0 |
| 0D | Ogre-Mage | 90 | 4 | 8+4 | 1 | 5 | 13 | 90 | 800/100/0 |
| 0E | Dwarven Demolition Squad | 40 | 0 | 4+2 | 1 | 4 | 11 | 200 | 700/250/0 |
| 0F | Goblin Sappers | 40 | 0 | 4+2 | 1 | 4 | 11 | 200 | 700/250/0 |
| 10 | Attack Peasant | 30 | 0 | 3+2 | 1 | 4 | 10 | 45 | 400/0/0 |
| 11 | Attack Peon | 30 | 0 | 3+2 | 1 | 4 | 10 | 45 | 400/0/0 |
| 12 | Ranger | 50 | 0 | 3+6 | 4 | 6 | 10 | 70 | 500/50/0 |
| 13 | Berserker | 50 | 0 | 3+6 | 4 | 6 | 10 | 70 | 500/50/0 |
| 1A | Human Oil Tanker | 90 | 10 | 0+0 | 1 | 4 | 10 | 50 | 400/200/0 |
| 1B | Orc Oil Tanker | 90 | 10 | 0+0 | 1 | 4 | 10 | 50 | 400/200/0 |
| 1C | Human Transport | 150 | 0 | 0+0 | 1 | 4 | 10 | 70 | 600/200/500 |
| 1D | Orc Transport | 150 | 0 | 0+0 | 1 | 4 | 10 | 70 | 600/200/500 |
| 1E | Elven Destroyer | 100 | 10 | 35+0 | 4 | 8 | 10 | 90 | 700/350/700 |
| 1F | Troll Destroyer | 100 | 10 | 35+0 | 4 | 8 | 10 | 90 | 700/350/700 |
| 20 | Battleship | 150 | 15 | 130+0 | 6 | 8 | 6 | 140 | 1000/500/1000 |
| 21 | Ogre Juggernaught | 150 | 15 | 130+0 | 6 | 8 | 6 | 140 | 1000/500/1000 |
| 26 | Gnomish Submarine | 60 | 0 | 50+0 | 4 | 5 | 7 | 100 | 800/150/900 |
| 27 | Giant Turtle | 60 | 0 | 50+0 | 4 | 5 | 7 | 100 | 800/150/900 |
| 28 | Gnomish Flying Machine | 150 | 2 | 0+0 | 1 | 9 | 17 | 65 | 500/100/0 |
| 29 | Goblin Zeppelin | 150 | 2 | 0+0 | 1 | 9 | 17 | 65 | 500/100/0 |
| 2A | Gryphon Rider | 100 | 5 | 0+16 | 4 | 6 | 14 | 250 | 2500/0/0 |
| 2B | Dragon | 100 | 5 | 0+16 | 4 | 6 | 14 | 250 | 2500/0/0 |
| 2D | Eye of Kilrogg | 100 | 0 | 0+0 | 1 | 3 | 42 | 0 | 0/0/0 |
| 31 | Cho'gall | 100 | 0 | 10+5 | 1 | 5 | 13 | 100 | 1100/50/0 |
| 32 | Lothar | 90 | 4 | 8+4 | 1 | 5 | 13 | 100 | 900/100/0 |
| 33 | Gul'dan | 40 | 0 | 0+3 | 3 | 8 | 8 | 120 | 1200/0/0 |
| 34 | Uther Lightbringer | 90 | 4 | 8+4 | 1 | 5 | 13 | 100 | 900/100/0 |
| 35 | Zuljin | 40 | 0 | 3+6 | 5 | 6 | 10 | 70 | 500/50/0 |
| 37 | Skeleton | 40 | 0 | 6+3 | 1 | 3 | 8 | 0 | 0/0/0 |
| 38 | Daemon | 60 | 2 | 9+1 | 2 | 5 | 14 | 0 | 0/0/0 |
| 39 | Critter | 5 | 0 | 0+0 | 1 | 2 | 3 | 0 | 0/0/0 |

| id | building | HP | arm | sight | time | gold/wood/oil | tiles | notes |
|---:|---|---:|---:|---:|---:|---|---|---|
| 3A | Farm | 400 | 20 | 3 | 100 | 500/250/0 | 2x2 | food +4 |
| 3B | Pig Farm | 400 | 20 | 3 | 100 | 500/250/0 | 2x2 | food +4 |
| 3C | Human Barracks | 800 | 20 | 3 | 200 | 700/450/0 | 3x3 |  |
| 3D | Orc Barracks | 800 | 20 | 3 | 200 | 700/450/0 | 3x3 |  |
| 3E | Church | 700 | 20 | 3 | 175 | 900/500/0 | 3x3 |  |
| 3F | Altar of Storms | 700 | 20 | 3 | 175 | 900/500/0 | 3x3 |  |
| 40 | Scout Tower | 100 | 20 | 9 | 60 | 550/200/0 | 2x2 |  |
| 41 | Watch Tower | 100 | 20 | 9 | 60 | 550/200/0 | 2x2 |  |
| 42 | Stables | 500 | 20 | 3 | 150 | 1000/300/0 | 3x3 |  |
| 43 | Ogre Mound | 500 | 20 | 3 | 150 | 1000/300/0 | 3x3 |  |
| 44 | Gnomish Inventor | 500 | 20 | 3 | 150 | 1000/400/0 | 3x3 |  |
| 45 | Goblin Alchemist | 500 | 20 | 3 | 150 | 1000/400/0 | 3x3 |  |
| 46 | Gryphon Aviary | 500 | 20 | 3 | 150 | 1000/400/0 | 3x3 |  |
| 47 | Dragon Roost | 500 | 20 | 3 | 150 | 1000/400/0 | 3x3 |  |
| 48 | Human Shipyard | 1100 | 20 | 3 | 200 | 800/450/0 | 3x3 |  |
| 49 | Orc Shipyard | 1100 | 20 | 3 | 200 | 800/450/0 | 3x3 |  |
| 4A | Town Hall | 1200 | 20 | 4 | 255 | 1200/800/0 | 4x4 | food +1 |
| 4B | Great Hall | 1200 | 20 | 4 | 255 | 1200/800/0 | 4x4 | food +1 |
| 4C | Elven Lumber Mill | 600 | 20 | 3 | 150 | 600/450/0 | 3x3 | +25% wood |
| 4D | Troll Lumber Mill | 600 | 20 | 3 | 150 | 600/450/0 | 3x3 | +25% wood |
| 4E | Human Foundry | 750 | 20 | 3 | 175 | 700/400/400 | 3x3 |  |
| 4F | Orc Foundry | 750 | 20 | 3 | 175 | 700/400/400 | 3x3 |  |
| 50 | Mage Tower | 500 | 20 | 3 | 125 | 1000/200/0 | 3x3 |  |
| 51 | Temple of the Damned | 500 | 20 | 3 | 125 | 1000/200/0 | 3x3 |  |
| 52 | Human Blacksmith | 775 | 20 | 3 | 200 | 800/450/100 | 3x3 |  |
| 53 | Orc Blacksmith | 775 | 20 | 3 | 200 | 800/450/100 | 3x3 |  |
| 54 | Human Refinery | 600 | 20 | 3 | 225 | 800/350/200 | 3x3 | +25% oil |
| 55 | Orc Refinery | 600 | 20 | 3 | 225 | 800/350/200 | 3x3 | +25% oil |
| 56 | Human Oil Platform | 650 | 20 | 3 | 200 | 700/450/0 | 3x3 |  |
| 57 | Orc Oil Platform | 650 | 20 | 3 | 200 | 700/450/0 | 3x3 |  |
| 58 | Keep | 1400 | 20 | 6 | 200 | 2000/1000/200 | 4x4 | food +1, +10% gold |
| 59 | Stronghold | 1400 | 20 | 6 | 200 | 2000/1000/200 | 4x4 | food +1, +10% gold |
| 5A | Castle | 1600 | 20 | 9 | 200 | 2500/1200/500 | 4x4 | food +1, +20% gold |
| 5B | Fortress | 1600 | 20 | 9 | 200 | 2500/1200/500 | 4x4 | food +1, +20% gold |
| 5C | Gold Mine | 25500 | 20 | 3 | 150 | 0/0/0 | 3x3 |  |
| 5D | Oil Patch | 0 | 0 | 0 | 0 | 0/0/0 | 3x3 |  |
| 60 | Human Guard Tower | 130 | 20 | 9 | 140 | 500/150/0 | 2x2 | dmg 4+12 range 6 |
| 61 | Orc Guard Tower | 130 | 20 | 9 | 140 | 500/150/0 | 2x2 | dmg 4+12 range 6 |
| 62 | Human Cannon Tower | 160 | 20 | 9 | 190 | 1000/300/0 | 2x2 | dmg 50+0 range 2-7 |
| 63 | Orc Cannon Tower | 160 | 20 | 9 | 190 | 1000/300/0 | 2x2 | dmg 50+0 range 2-7 |
| 64 | Circle of Power | 0 | 0 | 0 | 0 | 0/0/0 | 2x2 |  |
| 65 | Dark Portal | 5000 | 0 | 0 | 0 | 0/0/0 | 4x4 |  |
| 66 | Runestone | 5000 | 20 | 4 | 175 | 900/500/0 | 2x2 |  |
| 67 | Wall | 40 | 0 | 1 | 30 | 20/10/0 | 1x1 |  |
| 68 | Wall | 40 | 0 | 1 | 30 | 20/10/0 | 1x1 |  |

Notes:
- Costs in UDTA are stored /10; the table holds full values.
- `flags` is the UDTA flags word, bit-exact (`W2UF_*`). Ballista/catapult lack the land bit in
  UDTA; our `kind` derives LAND from UDTA type 0.
- Footprint: UDTA says 1x1 for ships and flyers, box 63x63 px (two tiles). All ships/flyers in
  the sample PUDs sit on even coordinates → naval and air units live on a **2x2-tile grid**
  (move in 2-tile steps, occupy one cell of the coarse grid). Wargus models them as 2x2 units.
  NIEPEWNE: exact occupancy rule for the land/sea map.
- Food: every non-building unit uses 1 (`food = -1`), farm/pig farm supply 4, each town hall
  tier supplies 1. A unit cannot be trained without free food ("Not enough food...build more
  farms."). Upper food limit (Wargus unit limit 200) — NIEPEWNE.
- Equivalents (`equiv`) map human↔orc unit for race conversion of PUD units (start units of a
  player whose race differs).
- Gold mine / oil patch amount: PUD UNIT.data × 2500 (`W2_GOLD_PER_PUD_UNIT`). Default when 0:
  gold 100000, oil 50000 (Wargus). Gold mine HP 25500 armor 20, but it is never attacked.
- Wargus deviations found in stats (WARGUS≠): ballista armor 5 (UDTA 0); walls armor 20
  (UDTA 0); building sight 1–6 (UDTA 3/4/6/9); guard/cannon tower priority & annoy;
  axethrower priority 50 (UDTA 55); daemon 10+2 armor 3 range 3 (UDTA 9+1, 2, 2); critter
  damage 80 (Wargus explosion hack, UDTA 0); Zuljin 120 HP (UDTA 40); dark portal cost/sight
  (UDTA none: not buildable in Tides); ships/flyers 2x2 tiles.

## 3. Time base and game speed

- Game logic runs in **cycles**. Wargus/Stratagus "normal" = **30 cycles/s**
  (`W2_CYCLES_PER_SECOND_NORMAL`). The Wargus speed slider goes 15..75 c/s in steps of 5.
  Our `w2_game_speeds[]` (15/22/30/45/60) is a proposal — NIEPEWNE, the original menu values
  must be measured.
- Animation `wait` values are in cycles at any speed (speed only changes cycles per second).
- Build/train/research `time` (UDTA/UGRD byte) is in "time units". Stratagus advances building
  construction by 100 per cycle against `time*600`, i.e. **1 unit = 6 cycles** (town hall 255 →
  1530 cycles = 51 s at normal). Its training/research code steps every `30/6 = 5` cycles which
  would give 1 unit = 5 cycles. `W2_TIME_UNIT_CYCLES = 6` — NIEPEWNE until measured.
- Haste halves animation waits (minimum 1), slow doubles them (Stratagus behaviour) —
  NIEPEWNE for the original.

## 4. Animations

`w2_anim_sets[]` × `W2_NUM_ANIM_KINDS` → ranges in `w2_anim_steps[]` (58 sets, 1357 steps).

Step = `{frame, wait, arg, flags}`:
- play: show `frame` (+ direction column), apply `arg` pixels of movement (unless the step has
  `W2AF_SOUND`/`W2AF_IDLE_TURN`, then `arg` is a sound id / turn amount), fire `W2AF_ATTACK`,
  then wait `wait` cycles.
- `W2AF_UNBREAK_BEGIN..END`: new orders are deferred until END (attack swing, move tile step).
- `W2AF_IDLE_TURN`: idle sequence `{frame 0 wait 4}{frame 0 wait 1, 1% chance turn ±1}` (all
  standing units randomly look around).
- STILL, MOVE, ATTACK loop; DEATH plays once and ends with `W2AF_DIE` → unit removed and its
  `corpse` unit spawned (corpses/rubble have their own DEATH sequence: decay frames 200 cycles each).
- MOVE: one tile step; for every set the `arg` sum is exactly 32 px. Diagonal steps move `arg`
  px on both axes. Cycles per tile ≈ 160 / `speed` (footman 16, knight 12, ballista 32,
  flyer 10–12, battleship 26, eye of kilrogg 4). Ships/flyers use the same 32-px sequence;
  they cover a 2-tile cell in two sequences.
- Gold mine / oil platform: STILL = frame 0, ACTIVE = frame 1 (mine with miner inside) / 2.
- Buildings: STILL frame 0; UPGRADE (being upgraded) frame 1.
- Wargus ship "wiggle" (bobbing) and "spawn dead-vision revealer" were dropped (WARGUS≠ extras).
  The revealer reflects a real rule though: after a unit dies its sight stays for ~160 cycles
  (Wargus: revealer lives 80 + 80 cycles with full then 1 tile sight) — NIEPEWNE for original.

Direction convention: headings 0..7 = N, NE, E, SE, S, SW, W, NW. Unit sprites store 5
columns (N..S); a step's `frame` F is the row base, sprite frame = F + heading for 0..4 and
F + (8 − heading) mirrored horizontally for 5..7 (`w2_sprite_frame()`). Frame bases are
multiples of 5. Missiles with `dirs = 9` use the same 5-stored layout (Stratagus convention).

Attack cycle lengths (cycles, normal): melee 25; archer 65; axethrower 74; mage/DK 40;
ballista/catapult 200; destroyer 120; battleship 230; submarine/turtle 115; gryphon/dragon 190;
guard tower 60; cannon tower 151. This *is* the attack rate (no separate cooldown field).

## 5. Combat

Damage (Stratagus formula, matches the community-known W2 rule):

    d = max(basic − armor, 1) + piercing        (bloodlust: basic and piercing ×2)
    d = d − random(0 .. (d+2)/2 − 1)            → about 50 %..100 % of d

NIEPEWNE: minimum of 1 for the basic part and the exact random rounding in the original.
Armor upgrades add to armor, weapon upgrades add to *piercing* (see §8).

- Unholy armor: target becomes invulnerable for 500 cycles at the cost of half its current HP
  (volatile units die). NIEPEWNE duration.
- Splash (`w2_missiles[].splash_range`, `splash_factor`): catapult rock, ballista bolt
  (range 2? factor 4), cannon (factor 3), big cannon (4), fireball / gryphon hammer / dragon
  breath (bounce missiles: 3–5 bounces, factor 2). Units in the splash radius take
  damage/factor (Stratagus). NIEPEWNE: original splash shapes (W2 hits a 3x3 area?).
- Attack range is measured in tiles between unit footprints (Chebyshev distance);
  `min_range` 2 for ballista/catapult/cannon tower (cannot hit adjacent units).
- `can_target` (UDTA): land 1, sea 2, air 4. Melee and ships: land only or land+sea; archers,
  mages, gryphons, destroyers, guard towers hit air. Submarines (`W2UF_SUBMARINE`) are visible
  only to units with `W2UF_SEE_SUBMARINE` (towers, flyers, eye, submarines) — NIEPEWNE rules.
- Ground attack (`W2UX_GROUND_ATTACK`): ballista, catapult, battleship/juggernaut can attack a
  position (and walls).
- Demolition squad / sappers: `W2SP_SUICIDE_BOMBER` (Demolish): 400 damage, radius 3 tiles in
  Wargus, unit dies; can destroy walls/rock. NIEPEWNE radius.
- Auto-acquire range: `react_human` (owner human) / `react_cpu` (owner computer) tiles.
- Target choice: highest `priority` among reachable enemies (NIEPEWNE tie rules).
- `W2UX_COWARD` (workers, balloons, tankers): do not auto-attack.
- Killed buildings leave rubble (`corpse` destroyed-NxN), units leave dead bodies; ships leave
  a wreck; units with `W2UX_EXPLODES` spawn the explosion missile.
- Score: `points` per kill.

## 6. Hit points, mana, repair

- No natural HP regeneration for units or buildings, except berserkers after
  *Berserker Regeneration*: +1 HP every 2 s (Wargus `regeneration-rate 1, frequency 2`) — NIEPEWNE.
- Healing spell: 1 HP per 6 mana (repeat-cast), organic non-building targets.
- Mana: casters (`W2UX_MANA`) max 255, start 84 (Wargus; original ≈ one third), +1 per second
  (NIEPEWNE rate).
- Repair (peasant/peon `W2UX_CAN_REPAIR`): Wargus 4 HP per repair step costing 1 gold + 1 wood
  (+1 oil for oil-cost buildings); repair range 1. NIEPEWNE exact cost ratio.
- Burning buildings: below 75 % HP small fire, below 50 % big fire (visual). Original may also
  burn down — NIEPEWNE.

## 7. Economy

`w2_harvest_info[]`:

| unit | resource | carry | at resource | at depot | sprite when loaded |
|---|---|---:|---|---|---|
| peasant/peon | gold | 100 | 150 cycles inside the mine | 150 cycles | entry 124/125 |
| peasant/peon | wood | 100 | 2 per 24 cycles → 50 chops = 1200 cycles | 150 | entry 122/123 |
| oil tanker | oil | 100 | 100 cycles in platform | 100 | full 126/127, empty 59/60 |

All NIEPEWNE as real-time values (Wargus tuning); 100 per trip is certain.
Only one worker inside a mine at a time (Stratagus) — NIEPEWNE for original.

- Depots (`store`, UDTA bits 12/18/24): town hall tiers accept gold + wood
  (`W2UF_STORE_HALL`), lumber mill wood (`W2UF_STORE_WOOD`), shipyard and refinery oil
  (`W2UF_STORE_OIL`). Workers walk to the nearest depot accepting their load.
- Income bonuses (`improve_res/pct`): keep/stronghold +10 % gold, castle/fortress +20 % gold,
  lumber mill +25 % wood, refinery +25 % oil (i.e. 110/120 gold, 125 wood, 125 oil per trip).
- Town hall cannot be built within 3 tiles of a gold mine; shipyard/refinery/oil platform keep
  3 tiles from oil patches except the platform which must be *on* a patch (strings 396/397/395).
- Oil platform replaces the patch; the patch returns when the platform dies.
- Trees regrow: never (Wargus `SetForestRegeneration(0)`).

## 8. Upgrades and research

`w2_upgrades[52]` in UGRD order (costs/times/icons from default UGRD; identical to Wargus).
Effects (`w2_modifiers[]`, 162 rows):

| upgrade | effect | applies to |
|---|---|---|
| sword/battle axe 1,2 | +2 piercing each | footman/grunt, knight/ogre, paladin/ogre-mage, dwarves/sappers, heroes |
| arrow/throwing axe 1,2 | +1 piercing each | archer/axethrower, ranger/berserker |
| shield 1,2 | +2 armor each | same as swords |
| ship cannon 1,2 | +5 piercing each | destroyer, battleship/juggernaut, submarine/turtle |
| ship armor 1,2 | +5 armor each | destroyer, battleship/juggernaut, transport |
| ballista/catapult 1,2 | +15 piercing each | ballista/catapult |
| ranger / berserker training | converts archer→ranger / axethrower→berserker | |
| longbow / lighter axes | +1 range (longbow also +1 sight) | archers/rangers, axethrowers/berserkers |
| ranger/berserker scouting | +3 sight | |
| marksmanship | +3 piercing | archers, rangers |
| berserker regeneration | HP regen | axethrowers, berserkers |
| paladin / ogre-mage | knight→paladin, ogre→ogre-mage | |
| spells | enable a spell for the caster type | |

The original strings say "UPGRADE SWORDS (Damage +2)", "ARROWS (Damage +1)", "SHIELDS (Armor +2)",
"CANNONS (Damage +5)", "SHIP ARMOR (Armor +5)", "LONGBOW (Range +1)", "SCOUTING (Sight:9)",
"MARKSMANSHIP (Damage +3)", "CATAPULTS/BALLISTAS (Damage +15)" — consistent.
WARGUS≠? Longbow +1 sight is Wargus only (string says Range +1). Holy vision, fireball, eye of
kilrogg and death coil cost 0 / no building (known at start; death coil UGRD time 100 gold 0).

Dependencies (`w2_dependencies[]`, 86 rows, alt groups = OR): e.g. archer ← lumber mill;
knight ← stables + blacksmith; keep ← barracks; castle ← stables + blacksmith + lumber mill;
mage tower/church/aviary ← castle; stables/inventor ← keep or castle; shipyard ← lumber mill;
foundry/refinery ← shipyard; level 2 weapon/armor upgrades ← level 1. NIEPEWNE: Wargus allows
stables/inventor with castle *or* keep (same in original as castle implies keep).

## 9. Spells (`w2_spells[19]`)

| spell | mana | range | effect (p1/p2) |
|---|---:|---:|---|
| Holy Vision | 70 | ∞ | reveal area, revealer 25 cycles (Wargus) |
| Healing | 6 | 6 | +1 HP per cast step, repeat |
| Exorcism | 4 | 10 | 1 dmg per step to undead, repeat |
| Fireball | 100 | 8 | missile 20 dmg, bounces |
| Slow | 50 | 10 | 1000 cycles |
| Flame Shield | 50 | 6 | 5 orbiting flames, 600 cycles, 1 dmg/hit |
| Invisibility | 200 | 6 | 2000 cycles (broken by attacking) |
| Polymorph | 200 | 10 | target → critter (organic) |
| Blizzard | 25 | 12 | 5 fields × 11 shards, rand 10 dmg, repeat |
| Eye of Kilrogg | 70 | 6 | summon eye, 765 cycles |
| Bloodlust | 50 | 6 | 1000 cycles, double damage |
| Runes | 200 | 10 | 5 mines (+ shape), 50 dmg, 2000 cycles |
| Death Coil | 100 | 10 | 50 dmg to organic units (original also heals caster — NIEPEWNE) |
| Haste | 50 | 6 | 1000 cycles |
| Raise Dead | 50 | 6 | skeleton from corpse, 3600 cycles, repeat |
| Whirlwind | 100 | 12 | tornado 800 cycles, 3 dmg/tick |
| Unholy Armor | 100 | 6 | invulnerable 500 cycles, costs ½ HP |
| Death and Decay | 25 | 12 | like blizzard, repeat |
| Demolish | 0 | 1 | 400 dmg radius 3, self-destruct |

Mana costs are reliable (they match the original manual); durations/damages are Wargus
values — NIEPEWNE.

## 10. Missiles (`w2_missiles[31]`)

Missile id == UDTA missile byte; ids 0..28 map to War Data entries 324+id (sprites).
29 = none (melee). 30 = death coil (re-uses touch-of-death sprite). Fields: speed px/step,
sleep cycles/step, splash range/factor, bounces, impact missile, impact sound. Arrow/axe 32
px/step; ballista bolt/catapult rock 8; cannons 16–22; spells 16.
`dirs = 9` sprites: 5 stored directions, mirrored like units.

## 11. Constructions (`w2_constructions[11]`)

Stages: 0 % construction-site frame 0, 25 % site frame 1, 50 % building sprite frame 1, done
→ building frame 0. "land2" (gold mine, runestone, dark portal) has no 50 % stage. Site
sprites per era: land 252 (forest/wasteland) / 262 (winter); wall 261/275/276; shipyard
253/254 (+263/264 winter); oil well 255/256, 265/266, 271/272 (wasteland); refinery 257/258,
267/268; foundry 259/260, 269/270. Swamp column = wartool's dummy mapping (no swamp on Mac).
HP grows linearly with progress (Stratagus) — NIEPEWNE start HP.

## 12. Command card (`w2_buttons[]`, 555 entries)

3x3 grid, `pos` 1..9 row-major. Level 0 = main card, levels 1/2 = worker build menus
(basic/advanced, opened by `W2B_BUTTON arg=1/2`, ESC at pos 9 returns), level 9 = generic
cancel shown while choosing a target. Several entries can share a level/pos: they are icon
variants (e.g. stop/attack icons change with shield/weapon level) — take the **last entry
whose requirement holds**. Requirements `W2R_*` plus `w2_dependencies` for train/build/research.
Generic buttons (`unit == W2_NONE`) come last: cancel, cancel-upgrade, cancel-train, cancel-build.
Hotkeys are the lower-case letter (27 = ESC); they match the first character of the original
hint strings (e.g. "fTRAIN FOOTMAN").
Wargus extras flagged `W2BF_WARGUS_EXTRA`: balloon/zeppelin "explore", barracks "train
attack peasant" (Minuteman), debug walls. Walls require network/skirmish game (`W2R_NETWORK`)
in Wargus — NIEPEWNE for the original (walls exist in campaigns via map data).

## 13. Sight and fog

- Sight in tiles (circle; Stratagus uses a precomputed circle mask) — NIEPEWNE exact shape.
- Two fog levels: unexplored (black) and fogged (seen before: terrain + buildings shown as last
  seen, units hidden). Buildings are `VisibleUnderFog`.
- Flyers see over everything; land sight not blocked by terrain in W2 (no line-of-sight).
- Building under construction has sight; dead unit keeps vision briefly (see §4).
- Holy vision / eye of kilrogg / scouting upgrades modify sight.

## 14. AI (overview from Wargus `scripts/ai`)

Script-driven build order loops ("land attack", "passive", "sea attack", "air attack" names
also exist in the original strings 160–191: "Land Attack", "Passive", "Sea Attack",
"Air Attack", campaign AIs `_Human N`/`_Orc N`). Wargus land-attack script:
1. ensure town hall, 1→4→8 workers, barracks, blacksmith, weapon/armor upgrades;
2. build attack forces of increasing size (1, 4, 16 soldiers), wait until ready, attack;
3. upgrade hall → keep → castle, stables, lumber mill, temple/mage tower, research spells;
4. multiple forces (0 = home defence, 1..8 attack groups) with mixes of soldiers, shooters,
   cavalry, casters, catapults, flyers; towers (guard/cannon) at home;
5. end loop repeats attacks forever and keeps worker count growing (25…45).
Difficulty in Wargus scales AI build/train/harvest speed (75 %, 100 %, 120 %, 150 %) and gives
small resource cheats on the hardest levels (WARGUS≠: original AI has no difficulty setting).
AI reaction ranges use `react_cpu`; `annoy` biases which enemy buildings the AI attacks.

## 15. Victory conditions

- Skirmish/multiplayer: a player is defeated when they own no units and no buildings
  (Wargus trigger `TotalNumUnits == 0`); victory when no opponents remain. NIEPEWNE: original
  may ignore walls/critters for this test.
- Campaign maps have scripted objectives (rescue, destroy all, reach circle of power, etc.),
  stored per level in the campaign data, not in the PUD.

## 16. Open points (summary)

1. Real cycles/s for each original speed setting and time-unit → cycles factor (5 vs 6).
2. Exact damage randomisation and minimum damage.
3. Splash geometry; submarine visibility rules; tower minimum range.
4. Gather/return wait times (Wargus values used).
5. Mana start/regeneration; berserker regeneration rate; spell durations.
6. 2x2 grid semantics of ships/flyers.
7. Whether damaged buildings burn down.
8. UDTA flag bits 4 (critter/summoned?) and 26 (mage class?) meaning.

All of these can be settled by RE of the Mac 68k code (CODE resources) or by measurement in
the original running under emulation.
