#!/usr/bin/env python3
"""PNG -> Workbench icon (.info): a NewIcons image (own palette, remapped by the system) + a planar fallback.

    python3 mkicon_newicons.py <in.png> <out.info> [--tool|--drawer|--project] [--stack 2000000]
                                [--selected <in_selected.png>] [--default-tool PROG]
    python3 mkicon_newicons.py --check <file.info>     # decode the NewIcons image back, print it as text

Rules (see ../15-ikony-newicons.md - each one was learned on a real Workbench):
- PNG alpha < 128 = transparent (NewIcons 'B': pen 0 transparent). At most 32 colours besides transparency
  (the palette must fit the first IM1= line; longer palettes NIEPEWNE). Up to 64x64 is safe; 36x40 is the grid.
- Every IMn= line holds whole 7-bit characters AND whole pixels (groups of 7 pixels = bpp characters), so it
  decodes the same in readers that carry bits across lines and in readers that do not.
- Selected image: given, or the normal one lightened by 1/3 (never Workbench's plane negation).
- Planar fallback (Workbench without NewIcons): depth 3, PlanePick 7, nearest of the stock 3.x pens
  1 black / 2 white / 3 blue, transparent -> pen 0.
- do_StackSize: 2000000 by default for tools (the stack of a program started from Workbench comes from here).
"""
import struct
import sys

STOCK = [(0x95, 0x95, 0x95), (0, 0, 0), (255, 255, 255), (0x3b, 0x67, 0xa2)]


def enc7(v):
    return chr(v + 0x20) if v < 0x50 else chr(v - 0x50 + 0xA1)


def dec7(c):
    o = ord(c)
    if 0x20 <= o <= 0x6F:
        return o - 0x20
    if 0xA1 <= o <= 0xD0:
        return o - 0xA1 + 0x50
    return None


def bits_to_chars(bits):
    bits = list(bits)
    while len(bits) % 7:
        bits.append(0)
    out = []
    for i in range(0, len(bits), 7):
        v = 0
        for b in bits[i:i + 7]:
            v = (v << 1) | b
        out.append(enc7(v))
    return ''.join(out)


def newicon_lines(pix, w, h, pal, prefix):
    """pix: list of palette indices (0 = transparent), pal: list of RGB incl. index 0."""
    n = len(pal)
    bpp = max(1, (n - 1).bit_length())
    head = 'B' + chr(w + 0x21) + chr(h + 0x21) + chr((n >> 6) + 0x21) + chr((n & 63) + 0x21)
    pbits = []
    for rgb in pal:
        for c in rgb:
            pbits += [(c >> (7 - k)) & 1 for k in range(8)]
    first = head + bits_to_chars(pbits)
    if len(prefix) + len(first) > 127:
        raise SystemExit("mkicon: %d colours do not fit the first line - use fewer" % (n - 1))
    lines = [first]
    groups = (123 - len(prefix)) // bpp                 # 7 pixels = bpp characters
    per_line = 7 * groups
    for i in range(0, len(pix), per_line):
        bits = []
        for p in pix[i:i + per_line]:
            bits += [(p >> (bpp - 1 - k)) & 1 for k in range(bpp)]
        lines.append(bits_to_chars(bits))
    return [prefix + l for l in lines]


