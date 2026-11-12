#!/bin/bash
# build_test_menu.sh - host (WSL/Linux gcc, ASan+UBSan) build and run of tools/test_menu.c (src/menu.c).
# Usage: bash tools/build_test_menu.sh [game data dir] [scratch out dir]
# Runs the test in hires and in lores; PNG shots of every screen go to the scratch dir, which must be
# OUTSIDE the repository (they show copyrighted art). Also compiles src/menu.c with m68k-amigaos-gcc
# when it is installed. Exit code 0 = everything passed.
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EXT=/mnt/d/GITHUB/_AmiW2-poza-repo
[ -d "$EXT" ] || EXT="$(cd "$ROOT/.." && pwd)/_AmiW2-poza-repo"
DATA="${1:-$EXT/gamedata/data}"
OUT="$(realpath -m "${2:-$EXT/scratch/menu}")"
case "$OUT" in "$ROOT"/*) echo "build_test_menu: refusing to write shots inside the repository" >&2; exit 2 ;; esac
BIN="$ROOT/build/host/test_menu"
mkdir -p "$BIN" "$OUT"
CFLAGS="-std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -DW2_HOST -I$ROOT/src -I$ROOT/native"
LDFLAGS="-fsanitize=address,undefined"
for w in fopen freopen remove unlink rename mkdir rmdir opendir stat access; do LDFLAGS="$LDFLAGS -Wl,--wrap=$w"; done
SRCS="src/menu.c src/assets.c src/war.c src/w2gfx_decode.c src/pud.c src/w2cfg.c src/w2_tables.c src/smk.c src/font35.c src/rng.c src/lores.c src/ehb.c src/snd.c src/w2_stubs.c native/host_gfx.c native/host_sys.c native/host_audio.c tools/test_menu.c"
FAIL=0
OBJS=""
for s in $SRCS; do
	o="$BIN/$(basename "${s%.c}").o"
	gcc $CFLAGS -c "$ROOT/$s" -o "$o" || FAIL=1
	OBJS="$OBJS $o"
done
[ $FAIL = 0 ] || { echo "build_test_menu: compile failed" >&2; exit 1; }
gcc $LDFLAGS -o "$BIN/test_menu" $OBJS || { echo "build_test_menu: link failed" >&2; exit 1; }

# PROGDIR for the run: w2.cfg is written there, maps/ holds extra PUDs for the directory listing
PD="$OUT/progdir"
rm -rf "$PD"; mkdir -p "$PD/maps"
for p in "$DATA"/../*.pud; do [ -f "$p" ] && cp "$p" "$PD/maps/"; done
for res in hires lores; do
	W2_HEADLESS=1 W2_DATA="$DATA" W2_RES=$res W2_PROGDIR="$PD" W2_SHOTDIR="$OUT/shots" W2_FINALSHOT=0 W2_AUDIOLOG=0 \
		"$BIN/test_menu" > "$OUT/test_$res.log" 2>&1
	rc=$?
	grep -E "^(PASS|FAIL)|failure|ERROR|runtime error" "$OUT/test_$res.log"
	[ $rc = 0 ] || { echo "build_test_menu: $res run FAILED (exit $rc) - see $OUT/test_$res.log" >&2; FAIL=1; }
done

# 68k compile check (warning-free)
M68K=/opt/amiga/bin/m68k-amigaos-gcc
if [ -x "$M68K" ]; then
	if $M68K -mcpu=68020 -msoft-float -noixemul -O1 -std=gnu99 -Wall -I"$ROOT/src" -I"$ROOT/native" -c "$ROOT/src/menu.c" -o "$BIN/menu_68k.o" 2> "$BIN/menu_68k.log"; then
		if [ -s "$BIN/menu_68k.log" ]; then cat "$BIN/menu_68k.log"; echo "build_test_menu: 68k compile has WARNINGS" >&2; FAIL=1
		else echo "68k: src/menu.c compiles warning-free"; fi
	else cat "$BIN/menu_68k.log"; echo "build_test_menu: 68k compile FAILED" >&2; FAIL=1; fi
fi
[ $FAIL = 0 ] && echo "build_test_menu: ALL OK (shots in $OUT/shots)"
exit $FAIL
