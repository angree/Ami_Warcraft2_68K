# Warcraft II 68K

A native port of Blizzard's **Warcraft II: Tides of Darkness** (1995) for classic 68k Amigas, AROS 68k
and MorphOS — one 68k binary for all of them, plus a native PowerPC binary for MorphOS.

The engine is our own code in plain C, written from scratch for the Amiga (chunky-to-planar, Paula,
no FPU needed). **No game data is included and none is needed from us: the game runs on the files of
your own Warcraft II CD**, which you convert once on a PC with the extractor below.

This is version 0.6.3: the human and orc campaigns of Tides of Darkness and custom games (skirmish)
against computer opponents on the maps of the CD. See "What works" below. Data converted for 0.1.0 works unchanged.

## 1. Extract the data from your own CD (on a PC)

You need your own **Warcraft II for Macintosh** CD (the port is developed and tested with it) as a CD
image: a `.cue` file with its `.bin` beside it. The image must be a raw one (`.cue`/`.bin`), because the
music is the CD audio of the disc. A PC CD image or a folder with the PC game files (`MAINDAT.WAR`, ...)
is accepted as well, but is untested.

**Windows:** unpack `Warcraft2_68K-<version>-converter.zip` anywhere and drag your `.cue` file onto
`extract.bat`. A window shows the progress; after a short while there is a folder `data` next to
`extract.bat`.

**Linux:**

```
./w2conv-linux /path/to/WarcraftII.cue .
```

What `data` contains:

| | |
|---|---|
| `War Data`, `War Resources`, `War Strings`, `War Sounds`, `War Music`, `War Movies` | the game archives of your CD, copied unchanged |
| `music/*.adp` | the CD audio tracks converted for the Amiga (about 20 MB) |
| `lores-*.pack` | the art of the 320x240 mode, scaled down once with a proper filter |
| `ehb-*.pack` | the art of the ECS (64 colour Extra-Half-Brite) mode |
| `pud/` | the custom maps of the CD |
| `conversion.txt` | what was converted, with checksums |

Nothing is downloaded and nothing is sent anywhere; the extractor only reads the image you give it.
Details: [docs/CONVERTER.md](docs/CONVERTER.md).

## 2. Install on the Amiga

1. Unpack `Warcraft2_68K-<version>.lha` — it holds the drawer `Warcraft2_68K`.
2. Copy the converted `data` folder into it, so that it is `Warcraft2_68K/data`.
3. Start **w2setup** once: display (AGA, RTG graphics card, ECS Extra-Half-Brite, Workbench window),
   resolution (640x480 or 320x240), sound (Paula, AHI, off), music, volumes, game speed. "Amiga bar"
   keeps the screen's title bar and depth gadget so you can switch to other programs.
4. Start **w2**. On MorphOS start **w2_MorphOS** instead: the same game as a native PowerPC program
   (much faster than the 68k binary under emulation; it uses the same `data` folder and the same settings).

Both programs have Workbench icons and take care of their 2 MB stack themselves.

Requirements: 68020 or better (68040 / fast 68030 recommended), AGA, an RTG card or ECS
(Extra-Half-Brite, 320x240 only), AmigaOS 3.x, AROS 68k or MorphOS, about 110 MB on disk for `data`.
Developed and tested with 2 MB chip + 64 MB fast RAM; the real minimum has not been measured yet.
Your own maps (`.pud`) go into a drawer `maps` next to the program.

## 3. What works in 0.6.3

- Both campaigns of Tides of Darkness (14 human and 14 orc missions) with the act screens, the
  briefings (text, voice, picture) and the movies of your CD, each mission's objectives as in the
  original ("Objectives" in the in-game menu), the units allowed per mission, rescues and heroes;
  progress is saved, every mission reached can be replayed.
- Spells and special abilities of both races (mana, all spells, Demolish), also cast by the computer.
- Transports: docking at the shore, boarding (up to 6), crossing, unloading; the computer ships
  troops to other islands when its AI profile asks for it.
- Custom game against 1–7 computer opponents on the 28 built-in multiplayer maps and the CD's custom
  maps; humans and orcs; gold, lumber and oil; building, training, research and upgrades; land, sea
  and air combat; fog of war; minimap; the original interface, sounds, unit voices and CD music.
- The computer players run the original AI programs from the game data (the profile each map gives
  them: land, sea, air, passive, campaign), with the old built-in plan as a fallback.
- In-game menu (F10 / Esc) with Options: sound and music volume, game speed, scroll speed — applied
  at once and saved.
- 640x480 (AGA, RTG) and 320x240 (AGA, RTG, ECS Extra-Half-Brite) with the hardware mouse pointer.
- "Amiga bar": the game keeps its full area under the screen's title bar (on RTG a slightly taller
  mode of the same width is used, e.g. 320x256; without one the bar is left out). "Show FPS" puts
  the frame rate into the bar when it is shown.

