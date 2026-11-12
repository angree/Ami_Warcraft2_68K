# RE notes — Warcraft II Mac 1.23 (68k) as the behaviour specification

Facts extracted from the player's own copy of the original Mac binary (resource fork `CODE`/`DATA`).
Prose and tables only; decompiled output lives outside the repo in `D:\GITHUB\_AmiW2-poza-repo\re68k\`
and never enters the repository.

## 0. How to rebuild the analysis

| step | command | output (outside repo) |
|---|---|---|
| 1. link image | `python tools/re/mac68k_link.py "hfs/Warcraft II.rsrc" D:\GITHUB\_AmiW2-poza-repo\re68k` (or the dir with `CODE_n.bin`/`DATA_0.bin`) | `w2_image.bin`, `w2_symbols.txt`, `w2_layout.txt`, `w2_jumptable.txt`, `w2_traps.txt` |
| 2. Ghidra language (once) | copy `tools/re/ghidra/mac68020.slaspec` + `mac68k.ldefs` into `<ghidra>\Ghidra\Processors\68000\data\languages\`, run `support\sleigh.bat <canonical path>\mac68020.slaspec` (path case must match the disk, e.g. `C:\Tools\...`) | `mac68020.sla` |
| 3. analyse + decompile | `tools\re\ghidra\run_w2.bat import` (fresh project) or `run_w2.bat export` (re-export after editing `w2_names.txt`) | `proj\w2`, `w2_all.c` |

Address conventions used below: the image is linked at **0x00100000**, `CODE n` sits at
`0x00100000 + n*0x8000` (segment header included), **A5 = 0x00210000**. Globals are written as
`A5+xxxx` / `A5-xxxx` (subtract 0x00210000 from a Ghidra `DAT_` address). Functions are written as
their image address plus the Ghidra name (`f<seg>_<offset>` = a jump-table entry point in CODE seg at
that offset, `FUN_` = segment-local). Mac A-line traps decompile as calls to `_TrapName` (pseudo block
at 0x00F00000 + (trap & 0xFFF)*2, see `tools/re/mac_traps.py`).

## 1. Program layout and loader (CODE 1)

- `CODE 0`: above-A5 size 0xD3D0, below-A5 size 0x8008, one jump-table entry (into CODE 1).
- `CODE 1` (1904 B) is a private segment loader. It zero-fills the A5 world, decompresses `DATA 0`
  into it, relocates, then patches `_LoadSeg`/`_UnloadSeg`/`_ExitToShell` with its own handlers and
  calls the first jump-table entry (A5-0x3A40 → CODE 2 +0x0E = `main_entry` 0x0011000E).
- `DATA 0`: `u32` offset of CODE 1's relocation lists, then 3 compressed blocks
  (`s32` A5-relative start + byte-code stream: `1xxxxxxx` literal run, `01xxxxxx` zero skip,
  `001xxxxx` byte repeat, `0001xxxx` 0xFF run, `00` end, `01..04` = 8-byte templates incl. unloaded
  jump-table entries `A9F0 <u32 offset> <u16 seg>`), then 3 relocation lists for the A5 world
  (deltas: A5, CODE1 base, A5) and 3 for CODE 1. Blocks cover A5-0x8000..0, A5+0x28..+0x341C,
  A5+0x73EE..+0xD3C8.
- Relocation list: `u32 count`, entries `1xxxxxxx` (offset += sext7·2), `01xxxxxx yyyyyyyy`
  (offset += sext14·2), `00xxxxxx +3 bytes` (offset = sext30·2); each adds the delta to a `u32`.
- `CODE 2..23`: 12-byte header `u16 jt_lo, u16 n_entries, s32 jt_offset(A5-rel), u32 reloc_offset`;
  relocation lists at `reloc_offset` (deltas: A5, CODE1 base, the segment itself).
- **Jump table lives below A5** (A5-0x3A40..A5, 1864 entries × 8 B). Loaded entry = `jmp abs.l`.
  Calls are `jsr d16(A5)` with a negative d16.
- `CODE 21` = RAD Smacker player with MacsBug names (44 functions: `smkmalloc`, `readframe`, …).
- `CODE 22` = C runtime (rand, time, memset …).

## 2. Unit-type table (initialised by data, not code)

The unit types are a **struct-of-arrays identical to the PUD `UDTA` section**. Defaults come from
`War Data` entry 0 (5694 bytes = UDTA without the leading "use defaults" word), read little-endian
and byte-swapped into the arrays by `ReadSwappedFields` 0x00138704 using a descriptor list
`{u32 ptr, u32 elem_size, u32 count}` at **A5-0x4AE2** (0x0020B51E, 32 entries, 0-terminated).
A PUD's `UDTA` section (handler `Pud_UDTA` 0x00144354, PUD section table at A5+0x826: 12-byte rows
`tag, handler, flag`) overwrites them when its first word is 0.

| field | address | elem × count | maps to (Wargus) |
|---|---|---|---|
| overlap frames | A5+3D18 | u16 × 110 | (frame overlap for construction) |
| obsolete | A5+3DF4..+40EE | 4 × u16 × 127 | — |
| sight | A5+41EC | u32 × 110 | SightRange |
| hit points | A5+43A4 | u16 × 110 | HitPoints |
| magic (has mana) | A5+4480 | u8 × 110 | CanCastSpell / mana |
| build time | A5+44EE | u8 × 110 | Costs time (×1) |
| gold / 10 | A5+455C | u8 × 110 | Costs gold |
| lumber / 10 | A5+45CA | u8 × 110 | Costs wood |
| oil / 10 | A5+4638 | u8 × 110 | Costs oil |
| tile size w,h | A5+46A6 | 2 × u16 × 110 | TileSize |
| box size w,h | A5+485E | 2 × u16 × 110 | BoxSize |
| attack range | A5+4A16 | u8 × 110 | MaxAttackRange |
| react range computer | A5+4A84 | u8 × 110 | ComputerReactionRange |
| react range human | A5+4AF2 | u8 × 110 | PersonReactionRange |
| armor | A5+4B60 | u8 × 110 | Armor |
| rect-selectable | A5+4BCE | u8 × 110 | SelectableByRectangle |
| priority | A5+4C3C | u8 × 110 | Priority |
| basic damage | A5+4CAA | u8 × 110 | BasicDamage |
| piercing damage | A5+4D18 | u8 × 110 | PiercingDamage |
| weapons upgradable | A5+4D86 | u8 × 110 | |
| armor upgradable | A5+4DF4 | u8 × 110 | |
| missile | A5+4E62 | u8 × 110 | Missile (index) |
| unit kind (land/air/sea) | A5+4ED0 | u8 × 110 | Type |
| decay rate | A5+4F3E | u8 × 110 | DecayRate |
| annoy computer | A5+4FAC | u8 × 110 | AnnoyComputerFactor |
| right-mouse action | A5+501A | u8 × 58 | RightMouseAction |
| point value | A5+5054 | u16 × 110 | Points |
| can target | A5+5130 | u8 × 110 | CanTargetLand/Sea/Air bits |
| flags | A5+519E | u32 × 110 | unit flags |

All values match Wargus' `units.lua` for the sampled units (footman 60 HP, armor 2, 6+3, 600 gold,
time 60). Full dump: `re68k\unit_types.csv`, field addresses `re68k\unit_fields.csv`.

## 3. Main loop and game-cycle timing

- Threads: the game runs as a Thread Manager thread (created by 0x00111F14, entry 0x00111F02); the
  main thread runs the Mac event loop `App_EventLoop` 0x00111D5C (WaitNextEvent, SystemTask every 20
  polls, `_ThreadDispatch`). Mac events are converted to 16-byte game events in a 16-entry ring at
  `*(A5+39A8)` (+0x100 head, +0x101 tail, +0x102 count; push 0x001794AA, pop 0x0017962A).
- Clock: a **5 ms Time Manager task** (installed 0x0019AAE2, handler 0x0019AB58) advances a ms
  counter `A5+3490`. During a game, 0x00138922 hooks 0x00138900 which adds 5 to the **game clock
  `A5+606E` (ms)** and raises a 20 ms flag `A5+6074` (palette cycling). `_TickCount` is not used.
- Speed: pref "speed" 0..6 (default 5) in `A5+378C`; keys +/- change it and the change is sent as a
  network command. Milliseconds per cycle come from a 3-phase table `A5+16F0[speed*3+phase]`
  (phase cycles 0,1,2 in `A5+607F`, read by 0x00138828); demo mode forces speed 5. Expected cycles
  per 10 s are stored at `A5+1706`.

| speed | ms/cycle (phases) | avg | cycles/s |
|---|---|---|---|
| 0 | 60 60 60 | 60.0 | 16.7 |
| 1 | 50 50 60 | 53.3 | 18.75 |
| 2 | 40 50 50 | 46.7 | 21.4 |
| 3 | 40 40 40 | 40.0 | 25 |
| 4 | 30 30 40 | 33.3 | 30 |
| 5 (default) | 20 30 30 | 26.7 | 37.5 |
| 6 | 20 20 20 | 20.0 | 50 |

- Session 0x00138C62: one cycle + draw, start clock, wait for net sync, run main loop 0x00138B1E,
  tear down.
- **Main loop 0x00138B1E** (while state `A5+60A6`==3): (1) resync — if speed changed, flag `A5+3A00`,
  or |next-cycle time `A5+607A` − clock| > 500 ms, set next = clock (time is dropped, no catch-up);
  (2) input — pop one event, dispatch via table `A5+1A3E[type]`; (3) simulation — while next <= clock,
  run **at most 3 cycles per loop pass** (0x00138A2C, the frame skip); network-turn countdown
  `A5+607E` reloaded from `A5+1680[latency*7+speed]` (≈ constant wall time per net turn), simulation
  stalls when the net exchange is not ready; (4) draw once per pass (0x00138AFA: viewport, panels,
  minimap, palette cycling, music check every 2 s, message timeouts). No sleep; yields via thread
  switch.
- **One game cycle 0x001389DA** (skipped while paused `A5+6072/6073`), in this order:
  1. per-unit visibility masks (f14_1016; 600 units at `*(A5+6056)`, stride 0x90);
  2. computer AI per player (f13_062A → 0x0013C3D4); every 50 cycles strategic AI (f8_0A5E,
     0x001685EC) and per-unit AI f8_1084 (counters `A5+573A`, `A5+573C`);
  3. every 100 cycles fog snapshot f8_4DDE;
  4. f13_4EB8: all unit actions, then 200 missiles at `*(A5+605A)` (stride 0x40) via class table
     `A5+290A`, then 0x0016BAE2, 0x0016BBFA, f14_0532;
  5. visibility f14_07FE; 6. minimap update f8_5556 every 40 cycles; 7. every 100 cycles fog diff
     f8_4E56; 8. f14_343E (callback every 200 cycles).

## 4. Random number generator

- `Random` 0x00188016 (JT A5-0708): **`seed = seed * 0x015A4E35 + 1` (u32); return
  `(seed >> 16) & 0x7FFF`**. Seed u32 at **A5+360A**; `SeedRandom` 0x0018800E.
- Seeding: startup and network init `SeedRandom(0)`; multiplayer start message 0x12 (0x0012C366)
  seeds with a packet byte for slot shuffling, then **`SeedRandom(0)` before play — every game
  starts with seed 0** on all machines. Single player (f9_4128 → 0x0014BE32) seeds with `time()` to
  pick human/computer slots and does not reseed (so SP games are not reproducible in the original).
- A second generator (C library `rand`, 0x001B000E: `s*0x41C64E6D+0x3039`, seed A5-3F50) is used
  only for menu/slot shuffles, never in the simulation.
- Notable callers: damage (§5), unit creation 0x0016C6CA (facing `r&7`, unit+0x80 = `r%6`, anim
  phase), idle turning/critter wandering 0x0012130E, missile spawn jitter 0x001295DE (`(r&7)-3` per
  axis), area spells 0x0012A140/0x0012A1AE, whirlwind 0x0012A8C0/0x0012A9BE, AI 0x0013C49A
  (`r%600`), 0x0014135E, 0x0014155E, 0x0014168A, 0x00141D06. Several missile/spell routines call
  `Random` and discard the result (0x00129B24, 0x00129B94, 0x0012A46C, 0x0012A6EC, 0x0012A7A4,
  f6_4DB0) — a sync-exact reimplementation must make the same calls in the same order.

## 6. Pathfinding

Not A* and not a flood fill: a **local line + wall-following ("bug") planner** that plans at most
**20 steps** at a time, with smoothing.

- Per-unit path: unit+0x30 = 20 direction bytes (0xFF-terminated), unit+0x7E = index (20 = invalid,
  forces replan). Goal tile unit+0x84/+0x86 or target unit unit+0x88.
- Directions 0=N clockwise; dx table A5-3FB0 `{0,1,1,1,0,-1,-1,-1}`, dy A5-3FA8
  `{-1,-1,0,1,1,1,0,-1}`; doubled tables A5-3FA0/A5-3F98 for units with unit+0x1C bit 2 (2-tile steps).
- Map buffers: tile flags u16 `*(A5+6A26)` (0x100 ground unit, 0x200 air unit present; unit pointer
  maps `*(A5+6A2A)`, `*(A5+6A2E)`), scratch direction map `*(A5+6A42)`. Map width `A5+6DA4`.
- Entry 0x0016B430: blocking mask `A5+56AA` = `A5+1714[movement type unit+0x2A]` =
  {0x09CE, 0x0200, 0x0903, 0x0901, 0, 6, 0xA, 0xA}; goal ring marked with 0x8000 (0x0016B076).
- Planner 0x0016B248: (1) Bresenham line to the goal (0x0016AEA8; line helpers 0x00142E06/0x00142EA6)
  — done if goal or 20 steps reached, else mark the last free tile 0x2000 and the rest of the line
  0x8000; (2) left-hand wall follow 0x0016A9AE, max 50 steps, succeeds on reaching an 0x8000 tile,
  loop erasure 0x0016A8F2, passable-unit test 0x0016A942; (3) exit check 0x0016B18A; (4) smoothing
  0x0016AB9E with pair table A5-4D26 `[d1*8+d2]` (<8 merge, 0x80|d replace both, 9 swap, 8 keep,
  0x40 reject); (5) if cost < 8 accept, else also try right-hand and keep the shorter; (6) fall back
  to the partial line; (7) copy ≤ 20 steps (0x0016ADEA).
- Blocked step (0x00150906): wait unit+7 = 15 cycles, retry counter +0x1000 in unit+0x1C; wait if the
  blocker is moving away (0x001507BC), otherwise replan; from 8 retries always replan; at 15 give up
  (f13_538A). Computer units blocked by a unit issue a command instead (f13_4166).
- `showpath` debug flag = bit 0x10000000 of the debug flags (toggle 0x0013033A); drawing site unknown.

## 5. Damage

All checked against the disassembly (Ghidra drops arguments in this chain).

- **`CalcDamage` 0x00131FFE (attacker, target):** `d = max(basic' − armor', 0) + piercing'` (armor
  only when there is a target). If `d == 0` → 0 (no damage). Else `h = (d+1) >> 1`,
  **`dmg = h + Random() % (h+1)`** (unsigned 32-bit modulo), capped at 255. Range =
  ceil(d/2) .. 2·ceil(d/2) (d=9 → 5..10). **There is no "50–100 %" roll and no minimum of 1 beyond
  this; d=0 deals nothing.**
