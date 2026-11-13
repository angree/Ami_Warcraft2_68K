# SPEC_AI — the computer player of the original (Warcraft II Mac 1.23, 68k)

Behaviour specification of the computer opponent, written from the Ghidra decompilation and
disassembly of the original Mac binary. Clean-room rules: prose and tables only, no code. Addresses
are image addresses as in `docs/RE_NOTES.md` (image at 0x00100000, A5 = 0x00210000, globals written
`A5+xxxx`). Unit record offsets refer to RE_NOTES §7 and SPEC_MECHANICS §0. Unit flag names
(worker 0x100, tanker 0x200, transport 0x400, oil platform 0x800, stores-gold 0x1000, ...) are the
ones listed in SPEC_MECHANICS §0.

Confidence tags: **CONFIRMED** = read directly from code (decompiler output checked against the
disassembly wherever Ghidra dropped arguments), **LIKELY** = strong inference, **UNKNOWN** = not
settled. **Wargus≠** marks a place where Wargus/Stratagus behaves differently.

Summary in one paragraph: the computer player is a **tiny byte-code script** (one per player, from
`War Data` entry 277) that only sets ~25 "desired" numbers and trigger flags and waits on 8
conditions. Everything else is hard-wired C logic: **buildings** periodically decide what to train or
research from those numbers, **idle workers** walk an ordered build/research list and balance
gold/lumber, every **50 cycles** a strategic pass launches attack waves at the human player with the
most units and gives each computer unit a **role** (guard, attacker, transport, explorer, escort),
and casters pick spells from fixed per-type priority lists. There is **no difficulty setting and no
resource cheat** for the computer.

---

## 1. Who is a computer player, and when the AI runs

- Controller byte per player `A5+6D4A[p]` (16 entries). At game start the PUD owner codes are
  remapped (0x0014BBB6, 0x0014BDAA, 0x0014C01E, 0x0014BB86): human 5 → **0** (human),
  computer 4 → **1** (computer), rescue-passive 6 → **2**, rescue-active 7 → **1** (run by the AI),
  unused human/computer slots → **3** (nobody). Only value **1** is driven by the AI. CONFIRMED.
  (SPEC_MECHANICS calls 3 "passive"; the remap above shows 2 = passive, 3 = nobody — LIKELY.)
- Only players 0..7 get a script and the per-cycle/50-cycle passes (loops bounded by 8 in
  0x0013C3D4, 0x00140A5E, 0x00141084). CONFIRMED.
- Order inside one game cycle (RE_NOTES §3, 0x001389DA): the AI runs **after** the per-unit
  visibility masks and **before** unit actions. AI_Tick 0x0016862A:
  1. every cycle: script interpreter for each computer player (0x0013C3D4 → 0x0013C28E);
  2. every 50 cycles (counter `A5+573A`, reload 50): zone map + attack-wave launcher
     (0x00140A5E), then the "no workers" tech fallback (0x001685EC);
  3. every 50 cycles (independent counter `A5+573C`, reload 50): base bounding boxes and per-unit
     role handlers (0x00141084).
  Both counters are set to **1** at game start (0x001684D8), so both passes run on the very first
  cycle and then on cycles 51, 101, ... CONFIRMED.
- Outside these passes the AI also acts through hooks (all CONFIRMED):
  - a completed computer building gets its production handler every few idle steps (§4);
  - an idle computer worker/tanker re-thinks whenever its idle order runs and after every delivery
    (§5);
  - unit creation (0x0016865A) assigns a role (§7) and ticks the build list;
  - unit death (0x0016870E) un-ticks the build list and may trigger the last stand (§6.5);
  - being hit triggers the call for help with a 13-tile radius for computer units (§8).
- Per-player AI memory (all cleared at game start by 0x001684D8): script context 8 × 0x34 bytes at
  `*(A5+5F5E)`; list "done" flags 16 × 64 bytes at `*(A5+5736)`; research "requested" flags
  16 × 27 bytes at `*(A5+7236)`; worker job counters `*(A5+5C54)` gold, `*(A5+5C58)` lumber,
  `*(A5+5C5C)` repair (u16 per player); buildings-being-started counters 16 × 47 u16 at
  `*(A5+5C60)`; town-hall retry cooldown `A5+5C64[p]` (0x0015236A). CONFIRMED.

## 2. The AI script (War Data entry 277)

### 2.1 Container

- Loaded at game start (0x0013C2D8) into `A5+5F72`; freed at the end (0x0013C2EE). Little-endian.
- Starts with a table of u16 offsets, one per script, terminated by 0 (32 scripts in the Mac data).
- A player's script number comes from the PUD **AIPL** section (16 bytes, handler 0x00144334 →
  `A5+5F62[p]`). CONFIRMED. The script numbers line up with the AI names in `War Strings` 1 (index 0
  "Land Attack", 1 "Passive", 25 "Sea Attack", 26 "Air Attack", the rest campaign scripts
  "_Human N" / "_Orc N") — LIKELY (same order, same count).
