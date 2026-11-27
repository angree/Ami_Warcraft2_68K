"""MorphOS icon for w2_MorphOS: our own drawn w2 icon (tools/icons/draw_icons.py output) scaled into a 64x64 PNG.
Ambient shows a plain PNG renamed to .info (!instrukcje 45-MorphOS-natywny-PPC-2D.md).
    py -I tools/icons/make_mos_icon.py <w2.png from draw_icons.py> <out w2_MorphOS.info>"""
import sys
from PIL import Image

src, out = sys.argv[1], sys.argv[2]
im = Image.open(src).convert("RGBA")
scale = min(64 / im.width, 64 / im.height)
w, h = max(1, int(im.width * scale)), max(1, int(im.height * scale))
im = im.resize((w, h), Image.NEAREST)             # keep the pixel-art edges sharp
canvas = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
canvas.paste(im, ((64 - w) // 2, (64 - h) // 2), im)
canvas.save(out, format="PNG")
print("%s: 64x64 PNG icon" % out)
