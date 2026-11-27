#!/bin/sh
# Amiga build of AmiW2 (68k; ONE binary for AmigaOS 3.x / AROS 68k / MorphOS, run-time detection only).
#   wsl -d Ubuntu-22.04 sh /mnt/d/GITHUB/Ami_Warcraft2/build/w.sh build/build.sh          build + deploy
#   ... build/build.sh clean                                                              objects from scratch
# Environment:
#   DEPLOY=<dir>        deploy folder (default /mnt/d/GITHUB/_AmiW2-poza-repo/emu/work = the guest's Work:)
#   DEPLOY_AROS=<dir>   copy for the AROS test guest (default /mnt/d/GITHUB/_AmiW2-poza-repo/emu/work-aros if it exists;
#                       DEPLOY_AROS=none to skip)
#   DEPLOY_PLAY=<dir>   copy for the user's play guest (default /mnt/d/GITHUB/_AmiW2-poza-repo/emu/work-play if it exists;
#                       none to skip). Its Work:run starts no job queue, so Workbench comes up normally.
#   W2_EXCLUDE="a.c b.c" more src/ files to leave out of the game (on top of the fixed list below)
#   SHELL_ONLY=1        build w2 from the platform shell alone (main_amiga.c stub test pattern + w2cfg.c),
#                       ignoring every game file in src/ - for testing the platform while game files are broken
#   STRICT=1            -Werror
# Every src/*.c is part of the game EXCEPT: setup_amiga.c (the settings tool), w2cfg.c (linked into both,
# listed explicitly), *_host.c, test_*.c and $W2_EXCLUDE - so teammates add game files without touching this.
# When a src/ file defines game_init(), main_amiga.c is compiled with -DW2_HAVE_GAME (its stub game drops out).
# Rules (!instrukcje 10-68k-2D.md): -mcpu=68020 -msoft-float, never -m68040/-lc/-lpthread, NEVER strip,
# -fbbb without i and e, fp_*.c with -fno-builtin linked ahead of -lm, refuse a link that pulls in libm881.
set -e
REPO=$(cd "$(dirname "$0")/.." && pwd)
WORK=$HOME/w2-build
OBJ=$WORK/obj
DEPLOY=${DEPLOY:-/mnt/d/GITHUB/_AmiW2-poza-repo/emu/work}
if [ -z "$DEPLOY_AROS" ] && [ -d /mnt/d/GITHUB/_AmiW2-poza-repo/emu/work-aros ]; then DEPLOY_AROS=/mnt/d/GITHUB/_AmiW2-poza-repo/emu/work-aros; fi
if [ -z "$DEPLOY_PLAY" ] && [ -d /mnt/d/GITHUB/_AmiW2-poza-repo/emu/work-play ]; then DEPLOY_PLAY=/mnt/d/GITHUB/_AmiW2-poza-repo/emu/work-play; fi
export PATH=/opt/amiga/bin:$PATH
[ "$1" = "clean" ] && rm -rf "$WORK"
mkdir -p "$OBJ" "$DEPLOY"

# sources are mirrored by CONTENT: on drvfs an edit does not always move the mtime WSL sees
rsync -rc --delete --exclude cgx-include "$REPO/native/" "$WORK/native/"
rsync -rc --delete "$REPO/native/cgx-include/" "$WORK/native/cgx-include/"
rsync -rc --delete --include '*.c' --include '*.h' --include '*/' --exclude '*' "$REPO/src/" "$WORK/src/"

CPU="-mcpu=68020 -msoft-float -noixemul"
OPT="-O1 -fomit-frame-pointer -fbbb=abcfhlmnprsz0"
WARN="-Wall"
[ "$STRICT" = "1" ] && WARN="$WARN -Werror"

