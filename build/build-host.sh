#!/bin/bash
# build-host.sh - Linux/WSL host build of AmiW2 (platform API emulated by native/host_*.c).
# Usage: build/build-host.sh [--test] [--no-sdl]
#   --test    after building, run the API self-test headless and check its PNG
#   --no-sdl  never compile the optional SDL2 window (default: on when sdl2-config exists;
#             the window only opens at run time when DISPLAY is set and W2_HEADLESS is not)
# Env: CC, W2_HOST_EXCLUDE="a.c b.c" (more src/ files to leave out), W2_API_DIR (where to take
#      amiga_gfx.h/amiga_audio.h from while native/ does not have them yet).
# Output: build/host/w2 (the game, or the self-test while src/main_amiga.c is missing or not
#         host-buildable) and build/host/host_selftest. See docs/HOST_TEST.md.
[ -z "${BASH_VERSION:-}" ] && exec bash "$0" "$@"   # build/w.sh runs scripts with sh
set -u
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="${W2_HOST_OUT:-$ROOT/build/host}"   # W2_HOST_OUT: own output dir for parallel builds
OBJ="$OUT/obj"
CC="${CC:-gcc}"
TEST=0
SDL=1
for a in "$@"; do
	case "$a" in
	--test) TEST=1 ;;
	--no-sdl) SDL=0 ;;
	*) echo "build-host: unknown option $a" >&2; exit 2 ;;
	esac
done

# Files never built for the host: Amiga-only by design.
EXCLUDE="setup_amiga.c ${W2_HOST_EXCLUDE:-}"

CFLAGS="-std=gnu99 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -DW2_HOST"
WRAPS="fopen freopen remove unlink rename mkdir rmdir opendir stat access"
LDFLAGS="-fsanitize=address,undefined"
for w in $WRAPS; do LDFLAGS="$LDFLAGS -Wl,--wrap=$w"; done