- `basic'` 0x00131D9A: basic_damage (A5+4CAA); if weapons_upgradable: naval (flags&8) + level
  `A5+6D1A[owner]`×5, siege (flags&4) + level `A5+711E[owner]`×15, others except types 8/9/18/19 +
  level `A5+6CDA[owner]`×2. Doubled while bloodlust (unit+0x48) > 0. Ranger (18) with
  `A5+715E[owner]` (Marksmanship) returns 0 here…
- `piercing'` 0x00131E9E: piercing_damage (A5+4D18); archers/axethrowers/rangers/berserkers +
  level `A5+6CCA[owner]`×1; …and the ranger's basic damage is added here instead (armor-piercing).
  Doubled by bloodlust.
- `armor'` 0x00131F8A: armor (A5+4B60); if armor_upgradable + level `A5+6CFA[owner]`×2 (land) or
  `A5+6D0A[owner]`×5 (naval). Bonus-per-level bytes at A5+223E = `01 02 02 05 05 0A 0F`.
- Attack rating 0x00131F4C = `min(255, basic'+piercing')` (no armor, no random) = launch damage of
  splash missiles.
- Hit event: animation opcode 0x0B → 0x001201CC → order hook `A5+1D94[order]` (attack hook
  0x0012016A). Melee (missile == 29 "none"): immediate `DamageUnit` with CalcDamage, or
  `DamageWallTile` 0x001320A8 when no target unit; otherwise launch a missile 0x001295DE.
