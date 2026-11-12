#!/bin/sh
# w.sh - run a script inside WSL with the drives mounted and the Amiga toolchain on PATH.
#   wsl -d Ubuntu-22.04 sh /mnt/d/GITHUB/Ami_Warcraft2/build/w.sh build/build.sh [args]
# WSL1 drops drvfs mounts now and then ("No such file or directory" on the script): remount first.
# Inline commands through PowerShell / Git Bash get their quoting mangled - always put them in a file.
for d in c d i; do
	ls /mnt/$d >/dev/null 2>&1 && [ -n "$(ls -A /mnt/$d 2>/dev/null)" ] && continue
	D=$(echo $d | tr a-z A-Z)
	sudo -n mkdir -p /mnt/$d 2>/dev/null; sudo -n mount -t drvfs $D: /mnt/$d 2>/dev/null
done
export PATH=/opt/amiga/bin:$PATH
cd /mnt/d/GITHUB/Ami_Warcraft2 || exit 99
s="$1"; shift
sh "$s" "$@"
