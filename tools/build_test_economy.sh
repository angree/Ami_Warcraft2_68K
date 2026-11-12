#!/bin/bash
# build_test_economy.sh - host (WSL gcc, ASan/UBSan) build + run of tools/test_economy.c.
# Usage: bash tools/build_test_economy.sh [gamedata dir] [scratch out dir]
# Links every src/*.c except the Amiga shell (main_amiga.c, setup_amiga.c) plus the host platform
# layer; modules not written yet come from the weak src/w2_stubs.c. The test supplies its own
# simple tile-step mover (move_toward*) while move.c does not exist.
# PNGs (rendered from the original data) go to the scratch dir OUTSIDE the repository.
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DATA=${1:-/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/data}
OUT=${2:-/mnt/d/GITHUB/_AmiW2-poza-repo/scratch/economy}
BIN="$ROOT/build/host/test_economy"
mkdir -p "$BIN" "$OUT"
CFLAGS="-std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -DW2_HOST -I$ROOT/src -I$ROOT/native"
LD="-fsanitize=address,undefined"
for w in fopen freopen remove unlink rename mkdir rmdir opendir stat access; do LD="$LD -Wl,--wrap=$w"; done
OBJS=""
for s in "$ROOT"/src/*.c "$ROOT"/native/host_gfx.c "$ROOT"/native/host_audio.c "$ROOT"/native/host_sys.c "$ROOT"/tools/test_economy.c; do
	b=$(basename "$s" .c)
	case "$b" in main_amiga|setup_amiga) continue ;; esac
	gcc $CFLAGS -c "$s" -o "$BIN/$b.o"
	OBJS="$OBJS $BIN/$b.o"
done
gcc $LD -o "$BIN/test_economy" $OBJS
echo "build_test_economy: built $BIN/test_economy"
ECON_DATA="$DATA" ECON_OUT="$OUT" W2_HEADLESS=1 W2_PROGDIR="$OUT" ASAN_OPTIONS=detect_leaks=0 "$BIN/test_economy"