- `DamageUnit` 0x00132148: ignore if target+0x1E & 0xF; attack reaction 0x00120E86; ignore if
  unholy armor (target+0x46) or dmg 0; `dmg >= hp` → kill (0x00159590 score, 0x0016C29C kill,
  0x00150A22 credit) else `hp -= dmg`.
- Walls: add `max(dmg>>2, 1)` to bits 11–15 of the map word `*(A5+6A22)`; ≥ 20 destroys the wall.
- Splash (missile types with A5+2988 = 1: big cannon, rock, bolt, small cannon; 0x00129D52):
  walls on centre + 4 neighbours; units in a 7×7 tile area hit once each (0x00129C54): with
  `m = max(dx², dy²)` to unit centre, m ≤ 0x1FF (≈22 px) full, m ≤ 0x6FF (≈42 px) damage/4, else
  none; minus armor unless `A5+296A[type]`; skip if ≤ 0; then the same `h + Random()%(h+1)`; passed as
  a byte (255 can wrap).
- God-mode cheat (A5+6878 bit 2) gives piercing 200 to the human side.

## 7. Unit and missile runtime records

600 unit slots × 0x90 bytes at `*(A5+6056)`; per-player lists linked via +0x68 headed at
`A5+5630[16]`, counts `A5+5674`, per-player cap `A5+5672` = 600 / active players. Free slot: owner
0x0E, +0x1E = 1. `CreateUnit` 0x0016C6CA (zeroes the record), unit tick 0x0016CEB8, timers
0x0016BBFA (once per unit pass).

