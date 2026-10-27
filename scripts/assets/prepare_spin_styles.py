#!/usr/bin/env python3
"""Restore exact shipped spin resources from bounded, lossless source parts."""
import argparse
import base64
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[2]
BUNDLE = ROOT / 'tools/spin_styles/bundled'
MAX_RAW = 64 * 1024 * 1024
MAX_PACKED = 16 * 1024 * 1024
PART_BYTES = 96 * 1024


def sha(data):
    return hashlib.sha256(data).hexdigest()


def decode(bundle=BUNDLE):
    bundle = Path(bundle)
    manifest = json.loads((bundle / 'manifest.json').read_text())
    if manifest['version'] != 1 or not 0 < manifest['raw_bytes'] <= MAX_RAW:
        raise ValueError('spin decoded-size limit or source version')
    if not 0 < manifest['packed_bytes'] <= MAX_PACKED or not 0 < len(manifest['parts']) <= 256:
        raise ValueError('spin packed-size/part-count limit')
    chunks = []
    for part in manifest['parts']:
        if Path(part['name']).name != part['name']:
            raise ValueError('invalid spin part name')
        path = bundle / part['name']
        if path.stat().st_size > 128 * 1024:
            raise ValueError('spin part size limit')
        encoded = path.read_bytes()
        if len(encoded) != part['encoded_bytes'] or sha(encoded) != part['sha256']:
            raise ValueError('spin part integrity: ' + part['name'])
        chunk = base64.b64decode(encoded, validate=True)
        if len(chunk) > PART_BYTES:
            raise ValueError('spin part decoded-size limit')
        chunks.append(chunk)
    packed = b''.join(chunks)
    if len(packed) != manifest['packed_bytes'] or sha(packed) != manifest['packed_sha256']:
        raise ValueError('spin packed pool integrity')
    decoder = zlib.decompressobj()
    raw = decoder.decompress(packed, manifest['raw_bytes'] + 1)
    if (len(raw) != manifest['raw_bytes'] or not decoder.eof or decoder.unconsumed_tail or
            decoder.unused_data or sha(raw) != manifest['raw_sha256']):
        raise ValueError('spin decoded pool integrity or size limit')
    paths = set()
    for entry in manifest['files']:
        path = PurePosixPath(entry['path'])
        if (path.is_absolute() or '..' in path.parts or len(path.parts) < 6 or
                path.parts[0] not in ('soh', 'mm') or
                path.parts[1:5] != ('assets', 'custom', 'objects', 'combo_spin') or str(path) in paths):
            raise ValueError('invalid or duplicate spin resource path')
        paths.add(str(path))
        start, size = entry['offset'], entry['bytes']
        if not 0 <= start < len(raw) or not 0 < size <= len(raw) - start:
            raise ValueError('spin resource pool range')
        if sha(raw[start:start + size]) != entry['sha256']:
            raise ValueError('spin resource integrity: ' + str(path))
    return manifest, raw


def prepare(output_root=ROOT, bundle=BUNDLE):
    manifest, raw = decode(bundle)
    root = Path(output_root).resolve()
    # Validate every destination before creating any output.
    for entry in manifest['files']:
        if not (root / entry['path']).resolve().is_relative_to(root):
            raise ValueError('spin resource destination escapes output root')
    written = 0
    for entry in manifest['files']:
        target = root / entry['path']
        data = raw[entry['offset']:entry['offset'] + entry['bytes']]
        if target.is_file() and target.read_bytes() == data:
            continue
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = None
        try:
            with tempfile.NamedTemporaryFile(dir=target.parent, prefix='.spin-style-', delete=False) as stream:
                temporary = Path(stream.name)
                stream.write(data)
            os.replace(temporary, target)
            written += 1
        finally:
            if temporary is not None:
                temporary.unlink(missing_ok=True)
    print(f'PASS exact spin resources ({len(manifest["files"])} paths; {written} files written)')
    return written


def pack():
    """Author source parts only from resources matching the preserved manifest."""
    outputs = json.loads((ROOT / 'tools/spin_styles/manifest.json').read_text())['outputs']
    unique, entries = {}, []
    for host, resources in sorted(outputs.items()):
        for name, digest in sorted(resources.items()):
            path = Path(host) / 'assets/custom' / name
            data = (ROOT / path).read_bytes()
            if sha(data) != digest:
                raise ValueError('Refusing to package changed spin resource: ' + str(path))
            unique[digest] = data
            entries.append({'path': path.as_posix(), 'sha256': digest, 'bytes': len(data)})
    pool, offsets, offset = [], {}, 0
    for digest, data in sorted(unique.items()):
        offsets[digest] = offset
        pool.append(data)
        offset += len(data)
    raw = b''.join(pool)
    packed = zlib.compress(raw, 9)
    if len(raw) > MAX_RAW or len(packed) > MAX_PACKED:
        raise ValueError('spin source pool size limit')
    BUNDLE.mkdir(parents=True, exist_ok=True)
    parts = []
    for number, start in enumerate(range(0, len(packed), PART_BYTES), 1):
        name = f'pool.zlib.{number:03d}.b64'
        encoded = base64.b64encode(packed[start:start + PART_BYTES])
        (BUNDLE / name).write_bytes(encoded)
        parts.append({'name': name, 'encoded_bytes': len(encoded), 'sha256': sha(encoded)})
    for entry in entries:
        entry['offset'] = offsets[entry['sha256']]
    manifest = {'version': 1, 'raw_bytes': len(raw), 'raw_sha256': sha(raw),
                'packed_bytes': len(packed), 'packed_sha256': sha(packed),
                'parts': parts, 'files': entries}
    (BUNDLE / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Packed {len(entries)} exact resources, {len(unique)} unique, {len(parts)} bounded parts')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pack', action='store_true', help='repackage current manifest-verified resources')
    parser.add_argument('--output-root', type=Path, default=ROOT)
    args = parser.parse_args()
    if args.pack:
        pack()
    else:
        prepare(args.output_root)
