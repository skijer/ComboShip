#!/usr/bin/env python3
"""Generate native forest gameplay with its bounded authored presentation.

Exploration framing, modified geometry and authored additions live in the port
archives. Unchanged stock geometry and stock textures remain owned by mm.o2r;
OoT imports that base owner through its existing MmAssets loader. All optional
texture overrides therefore continue through ordinary resource resolution.
"""
import argparse
from collections.abc import Mapping
import hashlib
import importlib.util
import json
import math
from pathlib import Path, PurePosixPath
import re
import struct
from types import SimpleNamespace
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
NATIVE_SHA = 'f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59'
# Original authoring donor and the immutable stock archive supplied 2026-10-20.
# The latter preserves the native stump records/UVs and three-room framing.
NATIVE_SHAS = frozenset((NATIVE_SHA, '2f44bafe1a00f44ddaf7eb5e5c99e73a3d23771038529d6d1748819ad575dc80'))
EDIT_SHA = '7fde769411d21792f6bc6cde62ee934fb17b8e24d7a01b8d74f30069cce49a83'
FINAL_SHA = '9097583c896a11c204e4950bd9a80aa8e69fb2a42307f55f50e75d4195f30e42'
NATIVE = 'scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS'
PRIVATE = 'custom/comboship/native_forest/'
COMBINED = 'scenes/nonmq/comboship_forest/comboship_forest'
MM = 'scenes/nonmq/skiejer_forest/skiejer_forest'
OOT = 'scenes/shared/skiejer_forest_oot/skiejer_forest_oot'
SCENES = {'combined': COMBINED, 'mm': MM, 'oot': OOT}
MARKER = PRIVATE + 'materials_v1'
BARK = 'scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0'
BUILDING = 'custom/prelude/Z2_LOST_WOODS/'
RECIPES = ('recipe7624751371212635', 'recipe3635031522573470', 'recipe215851318676066')
EXCLUDED_BENCH_RECIPES = ('recipe449915211495247', 'recipe7187869513033518', 'recipe388081030655925')
AUTHORED_SHA = '65901fcc27453a915be9414c7203e8295fce36ce69b7658653bbcdf978af8d0e'
AUTHORED_SOURCE = 'custom/skiejer_forest/'
AUTHORED = PRIVATE + 'authored/'
AUTHORED_MARKER = PRIVATE + 'authored_portal_v2'
AUTHORED_PORTAL = ('ReturnRootTunnel', 'ReturnMushrooms_-1', 'ReturnMushrooms_1')
AUTHORED_TEXTURES = frozenset(
    AUTHORED + 'prelude/' + name for name in (
        *(recipe + '_tex0' for recipe in (*RECIPES, *EXCLUDED_BENCH_RECIPES)),
        'black_market_skeijer_poster', 'black_market_wall_magic_mask',
        'black_market_roof_magic_mask', 'black_market_soft_magic_mask')) | frozenset(
    AUTHORED + 'textures/' + name for name in ('portal_bark', 'lw_kinoko_01_FC00'))
MOVED = tuple(NATIVE + '_room_00Vtx_' + suffix for suffix in ('0019D8', '002820'))
EXPANDED = frozenset((0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42))


def sha(data):
    return hashlib.sha256(data).hexdigest()