- Script header (0x0013C302): two u16 file offsets, then the program.
  - **Build/research list** (first offset): up to 64 bytes, 0xFF-terminated (§5.3).
  - **Building think periods** (second offset): u16 per building type, index = type − 0x3A;
    0xFFFF = this building type never runs its production handler (§4). Several scripts share
    these tables.
  CONFIRMED.

### 2.2 Script context (per player, 0x34 bytes)

| off | meaning | conf. |
|---|---|---|
| +0x00 | sleep counter (u32) | CONFIRMED |
| +0x04 | program counter | CONFIRMED |
| +0x08 | script variable, never read by code | CONFIRMED (no reader found) |
| +0x09 / +0x0A / +0x0B | trigger: launch land / sea / air attack at the next 50-cycle pass | CONFIRMED |
| +0x0C | transports pick a random coastal landing tile (set to 1 by default) | CONFIRMED |
| +0x0D / +0x0E | land wave: units per group / number of groups | CONFIRMED |
| +0x0F / +0x10 | sea wave: units per group / number of groups | CONFIRMED |
| +0x11 / +0x12 | air wave: units per group / number of groups | CONFIRMED |
| +0x13 | wanted workers | CONFIRMED |
| +0x14 | wanted footmen/grunts | CONFIRMED |
| +0x15 | wanted archers/axethrowers (rangers/berserkers after the upgrade) | CONFIRMED |
| +0x16 | wanted ballistas/catapults | CONFIRMED |
| +0x17 | wanted knights/ogres (paladins/ogre-mages after the upgrade) | CONFIRMED |
| +0x18 | wanted oil tankers | CONFIRMED |
| +0x19 | wanted destroyers | CONFIRMED |
| +0x1A | wanted transports | CONFIRMED |
| +0x1B | wanted battleships/juggernauts | CONFIRMED |
| +0x1C | wanted submarines/turtles | CONFIRMED |
| +0x1D | wanted mages/death knights | CONFIRMED |
| +0x1E | wanted flying machines/zeppelins | CONFIRMED |
| +0x1F | script variable (dwarves/sappers by position), never read | CONFIRMED (no reader) |
| +0x20 | wanted gryphon riders/dragons | CONFIRMED |
| +0x21 | script variable, never read | CONFIRMED (no reader) |
| +0x22 | **tech limit**: how many entries of the build list may be used (starts 0xFF, scripts count it up) | CONFIRMED |
| +0x24 / +0x28 | pointers to the build list / period table | CONFIRMED |
| +0x2C..+0x2F | base bounding box (min y, max x, max y, min x), §7.1 | CONFIRMED |
| +0x30 | rotating side index for role 5 (§7.6) | CONFIRMED |

"Wanted" values are compared with the per-player **army counters** (§3), not with total unit counts.

### 2.3 Instruction set (interpreter 0x0013C28E, opcode table `A5+0D1C`)

Each cycle, for each computer player: if the sleep counter is non-zero it is decremented and nothing
else happens; otherwise instructions are executed until one sets a non-zero sleep counter.

| op | bytes | effect | conf. |
|---|---|---|---|
| 00 | `00 var val` | context byte `var` := `val` | CONFIRMED (0x0013C09C) |
| 01 | `01 lo hi` | jump to file offset (u16) | CONFIRMED (0x0013C0BE) |
| 02 | `02 n0 n1 n2 n3` | sleep: counter := u32 n (the next n cycles are skipped) | CONFIRMED (0x0013C26C) |
| 03 | `03 c` | wait until condition c: if true continue after `c`, else rewind to this instruction; either way sleep 1 | CONFIRMED (0x0013C22E) |

Consequences: a passing condition costs one idle cycle; a failing one is re-tested every **2**
cycles. A sleep of n resumes on the (n+1)-th following cycle. No arithmetic, no calls, no stack.

Conditions (table `A5+0D2C`, CONFIRMED each):

| c | true when | address |
|---|---|---|
| 0 | the player owns ≥1 complete shipyard | 0x0013C0E4 |
| 1 | owns ≥1 complete keep/stronghold | 0x0013C0FC |
| 2 | owns ≥1 complete castle/fortress | 0x0013C114 |
| 3 | workers ≥ wanted workers (+0x13) | 0x0013C12C |
| 4 | land army ≥ (+0x0D) × (+0x0E) | 0x0013C18C |
| 5 | naval army ≥ (+0x0F) × (+0x10) | 0x0013C1C2 |
| 6 | air army ≥ (+0x11) × (+0x12) | 0x0013C1F8 |
| 7 | some enemy player (diplomacy matrix `A5+6F3A`) among players 0..7 still has workers | 0x0013C152 |

### 2.4 What the shipped scripts do (qualitative)

Read from the player's own data for orientation only; the engine must interpret the data, not
copy it. LIKELY as a description.

- **Passive**: empty list, only a jump into an idle loop → builds and trains nothing, never attacks.
- **Land Attack**: reset all variables; tech limit 3; 1 worker, wait for it; then 9 workers and a
  few footmen; then a repeating pattern: raise wanted units and the land group size, **wait until
  the army reaches group × groups**, *lower* the group size by 2, set the land trigger, sleep
  **500 cycles**, raise the tech limit by a few entries. Waves grow (first wave 1 unit, later 3, 5,
  7, 9 out of 11), keep → castle are awaited with conditions 1/2, mages and gryphons appear late,
  workers rise 9 → 15 → 19 → 25. Ends in an endless loop of identical waves.
  Because the group size is lowered after the wait, **the AI always keeps about two land units at
  home** that are not sent.
