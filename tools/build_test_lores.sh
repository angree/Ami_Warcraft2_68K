#!/bin/bash
# build_test_lores.sh - host (WSL gcc) build + run of tools/test_lores.c (lores art: filter, packs, drawing).
# Usage: bash tools/build_test_lores.sh [data dir] [shot dir] [--fast]
#   default: ASan/UBSan build. --fast: -O2 without sanitizers (for build-time measurements).
# The packs are written to build/host/test_lores/packs (deleted first, so every run measures a full build).
# PNGs go OUTSIDE the repository (they show copyrighted art).
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DATA="${1:-/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/data}"
SHOTS="${2:-/mnt/d/GITHUB/_AmiW2-poza-repo/scratch/lores}"
MODE="${3:-asan}"
OUT="$ROOT/build/host/test_lores"
mkdir -p "$OUT" "$SHOTS"
if [ "$MODE" = "--fast" ]; then
	CFLAGS="-std=gnu99 -O2 -Wall -DW2_HOST -I$ROOT/src -I$ROOT/native"; LDFLAGS=""
else
	CFLAGS="-std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -DW2_HOST -I$ROOT/src -I$ROOT/native"
	LDFLAGS="-fsanitize=address,undefined"
fi
for w in fopen freopen remove unlink rename mkdir rmdir opendir stat access; do LDFLAGS="$LDFLAGS -Wl,--wrap=$w"; done
SRCS=""
for s in "$ROOT"/src/*.c; do
	case "$(basename "$s")" in main_amiga.c|setup_amiga.c) continue ;; esac
	SRCS="$SRCS $s"
done
gcc $CFLAGS -o "$OUT/test_lores" "$ROOT/tools/test_lores.c" $SRCS \
	"$ROOT/native/host_gfx.c" "$ROOT/native/host_audio.c" "$ROOT/native/host_sys.c" $LDFLAGS -lm
echo "build_test_lores: built $OUT/test_lores ($MODE)"
rm -rf "$OUT/packs"; mkdir -p "$OUT/packs"
printf 'resolution=lores\nlores_pack_dir=PROGDIR:packs\n' > "$OUT/w2.cfg"
export W2_HEADLESS=1 W2_SHOTDIR="$SHOTS" W2_PROGDIR="$OUT" W2_TEST_DATA="$DATA" W2_FINALSHOT=0 W2_AUDIOLOG=0
"$OUT/test_lores"
echo "build_test_lores: OK -> $SHOTS"