def file_sha(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as source:
        for block in iter(lambda: source.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def header(kind):
    return struct.pack('<B3xIIQ', 0, kind, 0, 0xDEADBEEFDEADBEEF).ljust(64, b'\0')


def check_header(data, magic):
    if len(data) < 64 or data[0] != 0 or data[4:8] != magic or data[8:12] != bytes(4):
        raise ValueError(f'Unsupported little-endian version-zero {magic!r} resource')


class Reader:
    def __init__(self, data, offset=0):
        self.data, self.offset = data, offset

    def take(self, fmt):
        size = struct.calcsize('<' + fmt)
        if self.offset + size > len(self.data):
            raise ValueError('Truncated native resource')
        values = struct.unpack_from('<' + fmt, self.data, self.offset)
        self.offset += size
        return values[0] if len(values) == 1 else values

    def skip(self, count):
        if count < 0 or self.offset + count > len(self.data):
            raise ValueError('Truncated native resource')
        self.offset += count

    def text(self):
        count = self.take('I')
        start = self.offset
        self.skip(count)
        return self.data[start:self.offset].decode('utf-8')

    def finish(self):
        if self.offset != len(self.data):
            raise ValueError('Native resource has trailing bytes')


def string(value):
    data = value.encode('utf-8')
    return struct.pack('<I', len(data)) + data


def read_string(data):
    reader = Reader(data)
    value = reader.text()
    reader.finish()
    return value


def _mesh(reader):
    data, kind, count = reader.take('3B')
    if kind not in (0, 2):
        raise ValueError(f'Unsupported stock forest mesh type {kind}')
    entries = []
    for _ in range(count):
        entry = {'type': reader.take('B')}
        if kind == 2:
            entry['bounds'] = reader.take('4h')
        entry['opaque'], entry['translucent'] = reader.text(), reader.text()
        entries.append(entry)
    return {'data': data, 'type': kind, 'entries': entries}


def read_mesh(data):
    reader = Reader(data)
    mesh = _mesh(reader)
    reader.finish()
    return mesh


def write_mesh(mesh):
    entries = mesh['entries']
    result = struct.pack('<3B', mesh['data'], mesh['type'], len(entries))
    for entry in entries:
        result += struct.pack('<B', entry['type'])
        if mesh['type'] == 2:
            result += struct.pack('<4h', *entry['bounds'])
        result += string(entry['opaque']) + string(entry['translucent'])
    return result


def read_scene(data, oot=False):
    check_header(data, b'MORO')
    reader, result = Reader(data, 64), {}
    for _ in range(reader.take('I')):
        kind = reader.take('I')
        if kind in result:
            raise ValueError(f'Duplicate native scene command {kind}')
        start = reader.offset
        if kind in (0, 1, 14, 27):
            reader.skip(reader.take('I') * 16)
        elif kind == 2:
            for _ in range(reader.take('I')):
                reader.take('H')
                reader.skip(reader.take('H') * 6)
        elif kind in (3, 26):
            reader.text()
        elif kind == 4:
            for _ in range(reader.take('I')):
                reader.text()
                reader.take('II')
        elif kind in (6, 11, 19):
            reader.skip(reader.take('I') * 2)
        elif kind == 7:
            reader.take('bh')
        elif kind == 8:
            reader.take('bi' if oot else '6b')
        elif kind == 10:
            _mesh(reader)
        elif kind == 15:
            reader.skip(reader.take('I') * 22)
        elif kind in (16, 21):
            reader.take('3B')
        elif kind == 17:
            reader.take('4B')
        elif kind == 18:
            reader.take('2B')
        elif kind in (20, 25):
            pass
        elif kind == 22:
            reader.take('B')
        elif kind == 24:
            for _ in range(reader.take('I')):
                reader.text()
        elif kind == 28:
            count = reader.take('I')
            reader.take('H')
            reader.skip(count * 10)
        elif kind == 30:
            reader.skip(reader.take('I') * 10)
        elif kind == 31:
            for _ in range(reader.take('B')):
                reader.text()
                reader.take('HBB')
        else:
            raise ValueError(f'Unsupported native forest scene command {kind:#x}')
        result[kind] = data[start:reader.offset]
    reader.finish()
    return result


def write_scene(commands):
    return (header(0x4F524F4D) + struct.pack('<I', len(commands)) +
            b''.join(struct.pack('<I', key) + value for key, value in commands.items()))


def read_room_list(data):
    reader = Reader(data)
    rooms = [(reader.text(), reader.take('I'), reader.take('I')) for _ in range(reader.take('I'))]
    reader.finish()
    return rooms


def read_actor_list(data):
    reader = Reader(data)
    actors = [reader.take('HhhhhhhH') for _ in range(reader.take('I'))]
    reader.finish()
    return actors


def crc64(value):
    result = 0xFFFFFFFFFFFFFFFF
    for byte in value.encode():
        result ^= byte << 56
        for _ in range(8):
            result = ((result << 1) ^ (0x42F0E1EBA9EA3693 if result >> 63 else 0)) & 0xFFFFFFFFFFFFFFFF
    return result


def display_commands(data, path):
    check_header(data, b'TLDO')
    if len(data) < 80 or data[64] != 4 or (len(data) - 72) % 8:
        raise ValueError(f'{path}: unsupported F3DEX2 display list')
    offset = 72
    while offset < len(data):
        w0, w1 = struct.unpack_from('<II', data, offset)
        opcode = w0 >> 24
        width = 16 if opcode in EXPANDED else 8
        if offset + width > len(data):
            raise ValueError(f'{path}: truncated expanded display-list command')
        yield offset, w0, w1, opcode
        offset += width
        if opcode == 0xDF:
            if offset != len(data):
                raise ValueError(f'{path}: trailing display-list commands')
            return
    raise ValueError(f'{path}: missing native EndDisplayList')


def display_dependencies(data, path, hashes=None):
    result = {}
    for offset, _, _, opcode in display_commands(data, path):
        if opcode not in EXPANDED:
            continue
        hi, lo = struct.unpack_from('<II', data, offset + 8)
        value = (hi << 32) | lo
        if hashes is not None:
            if value not in hashes:
                raise ValueError(f'{path}: unresolved native resource hash {value:016x}')
            value = hashes[value]
        result[offset] = value
    return result


def vertex_positions(data):
    if data[4:8] == b'RRAO':
        check_header(data, b'RRAO')
        kind, count = struct.unpack_from('<II', data, 64)
        if kind != 25:
            raise ValueError('Expected native vertex array')
        offset = 72
    else:
        raise ValueError('Expected native vertex array')
    if len(data) != offset + count * 16:
        raise ValueError('Native vertex array length mismatch')
    return [struct.unpack_from('<3h', data, offset + index * 16) for index in range(count)]


def bounding_sphere(points):
    if not points:
        raise ValueError('Cannot bound empty native geometry')
    center = tuple(round((min(p[i] for p in points) + max(p[i] for p in points)) / 2) for i in range(3))
    radius = math.ceil(max(math.sqrt(sum((p[i] - center[i]) ** 2 for i in range(3))) for p in points)) + 1
    if any(abs(value) > 32767 for value in (*center, radius)):
        raise ValueError('Native forest culling bounds exceed signed 16-bit range')
    return (*center, radius)


def playable_collision(data):
    check_header(data, b'LOCO')
    reader, result = Reader(data, 64), bytearray(data)
    reader.take('6h')
    reader.skip(reader.take('I') * 6)
    reader.skip(reader.take('I') * 16)
    reader.skip(reader.take('I') * 8)
    cameras = reader.take('I')
    for _ in range(cameras):
        struct.pack_into('<H', result, reader.offset, 1)
        reader.take('Hhi')
    reader.skip(reader.take('I') * 6)
    reader.skip(reader.take('I') * 14)
    reader.finish()
    return bytes(result)


def scroll_bindings(data):
    check_header(data, b'NATO')
    reader, result = Reader(data, 64), []
    count = reader.take('I')
    if not 1 <= count <= 8:
        raise ValueError('Unsupported stock animated material count')
    for index in range(count):
        segment, kind = reader.take('bh')
        # Stock Lost Woods uses the native Empty sentinel, not scroll tracks.
        # Keep it byte-identical for MM and emit no OoT segment assignments.
        if count == 1 and segment == 0 and kind == 6:
            if data[reader.offset:] not in (b'', bytes(4)):
                raise ValueError('Unexpected native empty material trailer')
            return []
        native_segment = abs(segment) + 7
        if not segment or native_segment > 15 or kind not in (0, 1) or (segment < 0) != (index == count - 1):
            raise ValueError('Unsupported stock animated material binding')
        layers = []
        for _ in range(2 if kind == 1 else 1):
            x, y, width, height = reader.take('bbBB')
            if not width or not height:
                raise ValueError('Zero native animated texture dimensions')
            layers.append({'x': x, 'y': -y, 'width': width, 'height': height})
        if any(binding['segment'] == native_segment for binding in result):
            raise ValueError('Duplicate native animated material segment')
        result.append({'segment': native_segment, 'layers': layers})
    trailer = data[reader.offset:]
    if trailer not in (b'', bytes(4)):
        raise ValueError('Unexpected native animated material trailer')
    return result


def material_blob(bindings):
    """LUS Blob v0 containing compact v1 native scroll parameters, no pixels.

    BlobFactory reads the u32 byte length at resource offset 64 and appends
    16 zero bytes in memory. Those factory padding bytes are not serialized.
    """
    if not 0 <= len(bindings) <= 8:
        raise ValueError('Unsupported native material binding count')
    payload, segments = bytearray((1, len(bindings))), set()
    for binding in bindings:
        segment, layers = binding['segment'], binding['layers']
        if type(segment) is not int or not 8 <= segment <= 15 or segment in segments or not 1 <= len(layers) <= 2:
            raise ValueError('Invalid or duplicate native material binding')
        segments.add(segment)
        payload.extend((segment, len(layers)))
        for layer in layers:
            values = [layer[key] for key in ('x', 'y', 'width', 'height')]
            if (any(type(value) is not int for value in values) or
                    any(not -128 <= value <= 127 for value in values[:2]) or
                    any(not 1 <= value <= 255 for value in values[2:])):
                raise ValueError('Unrepresentable native material layer')
            payload.extend(struct.pack('<bbBB', *values))
    return header(0x4F424C42) + struct.pack('<I', len(payload)) + payload


def _dependency_groups(core, stock):
    groups, seen = (list(core), list(stock)), set()
    for paths in groups:
        if not 1 <= len(paths) <= 4096:
            raise ValueError('Native dependency count must be between 1 and 4096 per owner')
        for path in paths:
            if not isinstance(path, str) or not safe_resource_path(path):
                raise ValueError(f'Unsafe native dependency path: {path!r}')
            if not 1 <= len(path.encode('utf-8')) <= 512:
                raise ValueError('Native dependency path must contain at most 512 UTF-8 bytes')
            if path in seen:
                raise ValueError(f'Duplicate native dependency path: {path}')
            seen.add(path)
    return tuple(sorted(paths) for paths in groups)


def dependency_blob(core, stock):
    """LUS Blob v0: version byte, LE u16 counts, then length-prefixed UTF-8 paths."""
    core, stock = _dependency_groups(core, stock)
    payload = bytearray(struct.pack('<BHH', 1, len(core), len(stock)))
    for path in (*core, *stock):
        encoded = path.encode('utf-8')
        payload.extend(struct.pack('<H', len(encoded)))
        payload.extend(encoded)
    return header(0x4F424C42) + struct.pack('<I', len(payload)) + payload


def read_dependency_blob(data):
    """Validate serialized bytes only; the runtime Blob factory adds its own padding."""
    check_header(data, b'BLBO')
    if len(data) < 73 or struct.unpack_from('<I', data, 64)[0] != len(data) - 68:
        raise ValueError('Invalid native dependency Blob framing')
    reader = Reader(data, 68)
    version, core_count, stock_count = reader.take('BHH')
    if version != 1 or not 1 <= core_count <= 4096 or not 1 <= stock_count <= 4096:
        raise ValueError('Unsupported native dependency version or count')
    groups = []
    for count in (core_count, stock_count):
        paths = []
        for _ in range(count):
            length = reader.take('H')
            if not 1 <= length <= 512:
                raise ValueError('Invalid native dependency path length')
            start = reader.offset
            reader.skip(length)
            paths.append(data[start:reader.offset].decode('utf-8'))
        groups.append(paths)
    reader.finish()
    return _dependency_groups(*groups)


def _xml(tree):
    return ET.tostring(tree, encoding='utf-8')


def building_material():
    """Neutral default appearance; optional packs can override this DL alone."""
    tree = ET.Element('DisplayList', Version='0')
    ET.SubElement(tree, 'PipeSync')
    ET.SubElement(tree, 'Texture', S='0', T='0', Level='0', Tile='0', On='0')
    ET.SubElement(tree, 'SetGeometryMode', G_ZBUFFER='1', G_SHADE='1', G_LIGHTING='1', G_SHADING_SMOOTH='1')
    ET.SubElement(tree, 'SetPrimColor', M='0', L='0', R='160', G='144', B='120', A='255')
    combine = {}
    for cycle in ('0', '1'):
        for key, value in (('A', 'G_CCMUX_PRIMITIVE'), ('B', 'G_CCMUX_0'),
                           ('C', 'G_CCMUX_SHADE'), ('D', 'G_CCMUX_0'),
                           ('Aa', 'G_ACMUX_0'), ('Ab', 'G_ACMUX_0'),
                           ('Ac', 'G_ACMUX_0'), ('Ad', 'G_ACMUX_PRIMITIVE')):
            combine[key + cycle] = value
    ET.SubElement(tree, 'SetCombineLERP', combine)
    ET.SubElement(tree, 'EndDisplayList')
    return _xml(tree)


def building_geometry(display, vertices, source_name, target_vertex):
    """Retain authored loads/triangles and call an independently owned material."""
    hashes = {crc64(source_name + '_vtx'): target_vertex}
    tree = ET.Element('DisplayList', Version='0')
    ET.SubElement(tree, 'CallDisplayList', Path=target_vertex.removesuffix('_vtx') + '_material')
    count_vertices, triangles = len(vertex_positions(vertices)), 0
    material_ops = {0x20, 0x33, 0xD7, 0xD9, 0xE2, 0xE3, 0xE6, 0xE7, 0xEF, 0xF2, 0xF3, 0xF5, 0xFA, 0xFB, 0xFC}
    for offset, w0, w1, opcode in display_commands(display, source_name):
        if opcode == 0x32:
            hi, lo = struct.unpack_from('<II', display, offset + 8)
            path = hashes.get((hi << 32) | lo)
            count = (w0 >> 12) & 0xFF
            start = ((w0 & 0xFF) >> 1) - count
            if path is None or w1 % 16 or not 1 <= count <= 32 or start < 0 or start + count > 32 or w1 // 16 + count > count_vertices:
                raise ValueError('Unsupported authored building vertex load')
            ET.SubElement(tree, 'LoadVertices', Path=path, VertexBufferIndex=str(start),
                          VertexOffset=str(w1 // 16), Count=str(count))
        elif opcode in (0x05, 0x06, 0x26):
            if opcode == 0x26:
                rows = [(w0 & 0xFFFF, (w1 >> 16) & 0xFFFF, w1 & 0xFFFF)]
            else:
                words = (w0, w1) if opcode == 0x06 else (w0,)
                rows = [tuple(((word >> shift) & 0xFF) // 2 for shift in (16, 8, 0)) for word in words]
            for row in rows:
                if any(index >= 32 for index in row):
                    raise ValueError('Unsupported authored building triangle index')
                ET.SubElement(tree, 'Triangle1', V00=str(row[0]), V01=str(row[1]), V02=str(row[2]), Flag0='0')
                triangles += 1
        elif opcode == 0xDF:
            break
        elif opcode == 0xDE:
            # FINAL's two retained building lists call their animated material
            # segments. The geometry-only fallback has no animated material.
            # Reject branches/nonzero offsets rather than dropping geometry.
            segment = w1 >> 24
            if w0 != 0xDE000000 or not 8 <= segment <= 15 or w1 != (segment << 24) | 1:
                raise ValueError('Unsupported authored building material call')
        elif opcode == 0x31:
            # FINAL appends its three bench lists to the building wall list.
            # They are explicitly outside the native building/stump scope.
            hi, lo = struct.unpack_from('<II', display, offset + 8)
            excluded = {crc64(BUILDING + name) for name in EXCLUDED_BENCH_RECIPES}
            if w0 != 0x31000000 or w1 != 0 or ((hi << 32) | lo) not in excluded:
                raise ValueError('Unsupported authored building geometry call')
        elif opcode not in material_ops:
            raise ValueError(f'Unsupported authored building command {opcode:#x}')
    if not triangles:
        raise ValueError('Empty authored building geometry')
    ET.SubElement(tree, 'PipeSync')
    ET.SubElement(tree, 'Texture', S='65535', T='65535', Level='0', Tile='0', On='1')
    ET.SubElement(tree, 'EndDisplayList')
    return _xml(tree), triangles


def _actors(data, oot):
    result = []
    for original in read_actor_list(data):
        row = list(original)
        ambient_fairies = row[0] == 0x49 and row[7] == 0
        if row[0] in (0xD, 0x191, 0x192) or (oot and row[0] != 0xB3 and not ambient_fairies):
            continue
        if oot:
            row[0], row[4], row[5], row[6] = 0x97 if ambient_fairies else 0x151, 0, 0, 0
        result.append(struct.pack('<HhhhhhhH', *row))
    return struct.pack('<I', len(result)) + b''.join(result)


def _transitions(data, oot):
    reader = Reader(data)
    count = reader.take('I')
    rows = [reader.take('4bHhhhhH') for _ in range(count)]
    reader.finish()
    if len(rows) != 2 or rows[0][4] != 24 or rows[1][4] != 5:
        raise ValueError('Expected exactly one native tunnel and one authored building door')
    result = []
    for index, source in enumerate(rows):
        row = list(source)
        if oot:
            row[4] = 0x23 if index == 0 else 9
            angle = int(((row[8] >> 7) & 0x1FF) * 65536 / 360)
            row[8] = ((angle + 32768) & 65535) - 32768
            row[9] = 0x100 if index == 0 else 0
        result.append(struct.pack('<4bHhhhhH', *row))
    return struct.pack('<I', len(result)) + b''.join(result)


def _f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def _native_sine(angle):
    # Match Math_SinS/sins, including its table reflection and float rounding.
    source = (ROOT / 'mm/src/libultra/gu/sins.c').read_text()
    table = source[source.index('{', source.index('sintable')) + 1:source.index('};')]
    values = [int(value, 16) for value in re.findall(r'0x[0-9A-Fa-f]+', table)]
    if len(values) != 1024:
        raise ValueError('Inspect the changed native sine table before regenerating the portal')
    index = (angle & 65535) >> 4
    value = values[1023 - (index & 1023) if index & 1024 else index & 1023]
    if index & 2048:
        value = -value
    return _f32(value * _f32(1.0 / 32767.0))


def portal_resources():
    """Exact BuildPassage local geometry, with its native fixed-point matrix."""
    vertices = []

    def quad(points, shade):
        for x, y, z in points:
            vertices.append((int(x), int(y), int(z), 0, int(_f32(x * 32)), int(_f32(-y * 32)),
                             shade, shade, shade, 255))

    quad([(-68, 0, 0), (-44, 0, 0), (-44, 65, 0), (-68, 65, 0)], 200)
    quad([(44, 0, 0), (68, 0, 0), (68, 65, 0), (44, 65, 0)], 200)
    for index in range(10):
        a = _f32(_f32(index * _f32(3.14159265)) / 10)
        b = _f32(_f32((index + 1) * _f32(3.14159265)) / 10)
        ax, ay, bx, by = (_f32(math.cos(a)), _f32(math.sin(a)), _f32(math.cos(b)), _f32(math.sin(b)))
        inner_a = (_f32(ax * 44), _f32(65 + _f32(ay * 44)))
        outer_a = (_f32(ax * 68), _f32(65 + _f32(ay * 68)))
        inner_b = (_f32(bx * 44), _f32(65 + _f32(by * 44)))
        outer_b = (_f32(bx * 68), _f32(65 + _f32(by * 68)))
        quad([(*inner_a, 0), (*outer_a, 0), (*outer_b, 0), (*inner_b, 0)], 200)
        quad([(*inner_a, 0), (*inner_b, 0), (*inner_b, -45), (*inner_a, -45)], 65)
    quad([(-44, 0, -2), (44, 0, -2), (44, 110, -2), (-44, 110, -2)], 0)

    sine, cosine = _native_sine(12288), _native_sine(12288 + 0x4000)
    matrix = [(cosine, 0, -sine, 0), (0, 1, 0, 0), (sine, 0, cosine, 0), (-695, 0, -313, 1)]
    fixed = [int(_f32(value * 65536)) for row in matrix for value in row]
    integer, fraction = [], []
    for first, second in zip(fixed[::2], fixed[1::2]):
        integer.append((first & 0xFFFF0000) | ((second >> 16) & 65535))
        fraction.append(((first << 16) & 0xFFFF0000) | (second & 65535))
    packed_matrix = header(0x4F4D5458) + struct.pack('<16I', *integer, *fraction)
    matrix = [fixed[index:index + 4] for index in range(0, 16, 4)]
    world = [tuple((sum(vertex[k] * matrix[k][axis] for k in range(3)) + matrix[3][axis]) / 65536
                   for axis in range(3)) for vertex in vertices]

    material = ET.Element('DisplayList', Version='0')
    ET.SubElement(material, 'PipeSync')
    ET.SubElement(material, 'ClearGeometryMode', G_LIGHTING='1', G_CULL_BOTH='1')
    ET.SubElement(material, 'Texture', S='65535', T='65535', Level='0', Tile='0', On='1')
    combine = {}
    for cycle in ('0', '1'):
        for key, value in (('A', 'G_CCMUX_TEXEL0'), ('B', 'G_CCMUX_0'), ('C', 'G_CCMUX_SHADE'), ('D', 'G_CCMUX_0'),
                           ('Aa', 'G_ACMUX_0'), ('Ab', 'G_ACMUX_0'), ('Ac', 'G_ACMUX_0'), ('Ad', 'G_ACMUX_SHADE')):
            combine[key + cycle] = value
    ET.SubElement(material, 'SetCombineLERP', combine)
    ET.SubElement(material, 'LoadTextureBlock', Path=BARK, Format='0', Size='2', Width='32', Height='32',
                  MaskS='5', MaskT='5', ShiftS='0', ShiftT='0', CMS_TXWrap='1', CMT_TXWrap='1')
    ET.SubElement(material, 'EndDisplayList')

    root = ET.Element('DisplayList', Version='0')
    ET.SubElement(root, 'Matrix', Path=PRIVATE + 'portal_transform', Param='G_MTX_PUSH')
    ET.SubElement(root, 'CallDisplayList', Path=PRIVATE + 'portal_material')
    for index in range(23):
        ET.SubElement(root, 'LoadVertices', Path=PRIVATE + 'portal_vtx', VertexBufferIndex='0',
                      VertexOffset=str(index * 4), Count='4')
        for a, b, c in ((0, 1, 2), (0, 2, 3)):
            ET.SubElement(root, 'Triangle1', V00=str(a), V01=str(b), V02=str(c), Flag0='0')
    ET.SubElement(root, 'PopMatrix', Param='G_MTX_MODELVIEW')
    ET.SubElement(root, 'PipeSync')
    ET.SubElement(root, 'SetGeometryMode', G_LIGHTING='1', G_CULL_BACK='1')
    ET.SubElement(root, 'EndDisplayList')
    resources = {
        PRIVATE + 'portal_root': _xml(root), PRIVATE + 'portal_material': _xml(material),
        PRIVATE + 'portal_transform': packed_matrix,
        PRIVATE + 'portal_vtx': header(0x4F415252) + struct.pack('<II', 25, len(vertices)) +
        b''.join(struct.pack('<hhhHhhBBBB', *vertex) for vertex in vertices),
    }
    return resources, bounding_sphere(world), {
        'anchor': [-695, 0, -313], 'yaw': 12288, 'vertices': 92, 'quads': 23, 'triangles': 46,
        'source': 'mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp::BuildPassage',
        'material': PRIVATE + 'portal_material', 'stock_texture': BARK,
        'transform': 'native Matrix_RotateYS and guMtxF2L; local vertices retain exact native integer conversion'}


def native_stock_paths():
    """Stable external essentials; real graph dependencies are donor-derived."""
    return [BARK]


def _scene_paths(mode):
    if mode not in SCENES:
        raise ValueError(f'Unknown native forest mode: {mode}')
    prefix = SCENES[mode]
    return {prefix, prefix + '_collision', *(prefix + f'_room_{i}' for i in range(3))}


def dependency_path(mode):
    _scene_paths(mode)
    return PRIVATE + f'dependencies_{mode}_v1'


def dependency_inventory(resources, receipt, mode):
    """Every generated common resource and one host's headers, with stock ownership separate."""
    own_scene = _scene_paths(mode)
    if not own_scene <= resources.keys():
        raise ValueError(f'Missing native dependency scene resources for {mode}')
    all_scenes = set().union(*(_scene_paths(host) for host in SCENES))
    descriptors = {dependency_path(host) for host in SCENES}
    common = set(resources) - all_scenes - descriptors
    return _dependency_groups(common | own_scene, receipt['external_resources'])


def native_core_paths(mode=None):
    """Stable private core, optionally including one host's scene framing."""
    paths = {MARKER, PRIVATE + 'stock_materials', PRIVATE + 'texanim', PRIVATE + 'building_root',
             *(PRIVATE + name for name in ('portal_root', 'portal_material', 'portal_transform', 'portal_vtx')),
             *(PRIVATE + 'geometry/' + name.rsplit('/', 1)[1] for name in MOVED),
             *(PRIVATE + 'building/' + recipe + suffix for recipe in RECIPES for suffix in ('', '_vtx', '_material'))}
    if mode is not None:
        paths.update(_scene_paths(mode))
    return sorted(paths)


def native_core_header(resources=None):
    """Fixed control contract, or the complete prepared presentation contract."""
    result = '#pragma once\n// Generated by tools/black_market/native_forest.py::native_core_header.\n'
    result += '// Private core and external stock essentials have separate owners.\nnamespace NativeForestResources {\n'
    common = native_core_paths()
    if resources is not None:
        excluded = {dependency_path(mode) for mode in SCENES} | set().union(*(_scene_paths(mode) for mode in SCENES))
        common = sorted(set(resources) - excluded)
    version = 2 if resources is not None and AUTHORED_MARKER in resources else 1
    result += f'inline constexpr unsigned kPresentationVersion = {version};\n'
    result += f'inline constexpr const char* kAuthoredPortalMarker = "{AUTHORED_MARKER}";\n'
    result += 'inline constexpr float kAuthoredPortalRadius = 68.0f, kAuthoredPortalCenterY = 66.0f;\n'
    for name, paths in (('kCommon', common), ('kStock', native_stock_paths())):
        result += f'inline constexpr const char* {name}[] = {{\n'
        result += ''.join(f'    "{path}",\n' for path in paths) + '};\n'
    for name, path in (('kCombinedScene', COMBINED), ('kMmSoloScene', MM), ('kOotSoloScene', OOT)):
        result += f'inline constexpr const char* {name} = "{path}";\n'
    for name, mode in (('kCombinedDependencies', 'combined'), ('kMmSoloDependencies', 'mm'),
                       ('kOotSoloDependencies', 'oot')):
        result += f'inline constexpr const char* {name} = "{dependency_path(mode)}";\n'
    return result + '} // namespace NativeForestResources\n'


def build_resources(native, edit, final):
    """Pure transformation; CLI identity gates are in convert(), never bypassed."""
    hashes = {crc64(name): name for name in native}
    if len(hashes) != len(native):
        raise ValueError('Native archive has resource hash collisions')
    resources, external, textures, resolved, points, touched = {}, set(), set(), {}, {}, {}
    active = set()

    def visit(name):
        if name in resolved:
            return resolved[name]
        if name in active:
            raise ValueError(f'Recursive stock display list: {name}')
        if name not in native:
            raise ValueError(f'Missing native MM dependency: {name}')
        active.add(name)
        source = native[name]
        target = PRIVATE + 'geometry/' + name.rsplit('/', 1)[1]
        if name in MOVED:
            replacement = edit.get(name)
            if replacement is None:
                raise ValueError(f'Missing native stump vertex edit: {name}')
            points[name] = vertex_positions(replacement)
            resources[target], resolved[name], touched[name] = replacement, target, True
        elif source[4:8] == b'XETO':
            external.add(name)
            textures.add(name)
            resolved[name], points[name], touched[name] = name, [], False
        elif source[4:8] == b'TLDO':
            refs = display_dependencies(source, name, hashes)
            result, collected, changed = bytearray(source), [], False
            for offset, _, _, opcode in display_commands(source, name):
                if opcode not in EXPANDED or opcode == 0x33:
                    continue
                dependency = refs[offset]
                mapped = visit(dependency)
                collected.extend(points[dependency])
                changed |= touched[dependency]
                if mapped != dependency:
                    value = crc64(mapped)
                    struct.pack_into('<II', result, offset + 8, value >> 32, value & 0xFFFFFFFF)
            if changed:
                for offset, _, _, opcode in display_commands(source, name):
                    if opcode == 0x33:
                        value = crc64(target)
                        struct.pack_into('<II', result, offset + 8, value >> 32, value & 0xFFFFFFFF)
                resources[target], resolved[name] = bytes(result), target
            else:
                external.add(name)
                resolved[name] = name
            points[name], touched[name] = collected, changed
        else:
            # Render arrays/matrices/lights remain in the normal MM owner.
            external.add(name)
            resolved[name], touched[name] = name, False
            is_vertices = (source[4:8] == b'RRAO' and len(source) >= 72 and
                           struct.unpack_from('<I', source, 64)[0] == 25)
            points[name] = vertex_positions(source) if is_vertices else []
        active.remove(name)
        return resolved[name]

    source_scene = read_scene(native[NATIVE])
    rooms = read_room_list(source_scene[4])
    if len(rooms) != 3:
        raise ValueError('The original native forest must retain all three rooms')
    native_rooms, bounds_changes = [], []
    for room_number, (name, _, _) in enumerate(rooms):
        commands = read_scene(native[name])
        mesh = read_mesh(commands[10])
        if room_number < 2 and mesh['type'] != 2:
            raise ValueError('The two native exploration rooms must retain their type2 culling mesh')
        for entry_index, entry in enumerate(mesh['entries']):
            collected, changed = [], False
            for key in ('opaque', 'translucent'):
                name = entry[key]
                if name:
                    entry[key] = visit(name)
                    collected.extend(points[name])
                    changed |= touched[name]
            if changed and mesh['type'] == 2:
                before = entry['bounds']
                entry['bounds'] = bounding_sphere(collected)
                bounds_changes.append({'room': room_number, 'entry': entry_index,
                                       'before': list(before), 'after': list(entry['bounds'])})
        commands[10] = write_mesh(mesh)
        native_rooms.append(commands)
    if any(name not in resolved for name in MOVED):
        raise ValueError('Moved native stump vertices are not reachable from the original room mesh')

    building = ET.Element('DisplayList', Version='0')
    building_points, building_triangles, building_materials = [], 0, {}
    for recipe in RECIPES:
        original = BUILDING + recipe
        source_vtx = final.get(original + '_vtx', edit[original + '_vtx'])
        source_dl = final.get(original, edit[original])
        target_vtx = PRIVATE + 'building/' + recipe + '_vtx'
        target_dl = PRIVATE + 'building/' + recipe
        data, count = building_geometry(source_dl, source_vtx, original, target_vtx)
        resources[target_vtx], resources[target_dl] = source_vtx, data
        material = target_dl + '_material'
        resources[material] = building_material()
        building_materials[original] = material
        building_points.extend(vertex_positions(source_vtx))
        building_triangles += count
        ET.SubElement(building, 'CallDisplayList', Path=target_dl)
    ET.SubElement(building, 'EndDisplayList')
    resources[PRIVATE + 'building_root'] = _xml(building)
    room0 = read_mesh(native_rooms[0][10])
    room0['entries'].append({'type': 2, 'bounds': bounding_sphere(building_points),
                             'opaque': PRIVATE + 'building_root', 'translucent': ''})
    portal, portal_bounds, portal_report = portal_resources()
    resources.update(portal)
    visit(BARK)
    room0['entries'].append({'type': 2, 'bounds': portal_bounds, 'opaque': PRIVATE + 'portal_root', 'translucent': ''})
    native_rooms[0][10] = write_mesh(room0)

    animation_path = read_string(source_scene[26])
    if animation_path not in native:
        raise ValueError('Missing original stock texture animation')
    animation = native[animation_path]
    bindings = scroll_bindings(animation)
    resources[PRIVATE + 'texanim'] = animation
    resources[PRIVATE + 'stock_materials'] = material_blob(bindings)
    resources[MARKER] = b'Stock native forest materials v1\n'
    final_scene = read_scene(final['alt/' + NATIVE])
    collision_path = read_string(final_scene[3])
    collision = playable_collision(final['alt/' + collision_path])

    for prefix in (COMBINED, MM, OOT):
        oot = prefix == OOT
        spawn = struct.pack('<HhhhhhhH', 0, -486, 0, -227, 0 if oot else 7,
                            12288 if oot else (68 << 7) | 127, 0 if oot else 127, 0x0DFF if oot else 0x0EFF)
        commands = {
            21: bytes((0, 4, 0x3E)) if oot else source_scene[21],
            4: struct.pack('<I', 3) + b''.join(string(prefix + f'_room_{i}') + bytes(8) for i in range(3)),
            14: _transitions(final_scene[14], oot),
            3: string(prefix + '_collision'),
            6: struct.pack('<Ibb', 1, 0, 0),
            7: struct.pack('<bh', 0, 2),
            0: struct.pack('<I', 1) + spawn,
            17: bytes(4) if oot else source_scene[17],
            19: (source_scene[19] if prefix == COMBINED else struct.pack('<IH', 1, 0x61C if oot else 0xC000)),
            15: source_scene[15],
        }
        if not oot:
            commands[26] = string(PRIVATE + 'texanim')
        commands[20] = b''
        resources[prefix], resources[prefix + '_collision'] = write_scene(commands), collision
        for index, original in enumerate(native_rooms):
            room = {key: original[key] for key in (22, 8, 18, 16, 10) if key in original}
            room[8] = struct.pack('<bi', 0, 0) if oot else original[8]
            room[16], room[11] = bytes((255, 255, 0)), struct.pack('<I', 0)
            room[1], room[20] = _actors(original.get(1, struct.pack('<I', 0)), oot), b''
            resources[prefix + f'_room_{index}'] = write_scene(room)

    report = {'schema': 1, 'status': 'generated', 'native_stock_rooms': 3,
              'external_owner': 'normal mm.o2r; OoT uses the existing MmAssets base archive loader',
              'external_resources': sorted(external), 'external_textures': sorted(textures),
              'native_private_geometry': sorted(name for name in resources if name.startswith(PRIVATE + 'geometry/')),
              'culling_bounds': bounds_changes, 'building_triangles': building_triangles,
              'building_material': 'untextured native vertex shading', 'scroll_bindings': bindings,
              'building_materials': building_materials,
              'portal': portal_report,
              'excluded_final_bench_meshes': list(EXCLUDED_BENCH_RECIPES),
              'marker': MARKER, 'scene_paths': {'combined': COMBINED, 'mm': MM, 'oot': OOT},
              'dependency_paths': {mode: dependency_path(mode) for mode in SCENES},
              'custom_texture_payloads': 0,
              'runtime': 'unverified; source/resource generation does not establish configured gameplay'}
    for mode in SCENES:
        resources[dependency_path(mode)] = dependency_blob(*dependency_inventory(resources, report, mode))
    return resources, report


def _authored_target(path):
    tail = path.removeprefix(AUTHORED_SOURCE)
    if tail in {f'prelude/{recipe}{suffix}' for recipe in RECIPES for suffix in ('', '_vtx')}:
        return PRIVATE + 'building/' + tail.removeprefix('prelude/')
    return AUTHORED + tail


def _material_prefix(data, name):
    """Exact authored material before geometry; existing native triangles own drawing."""
    pieces = []
    allowed = {0x20, 0xD7, 0xD9, 0xDE, 0xE2, 0xE3, 0xE6, 0xE7,
               0xEF, 0xF2, 0xF3, 0xF5, 0xFA, 0xFB, 0xFC}
    for offset, _, _, opcode in display_commands(data, name):
        if opcode == 0x32:
            return data[:72] + b''.join(pieces) + struct.pack('<II', 0xDF000000, 0)
        if opcode == 0x33:
            continue
        if opcode not in allowed:
            raise ValueError(f'{name}: unexpected authored material command {opcode:#x}')
        pieces.append(data[offset:offset + (16 if opcode in EXPANDED else 8)])
    raise ValueError(f'{name}: no authored geometry after material')


def _calls(paths):
    root = ET.Element('DisplayList', Version='0')
    for path in paths:
        ET.SubElement(root, 'CallDisplayList', Path=path)
    ET.SubElement(root, 'EndDisplayList')
    return _xml(root)


def bake_presentation(baseline, receipt, donor):
    """Bake only the exact authored additions; input dict/archive are immutable.

    Idempotent for a current source bundle. The geometry-only native transform
    remains an independent control; this step never imports donor gameplay or
    full-terrain display lists.
    """
    if file_sha(donor) != AUTHORED_SHA:
        raise ValueError('Expected immutable authored forest SHA256 ' + AUTHORED_SHA)
    resources = dict(baseline)
    report = {key: value for key, value in receipt.items() if key not in ('resources', 'bundle')}
    with zipfile.ZipFile(donor) as archive:
        indexed = ArchiveResources(archive)
        hashes = {crc64(name): name for name in indexed}
        roots = [AUTHORED_SOURCE + 'prelude/' + recipe for recipe in RECIPES]
        roots += [AUTHORED_SOURCE + 'prelude/' + name for name in (
            'paste2461349930323740', 'black_magic_walls', 'black_magic_roof',
            'black_magic_bench_0', 'black_magic_bench_1', 'black_magic_bench_2', 'mirror_stump_violet_shimmer')]
        roots += [AUTHORED_SOURCE + 'room_0/' + name for name in (
            *AUTHORED_PORTAL, 'ReturnTunnelMouth', 'ReturnTunnelPinkLight')]
        source, active = {}, set()

        def collect(path):
            if path in source:
                return
            if path in active or not path.startswith(AUTHORED_SOURCE):
                raise ValueError('Recursive or non-private authored dependency: ' + path)
            if path.startswith(AUTHORED_SOURCE + 'room_1/') or (
                    path.startswith(AUTHORED_SOURCE + 'room_0/Geometry') and not path.endswith('_vtx')):
                raise ValueError('Full MM3D terrain is outside the native authored subset: ' + path)
            active.add(path)
            data = indexed[path]
            refs = []
            if data.startswith(b'<DisplayList'):
                refs = [node.get('Path') for node in ET.fromstring(data) if node.get('Path')]
            elif data[4:8] == b'TLDO':
                refs = [value for offset, value in display_dependencies(data, path, hashes).items()
                        if data[offset + 3] != 0x33]
            for dependency in refs:
                collect(dependency)
            active.remove(path)
            source[path] = data

        for path in roots:
            collect(path)
        if len(source) != 45 or len({path for path, data in source.items() if data[4:8] == b'XETO'}) != 12:
            raise ValueError('Immutable authored resource closure differs from the 45-resource contract')
        mapping = {path: _authored_target(path) for path in source}
        for recipe in RECIPES:
            original = AUTHORED_SOURCE + 'prelude/' + recipe
            if resources.get(mapping[original + '_vtx']) != source[original + '_vtx']:
                raise ValueError('Authored material would change native shop vertices: ' + recipe)
        for path, data in source.items():
            target = mapping[path]
            if path in {AUTHORED_SOURCE + 'prelude/' + recipe + '_vtx' for recipe in RECIPES}:
                continue
            if data.startswith(b'<DisplayList'):
                tree = ET.fromstring(data)
                for node in tree:
                    if node.get('Path'):
                        node.set('Path', mapping[node.get('Path')])
                data = _xml(tree)
            elif data[4:8] == b'TLDO':
                rewritten = bytearray(data)
                for offset, w0, w1, opcode in display_commands(data, path):
                    if opcode in EXPANDED:
                        hi, lo = struct.unpack_from('<II', data, offset + 8)
                        value = crc64(mapping[hashes[(hi << 32) | lo]])
                        struct.pack_into('<II', rewritten, offset + 8, value >> 32, value & 0xFFFFFFFF)
                    elif opcode == 0xDE and (w0 != 0xDE000000 or w1 != ((w1 >> 24) << 24) | 1 or
                                             w1 >> 24 not in range(8, 14)):
                        raise ValueError('Invalid authored scroll-segment call: ' + path)
                    elif opcode == 0xFA and path == AUTHORED_SOURCE + 'room_0/ReturnTunnelMouth':
                        if w1 != 0x7C28B860:
                            raise ValueError('Unexpected authored portal primitive color')
                        struct.pack_into('<I', rewritten, offset + 4, 0x44FF7460)
                data = bytes(rewritten)
            if path in {AUTHORED_SOURCE + 'prelude/' + recipe for recipe in RECIPES}:
                target += '_material'
                data = _material_prefix(data, path)
            resources[target] = data
        animation = indexed[AUTHORED_SOURCE + 'final_texanim']

    building = []
    for recipe in RECIPES:
        building.append(PRIVATE + 'building/' + recipe)
        if recipe == 'recipe3635031522573470':
            building.extend(AUTHORED + 'prelude/' + bench for bench in EXCLUDED_BENCH_RECIPES)
    building.append(AUTHORED + 'prelude/paste2461349930323740')
    resources[PRIVATE + 'building_root'] = _calls(building)
    resources[PRIVATE + 'building_xlu'] = _calls(AUTHORED + 'prelude/' + name for name in (
        'black_magic_walls', 'black_magic_roof', 'black_magic_bench_0', 'black_magic_bench_1', 'black_magic_bench_2'))
    resources[PRIVATE + 'portal_root'] = _calls(AUTHORED + 'room_0/' + name for name in AUTHORED_PORTAL)
    resources[PRIVATE + 'portal_xlu'] = _calls(AUTHORED + 'room_0/' + name for name in (
        'ReturnTunnelMouth', 'ReturnTunnelPinkLight'))
    resources[PRIVATE + 'stump_shimmer_root'] = _calls([AUTHORED + 'prelude/mirror_stump_violet_shimmer'])
    resources[AUTHORED_MARKER] = b'Native authored portal v2; radius 68, center y 66\n'
    resources[PRIVATE + 'texanim'] = animation
    bindings = scroll_bindings(animation)
    if [binding['segment'] for binding in bindings] != list(range(8, 14)):
        raise ValueError('Authored presentation requires all six scroll tracks')
    resources[PRIVATE + 'stock_materials'] = material_blob(bindings)

    target_hashes = {crc64(name): name for name in resources}
    cache, active = {}, set()
    def points(path):
        if not path:
            return []
        if path in cache:
            return cache[path]
        if path in active or path not in resources:
            raise ValueError('Missing or recursive native presentation geometry: ' + path)
        active.add(path)
        data, result = resources[path], []
        if data.startswith(b'<Vertex'):
            result = [tuple(int(node.get(axis)) for axis in ('X', 'Y', 'Z')) for node in ET.fromstring(data)]
        elif data[4:8] == b'RRAO':
            result = vertex_positions(data)
        elif data.startswith(b'<DisplayList'):
            for node in ET.fromstring(data):
                if node.tag in ('LoadVertices', 'CallDisplayList'):
                    result.extend(points(node.get('Path')))
        elif data[4:8] == b'TLDO':
            for offset, _, _, opcode in display_commands(data, path):
                if opcode in (0x31, 0x32):
                    hi, lo = struct.unpack_from('<II', data, offset + 8)
                    result.extend(points(target_hashes[(hi << 32) | lo]))
        active.remove(path)
        cache[path] = result
        return result

    culling = {}
    for mode, prefix in SCENES.items():
        room_path = prefix + '_room_0'
        commands = read_scene(resources[room_path], oot=mode == 'oot')
        mesh = read_mesh(commands[10])
        for root, xlu in (('building_root', 'building_xlu'), ('portal_root', 'portal_xlu')):
            entries = [entry for entry in mesh['entries'] if entry['opaque'] == PRIVATE + root]
            if len(entries) != 1:
                raise ValueError('Expected exactly one native draw root: ' + root)
            entry = entries[0]
            entry['translucent'] = PRIVATE + xlu
            entry['bounds'] = bounding_sphere(points(PRIVATE + root) + points(PRIVATE + xlu))
        shimmer = {'type': 2, 'bounds': bounding_sphere(points(PRIVATE + 'stump_shimmer_root')),
                   'opaque': '', 'translucent': PRIVATE + 'stump_shimmer_root'}
        existing = [entry for entry in mesh['entries'] if entry['translucent'] == PRIVATE + 'stump_shimmer_root']
        if len(existing) > 1:
            raise ValueError('Duplicate native stump-shimmer entry')
        if existing:
            existing[0].update(shimmer)
        else:
            mesh['entries'].append(shimmer)
        commands[10] = write_mesh(mesh)
        resources[room_path] = write_scene(commands)
        culling[mode] = {entry['opaque'] or entry['translucent']: list(entry['bounds'])
                         for entry in mesh['entries'] if entry['opaque'] in (PRIVATE + 'building_root', PRIVATE + 'portal_root')
                         or entry['translucent'] == PRIVATE + 'stump_shimmer_root'}

    original_bundle = receipt.get('presentation', {}).get('baseline_bundle', receipt.get('bundle'))
    control_portal = receipt.get('presentation', {}).get('control_portal', receipt.get('portal'))
    report.update(building_material='exact authored textured/scrolled materials; native geometry owner retained',
                  custom_texture_payloads=12, scroll_bindings=bindings, excluded_final_bench_meshes=[])
    report['presentation'] = {
        'version': 2, 'marker': AUTHORED_MARKER,
        'donor': {'name': Path(donor).name, 'sha256': AUTHORED_SHA}, 'baseline_bundle': original_bundle,
        'control_portal': control_portal,
        'sources': sorted(source), 'native_shop_vertex_arrays_reused': 3,
        'portal': {'root': PRIVATE + 'portal_root', 'radius': 68, 'center_y': 66,
                   'anchor': [-695, 0, -313], 'yaw': 12288, 'world_space_vertices': True,
                   'mouth_rgba_before': [124, 40, 184, 96], 'mouth_rgba': [68, 255, 116, 96]},
        'culling': culling, 'terrain_display_lists': 0,
        'preservation': 'Native gameplay framing/collision and shop geometry unchanged; authored pixels/vertices/scrolls exact; only mouth RGB changed',
        'runtime': 'unverified; archive/source checks are not configured gameplay acceptance',
    }
    report['portal'] = dict(report['presentation']['portal'])
    report['inputs'] = {**report.get('inputs', {}), 'authored': report['presentation']['donor']}
    for mode in SCENES:
        resources[dependency_path(mode)] = dependency_blob(*dependency_inventory(resources, report, mode))
    validate_resources(resources, report)
    return resources, report


def bake_mirror_access(baseline, receipt):
    """Apply the separately reviewed additive fallen-log access recipe."""
    if AUTHORED_MARKER not in baseline:
        raise ValueError('Mirror access requires the authored native presentation and bark owner')
    spec = importlib.util.spec_from_file_location('mirror_stump_access', Path(__file__).with_name('mirror_stump_access.py'))
    access = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(access)
    native = SimpleNamespace(SCENES=SCENES, check_header=check_header, playable_collision=playable_collision,
                             Reader=Reader, bounding_sphere=bounding_sphere, _xml=_xml, header=header)
    resources = dict(baseline)
    report = {key: value for key, value in receipt.items() if key not in ('resources', 'bundle')}
    addition = access.augment(native, resources)
    for mode, prefix in SCENES.items():
        path = prefix + '_room_0'
        commands = read_scene(resources[path], oot=mode == 'oot')
        mesh = read_mesh(commands[10])
        if mesh['type'] != 2:
            raise ValueError('Mirror access requires native type-two room culling')
        entry = {'type': 2, 'bounds': tuple(addition['bounds']), 'opaque': addition['root'], 'translucent': ''}
        existing = [value for value in mesh['entries'] if value['opaque'] == addition['root']]
        if len(existing) > 1 or (existing and existing[0] != entry):
            raise ValueError('Inconsistent native Mirror access room entry')
        if not existing:
            mesh['entries'].append(entry)
        commands[10] = write_mesh(mesh)
        resources[path] = write_scene(commands)
    report['mirror_access'] = addition
    for mode in SCENES:
        resources[dependency_path(mode)] = dependency_blob(*dependency_inventory(resources, report, mode))
    validate_resources(resources, report)
    return resources, report


def _read_archive(path, predicate=lambda name: True):
    with zipfile.ZipFile(path) as archive:
        resources = ArchiveResources(archive)
        return {name: resources[name] for name in resources if predicate(name)}


class ArchiveResources(Mapping):
    """Index the real ROM archive without expanding unrelated art into memory."""
    def __init__(self, archive):
        self.archive = archive
        names = archive.namelist()
        if len(names) != len(set(names)) or any(not safe_resource_path(name) for name in names if not name.endswith('/')):
            raise ValueError('Duplicate or unsafe native archive resource paths')
        self.names = tuple(name for name in names if not name.endswith('/'))

    def __iter__(self):
        return iter(self.names)

    def __len__(self):
        return len(self.names)

    def __getitem__(self, name):
        return self.archive.read(name)


def convert(native_mm, source_edit, final, authored=None):
    native_sha = file_sha(native_mm)
    if native_sha not in NATIVE_SHAS:
        raise ValueError('Expected immutable native MM SHA256 from the verified stock donor identities: ' +
                         ', '.join(sorted(NATIVE_SHAS)))
    for label, path, expected in (('native edit', source_edit, EDIT_SHA),
                                  ('FINAL geometry', final, FINAL_SHA)):
        if file_sha(path) != expected:
            raise ValueError(f'Expected immutable {label} SHA256 {expected}')
    edit = _read_archive(source_edit, lambda name: not name.startswith('prelude/'))
    final_data = _read_archive(final, lambda name: name in {
        'alt/' + NATIVE, 'alt/' + NATIVE + 'CollisionHeader_005DF8',
        *(BUILDING + recipe + suffix for recipe in RECIPES for suffix in ('', '_vtx'))})
    # Stock graph references may cross files. Index every name but expand only
    # dependencies actually reached by the three native forest room meshes.
    with zipfile.ZipFile(native_mm) as archive:
        resources, report = build_resources(ArchiveResources(archive), edit, final_data)
    report['inputs'] = {label: {'sha256': expected, 'name': Path(path).name}
                        for label, path, expected in (('native_mm', native_mm, native_sha),
                                                     ('source_edit', source_edit, EDIT_SHA), ('final', final, FINAL_SHA))}
    return bake_presentation(resources, report, authored) if authored is not None else (resources, report)


def safe_resource_path(name):
    if not isinstance(name, str) or any(ord(char) < 32 or ord(char) == 127 for char in name):
        return False
    path = PurePosixPath(name)
    return bool(path.parts) and not path.is_absolute() and str(path) == name and '\\' not in name and '..' not in path.parts


def validate_resources(resources, receipt=None):
    descriptors = {dependency_path(mode) for mode in SCENES}
    for name, data in resources.items():
        if not safe_resource_path(name) or not name.startswith((PRIVATE, COMBINED, MM, OOT)):
            raise ValueError(f'Outside native forest resource scope: {name}')
        if any(str(parent) in resources for parent in PurePosixPath(name).parents):
            raise ValueError(f'Native forest file/directory collision: {name}')
        if data[4:8] == b'XETO':
            if name not in AUTHORED_TEXTURES or AUTHORED_MARKER not in resources:
                raise ValueError(f'Native forest bundle forbids unlisted texture payloads: {name}')
            continue
        if name.endswith(('.png', '.jpg')):
            raise ValueError(f'Native forest bundle forbids image files: {name}')
        if name == MARKER and data == b'Stock native forest materials v1\n':
            continue
        if name == AUTHORED_MARKER and data == b'Native authored portal v2; radius 68, center y 66\n':
            continue
        if data.startswith(b'<'):
            tag = ET.fromstring(data).tag
            if tag != 'DisplayList' and not (tag == 'Vertex' and name.startswith(AUTHORED) and name.endswith('_vtx')):
                raise ValueError(f'Unsupported native forest XML encoding: {name}')
        elif data[4:8] == b'BLBO':
            check_header(data, b'BLBO')
            if name in descriptors:
                read_dependency_blob(data)
            elif (name != PRIVATE + 'stock_materials' or len(data) < 70 or
                    struct.unpack_from('<I', data, 64)[0] != len(data) - 68):
                raise ValueError(f'Unsupported native forest material Blob encoding: {name}')
        elif data[4:8] not in (b'MORO', b'LOCO', b'NATO', b'TLDO', b'RRAO', b'XTMO'):
            raise ValueError(f'Unsupported native forest resource encoding: {name}')
    if AUTHORED_MARKER in resources and {name for name, data in resources.items() if data[4:8] == b'XETO'} != AUTHORED_TEXTURES:
        raise ValueError('Native authored texture inventory is incomplete')
    stock_inventory = None
    for mode in SCENES:
        path = dependency_path(mode)
        if path not in resources:
            raise ValueError(f'Missing native dependency Blob: {path}')
        core, stock = read_dependency_blob(resources[path])
        if stock_inventory is None:
            stock_inventory = stock
        expected_core, expected_stock = dependency_inventory(
            resources, receipt if receipt is not None else {'external_resources': stock_inventory}, mode)
        if core != expected_core or stock != expected_stock:
            raise ValueError(f'Incomplete native dependency inventory: {path}')


def write_bundle(resources, report, output):
    validate_resources(resources, report)
    output = Path(output)
    with output.open('xb') as stream, zipfile.ZipFile(stream, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(resources.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.create_system, info.external_attr, info.compress_type = 3, 0o100644 << 16, zipfile.ZIP_DEFLATED
            archive.writestr(info, data, compresslevel=9)
    return {**report, 'bundle': {'name': output.name, 'sha256': file_sha(output), 'bytes': output.stat().st_size},
            'resources': {name: {'bytes': len(data), 'sha256': sha(data)} for name, data in sorted(resources.items())}}


def native_resource_header(resources, receipt):
    descriptors = {dependency_path(mode) for mode in SCENES}
    common = ({name for name in resources if name.startswith(PRIVATE)} - descriptors) | set(receipt['external_resources'])
    result = '#pragma once\n// Generated by tools/black_market/native_forest.py; complete per-host inventory.\n'
    for engine, modes in (('Mm', ('combined', 'mm')), ('Oot', ('oot',))):
        prefixes = tuple(SCENES[mode] for mode in modes)
        paths = common | {name for name in resources if any(name.startswith(prefix) for prefix in prefixes)}
        paths.update(dependency_path(mode) for mode in modes)
        result += f'inline constexpr const char* kNativeForest{engine}Resources[] = {{\n'
        result += ''.join(f'    "{name}",\n' for name in sorted(paths)) + '};\n'
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-mm', type=Path)
    parser.add_argument('--source-edit', type=Path, default=ROOT / 'tools/magic_mirror/scene/LostWoods-MirrorStump.prelude.o2r')
    parser.add_argument('--final', type=Path)
    parser.add_argument('--authored', type=Path, help='Immutable FINAL-avatar solo presentation donor')
    parser.add_argument('--base-manifest', type=Path, help='Verified geometry control or current source bundle to rebake')
    parser.add_argument('--mirror-access', action='store_true', help='Apply the separately reviewed additive fallen-log route')
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--manifest', required=True, type=Path)
    parser.add_argument('--contract', type=Path)
    parser.add_argument('--core-contract', type=Path)
    args = parser.parse_args()
    if args.base_manifest:
        if args.native_mm or args.final or not args.authored:
            parser.error('--base-manifest requires --authored and excludes --native-mm/--final')
        # Use the same manifest/hash/resource checks as the build preparation.
        import sys
        sys.path.insert(0, str(ROOT / 'scripts/assets'))
        from prepare_native_forest import verified_resources
        base = verified_resources(args.base_manifest)
        resources, report = bake_presentation(base, json.loads(args.base_manifest.read_text()), args.authored)
    elif not args.native_mm or not args.final:
        parser.error('Specify --native-mm and --final, or --base-manifest and --authored')
    inputs = {path.resolve() for path in (args.native_mm, args.source_edit, args.final, args.authored, args.base_manifest) if path}
    outputs = [path for path in (args.output, args.manifest, args.contract, args.core_contract) if path]
    if len({path.resolve() for path in outputs}) != len(outputs) or any(path.resolve() in inputs or path.exists() for path in outputs):
        parser.error('Outputs must be separate new paths; immutable donors cannot be overwritten')
    if not args.base_manifest:
        resources, report = convert(args.native_mm, args.source_edit, args.final, args.authored)
    if args.mirror_access:
        resources, report = bake_mirror_access(resources, report)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    manifest = write_bundle(resources, report, args.output)
    for path, data in ((args.manifest, json.dumps(manifest, indent=2, sort_keys=True) + '\n'),
                       (args.contract, native_resource_header(resources, report)),
                       (args.core_contract, native_core_header(resources))):
        if path:
            path.parent.mkdir(parents=True, exist_ok=True)
            with path.open('x') as stream:
                stream.write(data)
    print(json.dumps({'bundle': str(args.output), 'resources': len(resources), 'custom_texture_payloads': report['custom_texture_payloads'],
                      'sha256': manifest['bundle']['sha256']}, indent=2))


if __name__ == '__main__':
    main()
