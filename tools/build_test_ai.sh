#!/bin/sh
# Host (WSL/Linux gcc, ASan+UBSan) build + run of the AI test, then an m68k compile check of ai.c.
# Usage: sh tools/build_test_ai.sh [data dir] [scratch out dir]
# The decision log goes to the scratch dir OUTSIDE the repository.
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DATA=${1:-"$ROOT/../_AmiW2-poza-repo/gamedata/data"}
[ -d "$DATA" ] || DATA="/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/data"
OUT=${2:-"/mnt/d/GITHUB/_AmiW2-poza-repo/scratch/ai"}
BIN="$ROOT/build/host"
mkdir -p "$BIN" "$OUT"
API=""
[ -f "$ROOT/native/amiga_gfx.h" ] || API="-idirafter /mnt/i/GITHUB/Ami_EuropaUniversalis2/native"
gcc -std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -Wextra -Wno-unused-parameter \
    -Wno-sign-compare -DW2_HOST -I"$ROOT/src" -I"$ROOT/native" $API -o "$BIN/test_ai" \
    "$ROOT/tools/test_ai.c" "$ROOT/src/ai.c" "$ROOT/src/unit.c" "$ROOT/src/pud.c" "$ROOT/src/assets.c" \
    "$ROOT/src/war.c" "$ROOT/src/w2gfx_decode.c" "$ROOT/src/font35.c" "$ROOT/src/w2_tables.c" \
    "$ROOT/src/rng.c" "$ROOT/src/w2cfg.c" "$ROOT/src/w2_stubs.c"
ASAN_OPTIONS=detect_leaks=0 "$BIN/test_ai" "$DATA" "$OUT"
RC=$?
if [ -x /opt/amiga/bin/m68k-amigaos-gcc ]; then
	/opt/amiga/bin/m68k-amigaos-gcc -mcpu=68020 -msoft-float -noixemul -O1 -std=gnu99 -Wall -I"$ROOT/src" -I"$ROOT/native" $API \
	    -c "$ROOT/src/ai.c" -o "$BIN/ai_m68k.o" && echo "m68k: ai.c compiled ($(wc -c < "$BIN/ai_m68k.o") bytes object)"
fi
exit $RC