- **Sea Attack / Air Attack**: the same pattern with the sea/air variables, tankers and a shipyard
  early, plus a `wait 3000` before some waves.
- **Campaign scripts**: mostly timed (sleeps of 3000–15000 cycles between waves) instead of only
  army-size triggered.

**Wargus≠**: Wargus' Lua AI (forces 0..8, `AiNeed`/`AiForce`/`AiWaitForce`, difficulty-scaled
copies) is a re-creation, not the original; the original has exactly the four opcodes above and
the fixed variable set.

## 3. Counters the AI reads

Maintained by unit creation/completion and death (0x001314CE add, 0x001315A4 remove; per-type
pointer tables `A5+2628` and `A5+2470`). Only **complete** units count. CONFIRMED.

- Total of a class, per player (`A5+2628[type]`): e.g. workers `A5+660E` (peasant, peon and attack
  peasant/peon), town halls `A5+61EE`, keeps `A5+620E`, castles `A5+622E`, barracks `A5+626E`,
  towers (scout, guard, cannon) `A5+628E`, lumber mill `A5+62AE`, blacksmith `A5+62CE`, stables
  `A5+62EE`, farms `A5+630E`, church `A5+632E`, inventor `A5+634E`, aviary `A5+636E`, shipyard
  `A5+638E`, foundry `A5+63AE`, refinery `A5+63CE`, oil platforms `A5+624E`, tankers `A5+662E`.
  Mage tower/temple has **no** class counter (pointer is null).
- **Army counters** (`A5+2470[type]`, updated by 0x00131358): footman class `A5+66EE`,
  archer+ranger class `A5+670E`, ballista/catapult `A5+672E`, knight+paladin `A5+674E`, destroyer
  `A5+676E`, transport `A5+678E`, battleship `A5+67AE`, submarine `A5+67CE`, mage/DK `A5+67EE`,
  flyer `A5+680E`, dwarves `A5+682E`, gryphon/dragon `A5+684E`; plus the three domain totals:
  **land army** `A5+668E` (mobile, can attack, not naval, not air, not worker), **naval army**
  `A5+66AE` (naval, can attack), **air army** `A5+66CE` (air, can attack).
- Units pre-placed by the PUD with a non-zero "data" byte are flagged **guard** (+0x5F bit 2) and are
  **excluded from the army counters** and from attack waves; they get role 1 at their own tile
  (0x00143EDE). CONFIRMED.

## 4. Building production (train, research, tower and hall upgrades)

### 4.1 When a building thinks (0x0013272E, order 33 "building idle")

A complete, computer-owned building in its idle order increments its own counter (+0x6E) each time
the idle order runs; when the previous counter value exceeds the script's period for its type, the
counter is reset and the type's handler (table `A5+2B7A`, index type − 0x3A) runs. Period 0xFFFF =
never. CONFIRMED. The building idle animation yields every 5 cycles (yield + wait 4), so the handler
runs about every **(period + 2) × 5 cycles** (period 1 → 15 cycles). LIKELY (cadence from the
animation data). Every handler first requires the building to be complete and **not already
producing** (+0x1C bit 0x10). CONFIRMED.

### 4.2 Unit choice

"Need first" below means *wanted > 0 and have = 0*; "short" means *have < wanted* (army counters).
Each "train X" step checks its building prerequisites and returns **without falling through** once
it has tried (even when the start fails for lack of money/food), so a shortage blocks the rest of
the chain until the next think. CONFIRMED (0x0012894C..0x00128C2E all return 1 after calling the
production starter 0x0012438C).

**Barracks** (0x00128C56), first match wins:

| step | condition | trains |
|---|---|---|
| 1 | footman need first | footman/grunt |
| 2 | knight need first (needs blacksmith + stables) | knight/ogre, or paladin/ogre-mage once that upgrade is researched (`A5+6ACA` bit 0x100000) |
| 3 | archer need first (needs lumber mill) | archer/axethrower, or ranger/berserker once researched (`A5+712E`) |
| 4 | archer short and `Random & 3 == 1` | archer class |
| 5 | catapult short and `Random & 7 == 1` (needs blacksmith + lumber mill) | ballista/catapult |
| 6 | knight short | knight class |
| 7 | footman short | footman |
| 8 | catapult need first | catapult |
| 9 | archer short | archer class |
| 10 | catapult short | catapult |

Random is called only when the preceding short-test is true (RNG order matters for sync). CONFIRMED.

**Shipyard** (0x00128DDA): tanker need first → tanker; destroyer need first → destroyer;
battleship need first (needs foundry) → battleship; destroyer short and `Random&3==1` → destroyer;
battleship short and `Random&7==1` → battleship; submarine short and `Random&7==1` (needs inventor)
→ submarine; transport short and `Random&7==1` (needs foundry) → transport; tanker short →
tanker; destroyer short → destroyer; battleship short → battleship; transport short → transport;
finally submarine short → submarine. CONFIRMED.

