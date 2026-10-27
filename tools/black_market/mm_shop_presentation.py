#!/usr/bin/env python3
"""Private MM presentation variant; shared retained geometry stays byte-exact."""
import xml.etree.ElementTree as ET

import forest
import market_interior
import mm_native_shop

PREFIX = 'custom/skiejer_market/mm_shop_presentation_v1/'
ROOM_ROOT = 'custom/skiejer_market/room'
GEOMETRY = market_interior.GEOMETRY
OWNERS = [ROOM_ROOT, GEOMETRY, *(f'{GEOMETRY}_{kind}_{suffix}'
          for suffix in (0, 2, 6, 15, 17, 18) for kind in ('tri', 'vtx'))]
CLONES = {name: PREFIX + name.removeprefix('custom/skiejer_market/') for name in OWNERS}
# Retain the proven whole-basket and hanging-foliage clearance moves.
# Rear cauldrons are removed separately, with exact connected-piece checks.
# Shared/OoT source resources remain unchanged.
MOVES = {
    17: [([[-85, 107, 156], [-31, 175, 210]], (-50, 0, 0), 302, 240),
         ([[34, 129, 144], [77, 181, 186]], (60, 0, 0), 226, 176)],
    15: [([[-58, 166, 167], [-58, 191, 186]], (-50, 0, 0), 6, 2),
         ([[-65, 165, 183], [-55, 178, 183]], (-50, 0, 0), 6, 2),
         ([[-61, 188, 183], [-40, 212, 183]], (-50, 0, 0), 6, 2),
         ([[53, 174, 163], [70, 194, 165]], (60, 0, 0), 6, 2),
         ([[55, 175, 160], [56, 184, 168]], (60, 0, 0), 6, 2)],
}

# Complete disconnected rear-pot pieces, audited before any vertex moves.
# Remove their draws, including contents/attachments; retain all other faces.
REMOVALS = {
    0: [([[49, 37, 24], [97, 37, 72]], 25, 36)],
    2: [([[-120, 0, 0], [-62, 54, 54]], 116, 106),
        ([[-103, 60, 42], [-92, 62, 53]], 20, 16),
        ([[42, 0, 20], [105, 39, 79]], 94, 88)],
    6: [([[-104, 51, 27], [-53, 62, 53]], 80, 60)],
    18: [([[-114, 51, 2], [-69, 51, 47]], 30, 36)],
}


def encode(root):
    ET.indent(root, space='  ')
    return ET.tostring(root, encoding='utf-8') + b'\n'


def remove_components(resources, original, data, pieces):
    parts = market_interior.vertex_components(resources, original)
    removed = set()
    expected = 0
    for bounds, count, triangles in pieces:
        matches = [part for part in parts if part['bounds'] == bounds]
        if len(matches) != 1 or len(matches[0]['indices']) != count or matches[0]['triangles'] != triangles:
            raise ValueError('private MM rear-pot component identity changed')
        removed.update(matches[0]['indices'])
        expected += triangles
    root = ET.fromstring(data)
    cache, deleted = {}, 0
    for node in list(root):
        if node.tag == 'LoadVertices':
            start, offset, count = (int(node.get(key)) for key in ('VertexBufferIndex', 'VertexOffset', 'Count'))
            cache.update({start+i: offset+i for i in range(count)})
        elif node.tag in ('Triangle1', 'Triangles2'):
            keep = []
            for face in range(2 if node.tag == 'Triangles2' else 1):
                vertices = [int(node.get(f'V{face}{i}')) for i in range(3)]
                selected = [cache[index] in removed for index in vertices]
                if any(selected):
                    if not all(selected):
                        raise ValueError('private MM rear-pot triangle crosses an unaudited component')
                    deleted += 1
                else:
                    keep.append((vertices, node.get(f'Flag{face}', '0')))
            if not keep:
                root.remove(node)
            elif len(keep) == 1 and node.tag == 'Triangles2':
                vertices, flag = keep[0]
                node.tag = 'Triangle1'
                node.attrib.clear()
                node.attrib.update({f'V0{i}': str(index) for i, index in enumerate(vertices)})
                node.set('Flag0', flag)
    if deleted != expected:
        raise ValueError('private MM rear-pot triangle count changed')
    return encode(root)


def apply(resources):
    result = dict(resources)
    for original, clone in CLONES.items():
        root = ET.fromstring(resources[original])
        for node in root.iter():
            if node.get('Path') in CLONES:
                node.set('Path', CLONES[node.get('Path')])
        result[clone] = encode(root)
    for suffix, moves in MOVES.items():
        parts = market_interior.vertex_components(resources, f'{GEOMETRY}_tri_{suffix}')
        root = ET.fromstring(result[CLONES[f'{GEOMETRY}_vtx_{suffix}']])
        vertices = list(root)
        for bounds, delta, count, triangles in moves:
            matched = [part for part in parts if part['bounds'] == bounds]
            if len(matched) != 1 or len(matched[0]['indices']) != count or matched[0]['triangles'] != triangles:
                raise ValueError('private MM presentation component identity changed')
            for index in matched[0]['indices']:
                node = vertices[index]
                for axis, offset in zip('XYZ', delta):
                    node.set(axis, str(int(node.get(axis)) + offset))
        result[CLONES[f'{GEOMETRY}_vtx_{suffix}']] = encode(root)
    for suffix, pieces in REMOVALS.items():
        name = f'{GEOMETRY}_tri_{suffix}'
        result[CLONES[name]] = remove_components(resources, name, result[CLONES[name]], pieces)
    path = mm_native_shop.ROOM
    commands = mm_native_shop.native_commands(resources[path])
    old, new = forest.string(ROOM_ROOT), forest.string(CLONES[ROOM_ROOT])
    if commands[10].count(old) != 1:
        raise ValueError('private MM room mesh owner changed')
    commands[10] = commands[10].replace(old, new)
    result[path] = mm_native_shop._scene(resources[path], commands)
    return result
