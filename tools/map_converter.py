#!/usr/bin/env python3

# This script converts maps into Brute's binary formats. Maps must be in PWADs and in UDMF format.
# This requires the parsimonious package to run. (TODO use python environments to simplify this)

# TODO convert sectors into sets of convex shapes or otherwise throw error on concave shapes

import io
import os
import re
import struct
import sys

UNQUOTE = re.compile(r'\\(.)')

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

if len(sys.argv) != 3:
    print('usage: {} <input PWAD> <output folder>'.format(sys.argv[0]), file=sys.stderr)
    exit(1)

def handle_udmf(content, mapname):
    udmf = content.decode()
    # Parse UDMF.
    mapdata = UdmfVisitor().visit(udmf_grammar.parse(udmf))
    # Write vertices.
    out_vertices = bytearray()
    for vertex in mapdata['vertex']:
        x = round(vertex['x'])
        y = round(vertex['y'])
        out_vertices.extend(struct.pack('<hh', x, y))
    # Get which walls belong to which sector.
    walls = [[] for _ in range(len(mapdata['sector']))]
    for linedef in mapdata['linedef']:
        if 'twosided' in linedef:
            front_sector = mapdata['sidedef'][linedef['sidefront']]['sector']
            back_sector = mapdata['sidedef'][linedef['sideback']]['sector']
            front_xoffset = mapdata['sidedef'][linedef['sidefront']].get('offsetx', 0)
            back_xoffset = mapdata['sidedef'][linedef['sideback']].get('offsetx', 0)
            walls[front_sector].append((linedef['v1'], linedef['v2'], back_sector, front_xoffset))
            walls[back_sector].append((linedef['v2'], linedef['v1'], front_sector, back_xoffset))
        else:
            front_sector = mapdata['sidedef'][linedef['sidefront']]['sector']
            front_xoffset = mapdata['sidedef'][linedef['sidefront']].get('offsetx', 0)
            walls[front_sector].append((linedef['v1'], linedef['v2'], front_sector, front_xoffset))
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
        out_sectors.extend(struct.pack('<HHHH',
            len(wall_set),
            wall_index,
            mapdata['sector'][j].get('heightfloor', 0),
            mapdata['sector'][j].get('heightceiling', 128),
        ))
        for i, wall in enumerate(wall_set):
            assert wall[1] == wall_set[(i+1)%len(wall_set)][0]
            out_walls.extend(struct.pack('<HHB', wall[0], wall[2], wall[3]))
            wall_index += 1
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
    mappath = sys.argv[2] + '/maps/' + mapname.lower()
    os.makedirs(mappath, exist_ok=True)
    with open(mappath + '/vertices', 'wb') as file:
        file.write(out_vertices)
    with open(mappath + '/sectors', 'wb') as file:
        file.write(out_sectors)
    with open(mappath + '/walls', 'wb') as file:
        file.write(out_walls)

def read_patch(content):
    width, height = struct.unpack('<HH', content[0:4])
    pixels = bytearray(width * height)
    for x in range(width):
        offset = struct.unpack('<I', content[8+4*x:12+4*x])[0]
        while (y := content[offset]) != 0xff:
            offset += 1
            length = content[offset]
            offset += 2
            for _ in range(length):
                assert 0 <= y < height
                pixels[(width * y) + x] = content[offset]
                y += 1
                offset += 1
            offset += 1
    return width, height, pixels

def write_patch(name, content):
    width, height, pixels = read_patch(content)
    assert width >= 1 and width < 65536
    assert height >= 1 and height < 65536
    assert (width & (width - 1)) == 0
    assert (height & (height - 1)) == 0
    result = bytearray([((height.bit_length() - 1) << 4) | (width.bit_length() - 1)])
    for x in range(width):
        for y in range(height):
            result.append(pixels[(width * y) + x])
    patchpath = sys.argv[2] + '/patches/'
    os.makedirs(patchpath, exist_ok=True)
    with open(patchpath + '/' + name.lower(), 'wb') as file:
        file.write(result)

def write_flat(name, content):
    # result = bytearray()
    # for y in range(64):
    #     row = bytearray()
    #     for x in range(64):
    #         result.append(content[(64 * y) + x + x1])
    #         pixel = 0
    #         for x1 in range(2):
    #             pixel <<= 4
    #             p = content[(64 * y) + x + x1]
    #             assert 0 <= p < 16
    #             pixel |= p
    #         row.append(pixel)
    #     result += row
    flatpath = sys.argv[2] + '/flats/'
    os.makedirs(flatpath, exist_ok=True)
    with open(flatpath + '/' + name.lower(), 'wb') as file:
        file.write(content)

last_map_marker = None
in_patches = False
in_flats = False
def handle_lump(name, content):
    global last_map_marker
    global in_patches
    global in_flats

    if len(name) == 5 and name[0:3] == 'MAP' and name[3:5].isdigit():
        last_map_marker = name
        return
    elif name == 'ENDMAP':
        last_map_marker = None
        return

    if not in_patches and name == 'P_START':
        in_patches = True
        return
    elif in_patches and name == 'P_END':
        in_patches = False
        return

    if not in_flats and name == 'F_START':
        in_flats = True
        return
    elif in_flats and name == 'F_END':
        in_flats = False
        return

    if last_map_marker is not None and name == 'TEXTMAP':
        handle_udmf(content, last_map_marker)
    elif in_patches:
        write_patch(name, content)
    elif in_flats:
        write_flat(name, content)

# Parse WAD file.
lumps = {}
with open(sys.argv[1], 'rb') as wadfile:
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
        wadfile.seek(filepos, io.SEEK_SET)
        handle_lump(name.decode(), wadfile.read(size))
