# Warcraft II data formats (Mac CD 1.23, verified)

Status: every statement marked **verified** was checked against the real Mac files in `hfs/Data`
with our own tools (`tools/w2data.py`, `tools/test_war.c`; both produce identical CRCs over all
decompressed entries and all decoded sprite frames). Items marked **UNCERTAIN** are inferred or taken
from third-party notes (wargus, read as documentation only) and not proven against the original code.

Full per-entry listing: [ENTRY_MAP.md](ENTRY_MAP.md).

All multi-byte numbers in all formats are **little-endian** (also on the Mac). Read them byte-wise.

---

## 1. Files

| Mac file (`Data/`) | PC equivalent | archive id | entries | contents |
|---|---|---:|---:|---|
| `War Data` | MAINDAT.WAR | 1000 | 438 | palettes, tilesets, sprites, icons, UI images, cursors, fonts, PUDs, XMIDI, logo SMK, UI sounds, default unit data |
| `War Resources` | REZDAT.WAR | 3000 | 100 | menu widgets, menu panels/backgrounds, act title screens, menu layouts |
| `War Strings` | STRDAT.WAR | 4000 | 105 | string tables: unit names, menus, briefings, objectives |
| `War Sounds` | SFXDAT.SUD | 5000 | 293 | sound effects & unit voices (WAV) |
| `War Music` | SNDDAT.WAR | 2000 | 49 | **briefing/victory speech** (WAV 22 kHz), not music, plus 3 menu string tables |
| `War Movies` | MUDDAT.CUD | 6000 | 15 | Smacker SMK2 videos + 3 menu layouts |
| `Title Song` | (CD track) | - | - | plain RIFF WAVE, PCM mono 16-bit 22050 Hz, ~85 s |

* The CD is the **original game (Tides of Darkness)**, not Beyond the Dark Portal: no swamp tileset,
  no expansion campaigns. wartool's own Mac detection agrees ("original MAC CD": War Resources size !=
  2876978).
* **Entry numbering is identical to the PC original-CD tables** (wartool `Todo` rows with version 0)
  for War Data, War Resources, War Strings, War Sounds, War Music (=snddat) and War Movies (=muddat).
  Verified entry by entry by type/size/visual check (see ENTRY_MAP.md).
* wartool already has Mac support (`CD_MAC`, `ConvertToMac()` maps rezdat->War Resources,
  strdat->War Strings, maindat->War Data, snddat->War Music, muddat->War Movies, sfxdat->War Sounds).
  Its only Mac quirks are for the Mac *expansion* CD (image index 94..103 +7, palette 93 -> 100,
  text index >=99 +6); none apply to this original CD.
* Music in the game: XMIDI in War Data 379-429 and/or the CD audio tracks 2-17. "War Music" holds speech.
* `Custom PUDs/*.pud` are plain PUD files (same format as the PUD entries inside War Data).

## 2. Archive container (verified)

```
off 0  u32  magic = 0x00000019
off 4  u16  entry count
off 6  u16  archive id (1000/2000/3000/4000/5000/6000)
off 8  u32  offset[count]          absolute file offsets of the entries
entry: u32  w; size = w & 0x00FFFFFF (unpacked size), flags = w >> 24
       payload follows
```

* flags `0x00` = stored (payload = `size` bytes), `0x20` = compressed (below). No other values occur.
* **Placeholders**: 23 War Data entries, 1 War Strings and 2 War Music entries have an offset that is
  less than 4 bytes before the next entry (typically exactly 1 byte, or 1 byte before EOF for the last
  entry). They contain nothing; the "header" read there is garbage (bytes of the next entry). Rule used
  by our readers: `avail = (next larger offset or file size) - offset; avail < 4 -> empty entry`.
* Offsets are not necessarily sorted; compute `avail` from the sorted offset list.
* War Sounds: after each WAV payload (outside `size`) the original DOS file name is stored as a
  0-terminated string (`MISC\ERROR.WAV`...). Readers must not assume entries are contiguous.

### 2.1 Compression (flags 0x20) - verified on all 600+ compressed entries

LZSS with a 4096-byte history ring:

```
ring[4096] = all 0; rpos = 0; out = []
while len(out) < size:
    ctrl = next byte
    for bit in 0..7 (LSB first), while len(out) < size:
        if ctrl & (1<<bit):                # literal
            b = next byte; out += b; ring[rpos++ & 0xFFF] = b
        else:                              # back reference
            v = next u16 LE
            src = v & 0x0FFF               # ABSOLUTE ring position (not a distance!)
            n   = (v >> 12) + 3            # 3..18 bytes
            repeat n times (stop early when len(out) == size):
                b = ring[src++ & 0xFFF]; out += b; ring[rpos++ & 0xFFF] = b
```

Decoding stops exactly at `size` bytes, also in the middle of a group or a copy. Implementation note
(war.c): ring slot `s` always equals the newest output byte at position `k < pos` with
`k & 0xFFF == s` (or 0 if none), so the decoder needs no separate ring and can decompress directly
into the destination buffer.

## 3. Palettes (verified)

* 768 bytes = 256 x (R,G,B), **6-bit VGA values 0..63** (all palettes have max < 64).
  Convert with `v8 = (v << 2) | (v >> 4)` (or `v << 2`).
* Game palettes: War Data **2 = summer (forest)**, **10 = wasteland**, **18 = winter**. Units,
  buildings, icons, UI panels in War Data use the current era palette (115 of the 128 entries
  128-255 are identical in all three era palettes, the low half is terrain-specific).
* Other palettes are full-screen picture palettes: War Data 300 (title 299), 363-366 (victory/defeat
  359-362), 367 (human intro screens), 368 (orc intro screens); War Resources 14 (menu panels/background,
  most widgets), 16 (menu background 15), 17 (act title screens 19-26), 27 (credits background 28,
  human The End 29, first 3 widget frames, credits-arrow cursor), 32 (orc scroll 30); 18 and 31
  usage unknown.
* Index 0 = black; sprites carry their own transparency (RLE skips), cursors use 0 = transparent,
  full-screen images are opaque.

### 3.1 Player colours (verified in palette 2/10/18)

Sprites are drawn in **red = indices 208..211** (bright..dark). Remap these 4 indices per player:

| player | colour | palette indices |
|---:|---|---|
| 0 | red | 208-211 (identity) |
| 1 | blue | 212-215 |
| 2 | green | 216-219 |
| 3 | violet | 220-223 |
| 4 | orange | 224-227 |
| 5 | black | 228-231 |
| 6 | white | 232-235 |
| 7 | yellow | **12-15** (not in the 208.. block; 236-239 are browns/black) |

Values match wargus' `DefinePlayerColors` table exactly.

### 3.2 Colour cycling (UNCERTAIN - wargus tileset scripts; ranges contain blue water shades in our palettes)

| era | ranges (inclusive) |
|---|---|
| summer | 38-47 water, 48-56 coast boundary, 240-244 (icons/UI) |
| wasteland | 38-47, 64-70, 240-244 |
| winter | 40-47, 48-54, 205-207 (buildings), 240-244 |

Rotate each range by one entry per cycle step. The real period and direction must still be taken
from the original binary.

## 4. Sprites ("gfx", RLE) - verified on all 205 sprite entries / 2894 frames

```
u16 count, u16 box_w, u16 box_h
count x { u8 x, u8 y, u8 w, u8 h, u32 offset }   # offset from entry start;
                                                 # bit31 set -> w += 256, offset &= 0x7FFFFFFF
frame data at offset:
  u16 row_off[h]          # relative to the frame data start (row_off[0] == 2*h)
  row: until w pixels produced:
    c & 0x80  : skip (c & 0x7F) transparent pixels
    c & 0x40  : (c & 0x3F) times the following byte
    else      : (c & 0x3F) literal bytes follow
```

* `(x,y,w,h)` is the frame's bounding box inside the `box_w x box_h` cell. The engine draws the
  cell centred on the unit's tile footprint: `cell_x = tile_x*32 + (size_w*32 - box_w)/2`
  (same for y). Verified visually on a campaign map render (units/buildings line up with tiles).
* Empty frames (w or h 0) exist.
* **Direction layout** (verified on Footman 45): frames come in groups of 5 =
  **N, NE, E, SE, S**; frame number = `anim_frame_base + dir`, dir 0..4. **SW, W, NW = mirrored
  SE, E, NE** (dir 3,2,1 drawn with horizontal flip *inside the box*: `x' = box_w-1-x`).
* Animation bases (wargus anim.lua, matches the art): e.g. Footman: still 0, walk 5/10/15/20,
  attack 25/30/35/40, death 45/50/55. Real per-unit scripts are in War Data 278 (not decoded).
