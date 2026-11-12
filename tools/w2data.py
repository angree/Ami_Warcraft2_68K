#!/usr/bin/env python3
"""w2data.py - reference reader for Warcraft II data archives (PC .WAR / Mac "War *" files).

Our own implementation written from the format description in docs/DATA_FORMAT.md.
Pure Python 3, no third-party modules (PNG written with zlib).

Commands:
  list    <archive>                          entry table (size, flags, guessed type)
  extract <archive> <idx> <out>              decompressed entry bytes
  png     <archive> <idx> <out.png> [--palette ARCH:IDX] [--frame N] [--sheet]
                                             render sprite / image / cursor / font /
                                             tileset (give mega entry; uses mega+1 = mini)
  tileset <archive> <pal> <mega> <mini> <out.png>
                                             all megatiles, 16 per row
  pud     <file | archive:idx> [--png out.png --archive WARDATA]
                                             PUD summary (+ optional minimap/full map render)
  strings <archive> <idx>                    dump a string table / text entry
  scan    <archive>                          list with type details (frames, sizes...)

Never write rendered original data into the repository - use a scratch dir outside it.
"""
import os
import struct
import sys
import zlib

# ---------------------------------------------------------------------------
# Archive
# ---------------------------------------------------------------------------

ARCHIVE_MAGIC = 0x19


class WarArchive:
    def __init__(self, path):
        self.path = path
        with open(path, 'rb') as f:
            self.data = f.read()
        magic, self.count, self.id = struct.unpack_from('<IHH', self.data, 0)
        if magic != ARCHIVE_MAGIC:
            raise ValueError('%s: bad magic %08x' % (path, magic))
        self.offsets = list(struct.unpack_from('<%dI' % self.count, self.data, 8))

    def raw_header(self, idx):
        """(offset, unpacked size, flags). Placeholder entries (less than 4 bytes
        before the next entry / EOF) report size 0, flags 0xFF."""
        off = self.offsets[idx]
        if self.stored_size(idx) < 4:
            return off, 0, 0xFF
        word = struct.unpack_from('<I', self.data, off)[0]
        return off, word & 0xFFFFFF, word >> 24

    def stored_size(self, idx):
        """Bytes the entry occupies in the file (incl. 4-byte header)."""
        off = self.offsets[idx]
        nxt = len(self.data)
        for o in self.offsets:
            if off < o < nxt:
                nxt = o
        return nxt - off

    def load(self, idx):
        off, size, flags = self.raw_header(idx)
        src = off + 4
        if flags == 0xFF:
            return b''
        if flags == 0x00:
            return self.data[src:src + size]
        if flags != 0x20:
            raise ValueError('entry %d: unknown flags %02x' % (idx, flags))
        return decompress(self.data, src, size)


def decompress(data, pos, size):
    """LZSS: 4 KiB ring (zero filled, write pos 0), flag byte LSB first,
    bit=1 literal byte, bit=0 u16le ref: low 12 bits = absolute ring position,
    high 4 bits + 3 = length (3..18)."""
    out = bytearray()
    ring = bytearray(4096)
    rp = 0
    while len(out) < size:
        flags = data[pos]
        pos += 1
        for _ in range(8):
            if len(out) >= size:
                break
            if flags & 1:
                b = data[pos]
                pos += 1
                out.append(b)
                ring[rp] = b
                rp = (rp + 1) & 0xFFF
            else:
                ref = data[pos] | (data[pos + 1] << 8)
                pos += 2
                src = ref & 0xFFF
                n = (ref >> 12) + 3
                while n and len(out) < size:
                    b = ring[src]
                    src = (src + 1) & 0xFFF
                    out.append(b)
                    ring[rp] = b
                    rp = (rp + 1) & 0xFFF
                    n -= 1
            flags >>= 1
    return bytes(out)


# ---------------------------------------------------------------------------
# Type guessing
# ---------------------------------------------------------------------------

MAP_GROUPS = 0x9E
MAP_REC = 42


def is_palette(b):
    return len(b) == 768 and max(b) < 64


