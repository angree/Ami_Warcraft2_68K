# Host build (Linux / WSL) - run and test without an emulator

The game reaches the machine only through `amiga_gfx.h`, `amiga_audio.h` and `w2_sys.h`. The host build
replaces those with `native/host_gfx.c`, `native/host_audio.c` and `native/host_sys.c`, and builds every
`src/*.c` (except `setup_amiga.c`) with `-std=gnu99 -O1 -g -fsanitize=address,undefined -Wall -DW2_HOST`.
A run takes seconds, is deterministic, and any out-of-bounds access stops it with an ASan report.

## Build

```
wsl -d Ubuntu-22.04 -- bash /mnt/d/GITHUB/Ami_Warcraft2/build/build-host.sh [--test] [--no-sdl]
```

| output | what |
|---|---|
| `build/host/w2` | the game (`src/main_amiga.c` -> `w2_main()`); falls back to the self-test main if `main_amiga.c` is missing or does not compile for the host |
| `build/host/host_selftest` | `tools/host_selftest.c`: API-only test (640x480 gradient, scripted mouse cross, beep, `PROGDIR:` file) |

- `--test` also runs both headless: `build/host/selftest/selftest.png` and `build/host/run/game.png`
  (+ `build/host/run/w2.log`). Exit code non-zero on any failure.
- `W2_HOST_EXCLUDE="a.c b.c"` leaves more `src/` files out. `-DW2_HAVE_GAME` is added automatically once a
  `src/*.c` other than `main_amiga.c` defines `game_init` (same rule as `build.sh`).
- SDL2 window support is compiled in when `sdl2-config` exists (`--no-sdl` to skip). It is used only at run
  time when `DISPLAY`/`WAYLAND_DISPLAY` is set and `W2_HEADLESS` is not - WSL1 has no display, so headless
  is the default there.

## Run

```
cd /mnt/d/GITHUB/Ami_Warcraft2
W2_PROGDIR=/path/to/gamedir W2_INPUT=test.txt W2_SHOTDIR=shots build/host/w2
```

| env | default | meaning |
|---|---|---|
| `W2_PROGDIR` | `.` | directory `PROGDIR:` maps to (data, cfg, `w2.log`, `autoinput.txt`) |
| `W2_INPUT` | none | input script (below) |
| `W2_SHOTDIR` | `.` | where `shot` PNGs go; when set, `final.png` is also written at `amigagfx_close` (`W2_FINALSHOT=0` off) |
| `W2_STDOUT` | off | `1` = keep the game's log (`freopen(PROGDIR:w2.log, stdout)`) on the terminal |
| `W2_TIMEOUT` | 600000 | virtual ms after which a QUIT event is sent; 30 s later `exit(3)`. `0` = never |
| `W2_REALTIME` | 0 | `1` = real clock instead of the virtual one (default 1 when an SDL window is open) |
| `W2_POLL_MS` / `W2_MILLIS_US` | 1 / 20 | virtual time added by an empty `amigagfx_poll` / by each `amigagfx_millis` call |
| `W2_HEADLESS` | 0 | `1` = never open an SDL window |
| `W2_SCALE` | 1 | SDL window scale; F12 in the window = shot `f12.png` |
| `W2_WAV` | 0 | `1` = mix all sound (SFX + music) into `W2_SHOTDIR/audio.wav`, 22050 Hz mono |
| `W2_AUDIOLOG` | 1 | `0` = do not log audio calls |
| `W2_NOAUDIO` | 0 | `1` = `AmigaAudio_Open` fails (silent path) |
| `W2_TRACEFILES` | 0 | `1` = log every mapped file call |
| `W2_NTSC`, `W2_AGA`, `W2_CPU`, `W2_RTG` | PAL, 1, 60, 1 | what the machine queries answer |
| `W2_TMPDIR` | `/tmp` | target of `T:` and `RAM:` |

Host messages go to stderr as `[host <virtual ms>] ...`.

### Virtual clock

`amigagfx_millis` is virtual by default: an empty poll adds 1 ms, `w2_wait_tof` adds 20 ms (17 with
`W2_NTSC`), `w2_sleep_ms` its argument, every `millis` call 20 us (so busy-waits end). The same input gives
the same frames and the same shots on every run, however fast the host is.

### Paths

`fopen`, `freopen`, `remove`, `unlink`, `rename`, `mkdir`, `rmdir`, `opendir`, `stat`, `access` are linked
with `-Wl,--wrap`: `PROGDIR:x` -> `$W2_PROGDIR/x`, `T:`/`RAM:` -> `$W2_TMPDIR/`, any other `DEV:` ->
`$W2_PROGDIR/_DEV/` (e.g. `ENV:w2.cfg` -> `_ENV/w2.cfg`). Names that do not exist as written are matched
case-insensitively per path component, like AmigaDOS.

## Input script (`W2_INPUT`)

One action per line, `#` comments. Time is virtual ms since start: `at <ms>`, or relative to the previous
line with `+<ms>` / `after <ms>`.

```
at 500  move 100 120          # pointer move
at 600  down left 100 120     # button down (x y optional: current position); left|right
at 650  up left               # button up
at 800  click right 300 200   # down + up
at 900  key esc               # press + release (code | 0x80); keydown / keyup for one half
+100    key 0x44              # raw key code, decimal or 0x..; names: esc return space tab backspace del help
                              #   up down left right f1..f10 shift rshift ctrl alt ramiga lamiga a..z 0..9 - = [ ] ; ' , . /
+100    type hello            # letters/digits as key presses
+500    shot title            # W2_SHOTDIR/title.png (what was last BLITTED, with the current palette)
+100    quit                  # AMIGAGFX_EV_QUIT
+100    exit 0                # hard exit(code), for a game that ignores quit
```

A shot is taken when the queue reaches it, i.e. after the game has polled every earlier event. It shows
the display, not the chunky buffer: an area drawn but never passed to `amigagfx_blit` shows the old image,
exactly as on the Amiga (AGA/EHB blits are widened to 32-pixel columns like the c2p).

The game's own `PROGDIR:autoinput.txt` (see `main_amiga.c`) works on the host too, through `W2_PROGDIR`.

## EHB (backend=ehb)

With `backend=ehb` in `w2.cfg` (and `W2_AGA=0` for the "auto" path) the host display behaves like the
chipset's Extra-Half-Brite screen: `amigagfx_set_palette` keeps 4 bits per gun of registers 0..31 and
derives 32..63 as their halves (every gun shifted right once), a shot shows the blitted indices through
those 64 colours (only the low 6 bits of an index exist). Bugs are counted and the first few logged
as `EHB ERROR`: a palette call that touches a register >= 32, a blitted pixel >= 64. The exit line
`EHB check: 0 pixels >= 64 blitted, 0 palette calls beyond register 31 (clean)` must say clean.

## Notes

- `amigagfx_splash`, pointer images, title bar, Workbench window resize are accepted and ignored.
  `show_bar` takes 11 lines off the game height, like a topaz-8 screen bar.
- The chunky buffer is allocated at exactly `pitch * game_height`, so ASan catches any write outside it.
- LeakSanitizer is off by default (it does not work under WSL1); on real Linux:
  `ASAN_OPTIONS=detect_leaks=1`.
