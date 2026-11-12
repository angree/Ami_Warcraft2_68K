#!/usr/bin/env python3
"""release-gate.py - refuse a tree that holds any original Warcraft II file or anything made from one.

    python3 build/release-gate.py <dir> [<dir> ...]      exit 0 = clean, 1 = refused
    python3 build/release-gate.py --git                   checks every file git tracks / would add

The port ships only our own code and free (CC0) material; the player converts their own CD with
w2conv. A file is refused when it
  1. is a .WAR archive (u32 LE 0x19 or 0x18 magic + a plausible offset table) whatever it is called,
  2. starts with a magic of something we DERIVE from the original: W2LP (lores / EHB art pack),
     "RIFF....WAVE" inside a data tree, SMK2 (Smacker movie), the .adp music header (see MAGICS),
  3. has a name only original / converted data has (War Data, MAINDAT.WAR, *.pud, *.adp, *.pack,
     *.smk, CD images, the Ghidra output),
  4. is larger than 4 MB (nothing of ours is; a CD image or an archive is).
The model is AmiEU2's build/release-gate.py.
"""
import os
import re
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAGICS = (b"W2LP", b"SMK2", b"SMK4", b"W2AD")
BAD_NAMES = re.compile(
    r"^(war (data|resources|strings|sounds|music|movies)(\.rsrc)?|title song|"
    r"(maindat|rezdat|strdat|snddat)\.war|(sfxdat|muddat)\.(sud|cud)|conversion\.txt|"
    r".*\.(pud|adp|pack|smk|war|sud|cud|iso|bin|cue|toast|img|hfs|gzf|gpr|rep))$", re.I)
MAX_SIZE = 4 * 1024 * 1024


def is_war(head, size):
    if len(head) < 12:
        return False
    magic, count, _ = struct.unpack_from("<IHH", head, 0)
    if magic not in (0x19, 0x18) or not (1 <= count <= 4096) or size < 8 + 4 * count:
        return False
    first = struct.unpack_from("<I", head, 8)[0]
    return first == 8 + 4 * count


def check(files):
    bad = []
    for f in files:
        name = os.path.basename(f)
        try:
            size = os.path.getsize(f)
            with open(f, "rb") as fh:
                head = fh.read(16)
        except OSError as e:
            bad.append((f, "unreadable: %s" % e))
            continue
        if BAD_NAMES.match(name):
            bad.append((f, "name of original / converted data"))
        elif is_war(head, size):
            bad.append((f, "WAR archive"))
        elif head[:4] in MAGICS:
            bad.append((f, "converted data (%s)" % head[:4].decode("latin-1")))
        elif head[:4] == b"RIFF" and head[8:12] == b"WAVE":
            bad.append((f, "WAV sound"))
        elif size > MAX_SIZE:
            bad.append((f, "larger than 4 MB (%d bytes)" % size))
    return bad


def main():
    args = sys.argv[1:]
    if not args:
        sys.exit(__doc__)
    files = []
    if args == ["--git"]:
        out = subprocess.check_output(["git", "-C", ROOT, "ls-files", "-z", "--cached", "--others", "--exclude-standard"])
        files = [os.path.join(ROOT, p) for p in out.decode("utf-8", "replace").split("\0") if p]
    else:
        for d in args:
            if os.path.isfile(d):
                files.append(d)
            for dp, _, fn in os.walk(d):
                files += [os.path.join(dp, n) for n in fn]
    bad = check(files)
    for f, why in bad:
        print("REFUSED %s: %s" % (f, why))
    print("release-gate: %d files, %d refused" % (len(files), len(bad)))
    sys.exit(1 if bad or not files else 0)


if __name__ == "__main__":
    main()