| off | field | off | field |
|---|---|---|---|
| 00/02 | x, y pixels (s16) | 27 | unit type |
| 04 | anim script offset (u16) | 2A | unit kind (movement type) |
| 06 | redraw flags (0x20 dirty) | 2C / 2D | owner / original owner |
| 07 | anim wait counter | 2E | order (indexes A5+1D94/1D56/1D18/1C9E/1F7C) |
| 08 | anim kind | 30..43 | path: 20 direction bytes |
| 09 | sprite frame | 44 | invisibility timer |
| 0A | direction 0..7 (N, clockwise) | 46 | unholy armor timer |
| 0B | pending sound | 48 | bloodlust timer |
| 14 | serial (u32, from A5+165E) | 4A | haste (>0) / slow (<0) timer |
| 18/1A | tile x, y | 68 | next unit in list |
| 1C | kind flags (A5+171C[kind]); retries in top nibble | 74 | decay counter (A5+4F3E; −5 HP or −1 mana) |
| 1E | state flags (1 free, 0x100 constructing, 0x800/0x1000 anim paused, 0x4000 no attack) | 7E | path index (20 = invalid) |
| 22 | hit points | 84/86 | target tile |
| 26 | mana / regen counter | 88 | target unit pointer |

Missiles: 200 × 0x40 at `*(A5+605A)`; +00..0A as units, +20 remaining distance, +24/+26 arc, +28/+2A
target px, +2C target unit, +30 source unit, +34 type, +35 flags (1 free, 2 flying), +36 state,
+37 damage. Launch 0x001295DE: start = unit pos + per-type offset (`*(A5+60A8)`), target jitter
`(r&7)-3` per axis, distance `max(chebyshev, A5+294C[type]*32)`. Speed `A5+29A6[type]` px/tick
(arrow/axe 12, bolt/rock 8, cannon/lightning/sub/turtle 16). State handlers `A5+290A`: 0 launch,
1/3/4 straight, 2 arc (heights A5+2946), 5 effect, 6 impact 0x00129EEA, 7 breath.

