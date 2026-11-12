# Warcraft II (Mac 1.23) - archive entry map

Generated from a scan of the real Mac files (types/sizes) cross-checked against the PC entry tables. Index numbering is identical to the PC original-game CD (wartool tables, original-CD rows). Type column: decoded format (see DATA_FORMAT.md); `[pal N]` = palette entry in the same archive (War Resources images use War Resources palettes). Era names: summer = forest. "placeholder" = no data (offset < 4 bytes before next entry).

Unit type ids in parentheses are PUD unit types (hex).


## War Data (= PC MAINDAT.WAR, id 1000, 438 entries)

| # | type | size | content |
|---:|---|---:|---|
| 0 | data  | 5694 | Default unit data (PUD UDTA payload without the leading u16 flag word, 5694 bytes) |
| 1 | data  | 780 | Default upgrade data (PUD UGRD payload without the leading u16 flag word, 780 bytes) |
| 2 | palette  | 768 | summer palette |
| 3 | data  | 11904 | summer megatiles |
| 4 | data  | 224128 | summer minitiles |
| 5 | tilemap  | 6636 | summer tile map |
| 6 | data  | 352 | summer: aux table (u16 2, u16 87, 87 x (u16 a,u16 b) pairs) - purpose unknown, identical to 14 |
| 7 | data  | 824 | summer: aux tile table (u16 records) - purpose unknown |
| 8 | data  | 784 | summer: aux tile table (u16 records) - purpose unknown |
| 9 | empty placeholder | 0 | placeholder |
| 10 | palette  | 768 | wasteland palette |
| 11 | data  | 11936 | wasteland megatiles |
| 12 | data  | 234176 | wasteland minitiles |
| 13 | tilemap  | 6636 | wasteland tile map |
| 14 | data  | 352 | wasteland: aux table (same as 6) |
| 15 | data  | 764 | wasteland: aux tile table |
| 16 | data  | 784 | wasteland: aux tile table |
| 17 | empty placeholder | 0 | placeholder |
| 18 | palette  | 768 | winter palette |
| 19 | data  | 12128 | winter megatiles |
| 20 | data  | 235264 | winter minitiles |
| 21 | tilemap  | 6636 | winter tile map |
| 22 | data  | 352 | winter: aux table (different from 6) |
| 23 | data  | 724 | winter: aux tile table |
| 24 | data  | 784 | winter: aux tile table |
| 25 | empty placeholder | 0 | placeholder |
| 26 | data  | 60118 | Windows BMP 400x300 4-bit (PC leftover, unused) |
| 27 | data  | 121078 | Windows BMP 400x300 8-bit (PC leftover, unused) |
| 28 | empty placeholder | 0 | placeholder |
| 29 | empty placeholder | 0 | placeholder |
| 30 | empty placeholder | 0 | placeholder |
| 31 | empty placeholder | 0 | placeholder |
| 32 | empty placeholder | 0 | placeholder |
| 33 | sprite 65 frames 56x56 | 43809 | human/units/Dwarven Demolition Squad (type 0x0e) [pal 2] |
| 34 | sprite 75 frames 56x56 | 41204 | orc/units/Goblin Sappers (type 0x0f) [pal 2] |
| 35 | sprite 65 frames 80x80 | 83954 | human/units/Gryphon Rider (type 0x2a) [pal 2] |
| 36 | sprite 50 frames 88x80 | 69649 | orc/units/Dragon (type 0x2b) [pal 2] |
| 37 | sprite 5 frames 32x32 | 2570 | orc/units/Eye of Kilrogg (type 0x2d) [pal 2] |
| 38 | sprite 20 frames 80x80 | 25514 | human/units/Gnomish Flying Machine (type 0x28) [pal 2] |
| 39 | sprite 15 frames 72x72 | 29306 | human/units/Human Transport (type 0x1c) [pal 2] |
| 40 | sprite 15 frames 72x72 | 16384 | orc/units/Orc Transport (type 0x1d) [pal 2] |
| 41 | sprite 15 frames 88x88 | 43907 | human/units/Battleship (type 0x20) [pal 2] |
| 42 | sprite 15 frames 88x88 | 37432 | orc/units/Ogre Juggernaught (type 0x21) [pal 2] |
| 43 | sprite 15 frames 72x72 | 40302 | human/units/Gnomish Submarine (type 0x26) [pal 2] |
| 44 | sprite 15 frames 72x72 | 42952 | orc/units/Giant Turtle (type 0x27) [pal 2] |
| 45 | sprite 60 frames 72x72 | 45939 | human/units/Footman (type 0x00) [pal 2] |
| 46 | sprite 60 frames 72x72 | 56590 | orc/units/Grunt (type 0x01) [pal 2] |
| 47 | sprite 65 frames 72x72 | 31026 | Peasant (type 0x02, also 0x10 attack peasant): normal frames [pal 2] |
| 48 | sprite 65 frames 72x72 | 35915 | Peon (type 0x03, also 0x11): normal frames [pal 2] |
| 49 | sprite 20 frames 64x64 | 48047 | human/units/Ballista (type 0x04) [pal 2] |
| 50 | sprite 20 frames 64x64 | 42320 | orc/units/Catapult (type 0x05) [pal 2] |
| 51 | sprite 70 frames 72x72 | 79825 | human/units/Knight (type 0x06) [pal 2] |
| 52 | sprite 70 frames 72x72 | 86558 | orc/units/Ogre (type 0x07) [pal 2] |
| 53 | sprite 50 frames 72x72 | 45378 | human/units/Elven Archer (type 0x08) [pal 2] |
| 54 | sprite 60 frames 72x72 | 53847 | orc/units/Troll Axethrower (type 0x09) [pal 2] |
| 55 | sprite 80 frames 72x72 | 47459 | human/units/Mage (type 0x0a) [pal 2] |
| 56 | sprite 65 frames 32x32 | 10443 | unknown small 32x32 unit sprite (blue robed, 65 frames) - unused? |
| 57 | sprite 60 frames 32x32 | 8377 | unknown small 32x32 unit sprite (60 frames) - unused? |
| 58 | sprite 65 frames 72x72 | 74276 | orc/units/Death Knight (type 0x0b) [pal 2] |
| 59 | sprite 15 frames 72x72 | 26933 | human/units/Human Oil Tanker (type 0x1a)_empty [pal 2] |
| 60 | sprite 15 frames 72x72 | 14117 | orc/units/Orc Oil Tanker (type 0x1b)_empty [pal 2] |
| 61 | sprite 15 frames 80x88 | 33606 | human/units/Elven Destroyer (type 0x1e) [pal 2] |
| 62 | sprite 15 frames 88x88 | 25478 | orc/units/Troll Destroyer (type 0x1f) [pal 2] |
| 63 | sprite 10 frames 72x72 | 12113 | orc/units/Goblin Zeppelin (type 0x29) [pal 2] |
| 64 | sprite 10 frames 32x32 | 3352 | summer/neutral/units/Critter (type 0x39) [pal 2] |
| 65 | sprite 10 frames 32x32 | 4370 | wasteland/neutral/units/Critter (type 0x39) [pal 10] |
| 66 | sprite 10 frames 32x32 | 4035 | winter/neutral/units/Critter (type 0x39) [pal 18] |
| 67 | empty placeholder | 0 | placeholder |
| 68 | empty placeholder | 0 | placeholder |
| 69 | sprite 70 frames 56x56 | 31237 | neutral/units/Skeleton (type 0x37) [pal 2] |
| 70 | sprite 75 frames 72x72 | 76888 | neutral/units/Daemon (type 0x38) [pal 2] |
| 71 | empty placeholder | 0 | placeholder |
| 72 | empty placeholder | 0 | placeholder |
| 73 | empty placeholder | 0 | placeholder |
| 74 | empty placeholder | 0 | placeholder |
| 75 | empty placeholder | 0 | placeholder |
| 76 | empty placeholder | 0 | placeholder |
| 77 | empty placeholder | 0 | placeholder |
| 78 | empty placeholder | 0 | placeholder |
| 79 | empty placeholder | 0 | placeholder |
| 80 | sprite 2 frames 64x64 | 5801 | summer/human/buildings/Human Guard Tower (type 0x60) [pal 2] |
| 81 | sprite 2 frames 64x64 | 4657 | summer/orc/buildings/Orc Guard Tower (type 0x61) [pal 2] |
| 82 | sprite 2 frames 64x64 | 5802 | summer/human/buildings/Human Cannon Tower (type 0x62) [pal 2] |
| 83 | sprite 2 frames 64x64 | 5327 | summer/orc/buildings/Orc Cannon Tower (type 0x63) [pal 2] |
| 84 | sprite 2 frames 96x96 | 11706 | summer/human/buildings/Mage Tower (type 0x50) [pal 2] |
| 85 | sprite 2 frames 96x96 | 9655 | summer/orc/buildings/Temple of the Damned (type 0x51) [pal 2] |
| 86 | sprite 2 frames 128x128 | 17664 | summer/human/buildings/Keep (type 0x58) [pal 2] |
| 87 | sprite 2 frames 128x128 | 18879 | summer/orc/buildings/Stronghold (type 0x59) [pal 2] |
| 88 | sprite 2 frames 96x96 | 9938 | summer/human/buildings/Gryphon Aviary (type 0x46) [pal 2] |
| 89 | sprite 2 frames 96x96 | 11142 | summer/orc/buildings/Dragon Roost (type 0x47) [pal 2] |
| 90 | sprite 2 frames 96x96 | 12377 | summer/human/buildings/Gnomish Inventor (type 0x44) [pal 2] |
| 91 | sprite 2 frames 96x96 | 10181 | summer/orc/buildings/Goblin Alchemist (type 0x45) [pal 2] |
| 92 | sprite 2 frames 64x64 | 5337 | summer/human/buildings/Farm (type 0x3a) [pal 2] |
| 93 | sprite 2 frames 64x64 | 6629 | summer/orc/buildings/Pig Farm (type 0x3b) [pal 2] |
| 94 | sprite 2 frames 96x96 | 10413 | summer/human/buildings/Human Barracks (type 0x3c) [pal 2] |
| 95 | sprite 2 frames 96x96 | 10634 | summer/orc/buildings/Orc Barracks (type 0x3d) [pal 2] |
| 96 | sprite 2 frames 96x96 | 12311 | summer/human/buildings/Church (type 0x3e) [pal 2] |
| 97 | sprite 2 frames 96x96 | 10701 | summer/orc/buildings/Altar of Storms (type 0x3f) [pal 2] |
| 98 | sprite 2 frames 64x64 | 4548 | summer/human/buildings/Scout Tower (type 0x40) [pal 2] |
| 99 | sprite 2 frames 64x64 | 4427 | summer/orc/buildings/Watch Tower (type 0x41) [pal 2] |
| 100 | sprite 2 frames 128x128 | 14127 | summer/human/buildings/Town Hall (type 0x4a) [pal 2] |
| 101 | sprite 2 frames 128x128 | 17848 | summer/orc/buildings/Great Hall (type 0x4b) [pal 2] |
| 102 | sprite 2 frames 96x96 | 10103 | summer/human/buildings/Elven Lumber Mill (type 0x4c) [pal 2] |
| 103 | sprite 2 frames 96x96 | 9581 | summer/orc/buildings/Troll Lumber Mill (type 0x4d) [pal 2] |
| 104 | sprite 2 frames 96x96 | 10546 | summer/human/buildings/Stables (type 0x42) [pal 2] |
| 105 | sprite 2 frames 96x96 | 10598 | summer/orc/buildings/Ogre Mound (type 0x43) [pal 2] |
| 106 | sprite 2 frames 96x96 | 12300 | summer/human/buildings/Human Blacksmith (type 0x52) [pal 2] |
| 107 | sprite 2 frames 96x96 | 10637 | summer/orc/buildings/Orc Blacksmith (type 0x53) [pal 2] |
| 108 | sprite 2 frames 96x96 | 11533 | summer/human/buildings/Human Shipyard (type 0x48) [pal 2] |
| 109 | sprite 2 frames 96x96 | 9385 | summer/orc/buildings/Orc Shipyard (type 0x49) [pal 2] |
| 110 | sprite 2 frames 96x96 | 11250 | summer/human/buildings/Human Foundry (type 0x4e) [pal 2] |
| 111 | sprite 2 frames 96x96 | 9747 | summer/orc/buildings/Orc Foundry (type 0x4f) [pal 2] |
| 112 | sprite 2 frames 96x96 | 12151 | summer/human/buildings/Human Refinery (type 0x54) [pal 2] |
| 113 | sprite 2 frames 96x96 | 11478 | summer/orc/buildings/Orc Refinery (type 0x55) [pal 2] |
| 114 | sprite 3 frames 96x96 | 14607 | summer/human/buildings/Human Oil Platform (type 0x56) [pal 2] |
| 115 | sprite 3 frames 96x96 | 14306 | summer/orc/buildings/Orc Oil Platform (type 0x57) [pal 2] |
| 116 | sprite 2 frames 128x128 | 20433 | summer/human/buildings/Castle (type 0x5a) [pal 2] |
| 117 | sprite 2 frames 128x128 | 21548 | summer/orc/buildings/Fortress (type 0x5b) [pal 2] |
| 118 | sprite 1 frames 96x96 | 4115 | summer/neutral/buildings/Oil Patch (type 0x5d) [pal 2] |
| 119 | sprite 2 frames 96x96 | 12438 | summer/neutral/buildings/Gold Mine (type 0x5c) [pal 2] |
| 120 | sprite 35 frames 72x72 | 10425 | neutral/units/corpses [pal 2] |
| 121 | sprite 4 frames 64x64 | 8020 | summer destroyed site (large, 4 frames) [pal 2] |
| 122 | sprite 65 frames 72x72 | 16208 | Peasant carrying wood: frames 0-24 (walk) only valid, use 47 for frames 25+ [pal 2] |
| 123 | sprite 65 frames 72x72 | 17784 | Peon carrying wood: frames 0-24 valid, frames 25+ from 48 [pal 2] |
| 124 | sprite 65 frames 72x72 | 15599 | Peasant carrying gold: frames 0-24 valid, frames 25+ from 47 [pal 2] |
| 125 | sprite 65 frames 72x72 | 16545 | Peon carrying gold: frames 0-24 valid, frames 25+ from 48 [pal 2] |
| 126 | sprite 15 frames 72x72 | 27359 | human/units/Human Oil Tanker (type 0x1a)_full [pal 2] |
| 127 | sprite 15 frames 72x72 | 16049 | orc/units/Orc Oil Tanker (type 0x1b)_full [pal 2] |
| 128 | sprite 2 frames 128x128 | 18703 | winter/human/buildings/Keep (type 0x58) [pal 18] |
| 129 | sprite 2 frames 128x128 | 19516 | winter/orc/buildings/Stronghold (type 0x59) [pal 18] |
| 130 | sprite 2 frames 96x96 | 11449 | winter/human/buildings/Gryphon Aviary (type 0x46) [pal 18] |
| 131 | sprite 2 frames 96x96 | 11545 | winter/orc/buildings/Dragon Roost (type 0x47) [pal 18] |
| 132 | sprite 2 frames 96x96 | 13016 | winter/human/buildings/Gnomish Inventor (type 0x44) [pal 18] |
| 133 | sprite 2 frames 96x96 | 10763 | winter/orc/buildings/Goblin Alchemist (type 0x45) [pal 18] |
| 134 | sprite 2 frames 64x64 | 6122 | winter/human/buildings/Farm (type 0x3a) [pal 18] |
| 135 | sprite 2 frames 64x64 | 7035 | winter/orc/buildings/Pig Farm (type 0x3b) [pal 18] |
| 136 | sprite 2 frames 96x96 | 11402 | winter/human/buildings/Human Barracks (type 0x3c) [pal 18] |
| 137 | sprite 2 frames 96x96 | 12355 | winter/orc/buildings/Orc Barracks (type 0x3d) [pal 18] |
| 138 | sprite 2 frames 96x96 | 12970 | winter/human/buildings/Church (type 0x3e) [pal 18] |
| 139 | sprite 2 frames 96x96 | 13099 | winter/orc/buildings/Altar of Storms (type 0x3f) [pal 18] |
| 140 | sprite 2 frames 64x64 | 5176 | winter/human/buildings/Scout Tower (type 0x40) [pal 18] |
| 141 | sprite 2 frames 64x64 | 5519 | winter/orc/buildings/Watch Tower (type 0x41) [pal 18] |
| 142 | sprite 2 frames 128x128 | 15245 | winter/human/buildings/Town Hall (type 0x4a) [pal 18] |
| 143 | sprite 2 frames 128x128 | 18723 | winter/orc/buildings/Great Hall (type 0x4b) [pal 18] |
| 144 | sprite 2 frames 96x96 | 11411 | winter/human/buildings/Elven Lumber Mill (type 0x4c) [pal 18] |
| 145 | sprite 2 frames 96x96 | 11813 | winter/orc/buildings/Troll Lumber Mill (type 0x4d) [pal 18] |
| 146 | sprite 2 frames 96x96 | 12184 | winter/human/buildings/Stables (type 0x42) [pal 18] |
| 147 | sprite 2 frames 96x96 | 10815 | winter/orc/buildings/Ogre Mound (type 0x43) [pal 18] |
| 148 | sprite 2 frames 96x96 | 13400 | winter/human/buildings/Human Blacksmith (type 0x52) [pal 18] |
| 149 | sprite 2 frames 96x96 | 11015 | winter/orc/buildings/Orc Blacksmith (type 0x53) [pal 18] |
| 150 | sprite 2 frames 96x96 | 11742 | winter/human/buildings/Human Shipyard (type 0x48) [pal 18] |
| 151 | sprite 2 frames 96x96 | 9888 | winter/orc/buildings/Orc Shipyard (type 0x49) [pal 18] |
| 152 | sprite 2 frames 96x96 | 11466 | winter/human/buildings/Human Foundry (type 0x4e) [pal 18] |
| 153 | sprite 2 frames 96x96 | 9831 | winter/orc/buildings/Orc Foundry (type 0x4f) [pal 18] |
| 154 | sprite 2 frames 96x96 | 12184 | winter/human/buildings/Human Refinery (type 0x54) [pal 18] |
| 155 | sprite 2 frames 96x96 | 11697 | winter/orc/buildings/Orc Refinery (type 0x55) [pal 18] |
| 156 | sprite 3 frames 96x96 | 14672 | winter/human/buildings/Human Oil Platform (type 0x56) [pal 18] |
| 157 | sprite 3 frames 96x96 | 14346 | winter/orc/buildings/Orc Oil Platform (type 0x57) [pal 18] |
| 158 | sprite 2 frames 128x128 | 24688 | winter/human/buildings/Castle (type 0x5a) [pal 18] |
| 159 | sprite 2 frames 128x128 | 22973 | winter/orc/buildings/Fortress (type 0x5b) [pal 18] |
| 160 | sprite 2 frames 96x96 | 12605 | winter/human/buildings/Mage Tower (type 0x50) [pal 18] |
| 161 | sprite 2 frames 96x96 | 10928 | winter/orc/buildings/Temple of the Damned (type 0x51) [pal 18] |
| 162 | sprite 2 frames 96x96 | 14014 | winter/neutral/buildings/Gold Mine (type 0x5c) [pal 18] |
| 163 | sprite 4 frames 64x64 | 9215 | winter destroyed site (large) [pal 18] |
| 164 | sprite 1 frames 32x32 | 386 | human/x_startpoint [pal 2] |
| 165 | sprite 1 frames 32x32 | 338 | orc/o_startpoint [pal 2] |
| 166 | sprite 1 frames 64x64 | 1001 | neutral/buildings/Circle of Power (type 0x64) [pal 2] |
| 167 | sprite 1 frames 128x128 | 10952 | summer/neutral/buildings/Dark Portal (type 0x65) [pal 2] |
| 168 | sprite 2 frames 32x32 | 1876 | summer/neutral/buildings/Wall (type 0x67) [pal 2] |
| 169 | sprite 2 frames 64x64 | 6237 | winter/human/buildings/Human Guard Tower (type 0x60) [pal 18] |
| 170 | sprite 2 frames 64x64 | 6432 | winter/orc/buildings/Orc Guard Tower (type 0x61) [pal 18] |
| 171 | sprite 2 frames 64x64 | 6224 | winter/human/buildings/Human Cannon Tower (type 0x62) [pal 18] |
| 172 | sprite 2 frames 64x64 | 6451 | winter/orc/buildings/Orc Cannon Tower (type 0x63) [pal 18] |
| 173 | sprite 2 frames 64x64 | 4779 | wasteland/human/buildings/Farm (type 0x3a) [pal 10] |
| 174 | sprite 2 frames 64x64 | 6629 | wasteland/orc/buildings/Pig Farm (type 0x3b) [pal 10] |
| 175 | sprite 2 frames 96x96 | 9771 | wasteland/human/buildings/Elven Lumber Mill (type 0x4c) [pal 10] |
| 176 | sprite 2 frames 96x96 | 9581 | wasteland/orc/buildings/Troll Lumber Mill (type 0x4d) [pal 10] |
| 177 | sprite 3 frames 96x96 | 14456 | wasteland/human/buildings/Human Oil Platform (type 0x56) [pal 10] |
| 178 | sprite 3 frames 96x96 | 13568 | wasteland/orc/buildings/Orc Oil Platform (type 0x57) [pal 10] |
| 179 | sprite 2 frames 96x96 | 12307 | wasteland/neutral/buildings/Gold Mine (type 0x5c) [pal 10] |
| 180 | sprite 1 frames 96x96 | 3607 | wasteland/neutral/buildings/Oil Patch (type 0x5d) [pal 10] |
| 181 | sprite 1 frames 64x64 | 2908 | neutral/buildings/Runestone (type 0x66) [pal 2] |
| 182 | sprite 15 frames 72x72 | 40302 | wasteland/human/units/Gnomish Submarine (type 0x26) [pal 10] |
| 183 | sprite 15 frames 72x72 | 42911 | wasteland/orc/units/Giant Turtle (type 0x27) [pal 10] |
| 184 | sprite 1 frames 128x128 | 11262 | winter/neutral/buildings/Dark Portal (type 0x65) [pal 18] |
| 185 | sprite 1 frames 128x128 | 10869 | wasteland/neutral/buildings/Dark Portal (type 0x65) [pal 10] |
| 186 | sprite 1 frames 64x64 | 3091 | winter/neutral/buildings/Runestone (type 0x66) [pal 18] |
| 187 | gfu 4 frames 16x16 | 822 | ui/gold,wood,oil,mana [pal 2] |
| 188 | sprite 2 frames 32x32 | 1500 | wasteland small destroyed site [pal 10] |
| 189 | sprite 2 frames 32x32 | 1476 | summer small destroyed site [pal 2] |
| 190 | sprite 2 frames 32x32 | 1517 | winter small destroyed site [pal 18] |
| 191 | sprite 4 frames 64x64 | 8454 | wasteland destroyed site (large) [pal 10] |
| 192 | pud  | 14102 | campaigns/human/level01h |
| 193 | pud  | 14102 | campaigns/orc/level01o |
| 194 | pud  | 35862 | campaigns/human/level02h |
| 195 | pud  | 35814 | campaigns/orc/level02o |
| 196 | pud  | 35798 | campaigns/human/level03h |
| 197 | pud  | 35838 | campaigns/orc/level03o |
| 198 | pud  | 71894 | campaigns/human/level04h |
| 199 | pud  | 71822 | campaigns/orc/level04o |
| 200 | pud  | 122422 | campaigns/human/level05h |
| 201 | pud  | 122238 | campaigns/orc/level05o |
| 202 | pud  | 71974 | campaigns/human/level06h |
| 203 | pud  | 72158 | campaigns/orc/level06o |
| 204 | pud  | 72430 | campaigns/human/level07h |
| 205 | pud  | 122478 | campaigns/orc/level07o |
| 206 | pud  | 72350 | campaigns/human/level08h |
| 207 | pud  | 122654 | campaigns/orc/level08o |
| 208 | pud  | 72270 | campaigns/human/level09h |
| 209 | pud  | 72350 | campaigns/orc/level09o |
| 210 | pud  | 122406 | campaigns/human/level10h |
| 211 | pud  | 72318 | campaigns/orc/level10o |
| 212 | pud  | 72694 | campaigns/human/level11h |
| 213 | pud  | 122846 | campaigns/orc/level11o |
| 214 | pud  | 123006 | campaigns/human/level12h |
| 215 | pud  | 122998 | campaigns/orc/level12o |
| 216 | pud  | 123182 | campaigns/human/level13h |
| 217 | pud  | 122678 | campaigns/orc/level13o |
| 218 | pud  | 123070 | campaigns/human/level14h |
| 219 | pud  | 122854 | campaigns/orc/level14o |
| 220 | pud  | 71526 | maps/skirmish/(6)gold-separates-east-west |
| 221 | pud  | 71526 | maps/skirmish/(4)oil-is-the-key |
| 222 | pud  | 121902 | maps/skirmish/(8)bridge-to-bridge-combat |
| 223 | pud  | 121758 | maps/skirmish/(8)plains-of-snow |
| 224 | pud  | 121742 | maps/skirmish/(3)three-ways-to-cross |
| 225 | pud  | 121822 | maps/skirmish/(8)a-continent-to-explore |
| 226 | pud  | 121798 | maps/skirmish/(8)high-seas-combat |
| 227 | pud  | 121918 | maps/skirmish/(4)islands-in-the-stream |
| 228 | pud  | 72158 | maps/skirmish/(4)opposing-city-states |
| 229 | pud  | 71462 | maps/skirmish/(4)death-in-the-middle |
| 230 | pud  | 71702 | maps/skirmish/(5)the-spiral |
| 231 | pud  | 121654 | maps/skirmish/(2)mysterious-dragon-isle |
| 232 | pud  | 121734 | maps/skirmish/(4)the-four-corners |
| 233 | pud  | 121734 | maps/skirmish/(6)no-way-out-of-this-maze |
| 234 | pud  | 121830 | maps/skirmish/(8)fierce-ocean-combat |
| 235 | pud  | 35590 | maps/skirmish/(2)cross-the-streams |
| 236 | pud  | 71654 | maps/skirmish/(5)how-much-can-you-mine |
| 237 | pud  | 35590 | maps/skirmish/(2)x-marks-the-spot |
| 238 | pud  | 35574 | maps/skirmish/(2)one-way-in-one-way-out |
| 239 | pud  | 14086 | maps/skirmish/(4)nowhere-to-run |
| 240 | pud  | 71414 | maps/skirmish/(2)2-players |
| 241 | pud  | 121846 | maps/skirmish/(7)and-the-rivers-ran-as-blood |
| 242 | pud  | 71886 | maps/skirmish/(3)critter-attack |
| 243 | pud  | 121734 | maps/skirmish/(6)skull-isle |
| 244 | pud  | 121838 | maps/skirmish/(8)cross-over |
| 245 | pud  | 121894 | maps/skirmish/(8)garden-of-war |
| 246 | pud  | 71646 | maps/skirmish/(4)mine-the-center |
| 247 | pud  | 71462 | maps/skirmish/(3)river-fork |
| 248 | pud  | 14334 | maps/demo/demo01 |
| 249 | pud  | 14158 | maps/demo/demo02 |
| 250 | pud  | 14270 | maps/demo/demo03 |
| 251 | pud  | 14206 | maps/demo/demo04 |
| 252 | sprite 2 frames 64x64 | 3508 | neutral/buildings/land_construction_site [pal 2] |
| 253 | sprite 2 frames 96x96 | 3762 | human/buildings/shipyard_construction_site [pal 2] |
| 254 | sprite 2 frames 96x96 | 2365 | orc/buildings/shipyard_construction_site [pal 2] |
| 255 | sprite 2 frames 96x96 | 9119 | summer human oil well (platform) construction site [pal 2] |
| 256 | sprite 2 frames 96x96 | 9112 | summer orc oil well (platform) construction site [pal 2] |
| 257 | sprite 2 frames 96x96 | 8784 | human/buildings/refinery_construction_site [pal 2] |
| 258 | sprite 2 frames 96x96 | 4048 | orc/buildings/refinery_construction_site [pal 2] |
| 259 | sprite 2 frames 96x96 | 4528 | human/buildings/foundry_construction_site [pal 2] |
| 260 | sprite 2 frames 96x96 | 3745 | orc/buildings/foundry_construction_site [pal 2] |
| 261 | sprite 2 frames 32x32 | 1836 | summer wall construction site [pal 2] |
| 262 | sprite 2 frames 64x64 | 4412 | winter land construction site [pal 18] |
| 263 | sprite 3 frames 96x96 | 3891 | winter human shipyard construction site [pal 18] |
| 264 | sprite 2 frames 96x96 | 2514 | winter orc shipyard construction site [pal 18] |
| 265 | sprite 2 frames 96x96 | 9172 | winter human oil well construction site [pal 18] |
| 266 | sprite 2 frames 96x96 | 9132 | winter orc oil well construction site [pal 18] |
| 267 | sprite 2 frames 96x96 | 8885 | winter human refinery construction site [pal 18] |
| 268 | sprite 2 frames 96x96 | 4066 | winter orc refinery construction site [pal 18] |
| 269 | sprite 2 frames 96x96 | 4528 | winter human foundry construction site [pal 18] |
| 270 | sprite 2 frames 96x96 | 3745 | winter orc foundry construction site [pal 18] |
| 271 | sprite 2 frames 96x96 | 8679 | wasteland human oil well construction site [pal 10] |
| 272 | sprite 2 frames 96x96 | 8468 | wasteland orc oil well construction site [pal 10] |
| 273 | sprite 2 frames 32x32 | 1833 | winter/neutral/buildings/wall [pal 18] |
| 274 | sprite 2 frames 32x32 | 1833 | wasteland/neutral/buildings/wall [pal 10] |
| 275 | sprite 2 frames 32x32 | 1788 | winter wall construction site [pal 18] |
| 276 | sprite 2 frames 32x32 | 1821 | wasteland wall construction site [pal 10] |
| 277 | data  | 9237 | AI control programs (computer player scripts) - format not decoded |
| 278 | data  | 4998 | unit animation/movement control programs - format not decoded |
| 279 | font chars 32..239 max 52x50 | 64611 | large_episode_titles |
| 280 | font chars 32..239 max 32x35 | 27451 | small_episode_titles |
| 281 | font chars 32..239 max 17x17 | 11995 | large |
| 282 | font chars 32..239 max 13x14 | 8958 | game |
| 283 | font chars 32..239 max 7x6 | 4437 | small |
| 284 | data  | 390 | x86 machine code (PC driver leftover, unused) |
| 285 | empty placeholder | 0 | placeholder |
| 286 | empty placeholder | 0 | placeholder |
| 287 | image 448x16 | 7172 | ui/human/resource [pal 2] |
| 288 | image 448x16 | 7172 | ui/orc/resource [pal 2] |
| 289 | image 16x480 | 7684 | ui/human/filler-right [pal 2] |
| 290 | image 16x480 | 7684 | ui/orc/filler-right [pal 2] |
| 291 | image 448x16 | 7172 | ui/human/statusline [pal 2] |
| 292 | image 448x16 | 7172 | ui/orc/statusline [pal 2] |
| 293 | image 176x24 | 4228 | ui/human/menubutton [pal 2] |
| 294 | image 176x24 | 4228 | ui/orc/menubutton [pal 2] |
| 295 | image 176x136 | 23940 | ui/human/minimap [pal 2] |
| 296 | image 176x136 | 23940 | ui/orc/minimap [pal 2] |
| 297 | image 176x144 | 25348 | ui/human/buttonpanel [pal 2] |
| 298 | image 176x144 | 25348 | ui/orc/buttonpanel [pal 2] |
| 299 | image 640x480 | 307204 | ui/title [pal 300] |
| 300 | palette  | 768 | palette for 299 (title screen) |
| 301 | cursor 28x32 hot 3,2 | 904 | human/cursors/human_gauntlet [pal 2] |
| 302 | cursor 26x32 hot 3,2 | 840 | orc/cursors/orcish_claw [pal 2] |
| 303 | cursor 31x32 hot 3,2 | 1000 | human/cursors/human_dont_click_here [pal 2] |
| 304 | cursor 28x32 hot 3,2 | 904 | orc/cursors/orcish_dont_click_here [pal 2] |
| 305 | cursor 32x32 hot 15,15 | 1032 | human/cursors/yellow_eagle [pal 2] |
| 306 | cursor 32x32 hot 15,15 | 1032 | orc/cursors/yellow_crosshairs [pal 2] |
| 307 | cursor 32x32 hot 15,15 | 1032 | human/cursors/red_eagle [pal 2] |
| 308 | cursor 32x32 hot 15,15 | 1032 | orc/cursors/red_crosshairs [pal 2] |
| 309 | cursor 32x32 hot 15,15 | 1032 | human/cursors/green_eagle [pal 2] |
| 310 | cursor 32x32 hot 15,15 | 1032 | orc/cursors/green_crosshairs [pal 2] |
| 311 | cursor 34x35 hot 11,11 | 1198 | cursors/magnifying_glass [pal 2] |
| 312 | cursor 18x18 hot 8,8 | 332 | cursors/small_green_cross [pal 2] |
| 313 | cursor 23x35 hot 11,18 | 813 | cursors/hourglass [pal 2] |
| 314 | cursor 15x21 hot 0,0 | 323 | cursors/credits_arrow [pal 27] |
| 315 | cursor 32x24 hot 12,2 | 776 | cursors/arrow_N [pal 2] |
| 316 | cursor 32x24 hot 20,2 | 776 | cursors/arrow_NE [pal 2] |
| 317 | cursor 32x24 hot 22,10 | 776 | cursors/arrow_E [pal 2] |
| 318 | cursor 32x24 hot 20,18 | 776 | cursors/arrow_SE [pal 2] |
| 319 | cursor 32x24 hot 12,22 | 776 | cursors/arrow_S [pal 2] |
| 320 | cursor 32x24 hot 2,18 | 776 | cursors/arrow_SW [pal 2] |
| 321 | cursor 32x24 hot 4,10 | 776 | cursors/arrow_W [pal 2] |
| 322 | cursor 32x24 hot 2,2 | 776 | cursors/arrow_NW [pal 2] |
| 323 | gfu 5 frames 16x16 | 1326 | ui/bloodlust,haste,slow,invisible,shield [pal 2] |
| 324 | sprite 30 frames 32x32 | 3460 | missiles/lightning [pal 2] |
| 325 | sprite 15 frames 32x32 | 1638 | missiles/gryphon_hammer [pal 2] |
| 326 | sprite 5 frames 32x32 | 2339 | missiles/dragon_breath [pal 2] |
| 327 | sprite 5 frames 32x32 | 2339 | missiles/fireball [pal 2] |
| 328 | sprite 6 frames 32x48 | 3659 | missiles/flame_shield [pal 2] |
| 329 | sprite 4 frames 32x32 | 1029 | missiles/blizzard [pal 2] |
| 330 | sprite 8 frames 32x32 | 2069 | missiles/death_and_decay [pal 2] |
| 331 | sprite 20 frames 16x16 | 960 | missiles/big_cannon [pal 2] |
| 332 | sprite 6 frames 48x48 | 2478 | missiles/exorcism [pal 2] |
| 333 | sprite 6 frames 48x48 | 2637 | missiles/heal_effect [pal 2] |
| 334 | sprite 30 frames 32x32 | 3223 | missiles/touch_of_death [pal 2] |
| 335 | sprite 4 frames 16x16 | 606 | missiles/rune [pal 2] |
| 336 | sprite 4 frames 56x56 | 6601 | missiles/tornado [pal 2] |
| 337 | sprite 15 frames 32x32 | 4166 | missiles/catapult_rock [pal 2] |
| 338 | sprite 5 frames 64x64 | 2530 | missiles/ballista_bolt [pal 2] |
| 339 | sprite 5 frames 40x40 | 821 | missiles/arrow [pal 2] |
| 340 | sprite 15 frames 32x32 | 703 | missiles/axe [pal 2] |
| 341 | sprite 5 frames 40x40 | 2945 | missiles/submarine_missile [pal 2] |
| 342 | sprite 5 frames 40x40 | 2945 | missiles/turtle_missile [pal 2] |
| 343 | sprite 6 frames 32x48 | 3659 | missiles/small_fire [pal 2] |
| 344 | sprite 10 frames 48x48 | 13462 | missiles/big_fire [pal 2] |
| 345 | sprite 6 frames 48x48 | 5493 | missiles/ballista-catapult_impact [pal 2] |
| 346 | sprite 6 frames 32x32 | 968 | missiles/normal_spell [pal 2] |
| 347 | sprite 16 frames 64x64 | 24208 | missiles/explosion [pal 2] |
| 348 | sprite 15 frames 32x32 | 541 | missiles/cannon [pal 2] |
| 349 | sprite 4 frames 32x32 | 1183 | missiles/cannon_explosion [pal 2] |
| 350 | sprite 4 frames 32x32 | 1869 | missiles/cannon-tower_explosion [pal 2] |
| 351 | sprite 15 frames 32x32 | 986 | missiles/daemon_fire [pal 2] |
| 352 | sprite 4 frames 32x32 | 1873 | missiles/green_cross [pal 2] |
| 353 | sprite 3 frames 32x32 | 1283 | missiles/unit_shadow [pal 2] |
| 354 | gfu 4 frames 176x176 | 92966 | ui/human/infopanel [pal 2] |
| 355 | gfu 4 frames 176x176 | 92966 | ui/orc/infopanel [pal 2] |
| 356 | sprite 186 frames 46x38 | 260775 | summer/icons [pal 2] |
| 357 | sprite 186 frames 46x38 | 259419 | winter/icons [pal 18] |
| 358 | sprite 186 frames 46x38 | 259602 | wasteland/icons [pal 10] |
| 359 | image 640x480 | 307204 | ui/human/victory [pal 363] |
| 360 | image 640x480 | 307204 | ui/orc/victory [pal 364] |
| 361 | image 640x480 | 307204 | ui/human/defeat [pal 365] |
| 362 | image 640x480 | 307204 | ui/orc/defeat [pal 366] |
| 363 | palette  | 768 | palette for 359 |
| 364 | palette  | 768 | palette for 360 |
| 365 | palette  | 768 | palette for 361 |
| 366 | palette  | 768 | palette for 362 |
| 367 | palette  | 768 | palette for human intro screens 369,375-378 |
| 368 | palette  | 768 | palette for orc intro screens 370-374 |
| 369 | image 640x480 | 307204 | campaign human/interface/introscreen1 [pal 367] |
| 370 | image 640x480 | 307204 | campaign orc/interface/introscreen1 [pal 368] |
| 371 | image 640x480 | 307204 | campaign orc/interface/introscreen2 [pal 368] |
| 372 | image 640x480 | 307204 | campaign orc/interface/introscreen3 [pal 368] |
| 373 | image 640x480 | 307204 | campaign orc/interface/introscreen4 [pal 368] |
| 374 | image 640x480 | 307204 | campaign orc/interface/introscreen5 [pal 368] |
| 375 | image 640x480 | 307204 | campaign human/interface/introscreen2 [pal 367] |
| 376 | image 640x480 | 307204 | campaign human/interface/introscreen3 [pal 367] |
| 377 | image 640x480 | 307204 | campaign human/interface/introscreen4 [pal 367] |
| 378 | image 640x480 | 307204 | campaign human/interface/introscreen5 [pal 367] |
| 379 | iff XDIR | 21768 | XMIDI song "Human Battle 1" - variant A (driver set unknown) |
| 380 | iff XDIR | 14718 | XMIDI song "Human Battle 2" - variant A (driver set unknown) |
| 381 | iff XDIR | 18932 | XMIDI song "Human Battle 3" - variant A (driver set unknown) |
| 382 | iff XDIR | 22702 | XMIDI song "Human Battle 4" - variant A (driver set unknown) |
| 383 | iff XDIR | 16374 | XMIDI song "Orc Battle 1" - variant A (driver set unknown) |
| 384 | iff XDIR | 17122 | XMIDI song "Orc Battle 2" - variant A (driver set unknown) |
| 385 | iff XDIR | 24780 | XMIDI song "Orc Battle 3" - variant A (driver set unknown) |
| 386 | iff XDIR | 22536 | XMIDI song "Orc Battle 4" - variant A (driver set unknown) |
| 387 | iff XDIR | 3434 | XMIDI song "Human Defeat" - variant A (driver set unknown) |
| 388 | iff XDIR | 2612 | XMIDI song "Orc Defeat" - variant A (driver set unknown) |
| 389 | iff XDIR | 3070 | XMIDI song "Human Victory" - variant A (driver set unknown) |
| 390 | iff XDIR | 6250 | XMIDI song "Orc Victory" - variant A (driver set unknown) |
| 391 | iff XDIR | 18104 | XMIDI song "Orc Battle 5" - variant A (driver set unknown) |
| 392 | iff XDIR | 16030 | XMIDI song "I'm a Medieval Man" - variant A (driver set unknown) |
| 393 | iff XDIR | 18208 | XMIDI song "Human Battle 5" - variant A (driver set unknown) |
| 394 | iff XDIR | 6830 | XMIDI song "Human Briefing" - variant A (driver set unknown) |
| 395 | iff XDIR | 5308 | XMIDI song "Orc Briefing / Main Menu" - variant A (driver set unknown) |
| 396 | iff XDIR | 18832 | XMIDI song "Human Battle 1" - variant B |
| 397 | iff XDIR | 12022 | XMIDI song "Human Battle 2" - variant B |
| 398 | iff XDIR | 13286 | XMIDI song "Human Battle 3" - variant B |
| 399 | iff XDIR | 18802 | XMIDI song "Human Battle 4" - variant B |
| 400 | iff XDIR | 15130 | XMIDI song "Orc Battle 1" - variant B |
| 401 | iff XDIR | 13752 | XMIDI song "Orc Battle 2" - variant B |
| 402 | iff XDIR | 21118 | XMIDI song "Orc Battle 3" - variant B |
| 403 | iff XDIR | 17494 | XMIDI song "Orc Battle 4" - variant B |
| 404 | iff XDIR | 2696 | XMIDI song "Human Defeat" - variant B |
| 405 | iff XDIR | 2288 | XMIDI song "Orc Defeat" - variant B |
| 406 | iff XDIR | 2310 | XMIDI song "Human Victory" - variant B |
| 407 | iff XDIR | 3482 | XMIDI song "Orc Victory" - variant B |
| 408 | iff XDIR | 12470 | XMIDI song "Orc Battle 5" - variant B |
| 409 | iff XDIR | 13952 | XMIDI song "I'm a Medieval Man" - variant B |
| 410 | iff XDIR | 15838 | XMIDI song "Human Battle 5" - variant B |
| 411 | iff XDIR | 4842 | XMIDI song "Human Briefing" - variant B |
| 412 | iff XDIR | 4128 | XMIDI song "Orc Briefing / Main Menu" - variant B |
| 413 | iff XDIR | 25658 | XMIDI song "Human Battle 1" - variant C (the set wargus converts) |
| 414 | iff XDIR | 17652 | XMIDI song "Human Battle 2" - variant C (the set wargus converts) |
| 415 | iff XDIR | 21926 | XMIDI song "Human Battle 3" - variant C (the set wargus converts) |
| 416 | iff XDIR | 27612 | XMIDI song "Human Battle 4" - variant C (the set wargus converts) |
| 417 | iff XDIR | 19276 | XMIDI song "Orc Battle 1" - variant C (the set wargus converts) |
| 418 | iff XDIR | 19524 | XMIDI song "Orc Battle 2" - variant C (the set wargus converts) |
| 419 | iff XDIR | 27534 | XMIDI song "Orc Battle 3" - variant C (the set wargus converts) |
| 420 | iff XDIR | 23224 | XMIDI song "Orc Battle 4" - variant C (the set wargus converts) |
| 421 | iff XDIR | 5116 | XMIDI song "Human Defeat" - variant C (the set wargus converts) |
| 422 | iff XDIR | 3124 | XMIDI song "Orc Defeat" - variant C (the set wargus converts) |
| 423 | iff XDIR | 3594 | XMIDI song "Human Victory" - variant C (the set wargus converts) |
| 424 | iff XDIR | 6500 | XMIDI song "Orc Victory" - variant C (the set wargus converts) |
| 425 | iff XDIR | 16952 | XMIDI song "Orc Battle 5" - variant C (the set wargus converts) |
| 426 | iff XDIR | 16466 | XMIDI song "I'm a Medieval Man" - variant C (the set wargus converts) |
| 427 | iff XDIR | 22320 | XMIDI song "Human Battle 5" - variant C (the set wargus converts) |
| 428 | iff XDIR | 8578 | XMIDI song "Human Briefing" - variant C (the set wargus converts) |
| 429 | iff XDIR | 6180 | XMIDI song "Orc Briefing / Main Menu" - variant C (the set wargus converts) |
| 430 | smacker  | 793880 | Blizzard logo video (Smacker SMK2 320x160, 140 frames) |
| 431 | empty placeholder | 0 | placeholder |
| 432 | wav  | 1524 | ui/click |
| 433 | wav  | 110842 | human/act |
| 434 | wav  | 111076 | orc/act |
| 435 | wav  | 1444 | ui/highclick |
| 436 | wav  | 7740 | ui/statsthump |
| 437 | empty placeholder | 0 | placeholder |

