#!/usr/bin/env python3
"""Encode the chosen wanted-poster PNG into its existing native HD texture slot."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
POSTER_PNG = ROOT / 'tools/black_market/poster/skijer-wanted-poster.png'
POSTER_SHA = 'e3a3592412bb585fc8d68f87d3bfd6e9540366e96b739cabef1799253228d993'
RESOURCE = 'custom/prelude/Z2_LOST_WOODS/black_market_skeijer_poster'
SOURCE_SHA = '4bef56b0b4e3fbe89308c85e29eefbfce19b4f91d21a7ed79fc5c3c519c4a7ef'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def texture(original):
    if original[4:8] != b'XETO' or struct.unpack_from('<I', original, 8)[0] != 1:
        raise ValueError('wanted poster must use native texture v1')
    kind, old_w, old_h, flags, hscale, vscale, size = struct.unpack_from('<IIIIffI', original, 64)
    if kind != 1 or len(original) != 92 + size or size != old_w * old_h * 4:
        raise ValueError('wanted poster must be a complete RGBA8 HD texture')
    if digest(POSTER_PNG.read_bytes()) != POSTER_SHA:
        raise ValueError('chosen wanted-poster image identity changed')
    with Image.open(POSTER_PNG) as image:
        width, height = image.size
        # PNG decoding/engine encoding only: no repaint, crop or resampling.
        pixels = image.convert('RGBA').tobytes()
    if width * old_h != height * old_w:
        raise ValueError('poster aspect ratio changed')
    header = bytearray(original[:92])
    struct.pack_into('<IIIIffI', header, 64, kind, width, height, flags,
                     hscale * width / old_w, vscale * height / old_h, len(pixels))
    return bytes(header) + pixels


def build(source, output):
    source, output = Path(source), Path(output)
    if output.resolve() == source.resolve() or output.exists():
        raise ValueError('use a new candidate path; preserve source and existing outputs')
    if digest(source.read_bytes()) != SOURCE_SHA:
        raise ValueError('expected immutable native-segment FINAL candidate')
    with zipfile.ZipFile(source) as archive:
        names = archive.namelist()
        if len(set(names)) != len(names) or archive.testzip():
            raise ValueError('invalid source ZIP')
        resources = {name: archive.read(name) for name in names}
    before = resources[RESOURCE]
    resources[RESOURCE] = texture(before)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('xb') as stream, zipfile.ZipFile(stream, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(resources.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0)); info.create_system = 3
            info.external_attr = 0o100644 << 16; info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data, compresslevel=9)
    with zipfile.ZipFile(output) as archive:
        if archive.testzip() or set(archive.namelist()) != set(resources):
            raise ValueError('candidate archive framing changed')
        for name, data in resources.items():
            if archive.read(name) != data:
                raise ValueError('candidate resource verification failed: ' + name)
    return {'source_sha256': SOURCE_SHA, 'candidate_sha256': digest(output.read_bytes()),
            'name': output.name, 'size': output.stat().st_size, 'resource_count': len(resources),
            'changed_resources': [RESOURCE], 'unchanged_resources': len(resources) - 1,
            'poster_png_sha256': POSTER_SHA, 'texture_before_sha256': digest(before),
            'texture_after_sha256': digest(resources[RESOURCE]),
            'preservation': 'only wanted-poster texture replaced; scene, collision, art geometry and native scroll flags unchanged',
            'runtime': 'unverified configured game/Prelude'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path); parser.add_argument('output', type=Path)
    args = parser.parse_args()
    receipt = build(args.source, args.output)
    print(json.dumps(receipt, indent=2))
