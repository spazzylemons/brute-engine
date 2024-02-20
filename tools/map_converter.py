#!/usr/bin/env python3

# This script converts maps into Brute's binary formats. Maps must be in PWADs and in UDMF format.
# This requires the parsimonious package to run. (TODO use python environments to simplify this)

# TODO convert sectors into sets of convex shapes or otherwise throw error on concave shapes

import io
import os
import re
import shutil
import struct
import sys

from pack import Branch, Lump

from PIL import Image

UNQUOTE = re.compile(r'\\(.)')

PALETTE_CONV = (
    0x00, 0x10, 0x20, 0x30,
    0x40, 0x50, 0x60, 0x70,
    0x7f, 0x8f, 0x9f, 0xaf,
    0xbf, 0xcf, 0xdf, 0xef,
    0xff
)

from parsimonious import Grammar, NodeVisitor

udmf_grammar = Grammar(
    r"""
    translation_unit = ws global_expr*
    global_expr      = block / expr
    block            = identifier ws "{" ws expr* "}" ws
    expr             = identifier ws "=" ws value ws ";" ws
    identifier       = ~r"[A-Za-z_]+[A-Za-z0-9_]*"
    value            = float / integer / quoted_string / keyword
    integer          = ~r"[+-]?(0|[1-9][0-9]*)" / ~r"0x[0-9A-Fa-f]+"
    float            = ~r"[+-]?[0-9]+\.[0-9]*([eE][+-]?[0-9]+)?"
    quoted_string    = ~r"\"([^\"\\]*(\\.[^\\]*)*)\""
    keyword          = "true" / "false"
    ws               = (space / line_comment / block_comment)*
    space            = ~r"\s+"
    line_comment     = ~r"//[^\r\n]*"
    block_comment    = ~r"/\*.*?\*/"
    """
)

class UdmfVisitor(NodeVisitor):
    def visit_translation_unit(self, node, visited_children):
        result = {}
        for key, value in visited_children[1]:
            if isinstance(value, dict):
                if key not in result:
                    result[key] = []
                result[key].append(value)
            else:
                result[key] = value
        return result

    def visit_global_expr(self, node, visited_children):
        return visited_children[0]

    def visit_block(self, node, visited_children):
        name = visited_children[0]
        properties = {}
        for key, value in visited_children[4]:
            properties[key] = value
        return name, properties

    def visit_expr(self, node, visited_children):
        return visited_children[0], visited_children[4]

    def visit_identifier(self, node, visited_children):
        return node.full_text[node.start:node.end]

    def visit_value(self, node, visited_children):
        return visited_children[0]

    def visit_integer(self, node, visited_children):
        return int(node.full_text[node.start:node.end])

    def visit_float(self, node, visited_children):
        return float(node.full_text[node.start:node.end])

    def visit_quoted_string(self, node, visited_children):
        return UNQUOTE.sub(r'\1', node.full_text[node.start+1:node.end-1])

    def visit_keyword(self, node, visited_children):
        return node.full_text[node.start:node.end] == 'true'

    def generic_visit(self, node, visited_children):
        return visited_children or node

if len(sys.argv) != 4:
    print('usage: {} <input PWAD> <output folder>'.format(sys.argv[0]), file=sys.stderr)
    exit(1)

