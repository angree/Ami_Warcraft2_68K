# AmiSC methods - what to reuse in AmiW2

Source: the user's StarCraft port AmiSC (`I:\GITHUB\Amiga_OpenBW`, read only). File references
below are into that tree. Numbers are AmiSC's own measurements, with the config they were taken on.
Section 7 is the concrete recommendation for our engine.

Reference machines used in AmiSC measurements:
- **020/-80**: `winuae/bw-020-test-slow.uae` / `bw-020-play-slow.uae` - 68020, no FPU, no JIT,
  `cpu_throttle=-800` (the developer's "slow A1200" calibration).
- **040/40**: `winuae/bw-040-40.uae` - 68040, FPU off, no JIT, `cpu_throttle=-700` (SysInfo 1.77
  vs A4000/040/25).
- Numbers taken under JIT measure the PC, not the Amiga; AmiSC marks them as such.

---

## 1. LORES 320x240 - assets halved once, drawn natively

Files: `PLAN-LORES.md` (plan + measured stages), `core/amisc_lores.h` (the filter),
`core/amisc_lores_pack.h` (sprite/terrain cache files), `amiga/glue_lores.h` (menu pictures cache),
`amiga/glue_vpack.h` (menu videos pre-decoded), `core/amisc_view.h` (`render_shift`).

### 1.1 History - why native and not "draw 640, drop pixels"
- First lores = draw 640x480, keep every 2nd pixel of every 2nd row
  (`amisc_backend.cpp` `present_surface`). Nearest neighbour, thin lines vanish, and the CPU
  still draws 307 200 px to show 76 800.
- Drawing at 320x240 with full-size art shows a quarter of the battlefield and the 640x480
  console does not fit - rejected.
- Final: **all art halved ONCE with a real filter, cached on disk, drawn 1:1 into a 320x240
  canvas**. Same map area visible as in hires. Tile 32 -> 16, sprite /2, console 640x480 -> 320x240.

Measured AGA variants (020/-80, Blood Bath, same sim frame, PROGRESS.md "Warianty AGA"):

| variant | fps |
|---|---|
| AGA 640x480, full redraw | 6 |
| AGA 640x480 + dirty c2p | 8 |
| AGA draw 640, screen 320 (drop pixels) | 10 |
| same + dirty | 10 (no gain - see 3.2) |
| RTG 640x480 (no c2p) | 17-18 |

### 1.2 The filter (`core/amisc_lores.h`, `downscale2`)
- **Lanczos-3, 2:1, integer only.** 2:1 has a constant phase, so ONE 12-tap weight vector (16.16,
  sum 65536) serves every pixel: `242, 987, -2228, -4367, 8880, 29254, 29254, 8880, -4367, -2228,
  987, 242`, first tap at offset -5 from `2*x`. Lanczos-2 (8 taps) and box (2 taps) are kept as
  options. Weights computed once on PC and written as constants (no `sin()` in the binary).
- Multiply table `tab[tap][0..255] = w*v` - no MULS in the inner loop.
- Separable: horizontal pass into a ring of `taps` rows (no full intermediate image), then vertical.
- Horizontal sums shifted `>>10`, not `>>8` - Lanczos overshoot otherwise overflows 32 bits
  (was visible as dotted garbage above buildings).
- **Anti-ringing clamp**: result clamped to [min, max] of the input colours of its window.
- **Transparency is a weight, not a colour**: clear pixels add nothing; opaque weight decides
  opacity. Sprite pixel is opaque when >= `coverage_pct` (50) of its 2x2 block is opaque.
- **Edge = plain 2x2 average**, Lanczos only where the pixel's own 2x2 block is fully opaque and
  >= half the window weight is opaque (v0.7.3: 17 613 px filtered vs 1 590 averaged on 6 sprites).
  Before that, dividing by near-zero weight at edges gave random colours.
- **Index classes - "a palette byte is not always a colour"**: player-colour indices (SC 8-15)
  are averaged only among themselves, over RAMP POSITION (0..7), and the class with the larger
  weight wins. Measured: 17.3% of sprite pixels are team pixels, all stay in 8-15 after halving.
- **Non-colour images are halved by VOTE** (`downscale2_vote`): most frequent value of the 2x2
  block, ties -> first in reading order (deterministic), clear when fewer than `min_cover` set.
  Used for shadows, glow/fire levels, distortion, masks, selection rings (`min_cover` 1 for 1-px
  outlines, 2 for areas). Command icons/wireframes are ramps: averaged by index position.
- **Quantiser**: RGB -> index through a 32 KB LUT on 15-bit RGB, distance `2dr^2+4dg^2+3db^2`,
  built once per palette; entries 17/18/19 (pointer) are excluded via `allowed[]`.
  `glue_lores.h` has a pruned exact builder because the naive 8.4 M distance build is tens of
  seconds on a 68020.

### 1.3 When it runs and where results live
- Conversion is **once**, on first entry into lores, with a question screen (CONVERT /
  QUICK MODE) and a progress bar; headless runs never ask. QUICK MODE = the old pixel-drop path.
- Cache files under `PROGDIR:data/`: sprite pack `'AMLR'` (format 4), terrain pack per tileset
  `'AMLT'`, menu pictures `'AMMP'`, menu videos `menu-video-XXXXXXXX.pack`.
- **Stamp** = format, filter, file count, frame count, pixel count, palette sum (or rotate-xor hash
  of the source pixels per record in the menu pack). Mismatch -> rebuild (only the changed
  records in the menu pack).
- **Same structure as the originals, half size**: halved sprites are re-encoded into the original
  GRP RLE format and REPLACE the full sheets in memory, so the blitter is untouched ("a second
  drawing path is a second description of the same thing"). A launch with a good pack never
  decodes the full sheets (20 MB not read).
- Sprites quantised only into the **188 palette entries identical in all 8 tilesets** -> one
  sprite pack for every map; terrain per tileset.
- Measured: whole SC sprite set 834 GRP / 21 193 frames: PC 1.7 s; emulator (JIT, = PC speed)
  7.7 s; 16.4 MB -> 5.0 MB RAM, file 6.9 MB; 2nd start 0.56 s. Real 68020: "minutes to tens of
  minutes" (estimate, never measured) - hence the cache is mandatory.
- Data-file numbers in sprite pixels (attach points, bar widths, selection circles) are halved at
  the point of use, not in the pack.

### 1.4 Coordinates and hit-testing
- **In game**: world stays in hires map pixels; `render_shift` (=1 in lores) applied in
  `to_screen_x/y`, `tile_px()`, `blit_megatile<clip, MINI>`; `view_cols = vp_cols << shift`, so a
  click goes back through the existing `view_zoom`. Simulation identical in both modes.
- **Console/UI**: art halved; 17 `art_*` constants divided in ONE place
  (`layout_console_art(..., shift)`), all hit tests read the halved console image (its shape is not
  a rectangle). `art_wide()/art_tall()` replaced 10 hard-coded 640/480.
- **Original menu (glue)**: the layout is NOT halved. All rectangles stay in 640x480 units of the
  dialog files; only the surface (`surface::sh`) and the pointer (`h_poll`) convert, each in one
  place.
- Project rule paid for six times: **a clickable region is described once**; drawing and hit
  testing read the same description. A gate counter ("command card outside console: 0") checks it.
- Minimap: 128 -> 64 px window, still drawn procedurally (`amiga_mini_zoom`), not converted.

### 1.5 The 3x5 font (`amiga/glue_font_3x5.h`)
- **Licence: CC0 1.0**, drawn by hand for AmiSC, no existing bitmap copied (header says so).
- Used for text of the original menu at 320x240 (`glue_menu.h:1012-1062`): 640x480 fonts halved
  2:1 are a smudge. Drawn with a 1-px shadow at (+1,+1); cell 3x5, advance 4, line 6; all caps;
  Polish/German letters folded. (The other glue fonts in `glue_font_data.h` are generated from
  Orbitron, OFL - not ours to reuse.)
- In game AmiSC uses its own 5x7 font 1:1 at 320x240 (`bw_amiga_font.h`), drawn 2x in hires.
- Ported to C: `src/font35.h/.c` (CC0, header states it).

---

## 2. LOGIC FRAMESKIP - simulation at fixed rate, drawing whatever is left

Files: `amiga/bwshow.cpp:5284-5420` (loop), `core/sim/amisc_sim_flags.inc:150-207`
(`fast_logic_period`, `unit_thinks_now`), `core/sim/amisc_sim_path.inc:5011-5069`,
`core/sim/amisc_sim_frame.inc:180-370`, `amiga/bw_amiga_ui.h:660-720` (`fast_logic_levels`).

### 2.1 Two loop rates (the outer frameskip)
```
if (now - prev_tick > 2000) { log "clock jumped"; prev_tick = now; }   // drop, never sprint
caught_up = 0;
while (now - prev_tick >= step_ms && caught_up < 4) {                 // cap 4 ticks per draw
    if (ai_running) ai.step();      // AI once per LOGIC tick, never per render
    sim.step_frame();
    prev_tick += step_ms; ++caught_up;
}
render();                            // exactly one draw per pass
music_service(); sound_pump_sliced(); // per RENDERED frame (sim may be paused)
```
- Step = the original's game speed (SC: 167/111/83/67/56/48/42 ms). **MAXIMUM** = run
  `game_speed_max_steps` ticks per draw ignoring the clock (benchmark mode).
- Menus hold the sim (`amiga_sim_held()`, one function decides) but keep rendering; time spent in a
  menu is not owed (`prev_tick = now` on leaving).
- When the machine cannot keep up the game runs SLOWER (cap 4), never spirals. Profile prints
  `steps per draw X.XX (cap 4)` - at 4 the sim alone is the bottleneck. Default speed was lowered
  to SLOW in 0.7.1 because FASTEST ate ~80% of the machine for logic.
- Counters that should fire "every N sim frames" must use DISTANCE (`frame - last >= N`), not
  `frame % N` - with 4 ticks per draw the loop only observes every 4th frame number.

### 2.2 FAST LOGIC (the inner frameskip - "logic frameskip")
One menu dial, chosen BEFORE the match, never changed mid-match (a replay would have two rule
sets), logged at start. Levels `1/1` (bit exact, default), `1/2`, `1/4`, `1/8`:
```
period = {1,2,4,8}[level];
unit_thinks_now(u) = ((frame_now + u->index) % period) == 0;   // spread by unit index
```
- **Orders pass**: a unit that does not think this tick is skipped; the one that does calls
  `advance_unit(u, period)` - all per-unit timers (cooldowns, build time, upkeep) advance by
  `period`, so every RATE the player feels is unchanged; only reaction time coarsens
  (`period-1` ticks).
- **Movement pass**: states that ADVANCE A UNIT ALONG A ROUTE IT ALREADY HAS run every tick at
  every level (flyer, fake flyer, move-to-legal, follow path, scout path). Everything else
  (turning, collision resolution, target picking, asking for a path) waits for its thinking tick.
  Measured split on the developer's save: route following 3 420 us/tick (56 units), all deciding
  19 671 us/tick - 85% of the pass is deciding.
- Windowed per-tick state (cloak counter) is zeroed at the start of an N-tick window and read at
  its end (`cloak_window_opens/closes`), because each unit touches it only on its own tick.
- Deterministic: the skip depends only on `frame_now` and unit index, which are sim state.
  Determinism rules from `PLAN-OPTYMALIZACJI.md`: never budget/queue path requests per frame
  (path timing rides the RNG -> desync), never reorder tie-breaks; only skip work proven to give
  the same result, or make the change an explicit, recorded mode.
- Numbers (020/-80, 4 AIs, 374 units, sim tick): base 51.9 ms; old "fog every 2nd tick" -0.1%
  wall; old "resting units every 2nd tick" (FAST LOGIC 2) **-13.5%** (44.9 ms); capping A*
  nodes made it WORSE (+32% in `short_leg.expand`, searches fail and repeat) - removed. Movement
  was 52% and orders 34% of a tick. 040/40 base curve: 66 units 2.2 ms, 116 units 20.4 ms,
  211 units 86.8 ms, 400 units 579 ms per tick - superlinear (combat interaction).
- Safe (bit-exact) savings that went in: fog reveal memo keyed by (tile, sight, ground level) -
  `reveal_tile` only clears bits, so a repeat call is a no-op; short-path planner without heap
  allocation (43 malloc/free per call on libnix with a linear free list).

---

## 3. DRAWING PERFORMANCE

### 3.1 c2p
- Kalms c2p (`amiga/c2p1x1_6_c5_bm_040.s`, `c2p_rect.s`; THIRD-PARTY - "By Mikael Kalms",
  attribution required). Rect variant converts sub-rectangles; width granularity 32 px.
- Cost: 640x480 c2p ~110 ms of a ~145 ms frame (020/-80); 320x240 ~28 ms. 040/25 ~2800 px/ms,
  040/40 ~4500 px/ms -> 21 ms buys ~95 000 px, so **320x240 on AGA, 640x480 only on RTG**.
- RTG path: no c2p at all (WritePixelArray-style blit). EHB dropped: would need all art requantised
  to 64 colours on the user's machine, and at 320x240 c2p is no longer the bottleneck.

### 3.2 Dirty rectangles (`amiga/amisc_backend.cpp:108-235, 843-1091`)
- Grid of **32x16 cells** (32 = c2p granularity), one byte per cell, no rectangle merging.
- v1 compared the whole frame with a copy of the previous one: measured 640x480, 77 ms drawing
  of which comparing 17.7 ms and converting the 5% that changed 3.6 ms; libnix `memcmp` byte by
  byte - replaced by 8x u32 XOR per 32-px cell row, and only converted regions copied back.
- v2: **drawers MARK cells they touch** (`mark_dirty_rect`), unmarked cells are neither compared
  nor converted; the comparison stays the arbiter INSIDE marked cells. `dirty_paint 1` also skips
  PAINTING outside marked cells. Measured 320x240 AGA 020: sprites 15.9 -> 11.2 ms, drawing
  28.8 -> 23.5 ms, 28 -> 32 fps, picture bit-identical (checksums at 9 points).
- Runs of changed cells closer than 3 are merged into one c2p call.
- Full refresh when the display was touched by anyone else: `amigagfx_display_serial()` moves on
  every open/close/blit; mismatch -> convert everything once (bug: console stayed black after a
  menu otherwise).
- **Gate**: `dirty 2` compares everything and counts cells that changed but were NOT marked -
  must read 0 ("do not add a guard, add the measurement").
- Measured lesson: compare-based dirty gave +33% at 640 but **0% at 320** (c2p is cheap there and
  comparing eats the saving) - tie it to resolution unless the marks remove the compare.

### 3.3 Caches that removed whole phases
- **Terrain**: painted once into a clean buffer, copied whole every frame (sprites cover 41% of
  the view, so per-sprite restore would save ~1.5 ms and add a class of trail bugs). Invalidation
  by comparing a per-tile signature (megatile number << 1 | creep), scanned one tile wider than
  visible, plus view position and tileset. Terrain 91.7 ms -> microseconds.
- **Fog** is laid on the finished frame, never written into the terrain cache; fully visible tile
  costs one bit test, unexplored = memset per row, only the "explored, not seen" ring goes through
  the shading table.
- **Minimap**: terrain+fog+dots rebuilt every 12 game frames, copied between; view frame drawn
  fresh every frame: 16-17 ms -> 3.5 ms (040/40).
- **Scrolling**: AmiSC simply rebuilds the terrain cache when the view moves (no shift-copy). Scroll
  speed is derived from measured time (`host_millis`, 20 ms steps), never per frame; `std::chrono`
  on bebbo libstdc++ steps once per second. Right-drag scroll parks the pointer at screen centre.
- Menu videos decoded ONCE into row-delta streams (`glue_vpack.h`) when the loop closes
  bit-exactly - live Smacker + halving was as slow at 320 as at 640 on a 68040.

### 3.4 Sprite blit fast paths
- Halved art reuses the same RLE blitter (see 1.3). `blit_megatile<clip, MINI>` templates split
  clipped/unclipped and full/mini tile paths.
- **Tried and reverted**: `memset/memcpy` for RLE runs - terrain 24 -> 39 ms, 15 -> 12 fps; runs
  are short, a library call costs more than the byte loop. Measure run-length distribution first.
- Load-time remaps instead of per-pixel hooks (pointer colours 17-19, icon ramps): nothing is
  added to the per-pixel draw path, which runs over 17 M sprite pixels.
- Compiler: bebbo gcc 6.5 `-O1` only (O2 ICE, Os miscompiles), `-fbbb` without `i`.

---

## 4. HARDWARE POINTER AND PALETTE (`amiga/bw_cursor.h`, CLAUDE.md "Palette indices are not colours")

- Pointer = **hardware sprite 0** via Intuition `SetPointer` (AGA and RTG). A software pointer was
  rejected: it moves at frame rate.
- Its 3 colours ARE palette registers **17, 18, 19**. The GAME moves off them: every source pixel
  that names 17/18/19 is rewritten ONCE at load to the nearest other palette entry (`17->93,
  18->95, 19->21` on one tileset) - sprites, terrain, the console art (loaded later, own call) and
  the OUTPUTS of the shading tables (`dark.pcx` - inputs untouched). Check: `17:0 18:0 19:0` over
  the screen.
- Then 17-19 are ours: they change with the pointer shape (arrow; ring over a unit green own /
  yellow neutral / red enemy; sight while an order waits for a target). ONE copy of "what 17..19
  hold now" (`g_ptr_pens`), re-applied after every palette upload (the engine re-uploads the whole
  palette often).
- Shapes: from the game's own cursor sprites, reduced by the smallest integer factor >= 2 that
  fits 16x32 (a sprite pixel is 2 hires pixels on AGA hires), cells aligned to the hot spot, the
  BRIGHTEST pixel of a cell speaks for it (averaging turned the arrow into a blob), 3 colours by
  k-means with the dark EDGE reserved as its own register. Built-in ring/sight shapes when the data
  has none. Host probe prints every shape as text.
- In 320x240 the pointer stays as is (relatively bigger) - accepted as correct.
- Pens: never hard-code Intuition pen indices; pick nearest colours from the current palette
  (`amigagfx_apply_pens` after the screen is open). Icon ramps through a 256-entry map, never by
  writing palette entries (each tileset uses 1-15 differently). Copper per-line palettes considered,
  not used (no copper on RTG).

## 5. SOUND (`amiga/amiga_audio.h`, `amisc_backend.cpp:1097+`)
- audio.device, all 4 channels allocated at once (fail fast -> silent game). `BeginIO` only
  (`SendIO/DoIO` clear `ADIOF_PERVOL` -> volume 0), completion polled with `CheckIO`, never Wait.
- Effects: decoded 8-bit samples in FAST RAM; each channel has one CHIP staging buffer the sample
  is copied into just before play (24 MB of sound cannot live in chip). Max 131 070 bytes (Paula
  length register); 32 KB when the chip budget is capped (no AGA or <= 2 MB chip).
- Period = 3 546 895 / rate, min 124 (~28.6 kHz). Volume map engine 0..124 -> Paula 0..64
  rounding up.
- **Music** streams on channels 2+3 (same mono on L and R), two chip buffers queued per channel
  (gapless); SFX confined to 0+1 while music plays, all 4 otherwise. **Speech** on 3+2 without music,
  channel 1 alone with music. Refills pulled once per RENDERED frame, gap counter must stay 0.
- Missing samples are loaded asynchronously: queued, ONE sample fetched/decoded per rendered frame
  (a first play no longer stalls the sim). Sound is not game state, so this cannot desync.
- With no channels the game does not load samples at all.

## 6. SMACKER (`amiga/glue_smk.h`, `glue_vpack.h`)
- `glue_smk.h`: the user's own SMK2 decoder (written from the public format description): 4 big
  trees from 2 byte trees each, 3-entry recent-value cache, 4x4 blocks of 4 kinds; lookup tables
  (10-bit first level, 6-bit sub-tables, 8192-entry budget per tree, bit-walk beyond) instead of
  one step per bit. Video only (menu videos are silent).
- **Ported to C**: `src/smk.h/.c` - same algorithm, plus audio (Huffman DPCM and raw PCM, 8/16 bit,
  mono/stereo), frame time in us, Y-double/interlace flags. Two fixes vs the C++ original:
  big-tree depth limit 64 -> 500 (W2 movies have deeper trees), and the lo/hi byte reads of a
  big-tree leaf are now sequenced (C leaves `lo(b) | hi(b) << 8` order unspecified; gcc -O2
  reversed it and garbled 2 of 12 movies).
- Host test `tools/test_smk.c` on the Mac CD `War Movies` archive (15 entries, id 6000): **12 of 12
  SMK2 movies decode every frame with ASan/UBSan clean, 0 audio errors**; 3 entries are small
  non-SMK blobs. All are 320x144 (10) or 320x200 (2), 12 fps (83 330 us), one track 22 050 Hz
  8-bit mono Huffman DPCM; audio length matches video length (216 frames -> 396 884 bytes = 18.0 s);
  mean sample step across chunk borders equals the step inside chunks (predictor start correct).
  Frames inspected visually: correct pictures.
- vpack idea (decode once into a row-delta stream) is only worth it for LOOPING clips; W2 movies
  play once - decode live (64 000 px/frame at 12 fps), audio 1 837.5 bytes per frame -> stream
  through the music path (8-bit unsigned: xor 0x80 for Paula, period 161).

---

## 7. RECOMMENDATION FOR THE W2 ENGINE

### 7.1 Lores conversion of W2 assets
- **One converter module** (`src/lores.c`, C port of `core/amisc_lores.h`): Lanczos-3 constants +
  multiply table, ring of rows, `>>10` headroom, anti-ringing clamp, transparency as weight,
  2x2-average edge, 2x2 vote for non-colour images, 15-bit quantiser LUT that excludes 17/18/19
  (and any index we reserve). Same code builds on host (fast verification + optional PC
  pre-converter) and on 68k.
- **Terrain**: W2 megatile 32x32 = 4x4 minitiles of 8x8. Halve each 8x8 minitile -> 4x4 (filter
  within the minitile, edge pixels repeated, no transparency); megatile table unchanged, so lores
  megatiles are 16x16. One pack per era (summer/winter/wasteland/swamp), palette of that era.
- **Sprites** (units, buildings, missiles, corpses): halve every frame with transparency index 0
  and the **W2 player-colour ramp (208-211) as the team class** (average ramp position, never
  mixed with normal colours). Re-encode into the SAME RLE frame format the blitter reads, so lores
  is "another data set", not another drawer. Measure how many palette entries are identical across
  the 4 era palettes; if enough, quantise sprites only into those (one sprite pack for all maps,
  as AmiSC's 188), else one pack per era. Any colour-cycled index ranges are a class of their own
  (verify the ranges in RE_NOTES before coding).
- **Shadows, selection boxes, masks**: vote, never filter. Health bars, selection rectangles, build
  grid: procedural, constants halved at draw time.
- **UI**: hires panels (640x480 layout) halved once with Lanczos into a UI pack. Keep EVERY UI
  coordinate in 640 units; convert only in two places - the draw surface (`x >> shift`) and the
  pointer (`x << shift`). Hit tests read the same tables as drawing.
- **Icons**: halve (W2 icons 46x38 -> 23x19) as colour images with the team class.
- **Fonts**: lores text uses `src/font35` (CC0) with +1,+1 shadow; hires uses the game's fonts.
- **Minimap**: draw procedurally at 64x64 in lores (128 in hires), rebuild every ~12 cycles,
  view frame every frame.
- **Cursor**: hardware sprite (see 7.4) - not converted.
- **Cache**: `PROGDIR:data/lores-*.pack`, header = magic, format, filter, counts, palette sum /
  source hash; rebuild on mismatch. First lores start asks CONVERT / QUICK (2x2 average or vote -
  much cheaper) with a progress bar; also ship a host converter so a player can pre-build on PC.
  Never put a converted pack in the repo (derived from original data).
- **Coordinates**: world in hires map pixels everywhere (w2.h already: `view.scale_shift = 1`
  only at draw time). Click -> world = `(screen << shift) + view_origin`. Simulation identical.

### 7.2 Logic frameskip
- Main loop exactly as 2.1: fixed logic step = the original's cycle length for the chosen speed,
  accumulator with **cap 4 cycles per draw** and **drop after 2 s** gap; AI inside the logic tick;
  sound/music/sample loading per rendered frame; menus hold the sim; a MAXIMUM speed for benchmarks.
- FAST LOGIC dial `1/1, 1/2, 1/4, 1/8`, chosen in `w2setup`/before the match, stored in saves and
  replays, default 1/1. `w2_unit_thinks(slot) = ((cycle + slot) % period) == 0`.
  - Every cycle for every unit: moving along an existing path one step, projectile flight,
    animation/script frame timers, construction/training/research progress, mana regen,
    harvest timers (or advance them by `period` on the thinking cycle - pick ONE rule per counter
    and keep it in one function).
  - Only on thinking cycles: target acquisition, order state transitions, path requests, idle
    reactions, AI per-unit decisions; the call receives `period` so timers keep their rate.
  - Never: budgets or queues of path requests, anything whose ordering changes RNG consumption.
- Instrument from day one: per-section profile in us (E-clock), `cycles per draw` printed,
  sampled per-unit timers (1 tick in 16) so the instrument costs ~1%.

### 7.3 Dirty rectangles and caches
- Drawing marks a 32x16 cell grid (32 = c2p rect granularity); backend converts only marked cells,
  comparing inside them with 32-bit XOR; merge runs with gaps < 3 cells; full refresh when the
  display serial moved; `dirty 2` gate counts changed-but-unmarked cells and must print 0.
- Terrain cache buffer copied each frame; invalidate on view move or per-tile signature change
  (megatile number + dynamic state such as tree chopped/wall/building/rubble). For W2 at 16 px
  lores tiles a shift-copy on scroll is an optional later step - AmiSC does a full rebuild.
- Fog of war drawn on the finished frame (W2 fog is per tile: memset for black, shading table for
  explored-not-seen).
- Byte loops for RLE runs; test memcpy only after measuring run lengths.

### 7.4 Cursor and palette
- Hardware sprite 0 via SetPointer on all backends; registers 17-19 belong to the pointer: remap
  every source pixel and every shading-table OUTPUT that uses 17/18/19 to the nearest other entry
  once at load; one global holds the current pointer pens and is re-applied after each palette
  upload. Pointer shapes from W2's cursor sprites reduced >= 2:1 to 16 px, 3 colours with a
  reserved dark edge, hot spot cell-aligned; built-in fallbacks.
- Pens/UI colours by nearest-colour search in the current palette, never fixed indices.

### 7.5 Sound
- Paula: music (and movie audio) on 2+3, SFX on 0+1 while music plays; chip staging buffers per
  channel; samples decoded to Fast RAM; async loading one sample per rendered frame;
  `BeginIO`+`CheckIO` only. AHI backend as an alternative per the configurator.

### 7.6 Movies
- `src/smk.c` decodes W2 movies live: per frame `smk_next_frame(&v, SMK_DECODE_ALL)`, palette to
  the screen when `palette_changed`, pixels blitted (hires: 2x2 doubled or centred 320 on a 640
  screen; lores 1:1), track 0 audio appended to a stream played by the music path. Pace by the
  audio clock, drop video frames (decode with audio only would break deltas - decode video every
  frame, skip only the blit/c2p) when behind.