## 8. Animation tables

Not in the code image: **`War Data` entry 278** (resource 1278), loaded by 0x0015A386 into the
handle `*(A5+5A26)`, little-endian. `tab[140]` = offsets of sub-tables, index = unit type 0..109,
then 110 + missile type. Unit sub-table: 7 script offsets (corpses 105–109: 6); missile: 3. Kinds:
0 none, 1 death, 2 still, 3 move, 4 attack, 5 work (building), 6 transport step; order→kind table
A5+1D56. Set by 0x0015A3E0 (units) / 0x0015A42E (missiles): +4 = script, +7 = 1, +8 = kind.

Interpreter `AnimStep` 0x0019945A: each cycle `--wait`; at 0 run opcodes until one sets a wait.

| op | len | meaning |
|---|---|---|
| 00 | 1 | yield (wait 1, return 1 → order logic runs this cycle) |
| 01 n | 2 | wait n |
| 02 | 1 | halt |
| 03 lo hi | 3 | goto offset |
| 04 f | 2 | frame f |
| 05 n | 2 | move n px along direction (±1 tables) |
| 06 n f | 3 | move n, frame f |
| 07 n | 2 | wait n; ×2 slowed, /2 (min 1) hasted |
| 08 n | 2 | wait n; /2 if hasted **or** slowed (original bug) |
| 09 n | 2 | wait n; ×2 if slowed |
| 0A | 1 | play sound (unit+0x0B) |
| 0B | 1 | attack event (damage / missile) |
| 0C n | 2 | as 07 but n+1 when carrying (unit+0x75 & 0x20, uncertain) |
| 0D n / 0E n f | 2/3 | move with ±2 tables (ships, flyers) |
| 0F | 1 | change owner to 15 (corpses) |

