# AI control programs - War Data entry 277 (partial, NOT decoded)

Status: a 15-minute look only. `src/ai.c` does **not** use this data; it implements its own
"land attack" behaviour (see the header comment of `src/ai.c` and docs/SPEC_RULES.md section 14).
Nothing below is verified against the original executable. Everything is little-endian, like the PUDs.

## Container

- War Data entry 277 (= PC MAINDAT.WAR 277), 9237 bytes after decompression.
- Original strings 160-191 name the skirmish AIs ("Land Attack", "Passive", "Sea Attack",
  "Air Attack") and the campaign AIs (`_Human N` / `_Orc N`); the number of offsets below fits
  "a few skirmish scripts + the campaign scripts".

## Layout seen so far

1. **Offset table** at 0x0000: 36 u16 values, every one < file size, ending with 0x2200 0x0000.
   Sorted they are 0x009a, 0x00a2, 0x017a, 0x0274, ... 0x1e9e, 0x2200: offsets of records.
   Several come in pairs 8 bytes apart (0x009a/0x00a2, 0x0374/0x037c, 0x0464/0x046c,
   0x0a62/0x0a6a, 0x0c36/0x0c3e): a short 8-byte header record followed by the body.
2. **Record start**: two u16 that are again offsets into the file, e.g. `fe 15 f8 16`
   (0x15fe, 0x16f8). 0x15fe and 0x16f8 are shared tables (0x15fe: bytes like `ff 00 52 80 86 88 ff 00 ...`,
   0x16f8: runs of `ff`). Then one byte 0/1 and a byte 0x22 or 0x44 (count / size?), `00 00`.
3. **Initial settings list**: 3-byte entries `<id:u8> <value:u16>`, ids 0x09..0x22 in the order
   09 0a 0b 0c 21 0d 0e 0f 10 11 12 14 15 17 16 19 1b 1a 1c 1d 1f 1e 20 (values mostly 0, some 1/2);
   23 entries - plausibly one per AI unit class (worker, soldier, shooter, ...).
4. **Program**: byte code. Recurring patterns:
   - `00 0d <n> 03 04 00 09 01` / `00 0f <n> 03 05 00 0a 01` - small opcodes with unit-class ids
     and counts (look like "need n of class c" / "train").
   - `02 <u16> 00 00` with u16 values 3000, 4000, 12000, 14000, 18000, 22000, 24000, 27000 - very
     likely "wait/sleep N cycles" (or a gold/time threshold).
   - `01 <u16>` near record ends (e.g. `01 5d 01`, `01 3a 02`, `01 2f 03`) - looks like a jump to a
     record-relative offset (loop back).

## Next steps (if somebody wants the real scripts)

- Find the interpreter in the Mac binary (re68k): it reads entry 277 at game start; the opcode
  switch gives the exact meaning of `00 0d`, `00 0f`, `02`, `01`.
- Map the 23 settings ids to unit classes with the campaign maps (AIPL section of the PUD selects
  the script: map 220 uses AIPL 0 and 26).
