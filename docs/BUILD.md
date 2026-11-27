# AmiW2 — build, deploy, run, test

One 68k binary (`w2`) plus the settings tool (`w2setup`) for AmigaOS 3.x (AGA / ECS-EHB / RTG), AROS 68k and
MorphOS — run-time detection only, no `#ifdef` per OS. Rules: `I:\GITHUB\!instrukcje_kompilacji_68K_AROS_i_MorphOS\`
(README, 10-68k-2D, 90-testowanie-emulatory). Version: `src/version.h` (the only place).

## Build (WSL Ubuntu-22.04, bebbo gcc 6.5.0b in /opt/amiga/bin)

From PowerShell (never inline commands - put them in a script file):

```
wsl -d Ubuntu-22.04 sh /mnt/d/GITHUB/Ami_Warcraft2/build/w.sh build/build.sh          # build + deploy
wsl -d Ubuntu-22.04 sh /mnt/d/GITHUB/Ami_Warcraft2/build/w.sh build/build.sh clean    # from scratch
```

`build/w.sh` remounts the drvfs drives (WSL1 drops them), puts the toolchain on PATH, cds to the repo.
**Always check the last line is `built and deployed`** — a failed build leaves the OLD binary in place.

- Objects in `$HOME/w2-build` (WSL home; sources mirrored by content). Flags:
  `-mcpu=68020 -msoft-float -noixemul -O1 -fomit-frame-pointer -fbbb=abcfhlmnprsz0 -std=gnu99 -Wall`.
- **Every `src/*.c` is compiled into w2** except `setup_amiga.c`, `*_host.c`, `test_*.c` (and `w2cfg.c`/
  `main_amiga.c`, listed explicitly). Add game files without touching the script.
- A `src/` file that defines `int game_init(void)` switches `main_amiga.c` to the real game hooks
  (`-DW2_HAVE_GAME`); otherwise its stub test pattern runs.
- Env: `DEPLOY=dir` (default `D:\GITHUB\_AmiW2-poza-repo\emu\work`), `DEPLOY_AROS`, `DEPLOY_PLAY` (`none` skips),
  `W2_EXCLUDE="a.c b.c"`, `SHELL_ONLY=1` (platform shell only, ignore all game files), `STRICT=1` (-Werror).
- Gates: libm881 in the link trace = refused; FPU mnemonics in `native/*.s` = refused; never stripped.
- Icons: `tools/icons/draw_icons.py` (our own drawing -> 36x40 PNG) + `tools/mkicon_newicons.py` (from the build instructions, 15-ikony-newicons.md: NewIcons + planar fallback, WBTOOL, 2 MB stack, `--check` in the build).

## Code layout / API

- `native/` — platform layer (C, the only place with Amiga headers + `src/setup_amiga.c`). Contract:
  `native/amiga_gfx.h` (amigagfx_* — unchanged from AmiEU2), `amiga_audio.h`, `w2_sys.h`, `amiga_uclock.h`.
- `src/w2cfg.h` — settings (`w2_cfg` global, keys documented there) **and the game hooks** main calls:
  `game_init`, `game_tick` (fixed rate `w2_tick_ms()`), `game_draw(buf,pitch,w,h)` (return 1 = blit all),
  `game_event(ev)` (return 1 = consumed), `game_shutdown`, `game_wants_quit`; services `w2_log`, `w2_tick_ms`,
  `w2_frame_count`.
- `src/main_amiga.c` — shell: log, trap, cfg, screen, audio, loop, autoinput.

## Runtime (D:\GITHUB\_AmiW2-poza-repo\emu, copied from the EU2 runtime)

| file | what |
|---|---|
| `uae\winuae-w2.exe` | own WinUAE 2.8.1 copy |
| `boot.hdf` / `boot-play.hdf` | OS 3.1 boot disks (read-only mounts); User-Startup runs `Work:run` |
| `ROM3.1_A4000.40.70.ROM` | Kickstart |
| `aros-sys\` | AROS m68k nightly (ROM + system) |
| `work\`, `work-aros\`, `work-play\` | Work: of the three guests |

Configs (`winuae\`): `w2-ref020.uae` (reference: 68020, no FPU, no JIT, -80 %, AGA + uaegfx, 2 MB chip, 64 MB Z3),
`w2-aros.uae` (AROS), `w2-play.uae` (JIT, for the user; no job queue, Workbench comes up).
`Work:run` (LF endings!) = `Stack 2000000` + `Work:jobrun` (guest-side job queue).

## Run / test (harness in `winuae\harness`, PowerShell)

```
start-w2.ps1 -Config w2-ref020.uae                 # boots once, waits for jobrun
job-w2.ps1   -Script "Stack 2000000`nWork:w2"      # run anything inside the guest
test-w2.ps1  -Tag ref_aga -CfgLines "backend=aga"  # headless: cfg + autoinput, host shot, log, guest PPM->PNG
shot-w2.ps1  -Out D:\GITHUB\_AmiW2-poza-repo\emu\shots\x.png     # PrintWindow of OUR window
stop-w2.ps1  -Config w2-ref020.uae                 # WM_CLOSE to OUR instance only
```

Safety: instances are found by the full config path on the command line; never kill WinUAE by name or with
`-Force`; never synthesise host input — drive the program from `Work:autoinput.txt`
(`<ms> key|click|rclick|move|down|up|snap|log|quit ...`, see main_amiga.c). Read guest-written files from
PowerShell, not `/mnt/c` (drvfs caching). `w2.log` line 1 = version + `built:` stamp.

w2setup test rig: a `Work:w2setup.test` file holding `save`/`use`/`cancel` makes w2setup dump its window to
`w2setup.ppm`, log to `w2setup.log` and act without input.