Example footman attack: yield, frame 25, wait 3, 30, wait 3, 35, wait 3, sound, attack, 40, wait 5,
0, wait 10, loop (25 cycles per swing). Move speed = cycles per tile from the move scripts
(`re68k\anim_move_speed.csv`): footman/grunt/peasant/archer 16 per 32 px, knight/ogre 12,
mage/DK 18, sappers 14, ballista/catapult 32, skeleton 20, critter 48; ships 32 per 64 px,
battleship 53, submarine 43, flying machine/zeppelin 20, gryphon/dragon/daemon 24, eye 8. Wargus
`Speed` ≈ 5·px/cycles. Full dump: `re68k\animations.csv` (`dump_anims.py`).

## 9. Screen drawing

**Frame buffer and update path.** Offscreen 640×480×8 (allocated 0x0017D376, descriptor A5+38BA,
32-byte aligned); current draw surface pointer A5-43FC (opaque blit 0x0017832C, transparent blit
0x0017848E with index 0 transparent, lines 0x00178400/0x0017860A, rect 0x0017855E, clip
0x0017B72E). **Layers:** 24 × 20-byte records at A5+38C2 `{active, flags, x, y, w, h, param,
callback}`; **dirty grid 40×30 cells of 16×16 px** at A5+38C6 (mark rect 0x0017B144, whole layer
0x0017B20E). Per frame: changed menu panels (0x00181A8C), then layers redrawn from 23 down to 0
(0x0017B5B2), then dirty cells merged into rectangles and copied to the screen (0x0017B31A →
0x00199856). Layers: 0 software cursor (save-under), 6/7 full-screen images (+ building placement
ghost), 8 map viewport, 9 top-left buttons, 10 resource bar, 11 status line, 12 minimap, 13 right
filler, 14 command buttons, 15 unit info panel. Screen copy: direct 16-byte writes (MOVE16 on 040)
when pref A5+72E7 == 0 ("More Compatible Drawing" off, likely), window unobscured, on one 8-bit
device (0x0011331C, 0x001999C4); else `CopyBits` srcCopy from a PixMap over the buffer
(0x001998F8). A5+348A > 0 suspends output.

