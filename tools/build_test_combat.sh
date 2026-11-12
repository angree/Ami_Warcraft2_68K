#!/bin/sh
# Host (WSL/Linux gcc, ASan+UBSan) build + run of the combat/missile module test, plus a warning check
# of src/combat.c + src/missile.c with the Amiga cross compiler when it is installed.
# Usage: sh tools/build_test_combat.sh [data dir] [scratch out dir]
# PNG shots go to the scratch dir OUTSIDE the repository (they contain original art).
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DATA=${1:-"$ROOT/../_AmiW2-poza-repo/gamedata/data"}
[ -d "$DATA" ] || DATA="$ROOT/../../gamedata/data"
OUT=${2:-"$ROOT/../_AmiW2-poza-repo/scratch/combat"}
[ -d "$(dirname "$OUT")" ] || OUT="$ROOT/../../scratch/combat"
BIN="$ROOT/build/host"
mkdir -p "$BIN" "$OUT"
S="$ROOT/src"
gcc -std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -Wextra -Wno-unused-parameter \
    -DW2_HOST -I"$S" -I"$ROOT/native" -o "$BIN/test_combat" \
    "$ROOT/tools/test_combat.c" "$S/combat.c" "$S/missile.c" "$S/unit.c" "$S/unitdraw.c" "$S/assets.c" "$S/pud.c" "$S/war.c" \
    "$S/w2gfx_decode.c" "$S/w2_tables.c" "$S/rng.c" "$S/w2cfg.c" "$S/font35.c" "$S/w2_stubs.c"
if [ -x /opt/amiga/bin/m68k-amigaos-gcc ]; then
	for f in combat missile; do
		/opt/amiga/bin/m68k-amigaos-gcc -mcpu=68020 -msoft-float -noixemul -O1 -std=gnu99 -Wall \
			-I"$S" -I"$ROOT/native" -c "$S/$f.c" -o "$BIN/$f.m68k.o"
	done
	echo "build_test_combat: m68k compile of combat.c + missile.c OK"
fi
ASAN_OPTIONS=detect_leaks=0 "$BIN/test_combat" "$DATA" "$OUT"
