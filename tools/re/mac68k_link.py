#!/usr/bin/env python3
"""mac68k_link.py - rebuild the runtime image of the Mac 68k Warcraft II application.

Reads the player's own copy of the game (raw resource fork 'Warcraft II.rsrc', or the CODE_n.bin /
DATA_0.bin bodies already extracted from it) and does what the application's private loader in
CODE 1 does at run time.  Contains no bytes of the original program.

  * CODE 0  : u32 above-A5 size, u32 below-A5 size (rest unused by this loader).
  * DATA 0  : u32 offset of the CODE 1 relocation lists, then 3 compressed blocks, each =
              s32 A5-relative start + opcode stream:
                 1xxxxxxx  copy (x&7f)+1 literal bytes
                 01xxxxxx  skip (x&3f)+1 bytes (left zero)
                 001xxxxx  repeat next byte (x&1f)+2 times
                 0001xxxx  store (x&0f)+1 bytes of 0xFF
                 00000000  end of block
                 00000001  8 bytes "00000000 FFFF bbbb"
                 00000010  8 bytes "00000000 FF bbbbbb"
                 00000011  8 bytes "A9F0 0000 bbbb 00bb"   (unloaded jump-table entry)
                 00000100  8 bytes "A9F0 00bbbbbb 00bb"    (unloaded jump-table entry)
              then 3 relocation lists for the A5 world (base A5; delta = A5, CODE1 base, A5),
              and at the u32 offset 3 lists for CODE 1 (base CODE1; delta = A5, CODE1, CODE1).
  * Relocation list: u32 count, then per entry a packed offset (relative to the list base):
                 1xxxxxxx           off += sext7(x)*2
                 01xxxxxx yyyyyyyy  off += sext14(xy)*2
                 00xxxxxx +3 bytes  off  = sext30(...)*2
              and *(u32*)(base+off) += delta.
  * CODE 2..23: header u16 (low word of jtoff), u16 njt, s32 jtoff (A5-relative), u32 reloc_off;
              relocation lists at reloc_off (base = segment; delta = A5, CODE1 base, segment).
  * Jump table: below A5 (A5-0x3A40 .. A5), 8-byte entries; unloaded "A9F0 <u32 off> <u16 seg>",
              loaded "4EF9 <u32 abs>", abs = segment base (resource start, header included) + off.

Output (to an out dir outside the repo): w2_image.bin (flat big-endian image at IMAGE_BASE),
w2_symbols.txt ("ADDR name kind"), w2_layout.txt (memory map), w2_jumptable.txt,
w2_traps.txt (pseudo-address -> A-line trap name, see ghidra/mac68020.slaspec).

usage: python mac68k_link.py <dir with CODE_n.bin + DATA_0.bin | resource-fork file> <outdir>
"""
import os, re, struct, sys

IMAGE_BASE = 0x00100000
SEG_STRIDE = 0x8000           # CODE n at IMAGE_BASE + n*SEG_STRIDE (largest segment < 32 KB)
A5 = 0x00210000               # A5 world: A5-below .. A5+above
TRAP_BASE = 0x00F00000        # pseudo addresses for A-line traps: TRAP_BASE + (word & 0xfff)*2


def load_resources(src):
    code, data = {}, None
    if os.path.isdir(src):
        for fn in os.listdir(src):
            if fn.startswith('CODE_') and fn.endswith('.bin'):
                code[int(fn[5:-4])] = open(os.path.join(src, fn), 'rb').read()
        data = open(os.path.join(src, 'DATA_0.bin'), 'rb').read()
    else:
        import rsrcfork   # raw resource-fork file (*.rsrc) or a file that has a resource fork
        rf = rsrcfork.open(src, fork='data' if src.lower().endswith('.rsrc') else 'auto')
        for rid, r in rf[b'CODE'].items():
            code[rid] = r.data
        data = rf[b'DATA'][0].data
    return code, data