* Buildings: 2 frames (0 = finished, 1 = under construction stage); oil platform 3; gold mine
  2 (1 = active); construction sites 2-3 frames; walls 2.
* Icons (War Data 356/357/358 per era): one 186-frame sprite, 46x38 per icon; icon id = frame.
* "gfu" variant (uncompressed frames, same header, frame data = `w*h` raw bytes, no transparency):
  War Data 187 (resource icons 4x16x16), 323 (spell status 5x16x16), 354/355 (info panel 4x176x176),
  War Resources 0/1 (menu widgets, 50 frames, 300 px wide -> width high bit used).
* Detection rule (both our readers): header sane, every frame fits the box, offsets <= size;
  RLE if every non-empty frame has `u16 at offset == 2*h`; else gfu if every frame has `offset+w*h <= size`.

## 5. Images (verified)

`u16 w, u16 h, w*h bytes` (8-bit, row-major, opaque). UI panels (287-298 in War Data), full-screen
640x480 pictures, menu panels (War Resources 3-13). Palette: see ENTRY_MAP.

## 6. Cursors (verified)

`u16 hot_x, u16 hot_y, u16 w, u16 h, w*h bytes`, index 0 = transparent. War Data 301-322.

## 7. Fonts (verified - 5 fonts War Data 279-283)

```
"FONT "  (5 bytes)
u8  last_char            # 0xEF: glyph slots for chars 32..last_char (208 slots)
u8  max_w, u8 max_h
u32 glyph_off[last_char-31]   # from entry start, 0 = no glyph
glyph: u8 w, u8 h, u8 x_off, u8 y_off, then a stream of bytes:
   pos += byte >> 3   (skip transparent)   ; stop when pos >= w*h
   pixel at (pos % w, pos / w) = colour byte & 7 ; pos += 1
```

Colour 0 inside the stream (very rare) is treated as transparent. Glyph colours 1..7 are
logical shades mapped to palette indices by a per-text-colour table chosen by the engine (wargus
fonts.lua lists such tables, e.g. for player colours via 208..211; UNCERTAIN which tables the
original uses). Character set: DOS code page 437 (Mac data unchanged). 279 large episode titles
(52x50), 280 small episode titles (32x35), 281 large (17x17), 282 game (13x14), 283 small (7x6).

## 8. Tilesets (verified by rendering all three eras and full campaign maps)

Per era four entries in War Data: palette, megatiles, minitiles, tile map.

| era (PUD `ERA`) | palette | megatiles | minitiles | tile map | megatile count |
|---|---:|---:|---:|---:|---:|
| 0 summer (forest) | 2 | 3 | 4 | 5 | 372 |
| 1 winter | 18 | 19 | 20 | 21 | 379 |
| 2 wasteland | 10 | 11 | 12 | 13 | 373 |
| 3 swamp | expansion only - not on this CD; fall back to wasteland | | | | |

* **minitiles**: 64 bytes each = 8x8 pixels, row-major.
* **megatiles**: 32 bytes each = 16 x u16, 4x4 minitiles row-major forming a 32x32 tile.
  `ref = (minitile_index << 2) | (flip_x << 1) | flip_y` (minitile byte offset = `(ref & ~3) * 16`).
* **tile map** (6636 bytes): 0x9E groups x 42 bytes:
  `u16 megatile[16]; u8 terrain[8]; u16 flags`.
  **PUD MTXM value v -> megatile = group[v >> 4].megatile[v & 15]**.
  - Solid tiles `0x00T0..0x00TF` (T = group 1..0xC: 1/2 water, 3/4 coast, 5/6 ground/grass, 7 forest,
    8 mountains/rock, 9/B human walls, A/C orc walls). Low nibble = variation.
  - Boundary tiles `0x0ABC` (A = pair 1..9, B = corner shape, C = variation): groups 0x10..0x9D.
    A: 1 dark water/water, 2 water/coast, 3 dark coast/coast, 4 mount/coast, 5 coast/grass,
    6 dark grass/grass, 7 forest/grass, 8 human wall, 9 orc wall (wargus naming, consistent with
    the terrain bytes).
  - Wall/forest groups store damaged/destroyed variants at higher `j` (e.g. group 9: j 0, 2, 4).
  - `terrain[8]` = terrain codes (1..0xA) of the group's components; `flags` observed: 0x40 water,
    0x11 coast, 0x01 land, 0x81 forest/rock (impassable), 0x8D human wall, 0x89 orc wall.
    Bit meanings beyond that: UNCERTAIN (use PUD SQM for passability).
