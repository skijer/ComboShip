"""Static native-art material bindings for the built-in chamber geometry.

The optional visual archive replaces these same identities. Geometry and UV
resources are never changed here. A two-column native medallion atlas is the
only adaptation: it rearranges native texels without painting or filtering.
"""
import hashlib
import math
import struct
import xml.etree.ElementTree as ET

TEMPLE = 'scenes/shared/tokinoma_scene/'
SAGES = 'scenes/shared/kenjyanoma_scene/'
PRIVATE = 'custom/temple_sages/'

# These bindings follow the actual native images: masonry, columns, lattice,
# checker floor, pale pavers and red carpet. They are not filename heuristics.
HOST_BINDINGS = {
    'mjin_flash_mode_0': 'objects/object_mjin_flash/gLightMedallionPlatformTex',
    'tex_0035C0_ia8_png': TEMPLE + 'tokinoma_room_1Tex_0068C8',
    'toki_dai_01_5400': TEMPLE + 'tokinoma_sceneTex_00FFA0',
    'toki_dai_02_7E00': TEMPLE + 'tokinoma_sceneTex_00FFA0',
    'toki_dai_03_D200': TEMPLE + 'tokinoma_sceneTex_011FA0',
    'toki_glass_01bx_12600': TEMPLE + 'tokinoma_room_0Tex_0151D8',
    'toki_glass_01x_18A00': TEMPLE + 'tokinoma_room_0Tex_0151D8',
    'toki_glass_01xx_1B200': TEMPLE + 'tokinoma_room_0Tex_0151D8',
    'toki_kabe_01_22B00': TEMPLE + 'tokinoma_sceneTex_00DFA0',
    'toki_kabe_02_25500': TEMPLE + 'tokinoma_sceneTex_00EFA0',
    'toki_kabe_03_27F00': TEMPLE + 'tokinoma_sceneTex_00EFA0',
    'toki_kabe_04_25500': TEMPLE + 'tokinoma_sceneTex_0107A0',
    'toki_kabe_05x_35100': TEMPLE + 'tokinoma_sceneTex_00DFA0',
    'toki_kabe_06_32700': TEMPLE + 'tokinoma_sceneTex_011FA0',
    'toki_poal_01_39000': TEMPLE + 'tokinoma_sceneTex_011FA0',
    'toki_saku_01_41880': TEMPLE + 'tokinoma_room_0Tex_0151D8',
    'toki_step_01_42280': TEMPLE + 'tokinoma_sceneTex_00DFA0',
    'toki_ten_01x_44C80': TEMPLE + 'tokinoma_sceneTex_00DFA0',
    'toki_ten_02_40880': TEMPLE + 'tokinoma_sceneTex_00EFA0',
    'toki_ten_04_46480': TEMPLE + 'tokinoma_sceneTex_011FA0',
    'toki_try_01_47480': TEMPLE + 'tokinoma_room_1Tex_005458',
    'toki_try_02_47F00': TEMPLE + 'tokinoma_room_1Tex_005458',
    'toki_try_03_48980': TEMPLE + 'tokinoma_room_1Tex_005458',
    'toki_yuka_01_44A80': TEMPLE + 'tokinoma_sceneTex_00FFA0',
    'toki_yuka_02x_47480': TEMPLE + 'tokinoma_sceneTex_00D7A0',
    'toki_yuka_03_50780': TEMPLE + 'tokinoma_sceneTex_00FFA0',
    'toki_yuka_04_51201': TEMPLE + 'tokinoma_sceneTex_00CFA0',
}
CHAMBER_BINDINGS = {
    'rm_kja_water_02_13C00': SAGES + 'kenjyanoma_room_0Tex_00D618',
    'rm_kja_water_02_13C00___Copie': SAGES + 'kenjyanoma_room_0Tex_00D618',
    'rm_kja_water_04_16400': SAGES + 'kenjyanoma_room_0Tex_012508',
    'rm_kja_kabe_01__UDIM_': SAGES + 'kenjyanoma_room_0Tex_011D08',
    'rm_kja_kabe_02_4000': SAGES + 'kenjyanoma_room_0Tex_011D08',
    'rm_kja_kabe_03_8000': TEMPLE + 'tokinoma_sceneTex_00EFA0',
    'rm_kja_yuka_01_16C00': SAGES + 'kenjyanoma_room_0Tex_011D08',
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def logical_size(data):
    _, width, height = struct.unpack_from('<III', data, 64)
    if struct.unpack_from('<I', data, 8)[0]:
        flags, horizontal, vertical = struct.unpack_from('<Iff', data, 76)
        return round(width * (2 if flags & 1 else 1) / horizontal), round(height / vertical)
    return width, height


def native_rgba(data):
    kind, width, height = struct.unpack_from('<III', data, 64)
    if kind != 2:
        raise ValueError('Native medallion source must be RGBA16')
    offset = 92 if struct.unpack_from('<I', data, 8)[0] else 80
    values = struct.unpack('>' + str(width * height) + 'H', data[offset:])
    pixels = bytearray()
    for value in values:
        channels = ((value >> 11) & 31, (value >> 6) & 31, (value >> 1) & 31)
        pixels.extend(((channel << 3) | (channel >> 2)) for channel in channels)
        pixels.append(255 if value & 1 else 0)
    return width, height, bytes(pixels)


def medallion_atlas(native, gray=False):
    # Native floor half-discs use different orders. Spirit is vertical in the
    # colored source, and horizontal in the gray Temple source.
    if gray:
        region_specs = [('0091D8', 0, 0), ('0081D8', 0, 32),
                        ('00E1D8', 64, 0), ('00F1D8', 64, 32),
                        ('0131D8', 0, 64), ('0121D8', 0, 96),
                        ('00D1D8', 64, 64), ('00C1D8', 64, 96),
                        ('0111D8', 0, 128), ('0101D8', 0, 160),
                        ('00A1D8', 64, 128), ('00B1D8', 64, 160)]
        prefix = TEMPLE + 'tokinoma_room_0Tex_'
    else:
        region_specs = [('002618', 0, 0), ('001618', 0, 32),
                        ('007618', 64, 0), ('008618', 64, 32),
                        ('00C618', 0, 64), ('00B618', 0, 96),
                        ('006618', 64, 64), ('005618', 64, 96),
                        ('00A618', 0, 128), ('009618', 32, 128),
                        ('003618', 64, 128), ('004618', 64, 160)]
        prefix = SAGES + 'kenjyanoma_room_0Tex_'
    pixels = bytearray(128 * 256 * 4)
    regions = []
    for suffix, x, y in region_specs:
        path = prefix + suffix
        width, height, source = native_rgba(native[path])
        for row in range(height):
            start = ((y + row) * 128 + x) * 4
            pixels[start:start + width * 4] = source[row * width * 4:(row + 1) * width * 4]
        regions.append({'native_path': path, 'source': [0, 0, width, height],
                        'destination': [x, y, width, height], 'source_pixels_sha256': sha(source)})
    header = struct.pack('<B3xIIQ', 0, 0x4F544558, 1, 0xDEADBEEFDEADBEEF).ljust(64, b'\0')
    data = header + struct.pack('<IIIIffI', 2, 128, 256, 1, 8., 4., len(pixels)) + pixels
    return data, regions, sha(pixels)


def source_for(name):
    suffix = name.rsplit('/', 1)[-1]
    if name.startswith(TEMPLE) and suffix in HOST_BINDINGS:
        return HOST_BINDINGS[suffix]
    if name.startswith(PRIVATE + 'chamber/') and suffix in CHAMBER_BINDINGS:
        return CHAMBER_BINDINGS[suffix]
    if name == PRIVATE + 'texture/arcane_stone':
        return TEMPLE + 'tokinoma_sceneTex_00FFA0'
    if name == PRIVATE + 'texture/tunnel_stone':
        return TEMPLE + 'tokinoma_sceneTex_00EFA0'
    if name == PRIVATE + 'texture/veil':
        return SAGES + 'kenjyanoma_room_0Tex_010CE8'
    if name.startswith(PRIVATE + 'portals/') and suffix in ('texture_flow', 'texture_detail'):
        return SAGES + 'kenjyanoma_room_0Tex_012D08'
    if suffix.startswith('pillar_texture_'):
        index = int(suffix.rsplit('_', 1)[-1])
        return [SAGES + 'kenjyanoma_room_0Tex_00D618', SAGES + 'kenjyanoma_room_0Tex_011D08',
                SAGES + 'kenjyanoma_room_0Tex_011D08', TEMPLE + 'tokinoma_sceneTex_00EFA0'][index]
    return None


def shift(old, old_size, new_size):
    exponent = -old if old <= 10 else 16 - old
    exponent += round(math.log2(new_size / old_size))
    if not -10 <= exponent <= 5:
        raise ValueError('Native tile scale exceeds an N64 shift')
    return 16 - exponent if exponent > 0 else -exponent


def rematerial(data, old_resources, new_resources):
    tree = ET.fromstring(data)
    current = None
    for element in list(tree):
        if element.tag == 'ScrollTexture':
            tree.remove(element)
            continue
        if element.tag not in ('LoadTextureBlock', 'SetTextureImage', 'SetTile', 'SetTileSize', 'LoadBlock'):
            continue
        if element.tag in ('SetTextureImage', 'LoadTextureBlock'):
            path = element.get('Path')
            old = old_resources[path]
            new = new_resources[path]
            old_w, old_h = logical_size(old)
            width, height = logical_size(new)
            kind = struct.unpack_from('<I', new, 64)[0]
            formats = {2: ('G_IM_FMT_RGBA', 'G_IM_SIZ_16b', 0, 2, 16),
                       6: ('G_IM_FMT_I', 'G_IM_SIZ_8b', 4, 1, 8),
                       8: ('G_IM_FMT_IA', 'G_IM_SIZ_8b', 3, 1, 8),
                       9: ('G_IM_FMT_IA', 'G_IM_SIZ_16b', 3, 2, 16)}
            if kind not in formats:
                raise ValueError('Unsupported native material format ' + str(kind))
            fmt, siz, fmt_id, siz_id, bits = formats[kind]
            current = old_w, old_h, width, height, fmt, siz, bits
            if element.tag == 'LoadTextureBlock':
                element.set('Format', str(fmt_id)); element.set('Size', str(siz_id))
                element.set('Width', str(width)); element.set('Height', str(height))
                for axis, old_size, new_size in (('S', old_w, width), ('T', old_h, height)):
                    element.set('Mask' + axis, str(round(math.log2(new_size))))
                    element.set('Shift' + axis, str(shift(int(element.get('Shift' + axis, '0')), old_size, new_size)))
            else:
                element.set('Format', fmt)
                element.set('Size', 'G_IM_SIZ_16b_LOAD_BLOCK')
        elif current is not None:
            old_w, old_h, width, height, fmt, siz, bits = current
            line = max(1, width * bits // 64)
            if element.tag == 'SetTile':
                element.set('Format', fmt)
                render = element.get('Tile') != '7'
                element.set('Size', siz if render else 'G_IM_SIZ_16b_LOAD_BLOCK')
                element.set('Line', str(line if render else 0))
                for axis, old_size, new_size in (('S', old_w, width), ('T', old_h, height)):
                    element.set('Mask' + axis, str(round(math.log2(new_size))))
                    element.set('Shift' + axis, str(shift(int(element.get('Shift' + axis, '0')), old_size, new_size)))
            elif element.tag == 'SetTileSize':
                element.set('Lrs', str((width - 1) * 4 + int(element.get('Uls', '0'))))
                element.set('Lrt', str((height - 1) * 4 + int(element.get('Ult', '0'))))
            elif element.tag == 'LoadBlock':
                element.set('Lrs', str(width * height * bits // 16 - 1))
                element.set('Dxt', str(math.ceil(2048 / line)))
    ET.indent(tree, space='  ')
    return ET.tostring(tree, encoding='utf-8')


def apply(resources, native):
    # Geometry is not an Alt toggle. The optional archive still overrides the
    # existing selected names normally when a user enables its visual layer.
    base = {name.removeprefix('alt/'): data for name, data in resources.items() if not name.startswith('alt/')}
    base.update({name[4:]: data for name, data in resources.items() if name.startswith('alt/')})
    # This is authoring bookkeeping for the optional visual archive, not a
    # runtime material. Its old hashes do not describe this native-art base.
    base.pop(PRIVATE + 'manifest.json', None)
    old = dict(base)
    native_by_sha = {sha(data): path for path, data in native.items() if data[4:8] == b'XETO'}
    bindings = {}
    for name, data in list(base.items()):
        if data[4:8] != b'XETO':
            continue
        if name.endswith('/rm_kja_mark_01_C000') or name.endswith('/toki_mark_01_39000'):
            gray = name.endswith('/toki_mark_01_39000')
            data, regions, pixel_hash = medallion_atlas(native, gray)
            source = {'native_paths': [entry['native_path'] for entry in regions],
                      'atlas_regions': regions, 'pixels_sha256': pixel_hash}
        else:
            path = source_for(name) or native_by_sha.get(sha(data))
            if path is None:
                raise ValueError('No explicit native texture binding for ' + name)
            data = native[path]
            source = {'native_paths': [path]}
        base[name] = data
        bindings[name] = {**source, 'resource_sha256': sha(data), 'logical_size': list(logical_size(data))}
    for name, data in list(base.items()):
        if data.lstrip().startswith(b'<') and ET.fromstring(data).tag == 'DisplayList':
            base[name] = rematerial(data, old, base)
    # The optional archive supplies all authored/scrolling material data. Both
    # builtin resolution modes start from the same native-art graph.
    result = dict(base)
    result.update({'alt/' + name: data for name, data in base.items()})
    bindings.update({'alt/' + name: metadata for name, metadata in list(bindings.items())})
    return result, bindings
