#!/usr/bin/env python3
"""Draws the AmiW2 Workbench icon pictures (our own art, no game asset) as 36x40 PNGs.

    python3 tools/icons/draw_icons.py <outdir>      -> <outdir>/w2.png, <outdir>/w2setup.png

w2      : heraldic kite shield (blue/red quarters, gold rim, gold tower) over a crossed sword and axe
w2setup : steel cog wheel with a screwdriver across it
Shapes are rendered with 6x6 supersampling, then put on a fixed palette of < 32 colours (the NewIcons
limit in 15-ikony-newicons.md) and given a dark outline. Turn them into .info files with
tools/mkicon_newicons.py.
"""
import math
import os
import struct
import sys
import zlib

W, H, SS = 36, 40, 6

PALETTE = [
    (16, 16, 24),                                                    # outline
    (24, 40, 96), (40, 72, 160), (72, 112, 208), (128, 160, 232),    # blue
    (96, 16, 16), (160, 32, 32), (208, 64, 48), (240, 120, 96),      # red
    (120, 80, 16), (184, 136, 32), (232, 192, 64), (255, 240, 160),  # gold
    (64, 68, 80), (112, 118, 132), (160, 166, 180), (204, 210, 222), (248, 250, 255),  # steel
    (72, 40, 20), (120, 72, 36), (168, 112, 60),                     # wood
]


def clamp(v):
    return 0 if v < 0 else 255 if v > 255 else int(v)


def shade(c, k):
    return (clamp(c[0] * k), clamp(c[1] * k), clamp(c[2] * k))


def seg_dist(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    t = ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)
    t = 0 if t < 0 else 1 if t > 1 else t
    return math.hypot(px - ax - t * dx, py - ay - t * dy), t


