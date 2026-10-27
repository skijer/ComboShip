"""Set stone footsteps and strong native echo on walkable chamber collision.

The native collision reader stores surface data[1] before data[0]. Sound is
data[1] bits0..3, and echo is bits11..16. Native floor classification is a
normal Y strictly greater than COLPOLY_SNORMAL(0.5f), or16383. This changes
only those audio bits and floor polygons' type references. Shared wall and
ceiling types, camera/light/conveyor/hookshot flags, geometry and tail data
remain untouched.
"""
import struct

STONE = 2  # D_80119E10[2] == NA_SE_PL_WALK_CONCRETE - SFX_FLAG.
ECHO = 63  # Maximum of the native six-bit collision echo field.
FLOOR_NORMAL_MIN = 16383
AUDIO_MASK = 0xF | (0x3F << 11)
COLLISION = 'scenes/shared/tokinoma_scene/tokinoma_sceneCollisionHeader_0032F8'


def _parts(data):
    if not isinstance(data, bytes) or len(data) < 84 or data[4:8] != b'LOCO':
        raise ValueError('Expected a complete native binary collision resource')
    vertex_count = struct.unpack_from('<I', data, 76)[0]
    if not 0 < vertex_count <= 8192:
        raise ValueError('Collision vertex count is outside the native index range')
    polygon_count_offset = 80 + vertex_count * 6
    if polygon_count_offset + 4 > len(data):
        raise ValueError('Truncated collision vertices')
    polygon_count = struct.unpack_from('<I', data, polygon_count_offset)[0]
    if not 0 < polygon_count <= 65535:
        raise ValueError('Collision polygon count is outside the native range')
    polygon_offset = polygon_count_offset + 4
    surface_count_offset = polygon_offset + polygon_count * 16
    if surface_count_offset + 4 > len(data):
        raise ValueError('Truncated collision polygons')
    surface_count = struct.unpack_from('<I', data, surface_count_offset)[0]
    surface_offset = surface_count_offset + 4
    surface_end = surface_offset + surface_count * 8
    if not 0 < surface_count <= 65536 or surface_end > len(data):
        raise ValueError('Truncated or oversized collision surface array')
    polygons = [struct.unpack_from('<4H4h', data, polygon_offset + i * 16)
                for i in range(polygon_count)]
    if any(poly[0] >= surface_count for poly in polygons):
        raise ValueError('Collision polygon references an absent surface type')
    return polygon_offset, surface_count_offset, surface_offset, surface_end, polygons


def apply(data):
    """Return an idempotent native-audio correction without moving collision."""
    polygon_offset, count_offset, surface_offset, surface_end, polygons = _parts(data)
    original_count = (surface_end - surface_offset) // 8
    surfaces = bytearray(data[surface_offset:surface_end])
    result = bytearray(data[:count_offset])
    remapped = {}
    for index, poly in enumerate(polygons):
        if poly[5] <= FLOOR_NORMAL_MIN:
            continue
        old_type = poly[0]
        data1, data0 = struct.unpack_from('<II', surfaces, old_type * 8)
        corrected = (data1 & ~AUDIO_MASK) | STONE | (ECHO << 11)
        if data1 == corrected:
            continue
        if old_type not in remapped:
            new_type = len(surfaces) // 8
            if new_type >= 65536:
                raise ValueError('Collision surface type capacity exceeded')
            remapped[old_type] = new_type
            surfaces.extend(struct.pack('<II', corrected, data0))
        struct.pack_into('<H', result, polygon_offset + index * 16, remapped[old_type])
    if len(surfaces) // 8 == original_count:
        return data
    return bytes(result) + struct.pack('<I', len(surfaces) // 8) + bytes(surfaces) + data[surface_end:]


def inspect(data):
    """Describe native-reader-visible floor fields for provenance and QA."""
    _, _, offset, _, polygons = _parts(data)
    floor_types = sorted({poly[0] for poly in polygons if poly[5] > FLOOR_NORMAL_MIN})
    fields = []
    for index in floor_types:
        data1, _ = struct.unpack_from('<II', data, offset + index * 8)
        fields.append({'type': index, 'sound': data1 & 0xF, 'echo': (data1 >> 11) & 0x3F})
    return {'walkable_polygons': sum(poly[5] > FLOOR_NORMAL_MIN for poly in polygons),
            'floor_types': fields, 'stone_sound_index': STONE, 'collision_echo': ECHO,
            'floor_normal_y_strict_minimum': FLOOR_NORMAL_MIN}
