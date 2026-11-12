# Team brief — round 2 (engine modules)

We write OUR OWN Warcraft II engine in plain C (gnu99) for Amiga 68k (also AROS 68k, MorphOS), in the
style of the user's StarCraft port AmiSC. Repo: `D:\GITHUB\Ami_Warcraft2` (WSL: `/mnt/d/GITHUB/Ami_Warcraft2`).
Reply/notes in English in code; the user is Polish.

## Read first (short)
- `CLAUDE.md` (rules), `src/w2.h` (THE CONTRACT: shared structs + every module's API with owner file),
  `src/w2_tables.h` (unit types, animations, missiles, upgrades, spells, buttons, sounds — all game facts),
  `src/assets.h` (the only way to get data: sprites, palette, tiles, strings, fonts), `src/pud.h`.
- As needed: `docs/DATA_FORMAT.md`, `docs/ENTRY_MAP.md`, `docs/SPEC_RULES.md`, `docs/AMISC_METHODS.md`,
  `docs/HOST_TEST.md`, `docs/BUILD.md`, `docs/RE_NOTES.md` (may still be growing).
- Behaviour references (READ ONLY, never copy code): Wargus/Stratagus (GPLv2) in
  `D:\GITHUB\_AmiW2-poza-repo\wargus` and `...\stratagus` — read as documentation of game behaviour;
  decompiled original (if present) in `D:\GITHUB\_AmiW2-poza-repo\re68k\` — spec only, never paste it.
  AmiSC `I:\GITHUB\Amiga_OpenBW` (user's own code — methods may be reused freely). Never edit anything under `I:\GITHUB`.

## Hard rules
- **Write only the files you own** (listed in your task). Everything else is read-only. If the contract
  in `w2.h` is missing something you need, keep it file-local (static) or add a declaration in YOUR
  own header `src/<module>.h`; list contract change requests in your final report.
- Module-private per-unit data: your own static arrays indexed by unit slot (W2_MAX_UNITS).
- C rules (68k): no floats/doubles anywhere, no `sprintf` (use `snprintf`), no `argv`, no C++,
  endian-neutral file parsing (byte reads), no Amiga headers in `src/` (platform API only via
  `native/amiga_gfx.h` / `native/amiga_audio.h` / `native/w2_sys.h`), integers/fixed point, small
  stack use (no big local arrays — use static), malloc sparingly. Must compile cleanly with
  `-Wall` on host gcc AND with `/opt/amiga/bin/m68k-amigaos-gcc -mcpu=68020 -msoft-float -noixemul -O1 -std=gnu99 -Isrc -Inative -c`.
- Determinism: simulation randomness only through `w2_rand()`; no time/clock reads in simulation.
- Performance target: 68030/50–68040/40. Avoid per-pixel divisions, per-frame mallocs, O(n²) over all units per cycle.
- Original game data is copyrighted: never copy it, or anything rendered from it, into the repo.
  Screenshots/scratch go to `D:\GITHUB\_AmiW2-poza-repo\scratch\<your-module>\`.
- WSL: run `wsl -d Ubuntu-22.04 -- bash /mnt/d/...script.sh` from PowerShell. Write helper scripts to
  FILES; never generate C or Python code through shell heredocs (Git Bash mangles backslashes).
- **Emulators: do NOT start, stop or touch any WinUAE/QEMU.** Host testing only. (The orchestrator does emulator runs.)

## Your sandbox (parallel agents must not break each other's builds)
1. At start: copy the repo to your own sandbox, excluding original data and build output:
   `rsync -a --exclude hfs --exclude cd --exclude re --exclude WAR2.zip --exclude build/host /mnt/d/GITHUB/Ami_Warcraft2/ /mnt/d/GITHUB/_AmiW2-poza-repo/wt/<module>/`
2. Work and build ONLY in the sandbox. `src/w2_stubs.c` gives weak placeholders for every function of
   other modules, so your module links and runs on its own (host gcc).
3. Host build/run: `build/build-host.sh` in the sandbox (docs/HOST_TEST.md). Game data for host runs:
   `D:\GITHUB\_AmiW2-poza-repo\gamedata\data\` (Mac archives "War Data" etc.) — symlink or point
   `W2_PROGDIR`'s `data` at it (assets.c reads `w2_cfg.data_dir`, default `PROGDIR:data`). Extra PUDs:
   `D:\GITHUB\_AmiW2-poza-repo\gamedata\*.pud`; built-in skirmish maps are War Data entries 220..240
   (`pud_load_builtin`). For module tests you may write `tools/test_<module>.c` + `tools/build_test_<module>.sh`
   (you own those) that set `w2_cfg.data_dir` to the gamedata path, call `assets_init()`, `pud_load_builtin(220)` etc.
4. Prove your module works on the host (ASan clean), with PNG screenshots you have LOOKED AT where visual.
5. Also compile your files with m68k-amigaos-gcc (command above) — must be warning-free.
6. Finally copy ONLY your owned files from the sandbox into `/mnt/d/GITHUB/Ami_Warcraft2/` (same paths).

## Key facts
- Tiles 32x32 hires. Units: `tx,ty` tile (top-left for buildings) + `ox,oy` pixel offset while moving.
  Sprite box is centred on the footprint: `box_x = tx*32 + ox + (tile_w*32 - box_w)/2` (same for y).
- Directions 0=N clockwise 0..7. Sprites store 5 (N,NE,E,SE,S); SW/W/NW = SE/E/NE mirrored:
  `w2_dir_frame(base, dir, &mirror)`.
- Player colours: `assets_remap(player)`; palette indices 17..19 belong to the hardware pointer (assets handles it).
- Logic: `game_tick()` = one game cycle; normal speed 30 cycles/s. Logic frameskip: `unit_thinks(slot)`
  (heavy decisions only when true; animation/movement every cycle; pass the elapsed period to rates).
- Lores (320x240): world logic identical; drawing helpers in assets.c halve positions themselves;
  draw code uses hires coordinates and `VX()/VY()` only when writing raw pixels itself.
- Final report: SHORT (max 30 lines): what works with evidence (log lines, PNG paths), files, contract
  change requests, open problems.