* Megatiles 0..15 render blank (wartool treats them as raw fog-of-war data - UNCERTAIN).
* Era-specific sprites: winter has its own building set (128-172, 184, 186) and critter (66);
  wasteland only overrides farm, lumber mill, oil platform, gold mine, oil patch, submarine/turtle,
  dark portal, destroyed sites, walls (173-191, 274-276) - **all other wasteland buildings use the
  summer sprite drawn with the wasteland palette 10**. Icons: 356 summer, 357 winter, 358 wasteland.

## 9. Sounds (verified)

* War Sounds 2..292 and War Data 432..436: RIFF WAVE PCM **mono 8-bit unsigned 11025 Hz** (289 of
  291 sfx; 2 are 16-bit). War Data 433/434 (human/orc "act" sounds) and 435/436 (UI clicks): 22050 Hz 8-bit; 432 is 11025 Hz.
* War Music 2..45: speech, RIFF WAVE mono 8-bit **22050 Hz** (20-40 s each).
* War Sounds 0: u32 table of lengths; 1: description strings (`xxxxxxxx|Group` / name records).
* Use the RIFF chunks (`fmt `, `data`) - some `fmt ` chunks are 18 bytes, so do not hard-code offset 44.

## 10. String tables (verified)

```
u16 n; u16 off[n]; strings (0-terminated, CP437)   # string i at off[i], offsets from entry start
```

* War Strings 1: 428 strings; **string t+1 = name of unit type t** (string 0 is empty), followed by
  upgrade/spell/command names. Hotkey letters in menu strings are preceded by the hotkey char and
  bytes 0x04 / 0x01 (highlight on/off).
* Briefings War Strings 65..92 = n=1 tables (text at offset 4), order human 1, orc 1, human 2 ...
  War Strings 93 = 2 strings (human / orc victory text). 54 = objectives + level titles. 58 credits.
* Menu layouts (War Resources 33-99, War Movies 1/7/13): N x 72-byte widget records
  (rects in 640x480 coordinates); layout not decoded (UNCERTAIN).

## 11. Videos

Smacker `SMK2` (War Movies, War Data 430). 320x144 (cut-scenes), 320x200 (intro 5/6), logo 320x160.

## 12. PUD map format (verified on all War Data PUDs and Custom PUDs)

Sequence of sections: `char tag[4]; u32 length; data`. Order on this CD:
`TYPE VER DESC OWNR ERA DIM UDTA UGRD SIDE SGLD SLBR SOIL AIPL MTXM SQM OILM REGM UNIT`.

| tag | length | content |
|---|---|---|
| `TYPE` | 16 (Mac/this CD) | "WAR2 MAP\0\0" + u16 + u32 map id (PC files often 10) |
| `VER ` | 2 | 0x11 original, 0x13 expansion |
| `DESC` | 32 | description, 0-terminated |
| `OWNR` | 16 | per player 0..15: 2 neutral, 3 nobody, 4 computer, 5 human/person, 6 rescue passive, 7 rescue active (0/1 = passive/computer variants, UNCERTAIN) |
| `ERA ` / `ERAX` | 2 | tileset 0 summer, 1 winter, 2 wasteland, 3 swamp (`ERAX` overrides) |
| `DIM ` | 4 | u16 width, u16 height (32/64/96/128) |
| `UDTA` | 5696 (VER 0x11) / 5950 (VER 0x13) | unit data, below |
| `ALOW` | 384 | 16 players x u32 bitmask for: units/buildings, spells at start, spells allowed, spells researching, upgrades allowed, upgrades researching (optional) |
| `UGRD` | 782 | upgrade data: u16 use-default; u8 time[52]; u16 gold[52]; u16 lumber[52]; u16 oil[52]; u16 icon[52]; u16 group[52]; u32 flags[52] |
| `SIDE` | 16 | race per player: 0 human, 1 orc, 2 neutral |
| `SGLD` `SLBR` `SOIL` | 32 | u16 starting gold / lumber / oil per player |
| `AIPL` | 16 | AI program per player (index into AI scripts, War Data 277) |
| `MTXM` | w*h*2 | u16 tile values (section 8) row-major |
| `SQM ` | w*h*2 | u16 movement/passability flags per tile |
| `OILM` | w*h | u8, obsolete |
| `REGM` | w*h*2 | u16 action/region map (forest/wall/rock state) |
| `SIGN` | 4 | optional (BNE) |
| `UNIT` | n*8 | `u16 x, u16 y, u8 type, u8 player, u16 data`: x,y = top-left tile; data = gold/oil amount / 2500 for mines/patches/platforms, else 0 (passive) / 1 (active) |

