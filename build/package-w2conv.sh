#!/bin/sh
# Makes the Windows release folder of the data extractor OUTSIDE the repo:
#   <dist>/w2conv.exe, extract.bat, README.txt  (text files with CRLF line ends)
# Run inside WSL:  sh build/package-w2conv.sh [dist-dir]
# default dist-dir: /mnt/d/GITHUB/_AmiW2-poza-repo/dist/w2conv
# Builds w2conv first (build/build-w2conv.sh). Contains no game data.
set -e
cd "$(dirname "$0")/.."
DIST="${1:-/mnt/d/GITHUB/_AmiW2-poza-repo/dist/w2conv}"
sh build/build-w2conv.sh
[ -f build/host/w2conv.exe ] || { echo "package-w2conv: no w2conv.exe (mingw missing?)" >&2; exit 1; }
mkdir -p "$DIST"
rm -f "$DIST/w2conv.exe" "$DIST/extract.bat" "$DIST/README.txt"
cp build/host/w2conv.exe "$DIST/w2conv.exe"
for f in extract.bat README.txt; do
	sed 's/\r$//; s/$/\r/' "tools/w2conv/$f" > "$DIST/$f"
done
ls -la "$DIST"
echo "package-w2conv: $DIST ready (zip it as it is)"
