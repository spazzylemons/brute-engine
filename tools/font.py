"""
Convert a font to a branch of bob lumps.
"""

from bob import convert_bob
from PIL import Image
from pack import Branch, Lump
import struct

# notes:
# ascii-only for now, 16 glyphs per row.
def convert_font(path: str, branchname: str) -> Branch:
    img = Image.open(path)
    result = Branch(branchname)
    # Get height of font.
    height = 0
    while img.getpixel((0, height)) != (0xff, 0x00, 0xff):
        height += 1
    # Iterate through each glyph.
    y = 0
    glyphi = 32
    for glyphy in range(6):
        x = 0
        for glyphx in range(16):
            width = 0
            while img.getpixel((x + width, y)) != (0xff, 0x00, 0xff):
                width += 1
            glyph = img.crop((x, y, x + width, y + height))
            result.children.append(convert_bob(chr(glyphi), glyph))
            glyphi += 1
            x += width + 1
        y += height + 1
    return result
