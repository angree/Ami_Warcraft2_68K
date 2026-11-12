#!/bin/bash
# Host (WSL/Linux gcc, ASan/UBSan) build + run of the in-game UI test (tools/test_ui.c).
# Usage: bash tools/build_test_ui.sh [data dir] [shot dir]
# Links every src/*.c except the Amiga-only shell files; modules not written yet come from the weak
# placeholders in src/w2_stubs.c. Shots (rendered from the original data) go OUTSIDE the repository.
[ -z "${BASH_VERSION:-}" ] && exec bash "$0" "$@"
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DATA=${1:-"$ROOT/../_AmiW2-poza-repo/gamedata/data"}
OUT=${2:-"$ROOT/../_AmiW2-poza-repo/scratch/ui"}
[ -d "$DATA" ] || DATA="/mnt/d/GITHUB/_AmiW2-poza-repo/gamedata/data"
[ -d "$(dirname "$OUT")" ] || OUT="/mnt/d/GITHUB/_AmiW2-poza-repo/scratch/ui"
mkdir -p "$OUT"
BIN="$ROOT/build/host/test_ui"
OBJ="$ROOT/build/host/test_ui_obj"
mkdir -p "$OBJ"
CC=${CC:-gcc}
CFLAGS="-std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -DW2_HOST -I$ROOT/src -I$ROOT/native"
LDFLAGS="-fsanitize=address,undefined"
for w in fopen freopen remove unlink rename mkdir rmdir opendir stat access; do LDFLAGS="$LDFLAGS -Wl,--wrap=$w"; done
# the test records the commands the UI issues and passes them on (real module or stub)
for w in cmd_build cmd_smart cmd_move cmd_attack cmd_attack_move cmd_train cmd_stop; do LDFLAGS="$LDFLAGS -Wl,--wrap=$w"; done
OBJS=""
# test_ui.c first: its weak econ_* stand-ins must come before the w2_stubs.c placeholders
for s in "$ROOT"/tools/test_ui.c "$ROOT"/src/*.c "$ROOT"/native/host_gfx.c "$ROOT"/native/host_sys.c "$ROOT"/native/host_audio.c; do
	b=$(basename "$s" .c)
	case "$b" in main_amiga|setup_amiga) continue ;; esac
	$CC $CFLAGS -c "$s" -o "$OBJ/$b.o"
	OBJS="$OBJS $OBJ/$b.o"
done
$CC $LDFLAGS -o "$BIN" $OBJS
echo "build_test_ui: built $BIN"
RC=0
for mode in 0 1; do
	tag=$([ $mode = 1 ] && echo lores || echo hires)
	W2_HEADLESS=1 W2_SHOTDIR="$OUT" W2_PROGDIR="$OUT" W2_TEST_DATA="$DATA" W2_TEST_LORES=$mode W2_TEST_RACE=${W2_TEST_RACE:-} \
		"$BIN" > "$OUT/test_ui_$tag.log" 2>&1 || RC=$?
	grep -E "^(FAIL|test_ui:)|ERROR|runtime error" "$OUT/test_ui_$tag.log" || true
done
echo "build_test_ui: logs and shots in $OUT (exit $RC)"
exit $RC