**Map tiles.** Not drawn to the frame directly: a **256 KB tile ring** (`*(A5+6A46)`, 512-byte rows,
address `(y*512+x) mod 0x40000` = 16×16-tile window). One 32×32 tile = 4×4 minitiles of 8×8
(0x00198424): megatile table `*(A5+6A1E)` (16 words/tile), minitile pixels `*(A5+6A4A)`, index
`(word & 0xFFFC)*16`, bit 1 = h-flip, bit 0 = v-flip; map cells u16 `*(A5+6A22)`. **Viewport 14×14
tiles = 448×448 px at screen (176,16)** (constants A5+1662..166C); top-left tile A5+6DB2/6DB4.
Scrolling is whole tiles: full redraw 0x0012AF24, per-direction scrolls (e.g. 0x0012AD68) draw only
the exposed row/column. Viewport callback 0x001391AA: copy ring → frame (all, or only dirty 16-px
runs 0x001390D8), then moving units 0x00171440, missiles 0x001713C0, fog 0x00138F8A, messages
0x00144AD0, drag rectangle 0x00172C74.

**Sprites.** PC GRP format (header 0x0017AFB0: 8-byte frames `{x,y,w,h,u32 offset}`, offset bit 31 =
width+256; rows: LE row offsets, codes `0x80|n` skip, `0x40|n` repeat next byte, else n literals).
Decoders 0x00198F88 (normal), 0x001990EA (mirrored), 0x00199264 (into the ring with wrap); all
through a 256-byte remap table A5-40B0. Directions 0–4 normal, 5–7 mirrored (function table
A5-4CD0). **Player colours** (0x001701BA): remap 0xD0–0xD3 → base..base+3, bases (A5-4C90)
D0, D4, D8, DC, E0, E4, E8, 0C; minimap colours A5-4C80. Buildings and remembered "ghost" buildings
are drawn into the tile ring (draw mode A5+561C = 2; ghost list A5+5612, 25 × 0x90); moving units
are redrawn into the frame each viewport draw, ground pass then air pass (0x001710B4); flyers get
a shadow ~42 px lower (approx.). Selection box 0x001709F0 (size from box-size table), colours
A5+1724: own 0xFA, others 0xFB/0xF9 by diplomacy (which is which uncertain). Drag rectangle 0xFA.

**Fog of war.** Maps `*(A5+6A32)` = not-visible, `*(A5+6A3A)` = unexplored; byte 0 clear, 0x10
covered, 1..15 boundary shapes (tileset megatiles 1–15). Per tile (0x00138EE4, dirty cells only):
both 0x10 → solid black 0xFF; not-visible 0x10 + explored → **50 % checkerboard of 0xFF**; mixed
values → dithered edge (0x00198C9A) and/or black edge (0x001986DC). Every 100 cycles the fog map is
snapshot to `*(A5+6A36)` and changed tiles marked dirty (0x00144DDE / 0x00144E56). A5+6DA8 =
reveal map.

