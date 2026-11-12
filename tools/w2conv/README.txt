Warcraft II 68K - data extractor for Windows and Linux
==================================================

The Amiga game needs the data of YOUR OWN Warcraft II. This tool makes it, once, on a PC.
Nothing is downloaded and nothing is sent anywhere; it only reads what you give it.

What to do
----------
1. Unpack this folder anywhere (a path with spaces is fine).
2. Drag and drop ONE of these onto extract.bat:
     - your Mac Warcraft II CD image: the .cue file (its .bin file must be in the same folder,
       under the name written in the .cue), or an .iso;
     - your PC Warcraft II CD image (.cue/.bin or .iso);
     - a folder with the game files ("War Data", "War Resources", ... or MAINDAT.WAR, REZDAT.WAR, ...).
   A window shows the progress. It takes a few seconds to a minute.
3. The result is the folder "data" next to extract.bat. Copy it to the Amiga, into the game's
   drawer, so that it is called "data" there (Warcraft2_68K/data).

What is in "data"
-----------------
  War Data, War Resources, ...   the game archives, copied unchanged
  music/*.adp                    the CD audio music, converted for the Amiga (needs a .cue/.bin image;
                                 a plain .iso or a folder has no CD audio, the game then stays silent)
  lores-*.pack                   the ready-made art of the 320x240 (lores) mode - without them the game
                                 would build them itself on its first lores start (minutes on a 68030)
  ehb-*.pack                     the same art for the ECS mode (Extra-Half-Brite, 64 colours)
  pud/                           the custom maps of the Mac CD
  conversion.txt                 what was converted (sizes, checksums) and "result ok"

Command line
------------
  w2conv.exe [options] <source> <outdir>       the folder "data" is made inside <outdir>
    --rate 11025|14700|22050   music sample rate (default 14700)
    --no-music                 no music
    --no-lores                 no lores packs
    --no-ehb                   no ECS (Extra-Half-Brite) packs
    --lores-only <outdir>      only (re)build the lores and ECS packs of <outdir>\data
    --cue <file.cue>           take the music from this CD image (when <source> is a folder)
    --list <image.cue>         list the files of a Mac CD image

Linux
-----
  ./w2conv-linux [options] <source> <outdir>   the same tool, same options (chmod +x w2conv-linux first)