def load_png(path):
    from PIL import Image
    im = Image.open(path).convert('RGBA')
    w, h = im.size
    if w > 93 or h > 93:
        raise SystemExit("mkicon: %dx%d is too big for NewIcons (one character per dimension)" % (w, h))
    px = list(im.getdata())
    opaque = Image.new('RGB', (w, h))
    opaque.putdata([(r, g, b) for r, g, b, a in px])
    q = opaque.quantize(colors=32, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    qpal = q.getpalette()[:32 * 3]
    used = sorted(set(c for c, (r, g, b, a) in zip(q.getdata(), px) if a >= 128))   # transparent pixels take no colour
    remap = {old: i + 1 for i, old in enumerate(used)}
    pal = [(0, 0, 0)] + [tuple(qpal[3 * u:3 * u + 3]) for u in used]
    idx = [0 if a < 128 else remap[c] for c, (r, g, b, a) in zip(q.getdata(), px)]
    return w, h, idx, pal


def planar_image(w, h, idx, pal):
    def nearest(rgb):
        return min(range(1, 4), key=lambda k: sum((a - b) ** 2 for a, b in zip(rgb, STOCK[k])))
    p = [0 if i == 0 else nearest(pal[i]) for i in idx]
    words = (w + 15) // 16
    data = bytearray()
    for plane in range(3):
        for y in range(h):
            bits = 0
            for x in range(w):
                bits = (bits << 1) | ((p[y * w + x] >> plane) & 1)
            data += (bits << (words * 16 - w)).to_bytes(words * 2, 'big')
    return struct.pack('>hhhhhIBBI', 0, 0, w, h, 3, 1, 7, 0, 0) + bytes(data)


def make_info(png, sel_png, kind, stack, deftool):
    w, h, idx, pal = load_png(png)
    if sel_png:
        w2, h2, idx2, pal2 = load_png(sel_png)
        if (w2, h2) != (w, h):
            raise SystemExit("mkicon: the selected image must have the same size")
    else:
        idx2, pal2 = idx, [(0, 0, 0)] + [tuple(min(255, c + (255 - c) // 3) for c in rgb) for rgb in pal[1:]]
    tt = ["*** DON'T EDIT THE FOLLOWING LINES!! ***"] + newicon_lines(idx, w, h, pal, "IM1=") + newicon_lines(idx2, w, h, pal2, "IM2=")
    dtype = {'--tool': 3, '--project': 4, '--drawer': 2}[kind]
    gadget = struct.pack('>IhhhhHHHIIIiIHI', 0, 0, 0, w, h, 0x0006, 0x0003, 1, 1, 1, 0, 0, 0, 0, 0)
    head = struct.pack('>HH', 0xE310, 1) + gadget + struct.pack(
        '>BBIIIIIII', dtype, 0, 1 if deftool else 0, 1, 0x80000000, 0x80000000, 1 if dtype == 2 else 0, 0, stack)
    out = head
    if dtype == 2:                                   # DrawerData (OS 1.x part, 56 bytes): a window at 50,50 300x150
        nw = struct.pack('>hhhhBBIIIIIIIhhHHH', 50, 50, 300, 150, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1)
        out += nw + struct.pack('>ii', 0, 0)
    out += planar_image(w, h, idx, pal) + planar_image(w, h, idx2, pal2)
    if deftool:
        b = deftool.encode('latin-1') + b'\0'
        out += struct.pack('>I', len(b)) + b
    out += struct.pack('>I', (len(tt) + 1) * 4)
    for t in tt:
        b = t.encode('latin-1') + b'\0'
        out += struct.pack('>I', len(b)) + b
    return out, w, h, len(pal) - 1


def check(path):
    """Independent reader: finds IM1= tooltypes, decodes header, palette and image, prints the picture."""
    d = open(path, 'rb').read()
    tts = [t.decode('latin-1') for t in d.split(b'\0')]
    lines = []
    for t in tts:
        k = t.find('IM1=')
        if k >= 0:
            lines.append(t[k + 4:])
    if not lines:
        raise SystemExit("no IM1= tooltypes")
    h0 = lines[0]
    w, h = ord(h0[1]) - 0x21, ord(h0[2]) - 0x21
    n = ((ord(h0[3]) - 0x21) << 6) | (ord(h0[4]) - 0x21)
    bpp = max(1, (n - 1).bit_length())
    pbits = []
    for c in h0[5:]:
        v = dec7(c)
        pbits += [(v >> (6 - k)) & 1 for k in range(7)]
    pal = [tuple(int(''.join(map(str, pbits[(i * 3 + j) * 8:(i * 3 + j) * 8 + 8])), 2) for j in range(3)) for i in range(n)]
    pix = []
    for l in lines[1:]:
        bits = []
        for c in l:
            v = dec7(c)
            if v is None:
                raise SystemExit("bad character %r" % c)
            bits += [(v >> (6 - k)) & 1 for k in range(7)]
        for i in range(0, len(bits) - bpp + 1, bpp):
            pix.append(int(''.join(map(str, bits[i:i + bpp])), 2))
    pix = pix[:w * h]
    print("%s: NewIcons %s %dx%d, %d colours (%d bits), %d IM1 lines, %d pixels decoded" % (
        path, 'transparent' if h0[0] == 'B' else 'opaque', w, h, n, bpp, len(lines), len(pix)))
    chars = " .:-=+*#%@ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    for y in range(h):
        print('  |' + ''.join(chars[p] if p < len(chars) else '?' for p in pix[y * w:(y + 1) * w]) + '|')
    if len(pix) != w * h:
        raise SystemExit("PIXEL COUNT WRONG")


def main():
    a = sys.argv[1:]
    if len(a) == 2 and a[0] == '--check':
        return check(a[1])
    if len(a) < 2:
        raise SystemExit(__doc__)
    png, out = a[0], a[1]
    kind, stack, sel, deftool = '--tool', 2000000, None, None
    i = 2
    while i < len(a):
        if a[i] in ('--tool', '--drawer', '--project'): kind = a[i]
        elif a[i] == '--stack': i += 1; stack = int(a[i])
        elif a[i] == '--selected': i += 1; sel = a[i]
        elif a[i] == '--default-tool': i += 1; deftool = a[i]
        else: raise SystemExit("unknown option %s" % a[i])
        i += 1
    data, w, h, n = make_info(png, sel, kind, stack, deftool)
    open(out, 'wb').write(data)
    print("mkicon: %s %dx%d, %d colours, %d bytes, stack %d" % (out, w, h, n, len(data), stack))


if __name__ == '__main__':
    main()