**Town hall / keep / castle** (0x00128F74): workers short → train peasant/peon. Otherwise a town
hall upgrades to keep when the list item "keep" (0x98) is requested and the player has a barracks
and both counters keep/castle allow it; a keep upgrades to castle when item 0x99 is requested and
the player has stables, blacksmith and lumber mill. The item is ticked when the upgrade starts.
CONFIRMED.

**Inventor** (0x001294B6): flying machine/zeppelin when short (needs lumber mill). **Aviary/roost**
(0x00129452): gryphon/dragon when short. Both skip their turn while the player has a castle and
still needs a first knight (so the knight is not starved of gold). CONFIRMED.

**Mage tower/temple** (0x0012935A): mage/DK need first → train; then research (§4.3); then mage
short → train. Same castle/knight skip rule. CONFIRMED.

**Scout tower** (0x00128D98): always upgrades itself to a **guard tower** when it thinks (never
to a cannon tower). CONFIRMED.

Farms, stables, refineries, platforms and other buildings have no handler. CONFIRMED.

### 4.3 Research

Blacksmith (0x001291E2), lumber mill (0x00129092), foundry (0x0012913A), church/altar (0x00129298)
and mage tower/temple walk a fixed table of research rows (12-byte rows at `A5+2C2E`, `A5+2CBE`,
`A5+2D1E`, `A5+2DC6`, `A5+2E3E`; even rows human, odd rows orc, chosen by the race byte
`A5+6D3A[p]`). A row starts its research when the AI has **requested** that research item (§5.3)
and it is neither researched nor in progress, and (for levelled upgrades) the current level equals
the row's level. Starting it ticks the item as done. Blacksmith and church/mage tower first give
priority to their first knight/mage need. CONFIRMED.

Research item ids (list bytes 0x80..0x9A; the second column is the UGRD upgrade index human/orc):

| item | building | upgrade | item | building | upgrade |
|---|---|---|---|---|---|
| 0x80 | lumber mill | arrows/axes 1 (4/6) | 0x8C | foundry | cannons 1 (12/14) |
| 0x81 | lumber mill | arrows/axes 2 (5/7) | 0x8D | foundry | cannons 2 (13/15) |
| 0x82 | lumber mill | ranger/berserker (24/28) | 0x8E | foundry | ship armor 1 (16/18) |
| 0x83 | lumber mill | marksmanship/regeneration (27/31) | 0x8F | foundry | ship armor 2 (17/19) |
| 0x84 | lumber mill | longbow/lighter axes (25/29) | 0x90 | church/altar | paladin/ogre-mage (33/32) |
| 0x85 | lumber mill | scouting (26/30) | 0x91 | church/altar | healing/bloodlust (35/44) |
| 0x86 | blacksmith | swords/axes 1 (0/2) | 0x92 | church/altar | exorcism/runes (36/50) |
| 0x87 | blacksmith | swords/axes 2 (1/3) | 0x93 | mage tower/temple | flame shield/raise dead (37/45) |
| 0x88 | blacksmith | shields 1 (8/10) | 0x94 | mage tower/temple | slow/haste (39/48) |
| 0x89 | blacksmith | shields 2 (9/11) | 0x95 | mage tower/temple | invisibility/whirlwind (40/47) |
| 0x8A | blacksmith | ballista/catapult 1 (22/20) | 0x96 | mage tower/temple | polymorph/unholy armor (41/49) |
| 0x8B | blacksmith | ballista/catapult 2 (23/21) | 0x97 | mage tower/temple | blizzard/death and decay (42/51) |
| 0x98 | town hall | upgrade to keep | 0x99 | keep | upgrade to castle |

CONFIRMED (table dump; the upgrade names follow the UGRD order).

## 5. Workers and tankers

### 5.1 When a worker thinks

Computer workers think from their idle order handler (0x00120D14 → 0x00152890) and after every
delivery (0x0013BA1E). Tankers use 0x00152952. If a worker finds nothing to do, its animation wait is
set to **44 + (Random & 15)** cycles (0x0015272A) before the idle handler runs again. CONFIRMED.

### 5.2 Worker decision order (0x00152890)

1. Release the job it had (gold/lumber/repair/build counters, and the build-site reservation).
2. Self-defence: the generic "attacked worker" reaction (0x00120C66); if it acts, stop.
3. Find a reachable depot (same land region, `A5+6A4E` region map) for its job (0x00151B04).
   - **No depot**: try to build a **town hall** (0x0015274E) — only if none is already being
     started by this player, then a 10-attempt cooldown `A5+5C64[p]`; the site is searched first
     with one tile of extra margin around the 4×4 footprint, then without (0x001529B6).
   - **Depot, carrying goods**: return them. Stop.
   - **Depot, empty**: farm rule (§5.4), then the build list (§5.3).
4. If not carrying: repair rule (§5.6), then gold/lumber assignment (§5.5).
5. Otherwise: random idle delay (above).
CONFIRMED.

