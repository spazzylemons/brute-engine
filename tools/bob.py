"""
Convert an image into a bob lump.
"""

from PIL import Image
from pack import Lump
import struct

def convert_bob(name: str, img: Image) -> Lump:
    data = bytearray()
    for y in range(img.height):
        width = 0
        bbyte = 0
        wbyte = 0
        for x in range(img.width):
            bbyte <<= 1
            wbyte <<= 1
            match img.getpixel((x, y)):
                case (0xff, 0xff, 0xff):
                    wbyte |= 1
                case (0x00, 0x00, 0x00):
                    bbyte |= 1
                case (0x00, 0xff, 0x00):
                    pass
                case _:
                    raise RuntimeError()
            width += 1
            if width == 8:
                data.append(0xff ^ bbyte)
                data.append(wbyte)
                bbyte = 0
                wbyte = 0
                width = 0
        if width != 0:
            bbyte <<= (8 - width)
            wbyte <<= (8 - width)
            data.append(0xff ^ bbyte)
            data.append(wbyte)
    return Lump(name, struct.pack('<HH', img.width, img.height) + data)
