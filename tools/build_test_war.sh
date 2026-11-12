#!/bin/sh
# Host (WSL/Linux gcc) build + run of the archive/gfx reader test.
# Usage: sh tools/build_test_war.sh [data dir] [scratch out dir]
# Outputs (PPMs) go to the scratch dir OUTSIDE the repository (copyrighted data).
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DATA=${1:-"$ROOT/hfs/Data"}
OUT=${2:-"$ROOT/../_AmiW2-poza-repo/formats"}
BIN="$ROOT/build/host"
mkdir -p "$BIN" "$OUT"
gcc -std=gnu99 -O2 -Wall -Wextra -Wno-unused-parameter -fno-strict-aliasing \
    -o "$BIN/test_war" "$ROOT/tools/test_war.c" "$ROOT/src/war.c" "$ROOT/src/w2gfx_decode.c"
# big-endian sanity is by construction (byte-wise reads); also build with -m32 if available
"$BIN/test_war" "$DATA" "$OUT"