New in 0.6.3: a native MorphOS PowerPC binary, `w2_MorphOS`, in the drawer next to `w2` (tested on
MorphOS 3.20 in QEMU: RTG 640x480, computer vs computer at 110-140 frames per second).

New in 0.6.2: ships attack and destroy enemy oil platforms, as in the original (they could not
target them before, which blocked the sea missions); a destroyed platform gives its oil patch back with
the oil that was left.

New in 0.6.1: walls can be attacked and destroyed as in the original (Attack button or right-click
on a wall; a wall cracks, then falls to passable rubble; catapults, cannons and Demolish too; Demolish
also clears rocks). This fixes human mission 2, where the elven archers' pen must be broken open.

New in 0.6.0: the campaigns, spells and transports (above), all checked against the original game;
idle units look for targets once per idle animation loop as in the original (much faster on big
maps with many units: an orc mission with 126 units went from 7 to 27 fps on RTG 640x480).

New in 0.5.0: much faster drawing. The panel around the map is drawn only where it changed, the
map area is converted to the screen only in the 32x16 cells that changed (AGA / ECS), and the terrain
with its fog is kept in a cache and redrawn only for the tiles that changed. Measured in WinUAE
(68020, CPU throttled, no JIT), computer vs computer: AGA 320x240 29.7 -> 49.1 fps, AGA 640x480
11.6 -> 34.7 fps, RTG 640x480 26.6 -> 49.4 fps, ECS 34.1 -> 48.9 fps. 640x480 on AGA is now playable.

New in 0.4.0: the game logic now runs as in the original - every unit decides every game cycle, the
end of the game is checked every cycle, longer path searches - except two measured speed savings that
stay: the fog of war is updated every 4th cycle (every cycle costs 11-15 % frame rate on a 68020) and
at most 3 new path searches start per cycle (no stutter when a big army starts moving). The "Logic
skip" setting is gone: measured, it gained nothing.

New in 0.3.0: the original AI programs drive the computer players, and the computer only reacts to
what it can see; game rules checked against the original and corrected — building distance from
gold mines and oil, shallow water blocks land units and ships, coast is not buildable, gold mines
with no amount set hold 300 000, units marked passive on the map stay home, a unit that cannot leave
its building is refunded, a cancelled construction returns 75 % of gold and lumber (no oil), repair
costs 1 gold + 1 lumber every second step, melee hits need no range check, splash damage spares the
shooter; seven more multiplayer maps; missiles and attacks never hit a new unit that took a dead
unit's place; sight upgrades (Scouting, Longbow) now reveal the fog.

New in 0.2.0: the in-game Options page; gold mines and oil patches can be selected under the fog
once explored ("Gold Left"); 320x240 on RTG with the Amiga bar no longer cuts off the bottom of the
game; the frame counter goes into the title bar.

- Not yet: saving games, multiplayer, the Beyond the Dark Portal expansion.
  On MorphOS the native binary was tested in QEMU, not yet on real hardware. Big battles and long scrolling still cost
  more than a quiet screen.

## 4. Building

See [docs/BUILD.md](docs/BUILD.md) (bebbo's m68k-amigaos-gcc 6.5 in WSL/Linux; a host build with
sanitizers for testing: [docs/HOST_TEST.md](docs/HOST_TEST.md)). The CyberGraphX SDK headers are not
part of this repository (`native/cgx-include/`).

## 5. Legal

Warcraft II is a trademark of Blizzard Entertainment. This project is not affiliated with or endorsed
by Blizzard. It contains no Blizzard code and no Blizzard data, and it is useless without an original
copy of the game.

The source is under the GNU General Public License version 2 (see `LICENSE`). The 3x5 font is CC0.
The chunky-to-planar routines are by Mikael Kalms (public domain).

## 6. What it is based on

- **The original game itself.** How the game behaves (rules, timings, the interface layout, the AI's
  habits) is taken from our own analysis of the Macintosh version's 68k program, used as a
  specification only. None of its code is in this repository.
- **[Wargus](https://github.com/Wargus/wargus)** on the **[Stratagus](https://github.com/Wargus/stratagus)**
  engine (GPL v2), the open-source Warcraft II project. We read it as documentation, not as code to
  reuse: its data extractor documents the archive and image formats, and its scripts document the
  game facts (unit and upgrade costs, hit points, damage, ranges, build times, dependencies). Our unit
  and upgrade tables were generated from those facts and checked against the original where they
  differ. No Wargus or Stratagus source code is used; the engine is written from scratch.
- **Our own Amiga ports**: the methods of the StarCraft port (logic spread over frames, the lores 2:1
  mode, the 3x5 font, the hardware pointer, Smacker playback) and the platform layer and Workbench
  configurator of the Europa Universalis II port.