## War Resources (= PC REZDAT.WAR, id 3000, 100 entries)

| # | type | size | content |
|---:|---|---:|---|
| 0 | gfu 50 frames 300x164 | 88845 | human menu widgets (gfu, 50 frames: buttons/radio/checkbox/arrows/sliders/pulldown), frames 0-2 use pal 27, rest pal 14 |
| 1 | gfu 50 frames 300x164 | 88845 | orc menu widgets (same layout as 0) |
| 2 | image 18x18 | 328 | ui/cd-icon [pal 14] |
| 3 | image 256x288 | 73732 | ui/human/panel_1 [pal 14] |
| 4 | image 256x288 | 73732 | ui/orc/panel_1 [pal 14] |
| 5 | image 288x256 | 73732 | ui/human/panel_2 [pal 14] |
| 6 | image 288x256 | 73732 | ui/orc/panel_2 [pal 14] |
| 7 | image 384x256 | 98308 | ui/human/panel_3 [pal 14] |
| 8 | image 384x256 | 98308 | ui/orc/panel_3 [pal 14] |
| 9 | image 288x128 | 36868 | ui/human/panel_4 [pal 14] |
| 10 | image 288x128 | 36868 | ui/orc/panel_4 [pal 14] |
| 11 | image 352x352 | 123908 | ui/human/panel_5 [pal 14] |
| 12 | image 352x352 | 123908 | ui/orc/panel_5 [pal 14] |
| 13 | image 640x480 | 307204 | ui/Menu_background_with_title [pal 14] |
| 14 | palette  | 768 | palette: menus / panels (3-13) |
| 15 | image 640x480 | 307204 | ui/Menu_background_without_title [pal 16] |
| 16 | palette  | 768 | palette for 15 |
| 17 | palette  | 768 | palette: act title screens 19-26 |
| 18 | palette  | 768 | palette (same size as 17; usage unknown) |
| 19 | image 640x480 | 307204 | campaign act title screen: human Act I - Shores of Lordaeron [pal 17] |
| 20 | image 640x480 | 307204 | campaign act title screen: orc Act I - Seas of Blood [pal 17] |
| 21 | image 640x480 | 307204 | campaign act title screen: human Act II - Khaz Modan [pal 17] |
| 22 | image 640x480 | 307204 | campaign act title screen: orc Act II - Khaz Modan [pal 17] |
| 23 | image 640x480 | 307204 | campaign act title screen: human Act III - The Northlands [pal 17] |
| 24 | image 640x480 | 307204 | campaign act title screen: orc Act III - Quel'Thalas [pal 17] |
| 25 | image 640x480 | 307204 | campaign act title screen: human Act IV - Return to Azeroth [pal 17] |
| 26 | image 640x480 | 307204 | campaign act title screen: orc Act IV - Tides of Darkness [pal 17] |
| 27 | palette  | 768 | palette: credits/The End (28, 29), credits arrow cursor |
| 28 | image 640x480 | 307204 | ui/Credits_background [pal 27] |
| 29 | image 640x480 | 307204 | ui/human/The_End [pal 27] |
| 30 | image 640x480 | 307204 | ui/orc/Smashing_of_Lordaeron_scroll [pal 32] |
| 31 | palette  | 768 | palette (unknown user) |
| 32 | palette  | 768 | palette for 30 |
| 33 | data  | 576 | menu/dialog layout (N x 72-byte widget records) |
| 34 | data  | 288 | menu/dialog layout (N x 72-byte widget records) |
| 35 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 36 | data  | 144 | menu/dialog layout (N x 72-byte widget records) |
| 37 | data  | 2016 | menu/dialog layout (N x 72-byte widget records) |
| 38 | data  | 720 | menu/dialog layout (N x 72-byte widget records) |
| 39 | data  | 360 | menu/dialog layout (N x 72-byte widget records) |
| 40 | data  | 144 | menu/dialog layout (N x 72-byte widget records) |
| 41 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 42 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 43 | data  | 288 | menu/dialog layout (N x 72-byte widget records) |
| 44 | data  | 648 | menu/dialog layout (N x 72-byte widget records) |
| 45 | data  | 360 | menu/dialog layout (N x 72-byte widget records) |
| 46 | data  | 648 | menu/dialog layout (N x 72-byte widget records) |
| 47 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 48 | data  | 1800 | menu/dialog layout (N x 72-byte widget records) |
| 49 | data  | 1008 | menu/dialog layout (N x 72-byte widget records) |
| 50 | data  | 1152 | menu/dialog layout (N x 72-byte widget records) |
| 51 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 52 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 53 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 54 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 55 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 56 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 57 | data  | 360 | menu/dialog layout (N x 72-byte widget records) |
| 58 | data  | 288 | menu/dialog layout (N x 72-byte widget records) |
| 59 | data  | 2232 | menu/dialog layout (N x 72-byte widget records) |
| 60 | data  | 3384 | menu/dialog layout (N x 72-byte widget records) |
| 61 | data  | 4536 | menu/dialog layout (N x 72-byte widget records) |
| 62 | data  | 5616 | menu/dialog layout (N x 72-byte widget records) |
| 63 | data  | 576 | menu/dialog layout (N x 72-byte widget records) |
| 64 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 65 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 66 | data  | 1368 | menu/dialog layout (N x 72-byte widget records) |
| 67 | data  | 1440 | menu/dialog layout (N x 72-byte widget records) |
| 68 | data  | 288 | menu/dialog layout (N x 72-byte widget records) |
| 69 | data  | 216 | menu/dialog layout (N x 72-byte widget records) |
| 70 | data  | 216 | menu/dialog layout (N x 72-byte widget records) |
| 71 | data  | 216 | menu/dialog layout (N x 72-byte widget records) |
| 72 | data  | 936 | menu/dialog layout (N x 72-byte widget records) |
| 73 | data  | 1152 | menu/dialog layout (N x 72-byte widget records) |
| 74 | data  | 1008 | menu/dialog layout (N x 72-byte widget records) |
| 75 | data  | 720 | menu/dialog layout (N x 72-byte widget records) |
| 76 | data  | 216 | menu/dialog layout (N x 72-byte widget records) |
| 77 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 78 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 79 | data  | 360 | menu/dialog layout (N x 72-byte widget records) |
| 80 | data  | 4248 | menu/dialog layout (N x 72-byte widget records) |
| 81 | data  | 360 | menu/dialog layout (N x 72-byte widget records) |
| 82 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 83 | data  | 432 | menu/dialog layout (N x 72-byte widget records) |
| 84 | data  | 216 | menu/dialog layout (N x 72-byte widget records) |
| 85 | data  | 864 | menu/dialog layout (N x 72-byte widget records) |
| 86 | data  | 1296 | menu/dialog layout (N x 72-byte widget records) |
| 87 | data  | 360 | menu/dialog layout (N x 72-byte widget records) |
| 88 | data  | 216 | menu/dialog layout (N x 72-byte widget records) |
| 89 | data  | 1080 | menu/dialog layout (N x 72-byte widget records) |
| 90 | data  | 144 | menu/dialog layout (N x 72-byte widget records) |
| 91 | image 18x18 | 328 | image 18x18 (icon, like 2) |
| 92 | data  | 4392 | menu/dialog layout (N x 72-byte widget records) |
| 93 | data  | 216 | menu/dialog layout (N x 72-byte widget records) |
| 94 | data  | 792 | menu/dialog layout (N x 72-byte widget records) |
| 95 | data  | 864 | menu/dialog layout (N x 72-byte widget records) |
| 96 | image 320x192 | 61444 | image 320x192 (unknown screen) [pal 14?] |
| 97 | data  | 216 | menu/dialog layout (N x 72-byte widget records) |
| 98 | data  | 864 | menu/dialog layout (N x 72-byte widget records) |
| 99 | data  | 1296 | menu/dialog layout (N x 72-byte widget records) |