Player 15 = neutral (critters, mines). Start locations are units 0x5E/0x5F.

### 12.1 UDTA (verified: field boundaries line up with known stats; default copy = War Data 0)

`u16 use_default` (1 = ignore and use built-in defaults - campaign PUDs carry stale data with 1),
then arrays indexed by unit type (110 entries unless noted):

| field | type | notes (Footman values from War Data 0) |
|---|---|---|
| overlap frames | u16[110] | 0 for units, 6..15 for buildings |
| obsolete | u16[508] | |
| sight | u32[110] | tiles (Footman 4) |
| hit points | u16[110] | 60 |
| magic | u8[110] | 1 = has mana |
| build time | u8[110] | 60 (x6 = game ticks, UNCERTAIN) |
| gold cost | u8[110] | x10 -> 600 |
| lumber cost | u8[110] | x10 |
| oil cost | u8[110] | x10 |
| size | u32[110] | tiles: low u16 / high u16 = w / h (all footprints are square; order UNCERTAIN) |
| box | u32[110] | selection box pixels (low/high u16), Footman 31x31 |
| attack range | u8[110] | 1 |
| react range computer | u8[110] | |
| react range human | u8[110] | |
| armor | u8[110] | 2 |
| selectable via rectangle | u8[110] | |
| priority | u8[110] | |
| basic damage | u8[110] | 6 |
| piercing damage | u8[110] | 3 |
| weapons upgradable | u8[110] | |
| armor upgradable | u8[110] | |
| missile weapon | u8[110] | |
| unit type | u8[110] | 0 land, 1 fly, 2 naval |
| decay rate | u8[110] | |
| annoy computer factor | u8[110] | |
| 2nd mouse button action | u8[58] | units 0..57 only |
| point value | u16[110] | 50 |
| can target | u8[110] | 1 land, 2 sea, 4 air |
| flags | u32[110] | Footman 0x08080001 |
| obsolete | u16[127] | **only in VER 0x13 (5950-byte) UDTA** |

War Data 0 = this structure without `use_default` (5694 bytes); War Data 1 = UGRD without its
leading u16 (780 bytes).

### 12.2 Unit types (names from War Strings 1; verified)

| id | name | id | name | id | name |
|---|---|---|---|---|---|
| 00 | Footman | 23 | (Deathwing, exp.) | 46 | Gryphon Aviary |
| 01 | Grunt | 24 | - | 47 | Dragon Roost |
| 02 | Peasant | 25 | - | 48 | Human Shipyard |
| 03 | Peon | 26 | Gnomish Submarine | 49 | Orc Shipyard |
| 04 | Ballista | 27 | Giant Turtle | 4A | Town Hall |
| 05 | Catapult | 28 | Gnomish Flying Machine | 4B | Great Hall |
| 06 | Knight | 29 | Goblin Zeppelin | 4C | Elven Lumber Mill |
| 07 | Ogre | 2A | Gryphon Rider | 4D | Troll Lumber Mill |
| 08 | Elven Archer | 2B | Dragon | 4E | Human Foundry |
| 09 | Troll Axethrower | 2C | (Turalyon, exp.) | 4F | Orc Foundry |
| 0A | Mage | 2D | Eye of Kilrogg | 50 | Mage Tower |
| 0B | Death Knight | 2E | (Danath, exp.) | 51 | Temple of the Damned |
| 0C | Paladin | 2F | (Korgath, exp.) | 52 | Human Blacksmith |
| 0D | Ogre-Mage | 30 | - | 53 | Orc Blacksmith |
| 0E | Dwarven Demolition Squad | 31 | Cho'gall | 54 | Human Refinery |
| 0F | Goblin Sappers | 32 | Lothar | 55 | Orc Refinery |
| 10 | Attack Peasant | 33 | Gul'dan | 56 | Human Oil Platform |
| 11 | Attack Peon | 34 | Uther Lightbringer | 57 | Orc Oil Platform |
| 12 | Ranger | 35 | Zuljin | 58 | Keep |
| 13 | Berserker | 36 | - | 59 | Stronghold |
| 14-19 | (expansion heroes) | 37 | Skeleton | 5A | Castle |
| 1A | Human Oil Tanker | 38 | Daemon | 5B | Fortress |
| 1B | Orc Oil Tanker | 39 | Critter | 5C | Gold Mine |
| 1C | Human Transport | 3A | Farm | 5D | Oil Patch |
| 1D | Orc Transport | 3B | Pig Farm | 5E | Human Start Location |
| 1E | Elven Destroyer | 3C | Human Barracks | 5F | Orc Start Location |
| 1F | Troll Destroyer | 3D | Orc Barracks | 60 | Human Guard Tower |
| 20 | Battleship | 3E | Church | 61 | Orc Guard Tower |
| 21 | Ogre Juggernaught | 3F | Altar of Storms | 62 | Human Cannon Tower |
| 22 | - | 40 | Scout Tower (human) | 63 | Orc Cannon Tower |
| | | 41 | Watch Tower (orc) | 64 | Circle of Power |
| | | 42 | Stables | 65 | Dark Portal |
| | | 43 | Ogre Mound | 66 | Runestone |
| | | 44 | Gnomish Inventor | 67 | Wall (human) |
| | | 45 | Goblin Alchemist | 68 | (orc wall, unnamed) |

