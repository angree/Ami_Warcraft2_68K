#!/bin/sh
# Builds the player's data converter w2conv (PC side): Linux build/host/w2conv and, when the mingw
# cross compiler is installed, Windows build/host/w2conv.exe. Run inside WSL:
#   sh build/build-w2conv.sh
# The lores / EHB pack step links the game's own modules (src/lores.c, ehb.c, assets.c, war.c,
# w2gfx_decode.c, w2cfg.c, w2_tables.c, font35.c) unchanged; tools/w2conv/lores_pack.c is their platform shim.
set -e
cd "$(dirname "$0")/.."
mkdir -p build/host
SRC="tools/w2conv/w2conv.c tools/w2conv/cdimage.c tools/w2conv/hfs.c tools/w2conv/adpcm_enc.c tools/w2conv/lores_pack.c"
GAME="src/lores.c src/ehb.c src/assets.c src/war.c src/w2gfx_decode.c src/w2cfg.c src/w2_tables.c src/font35.c"
CF="-O2 -Wall -Wextra -Wno-unused-parameter -std=gnu99 -Isrc -Inative"
GF="-O2 -std=gnu99 -Isrc -Inative -w"   # the game's files: built as they are, their warnings are not ours
build() {   # build <cc> <out> [ldflags]
	cc="$1"; out="$2"; shift 2
	obj="build/host/w2conv-obj-$cc"
	rm -rf "$obj"; mkdir -p "$obj"
	for f in $GAME; do $cc $GF -c "$f" -o "$obj/$(basename "$f" .c).o"; done
	$cc $CF -o "$out" $SRC "$obj"/*.o "$@"
	rm -rf "$obj"
}
build gcc build/host/w2conv -lm
if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
	build x86_64-w64-mingw32-gcc build/host/w2conv.exe -static -lm
	ls -la build/host/w2conv.exe
else
	echo "build-w2conv: x86_64-w64-mingw32-gcc not found - Windows build skipped"
fi
ls -la build/host/w2conv
echo "w2conv built"