def sprite_info(b):
    """Return (count, maxw, maxh, frames, kind) if b parses as a sprite, else None.
    kind = 'sprite' (RLE rows) or 'gfu' (raw w*h rows). Same rules as w2_sprite_probe()."""
    if len(b) < 6:
        return None
    count, mw, mh = struct.unpack_from('<HHH', b, 0)
    if count == 0 or mw == 0 or mh == 0 or 6 + 8 * count > len(b):
        return None
    frames = []
    rle_ok = raw_ok = True
    nonempty = 0
    for i in range(count):
        x, y, w, h, off = struct.unpack_from('<BBBBI', b, 6 + 8 * i)
        if off & 0x80000000:
            w += 256
            off &= 0x7FFFFFFF
        if x + w > mw or y + h > mh or off > len(b):
            return None
        frames.append((x, y, w, h, off))
        if w == 0 or h == 0:
            continue
        nonempty += 1
        if off + 2 * h > len(b) or struct.unpack_from('<H', b, off)[0] != 2 * h:
            rle_ok = False
        if off + w * h > len(b):
            raw_ok = False
    if nonempty and rle_ok:
        return count, mw, mh, frames, 'sprite'
    if raw_ok:
        return count, mw, mh, frames, 'gfu'
    return None


def guess(b):
    if len(b) == 0:
        return 'empty', 'placeholder'
    if b[:4] == b'RIFF':
        return 'wav', ''
    if b[:4] == b'TYPE' and b[8:16] == b'WAR2 MAP':
        return 'pud', ''
    if b[:5] == b'FONT ':
        return 'font', 'chars 32..%d max %dx%d' % (b[5], b[6], b[7])
    if b[:4] in (b'SMK2', b'SMK4'):
        return 'smacker', ''
    if b[:4] == b'FORM':
        return 'iff', b[8:12].decode('latin1')
    if is_palette(b):
        return 'palette', ''
    if len(b) == MAP_GROUPS * MAP_REC:
        return 'tilemap', ''
    if len(b) >= 4:
        w, h = struct.unpack_from('<HH', b, 0)
        if w and h and 4 + w * h == len(b):
            return 'image', '%dx%d' % (w, h)
    if len(b) >= 8:
        hx, hy, w, h = struct.unpack_from('<HHHH', b, 0)
        if w and h and 8 + w * h == len(b) and w <= 64 and h <= 64:
            return 'cursor', '%dx%d hot %d,%d' % (w, h, hx, hy)
    info = sprite_info(b)
    if info:
        return info[4], '%d frames %dx%d' % (info[0], info[1], info[2])
    if len(b) % 64 == 0 and len(b) >= 64 * 64:
        return 'mini?', '%d minitiles' % (len(b) // 64)
    if len(b) % 32 == 0:
        return 'mega?', '%d megatiles' % (len(b) // 32)
    printable = sum(1 for c in b[:200] if 32 <= c < 127 or c in (9, 10, 13))
    if printable > 0.9 * min(200, len(b)):
        return 'text', repr(b[:40].decode('latin1'))
    if len(b) >= 4:
        n = struct.unpack_from('<H', b, 0)[0]
        if 0 < n < 2000 and 2 * n <= len(b):
            offs = struct.unpack_from('<%dH' % n, b, 0)
            if all(offs[i] <= len(b) for i in range(1, n)):
                return 'strtab?', '%d strings' % n
    return 'data', ''


# ---------------------------------------------------------------------------
# Palette
# ---------------------------------------------------------------------------

def palette_rgb(p):
    """6-bit VGA palette (768 bytes) -> list of (r,g,b) 8-bit. (v<<2)|(v>>4) scales 63->255."""
    return [tuple(((p[i * 3 + c] & 63) << 2) | ((p[i * 3 + c] & 63) >> 4) for c in range(3))
            for i in range(256)]


# ---------------------------------------------------------------------------
# PNG writer (indexed, with optional transparent index)
# ---------------------------------------------------------------------------

def write_png(path, w, h, pixels, pal, transparent=None):
    def chunk(tag, data):
        c = struct.pack('>I', len(data)) + tag + data
        return c + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        raw += pixels[y * w:(y + 1) * w]
    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 3, 0, 0, 0))
    png += chunk(b'PLTE', b''.join(bytes(c) for c in pal))
    if transparent is not None:
        t = bytearray([255] * 256)
        t[transparent] = 0
        png += chunk(b'tRNS', bytes(t))
    png += chunk(b'IDAT', zlib.compress(bytes(raw), 9))
    png += chunk(b'IEND', b'')
    with open(path, 'wb') as f:
        f.write(png)


