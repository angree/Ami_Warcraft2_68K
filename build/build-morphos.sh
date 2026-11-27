#!/bin/sh
# build-morphos.sh - NATIVE MorphOS PPC build of w2 (the 68k binary keeps running on MorphOS too).
# Needs metaneutrons/morphos-cross (GCC 15, MorphOS SDK 3.20) on PATH - a Linux host: GitHub Actions
# (.github/workflows/morphos.yml) or any Linux machine; NOT the Windows build (no Windows ppc-morphos-gcc).
#   sh build/build-morphos.sh            -> build/mos/w2_MorphOS
# Flags from !instrukcje 45-MorphOS-natywny-PPC-2D.md: -fsigned-char is mandatory (char is unsigned on PPC,
# the deterministic simulation depends on it), -fno-strict-aliasing -fwrapv against -O2 using undefined behaviour.
# MorphOS-only code: native/mos_c2p_stubs.c (68k c2p never called - MorphOS takes the RTG path) and the
# #ifdef __MORPHOS__ parts of amiga_stack.c (crt0 __stack), amiga_trap.c (no 68k trap frames),
# amiga_uclock.c (TimerBase type), amiga_gfx.c (CGX header from the SDK).
set -e
REPO=$(cd "$(dirname "$0")/.." && pwd)
OUT=${OUT:-$REPO/build/mos}
CC=${CC:-ppc-morphos-gcc}
rm -rf "$OUT"; mkdir -p "$OUT/obj"
OPT="-O2 -fno-strict-aliasing -fwrapv -fsigned-char -noixemul"
CF="$OPT -std=gnu99 -Wall -I$REPO/native -I$REPO/src"
$CC --version | head -1
OBJS=""
for p in "$REPO"/src/*.c; do
	f=$(basename "$p")
	case "$f" in setup_amiga.c|main_amiga.c|*_host.c|test_*.c) continue ;; esac
	$CC $CF -c "$p" -o "$OUT/obj/$f.o" 2> "$OUT/obj/$f.warn" || { cat "$OUT/obj/$f.warn"; echo "COMPILE FAILED: $f"; exit 1; }
	[ -s "$OUT/obj/$f.warn" ] && { echo "--- warnings: $f"; cat "$OUT/obj/$f.warn"; }
	OBJS="$OBJS $OUT/obj/$f.o"
done
for b in amiga_gfx amiga_stack amiga_trap amiga_uclock amiga_audio amiga_adpcm w2_sys cellcmp mos_c2p_stubs; do
	$CC $CF -c "$REPO/native/$b.c" -o "$OUT/obj/$b.o" 2> "$OUT/obj/$b.warn" || { cat "$OUT/obj/$b.warn"; echo "COMPILE FAILED: $b.c"; exit 1; }
	[ -s "$OUT/obj/$b.warn" ] && { echo "--- warnings: $b.c"; cat "$OUT/obj/$b.warn"; }
	OBJS="$OBJS $OUT/obj/$b.o"
done
$CC $CF -DW2_HAVE_GAME -c "$REPO/src/main_amiga.c" -o "$OUT/obj/main_amiga.o"
$CC $OPT -o "$OUT/w2_MorphOS" "$OUT/obj/main_amiga.o" $OBJS -lm
file "$OUT/w2_MorphOS" || true
VER=$(sed -n 's/^#define W2_VERSION "\(.*\)"/\1/p' "$REPO/src/version.h")
strings -a "$OUT/w2_MorphOS" | grep -q "$VER" || { echo "w2_MorphOS does not carry version $VER"; exit 1; }
ls -la "$OUT/w2_MorphOS"
echo "MorphOS PPC build OK ($VER)"
