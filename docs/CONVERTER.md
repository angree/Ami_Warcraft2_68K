# w2conv - making the AmiW2 data from your Warcraft II

AmiW2 ships no game data. `w2conv` (Windows `w2conv.exe`, Linux `w2conv`) builds the Amiga `data`
folder from **your own** Warcraft II: the Mac CD, the PC CD, or the installed game files.

## Windows

1. Unpack the release folder (`w2conv.exe`, `extract.bat`, `README.txt`) anywhere - spaces and
   parentheses in the path are fine.
2. Drop your CD image (`.cue` with its `.bin` beside it, or `.iso`) or your game folder onto `extract.bat`.
   A console window shows the progress (archives, music tracks, lores packs in 5% steps) and waits
   for a key at the end. Double-clicked without anything dropped, it only says what to do.
3. Copy the new folder `data` (next to `extract.bat`) to the Amiga, into the Warcraft2_68K drawer (`Warcraft2_68K/data`).

## Command line (Windows / Linux)

```
w2conv [options] <source> <outdir>        ->  <outdir>/data
  <source>   Mac or PC CD image (.cue + .bin, .iso), or a folder with the game files
  --cue X    take the music from CD image X when <source> is a folder
  --rate N   music sample rate 11025, 14700 (default) or 22050 Hz
  --no-music skip the music
  --no-lores skip the lores packs
  --no-ehb   skip the ECS Extra-Half-Brite packs
  --list     list the files on a Mac CD image
w2conv --lores-only <outdir>              only (re)build the lores + EHB packs of <outdir>/data
                                          (with --no-lores / --no-ehb: only the other kind)
```

## What you get

| in `data/` | from |
|---|---|
| `War Data`, `War Resources`, `War Strings`, `War Sounds`, `War Music`, `War Movies` (Mac) or `MAINDAT.WAR`, `REZDAT.WAR`, `STRDAT.WAR`, `SFXDAT.SUD`, `SNDDAT.WAR`, `MUDDAT.CUD` (PC) | copied unchanged; the game accepts both names |
| `Title Song`, `music/title.adp` | Mac CD menu music |
| `music/trackNN.adp` | CD audio tracks 2-17 -> mono IMA ADPCM (about 20 MB for the whole CD at 14700 Hz) |
| `lores-summer.pack`, `lores-winter.pack`, `lores-wasteland.pack` | the 320x240 mode's halved art (about 2.9 MB), see below |
| `ehb-summer.pack`, `ehb-winter.pack`, `ehb-wasteland.pack` | the ECS mode's art (backend=ehb: 320x240, 64 colours), see below |
| `pud/*.pud` | Mac CD "Custom PUDs" |
| `conversion.txt` | version stamp (`AmiW2 data conversion 1`), file sizes and CRC32s (archives, `lores` / `ehb` lines for the packs), `result ok`; written with LF line ends on every host |

Without CD audio (a game folder and no `--cue`) everything works, only the music stays silent.
The music is taken from the Red Book audio of the CD, so it needs a `.cue`/`.bin` (raw) image, not
a plain `.iso`. A Mac CD read by our own HFS reader and a PC CD (ISO 9660 data track) both work.

## Lores packs

The lores mode needs `<data_dir>/lores-<era>.pack`; without them the game builds them itself on its
first lores start (about 2 minutes on a 68030). w2conv builds them on the PC in well under a second
with **the game's own code**: `src/lores.c`, `assets.c`, `war.c`, `w2gfx_decode.c`, `w2cfg.c`,
`w2_tables.c`, `font35.c` are linked unchanged; `tools/w2conv/lores_pack.c` is only the platform shim
(`w2_log` -> stdout, `amigagfx_millis` -> `clock()`, `amigagfx_set_palette` no-op) and calls
`assets_init()` + `lores_prepare()` with `resolution=lores`, `data_dir=data` from inside `<outdir>`
(so long Windows paths never hit the game's 128/300-byte path buffers). The packs are endian- and
word-size-neutral: w2conv.exe (Windows, LLP64), w2conv (Linux, LP64) and the host game write
byte-identical files, and the game accepts them (`lores: ... ok`, no rebuild). Packs already present
and current are kept (same header check as the game).

## EHB packs (ECS, backend=ehb)

An Extra-Half-Brite screen shows 64 colours: 32 registers of 12 bits and their 32 hardware halves.
`<data_dir>/ehb-<era>.pack` is the lores pack again (same format, same halving code) with a different
DESTINATION palette: source pixels are read through the era's 256 colours and the result is quantised
to the era's 64-colour EHB palette, so every byte is 0..63. That palette comes from `src/ehb.c`
(linked into w2conv unchanged; integers only, so the PC and the Amiga compute the same bytes):

- register 0 black; one register per player (red 1, blue 2, green 3, violet 4, orange 5, yellow 6,
  black 17, white 19) - the 4-shade player ramp becomes register (2 bright shades) + its half (2 dark);
- 17 / 18 / 19 are also the hardware pointer's colours on OCS/ECS: black player, grey 152, white player;
- the other 22 registers: weighted k-means over the 256 source colours on the 12-bit grid (weights =
  pixel counts of the era's megatiles and of the pack's sprites, each half of the total), every colour
  served by a register or by a half; then rounds of "drop the least useful register, seed it at the
  worst served colour".

Sprites carry the player colour as the placeholder pair 4 / 36 (the violet player's own register),
which the remap tables send to the drawing player's register; no other converted pixel uses that pair.
The 64 colours are stored at the end of the pack (`"EHBP"` + 64 x RGB after the sprite data); the game
reads them from there. The pack stamp is the source palette + `EHB_VERSION` (ehb.h). Without the packs
the game builds them itself at its first EHB start, like the lores ones. `w2conv` builds them right
after the lores packs (`--no-ehb` skips). Menu pictures get their own 64 colours at run time (same
k-means weighted by the picture; 0 black, 17/18/19 = 40/152/240 grey for the pointer).

## Building and the release

- `sh build/build-w2conv.sh` in WSL/Linux: `build/host/w2conv` and, with `x86_64-w64-mingw32-gcc`,
  the static `build/host/w2conv.exe`.
- `sh build/package-w2conv.sh [dir]` builds and writes the release folder (default
  `D:\GITHUB\_AmiW2-poza-repo\dist\w2conv\`, outside the repo): `w2conv.exe`, `extract.bat`,
  `README.txt` (from `tools/w2conv/`, CRLF). It contains no game data.

## Verified (2026-11-13, EHB packs)

- `w2conv.exe --lores-only` run natively on Windows on a copy of the data: the three `ehb-*.pack` are
  byte-identical to the ones the host game (`backend=ehb`) writes itself; the host game given them logs
  `ehb: ... ok` three times and does not rebuild. The three `lores-*.pack` did not change.

## Verified (2026-11-12, Mac CD image WAR2_122.CUE)

- `w2conv.exe` run natively on Windows (also through `extract.bat` from a folder with spaces and
  parentheses): all 32 data files byte-identical to the Linux build's output; CRCs in
  `conversion.txt` identical.
- The three packs from `w2conv.exe` are byte-identical to the ones the host game (`build-host.sh`,
  lores, autostart 220) writes, and to Linux `w2conv --lores-only`; the host game given the w2conv
  data logs `lores: ... ok` for all three and does not rebuild.