def s32(v):
    return v - (1 << 32) if v & 0x80000000 else v


def decompress(data, p, mem, idx):
    """three blocks into mem (bytearray); idx maps an A5 offset to a mem index; returns new p"""
    for _ in range(3):
        i = idx(s32(struct.unpack_from('>I', data, p)[0])); p += 4
        while True:
            b = data[p]; p += 1
            if b & 0x80:
                n = (b & 0x7f) + 1; mem[i:i + n] = data[p:p + n]; i += n; p += n
            elif b & 0x40:
                i += (b & 0x3f) + 1
            elif b & 0x20:
                n = (b & 0x1f) + 2; mem[i:i + n] = bytes([data[p]]) * n; i += n; p += 1
            elif b & 0x10:
                n = (b & 0x0f) + 1; mem[i:i + n] = b'\xff' * n; i += n
            elif b == 0:
                break
            elif b == 1:
                mem[i + 4:i + 8] = b'\xff\xff' + data[p:p + 2]; p += 2; i += 8
            elif b == 2:
                mem[i + 4:i + 8] = b'\xff' + data[p:p + 3]; p += 3; i += 8
            elif b == 3:
                mem[i:i + 2] = b'\xa9\xf0'; mem[i + 4:i + 6] = data[p:p + 2]; mem[i + 7] = data[p + 2]
                p += 3; i += 8
            elif b == 4:
                mem[i:i + 2] = b'\xa9\xf0'; mem[i + 3:i + 6] = data[p:p + 3]; mem[i + 7] = data[p + 3]
                p += 4; i += 8
            else:
                raise ValueError('DATA 0: bad opcode %02x at %x' % (b, p - 1))
    return p


def reloc_list(stream, p):
    """returns (offsets, new p)"""
    n = struct.unpack_from('>I', stream, p)[0]; p += 4
    off, out = 0, []
    for _ in range(n):
        b = stream[p]; p += 1
        if b & 0x80:
            v = (b << 1) & 0xff
            off += v - 256 if v & 0x80 else v
        elif b & 0x40:
            w = (((b << 8) | stream[p]) << 2) & 0xffff; p += 1
            off += (w - 0x10000 if w & 0x8000 else w) >> 1
        else:
            l = (struct.unpack_from('>I', stream, p - 1)[0] << 2) & 0xffffffff; p += 3
            off = s32(l) >> 1
        out.append(off)
    return out, p


