#!/bin/sh
# release.sh - builds the two release archives OUTSIDE the repo (run through build/w.sh in WSL):
#   wsl -d Ubuntu-22.04 sh /mnt/d/GITHUB/Ami_Warcraft2/build/w.sh build/release.sh [outdir]
#     <out>/Warcraft2_68K-<ver>.lha            Amiga drawer: w2, w2setup, icons, README, LICENSE
#     <out>/Warcraft2_68K-<ver>-converter.zip  PC data extractor: w2conv.exe, w2conv-linux, extract.bat, README.txt
# NO game data: build/release-gate.py refuses the trees if anything original or derived is inside.
# The emulator guests are not touched (the build deploys into a private staging folder).
set -e
cd "$(dirname "$0")/.."
REPO=$(pwd)
VER=$(sed -n 's/^#define W2_VERSION "\(.*\)"/\1/p' src/version.h)
[ -n "$VER" ] || { echo "release: no version in src/version.h"; exit 1; }
OUT="${1:-/mnt/d/GITHUB/_AmiW2-poza-repo/dist/release-$VER}"
STAGE=$HOME/w2-release
rm -rf "$STAGE" "$OUT"
mkdir -p "$STAGE/build" "$STAGE/Warcraft2_68K" "$STAGE/conv/Warcraft2_68K-converter" "$OUT"

# 1. the Amiga binaries + icons (never stripped)
DEPLOY="$STAGE/build" DEPLOY_AROS=none DEPLOY_PLAY=none sh build/build.sh > "$STAGE/build.log" 2>&1 || { tail -20 "$STAGE/build.log"; echo "release: BUILD FAILED"; exit 1; }
tail -1 "$STAGE/build.log" | grep -q 'built and deployed' || { echo "release: build did not finish"; exit 1; }
if grep -q 'libm881' "$STAGE/build.log"; then echo "release: libm881 linked (FPU code)"; exit 1; fi
D="$STAGE/Warcraft2_68K"
for f in w2 w2.info w2setup w2setup.info; do cp "$STAGE/build/$f" "$D/$f"; done
strings -a "$D/w2" | grep -q "Warcraft II 68K" || { echo "release: w2 has no name string"; exit 1; }
strings -a "$D/w2" | grep -q "^$VER\$\|$VER" || { echo "release: w2 does not carry version $VER"; exit 1; }
sed 's/\r$//' README.md > "$D/README.md"
cp LICENSE "$D/LICENSE"
python3 -W ignore tools/mkicon_newicons.py "$HOME/w2-build/icons/w2.png" "$STAGE/Warcraft2_68K.info" --drawer > /dev/null
python3 -W ignore tools/mkicon_newicons.py --check "$STAGE/Warcraft2_68K.info" > /dev/null

# 2. the PC converter
sh build/build-w2conv.sh > "$STAGE/conv.log" 2>&1 || { tail -20 "$STAGE/conv.log"; echo "release: W2CONV BUILD FAILED"; exit 1; }
C="$STAGE/conv/Warcraft2_68K-converter"
cp build/host/w2conv.exe "$C/w2conv.exe"
cp build/host/w2conv "$C/w2conv-linux"
for f in extract.bat README.txt; do sed 's/\r$//; s/$/\r/' "tools/w2conv/$f" > "$C/$f"; done

# 3. gate: nothing original, nothing derived
python3 build/release-gate.py "$STAGE/Warcraft2_68K" "$STAGE/conv" "$STAGE/Warcraft2_68K.info"

# 4. archives + control unpack
LHA="$OUT/Warcraft2_68K-$VER.lha"
ZIP="$OUT/Warcraft2_68K-$VER-converter.zip"
( cd "$STAGE" && lha ao5 "$LHA" Warcraft2_68K Warcraft2_68K.info > /dev/null )
( cd "$STAGE/conv" && zip -rq "$ZIP" Warcraft2_68K-converter )
mkdir -p "$STAGE/chk/lha" "$STAGE/chk/zip"
( cd "$STAGE/chk/lha" && lha xq "$LHA" )
( cd "$STAGE/chk/zip" && unzip -q "$ZIP" )
( cd "$STAGE" && find Warcraft2_68K Warcraft2_68K.info -type f | sort | xargs md5sum ) > "$STAGE/a.md5"
( cd "$STAGE/chk/lha" && find Warcraft2_68K Warcraft2_68K.info -type f | sort | xargs md5sum ) > "$STAGE/b.md5"
cmp "$STAGE/a.md5" "$STAGE/b.md5" || { echo "release: LHA CONTENT DIFFERS"; exit 1; }
( cd "$STAGE/conv" && find . -type f | sort | xargs md5sum ) > "$STAGE/c.md5"
( cd "$STAGE/chk/zip" && find . -type f | sort | xargs md5sum ) > "$STAGE/d.md5"
cmp "$STAGE/c.md5" "$STAGE/d.md5" || { echo "release: ZIP CONTENT DIFFERS"; exit 1; }
echo "--- lha"; lha l "$LHA" | tail -n +3
echo "--- zip"; unzip -l "$ZIP" | tail -n +4
ls -la "$OUT"
echo "release $VER ready in $OUT"
