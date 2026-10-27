#!/usr/bin/env python3
"""Append one physical fallen-log route to the retained native Mirror stump.

The caller supplies the native_forest module to keep serialization and resource
ownership in that generator. This recipe never writes archives or changes room,
scene, actor, camera, waterbox, original polygon, or original vertex records.
"""
import math
import struct
import xml.etree.ElementTree as ET

PREFIX = 'custom/comboship/native_forest/'
BARK = PREFIX + 'authored/textures/portal_bark'
ROOT = PREFIX + 'mirror_access_root'
MATERIAL = PREFIX + 'mirror_access_material'
VERTICES = PREFIX + 'mirror_access_vtx'


def _shape():
    # A broad flat top and a chamfered bark-covered underside make a fallen
    # trunk, rather than an invisible climb flag. The toe is partly buried.
    cross_section = ((-32, 0), (-40, -7), (-40, -21), (-32, -28),
                     (32, -28), (40, -21), (40, -7), (32, 0))
    points = [(x, y + dy, 416 + dz) for x, y in ((-105, 0), (-345, 80))
              for dz, dy in cross_section]
    faces = [[i, (i + 1) % 8, (i + 1) % 8 + 8, i + 8] for i in range(8)]
    faces += [list(reversed(range(8))), list(range(8, 16))]
    return points, faces


def _collision_parts(native, data):
    native.check_header(data, b'LOCO')
    native.playable_collision(data)  # Validate the complete camera/water tail.
    reader = native.Reader(data, 64)
    bounds = reader.take('6h')
    vertices = [reader.take('3h') for _ in range(reader.take('I'))]
    polygons = [reader.take('8h') for _ in range(reader.take('I'))]
    surfaces = [reader.take('2I') for _ in range(reader.take('I'))]
    return bounds, vertices, polygons, surfaces, data[reader.offset:]


def _triangle(points, indices, base, surface):
    a, b, c = (points[i] for i in indices)
    u, v = ([b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)])
    cross = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
    length = math.sqrt(sum(value * value for value in cross))
    if not length:
        raise ValueError('Degenerate Mirror access triangle')
    normal = tuple(value / length for value in cross)
    distance = round(-sum(normal[i] * a[i] for i in range(3)))
    return (surface, *(base + i for i in indices), *(int(value * 32767) for value in normal), distance)


def _append_collision(native, data, points, faces):
    bounds, original, polygons, surfaces, tail = _collision_parts(native, data)
    # This route is authored for the retained native scene. Moving/resizing the
    # stump requires a new route review; never leave a ramp at obsolete coords.
    cap = [poly for poly in polygons if poly[5] >= 30000 and
           all(original[index & 0x1FFF][1] == 80 and
               -432 <= original[index & 0x1FFF][0] <= -322 and
               360 <= original[index & 0x1FFF][2] <= 472 for index in poly[1:4])]
    if len(cap) != 12 or not surfaces:
        raise ValueError('Mirror access requires the retained native stump cap')
    if len(original) + len(points) > 0x2000 or len(polygons) + 28 > 0xFFFF or len(surfaces) >= 0x7FFF:
        raise ValueError('Mirror access exceeds native collision limits')
    if any(not bounds[axis] <= p[axis] <= bounds[axis + 3] for p in points for axis in range(3)):
        raise ValueError('Mirror access would change protected scene collision bounds')
    additions = [_triangle(points, (face[0], face[i], face[i + 1]), len(original), len(surfaces))
                 for face in faces for i in range(1, len(face) - 1)]
    # Surface 0 is the stump's original wood surface. Clone it for the log;
    # changing it in place would alter hundreds of unrelated forest polygons.
    result = data[:76]
    result += struct.pack('<I', len(original) + len(points))
    result += b''.join(struct.pack('<3h', *point) for point in original + points)
    result += struct.pack('<I', len(polygons) + len(additions))
    result += b''.join(struct.pack('<8h', *poly) for poly in polygons + additions)
    result += struct.pack('<I', len(surfaces) + 1)
    result += b''.join(struct.pack('<2I', *surface) for surface in surfaces + [surfaces[0]])
    return result + tail


def _visual(native, points, faces):
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
    ET.SubElement(root, 'CallDisplayList', Path=MATERIAL)
    vertices = []
    for face_index, face in enumerate(faces):
        shade = (195, 155, 105, 80, 105, 155, 195, 235, 200, 200)[face_index]
        offset = len(vertices)
        for index in face:
            x, y, z = points[index]
            vertices.append((x, y, z, 0, (-105 - x) * 16, (z - 384) * 16,
                             shade, shade, shade, 255))
        ET.SubElement(root, 'LoadVertices', Path=VERTICES, VertexBufferIndex='0',
                      VertexOffset=str(offset), Count=str(len(face)))
        for i in range(1, len(face) - 1):
            ET.SubElement(root, 'Triangle1', V00='0', V01=str(i), V02=str(i + 1), Flag0='0')
    ET.SubElement(root, 'PipeSync')
    ET.SubElement(root, 'SetGeometryMode', G_LIGHTING='1', G_CULL_BACK='1')
    ET.SubElement(root, 'EndDisplayList')
    return {ROOT: native._xml(root), MATERIAL: native._xml(material),
            VERTICES: native.header(0x4F415252) + struct.pack('<II', 25, len(vertices)) +
            b''.join(struct.pack('<hhhHhhBBBB', *vertex) for vertex in vertices)}


def augment(native, resources):
    """Mutate a complete native resource dict atomically; return room-mesh inputs.

    The caller adds report['root'] with report['bounds'] to each native room-0
    mesh, and includes the three new resources in its dependency descriptors.
    """
    paths = [prefix + '_collision' for prefix in native.SCENES.values()]
    if any(path not in resources for path in paths):
        raise ValueError('Mirror access requires all three native collision resources')
    points, faces = _shape()
    visual = _visual(native, points, faces)
    already_present = any(path in resources for path in visual)
    if already_present:
        if any(resources.get(path) != data for path, data in visual.items()):
            raise ValueError('Incomplete or different Mirror access mesh already exists')
        for path in paths:
            _, vertices, polygons, surfaces, _ = _collision_parts(native, resources[path])
            expected = [_triangle(points, (face[0], face[i], face[i + 1]), len(vertices) - 16, len(surfaces) - 1)
                        for face in faces for i in range(1, len(face) - 1)]
            if vertices[-16:] != points or polygons[-28:] != expected or surfaces[-1] != surfaces[0]:
                raise ValueError('Existing Mirror access mesh/collision do not match')
    else:
        changes = {path: _append_collision(native, resources[path], points, faces) for path in paths}
        changes.update(visual)
        resources.update(changes)
    return {'root': ROOT, 'material': MATERIAL, 'vertex_resource': VERTICES,
            'bounds': native.bounding_sphere(points), 'collision_paths': paths,
            'start': [-105, 0, 416], 'end': [-345, 80, 416], 'walk_width': 64,
            'run': 240, 'rise': 80, 'slope_degrees': math.degrees(math.atan2(80, 240)),
            'collision_delta': {'vertices': 16, 'polygons': 28, 'surfaces': 1},
            'visual_vertices': 48, 'visual_triangles': 28, 'texture': BARK,
            'preserved': 'all original collision/header/camera/water records; every scene/actor/room resource',
            'already_present': already_present}