def main():
    src, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    code, data = load_resources(src)
    above, below = struct.unpack_from('>II', code[0], 0)
    lo, hi = A5 - below, A5 + above
    end = max(hi, IMAGE_BASE + SEG_STRIDE * (max(code) + 1))
    img = bytearray(end - IMAGE_BASE)
    syms = []   # (addr, name, kind)

    def ix(addr):
        return addr - IMAGE_BASE

    def addl(addr, delta):
        a = ix(addr)
        struct.pack_into('>I', img, a, (struct.unpack_from('>I', img, a)[0] + delta) & 0xffffffff)

    segbase = {n: IMAGE_BASE + n * SEG_STRIDE for n in code if n > 0}
    for n, b in code.items():
        if n == 0:
            continue
        assert len(b) <= SEG_STRIDE
        img[ix(segbase[n]):ix(segbase[n]) + len(b)] = b
        syms.append((segbase[n], 'CODE_%d' % n, 'segment'))
        # MacsBug symbols: rts / jmp (a0) + (0x80|len) + name -> names the enclosing function
        for m in re.finditer(rb'(?:\x4e\x75|\x4e\xd0)([\x82-\x9f])', b):
            ln = m.group(1)[0] & 0x7f
            nm = b[m.end():m.end() + ln]
            if re.fullmatch(rb'[A-Za-z_][A-Za-z0-9_.:~]*', nm):
                syms.append((segbase[n] + m.start(), nm.decode(), 'macsbug'))

    # A5 world + CODE 1
    c1 = segbase[1]
    p = decompress(data, 4, img, lambda off: ix(A5 + off))
    for delta in (A5, c1, A5):
        offs, p = reloc_list(data, p)
        for o in offs:
            addl(A5 + o, delta)
    p = struct.unpack_from('>I', data, 0)[0]
    for delta in (A5, c1, c1):
        offs, p = reloc_list(data, p)
        for o in offs:
            addl(c1 + o, delta)

    # segments 2..n: relocate and put their jump-table entries into the "loaded" state
    jt = []
    for n in sorted(code):
        if n < 2:
            continue
        b = code[n]; base = segbase[n]
        _, njt, jto, roff = struct.unpack_from('>HHiI', b, 0)
        q = roff
        for delta in (A5, c1, base):
            offs, q = reloc_list(b, q)
            for o in offs:
                addl(base + o, delta)
        syms.append((base + roff, 'CODE_%d_relocs' % n, 'data'))
        for k in range(njt):
            e = A5 + jto + 8 * k
            w0, off, sn = struct.unpack_from('>HIH', img, ix(e))
            if w0 != 0xA9F0 or sn != n:
                print('warn: JT entry %x: %04x %x seg %d (expected %d)' % (e, w0, off, sn, n))
            tgt = base + off
            struct.pack_into('>HI', img, ix(e), 0x4EF9, tgt)
            jt.append((e, tgt, n, k))
            syms.append((e, 'JT_m%04X' % (A5 - e), 'jt'))
            syms.append((tgt, 'f%d_%04X' % (n, off), 'func'))
    for o, nm in ((4, 'CODE1_start'), (0x10e, 'CODE1_DecompressData'), (0x212, 'CODE1_RelocList'),
                  (0x286, 'CODE1_Relocate3'), (0x2b2, 'CODE1_Unrelocate3'),
                  (0x32a, 'CODE1_LoadSegPatch'), (0x3f0, 'CODE1_UnloadSegPatch'),
                  (0x4de, 'CODE1_mul32'), (0x510, 'CODE1_switch_helper')):
        syms.append((c1 + o, nm, 'func'))
    syms.append((A5, 'A5_WORLD', 'data'))
    syms.append((min(e for e, *_ in jt), 'JUMP_TABLE', 'data'))

    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from mac_traps import trap_name
    with open(os.path.join(outdir, 'w2_traps.txt'), 'w') as f:
        for w in range(0xA000, 0xB000):
            f.write('%08X %s\n' % (TRAP_BASE + (w & 0xfff) * 2, trap_name(w)))
    open(os.path.join(outdir, 'w2_image.bin'), 'wb').write(img)
    with open(os.path.join(outdir, 'w2_symbols.txt'), 'w') as f:
        for a, nm, k in sorted(set(syms)):
            f.write('%08X %s %s\n' % (a, nm, k))
    with open(os.path.join(outdir, 'w2_layout.txt'), 'w') as f:
        f.write('IMAGE_BASE %08X\nIMAGE_SIZE %08X\nA5 %08X\nA5_LO %08X\nA5_HI %08X\nTRAP_BASE %08X\n'
                % (IMAGE_BASE, len(img), A5, lo, hi, TRAP_BASE))
        for n in sorted(segbase):
            f.write('SEG %d %08X %08X\n' % (n, segbase[n], len(code[n])))
    with open(os.path.join(outdir, 'w2_jumptable.txt'), 'w') as f:
        for e, t, n, k in jt:
            f.write('%08X A5-%04X -> %08X seg %d #%d\n' % (e, A5 - e, t, n, k))
    print('image %08X..%08X, A5=%08X world %08X..%08X, %d JT entries, entry %08X'
          % (IMAGE_BASE, IMAGE_BASE + len(img), A5, lo, hi, len(jt), jt[0][1]))


if __name__ == '__main__':
    main()
