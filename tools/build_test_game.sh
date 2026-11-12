#!/bin/bash
# build_test_game.sh - host (WSL gcc, ASan) build + run of tools/test_game.c (game shell, terrain, fog,
# minimap). Links the host platform layer and every src/*.c except main_amiga.c / setup_amiga.c; modules
# not written yet come from the weak stubs in src/w2_stubs.c.
# Usage: bash tools/build_test_game.sh [data dir] [shot dir] [custom pud]
# PNGs go OUTSIDE the repository (they show copyrighted art).
set -eu
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DATA="${1:-/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/data}"
SHOTS="${2:-/mnt/d/GITHUB/_AmiW2-poza-repo/scratch/game}"
PUD="${3:-/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/Islands.pud}"
OUT="$ROOT/build/host/test_game"
mkdir -p "$OUT" "$SHOTS"
CFLAGS="-std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -DW2_HOST -I$ROOT/src -I$ROOT/native"
LDFLAGS="-fsanitize=address,undefined"
for w in fopen freopen remove unlink rename mkdir rmdir opendir stat access; do LDFLAGS="$LDFLAGS -Wl,--wrap=$w"; done
[ -f "$ROOT/src/unit.c" ] && CFLAGS="$CFLAGS -DHAVE_UNIT_C"   # else the test brings a minimal unit_create
SRCS=""
for s in "$ROOT"/src/*.c; do
	case "$(basename "$s")" in main_amiga.c|setup_amiga.c) continue ;; esac
	SRCS="$SRCS $s"
done
gcc $CFLAGS -o "$OUT/test_game" "$ROOT/tools/test_game.c" $SRCS \
	"$ROOT/native/host_gfx.c" "$ROOT/native/host_audio.c" "$ROOT/native/host_sys.c" $LDFLAGS
echo "build_test_game: built $OUT/test_game"
export W2_HEADLESS=1 W2_SHOTDIR="$SHOTS" W2_PROGDIR="$OUT" W2_TEST_DATA="$DATA" W2_TEST_PUD="$PUD" W2_FINALSHOT=0 W2_AUDIOLOG=0
W2_TEST_LORES=0 "$OUT/test_game"
W2_TEST_LORES=1 "$OUT/test_game"
echo "build_test_game: OK -> $SHOTS"
