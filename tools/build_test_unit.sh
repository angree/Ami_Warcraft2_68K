#!/bin/bash
# Host build + run of the unit module test (tools/test_unit.c): ASan/UBSan, -Wall.
# Usage: bash tools/build_test_unit.sh [--m68k]
#   --m68k  also compile src/unit.c and src/unitdraw.c with the Amiga cross compiler (warning check)
# Env: W2_DATA (game data dir), W2_PUDDIR (extra PUDs), W2_SHOTDIR (PNG output, outside the repo)
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/build/host"
mkdir -p "$OUT"
export W2_SHOTDIR="${W2_SHOTDIR:-/mnt/d/GITHUB/_AmiW2-poza-repo/scratch/unit}"
mkdir -p "$W2_SHOTDIR"
SRC="unit unitdraw assets war w2gfx_decode pud rng w2_tables w2cfg font35 w2_stubs"
FILES="$ROOT/tools/test_unit.c"
for s in $SRC; do FILES="$FILES $ROOT/src/$s.c"; done
gcc -std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -Wextra -Wno-unused-parameter \
	-Wno-sign-compare -DW2_HOST -I"$ROOT/src" -I"$ROOT/native" -o "$OUT/test_unit" $FILES
if [ "${1:-}" = "--m68k" ]; then
	for f in unit unitdraw; do
		/opt/amiga/bin/m68k-amigaos-gcc -mcpu=68020 -msoft-float -noixemul -O1 -std=gnu99 -Wall -Wextra -Wno-unused-parameter \
			-I"$ROOT/src" -I"$ROOT/native" -c "$ROOT/src/$f.c" -o "$OUT/$f.m68k.o"
		echo "m68k: $f.c OK"
	done
fi
ASAN_OPTIONS=detect_leaks=0 "$OUT/test_unit"