# ---------------------------------------------------------------------------
# Decoders
# ---------------------------------------------------------------------------

def decode_sprite_frame(b, frame, dst, pitch, dx, dy, flip=False, rle=True):
    """Draw one frame into dst (bytearray) at (dx,dy) = top-left of the sprite box.
    Transparent pixels are skipped. flip mirrors inside the maxw box."""
    count, mw, mh = struct.unpack_from('<HHH', b, 0)
    x, y, w, h, off = struct.unpack_from('<BBBBI', b, 6 + 8 * frame)
    if off & 0x80000000:
        w += 256
        off &= 0x7FFFFFFF
    for row in range(h):
        if rle:
            p = off + struct.unpack_from('<H', b, off + 2 * row)[0]
        else:
            p = off + row * w
        col = 0
        oy = dy + y + row
        while col < w:
            if rle:
                c = b[p]
                p += 1
                if c & 0x80:
                    col += c & 0x7F
                    continue
                if c & 0x40:
                    n = c & 0x3F
                    v = b[p]
                    p += 1
                    run = [v] * n
                else:
                    n = c & 0x3F
                    run = b[p:p + n]
                    p += n
            else:
                run = b[p:p + w]
                p += w
                n = w
            for k in range(n):
                px = x + col + k
                if flip:
                    px = mw - 1 - px
                dst[oy * pitch + dx + px] = run[k]
            col += n


