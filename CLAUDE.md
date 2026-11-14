# CLAUDE.md — Ami_Warcraft2 (AmiW2)

The user communicates in Polish; reply in Polish. Everything the program prints is English.
Read `LEFTOFF.md` first.

## What this is

A native Amiga port of Warcraft II as **our own engine in plain C** (gnu99), in the style of the
user's earlier ports: AmiSC (`I:\GITHUB\Amiga_OpenBW`, StarCraft — methods: logic frameskip, lores
2:1, 3x5 font, hardware pointer, Smacker) and AmiEU2 (`I:\GITHUB\Ami_EuropaUniversalis2` — newest
platform layer `native/`, GadTools configurator, one binary for AmigaOS/AROS/MorphOS).

- Behaviour spec: the original **Mac 68k binary** (Warcraft II Mac 1.23), decompiled in Ghidra
  (outside the repo, `D:\GITHUB\_AmiW2-poza-repo\re68k`) — **decompiled code never enters the repo**.
  Facts in `docs/RE_NOTES.md`.
- Secondary reference: Wargus/Stratagus (GPLv2) in `D:\GITHUB\_AmiW2-poza-repo\` — read as
  documentation of game facts and file formats, never copy code.
- Player data: the player's own Mac CD files (`War Data`, `War Resources`, ...) or PC `.WAR` files
  (same archive format). **No original file and nothing derived from one is ever committed or
  shipped.** `.gitignore` covers `WAR2.zip cd/ hfs/ re/ data/`.

## Targets (user decisions 2026-11-12)

- Fast on 68040/40 and 68030/50 with RTG (like AmiSC); AGA, ECS-EHB, RTG (8-bit and deep), WB window.
- Native **hires 640x480** and converted **lores 320x240** (assets halved, CC0 3x5 font).
- **AROS 68k and MorphOS from day one**, one 68k binary, run-time detection.
- Configurator `w2setup` (Intuition/GadTools): backend, resolution, Paula/AHI/off, music, volumes,
  logic frameskip, speed.
- Logic follows the original except the fog of war (every 4 cycles) and max 3 new path searches per
  cycle; logic frameskip was measured useless and removed in 0.4.0 (user decision 2026-11-14).

## Rules

Read `I:\GITHUB\!instrukcje_kompilacji_68K_AROS_i_MorphOS\README.md` + `10-68k-2D.md` before writing
Amiga code: bebbo gcc 6.5.0b in WSL, `-mcpu=68020 -msoft-float`, never `-m68040`/`-lc`/`-lpthread`,
never strip, never `sprintf`, no floats, `-fbbb` without `i`/`e`, `Stack 2000000`, `PROGDIR:` only,
Amiga headers only in `native/*.c`, own log with fflush, check libm881 in the link trace.
Emulator safety: `90-testowanie-emulatory.md` (never kill by name, never `-Force`, never host input,
never touch other projects' instances; runtime of this project only in `C:\temp\amiga_w2`).
WSL commands go into script FILES (no heredoc code generation through Git Bash).

## Layout

```
native/   platform layer (from EU2) + host_*.c (host test backend)
src/      the game (one module per file; shared contract in src/w2.h)
build/    build.sh (Amiga), build-host.sh (host, ASan), w.sh
tools/    our own tools (w2data.py reader, extractors, re/ scripts)
docs/     DATA_FORMAT, ENTRY_MAP, SPEC_RULES, RE_NOTES, AMISC_METHODS, BUILD, HOST_TEST
winuae/   configs + harness (runtime in C:\temp\amiga_w2)
```

## Team rules (parallel agents)

- Never two writing agents on one file. Module-private per-unit data lives in the module's own
  arrays indexed by unit slot, not in new fields of the shared structs.
- `src/w2.h` (shared structs) is changed only by the orchestrator.
- A step is done when it ran (host harness or emulator) and showed green, not when it compiles.
