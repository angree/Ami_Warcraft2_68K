#!/bin/bash
# build_test_move.sh - host (WSL/Linux gcc, ASan+UBSan) build + run of the movement/pathfinding test.
# Usage: bash tools/build_test_move.sh [--no-run]
# Env:   W2_DATA    game data dir with "War Data" etc. (default /mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/data)
#        W2_SHOTDIR where the PNG shots go - OUTSIDE the repository (default .../_AmiW2-poza-repo/scratch/move)
# Also compiles src/path.c + src/move.c with the Amiga cross compiler when it is installed (warnings = failure).
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/build/host"
mkdir -p "$OUT"
export W2_SHOTDIR="${W2_SHOTDIR:-/mnt/d/GITHUB/_AmiW2-poza-repo/scratch/move}"
mkdir -p "$W2_SHOTDIR"
SRC="w2_stubs w2_tables assets font35 pud war w2gfx_decode rng w2cfg unit path move"
FILES=""
for s in $SRC; do FILES="$FILES $ROOT/src/$s.c"; done
gcc -std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -DW2_HOST \
    -I"$ROOT/src" -I"$ROOT/native" -o "$OUT/test_move" "$ROOT/tools/test_move.c" $FILES
echo "build_test_move: host build OK -> $OUT/test_move"

M68K=/opt/amiga/bin/m68k-amigaos-gcc
if [ -x "$M68K" ]; then
	for f in path move; do
		LOG=$("$M68K" -mcpu=68020 -msoft-float -noixemul -O1 -std=gnu99 -Wall -I"$ROOT/src" -I"$ROOT/native" \
		      -c "$ROOT/src/$f.c" -o "$OUT/$f.m68k.o" 2>&1) || { echo "$LOG"; echo "build_test_move: m68k compile of $f.c FAILED"; exit 1; }
		if [ -n "$LOG" ]; then echo "$LOG"; echo "build_test_move: m68k warnings in $f.c"; exit 1; fi
	done
	echo "build_test_move: m68k compile of path.c move.c OK (warning-free)"
fi

[ "${1:-}" = "--no-run" ] && exit 0
ASAN_OPTIONS=detect_leaks=0 "$OUT/test_move"