def sprite_sheet(b, rle=True, per_row=5, flip=False):
    count, mw, mh = struct.unpack_from('<HHH', b, 0)
    cols = min(per_row, count)
    rows = (count + cols - 1) // cols
    W, H = cols * mw, rows * mh
    img = bytearray(W * H)
    for i in range(count):
        decode_sprite_frame(b, i, img, W, (i % cols) * mw, (i // cols) * mh, flip, rle)
    return W, H, img


def decode_image(b):
    w, h = struct.unpack_from('<HH', b, 0)
    return w, h, bytearray(b[4:4 + w * h])


def decode_cursor(b):
    hx, hy, w, h = struct.unpack_from('<HHHH', b, 0)
    return w, h, bytearray(b[8:8 + w * h]), (hx, hy)


def decode_font(b, per_row=16, colors=(0, 228, 239, 0, 230, 231, 232, 233)):
    """Font: 'FONT ' + u8 last, u8 maxw, u8 maxh, u32 offs[last-32+1] (chars 32..last,
    0 = no glyph). glyph: u8 w,h,xoff,yoff then bytes: hi5 = skip transparent, lo3 = colour."""
    last, mw, mh = b[5], b[6], b[7]
    n = last - 32 + 1
    offs = struct.unpack_from('<%dI' % n, b, 8)
    rows = (n + per_row - 1) // per_row
    W, H = per_row * mw, rows * mh
    img = bytearray(W * H)
    for i, o in enumerate(offs):
        if not o:
            continue
        w, h, xo, yo = b[o], b[o + 1], b[o + 2], b[o + 3]
        p = o + 4
        bx = (i % per_row) * mw + xo
        by = (i // per_row) * mh + yo
        pos = 0
        total = w * h
        while pos < total:
            c = b[p]
            p += 1
            pos += c >> 3
            if pos >= total:
                break
            img[(by + pos // w) * W + bx + pos % w] = colors[c & 7]
            pos += 1
    return W, H, img


def decode_tileset(pal, mega, mini, per_row=16):
    n = len(mega) // 32
    rows = (n + per_row - 1) // per_row
    W, H = per_row * 32, rows * 32
    img = bytearray(W * H)
    for t in range(n):
        ox = (t % per_row) * 32
        oy = (t // per_row) * 32
        for m in range(16):
            v = struct.unpack_from('<H', mega, t * 32 + m * 2)[0]
            mi = v >> 2
            fx = v & 2
            fy = v & 1
            base = mi * 64
            for yy in range(8):
                sy = 7 - yy if fy else yy
                for xx in range(8):
                    sx = 7 - xx if fx else xx
                    img[(oy + (m // 4) * 8 + yy) * W + ox + (m % 4) * 8 + xx] = mini[base + sy * 8 + sx]
    return W, H, img


def tile_map_table(mp):
    """PUD tile value v (0..0x9DF) -> megatile = table[v]."""
    t = [0] * (MAP_GROUPS * 16)
    for g in range(MAP_GROUPS):
        for j in range(16):
            t[(g << 4) | j] = struct.unpack_from('<H', mp, g * MAP_REC + j * 2)[0]
    return t


# ---------------------------------------------------------------------------
# PUD
# ---------------------------------------------------------------------------

UNIT_NAMES = [
    'Footman', 'Grunt', 'Peasant', 'Peon', 'Ballista', 'Catapult', 'Knight', 'Ogre',
    'Elven Archer', 'Troll Axethrower', 'Mage', 'Death Knight', 'Paladin', 'Ogre-Mage',
    'Dwarven Demolition Squad', 'Goblin Sappers', 'Attack Peasant', 'Attack Peon',
    'Ranger', 'Berserker', 'Alleria', 'Teron Gorefiend', 'Kurdan and Sky\'ree', 'Dentarg',
    'Khadgar', 'Grom Hellscream', 'Human Oil Tanker', 'Orc Oil Tanker', 'Human Transport',
    'Orc Transport', 'Elven Destroyer', 'Troll Destroyer', 'Battleship', 'Ogre Juggernaught',
    '(unused 0x22)', 'Deathwing', '(unused 0x24)', '(unused 0x25)', 'Gnomish Submarine',
    'Giant Turtle', 'Gnomish Flying Machine', 'Goblin Zeppelin', 'Gryphon Rider', 'Dragon',
    'Turalyon', 'Eye of Kilrogg', 'Danath', 'Korgath Bladefist', '(unused 0x30)', 'Cho\'gall',
    'Lothar', 'Gul\'dan', 'Uther Lightbringer', 'Zuljin', '(unused 0x36)', 'Skeleton',
    'Daemon', 'Critter', 'Farm', 'Pig Farm', 'Human Barracks', 'Orc Barracks', 'Church',
    'Altar of Storms', 'Human Scout Tower', 'Orc Scout Tower', 'Stables', 'Ogre Mound',
    'Gnomish Inventor', 'Goblin Alchemist', 'Gryphon Aviary', 'Dragon Roost',
    'Human Shipyard', 'Orc Shipyard', 'Town Hall', 'Great Hall', 'Elven Lumber Mill',
    'Troll Lumber Mill', 'Human Foundry', 'Orc Foundry', 'Mage Tower',
    'Temple of the Damned', 'Human Blacksmith', 'Orc Blacksmith', 'Human Refinery',
    'Orc Refinery', 'Human Oil Platform', 'Orc Oil Platform', 'Keep', 'Stronghold',
    'Castle', 'Fortress', 'Gold Mine', 'Oil Patch', 'Human Start Location',
    'Orc Start Location', 'Human Guard Tower', 'Orc Guard Tower', 'Human Cannon Tower',
    'Orc Cannon Tower', 'Circle of Power', 'Dark Portal', 'Runestone', 'Human Wall', 'Orc Wall',
]

ERA_NAMES = ['summer (forest)', 'winter', 'wasteland', 'swamp']


def parse_pud(b):
    secs = []
    p = 0
    while p + 8 <= len(b):
        tag = b[p:p + 4].decode('latin1')
        n = struct.unpack_from('<I', b, p + 4)[0]
        secs.append((tag, p + 8, n))
        p += 8 + n
    return secs


def pud_summary(b):
    secs = parse_pud(b)
    d = {t: (o, n) for (t, o, n) in secs}
    out = []
    out.append('sections: ' + ' '.join('%s(%d)' % (t, n) for (t, o, n) in secs))
    if 'VER ' in d:
        out.append('version: 0x%x' % struct.unpack_from('<H', b, d['VER '][0])[0])
    if 'DESC' in d:
        o, n = d['DESC']
        out.append('desc: %r' % b[o:o + n].split(b'\0')[0].decode('latin1'))
    era_tag = 'ERAX' if 'ERAX' in d else 'ERA '
    era = None
    if era_tag in d:
        era = struct.unpack_from('<H', b, d[era_tag][0])[0]
        out.append('era (%s): %d %s' % (era_tag, era, ERA_NAMES[era] if era < 4 else '?'))
    w = h = 0
    if 'DIM ' in d:
        w, h = struct.unpack_from('<HH', b, d['DIM '][0])
        out.append('size: %dx%d' % (w, h))
    if 'OWNR' in d:
        o, n = d['OWNR']
        out.append('owners: ' + ' '.join('%d' % c for c in b[o:o + 16]))
    if 'SIDE' in d:
        o, n = d['SIDE']
        out.append('races:  ' + ' '.join('%d' % c for c in b[o:o + 16]))
    for tag in ('SGLD', 'SLBR', 'SOIL'):
        if tag in d:
            o, n = d[tag]
            out.append('%s: %s' % (tag, list(struct.unpack_from('<16H', b, o))))
    if 'AIPL' in d:
        o, n = d['AIPL']
        out.append('AIPL: ' + ' '.join('%d' % c for c in b[o:o + 16]))
    if 'UNIT' in d:
        o, n = d['UNIT']
        cnt = {}
        for i in range(n // 8):
            x, y, t, pl, data = struct.unpack_from('<HHBBH', b, o + i * 8)
            cnt[t] = cnt.get(t, 0) + 1
        out.append('units: %d' % (n // 8))
        for t in sorted(cnt):
            nm = UNIT_NAMES[t] if t < len(UNIT_NAMES) else '?'
            out.append('  0x%02x %-26s x%d' % (t, nm, cnt[t]))
    if 'UDTA' in d:
        o, n = d['UDTA']
        dflt = struct.unpack_from('<H', b, o)[0]
        out.append('UDTA: %d bytes, use-default=%d' % (n, dflt))
    return '\n'.join(out), d, w, h, era


# UDTA field layout (offsets relative to section data); arrays of 110 unless noted
UDTA_FIELDS = [
    ('default_flag', 'H', 1), ('overlap_frames', 'H', 110), ('obsolete1', 'H', 508),
    ('sight', 'I', 110), ('hp', 'H', 110), ('magic', 'B', 110), ('build_time', 'B', 110),
    ('gold', 'B', 110), ('lumber', 'B', 110), ('oil', 'B', 110), ('size', 'I', 110),
    ('box', 'I', 110), ('range', 'B', 110), ('react_cpu', 'B', 110), ('react_human', 'B', 110),
    ('armor', 'B', 110), ('rect_select', 'B', 110), ('priority', 'B', 110),
    ('basic_damage', 'B', 110), ('piercing_damage', 'B', 110), ('weapon_upgradable', 'B', 110),
    ('armor_upgradable', 'B', 110), ('missile', 'B', 110), ('unit_type', 'B', 110),
    ('decay', 'B', 110), ('annoy', 'B', 110), ('mouse_right', 'B', 58), ('points', 'H', 110),
    ('can_target', 'B', 110), ('flags', 'I', 110), ('obsolete2', 'H', 127),
]


def parse_udta(b, o):
    res = {}
    for name, f, n in UDTA_FIELDS:
        sz = struct.calcsize('<' + f)
        res[name] = list(struct.unpack_from('<%d%s' % (n, f), b, o))
        o += sz * n
    return res, o


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def load_spec(spec):
    """'path:idx' -> bytes ; plain path -> file bytes."""
    if ':' in spec and not os.path.exists(spec):
        path, idx = spec.rsplit(':', 1)
        return WarArchive(path).load(int(idx))
    with open(spec, 'rb') as f:
        return f.read()


def default_palette():
    return [(i, i, i) for i in range(256)]


def cmd_list(a, detail=False):
    ar = WarArchive(a)
    print('%s: %d entries, id %d' % (a, ar.count, ar.id))
    for i in range(ar.count):
        off, size, flags = ar.raw_header(i)
        try:
            b = ar.load(i)
            t, info = guess(b)
        except Exception as e:  # noqa
            t, info = 'ERROR', str(e)
        print('%4d off %8d size %8d %s stored %8d  %-8s %s' % (
            i, off, size, 'Z' if flags == 0x20 else '-', ar.stored_size(i), t, info))


CLASS_OF = {'empty': 'empty', 'wav': 'wav', 'pud': 'pud', 'font': 'font', 'smacker': 'smacker',
            'iff': 'iff', 'palette': 'palette', 'tilemap': 'tilemap', 'image': 'image',
            'cursor': 'cursor', 'sprite': 'sprite', 'gfu': 'gfu'}
CLASS_ORDER = ['empty', 'wav', 'pud', 'font', 'smacker', 'iff', 'palette', 'tilemap', 'image',
               'cursor', 'sprite', 'gfu', 'other']


def selftest(datadir):
    """Same report as tools/test_war.c (CRC32 of all entries, decoded sprite pixels)."""
    total_s = total_f = 0
    for name in ('War Data', 'War Resources', 'War Strings', 'War Sounds', 'War Music', 'War Movies'):
        ar = WarArchive(os.path.join(datadir, name))
        crc = pix = 0
        nbytes = sprites = frames = 0
        hist = {}
        for i in range(ar.count):
            b = ar.load(i)
            crc = zlib.crc32(b, crc)
            nbytes += len(b)
            t = CLASS_OF.get(guess(b)[0], 'other')
            hist[t] = hist.get(t, 0) + 1
            if t in ('sprite', 'gfu'):
                sprites += 1
                count, mw, mh = struct.unpack_from('<HHH', b, 0)
                for f in range(count):
                    buf = bytearray(mw * mh)
                    decode_sprite_frame(b, f, buf, mw, 0, 0, False, t == 'sprite')
                    pix = zlib.crc32(buf, pix)
                    buf = bytearray(mw * mh)
                    decode_sprite_frame(b, f, buf, mw, 0, 0, True, t == 'sprite')
                    for yy in range(mh):           # emulate clip rect (2,2)-(mw-2,mh-2)
                        for xx in range(mw):
                            if xx < 2 or yy < 2 or xx >= mw - 2 or yy >= mh - 2:
                                buf[yy * mw + xx] = 0
                    pix = zlib.crc32(buf, pix)
                    frames += 1
        total_s += sprites
        total_f += frames
        print('%-14s id %4d entries %3d bytes %9d crc %08x sprites %3d frames %5d pixcrc %08x' % (
            name, ar.id, ar.count, nbytes, crc, sprites, frames, pix))
        print('               types:' + ''.join(' %s=%d' % (k, hist[k]) for k in CLASS_ORDER if k in hist))
    print('total sprites %d frames %d' % (total_s, total_f))


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 1
    cmd = argv[1]
    args = argv[2:]
    opts = {}
    pos = []
    i = 0
    while i < len(args):
        if args[i].startswith('--'):
            key = args[i][2:]
            if key in ('sheet', 'flip', 'raw'):
                opts[key] = True
            else:
                opts[key] = args[i + 1]
                i += 1
        else:
            pos.append(args[i])
        i += 1
    if cmd in ('list', 'scan'):
        cmd_list(pos[0], cmd == 'scan')
    elif cmd == 'extract':
        b = WarArchive(pos[0]).load(int(pos[1]))
        with open(pos[2], 'wb') as f:
            f.write(b)
        print('%d bytes' % len(b))
    elif cmd == 'png':
        b = WarArchive(pos[0]).load(int(pos[1]))
        pal = palette_rgb(load_spec(opts['palette'])) if 'palette' in opts else default_palette()
        t, info = guess(b)
        tr = None
        if t == 'image':
            w, h, img = decode_image(b)
        elif t == 'cursor':
            w, h, img, hot = decode_cursor(b)
            tr = 0
        elif t == 'font':
            w, h, img = decode_font(b)
            tr = 0
        elif t in ('sprite', 'gfu'):
            rle = (t == 'sprite')
            if 'frame' in opts:
                count, mw, mh = struct.unpack_from('<HHH', b, 0)
                w, h = mw, mh
                img = bytearray(w * h)
                decode_sprite_frame(b, int(opts['frame']), img, w, 0, 0, 'flip' in opts, rle)
            else:
                w, h, img = sprite_sheet(b, rle, int(opts.get('perrow', 5)), 'flip' in opts)
            tr = 0
        elif t == 'palette':
            pal = palette_rgb(b)
            w, h = 16 * 8, 16 * 8
            img = bytearray(w * h)
            for y in range(h):
                for x in range(w):
                    img[y * w + x] = (y // 8) * 16 + x // 8
        else:
            print('cannot render type %s' % t)
            return 1
        write_png(pos[2], w, h, img, pal, tr)
        print('%s %s -> %dx%d %s' % (t, info, w, h, pos[2]))
    elif cmd == 'tileset':
        ar = WarArchive(pos[0])
        pal = palette_rgb(ar.load(int(pos[1])))
        w, h, img = decode_tileset(pal, ar.load(int(pos[2])), ar.load(int(pos[3])))
        write_png(pos[4], w, h, img, pal)
        print('%dx%d -> %s' % (w, h, pos[4]))
    elif cmd == 'pud':
        b = load_spec(pos[0])
        text, d, w, h, era = pud_summary(b)
        print(text)
        if 'udta' in opts and 'UDTA' in d:
            u, end = parse_udta(b, d['UDTA'][0])
            print('UDTA parsed %d of %d bytes' % (end - d['UDTA'][0], d['UDTA'][1]))
            for k in range(int(opts['udta'])):
                print('  0x%02x %-24s hp %3d sight %d armor %d dmg %d/%d range %d gold %d lumber %d oil %d time %d size %08x box %08x pts %d type %d flags %08x' % (
                    k, UNIT_NAMES[k][:24], u['hp'][k], u['sight'][k], u['armor'][k], u['basic_damage'][k],
                    u['piercing_damage'][k], u['range'][k], u['gold'][k] * 10, u['lumber'][k] * 10, u['oil'][k] * 10,
                    u['build_time'][k], u['size'][k], u['box'][k], u['points'][k], u['unit_type'][k], u['flags'][k]))
        if 'png' in opts:
            ar = WarArchive(opts['archive'])
            base = {0: 2, 1: 18, 2: 10, 3: 10}[era if era is not None else 0]
            if 'tsbase' in opts:
                base = int(opts['tsbase'])
            pal = palette_rgb(ar.load(base))
            mega = ar.load(base + 1)
            mini = ar.load(base + 2)
            table = tile_map_table(ar.load(base + 3))
            o, n = d['MTXM']
            scale = int(opts.get('scale', 1))  # pixels per tile: 1 = minimap colour, 32 = full
            if scale == 32:
                tw, th, timg = decode_tileset(pal, mega, mini, 1)
                W, H = w * 32, h * 32
                img = bytearray(W * H)
                for ty in range(h):
                    for tx in range(w):
                        v = struct.unpack_from('<H', b, o + (ty * w + tx) * 2)[0]
                        m = table[v] if v < len(table) else 0
                        for yy in range(32):
                            s = (m * 32 + yy) * 32
                            img[(ty * 32 + yy) * W + tx * 32:(ty * 32 + yy) * W + tx * 32 + 32] = timg[s:s + 32]
            else:
                W, H = w * scale, h * scale
                img = bytearray(W * H)
                for ty in range(h):
                    for tx in range(w):
                        v = struct.unpack_from('<H', b, o + (ty * w + tx) * 2)[0]
                        m = table[v] if v < len(table) else 0
                        # minimap colour: centre pixel of the megatile
                        mv = struct.unpack_from('<H', mega, m * 32 + 5 * 2)[0]
                        c = mini[(mv >> 2) * 64 + 4 * 8 + 4]
                        for yy in range(scale):
                            for xx in range(scale):
                                img[(ty * scale + yy) * W + tx * scale + xx] = c
            write_png(opts['png'], W, H, img, pal)
            print('map -> %s %dx%d' % (opts['png'], W, H))
    elif cmd == 'selftest':
        selftest(pos[0])
    elif cmd == 'strings':
        b = WarArchive(pos[0]).load(int(pos[1]))
        n = struct.unpack_from('<H', b, 0)[0]
        print('count %d' % n)
        offs = struct.unpack_from('<%dH' % n, b, 0)
        for k in range(1, n):
            s = b[offs[k]:].split(b'\0')[0].decode('latin1')
            print('%4d %r' % (k, s))
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