## War Strings (= PC STRDAT.WAR, id 4000, 105 entries)

| # | type | size | content |
|---:|---|---:|---|
| 0 | strtab 2 strings | 23 | strings: "TEXTBOX"; "MESSAGE:" |
| 1 | strtab 428 strings | 7908 | unit/upgrade/spell names + in-game strings (string t+1 = unit type t) |
| 2 | strtab 5 strings | 59 | strings: "STAT_F10_DLG"; "Menu"; "Pause" |
| 3 | strtab 2 strings | 18 | strings: "Title"; "oOK" |
| 4 | strtab 6 strings | 121 | strings: "MainMenu"; "sSingle Player Game"; "mMulti Player Game" |
| 5 | strtab 4 strings | 76 | strings: "MultiPlayer"; "cConnect"; "pPrevious Menu" |
| 6 | strtab 4 strings | 73 | strings: "NewCampaign"; "oOrc Campaign"; "hHuman Campaign" |
| 7 | strtab 9 strings | 137 | strings: "GameMenu"; "Return to Game (Esc)"; "sSave" |
| 8 | strtab 5 strings | 84 | strings: "HelpMenu"; "Previous (Esc)"; "hKeystroke Help" |
| 9 | strtab 9 strings | 162 | strings: "GameMenu"; "Previous (Esc)"; "rRestart Scenario" |
| 10 | strtab 6 strings | 87 | strings: "Options"; "Previous (Esc)"; "sSound" |
| 11 | strtab 21 strings | 305 | strings: "SND_DLG"; "oOK"; "cCancel" |
| 12 | strtab 14 strings | 218 | strings: "Screen"; "oOK"; "cCancel" |
| 13 | strtab 13 strings | 130 | strings: "Speed"; "oOK"; "cCancel" |
| 14 | strtab 6 strings | 94 | strings: "Quit"; "xExit Program"; "Cancel (Esc)" |
| 15 | strtab 6 strings | 105 | strings: "Restart"; "rRestart Scenario"; "Cancel (Esc)" |
| 16 | strtab 6 strings | 109 | strings: "Restart"; "rReturn to Chatroom"; "Cancel (Esc)" |
| 17 | strtab 6 strings | 102 | strings: "Restart"; "lLoad Custom"; "Cancel (Esc)" |
| 18 | strtab 6 strings | 103 | strings: "Restart"; "sSurrender"; "Cancel (Esc)" |
| 19 | strtab 6 strings | 99 | strings: "Quit"; "qQuit to Menu"; "Cancel (Esc)" |
| 20 | strtab 5 strings | 85 | strings: "Win_Mission"; "vVictory"; "sSave Game" |
| 21 | strtab 4 strings | 60 | strings: "Lose_Mission"; "oOK"; "You failed to" |
| 22 | strtab 12 strings | 114 | strings: "Mission_Stats"; "Continue"; "Outcome" |
| 23 | strtab 12 strings | 114 | strings: "Mission_Stats"; "Continue"; "Outcome" |
| 24 | strtab 12 strings | 114 | strings: "Mission_Stats"; "Continue"; "Outcome" |
| 25 | strtab 11 strings | 103 | strings: "Mission_Stats"; "Outcome"; "Rank" |
| 26 | strtab 5 strings | 59 | strings: "Savegame"; "sSave"; "dDelete" |
| 27 | strtab 4 strings | 47 | strings: "Loadgame"; "lLoad"; "cCancel" |
| 28 | strtab 4 strings | 47 | strings: "Loadgame"; "lLoad"; "cCancel" |
| 29 | strtab 5 strings | 69 | strings: "Ally_Filters"; "OK"; "Cancel" |
| 30 | strtab 6 strings | 81 | strings: "Message_Filters"; "OK"; "Cancel" |
| 31 | strtab 3 strings | 41 | strings: "Ok_Cancel_Dialog"; "oOK"; "cCancel" |
| 32 | strtab 2 strings | 18 | strings: "BigOK"; "oOK" |
| 33 | strtab 2 strings | 29 | strings: "Ok_Cancel_Dialog"; "oOK" |
| 34 | strtab 2 strings | 30 | strings: "Cancel_Dialog"; "cCancel" |
| 35 | strtab 9 strings | 110 | strings: "DirectLink"; "cConnect"; "pPrevious Menu" |
| 36 | strtab 11 strings | 118 | strings: "Modem"; "cConnect"; "mModem Setup" |
| 37 | strtab 10 strings | 106 | strings: "ModemConfig"; "oOK"; "cCancel" |
| 38 | strtab 7 strings | 118 | strings: "Viewgame"; "jJoin Game"; "cCancel Join" |
| 39 | strtab 3 strings | 65 | strings: "dialog"; "Cancel"; "Waiting for response from remote system." |
| 40 | strtab 87 strings | 2366 | strings: "Connection terminated."; "Unable to allocate real mode memory."; "Unable to locate IPX network." |
| 41 | strtab 5 strings | 80 | strings: "Tips_Dialog"; "oOK"; "nNext Tip" |
| 42 | empty placeholder | 0 | placeholder |
| 43 | strtab 58 strings | 6366 | strings: "You can get additional tips by selecting"; "You can hide and show the Macintosh menu"; "You can create additional peasants in yo" |
| 44 | strtab 3 strings | 32 | strings: "INFOBOX"; "Allies"; "Messages" |
| 45 | strtab 12 strings | 200 | strings: "dialog"; "lLeave Game"; "Multi Player Setup" |
| 46 | strtab 34 strings | 527 | strings: "Built-in scenario"; "Custom scenario"; "Saved game" |
| 47 | strtab 4 strings | 115 | menu strings ("dialog", "Waiting for players:"...) |
| 48 | strtab 5 strings | 60 | strings: "HelpScreen"; "oOK"; "uUp" |
| 49 | strtab 4 strings | 55 | strings: "Connect"; "cCancel"; "Connecting..." |
| 50 | strtab 3 strings | 29 | strings: "CMsg"; "oOK"; "cCancel" |
| 51 | strtab 2 strings | 394 | stored-compressed text: "Please insert the Warcraft II CD..." |
| 52 | strtab 4 strings | 60 | strings: "Game_Name_Dialog"; "oOK"; "cCancel" |
| 53 | strtab 4 strings | 59 | strings: "Objectives"; "Previous (Esc)"; "SCENARIO" |
| 54 | strtab 69 strings | 2585 | campaign objectives + level titles (string table) |
| 55 | strtab 3 strings | 44 | strings: "Human_Dispatch"; "Continue"; "Objectives:" |
| 56 | strtab 3 strings | 42 | strings: "Orc_Dispatch"; "Continue"; "Objectives:" |
| 57 | strtab 2 strings | 23 | strings: "Credits"; "Continue" |
| 58 | strtab 1 strings | 4352 | credits text |
| 59 | strtab 41 strings | 413 | strings: "Servant"; "Slave"; "Peasant" |
| 60 | strtab 42 strings | 957 | strings: "Cmd-Shift-H - Keystroke Help"; "Cmd-space - Hide/show menu bar"; "Option-Cmd-1 - Save Map Location 1" |
| 61 | strtab 18 strings | 1012 | strings: "Answer"; "Call"; "Modem" |
| 62 | strtab 16 strings | 214 | strings: "Act I"; "The Shores of Lordaeron"; "Act II" |
| 63 | strtab 10 strings | 95 | strings: "dialog"; "OK"; "Cancel" |
| 64 | strtab 50 strings | 878 | strings: "Built-in scenario"; "Custom scenario"; "Saved game" |
| 65 | strtab 1 strings | 545 | mission briefing text: human/level01h |
| 66 | strtab 1 strings | 509 | mission briefing text: orc/level01o |
| 67 | strtab 1 strings | 694 | mission briefing text: human/level02h |
| 68 | strtab 1 strings | 493 | mission briefing text: orc/level02o |
| 69 | strtab 1 strings | 701 | mission briefing text: human/level03h |
| 70 | strtab 1 strings | 578 | mission briefing text: orc/level03o |
| 71 | strtab 1 strings | 572 | mission briefing text: human/level04h |
| 72 | strtab 1 strings | 439 | mission briefing text: orc/level04o |
| 73 | strtab 1 strings | 795 | mission briefing text: human/level05h |
| 74 | strtab 1 strings | 520 | mission briefing text: orc/level05o |
| 75 | strtab 1 strings | 396 | mission briefing text: human/level06h |
| 76 | strtab 1 strings | 422 | mission briefing text: orc/level06o |
| 77 | strtab 1 strings | 544 | mission briefing text: human/level07h |
| 78 | strtab 1 strings | 471 | mission briefing text: orc/level07o |
| 79 | strtab 1 strings | 666 | mission briefing text: human/level08h |
| 80 | strtab 1 strings | 487 | mission briefing text: orc/level08o |
| 81 | strtab 1 strings | 352 | mission briefing text: human/level09h |
| 82 | strtab 1 strings | 482 | mission briefing text: orc/level09o |
| 83 | strtab 1 strings | 357 | mission briefing text: human/level10h |
| 84 | strtab 1 strings | 353 | mission briefing text: orc/level10o |
| 85 | strtab 1 strings | 687 | mission briefing text: human/level11h |
| 86 | strtab 1 strings | 667 | mission briefing text: orc/level11o |
| 87 | strtab 1 strings | 424 | mission briefing text: human/level12h |
| 88 | strtab 1 strings | 786 | mission briefing text: orc/level12o |
| 89 | strtab 1 strings | 737 | mission briefing text: human/level13h |
| 90 | strtab 1 strings | 695 | mission briefing text: orc/level13o |
| 91 | strtab 1 strings | 456 | mission briefing text: human/level14h |
| 92 | strtab 1 strings | 307 | mission briefing text: orc/level14o |
| 93 | strtab 2 strings | 1082 | victory texts: string 0 human, string 1 orc |
| 94 | strtab 8 strings | 155 | strings: "MainMenu"; "lSlide Show"; "hHow to Order" |
| 95 | strtab 4 strings | 73 | strings: "NewCampaign"; "hHuman Campaign"; "oOrc Campaign" |
| 96 | strtab 3 strings | 34 | strings: "Slideshow"; "Continue"; "Cancel" |
| 97 | strtab 2 strings | 23 | strings: "Ordering_Info"; "OK" |
| 98 | strtab 15 strings | 3438 | strings: "Warcraft II: Tides of Darkness takes you"; "Having formed an alliance with the Human"; "Invention and magic combines to create g" |
| 99 | strtab 8 strings | 145 | strings: "Viewgame"; "jJoin Game"; "cCancel Join" |
| 100 | strtab 8 strings | 179 | strings: "Viewgame"; "jJoin Game"; "cCancel Join" |
| 101 | strtab 2 strings | 24 | strings: "AboutScreen"; "oOK" |
| 102 | strtab 2 strings | 24 | strings: "AboutScreen"; "oOK" |
| 103 | strtab 8 strings | 145 | strings: "Viewgame"; "jJoin Game"; "cCancel Join" |
| 104 | strtab 12 strings | 107 | strings: "dialog"; "OK"; "Cancel" |