mkdir -p "$OBJ"
rm -f "$OBJ"/*.o
INC="-I$ROOT/src -I$ROOT/native"
# Same rule as build.sh: once some src/ file other than main_amiga.c defines game_init, the stub game in
# main_amiga.c is switched off with -DW2_HAVE_GAME.
for s in "$ROOT"/src/*.c; do
	[ "$(basename "$s")" = main_amiga.c ] && continue
	if grep -Eq '^[a-z].*[^a-z_]game_init[[:space:]]*\(' "$s"; then
		CFLAGS="$CFLAGS -DW2_HAVE_GAME"; echo "build-host: game_init in $(basename "$s") -> -DW2_HAVE_GAME"; break
	fi
done

# The API headers come from native/ (copied from the EU2 port). Until they are there, use the EU2
# originals - only those two headers, so nothing else of EU2 leaks into the include path.
if [ ! -f "$ROOT/native/amiga_gfx.h" ] || [ ! -f "$ROOT/native/amiga_audio.h" ]; then
	API_DIR="${W2_API_DIR:-/mnt/i/GITHUB/Ami_EuropaUniversalis2/native}"
	mkdir -p "$OUT/api"
	for h in amiga_gfx.h amiga_audio.h; do
		[ -f "$ROOT/native/$h" ] && continue
		if [ ! -f "$API_DIR/$h" ]; then echo "build-host: native/$h missing and not in $API_DIR" >&2; exit 1; fi
		cp "$API_DIR/$h" "$OUT/api/$h"
		echo "build-host: WARNING native/$h missing - using $API_DIR/$h"
	done
	INC="$INC -idirafter $OUT/api"
fi

SDLCF=""; SDLLIB=""
if [ "$SDL" = 1 ] && command -v sdl2-config >/dev/null 2>&1; then
	SDLCF="-DHOST_SDL $(sdl2-config --cflags)"
	SDLLIB="$(sdl2-config --libs)"
fi

FAIL=0
compile() {   # compile <src> <obj> [extra flags]
	local src="$1" obj="$2"; shift 2
	if ! $CC $CFLAGS $INC "$@" -c "$src" -o "$obj" 2>"$obj.log"; then
		cat "$obj.log" >&2
		return 1
	fi
	[ -s "$obj.log" ] && cat "$obj.log" >&2
	rm -f "$obj.log"
	return 0
}

# --- platform layer
for f in host_gfx host_audio host_sys amiga_adpcm cellcmp; do
	extra=""; [ "$f" = host_gfx ] && extra="$SDLCF"
	compile "$ROOT/native/$f.c" "$OBJ/$f.o" $extra || FAIL=1
done
compile "$ROOT/tools/host_selftest.c" "$OBJ/host_selftest.o" || FAIL=1
[ $FAIL = 0 ] || { echo "build-host: platform layer failed to compile" >&2; exit 1; }

# --- game sources (in parallel)
GAME_OBJS=""
HAVE_MAIN=0
PIDS=""
shopt -s nullglob
for src in "$ROOT"/src/*.c; do
	b="$(basename "$src")"
	skip=0
	for e in $EXCLUDE; do [ "$b" = "$e" ] && skip=1; done
	[ $skip = 1 ] && { echo "build-host: excluded $b"; continue; }
	o="$OBJ/${b%.c}.o"
	if [ "$b" = main_amiga.c ]; then
		# main_amiga.c is used only when it builds through the API alone; otherwise the self-test main.
		if compile "$src" "$o"; then HAVE_MAIN=1; GAME_OBJS="$GAME_OBJS $o"
		else echo "build-host: WARNING src/main_amiga.c does not compile for the host - using tools/host_selftest.c as main" >&2
		fi
		continue
	fi
	compile "$src" "$o" & PIDS="$PIDS $!"
	GAME_OBJS="$GAME_OBJS $o"
done
for p in $PIDS; do wait "$p" || FAIL=1; done
[ $FAIL = 0 ] || { echo "build-host: game sources failed to compile" >&2; exit 1; }

PLAT="$OBJ/host_gfx.o $OBJ/host_audio.o $OBJ/host_sys.o $OBJ/amiga_adpcm.o $OBJ/cellcmp.o"
# self-test: the platform layer alone
$CC $LDFLAGS -o "$OUT/host_selftest" "$OBJ/host_selftest.o" $PLAT $SDLLIB || exit 1
if [ $HAVE_MAIN = 1 ]; then
	$CC $LDFLAGS -o "$OUT/w2" $GAME_OBJS $PLAT $SDLLIB || { echo "build-host: link of w2 failed" >&2; exit 1; }
	echo "build-host: OK -> build/host/w2 (game)  build/host/host_selftest${SDLCF:+  [SDL window support]}"
else
	$CC $LDFLAGS -o "$OUT/w2" "$OBJ/host_selftest.o" $GAME_OBJS $PLAT $SDLLIB || { echo "build-host: link of w2 failed" >&2; exit 1; }
	echo "build-host: OK -> build/host/w2 (SELF-TEST main, no src/main_amiga.c yet)${SDLCF:+  [SDL window support]}"
fi

# --- optional self-test run
if [ $TEST = 1 ]; then
	T="$OUT/selftest"
	rm -rf "$T"; mkdir -p "$T"
	printf '%s\n' '# selftest input' 'at 100 move 100 100' 'at 200 click left 320 240' \
		'+100 key space' '+100 move 600 50' '+50 shot selftest' '+50 quit' > "$T/input.txt"
	W2_HEADLESS=1 W2_INPUT="$T/input.txt" W2_SHOTDIR="$T" W2_PROGDIR="$T" W2_WAV=1 \
		"$OUT/host_selftest" || { echo "build-host: self-test FAILED (exit $?)" >&2; exit 1; }
	for need in selftest.png final.png selftest.txt audio.wav; do
		[ -s "$T/$need" ] || { echo "build-host: self-test FAILED - no $need" >&2; exit 1; }
	done
	echo "build-host: self-test OK -> build/host/selftest/selftest.png"
	if [ $HAVE_MAIN = 1 ]; then
		# the real program: 3 virtual seconds, a shot, quit; its log is build/host/run/w2.log
		R="$OUT/run"
		mkdir -p "$R"; rm -f "$R"/*.png
		printf '%s\n' 'at 1000 move 160 100' 'at 2000 click left 40 40' 'at 3000 shot game' 'at 3100 quit' > "$R/input.txt"
		W2_HEADLESS=1 W2_INPUT="$R/input.txt" W2_SHOTDIR="$R" W2_PROGDIR="${W2_PROGDIR:-$R}" W2_TIMEOUT=60000 \
			"$OUT/w2" || { echo "build-host: w2 run FAILED (exit $?) - see $R/w2.log" >&2; exit 1; }
		[ -s "$R/game.png" ] || { echo "build-host: w2 run FAILED - no game.png" >&2; exit 1; }
		echo "build-host: w2 run OK -> build/host/run/game.png, log build/host/run/w2.log"
	fi
fi