### 5.3 The build/research list

The script's list (§2.1) is an ordered program of goals; bytes < 0x80 are **building types**
(race-normalised: the AI uses `type & 0xFE` plus its own race bit), bytes 0x80..0x9A are research
items (§4.3). Per player there is a **done** flag for each list position. The list is consulted by
an idle, empty worker that has a depot (0x00152546): walk the positions up to the **tech limit**
(+0x22) and up to 64; at the first position not done:

- research item → mark it **requested** (the owning building starts it, §4.3) and stop;
- building → try to build it (0x00151D7E):
  - prerequisites missing (per-type check table `A5-77BE` = 0x00208842) → stop the walk;
  - not affordable (0x00123F64) → stop the walk (the AI **saves up** for the next goal);
  - one of that type already being started by this player → skip to the next position;
  - no site found → skip to the next position;
  - otherwise the worker is sent to build it → done for this worker.

Town halls in the list use the expansion site search (§5.7); other buildings use per-type placement
routines (table at 0x002088FE, index type − 0x3A). CONFIRMED.

Done-flag bookkeeping (CONFIRMED):
- a building completed by the player ticks the first matching not-done position (0x0016865A);
- a building of the player that **dies un-ticks** the first matching done position (0x0016870E) →
  **the AI rebuilds lost buildings** in list order;
- at game start existing buildings tick their positions (0x001683D6; a keep also ticks the keep
  upgrade, a castle both upgrades), and already researched upgrades tick their research items
  (0x00168058, campaign carry-over);
- the list may contain the same building type several times (several barracks, many towers).
- Every 50 cycles, a computer player that has **no workers at all** marks its first not-done
  research item (up to the tech limit) as requested (0x001685EC/0x0016855C), so research can go on
  without workers.

### 5.4 Farms (0x0015260A)

A worker builds a farm (if prerequisites and money allow, one site search) when
**food used + 3 ≥ food supply + 4 × (farms this player is currently starting)**; food used =
non-building units minus summoned/critters (`A5+646E` − `A5+664E`), supply from `A5+640E`.
So the AI keeps ≥ 3 free food and does not over-build while farms are in progress. CONFIRMED (the
supply counter name from SPEC_MECHANICS §5, LIKELY).

### 5.5 Gold vs lumber (0x0015241C)

Target number of lumber workers L (W = current workers, gold G, lumber T, from `A5+6C4A`/`A5+6C0A`):

| case (first that applies) | L |
|---|---|
| G < 500 and W < 5 | 0 |
| T ≥ 2000 | 0 |
| G < 1000 and T ≥ 500 | 0 |
| T < 500 and G ≥ 1000 | (3·W + 3) / 4 |
| otherwise | W / 2 |

If the current lumber-job count is below L, the worker goes chopping (tree search 0x0016A9AE region,
via 0x0016D9AE "nearest tree" helper; order harvest). Otherwise it goes to gold; if no mine is
usable it chops anyway. CONFIRMED.

**Mine choice** (0x00151C22): among gold mines in the worker's land region with loads left, minimise
`dist(depot, mine) + dist(mine, worker)/2`. After choosing, the tiles on a straight line from the
depot to the mine are flagged **no-build** (map flag 0x400, 0x00151B7A) so later AI buildings keep
the mining path free. CONFIRMED. Several AI workers may use the same mine (SPEC_MECHANICS §3.1).

### 5.6 Repair (0x001522E0, 0x001521C6)

At most **(W − 1)/2 + 1** workers repair at the same time. Candidate buildings: own, complete, not
an oil platform, in the worker's region, **HP < 3/4 of max**, and no armed enemy within 3 tiles
(box ±3 around the building, both ground and air unit maps, 0x001520CC). When the player is poor
(gold < 200 **and** lumber < 100) only town-hall tiers are repaired. Among candidates the one with
the highest "annoy computer" UDTA value (`A5+4FAC`) is chosen. CONFIRMED (selection keeps the max;
return value lost by Ghidra, LIKELY the chosen building).

### 5.7 Expansion (new town hall site, 0x00152FA8)

Used when the list asks for a town hall or when no depot is reachable. Candidate gold mines: loads
left > 49 and **no town hall of any player** inside a 13×13 box around the mine (0x00152F16). For
each, a site search spirals outward from the mine (0x00152C72); the site must be in the worker's
region and closer than 11 tiles to the mine. The mine whose site is closest wins. CONFIRMED structure;
whether "closest" is measured to the mine (as the code reads) or to the worker is LIKELY the former.

### 5.8 Tankers (0x00152952)

Empty tanker: go to the oil platform minimising `dist(depot, platform) + dist(platform, tanker)/2`
(own, complete, with oil left); if there is none, **build an oil platform** on the oil patch nearest
to its oil depot (0x00151F14). Loaded tanker: return to the nearest oil depot. CONFIRMED.

## 6. Attacks

### 6.1 Launch (every 50 cycles, 0x00140A5E)

For each computer player with units, for each domain whose trigger (+0x09 land, +0x0A sea, +0x0B
air) is set: repeat *groups* times (+0x0E/+0x10/+0x12): choose a target player and a target point,
then send up to *group size* (+0x0D/+0x0F/+0x11) eligible units there. The trigger is cleared
afterwards **even if nothing could be sent**. CONFIRMED.