def handle_udmf(content):
    udmf = content.decode()
    # Parse UDMF.
    mapdata = UdmfVisitor().visit(udmf_grammar.parse(udmf))
    # Write vertices.
    out_vertices = bytearray()
    for vertex in mapdata['vertex']:
        x = round(vertex['x'])
        y = round(vertex['y'])
        out_vertices.extend(struct.pack('<hh', x, y))
    # Collection of patch names to use.
    patchnames = {}
    def get_patch_id(name):
        # Lack of patch means ID 0
        if name is None:
            return 0
        # Check if already registered
        if name not in patchnames:
            # IDs start at 1, because 0 means no patch
            patchnames[name] = len(patchnames) + 1
        return patchnames[name]
    # Collection of flat names to use.
    flatnames = {}
    def get_flat_id(name):
        # TODO check sky texture and return 0
        # Check if already registered
        if name not in flatnames:
            # IDs start at 1, because 0 means no flat
            flatnames[name] = len(flatnames) + 1
        return flatnames[name]
    # Get which walls belong to which sector, and other sidedef info.
    def get_sidedef_info(sidedef_id):
        sidedef = mapdata['sidedef'][sidedef_id]
        sector = sidedef['sector']
        xoffset = sidedef.get('offsetx', 0)
        yoffset = sidedef.get('offsety', 0)
        texturetop = get_patch_id(sidedef.get('texturetop'))
        texturemiddle = get_patch_id(sidedef.get('texturemiddle'))
        texturebottom = get_patch_id(sidedef.get('texturebottom'))
        return sector, xoffset, yoffset, texturetop, texturemiddle, texturebottom

    walls = [[] for _ in range(len(mapdata['sector']))]
    for linedef in mapdata['linedef']:
        if 'twosided' in linedef:
            front_data = get_sidedef_info(linedef['sidefront'])
            back_data = get_sidedef_info(linedef['sideback'])
            walls[front_data[0]].append((linedef['v1'], linedef['v2'], back_data[0], *front_data[1:]))
            walls[back_data[0]].append((linedef['v2'], linedef['v1'], front_data[0], *back_data[1:]))
        else:
            front_data = get_sidedef_info(linedef['sidefront'])
            walls[front_data[0]].append((linedef['v1'], linedef['v2'], *front_data))
    # Fix each wall set so its lines are in order.
    for i, wall_set in enumerate(walls):
        new_wall_set = [wall_set[0]]
        while len(new_wall_set) < len(wall_set):
            new_wall_set.append([wall for wall in wall_set if wall[0] == new_wall_set[-1][1]][0])
        walls[i] = new_wall_set
    # Write walls and sectors.
    out_walls = bytearray()
    out_sectors = bytearray()
    wall_index = 0
    for j, wall_set in enumerate(walls):
        mapsector = mapdata['sector'][j]
        out_sectors.extend(struct.pack('<HHhhBB',
            len(wall_set),
            wall_index,
            mapsector.get('heightfloor', 0),
            mapsector.get('heightceiling', 0),
            get_flat_id(mapsector['texturefloor']),
            get_flat_id(mapsector['textureceiling']),
        ))
        for i, wall in enumerate(wall_set):
            assert wall[1] == wall_set[(i+1)%len(wall_set)][0]
            out_walls.extend(struct.pack('<HHBBBBB', wall[0], wall[2], wall[3], wall[4], wall[5], wall[6], wall[7]))
            wall_index += 1
    out_patches = bytearray()
    for name in patchnames:
        encoded_name = name.lower().encode()
        assert len(encoded_name) <= 8
        out_patches.extend(encoded_name.ljust(8, b'\0'))
    out_flats = bytearray()
    for name in flatnames:
        encoded_name = name.lower().encode()
        assert len(encoded_name) <= 8
        out_flats.extend(encoded_name.ljust(8, b'\0'))
    # If uncommented, outputs a list of Desmos equations to plot the map.
    # for wall_set in walls:
    #     # \operatorname{polygon}\left(\left(1,2\right),\left(3,4\right)\right)
    #     print(r'\operatorname{polygon}\left(', end='')
    #     for i, wall in enumerate(wall_set):
    #         if i != 0:
    #             print(',', end='')
    #         print(r'\left(', end='')
    #         print(mapdata['vertex'][wall[0]]['x'], end='')
    #         print(',', end='')
    #         print(mapdata['vertex'][wall[0]]['y'], end='')
    #         print(r'\right)', end='')
    #     print(r'\right)')
    # Commit to files.
    result = {}
    result['vertices'] = out_vertices
    result['sectors'] = out_sectors
    result['walls'] = out_walls
    result['patches'] = out_patches
    result['flats'] = out_flats
    return result

def read_image(filepath):
    img = Image.open(filepath)
    width, height = img.size
    result = bytearray(width * height)
    for y in range(height):
        for x in range(width):
            result[(y * width) + x] = PALETTE_CONV.index(img.getpixel((x, y)))
    return width, height, result

def write_patch(filepath) -> bytes:
    width, height, pixels = read_image(filepath)
    assert width >= 1 and width < 65536
    assert height >= 1 and height < 65536
    assert (width & (width - 1)) == 0
    assert (height & (height - 1)) == 0
    result = bytearray([((height.bit_length() - 1) << 4) | (width.bit_length() - 1)])
    for x in range(width):
        for y in range(height):
            result.append(pixels[(width * y) + x])
    return result

def write_flat(filepath) -> bytes:
    width, height, pixels = read_image(filepath)
    assert width == 64 and height == 64
    return pixels