## War Sounds (= PC SFXDAT.SUD, id 5000, 293 entries)

All sound entries: RIFF WAVE PCM mono 8-bit unsigned 11025 Hz (2 entries are 16-bit). Each stored entry is followed (outside its size) by the original DOS file name, shown here.

| # | type | size | content |
|---:|---|---:|---|
| 0 | data  | 1164 | table of u32 file lengths |
| 1 | data  | 11020 | sound description strings (group|name records) |
| 2 | wav  | 2904 | `MISC\ERROR.WAV` ui/placement_error |
| 3 | wav  | 2441 | `MISC\THUNK.WAV` ui/placement_success |
| 4 | wav  | 24238 | `MISC\CONSTRCT.WAV` misc/building_construction |
| 5 | wav  | 10106 | `HUMAN\HWHAT1.WAV` human/basic_voices/selected/1 |
| 6 | wav  | 5916 | `ORC\OWHAT1.WAV` orc/basic_voices/selected/1 |
| 7 | wav  | 11194 | `HUMAN\HWHAT2.WAV` human/basic_voices/selected/2 |
| 8 | wav  | 8916 | `ORC\OWHAT2.WAV` orc/basic_voices/selected/2 |
| 9 | wav  | 8940 | `HUMAN\HWHAT3.WAV` human/basic_voices/selected/3 |
| 10 | wav  | 9738 | `ORC\OWHAT3.WAV` orc/basic_voices/selected/3 |
| 11 | wav  | 5994 | `HUMAN\HWHAT4.WAV` human/basic_voices/selected/4 |
| 12 | wav  | 7000 | `ORC\OWHAT4.WAV` orc/basic_voices/selected/4 |
| 13 | wav  | 6952 | `HUMAN\HWHAT5.WAV` human/basic_voices/selected/5 |
| 14 | wav  | 12672 | `ORC\OWHAT5.WAV` orc/basic_voices/selected/5 |
| 15 | wav  | 13784 | `HUMAN\HWHAT6.WAV` human/basic_voices/selected/6 |
| 16 | wav  | 3970 | `ORC\OWHAT6.WAV` orc/basic_voices/selected/6 |
| 17 | wav  | 16162 | `HUMAN\HPISSED1.WAV` human/basic_voices/annoyed/1 |
| 18 | wav  | 13728 | `ORC\OPISSED1.WAV` orc/basic_voices/annoyed/1 |
| 19 | wav  | 17808 | `HUMAN\HPISSED2.WAV` human/basic_voices/annoyed/2 |
| 20 | wav  | 8570 | `ORC\OPISSED2.WAV` orc/basic_voices/annoyed/2 |
| 21 | wav  | 21422 | `HUMAN\HPISSED3.WAV` human/basic_voices/annoyed/3 |
| 22 | wav  | 8728 | `ORC\OPISSED3.WAV` orc/basic_voices/annoyed/3 |
| 23 | wav  | 20254 | `HUMAN\HPISSED4.WAV` human/basic_voices/annoyed/4 |
| 24 | wav  | 20494 | `ORC\OPISSED4.WAV` orc/basic_voices/annoyed/4 |
| 25 | wav  | 21486 | `HUMAN\HPISSED5.WAV` human/basic_voices/annoyed/5 |
| 26 | wav  | 18368 | `ORC\OPISSED5.WAV` orc/basic_voices/annoyed/5 |
| 27 | wav  | 18918 | `HUMAN\HPISSED6.WAV` human/basic_voices/annoyed/6 |
| 28 | wav  | 19274 | `ORC\OPISSED6.WAV` orc/basic_voices/annoyed/6 |
| 29 | wav  | 20068 | `HUMAN\HPISSED7.WAV` human/basic_voices/annoyed/7 |
| 30 | wav  | 25216 | `ORC\OPISSED7.WAV` orc/basic_voices/annoyed/7 |
| 31 | wav  | 17594 | `MISC\EXPLODE.WAV` misc/explosion |
| 32 | wav  | 6586 | `HUMAN\HYESSIR1.WAV` human/basic_voices/acknowledgement/1 |
| 33 | wav  | 8244 | `ORC\OYESSIR1.WAV` orc/basic_voices/acknowledgement/1 |
| 34 | wav  | 11114 | `HUMAN\HYESSIR2.WAV` human/basic_voices/acknowledgement/2 |
| 35 | wav  | 7658 | `ORC\OYESSIR2.WAV` orc/basic_voices/acknowledgement/2 |
| 36 | wav  | 10314 | `HUMAN\HYESSIR3.WAV` human/basic_voices/acknowledgement/3 |
| 37 | wav  | 9418 | `ORC\OYESSIR3.WAV` orc/basic_voices/acknowledgement/3 |
| 38 | wav  | 11538 | `HUMAN\HYESSIR4.WAV` human/basic_voices/acknowledgement/4 |
| 39 | wav  | 7960 | `ORC\OYESSIR4.WAV` orc/basic_voices/acknowledgement/4 |
| 40 | wav  | 13676 | `HUMAN\HWRKDONE.WAV` human/basic_voices/work_complete |
| 41 | wav  | 10800 | `ORC\OWRKDONE.WAV` orc/basic_voices/work_complete |
| 42 | wav  | 12776 | `PEASANT\PSWRKDON.WAV` human/units/peasant/work_complete |
| 43 | wav  | 14246 | `HUMAN\HREADY.WAV` human/basic_voices/ready |
| 44 | wav  | 19334 | `ORC\OREADY.WAV` orc/basic_voices/ready |
| 45 | wav  | 13100 | `HUMAN\HHELP1.WAV` human/basic_voices/help/1 |
| 46 | wav  | 16004 | `ORC\OHELP1.WAV` orc/basic_voices/help/1 |
| 47 | wav  | 14814 | `HUMAN\HHELP2.WAV` human/basic_voices/help/2 |
| 48 | wav  | 21476 | `ORC\OHELP2.WAV` orc/basic_voices/help/2 |
| 49 | wav  | 14004 | `HUMAN\HDEAD.WAV` human/basic_voices/dead |
| 50 | wav  | 15898 | `ORC\ODEAD.WAV` orc/basic_voices/dead |
| 51 | wav  | 27890 | `SHIPS\SHIPSINK.WAV` ships/sinking |
| 52 | wav  | 34894 | `MISC\BLDEXPL1.WAV` misc/building_explosion/1 |
| 53 | wav  | 31150 | `MISC\BLDEXPL2.WAV` misc/building_explosion/2 |
| 54 | wav  | 43646 | `MISC\BLDEXPL3.WAV` misc/building_explosion/3 |
| 55 | wav  | 9354 | `MISC\CATAPULT.WAV` missiles/catapult-ballista_attack |
| 56 | wav  | 1084 | `MISC\TREE1.WAV` misc/tree_chopping/1 |
| 57 | wav  | 1108 | `MISC\TREE2.WAV` misc/tree_chopping/2 |
| 58 | wav  | 1098 | `MISC\TREE3.WAV` misc/tree_chopping/3 |
| 59 | wav  | 1292 | `MISC\TREE4.WAV` misc/tree_chopping/4 |
| 60 | wav  | 3520 | `MISC\SWORD1.WAV` missiles/sword_attack/1 |
| 61 | wav  | 4360 | `MISC\SWORD2.WAV` missiles/sword_attack/2 |
| 62 | wav  | 4854 | `MISC\SWORD3.WAV` missiles/sword_attack/3 |
| 63 | wav  | 2846 | `MISC\PUNCH.WAV` missiles/punch |
| 64 | wav  | 5260 | `MISC\FIREHIT.WAV` missiles/fireball_hit |
| 65 | wav  | 4920 | `MISC\FIREBALL.WAV` missiles/fireball_throw |
| 66 | wav  | 5276 | `MISC\BOWFIRE.WAV` missiles/bow_throw |
| 67 | wav  | 4452 | `MISC\BOWHIT.WAV` missiles/bow_hit |
| 68 | wav  | 10001 | `SPELLS\SPELL.WAV` spells/basic_spell_sound |
| 69 | wav  | 19954 | `BLDG\SMITH.WAV` buildings/blacksmith |
| 70 | wav  | 42558 | `BLDG\HCHANT.WAV` human/buildings/church |
| 71 | wav  | 26586 | `BLDG\OCHANT.WAV` orc/buildings/altar_of_storms |
| 72 | wav  | 21740 | `BLDG\STABLES.WAV` human/buildings/stables |
| 73 | wav  | 23894 | `BLDG\OGRECAMP.WAV` orc/buildings/ogre_mound |
| 74 | wav  | 19250 | `BLDG\HFARM.WAV` human/buildings/farm |
| 75 | wav  | 14982 | `BLDG\OFARM.WAV` orc/buildings/pig_farm |
| 76 | wav  | 18896 | `BLDG\MINE.WAV` neutral/buildings/gold_mine |
| 77 | wav  | 12452 | `MISC\AXE.WAV` missiles/axe_throw |
| 78 | wav  | 16858 | `SHIPS\FOGHORN.WAV` ships/tanker/acknowledgement/1 |
| 79 | wav  | 6126 | `MISC\FIST.WAV` missiles/fist |
| 80 | wav  | 17004 | `BLDG\SHIPBELL.WAV` buildings/shipyard |
| 81 | wav  | 3264 | `MISC\PEONATAK.WAV` human/units/peasant/attack |
| 82 | wav  | 24966 | `BLDG\OILPLAT.WAV` buildings/oil_platform |
| 83 | wav  | 19898 | `BLDG\OILREFIN.WAV` buildings/oil_refinery |
| 84 | wav  | 25940 | `BLDG\LUMBMILL.WAV` buildings/lumbermill |
| 85 | wav  | 16562 | `MISC\DOCK.WAV` misc/transport_docking |
| 86 | wav  | 22382 | `MISC\BURNING.WAV` misc/burning |
| 87 | wav  | 12944 | `BLDG\AVIARY.WAV` human/buildings/gryphon_aviary |
| 88 | wav  | 22872 | `BLDG\DRAGON.WAV` orc/buildings/dragon_roost |
| 89 | wav  | 22392 | `BLDG\FOUNDRY.WAV` buildings/foundry |
| 90 | wav  | 22426 | `BLDG\INVENTOR.WAV` human/buildings/gnomish_inventor |
| 91 | wav  | 22382 | `BLDG\ALCHEMST.WAV` orc/buildings/goblin_alchemist |
| 92 | wav  | 24560 | `BLDG\WZRDTOWR.WAV` human/buildings/mage_tower |
| 93 | wav  | 40816 | `BLDG\DTHTOWER.WAV` orc/buildings/temple_of_the_damned |
| 94 | wav  | 11470 | `MISC\HCAPTURE.WAV` human/capture |
| 95 | wav  | 11020 | `MISC\OCAPTURE.WAV` orc/capture |
| 96 | wav  | 34196 | `MISC\HRESCUE.WAV` human/rescue |
| 97 | wav  | 23766 | `MISC\ORESCUE.WAV` orc/rescue |
| 98 | wav  | 26122 | `SPELLS\BLODLUST.WAV` spells/bloodlust |
| 99 | wav  | 31218 | `SPELLS\DECAY.WAV` spells/death_and_decay |
| 100 | wav  | 22646 | `SPELLS\DETHCOIL.WAV` spells/death_coil |
| 101 | wav  | 22372 | `SPELLS\EXORCISM.WAV` spells/exorcism |
| 102 | wav  | 28598 | `SPELLS\FLAMSHLD.WAV` spells/flame_shield |
| 103 | wav  | 21858 | `SPELLS\HASTE.WAV` spells/haste |
| 104 | wav  | 20528 | `SPELLS\HEAL.WAV` spells/healing |
| 105 | wav  | 19742 | `SPELLS\HOLYVISN.WAV` spells/holy_vision |
| 106 | wav  | 28598 | `SPELLS\ICESTORM.WAV` spells/blizzard |
| 107 | wav  | 12878 | `SPELLS\INVISIBL.WAV` spells/invisibility |
| 108 | wav  | 25818 | `SPELLS\IOKILROG.WAV` spells/eye_of_kilrogg |
| 109 | wav  | 15682 | `SPELLS\MORPH.WAV` spells/polymorph |
| 110 | wav  | 25132 | `SPELLS\SLOW.WAV` spells/slow |
| 111 | wav  | 27138 | `SPELLS\THUNDER.WAV` spells/lightning |
| 112 | wav  | 6054 | `SPELLS\TOUCHDRK.WAV` spells/touch_of_darkness |
| 113 | wav  | 9326 | `SPELLS\UNHLYARM.WAV` spells/unholy_armor |
| 114 | wav  | 28598 | `SPELLS\WHRLWIND.WAV` spells/whirlwind |
| 115 | wav  | 9860 | `PEON\PNREADY.WAV` orc/peon/ready |
| 116 | wav  | 24192 | `DEATHKNT\DKPISSD1.WAV` orc/units/death_knight/annoyed/1 |
| 117 | wav  | 17216 | `DEATHKNT\DKPISSD2.WAV` orc/units/death_knight/annoyed/2 |
| 118 | wav  | 42294 | `DEATHKNT\DKPISSD3.WAV` orc/units/death_knight/annoyed/3 |
| 119 | wav  | 22894 | `DEATHKNT\DKREADY.WAV` orc/units/death_knight/ready |
| 120 | wav  | 15446 | `DEATHKNT\DKWHAT1.WAV` orc/units/death_knight/selected/1 |
| 121 | wav  | 19578 | `DEATHKNT\DKWHAT2.WAV` orc/units/death_knight/selected/2 |
| 122 | wav  | 27628 | `DEATHKNT\DKYESSR1.WAV` orc/units/death_knight/acknowledgement/1 |
| 123 | wav  | 17824 | `DEATHKNT\DKYESSR2.WAV` orc/units/death_knight/acknowledgement/2 |
| 124 | wav  | 18204 | `DEATHKNT\DKYESSR3.WAV` orc/units/death_knight/acknowledgement/3 |
| 125 | wav  | 14040 | `DWARF\DWPISSD1.WAV` human/units/dwarven_demolition_squad/annoyed/1 |
| 126 | wav  | 11004 | `DWARF\DWPISSD2.WAV` human/units/dwarven_demolition_squad/annoyed/2 |
| 127 | wav  | 19104 | `DWARF\DWPISSD3.WAV` human/units/dwarven_demolition_squad/annoyed/3 |
| 128 | wav  | 9584 | `DWARF\DWREADY.WAV`  |
| 129 | wav  | 10430 | `DWARF\DWHAT1.WAV`  |
| 130 | wav  | 4186 | `DWARF\DWHAT2.WAV`  |
| 131 | wav  | 10950 | `DWARF\DWYESSR1.WAV`  |
| 132 | wav  | 6324 | `DWARF\DWYESSR2.WAV`  |
| 133 | wav  | 4108 | `DWARF\DWYESSR3.WAV`  |
| 134 | wav  | 5260 | `DWARF\DWYESSR4.WAV`  |
| 135 | wav  | 7812 | `DWARF\DWYESSR5.WAV`  |
| 136 | wav  | 11750 | `ELVES\EPISSED1.WAV` human/units/elven_archer-ranger/annoyed/1 |
| 137 | wav  | 15770 | `ELVES\EPISSED2.WAV` human/units/elven_archer-ranger/annoyed/2 |
| 138 | wav  | 27390 | `ELVES\EPISSED3.WAV` human/units/elven_archer-ranger/annoyed/3 |
| 139 | wav  | 14770 | `ELVES\EREADY.WAV` human/units/elven_archer-ranger/ready |
| 140 | wav  | 11848 | `ELVES\EWHAT1.WAV` human/units/elven_archer-ranger/selected/1 |
| 141 | wav  | 9668 | `ELVES\EWHAT2.WAV` human/units/elven_archer-ranger/selected/2 |
| 142 | wav  | 8790 | `ELVES\EWHAT3.WAV` human/units/elven_archer-ranger/selected/3 |
| 143 | wav  | 10198 | `ELVES\EWHAT4.WAV` human/units/elven_archer-ranger/selected/4 |
| 144 | wav  | 5992 | `ELVES\EYESSIR1.WAV`  |
| 145 | wav  | 10244 | `ELVES\EYESSIR2.WAV`  |
| 146 | wav  | 19872 | `ELVES\EYESSIR3.WAV`  |
| 147 | wav  | 6852 | `ELVES\EYESSIR4.WAV`  |
| 148 | wav  | 11028 | `GNOME\GNPISSD1.WAV` human/units/gnomish_flying_machine/annoyed/1 |
| 149 | wav  | 11910 | `GNOME\GNPISSD2.WAV` human/units/gnomish_flying_machine/annoyed/2 |
| 150 | wav  | 12872 | `GNOME\GNPISSD3.WAV` human/units/gnomish_flying_machine/annoyed/3 |
| 151 | wav  | 23472 | `GNOME\GNPISSD4.WAV` human/units/gnomish_flying_machine/annoyed/4 |
| 152 | wav  | 22642 | `GNOME\GNPISSD5.WAV` human/units/gnomish_flying_machine/annoyed/5 |
| 153 | wav  | 20956 | `GNOME\GNREADY.WAV` human/units/gnomish_flying_machine/ready |
| 154 | wav  | 13476 | `GNOME\GNYESSR1.WAV`  |
| 155 | wav  | 17066 | `GOBLIN\GOPISSD1.WAV` orc/units/goblin_sappers/annoyed/1 |
| 156 | wav  | 12630 | `GOBLIN\GOPISSD2.WAV` orc/units/goblin_sappers/annoyed/2 |
| 157 | wav  | 19488 | `GOBLIN\GOPISSD3.WAV` orc/units/goblin_sappers/annoyed/3 |
| 158 | wav  | 9330 | `GOBLIN\GOREADY.WAV` orc/units/goblin_sappers/ready |
| 159 | wav  | 6776 | `GOBLIN\GOWHAT1.WAV` orc/units/goblin_sappers/selected/1 |
| 160 | wav  | 7996 | `GOBLIN\GOWHAT2.WAV` orc/units/goblin_sappers/selected/2 |
| 161 | wav  | 6760 | `GOBLIN\GOWHAT3.WAV` orc/units/goblin_sappers/selected/3 |
| 162 | wav  | 7848 | `GOBLIN\GOWHAT4.WAV` orc/units/goblin_sappers/selected/4 |
| 163 | wav  | 8308 | `GOBLIN\GOYESSR1.WAV` orc/units/goblin_sappers/acknowledgement/1 |
| 164 | wav  | 10456 | `GOBLIN\GOYESSR2.WAV` orc/units/goblin_sappers/acknowledgement/2 |
| 165 | wav  | 10480 | `GOBLIN\GOYESSR3.WAV` orc/units/goblin_sappers/acknowledgement/3 |
| 166 | wav  | 8068 | `GOBLIN\GOYESSR4.WAV` orc/units/goblin_sappers/acknowledgement/4 |
| 167 | wav  | 22954 | `ZEPPELIN\GBPISSD1.WAV` orc/units/goblin_zeppelin/annoyed/1 |
| 168 | wav  | 20494 | `ZEPPELIN\GBPISSD2.WAV` orc/units/goblin_zeppelin/annoyed/2 |
| 169 | wav  | 10462 | `ZEPPELIN\GBREADY.WAV` orc/units/goblin_zeppelin/ready |
| 170 | wav  | 14068 | `ZEPPELIN\GBYESSR1.WAV` orc/units/goblin_zeppelin/acknowledgement/1 |
| 171 | wav  | 19556 | `KNIGHT\KNPISSD1.WAV` human/units/knight/annoyed/1 |
| 172 | wav  | 15934 | `KNIGHT\KNPISSD2.WAV` human/units/knight/annoyed/2 |
| 173 | wav  | 23278 | `KNIGHT\KNPISSD3.WAV` human/units/knight/annoyed/3 |
| 174 | wav  | 17878 | `KNIGHT\KNREADY.WAV` human/units/knight/ready |
| 175 | wav  | 13374 | `KNIGHT\KNWHAT1.WAV` human/units/knight/selected/1 |
| 176 | wav  | 14436 | `KNIGHT\KNWHAT2.WAV` human/units/knight/selected/2 |
| 177 | wav  | 9574 | `KNIGHT\KNWHAT3.WAV` human/units/knight/selected/3 |
| 178 | wav  | 7472 | `KNIGHT\KNWHAT4.WAV` human/units/knight/selected/4 |
| 179 | wav  | 12560 | `KNIGHT\KNYESSR1.WAV` human/units/knight/acknowledgement/1 |
| 180 | wav  | 11714 | `KNIGHT\KNYESSR2.WAV` human/units/knight/acknowledgement/2 |
| 181 | wav  | 14708 | `KNIGHT\KNYESSR3.WAV` human/units/knight/acknowledgement/3 |
| 182 | wav  | 15720 | `KNIGHT\KNYESSR4.WAV` human/units/knight/acknowledgement/4 |
| 183 | wav  | 21536 | `PALADIN\PKPISSD1.WAV` human/units/paladin/annoyed/1 |
| 184 | wav  | 16634 | `PALADIN\PKPISSD2.WAV` human/units/paladin/annoyed/2 |
| 185 | wav  | 25528 | `PALADIN\PKPISSD3.WAV` human/units/paladin/annoyed/3 |
| 186 | wav  | 19832 | `PALADIN\PKREADY.WAV` human/units/paladin/ready |
| 187 | wav  | 15594 | `PALADIN\PKWHAT1.WAV` human/units/paladin/selected/1 |
| 188 | wav  | 16170 | `PALADIN\PKWHAT2.WAV` human/units/paladin/selected/2 |
| 189 | wav  | 11048 | `PALADIN\PKWHAT3.WAV` human/units/paladin/selected/3 |
| 190 | wav  | 14352 | `PALADIN\PKWHAT4.WAV` human/units/paladin/selected/4 |
| 191 | wav  | 14118 | `PALADIN\PKYESSR1.WAV` human/units/paladin/acknowledgement/1 |
| 192 | wav  | 13272 | `PALADIN\PKYESSR2.WAV` human/units/paladin/acknowledgement/2 |
| 193 | wav  | 16458 | `PALADIN\PKYESSR3.WAV` human/units/paladin/acknowledgement/3 |
| 194 | wav  | 17564 | `PALADIN\PKYESSR4.WAV` human/units/paladin/acknowledgement/4 |
| 195 | wav  | 22570 | `OGRE\OGPISSD1.WAV` orc/units/ogre/annoyed/1 |
| 196 | wav  | 21380 | `OGRE\OGPISSD2.WAV` orc/units/ogre/annoyed/2 |
| 197 | wav  | 17378 | `OGRE\OGPISSD3.WAV` orc/units/ogre/annoyed/3 |
| 198 | wav  | 27386 | `OGRE\OGPISSD4.WAV` orc/units/ogre/annoyed/4 |
| 199 | wav  | 15244 | `OGRE\OGPISSD5.WAV` orc/units/ogre/annoyed/5 |
| 200 | wav  | 24240 | `OGRE\OGREADY.WAV` orc/units/ogre/ready |
| 201 | wav  | 9116 | `OGRE\OGWHAT1.WAV` orc/units/ogre/selected/1 |
| 202 | wav  | 9640 | `OGRE\OGWHAT2.WAV` orc/units/ogre/selected/2 |
| 203 | wav  | 6472 | `OGRE\OGWHAT3.WAV` orc/units/ogre/selected/3 |
| 204 | wav  | 9644 | `OGRE\OGWHAT4.WAV` orc/units/ogre/selected/4 |
| 205 | wav  | 8924 | `OGRE\OGYESSR1.WAV` orc/units/ogre/acknowledgement/1 |
| 206 | wav  | 10730 | `OGRE\OGYESSR2.WAV` orc/units/ogre/acknowledgement/2 |
| 207 | wav  | 10300 | `OGRE\OGYESSR3.WAV` orc/units/ogre/acknowledgement/3 |
| 208 | wav  | 21408 | `OGREMAGE\OMPISSD1.WAV` orc/units/ogre-mage/annoyed/1 |
| 209 | wav  | 19782 | `OGREMAGE\OMPISSD2.WAV` orc/units/ogre-mage/annoyed/2 |
| 210 | wav  | 24468 | `OGREMAGE\OMPISSD3.WAV` orc/units/ogre-mage/annoyed/3 |
| 211 | wav  | 15404 | `OGREMAGE\OMREADY.WAV` orc/units/ogre-mage/ready |
| 212 | wav  | 23838 | `OGREMAGE\OMWHAT1.WAV` orc/units/ogre-mage/selected/1 |
| 213 | wav  | 6038 | `OGREMAGE\OMWHAT2.WAV` orc/units/ogre-mage/selected/2 |
| 214 | wav  | 6348 | `OGREMAGE\OMWHAT3.WAV` orc/units/ogre-mage/selected/3 |
| 215 | wav  | 7932 | `OGREMAGE\OMWHAT4.WAV` orc/units/ogre-mage/selected/4 |
| 216 | wav  | 11382 | `OGREMAGE\OMYESSR1.WAV` orc/units/ogre-mage/acknowledgement/1 |
| 217 | wav  | 9860 | `OGREMAGE\OMYESSR2.WAV` orc/units/ogre-mage/acknowledgement/2 |
| 218 | wav  | 10358 | `OGREMAGE\OMYESSR3.WAV` orc/units/ogre-mage/acknowledgement/3 |
| 219 | wav  | 21242 | `SHIPS\HSHPPIS1.WAV` human/ships/annoyed/1 |
| 220 | wav  | 9372 | `SHIPS\OSHPPIS1.WAV` orc/ships/annoyed/1 |
| 221 | wav  | 22410 | `SHIPS\HSHPPIS2.WAV` human/ships/annoyed/2 |
| 222 | wav  | 13254 | `SHIPS\OSHPPIS2.WAV` orc/ships/annoyed/2 |
| 223 | wav  | 24492 | `SHIPS\HSHPPIS3.WAV` human/ships/annoyed/3 |
| 224 | wav  | 35408 | `SHIPS\OSHPPIS3.WAV` orc/ships/annoyed/3 |
| 225 | wav  | 14524 | `SHIPS\HSHPREDY.WAV` human/ships/ready |
| 226 | wav  | 12608 | `SHIPS\OSHPREDY.WAV` orc/ships/ready |
| 227 | wav  | 10256 | `SHIPS\HSHPWHT1.WAV` human/ships/selected/1 |
| 228 | wav  | 5914 | `SHIPS\OSHPWHT1.WAV` orc/ships/selected/1 |
| 229 | wav  | 6490 | `SHIPS\HSHPWHT2.WAV` human/ships/selected/2 |
| 230 | wav  | 8474 | `SHIPS\OSHPWHT2.WAV` orc/ships/selected/2 |
| 231 | wav  | 9556 | `SHIPS\HSHPWHT3.WAV` human/ships/selected/3 |
| 232 | wav  | 10130 | `SHIPS\OSHPWHT3.WAV` orc/ships/selected/3 |
| 233 | wav  | 10890 | `SHIPS\HSHPYES1.WAV` human/ships/acknowledgement/1 |
| 234 | wav  | 8422 | `SHIPS\OSHPYES1.WAV` orc/ships/acknowledgement/1 |
| 235 | wav  | 10930 | `SHIPS\HSHPYES2.WAV` human/ships/acknowledgement/2 |
| 236 | wav  | 11462 | `SHIPS\OSHPYES2.WAV` orc/ships/acknowledgement/2 |
| 237 | wav  | 10274 | `SHIPS\HSHPYES3.WAV` human/ships/acknowledgement/3 |
| 238 | wav  | 10830 | `SHIPS\OSHPYES3.WAV` orc/ships/acknowledgement/3 |
| 239 | wav  | 12710 | `SHIPS\SUBPISS1.WAV` human/ships/gnomish_submarine/annoyed/1 |
| 240 | wav  | 7820 | `SHIPS\SUBPISS2.WAV` human/ships/gnomish_submarine/annoyed/2 |
| 241 | wav  | 9422 | `SHIPS\SUBPISS3.WAV` human/ships/gnomish_submarine/annoyed/3 |
| 242 | wav  | 16078 | `SHIPS\SUBPISS4.WAV` human/ships/gnomish_submarine/annoyed/4 |
| 243 | wav  | 11954 | `TROLL\TRPISSD1.WAV` orc/units/troll_axethrower-berserker/annoyed/1 |
| 244 | wav  | 29692 | `TROLL\TRPISSD2.WAV` orc/units/troll_axethrower-berserker/annoyed/2 |
| 245 | wav  | 24998 | `TROLL\TRPISSD3.WAV` orc/units/troll_axethrower-berserker/annoyed/3 |
| 246 | wav  | 14530 | `TROLL\TRREADY.WAV` orc/units/troll_axethrower-berserker/ready |
| 247 | wav  | 6628 | `TROLL\TRWHAT1.WAV`  |
| 248 | wav  | 10038 | `TROLL\TRWHAT2.WAV`  |
| 249 | wav  | 9628 | `TROLL\TRWHAT3.WAV`  |
| 250 | wav  | 8538 | `TROLL\TRYESSR1.WAV`  |
| 251 | wav  | 6724 | `TROLL\TRYESSR2.WAV`  |
| 252 | wav  | 10460 | `TROLL\TRYESSR3.WAV`  |
| 253 | wav  | 16476 | `WIZARD\WZPISSD1.WAV` human/units/mage/annoyed/1 |
| 254 | wav  | 14602 | `WIZARD\WZPISSD2.WAV` human/units/mage/annoyed/2 |
| 255 | wav  | 40214 | `WIZARD\WZPISSD3.WAV` human/units/mage/annoyed/3 |
| 256 | wav  | 14056 | `WIZARD\WZREADY.WAV` human/units/mage/ready |
| 257 | wav  | 9154 | `WIZARD\WZWHAT1.WAV` human/units/mage/selected/1 |
| 258 | wav  | 16234 | `WIZARD\WZWHAT2.WAV` human/units/mage/selected/2 |
| 259 | wav  | 12574 | `WIZARD\WZWHAT3.WAV` human/units/mage/selected/3 |
| 260 | wav  | 10850 | `WIZARD\WZYESSR1.WAV` human/units/mage/acknowledgement/1 |
| 261 | wav  | 9766 | `WIZARD\WZYESSR2.WAV` human/units/mage/acknowledgement/2 |
| 262 | wav  | 8378 | `WIZARD\WZYESSR3.WAV` human/units/mage/acknowledgement/3 |
| 263 | wav  | 12088 | `PEASANT\PSREADY.WAV` human/units/peasant/ready |
| 264 | wav  | 7660 | `PEASANT\PSPISSD1.WAV` human/units/peasant/annoyed/1 |
| 265 | wav  | 8838 | `PEASANT\PSPISSD2.WAV` human/units/peasant/annoyed/2 |
| 266 | wav  | 12276 | `PEASANT\PSPISSD3.WAV` human/units/peasant/annoyed/3 |
| 267 | wav  | 10086 | `PEASANT\PSPISSD4.WAV` human/units/peasant/annoyed/4 |
| 268 | wav  | 19080 | `PEASANT\PSPISSD5.WAV` human/units/peasant/annoyed/5 |
| 269 | wav  | 14702 | `PEASANT\PSPISSD6.WAV` human/units/peasant/annoyed/6 |
| 270 | wav  | 17648 | `PEASANT\PSPISSD7.WAV` human/units/peasant/annoyed/7 |
| 271 | wav  | 6172 | `PEASANT\PSWHAT1.WAV` human/units/peasant/selected/1 |
| 272 | wav  | 8452 | `PEASANT\PSWHAT2.WAV` human/units/peasant/selected/2 |
| 273 | wav  | 8514 | `PEASANT\PSWHAT3.WAV` human/units/peasant/selected/3 |
| 274 | wav  | 8238 | `PEASANT\PSWHAT4.WAV` human/units/peasant/selected/4 |
| 275 | wav  | 8972 | `PEASANT\PSYESSR1.WAV` human/units/peasant/acknowledgement/1 |
| 276 | wav  | 10024 | `PEASANT\PSYESSR2.WAV` human/units/peasant/acknowledgement/2 |
| 277 | wav  | 6746 | `PEASANT\PSYESSR3.WAV` human/units/peasant/acknowledgement/3 |
| 278 | wav  | 11028 | `PEASANT\PSYESSR4.WAV` human/units/peasant/acknowledgement/4 |
| 279 | wav  | 22872 | `DRAGON\DRREADY.WAV` orc/units/dragon/ready |
| 280 | wav  | 11244 | `DRAGON\DRWHAT.WAV` orc/units/dragon/selected/1 |
| 281 | wav  | 19978 | `DRAGON\DRYESSR1.WAV` orc/units/dragon/acknowledgement/1 |
| 282 | wav  | 15576 | `DRAGON\DRYESSR2.WAV` orc/units/dragon/acknowledgement/2 |
| 283 | wav  | 22708 | `GRIFFON\GRWHAT.WAV` human/units/gryphon_rider/selected/1 |
| 284 | wav  | 12944 | `GRIFFON\GRIFFON1.WAV` human/units/gryphon_rider/acknowledgement/1; human/units/gryphon_rider/ready |
| 285 | wav  | 12440 | `GRIFFON\GRIFFON2.WAV` human/units/gryphon_rider/acknowledgement/2 |
| 286 | wav  | 9524 | `MISC\SHEEP.WAV` neutral/units/sheep/selected/1 |
| 287 | wav  | 15554 | `MISC\SHPISD1.WAV` neutral/units/sheep/annoyed/1 |
| 288 | wav  | 3828 | `MISC\SEAL.WAV` neutral/units/seal/selected/1 |
| 289 | wav  | 4990 | `MISC\SEALPISD.WAV` neutral/units/seal/annoyed/1 |
| 290 | wav  | 10928 | `MISC\PIG.WAV` neutral/units/pig/selected/1 |
| 291 | wav  | 9996 | `MISC\PIGPISSD.WAV` neutral/units/pig/annoyed/1 |
| 292 | wav  | 19114 | `MISC\CATYESSR.WAV` units/catapult-ballista/acknowledgement/1 |