- **Target player** (0x001404D8): among players 0..7 whose controller is **human** (0), the one with
  the **most non-building units** (`A5+646E`); ties → lowest index; nobody with > 0 units → no wave.
  CONFIRMED. The computer never attacks another computer player on its own. **Wargus≠**.
- **Eligible units** (walk the AI's unit list, which is newest-first): passes the domain filter,
  is not a guard (+0x5F bit 2) and is not already an attacker (role 2). Each sent unit gets role 2
  with the target point (§7). Units that cannot reach the point by land are still counted and are
  marked "needs transport" (§7.4). CONFIRMED.
  - land (0x00140850): mobile, can attack, not naval, not air, not worker/tanker (and not flag bits
    0x2000000/0x4000000);
  - sea (0x001408D8): naval and can attack (no tankers/transports);
  - air (0x0014093A): air and can attack (no flying machines/zeppelins, which cannot attack).

### 6.2 Land target point (0x0014059C)

1. Anchor = the target player's town hall nearest to its newest unit (0x00152E0A), else its newest
   building, else nothing.
2. Over the **zone map** (§6.4): among land zones (terrain class 0) whose owner is the target
   player, take the one **farthest** from the anchor. Default = the anchor's own zone.
3. Target point = tile of the target player's unit nearest to that zone's centre (excluding ships,
   armed flyers and oil platforms, 0x0014146C), else the zone centre (zone·8 + 4).
CONFIRMED (the "farthest" comparison verified in the disassembly). The practical effect: the wave
goes for the human's outlying holdings / field army rather than the main hall. LIKELY reading.

### 6.3 Sea and air target points

- **Sea** (0x001406A4): the target player's newest oil platform; else a shipyard (the last one
  found in its list); else its naval unit farthest from its hall; none → no wave. CONFIRMED.
- **Air** (0x00140774): a gold mine in the target player's own list (rare) nearest its hall; else
  its town hall (the last found); else its unit farthest from its hall. CONFIRMED.

### 6.4 Zone (threat) map (0x00140300, 0x0014037C, 0x00140400)

- The map is cut into **8×8-tile zones** (`A5+5EE2/5EE3` = map size / 8), records of 0x42 bytes at
  `*(A5+5F58)`: owner byte, terrain byte, then per player two s16 sums (vs-ground, vs-sea).
- Terrain class, computed once at game start: per zone count water tiles (map flag 0x40) and land
  tiles (flag 1): more land → 0, more water → 1, equal → 2. CONFIRMED.
- Every 50 cycles each zone's per-player sums are rebuilt from the units standing in it
  (0x00140258 / 0x0014000E). A unit contributes:

  | unit | contribution |
  |---|---|
  | owner 15 (neutral) or dead/hidden | 0 |
  | worker | −2 |
  | town-hall tier (stores gold) | −1 |
  | transport | −5 |
  | tanker | −2 |
  | any building / immobile | −1 |
  | armed unit that can hit land (vs-ground sum) or sea (vs-sea sum) | HP × strength[type] / 60 |

  strength[type] (0x00140400, computed at load for types < 0x3A) =
  (piercing + (basic − 1)/2) × rate[type] / rate[grunt], where rate[type] = 10 × footman attack-cycle
  length / own attack-cycle length (0x0015A2C6, from the animation data). 60 is the grunt's HP.
  CONFIRMED.
- Zone owner (0x00140152): the player with the **largest non-zero** sum in the zone's domain
  (ground for land zones, sea for water zones), considering only sums > −100; mixed zones (class 2)
  have no owner (0xFF). CONFIRMED.

### 6.5 Last stand (0x0016870E → 0x00140BB0)

When a computer player loses a building and has **fewer than 3 buildings left** (not counting oil
platforms), every mobile unit (including workers and tankers) is sent at the target player: armed
units become attackers (role 2) toward the land target point, workers and tankers get a plain move
order there. CONFIRMED.

## 7. Unit roles (unit+0x5E) and the 50-cycle unit pass

### 7.1 Assignment and the base box

- Role assignment (0x00141224): role + post/target tile in +0x58, cleared "needs transport" logic:
  if the unit is a non-naval ground unit and the tile is in another land region, it is flagged
  **needs transport** (+0x5F bit 1) with the destination region in +0x5C. CONFIRMED.
- On creation (0x00140E2A) a computer unit gets: worker/tanker → no role (0); transport → role 3;
  naval → role 6; air → role 4; anything else (land fighters) → role 1 at the **rally point**.
  CONFIRMED.
- Rally point (0x00140CD4): walk a straight line from the player's nearest town hall toward the
  nearest enemy unit until it leaves the **base box**; if it ends off-map use a random tile ±6 around
  the unit (0x0014135E), if blocked the nearest free tile; if it is not in the unit's land region the
  unit stays where it is. CONFIRMED.
