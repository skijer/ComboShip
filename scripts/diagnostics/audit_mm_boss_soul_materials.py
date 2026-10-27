#!/usr/bin/env python3
"""Read native MM boss rig/material dependencies without editing any assets.

Accepts the user's native MM o2r explicitly. This establishes archive/static
material facts; it does not validate installed Alt packs or runtime appearance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/mods'))
from build_mm_tp_dungeon_bottles import commands, crc64

RIGS = {
    'Goht': 'objects/object_boss_hakugin/gGohtSkel',
    'Odolwa': 'objects/object_boss01/gOdolwaSkel',
    'Gyorg': 'objects/object_boss03/gGyorgSkel',
    'Twinmold': 'objects/object_boss02/gTwinmoldHeadSkel',
}


def string(blob, offset):
    size = struct.unpack_from('<I', blob, offset)[0]
    start = offset + 4
    assert start + size <= len(blob)
    return blob[start:start + size].decode(), start + size


def limb_mesh(blob):
    assert blob[4:8] == b'BLSO' and blob[64] == 1  # Standard limb
    _, offset = string(blob, 66)  # skin display list
    modifications = struct.unpack_from('<I', blob, offset + 2)[0]
    assert modifications == 0
    _, offset = string(blob, offset + 6)  # second skin display list
    offset += 18  # skin float positions and u16 rotations
    _, offset = string(blob, offset)  # child resource
    _, offset = string(blob, offset)  # sibling resource
    mesh, _ = string(blob, offset)
    return mesh


def audit(archive, name, path, hashes):
    blob = archive.read(path)
    assert blob[4:8] == b'LKSO'
    kind, limb_type = blob[64:66]
    limb_count, flex_count = struct.unpack_from('<II', blob, 66)
    assert kind in (0, 1) and limb_type == 1 and blob[74] == 1
    table_count = struct.unpack_from('<I', blob, 75)[0]
    assert table_count == limb_count
    offset, meshes = 79, []
    for _ in range(table_count):
        limb, offset = string(blob, offset)
        mesh = limb_mesh(archive.read(limb))
        if mesh:
            meshes.append(mesh)
    textures, dynamic, palettes, combine, lut, prim, matrix_segments = set(), set(), set(), set(), set(), set(), set()
    for mesh in meshes:
        for opcode, w0, w1, resource_hash, _ in commands(archive.read(mesh)):
            if resource_hash is not None:
                assert resource_hash in hashes, f'{name}: unresolved resource {resource_hash:x}'
                if opcode == 0x20:
                    texture = hashes[resource_hash]
                    assert archive.read(texture)[4:8] == b'XETO'
                    textures.add(texture)
                    if texture.endswith('TLUT'):
                        palettes.add(texture)
            if opcode == 0xFD:
                dynamic.add(w1 >> 24)
            if opcode == 0xDA:
                matrix_segments.add(w1 >> 24)
            if opcode == 0xFC:
                combine.add(f'{w0:08x} {w1:08x}')
            if opcode == 0xE3 and (w0 & 0xFFFFFF) == 0x1001:
                lut.add(w1)
            if opcode == 0xFA:
                prim.add(f'{w1:08x}')
    assert dynamic == ({8} if name in ('Goht', 'Twinmold') else set())
    assert kind == 0 or flex_count == len(meshes)
    return {
        'skeleton': path, 'skeleton_type': 'normal' if kind == 0 else 'flex',
        'limbs': limb_count, 'flex_matrices': flex_count, 'limb_meshes': len(meshes),
        'dynamic_texture_segments': sorted(dynamic), 'mesh_matrix_segments': sorted(matrix_segments),
        'texture_resources': len(textures), 'tlut_resources': len(palettes),
        'combine_commands': sorted(combine), 'lut_modes': sorted(lut),
        'mesh_primitive_colors': sorted(prim),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-mm', type=Path, required=True)
    args = parser.parse_args()
    with args.native_mm.open('rb') as handle:
        checksum = hashlib.file_digest(handle, 'sha256').hexdigest()
    with zipfile.ZipFile(args.native_mm) as archive:
        hashes = {crc64(path): path for path in archive.namelist()}
        report = {name: audit(archive, name, path, hashes) for name, path in RIGS.items()}
    print(json.dumps({'native_mm_sha256': checksum, 'bosses': report}, indent=2))


if __name__ == '__main__':
    main()