## War Music (= PC SNDDAT.WAR, id 2000, 49 entries)

| # | type | size | content |
|---:|---|---:|---|
| 0 | data  | 164 | menu string table (Custom Game Setup) |
| 1 | empty placeholder | 0 | placeholder |
| 2 | wav  | 796022 | campaign human/victory |
| 3 | wav  | 838284 | campaign orc/victory |
| 4 | wav  | 428280 | campaign human/level01h-intro1 |
| 5 | wav  | 317104 | campaign human/level01h-intro2 |
| 6 | wav  | 495936 | campaign human/level02h-intro1 |
| 7 | wav  | 429034 | campaign human/level02h-intro2 |
| 8 | wav  | 424970 | campaign human/level03h-intro1 |
| 9 | wav  | 508688 | campaign human/level03h-intro2 |
| 10 | wav  | 428866 | campaign human/level04h-intro1 |
| 11 | wav  | 373846 | campaign human/level04h-intro2 |
| 12 | wav  | 703966 | campaign human/level05h-intro1 |
| 13 | wav  | 395906 | campaign human/level05h-intro2 |
| 14 | wav  | 592718 | campaign human/level06h-intro1 |
| 15 | wav  | 455942 | campaign human/level07h-intro1 |
| 16 | wav  | 346224 | campaign human/level07h-intro2 |
| 17 | wav  | 454104 | campaign human/level08h-intro1 |
| 18 | wav  | 459346 | campaign human/level08h-intro2 |
| 19 | wav  | 488120 | campaign human/level09h-intro1 |
| 20 | wav  | 495700 | campaign human/level10h-intro1 |
| 21 | wav  | 570694 | campaign human/level11h-intro1 |
| 22 | wav  | 381614 | campaign human/level11h-intro2 |
| 23 | wav  | 598026 | campaign human/level12h-intro1 |
| 24 | wav  | 549340 | campaign human/level13h-intro1 |
| 25 | wav  | 491166 | campaign human/level13h-intro2 |
| 26 | wav  | 657130 | campaign human/level14h-intro1 |
| 27 | wav  | 729272 | campaign orc/level01o-intro1 |
| 28 | wav  | 701508 | campaign orc/level02o-intro1 |
| 29 | wav  | 532626 | campaign orc/level03o-intro1 |
| 30 | wav  | 305016 | campaign orc/level03o-intro2 |
| 31 | wav  | 663272 | campaign orc/level04o-intro1 |
| 32 | wav  | 824186 | campaign orc/level05o-intro1 |
| 33 | wav  | 634644 | campaign orc/level06o-intro1 |
| 34 | wav  | 487042 | campaign orc/level07o-intro1 |
| 35 | wav  | 243446 | campaign orc/level07o-intro2 |
| 36 | wav  | 769272 | campaign orc/level08o-intro1 |
| 37 | wav  | 732356 | campaign orc/level09o-intro1 |
| 38 | wav  | 529222 | campaign orc/level10o-intro1 |
| 39 | wav  | 503020 | campaign orc/level11o-intro1 |
| 40 | wav  | 516122 | campaign orc/level11o-intro2 |
| 41 | wav  | 398840 | campaign orc/level12o-intro1 |
| 42 | wav  | 772498 | campaign orc/level12o-intro2 |
| 43 | wav  | 862984 | campaign orc/level13o-intro1 |
| 44 | wav  | 258780 | campaign orc/level13o-intro2 |
| 45 | wav  | 504158 | campaign orc/level14o-intro1 |
| 46 | empty placeholder | 0 | placeholder |
| 47 | data  | 90 | menu string table (Single Player) |
| 48 | data  | 238 | menu string table (Multi Player Setup) |

## War Movies (= PC MUDDAT.CUD, id 6000, 15 entries)

| # | type | size | content |
|---:|---|---:|---|
| 0 | smacker  | 1160252 | Smacker video: orc-1 |
| 1 | data  | 1296 | menu/dialog layout (72-byte records) |
| 2 | smacker  | 1405084 | Smacker video: orc-3 |
| 3 | smacker  | 4315168 | Smacker video: human-4 (dup of 4?) |
| 4 | smacker  | 5529336 | Smacker video: human-4 |
| 5 | smacker  | 9046736 | Smacker video: game intro (dup of 6?) |
| 6 | smacker  | 12569008 | Smacker video: game intro |
| 7 | data  | 360 | menu/dialog layout (72-byte records) |
| 8 | smacker  | 4915016 | Smacker video: orc-4 (dup of 9?) |
| 9 | smacker  | 6162568 | Smacker video: orc-4 |
| 10 | smacker  | 1265684 | Smacker video: human-3 |
| 11 | smacker  | 909936 | Smacker video: human-1 |
| 12 | smacker  | 987408 | Smacker video: orc-2 |
| 13 | data  | 4392 | menu/dialog layout (72-byte records) |
| 14 | smacker  | 2467820 | Smacker video: human-2 |