- Base box (0x00140F18, every 50 cycles): bounding box of the player's buildings excluding oil
  platforms and shore buildings, then min −5 / max +8 tiles, clamped to the map. CONFIRMED. Quirk:
  the min/max update is an if/else-if, so the very first building only sets the minimum (with a
  single building the max edges come out as 7). LIKELY (as decompiled).

### 7.2 The 50-cycle pass (0x00141084)

For every live computer unit with a role ≠ 0: casters (flag 0x20000) first try a spell (§9); if
they cast, the role handler is skipped. Otherwise the role handler (table `A5+0CE4`) runs.
CONFIRMED.

| role | handler | behaviour |
|---|---|---|
| 0, 7 | none | 7 = escort ship following a transport |
| 1 guard | 0x00141B20 | see 7.3 |
| 2 attacker | 0x00141D72 | see 7.3 |
| 3 transport | 0x00142010 | see 7.4 |
| 4 explorer | 0x0014155E | when idle: attack-move to a random tile within ±(map/4) of itself on both axes (`Random % (map/2)` per axis) |
| 5 | 0x00141AAE | when idle: patrol toward one side of the base box (sides rotate through +0x30, map-edge sides skipped); no side → becomes explorer. Never assigned by the code found — UNKNOWN use |
| 6 naval escort | 0x00141848 | when idle: patrol between the oil depot used by its tanker (or itself) and the nearest own oil platform (else nearest oil patch, else a random tile ±2) |

### 7.3 Guards and attackers

- **Guard (role 1)**: skip workers/tankers. Look at the zone the unit stands in. If another player
  owns it and that owner's strength (same domain) × 4 exceeds the unit's own player strength × 10
  (i.e. the enemy is > 2.5× stronger there), or if the unit is idle: when it is farther than
  **react_range_computer / 2 + 4** tiles from its post (0x00141504), it is re-assigned and **moves
  back to the post**; its attacker memory is cleared. CONFIRMED.
- **Attacker (role 2)**: if farther than the same distance from the target point and its current
  order is not flagged as engaged (`A5+1C9E[order]` bit 0x100) → attack-move to the target point
  again. Else, when idle at the target: attack the **nearest enemy unit anywhere on the map** that it
  can target (0x001413E8, no range limit); if there is none it becomes an **explorer** (role 4).
  CONFIRMED.

### 7.4 Transports (role 3, 0x00142010)

LIKELY in detail (several arguments lost by Ghidra), CONFIRMED in structure:
- Runs when the transport is idle or in its load order. It counts its passengers (6 slots).
- Empty: if units assigned to it are walking to it, wait/fetch the nearest; else collect up to **6**
  units flagged "needs transport" whose current transport is invalid and which all share one
  destination region; order them to board. Nothing to carry → go to the nearest own town hall.
- Loaded: choose the landing tile (the passengers' destination; with +0x0C set, search outward in a
  random direction for a tile of the destination region that is coast, 0x00141D06/0x00141C7A);
  assign escorts: the nearest armed ships/flyers not yet escorting become role 7 and follow; then
  sail to the landing tile and unload. Quirk: the escort count is read from **+0x0F of the context
  of player 0, 1, 2 ...** (indexed by the loop counter, not by the owner) — an original bug, LIKELY.

## 8. Defence and reactions

- **Call for help** (0x00120A12, SPEC_MECHANICS §2.5): when a computer unit is hit by an enemy,
  every idle armed (non-worker, non-tower) unit of the same player within **13 tiles** of the victim
  that can target the attacker attacks it (human players: 2, or 4 for ships). Guards (+0x5F bit 2)
  join only if within **4 tiles** of the victim, and then become attackers (role 2). CONFIRMED.
- **Submarine attacks**: if the attacker is a submarine, the nearest own air unit or submarine-flagged
  unit with no attacker memory is sent (attack-move) to the victim's position (0x001412DA). CONFIRMED.
- **Workers under attack** flee only when idle (SPEC_MECHANICS §2.5). CONFIRMED.
- **Towers**: scout towers auto-upgrade to guard towers (§4.2); towers in the list are rebuilt when
  destroyed (§5.3). CONFIRMED.
- There is **no explicit "base under attack → recall army"** logic; defence is the 13-tile help
  call, the guards' return-to-post and newly trained units rallying at the base edge. CONFIRMED (no
  other caller of the role assigner).

## 9. Spellcasters (0x0013D722, every 50 cycles)

A caster casts at most one spell per pass. Each attempt requires the spell to be researched
(`A5+6ACA[p]` bit) and mana ≥ the spell's cost (`A5-739C` table, SPEC_MECHANICS §2.6). Target search
methods: **box** = scan the 31×31 tiles centred on the caster row by row, first hit (0x0013C404);
**random** = up to 10 random unit slots `Random % 600` (0x0013C49A); **nearest** = nearest enemy unit
on the map (0x001413E8). "Clear of friends" = no unit friendly to the caster within r tiles of the
target (0x0013C4FA). Casters that are currently invisible (+0x44 ≠ 0) try only the first group.
CONFIRMED (spell names via the cost table, LIKELY).

