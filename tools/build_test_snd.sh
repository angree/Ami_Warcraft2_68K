#!/bin/bash
# Host build + run of tools/test_snd.c (src/snd.c on the host audio backend, ASan/UBSan).
# Usage (WSL): bash tools/build_test_snd.sh [data dir] [scratch out dir]
#   data dir     a w2conv output "data" folder (archives + music/), default gamedata/conv/data
#   scratch dir  where audio.wav goes (outside the repository: it holds original game audio)
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
DATA=${1:-"$ROOT/../../gamedata/conv/data"}
[ -d "$DATA" ] || DATA="/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/conv/data"
OUT=${2:-"/mnt/d/GITHUB/_AmiW2-poza-repo/scratch/snd"}
BIN="$ROOT/build/host"
mkdir -p "$BIN" "$OUT"
CF="-std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -DW2_HOST -I$ROOT/src -I$ROOT/native"
LD="-fsanitize=address,undefined"
for w in fopen freopen remove unlink rename mkdir rmdir opendir stat access; do LD="$LD -Wl,--wrap=$w"; done
SRC="$ROOT/tools/test_snd.c $ROOT/src/snd.c $ROOT/src/assets.c $ROOT/src/war.c $ROOT/src/w2gfx_decode.c \
     $ROOT/src/w2_tables.c $ROOT/src/w2cfg.c $ROOT/src/w2_stubs.c $ROOT/src/rng.c $ROOT/src/font35.c \
     $ROOT/native/host_gfx.c $ROOT/native/host_audio.c $ROOT/native/host_sys.c $ROOT/native/amiga_adpcm.c"
gcc $CF -o "$BIN/test_snd" $SRC $LD -lm
rm -f "$OUT/audio.wav"
W2_HEADLESS=1 W2_WAV=1 W2_SHOTDIR="$OUT" W2_PROGDIR="$OUT" W2_DATA="$DATA" W2_FINALSHOT=0 "$BIN/test_snd"