# the game's src/ files
EXCL=" setup_amiga.c w2cfg.c main_amiga.c $W2_EXCLUDE "
GAMESRC=""
for p in "$WORK"/src/*.c; do
	f=$(basename "$p")
	[ "$SHELL_ONLY" = "1" ] && continue
	case "$f" in *_host.c|test_*.c) continue ;; esac
	case "$EXCL" in *" $f "*) continue ;; esac
	GAMESRC="$GAMESRC $f"
done
HAVE_GAME=""
for f in $GAMESRC; do
	if grep -Eq '^[[:space:]]*int[[:space:]]+game_init[[:space:]]*\([[:space:]]*(void)?[[:space:]]*\)[[:space:]]*\{?[[:space:]]*$' "$WORK/src/$f"; then HAVE_GAME="-DW2_HAVE_GAME"; fi
done

CFLAGS="$CPU $OPT $WARN -std=gnu99 -D__AMIGA__ -I$WORK/native -I$WORK/native/cgx-include -I$WORK/src"
SUM=$(printf '%s %s' "$CFLAGS" "$HAVE_GAME" | md5sum | cut -d' ' -f1)
if [ "$(cat "$WORK/flags.sum" 2>/dev/null)" != "$SUM" ]; then rm -f "$OBJ"/*.o "$OBJ"/*.d "$OBJ"/*.warn; echo "$SUM" > "$WORK/flags.sum"; fi

NWARN=0
cc() {   # cc <dir> <file> [extra flags]   (object rebuilt when the file or any header it read is newer)
	o="$OBJ/$(echo "$2" | tr / _).o"
	if [ -f "$o" ] && [ ! "$1/$2" -nt "$o" ] && [ -f "$o.d" ] && ! (sed -e 's/\\$//' -e 's/^[^:]*://' "$o.d" | tr ' ' '\n' | grep -v '^$' | while read -r d; do [ -f "$d" ] && [ "$d" -nt "$o" ] && echo x; done | grep -q x); then return 0; fi
	m68k-amigaos-gcc $CFLAGS $3 -MMD -MP -MF "$o.d" -c "$1/$2" -o "$o" 2>"$o.warn" || { cat "$o.warn"; echo "COMPILE FAILED: $2"; exit 1; }
	if [ -s "$o.warn" ]; then echo "--- warnings: $2"; cat "$o.warn"; fi
	return 0
}

for f in fp_conv.c fp_single.c fp_double.c; do cc "$WORK/native" $f -fno-builtin; done
NAT="amiga_gfx amiga_trap amiga_stack amiga_uclock amiga_audio amiga_adpcm w2_sys cellcmp"
for f in $NAT; do cc "$WORK/native" $f.c; done
for f in $GAMESRC w2cfg.c; do cc "$WORK/src" $f; done
if [ ! -f "$OBJ/c2p_glue.o" ] || [ "$WORK/native/c2p_glue.s" -nt "$OBJ/c2p_glue.o" ] || [ "$WORK/native/c2p_rect.s" -nt "$OBJ/c2p_glue.o" ] || [ "$WORK/native/c2p1x1_6_c5_bm_040.s" -nt "$OBJ/c2p_glue.o" ]; then
	vasmm68k_mot -quiet -Fhunk -m68020 -no-opt -I/opt/amiga/m68k-amigaos/ndk-include -I"$WORK/native" -o "$OBJ/c2p_glue.o" "$WORK/native/c2p_glue.s"
fi
if grep -Eiq '^[[:space:]]+f(add|sub|mul|div|move|sqrt|int|cmp|mov)' "$WORK"/native/*.s; then echo "FPU GATE: FPU mnemonic in a .s file - refusing"; exit 1; fi
rm -f "$OBJ/main_amiga.c.o"   # the built: stamp in the log must belong to THIS build
cc "$WORK/src" main_amiga.c "$HAVE_GAME"
[ -n "$HAVE_GAME" ] && echo "game hooks: from src/ (W2_HAVE_GAME)" || echo "game hooks: stub test pattern (no src/ file defines game_init yet)"

OBJS="$OBJ/main_amiga.c.o $OBJ/w2cfg.c.o"
for f in $GAMESRC; do OBJS="$OBJS $OBJ/$f.o"; done
for f in $NAT; do OBJS="$OBJS $OBJ/$f.c.o"; done
# fp_*.o BEFORE -lm: our __mulsf3/__divsf3 win over the ROM IEEE entries (Line-F on a KS 3.1 without FPU)
OBJS="$OBJS $OBJ/c2p_glue.o $OBJ/fp_conv.c.o $OBJ/fp_single.c.o $OBJ/fp_double.c.o"
m68k-amigaos-gcc $CPU -Wl,-Map,"$WORK/w2.map" -Wl,-t -o "$WORK/w2" $OBJS -lamiga -lm > "$WORK/link.txt" 2>&1 || { cat "$WORK/link.txt"; echo "LINK FAILED w2"; exit 1; }
if grep -q libm881 "$WORK/link.txt"; then echo "FPU GATE: libm881 was linked into w2 - refusing"; exit 1; fi
if grep -q "duplicate section" "$WORK/link.txt"; then grep "duplicate section" "$WORK/link.txt" | head -3; echo "WARNING: duplicate section (Hunk has no COMDAT - not cosmetic)"; fi

# w2setup: the settings tool (GadTools); no game code, no maths
cc "$WORK/src" setup_amiga.c
m68k-amigaos-gcc $CPU -Wl,-t -o "$WORK/w2setup" "$OBJ/setup_amiga.c.o" "$OBJ/w2cfg.c.o" -lamiga > "$WORK/link_setup.txt" 2>&1 || { cat "$WORK/link_setup.txt"; echo "LINK FAILED w2setup"; exit 1; }
if grep -q libm881 "$WORK/link_setup.txt"; then echo "FPU GATE: libm881 was linked into w2setup - refusing"; exit 1; fi

# jobrun: the guest-side job queue of the test harness (never shipped to players)
cc "$WORK/native" jobrun.c
m68k-amigaos-gcc $CPU -o "$WORK/jobrun" "$OBJ/jobrun.c.o" > "$WORK/link_jobrun.txt" 2>&1 || { cat "$WORK/link_jobrun.txt"; echo "LINK FAILED jobrun"; exit 1; }

deploy() {   # deploy <dir> [play]
	d=$1
	mkdir -p "$d"
	# NEVER strip (a stripped Hunk halts the machine)
	cp "$WORK/w2" "$d/w2"
	cp "$WORK/w2setup" "$d/w2setup"
	cmp -s "$WORK/jobrun" "$d/jobrun" || cp "$WORK/jobrun" "$d/jobrun"
	# Workbench icons: our own drawing, NewIcons + planar fallback, WBTOOL, 2 MB stack
	# (tools/icons/draw_icons.py -> PNG, tools/mkicon_newicons.py from the build instructions, 15-ikony-newicons.md)
	python3 "$REPO/tools/icons/draw_icons.py" "$WORK/icons" > /dev/null || { echo "ICONS FAILED"; exit 1; }
	for ic in w2 w2setup; do
		python3 -W ignore "$REPO/tools/mkicon_newicons.py" "$WORK/icons/$ic.png" "$d/$ic.info" --tool --stack 2000000 > /dev/null &&
		python3 -W ignore "$REPO/tools/mkicon_newicons.py" --check "$d/$ic.info" > /dev/null || { echo "ICONS FAILED ($ic)"; exit 1; }
	done
	# Work:run (executed by the boot disk's User-Startup): LF line endings, or AmigaDOS runs nothing
	if [ "$2" = "play" ]; then printf '; AmiW2 play guest: no job queue, Workbench comes up (double-click w2setup / w2)\nStack 2000000\n' > "$d/run.new"
	else printf 'Stack 2000000\nWork:jobrun\n' > "$d/run.new"; fi
	if cmp -s "$d/run.new" "$d/run"; then rm "$d/run.new"; else mv "$d/run.new" "$d/run"; fi
}
deploy "$DEPLOY"
if [ -n "$DEPLOY_AROS" ] && [ "$DEPLOY_AROS" != "none" ]; then deploy "$DEPLOY_AROS"; echo "also deployed to $DEPLOY_AROS"; fi
if [ -n "$DEPLOY_PLAY" ] && [ "$DEPLOY_PLAY" != "none" ]; then deploy "$DEPLOY_PLAY" play; echo "also deployed to $DEPLOY_PLAY"; fi
m68k-amigaos-size "$WORK/w2" "$WORK/w2setup" 2>/dev/null | tail -2 || true
ls -la "$WORK/w2" "$WORK/w2setup"
echo "built and deployed"
