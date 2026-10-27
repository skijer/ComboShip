#!/usr/bin/env python3
"""Bind the supplied POC1 return tunnel to ComboShip without changing scenery.

The active room mesh reference is the engine/archive contract. A tiny wrapper
selects the authored tunnel bounds and suppresses the fallback arch. The light's
RGB becomes deep purple. Geometry, UVs, alpha, textures and progression stay intact.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zipfile

BASE_PACK_SHA256 = '3715273f31025f862567c7f5c771c2e803b6e779e801117cf310cd02bf904317'
ROOM_PREFIX = 'alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00'
OPA = 'custom/cor_mm3d/lost_woods_poc1/room_0/opa'
WRAPPER = OPA + '_combo_return_v1'
GLOW = 'alt/custom/cor_mm3d/lost_woods_poc1/room_0/ReturnTunnelPinkLight_vtx'
PURPLE = (96, 32, 176)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def mesh_command(raw):
    # Serialized SetMesh layout used by the supplied exporter and the native
    # SetMeshFactory. Validate complete path fields before accepting a match.
    matches = []
    for match in re.finditer(b'\x0a\x00\x00\x00', raw[64:]):
        start = 64 + match.start()
        try:
            _, kind, count = struct.unpack_from('<BBB', raw, start + 4)
            if kind != 0 or count != 1 or raw[start + 7] != 0:
                continue
            offset = start + 8
            paths = []
            for _ in range(2):
                length, = struct.unpack_from('<I', raw, offset)
                offset += 4
                if length >= 1024 or offset + length > len(raw):
                    raise ValueError('invalid mesh path length')
                path = raw[offset:offset + length].decode()
                if path and not path.startswith('custom/'):
                    raise ValueError('not the private POC1 mesh')
                paths.append(path)
                offset += length
            if paths[0] == OPA:
                matches.append((start, offset, paths))
        except (ValueError, struct.error, IndexError):
            continue
    if len(matches) != 1:
        raise ValueError('expected exactly one authored POC1 room mesh')
    return matches[0]


def bind(source, target):
    if source.resolve() == target.resolve() or target.exists():
        raise ValueError('choose a new candidate output; the baseline must be preserved')
    if sha(source.read_bytes()) != BASE_PACK_SHA256:
        raise ValueError('input differs from the supplied Lost Woods POC1 baseline')
    changes = {}
    glow_change = None
    with zipfile.ZipFile(source) as before:
        if before.testzip() is not None:
            raise ValueError('baseline ZIP integrity failure')
        names = before.namelist()
        if len(names) != len(set(names)):
            raise ValueError('duplicate baseline resource names')
        with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as after:
            for info in before.infolist():
                data = before.read(info)
                if re.fullmatch(re.escape(ROOM_PREFIX) + r'(?:Set_[0-9A-F]+)?', info.filename):
                    start, end, paths = mesh_command(data)
                    command = struct.pack('<iBBBB', 10, 0, 0, 1, 0)
                    for path in (WRAPPER, paths[1]):
                        encoded = path.encode()
                        command += struct.pack('<I', len(encoded)) + encoded
                    candidate = data[:start] + command + data[end:]
                    assert data[:start] == candidate[:start]
                    assert data[end:] == candidate[start + len(command):]
                    changes[info.filename] = {'baseline_sha256': sha(data), 'candidate_sha256': sha(candidate),
                                              'prefix_sha256': sha(data[:start]), 'suffix_sha256': sha(data[end:])}
                    data = candidate
                elif info.filename == GLOW:
                    vertices = ET.fromstring(data)
                    assert vertices.tag == 'Vertex' and len(vertices) == 60
                    original_attributes = [dict(v.attrib) for v in vertices]
                    for vertex in vertices:
                        assert tuple(vertex.get(c) for c in ('R', 'G', 'B')) == ('255', '107', '184')
                        for channel, value in zip(('R', 'G', 'B'), PURPLE):
                            vertex.set(channel, str(value))
                    candidate = ET.tostring(vertices, encoding='utf-8') + b'\n'
                    for old, vertex in zip(original_attributes, vertices):
                        assert {k: v for k, v in old.items() if k not in ('R', 'G', 'B')} == {
                            k: v for k, v in vertex.attrib.items() if k not in ('R', 'G', 'B')}
                    glow_change = {'resource': GLOW, 'baseline_sha256': sha(data), 'candidate_sha256': sha(candidate),
                                   'rgb': PURPLE, 'vertices': len(vertices), 'alpha_geometry_uvs': 'unchanged'}
                    data = candidate
                after.writestr(info, data)
            wrapper = f'<DisplayList Version="0">\n  <CallDisplayList Path="{OPA}" />\n  <EndDisplayList />\n</DisplayList>\n'
            after.writestr('alt/' + WRAPPER, wrapper)
    if len(changes) != 4 or glow_change is None:
        target.unlink()
        raise ValueError('expected all four room-zero/setup headers')
    with zipfile.ZipFile(source) as before, zipfile.ZipFile(target) as after:
        assert after.testzip() is None
        assert set(after.namelist()) == set(before.namelist()) | {'alt/' + WRAPPER}
        for name in before.namelist():
            if name not in changes and name != GLOW:
                assert before.read(name) == after.read(name), name
    return {'baseline_sha256': BASE_PACK_SHA256, 'candidate_sha256': sha(target.read_bytes()),
            'source_resource_count': len(names), 'candidate_resource_count': len(names) + 1,
            'changed_room_headers': changes, 'glow_change': glow_change, 'added_mesh_wrapper': 'alt/' + WRAPPER,
            'anchor': {'scene': 'SCENE_LOST_WOODS', 'room': 0, 'position': [-417, 0, -122], 'yaw': 11520},
            'bounds_local': {'x': [-58, 58], 'y': [-40, 130], 'z': [-62, 12], 'arm_distance': 120},
            'preservation': 'Every existing resource except four opaque mesh references and glow RGB is byte-identical.',
            'runtime': 'Pending; requires the matching ComboShip source and alternate assets.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('target', type=Path)
    parser.add_argument('--receipt', type=Path, required=True)
    args = parser.parse_args()
    receipt = bind(args.source, args.target)
    args.receipt.write_text(json.dumps(receipt, indent=2) + '\n')
    print('PASS archive binding: four mesh references, one wrapper, purple glow RGB; geometry/textures/alpha unchanged')


if __name__ == '__main__':
    main()