def in_poly(x, y, pts):
    inside = False
    j = len(pts) - 1
    for i in range(len(pts)):
        xi, yi = pts[i]
        xj, yj = pts[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
            inside = not inside
        j = i
    return inside


def light(x, y):
    """top-left light: 1.25 at the top-left corner down to 0.75 at the bottom-right"""
    return 1.25 - 0.5 * ((x / W) + (y / H)) / 2


# ------------------------------------------------------------------ w2: shield, sword, axe
def shield_hw(y):
    """half width of the kite shield at y (None outside)"""
    top, mid, tip = 5.0, 19.0, 37.0
    if y < top or y > tip:
        return None
    if y <= mid:
        return 12.5 - 1.2 * ((mid - y) / (mid - top)) ** 2
    t = (y - mid) / (tip - mid)
    return 12.5 * math.sqrt(max(0.0, 1 - t * t)) * (1 - 0.15 * t)


def game_sample(x, y):
    c = None
    # sword: blade top-left -> guard -> grip -> pommel at the bottom right
    d, t = seg_dist(x, y, 3.0, 2.5, 25.0, 28.0)
    if d < 1.5:
        c = PALETTE[17] if d < 0.5 else shade(PALETTE[15], light(x, y))
    d, _ = seg_dist(x, y, 21.0, 31.5, 28.5, 24.0)
    if d < 1.1:
        c = shade(PALETTE[11], light(x, y))
    d, _ = seg_dist(x, y, 25.0, 28.0, 30.5, 34.5)
    if d < 1.1:
        c = shade(PALETTE[19], light(x, y))
    if math.hypot(x - 31.5, y - 35.5) < 1.9:
        c = shade(PALETTE[11], light(x, y))
    # axe: handle bottom-left -> head at the top right
    d, _ = seg_dist(x, y, 5.0, 37.0, 30.0, 4.5)
    if d < 1.2:
        c = shade(PALETTE[19], light(x, y))
    head = [(25.5, 6.0), (27.5, 1.0), (31.0, 0.5), (35.0, 2.5), (35.5, 7.0), (34.0, 11.5), (31.5, 9.5), (28.0, 8.5)]
    if in_poly(x, y, head):
        edge = (x - 31.0) + (y - 6.0) * -0.3
        c = PALETTE[17] if edge > 3.0 else shade(PALETTE[14], light(x, y) * 1.1)
    # shield
    hw = shield_hw(y)
    if hw is not None and abs(x - 18.0) <= hw:
        inner = hw - abs(x - 18.0)
        hw2 = shield_hw(y + 1.6)
        rim = inner < 1.6 or y < 6.6 or hw2 is None or abs(x - 18.0) > hw2 - 0.0 and y > 30
        k = light(x, y)
        if rim:
            c = shade(PALETTE[11], k)
        else:
            left, top = x < 18.0, y < 18.5
            field = PALETTE[2] if left == top else PALETTE[6]
            c = shade(field, k)
            # gold tower in the middle: body, three merlons, black door
            if 14.0 <= x <= 22.0 and 13.0 <= y <= 25.5:
                c = shade(PALETTE[11], k)
                if 16.8 <= x <= 19.2 and y >= 21.0:
                    c = PALETTE[0]
                if 17.4 <= x <= 18.6 and 16.0 <= y <= 18.5:
                    c = PALETTE[0]
            if 10.5 <= y < 13.0 and (14.0 <= x <= 15.8 or 17.1 <= x <= 18.9 or 20.2 <= x <= 22.0):
                c = shade(PALETTE[11], k)
    return c


# ------------------------------------------------------------------ w2setup: cog + screwdriver
def setup_sample(x, y):
    c = None
    cx, cy = 17.0, 21.0
    r = math.hypot(x - cx, y - cy)
    a = math.atan2(y - cy, x - cx)
    teeth = 8
    ph = (a * teeth / (2 * math.pi)) % 1.0
    tooth = 0.22 < ph < 0.78
    outer = 15.5 if tooth else 12.0
    if 5.0 < r < outer:
        # light from the top left on the ring, inner bevel darker
        k = 1.0 + 0.3 * math.cos(a + 2.4)
        base = PALETTE[15] if r > 7.0 else PALETTE[13]
        c = shade(base, k)
        if 10.5 < r < 11.3:
            c = shade(PALETTE[13], k)
    # screwdriver across: red handle bottom-left, steel shaft to the top right
    d, _ = seg_dist(x, y, 3.0, 37.0, 11.5, 28.5)
    if d < 2.6:
        c = shade(PALETTE[7] if d < 1.0 else PALETTE[6], light(x, y))
    d, _ = seg_dist(x, y, 11.5, 28.5, 31.5, 8.5)
    if d < 1.4:
        c = PALETTE[17] if d < 0.5 else shade(PALETTE[14], light(x, y))
    d, _ = seg_dist(x, y, 31.5, 8.5, 34.0, 6.0)
    if d < 1.4:
        c = shade(PALETTE[13], light(x, y))
    return c


def render(sample):
    rgba = []
    for py in range(H):
        row = []
        for px in range(W):
            n, votes = 0, {}
            for sy in range(SS):
                for sx in range(SS):
                    c = sample(px + (sx + 0.5) / SS, py + (sy + 0.5) / SS)
                    if c is not None:
                        n += 1
                        q = nearest(c)
                        votes[q] = votes.get(q, 0) + 1
            # dominant palette colour, no blending: crisp pixel art without muddy edge colours
            row.append(None if n * 2 < SS * SS else max(votes.items(), key=lambda kv: kv[1])[0])
        rgba.append(row)
    # dark outline on opaque pixels that touch transparency (or the border)
    out = []
    for y in range(H):
        row = []
        for x in range(W):
            c = rgba[y][x]
            if c is None:
                row.append(None)
                continue
            edge = False
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                nx, ny = x + dx, y + dy
                if not (0 <= nx < W and 0 <= ny < H) or rgba[ny][nx] is None:
                    edge = True
            row.append(PALETTE[0] if edge else nearest(c))
        out.append(row)
    return out


def nearest(c):
    best, bd = None, 1e9
    for p in PALETTE:
        d = (p[0] - c[0]) ** 2 * 3 + (p[1] - c[1]) ** 2 * 4 + (p[2] - c[2]) ** 2 * 2
        if d < bd:
            best, bd = p, d
    return best


def write_png(path, img):
    raw = bytearray()
    for row in img:
        raw.append(0)
        for c in row:
            raw += bytes((0, 0, 0, 0)) if c is None else bytes((c[0], c[1], c[2], 255))

    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)
    png = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', W, H, 8, 6, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(bytes(raw), 9)) + chunk(b'IEND', b'')
    with open(path, 'wb') as f:
        f.write(png)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else '.'
    os.makedirs(out, exist_ok=True)
    for name, fn in (('w2', game_sample), ('w2setup', setup_sample)):
        img = render(fn)
        cols = {c for row in img for c in row if c is not None}
        assert len(cols) <= 32, (name, len(cols))
        write_png(os.path.join(out, name + '.png'), img)
        print('%s.png: %dx%d, %d colours' % (name, W, H, len(cols)))


if __name__ == '__main__':
    main()