**Minimap** (0x00145106, layer 12). Panel (0,24) 176×136; **128×128 image at screen (24,26)**.
Refresh every 40 calls (A5+5DD2) or within 4 after scrolling (0x00145556); terrain image
(`*(A5+5DE6)`) rebuilt every 6th refresh. Terrain colour = pixel (7,6) of a minitile of the
megatile; scale by map size 32→×4, 64→×2, 96→×4/3, 128→×1. Fog: checkerboard alternately tests the
not-visible / unexplored maps, non-zero → 0xFF. Units (0x001453AA): filled rects min 2 px, own 0xFA,
others A5-4C80[colour], gold mine 0xFF, critter 0xF7. Viewport frame outline 0xF6.

**Palette.** 6-bit RGB scaled `v<<2 | v<<10` into a CTab (A5+351C), applied with AnimatePalette or
CTab2Palette+ActivatePalette (0x00199CFE); Mac-specific: entries 0xF6–0xFF reserved, the code uses
0xFF as black. Fades 0x0017D060/0x0017D0C8 (paced by ms clock). Colour cycling: 8 slots × 16 B at
A5+36E0 `{on, delay, counter, first, last, table or 0=rotate, len}`, per-tileset definitions via
pointers at A5-8000 (ranges CA, 26–2F, 40–46, F0–F4, 03); only when pref A5+37A8 bit 0x40.

## 10. UI layout (640×480)

From the War Resources menu records (0x48-byte LE records, entry = menu id − 3000) and code.

| area | screen rect | menu id / image | layer | notes |
|---|---|---|---|---|
| top-left bar | (0,0) 176×24 | 0xBDF | 9 | buttons (4,2) 80×20, (88,2) 80×20, (24,2) 128×20 |
| minimap panel | (0,24) 176×136 | image 0x50F+race | 12 | minimap (24,26) 128×128 |
| unit info panel | (0,160) 176×176 | 0xBDD | 15 | text lines (60,170/185/200) 109×14; stats x=9/105, y=232+15n; multi-select 3×3 of 54×53 at x 5/61/117, y 165/220/275; progress bar (12,313) 152×14; icon (107,238) |
| command buttons | (0,336) 176×144 | 0xBDE | 14 | **3×3 buttons 54×46 at x 5/61/117, y 336/383/430** (icons +4) |
| resource bar | (176,0) 448×16 | 0xBDA / 0xBDB, image 0x507 | 10 | 0xBDA: three 88×16 widgets at x 264/352/440; 0xBDB: x 192/280/368 + two 80×15 buttons at 456/536; icon + number at icon width + 18 (0x00168B2C). **Differs from PC (176/251/326).** |
| map viewport | (176,16) 448×448 | — | 8 | 14×14 tiles |
| status line | (176,464) 448×16 | 0xBD9, image 0x50B | 11 | text (176,466) 290×14; 52×16 boxes x 466/518/570 |
| right filler | (624,0) 16×480 | 0xBDC, image 0x509 | 13 | |
| messages | on viewport | — | — | up to 8 lines at x 186, y 26 + n·line height (A5+5E44) |

## 11. Open questions

- Exact names of order indexes (tables A5+1D94/1D56/1D18/1C9E/1F7C) and anim kinds 5/6.
- unit+0x75 bit 0x20 (likely "carrying resource"); A5+6D4A meaning (human/computer?).
- Spell missiles (states 7–13) not decoded beyond dragon breath; `showpath` draw site unknown.
- Fog edge-shape pixel rules; selection colour ally/enemy assignment; A5+72E7 = "More Compatible
  Drawing" not proven.
- Pathfinding exit-check rule (0x0016B18A) and the 2-tile step mode purpose.
- AI (f13_062A, f8_0A5E, f8_1084) not documented yet.