def write_sprite(filepath) -> bytes:
    img = Image.open(filepath)
    # Use the grAb chunk, if present, to get the image offsets.
    offx = 0
    offy = 0
    for (chunktype, data) in img.private_chunks:
        if chunktype == b'grAb':
            offx, offy = struct.unpack('>ii', data)
    width, height = img.size
    postarrays = []
    postoffset = None
    for x in range(width):
        posts = []
        lastpoststart = 0
        for y in range(height):
            color, alpha = img.getpixel((x, y))
            if alpha == 255:
                if postoffset is None:
                    postoffset = y - lastpoststart
                    postdata = bytearray()
                postdata.append(PALETTE_CONV.index(color))
            elif postoffset is not None:
                posts.append((postoffset, postdata))
                postoffset = None
                lastpoststart = y
        if postoffset is not None:
            posts.append((postoffset, postdata))
            postoffset = None
        postarrays.append(posts)
    result = bytearray()
    result.extend(struct.pack('<hhHH', offx, offy, width, height))
    # TODO consider post compression later on?
    chunks = bytearray()
    for postarray in postarrays:
        result.extend(struct.pack('<I', len(chunks)))
        chunk = bytearray()
        for offset, data in postarray:
            chunk.extend(struct.pack('<BB', len(data), offset))
            chunk.extend(data)
        chunk.append(0)
        chunks.extend(chunk)
    result.extend(chunks)
    return result

# Game assets are stored in GZDoom directory format before being converted to
# custom formats for our engine.

def copy_flats():
    srcdir = sys.argv[1] + '/flats'
    dstdir = sys.argv[3] + '/flats'
    os.makedirs(dstdir, exist_ok=True)
    for filename in os.listdir(srcdir):
        # Get output name.
        name, _ = os.path.splitext(filename)
        name = name.lower()
        # Load PNG.
        with open(dstdir + '/' + name, 'wb') as outfile:
            outfile.write(write_flat(srcdir + '/' + filename))

def copy_patches():
    srcdir = sys.argv[1] + '/patches'
    dstdir = sys.argv[3] + '/patches'
    os.makedirs(dstdir, exist_ok=True)
    for filename in os.listdir(srcdir):
        # Get output name.
        name, _ = os.path.splitext(filename)
        name = name.lower()
        with open(dstdir + '/' + name, 'wb') as outfile:
            outfile.write(write_patch(srcdir + '/' + filename))

def copy_maps():
    srcdir = sys.argv[1] + '/maps'
    dstdir = sys.argv[3] + '/maps'
    for filename in os.listdir(srcdir):
        mapname, _ = os.path.splitext(filename)
        mapname = mapname.lower()
        # Parse WAD file.
        with open(srcdir + '/' + filename, 'rb') as wadfile:
            if wadfile.read(4) != b'PWAD':
                print('Not a PWAD', file=sys.stderr)
                exit(1)
            numlumps, infotableofs = struct.unpack('<ii', wadfile.read(8))
            for i in range(numlumps):
                wadfile.seek(infotableofs + 16 * i, io.SEEK_SET)
                filepos, size = struct.unpack('<ii', wadfile.read(8))
                name = wadfile.read(8)
                if 0 in name:
                    name = name[:name.index(0)]
                name = name.decode()
                if name == 'TEXTMAP':
                    wadfile.seek(filepos, io.SEEK_SET)
                    mapdata = handle_udmf(wadfile.read(size))
                    mapdir = dstdir + '/' + mapname
                    os.makedirs(mapdir, exist_ok=True)
                    for key in mapdata:
                        with open(mapdir + '/' + key, 'wb') as file:
                            file.write(mapdata[key])

def copy_sprites():
    srcdir = sys.argv[1] + '/sprites'
    dstdir = sys.argv[3] + '/sprites'
    os.makedirs(dstdir, exist_ok=True)
    # Iterate through sprites.
    for filename in os.listdir(srcdir):
        # Get output name.
        name, _ = os.path.splitext(filename)
        name = name.lower()
        # Load PNG.
        sprite = write_sprite(srcdir + '/' + filename)
        with open(dstdir + '/' + name, 'wb') as file:
            file.write(sprite)

try:
    shutil.rmtree(sys.argv[3])
except FileNotFoundError:
    pass

copy_sprites()
copy_flats()
copy_patches()
copy_maps()