0x69..0x6D: unused slots of the 110-entry UDTA arrays. Hero units 0x31-0x35 reuse the art of Ogre-Mage,
Knight, Death Knight, Paladin, Troll Axethrower (UNCERTAIN mapping, as used in tools/test_war.c).

### 12.3 Unit type -> sprite (summer, War Data)

Units: 00:45 01:46 02:47 03:48 04:49 05:50 06:51 07:52 08:53 09:54 0A:55 0B:58 0C:51 0D:52
0E:33 0F:34 10:47 11:48 12:53 13:54 1A:59(126 full) 1B:60(127 full) 1C:39 1D:40 1E:61 1F:62
20:41 21:42 26:43 27:44 28:38 29:63 2A:35 2B:36 2D:37 37:69 38:70 39:64.
Buildings: 3A:92 3B:93 3C:94 3D:95 3E:96 3F:97 40:98 41:99 42:104 43:105 44:90 45:91 46:88 47:89
48:108 49:109 4A:100 4B:101 4C:102 4D:103 4E:110 4F:111 50:84 51:85 52:106 53:107 54:112 55:113
56:114 57:115 58:86 59:87 5A:116 5B:117 5C:119 5D:118 5E:164 5F:165 60:80 61:81 62:82 63:83
64:166 65:167 66:181 67:168. Winter/wasteland overrides: ENTRY_MAP.md.

## 13. Reference tools in this repo

* `tools/w2data.py` - `list`, `extract`, `png [--palette ARCH:IDX] [--frame N] [--flip]`,
  `tileset`, `pud [--png out --archive WARDATA --scale 1|N|32] [--udta N]`, `strings`, `selftest`.
* `src/war.[ch]` - archive reader (`war_open/war_count/war_load/war_load_into/war_free`).
* `src/w2gfx_decode.[ch]` - sprite (RLE/raw, clip, flip, colour remap), image, cursor, font,
  megatile/tileset, PUD tile -> megatile, PUD section lookup.
* `tools/test_war.c` + `tools/build_test_war.sh` - host test; output must equal
  `python tools/w2data.py selftest hfs/Data`:

```
War Data       id 1000 entries 438 bytes  16383965 crc e0c58f3f sprites 209 frames  2894 pixcrc c35f07e8
War Resources  id 3000 entries 100 bytes   5111530 crc 130b7494 sprites   2 frames   100 pixcrc cf1de376
War Strings    id 4000 entries 105 bytes     53063 crc d08d291e
War Sounds     id 5000 entries 293 bytes   4294924 crc 85a9c6b0
War Music      id 2000 entries  49 bytes  23649782 crc 2ac2ca25
War Movies     id 6000 entries  15 bytes  50740064 crc d948ae23
```

Rendered verification images live only in `../_AmiW2-poza-repo/formats/` (outside the repo).
