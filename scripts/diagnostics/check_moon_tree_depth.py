#!/usr/bin/env python3
"""Audit actual Moon-tree triangle depth state and Prelude stored draw bytes."""
import argparse
import json
from pathlib import Path
import struct
import zipfile

PROJECT = 'prelude/project/edits.json'
POOL = 'prelude/project/blobs.bin'
LABELS = frozenset(('sou_wood_01', 'sou_ha_01', 'sou_ha_02', 'sou_ha_03', 'sou_sida_01'))
EXPANDED = frozenset((0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42))


def commands(data):
    if len(data) < 80 or data[4:8] != b'TLDO' or data[64] != 4 or (len(data) - 72) % 8:
        raise ValueError('Expected a little-endian F3DEX2 display list')
    offset = 72
    while offset < len(data):
        w0, w1 = struct.unpack_from('<II', data, offset)
        opcode = w0 >> 24
        width = 16 if opcode in EXPANDED else 8
        if offset + width > len(data):
            raise ValueError('Truncated display-list command')
        yield offset, w0, w1, opcode
        offset += width
        if opcode == 0xDF:
            if offset != len(data):
                raise ValueError('Trailing display-list commands')
            return
    raise ValueError('Missing EndDisplayList')


def tree_shapes(project):
    if project.get('codec') != 2:
        raise ValueError('Expected Prelude codec 2')
    found = []
    for edit in project['edits']['Z2_00KEIKOKU']:
        for shape in edit.get('data', {}).get('shapes', []):
            if shape.get('placed', {}).get('label') not in ('Moon-Tree-RootSeamFix', 'Moon-Tree-Base-Foliage'):
                continue
            labels = {texture['label'] for texture in shape['stored']['textures']}
            if not labels or not labels <= LABELS:
                raise ValueError('Unexpected Moon-tree material inventory')
            found.append(shape)
    if len(found) != 134 or len({shape['newDlPath'] for shape in found}) != 134:
        raise ValueError('Expected the exact 134-draw Moon-tree placement')
    return found


def blob_range(project, reference):
    desc = project['blobs'][reference['$blob']]
    if desc['t'] != 'u8' or min(desc['o'], desc['n']) < 0:
        raise ValueError('Invalid stored display-list blob')
    return desc.get('p', POOL), desc['o'], desc['n']


def triangle_depth(data):
    geometry = low = triangles = 0
    failures = []
    for offset, w0, w1, opcode in commands(data):
        if opcode == 0xD9:
            geometry = (geometry & (w0 & 0xFFFFFF)) | w1
        elif opcode == 0xE2:
            length = (w0 & 255) + 1
            shift = 32 - ((w0 >> 8) & 255) - length
            if not 0 <= shift <= 31:
                raise ValueError('Invalid SetOtherMode shift')
            mask = ((1 << length) - 1) << shift
            low = (low & ~mask) | (w1 & mask)
        elif opcode in (0x05, 0x06):
            count = 1 if opcode == 0x05 else 2
            triangles += count
            # Match Interpreter::DrawTriangle: vertex depth requires G_ZBUFFER;
            # Z_CMP and Z_UPD alone do not enable the renderer's depth test.
            if not (geometry & 1 and low & 0x10 and low & 0x20):
                failures.append({'offset': offset, 'geometry': geometry, 'other_mode_l': low})
    return triangles, failures


def audit(path):
    with zipfile.ZipFile(path) as archive:
        if len(archive.namelist()) != len(set(archive.namelist())):
            raise ValueError('Duplicate resource names')
        project = json.loads(archive.read(PROJECT))
        shapes = tree_shapes(project)
        failures, total = [], 0
        for shape in shapes:
            name = shape['newDlPath']
            data = archive.read(name)
            owner, offset, length = blob_range(project, shape['stored']['dl'])
            if archive.read(owner)[offset:offset + length] != data:
                raise ValueError('Prelude stored/runtime display lists differ: ' + name)
            triangles, bad = triangle_depth(data)
            if not triangles:
                raise ValueError('Tree draw has no triangles: ' + name)
            total += triangles
            if bad:
                failures.append({'path': name, 'bad_triangle_commands': len(bad), 'first': bad[0]})
    return {'tree_draws': len(shapes), 'triangles': total,
            'stored_runtime_parity': 'passed', 'depth_failures': failures}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    args = parser.parse_args()
    result = audit(args.archive)
    bad = result['depth_failures']
    print(json.dumps({**result, 'depth_failures': len(bad), 'first_failure': bad[0] if bad else None}, indent=2))
    raise SystemExit(bool(bad))


if __name__ == '__main__':
    main()
