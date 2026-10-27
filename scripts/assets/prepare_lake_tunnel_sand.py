#!/usr/bin/env python3
"""Prepare the dedicated Lake tunnel sand without replacing scene atlases."""
import argparse
import json
from pathlib import Path
import struct
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/cumulative_assets'))
from cumulative_source_pool import decode, pack, sha, write_files

DEFAULT_MANIFEST = ROOT / 'tools/lake_tunnel_sand/manifest.json'
SAND = 'textures/comboship/regional/lake_hylia/sand'


def verified_resources(manifest_path=DEFAULT_MANIFEST, source_directory=None):
    manifest_path = Path(manifest_path)
    metadata = json.loads(manifest_path.read_text())
    if metadata.get('schema') != 1 or set(metadata.get('resources', {})) != {SAND, 'alt/' + SAND}:
        raise ValueError('Lake sand inventory must contain only the dedicated native/Alt material')
    expected = metadata['resources']
    storage = metadata.get('bundle', {})
    if storage.get('encoding') != 'zlib-base64-pool' or storage.get('directory') != 'bundled':
        raise ValueError('Lake sand source must use bounded compressed text parts')
    bundle = Path(source_directory or manifest_path.parent) / 'bundled'
    actual = json.loads((bundle / 'manifest.json').read_text())
    summary = {key: value for key, value in actual.items() if key != 'files'}
    if {key: value for key, value in storage.items() if key != 'directory'} != summary:
        raise ValueError('Lake sand source pool manifest hash/size identity mismatch')
    resources = decode(bundle, expected)
    native = resources[SAND]
    if resources['alt/' + SAND] != native or native[4:8] != b'XETO' or len(native) < 92:
        raise ValueError('Lake sand native/Alt resources must be identical textures')
    typ, width, height, flags, horizontal, vertical, size = struct.unpack_from('<IIIIffI', native, 64)
    if ((typ, flags) != (2, 1) or horizontal <= 0 or vertical <= 0 or
            width * 4 != 32 * 2 * horizontal or height != 32 * vertical or
            size != width * height * 4 or len(native) != 92 + size):
        raise ValueError('Lake sand must retain the renderer 32x32 RGBA16 logical tile contract')
    return resources


def prepare(output_root, manifest_path=DEFAULT_MANIFEST, source_directory=None):
    # Complete hash/membership/format verification before touching either path.
    resources = verified_resources(manifest_path, source_directory)
    restored = write_files(output_root, {'soh/assets/custom/' + name: data for name, data in resources.items()})
    print(f'PASS Lake tunnel sand: 2 dedicated native/Alt resources ({restored} restored); scene atlases untouched')


def pack_source(archive_path, manifest_path=DEFAULT_MANIFEST):
    manifest_path = Path(manifest_path)
    metadata = json.loads(manifest_path.read_text())
    archive_path = Path(archive_path)
    source = metadata['source']
    payload = archive_path.read_bytes()
    if len(payload) != source['bytes'] or sha(payload) != source['sha256']:
        raise ValueError('Refusing changed Lake sand authoring archive')
    with zipfile.ZipFile(archive_path) as archive:
        data = archive.read(SAND)
    resources = {SAND: data, 'alt/' + SAND: data}
    expected = metadata['resources']
    for name, resource in resources.items():
        if len(resource) != expected[name]['bytes'] or sha(resource) != expected[name]['sha256']:
            raise ValueError('Refusing changed Lake sand source pixels/metadata')
    manifest = pack(resources, manifest_path.parent / 'bundled')
    metadata['bundle'] = {'directory': 'bundled', **{key: value for key, value in manifest.items() if key != 'files'}}
    manifest_path.write_text(json.dumps(metadata, indent=2, sort_keys=True) + '\n')
    print(f'Packed exact Lake sand in {len(manifest["parts"])} bounded text parts')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-root', type=Path, default=ROOT)
    parser.add_argument('--manifest', type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument('--pack-from-archive', type=Path, help='author text parts from the pinned source archive')
    args = parser.parse_args()
    try:
        if args.pack_from_archive:
            pack_source(args.pack_from_archive, args.manifest)
        else:
            prepare(args.output_root, args.manifest)
    except (ValueError, OSError, zipfile.BadZipFile) as error:
        parser.exit(1, str(error) + '\n')


if __name__ == '__main__':
    main()