| caster | while invisible | normal priority order |
|---|---|---|
| paladin (0x0C) | exorcism (random) | healing (box: friendly organic damaged) → exorcism (box: enemy undead) → holy vision (random enemy) |
| ogre-mage (0x0D) | runes (random enemy worker) | bloodlust (box: friendly organic attacking, not bloodlusted; caster sets the target's flag) → runes (random enemy worker) → one more check of the bloodlust research bit (UNKNOWN) |
| mage (0x0A) | blizzard (nearest enemy worker or hall, clear of friends 3) → polymorph (random enemy caster) | polymorph (box: enemy armed organic) → fireball (box: enemy, clear of friends 3, or anyway when caster HP < ½) → invisibility (box: friendly ranged/air unit, only if the caster is an attacker) → blizzard (nearest enemy building clear of friends 3) → flame shield (box) → slow (box: enemy non-building, not slowed, not idle) → fallback |
| death knight (0x0B) | death and decay (nearest enemy worker/hall) → whirlwind (random enemy worker/hall) | raise dead (box: dead body) → unholy armor (box: friendly attacking at ≤ ½ HP) → death and decay (nearest enemy building) → death coil (box: enemy organic) → death coil (box: enemy organic next to its ally) → whirlwind (nearest enemy, clear of friends 4) → haste (box: friendly attacking, not hasted/slowed) → fallback |

Fallback for mages/DKs with **mana < 50**: if something attacked them, attack it back; otherwise
they get role 1 with the post at their own mage tower/temple and walk there (0x0013D424,
0x0013D45C). Unit flags +0x4C bits mark targets already affected so that casters do not double up.
CONFIRMED.

## 10. Randomness used by the AI (sync-relevant, all through the game RNG `Random` 0x00188016)

| where | use |
|---|---|
| 0x00128C56 / 0x00128DDA | 1-in-4 / 1-in-8 unit choices in barracks and shipyard (§4.2) |
| 0x0015272A | idle worker delay 44 + (r & 15) |
| 0x0013C49A | random unit slot for spells (`r % 600`, ≤ 10 tries) |
| 0x0014135E | rally point fallback ±6 (`r % 12 − 6` per axis) |
| 0x0014155E | explorer wander (`r % (map/2)` per axis) |
| 0x0014168A | naval escort fallback (`r % 5 − 2` per axis) |
| 0x00141D06 | transport landing search start direction (`r & 7`) |

There is **no other randomness** in target-player, target-zone or production order choice.
CONFIRMED.

## 11. Difficulty, handicap, cheats

- No difficulty or handicap variable is read anywhere in the AI code. CONFIRMED (no such reader).
- No resource bonus for computer players: the only resource grants are the start values (1000 each,
  0x0013800E, before PUD values) and the cheat codes, which add to **all 16 players** alike
  (0x001321xx). CONFIRMED. **Wargus≠** (Wargus difficulty scales AI speed and gives resources).
- The only per-AI tuning is the script choice (AIPL) and the script's own numbers.

## 12. Binary vs Wargus (summary)

| topic | original | Wargus |
|---|---|---|
| script | 4-opcode byte code, fixed variables, from War Data 277 | Lua functions, forces, `AiNeed`, `AiResearch` |
| think rate | script every cycle; strategy and units every 50 cycles; buildings per period × 5 cycles | per-script sleep, Stratagus AI loop |
| attack target | human player with most mobile units; their zone farthest from their hall | nearest/available enemy, force-based |
| production | building-driven, fixed priority chains with 1/4, 1/8 random picks | `AiNeed` counts, any building |
| economy | lumber share formula (§5.5), 3-food headroom, repair rule (§5.6) | Stratagus resource ratios |
| rebuild | list positions un-ticked on loss → rebuilt in order | Lua script dependent |
| last stand | < 3 buildings → everything attacks | none |
| difficulty | none | 4 levels, speed/resource scaling |

## 13. Open questions

1. +0x5E role 5 (base-side patrol) has a handler but no code path assigning it — maybe set from
   saved games (0x0015A83C / 0x0015A882 read the context) or unused.
2. Exact semantics of the expansion "closest site" measure (§5.7) and of the tree search used for
   AI lumber jobs (0x0016D9AE helper, 0x0016C19AE region) — only the order is confirmed.
3. Ogre-mage third attempt (0x0013CF66) — checks a research bit and stops the unit; purpose unknown.
4. Meaning of the `+0x4C` per-target "already affected" bits (1, 2, 4, 8, 0x10, 0x20, 0x40) — which
   spell sets which bit (in the spell code, SPEC_MECHANICS §2.6).
5. Transport loop details (who waits for whom, unload trigger) — Ghidra lost most arguments in
   0x00142010; needs a disassembly pass.
6. The order flag `A5+1C9E` bit 0x100 used by attackers (§7.3) — LIKELY "attacking/engaged".
7. Whether the base-box if/else-if quirk (§7.1) is real or a decompiler artefact — check the
   disassembly of 0x00140F18.
8. Saved-game restore of the AI context (0x0015A83C) — not examined.
9. Script variable +0x1F/+0x21/+0x08 are written by scripts but never read: confirm with a search of
   all `A5+5F5E`-relative accesses in the disassembly (Ghidra may hide some).
