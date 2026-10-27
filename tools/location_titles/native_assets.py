#!/usr/bin/env python3
"""Built-in native IA8 title artwork and optional smooth HD replacements."""
import base64
import hashlib
import json
from pathlib import Path
import re
import struct
import zipfile
import zlib

ASSETS = Path(__file__).parent / 'assets/native-v3'
PREFIX = 'custom/oot_location_titles/v1/'


def _require(condition, message):
    if not condition:
        raise ValueError('native title assets: ' + message)


def resources(directory=ASSETS):
    directory = Path(directory)
    manifest = json.loads((directory / 'manifest.json').read_text())
    _require(manifest['version'] == 1, 'manifest version')
    titles = manifest['titles']
    _require(len(titles) == 67 and len({row['id'] for row in titles}) == 67 and
             all(re.fullmatch('[0-9a-f]{4}', row['id']) for row in titles), 'English title inventory')
    expected = {PREFIX + '1/' + row['id'] for row in titles} | {PREFIX + 'market', PREFIX + 'lost_woods'}
    _require(set(manifest['resources']) == expected, 'resource inventory')
    encoded = (directory / 'cards.zlib.b64').read_bytes()
    _require(len(encoded) == manifest['encoded_bytes'] <= 131072 and
             hashlib.sha256(encoded).hexdigest() == manifest['encoded_sha256'], 'encoded size/hash')
    packed = base64.b64decode(encoded, validate=True)
    _require(len(packed) == manifest['packed_bytes'] <= 98304 and
             hashlib.sha256(packed).hexdigest() == manifest['packed_sha256'], 'compressed size/hash')
    size = manifest['decoded_bytes']
    _require(type(size) is int and 0 < size <= 2 * 1024 * 1024, 'decoded bound')
    decoder = zlib.decompressobj()
    data = decoder.decompress(packed, size + 1)
    _require(len(data) == size and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail and
             hashlib.sha256(data).hexdigest() == manifest['decoded_sha256'], 'decoded bounds/hash')
    _require(data[:8] == b'OTITLES1' and struct.unpack_from('<I', data, 8)[0] == 69, 'stream header')
    offset, result = 12, {}
    for _ in range(69):
        _require(offset + 2 <= len(data), 'name framing')
        length = struct.unpack_from('<H', data, offset)[0]
        offset += 2
        _require(0 < length <= 100 and offset + length + 4 <= len(data), 'name length')
        name = data[offset:offset + length].decode('ascii')
        offset += length
        count = struct.unpack_from('<I', data, offset)[0]
        offset += 4
        _require(name in expected and name not in result and 80 <= count <= 5840 and
                 offset + count <= len(data), 'resource name/size')
        body = data[offset:offset + count]
        offset += count
        record = manifest['resources'][name]
        _require(count == record['bytes'] and hashlib.sha256(body).hexdigest() == record['sha256'], 'resource hash')
        width = 144 if name.endswith('/lost_woods') else 240
        _require(body[:64] == struct.pack('<B3xIIQ', 0, 0x4F544558, 0, 0xDEADBEEFDEADBEEF).ljust(64, b'\0') and
                 struct.unpack_from('<IIII', body, 64) == (8, width, 24, width * 24) and
                 len(body) == 80 + width * 24, 'native IA8 framing')
        pixels = body[80:]
        edge = pixels[:width] + pixels[-width:] + pixels[::width] + pixels[width - 1::width]
        if name.endswith('/lost_woods'):
            # Its original native donor has alpha1 on the final row. Preserve
            # the exact source instead of silently clipping an accepted card.
            _require(hashlib.sha256(body).hexdigest() == manifest['forest_donor_sha256'], 'native forest identity')
        else:
            _require(not any(value & 15 for value in edge), 'transparent perimeter')
        _require(any(value >> 4 and value & 15 for value in pixels) and
                 any(not value >> 4 and value & 15 for value in pixels), 'face and baked black halo')
        result[name] = body
    _require(offset == len(data) and set(result) == expected, 'stream terminator/inventory')
    return result


def hd_resources(directory=ASSETS):
    # The native bitmap is the implementation, while the optional replacement
    # uses genuinely restored curves. It never merely repeats native pixels.
    import hd_assets
    require_native = resources(directory)
    result = hd_assets.resources()
    _require(set(result) == set(require_native), 'HD/native inventory parity')
    return result


def write_pack(path, hd=False):
    values = hd_resources() if hd else resources()
    with zipfile.ZipFile(path, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(values.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--hd', action='store_true')
    args = parser.parse_args()
    write_pack(args.output, args.hd)
    print('PASS: 69 ' + ('smooth HD replacement' if args.hd else 'native IA8') + ' title resources')
