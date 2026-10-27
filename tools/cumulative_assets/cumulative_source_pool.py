"""Lossless source pools using the proven 96 KiB zlib / 128 KiB base64 parts."""
import base64
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import tempfile
import zlib

MAX_RAW = 64 * 1024 * 1024
MAX_PACKED = 16 * 1024 * 1024
PART_BYTES = 96 * 1024
ENCODED_PART_BYTES = 128 * 1024


def sha(data):
    return hashlib.sha256(data).hexdigest()


def _size(value, label, limit):
    if type(value) is not int or not 0 < value <= limit:
        raise ValueError('source pool ' + label + ' size limit')
    return value


def _digest(value, label):
    if not isinstance(value, str) or not re.fullmatch('[0-9a-f]{64}', value):
        raise ValueError('source pool ' + label + ' hash')
    return value


def _path(value):
    if not isinstance(value, str) or '\\' in value or '\0' in value:
        raise ValueError('invalid source resource path')
    path = PurePosixPath(value)
    if path.is_absolute() or not path.parts or '..' in path.parts or str(path) != value:
        raise ValueError('invalid source resource path')
    return value


def decode(bundle, expected):
    """Validate all bounds, identities and exact output membership before writes."""
    bundle = Path(bundle)
    manifest = json.loads((bundle / 'manifest.json').read_text())
    if manifest.get('version') != 1 or manifest.get('encoding') != 'zlib-base64-pool':
        raise ValueError('unsupported source pool version/encoding')
    raw_size = _size(manifest.get('raw_bytes'), 'decoded', MAX_RAW)
    packed_size = _size(manifest.get('packed_bytes'), 'packed', MAX_PACKED)
    raw_sha = _digest(manifest.get('raw_sha256'), 'decoded')
    packed_sha = _digest(manifest.get('packed_sha256'), 'packed')
    entries = manifest.get('files')
    if not isinstance(entries, list) or not 0 < len(entries) <= 1024:
        raise ValueError('invalid source resource inventory')
    seen = set()
    for entry in entries:
        name = _path(entry.get('path'))
        if name in seen or name not in expected:
            raise ValueError('invalid or duplicate source resource path: ' + name)
        seen.add(name)
        start, size = entry.get('offset'), entry.get('bytes')
        if type(start) is not int or not 0 <= start < raw_size:
            raise ValueError('source resource pool range: ' + name)
        _size(size, 'resource', raw_size - start)
        digest = _digest(entry.get('sha256'), 'resource')
        if size != expected[name]['bytes'] or digest != expected[name]['sha256']:
            raise ValueError('source resource hash/size identity mismatch: ' + name)
    if seen != set(expected):
        raise ValueError('source resource inventory membership mismatch')
    parts = manifest.get('parts')
    if not isinstance(parts, list) or not 0 < len(parts) <= 256:
        raise ValueError('source part count limit')
    total = 0
    for number, part in enumerate(parts, 1):
        if part.get('name') != f'pool.zlib.{number:03d}.b64':
            raise ValueError('source part order/path')
        size = _size(part.get('bytes'), 'part decoded', PART_BYTES)
        encoded_size = _size(part.get('encoded_bytes'), 'part encoded', ENCODED_PART_BYTES)
        if encoded_size != 4 * ((size + 2) // 3):
            raise ValueError('source part encoded-size mismatch')
        _digest(part.get('sha256'), 'part encoded')
        _digest(part.get('decoded_sha256'), 'part decoded')
        total += size
    if total != packed_size:
        raise ValueError('source part aggregate size mismatch')
    chunks = []
    for part in parts:
        path = bundle / part['name']
        if not path.is_file() or path.stat().st_size != part['encoded_bytes']:
            raise ValueError('missing or incorrect source part size: ' + part['name'])
        encoded = path.read_bytes()
        if sha(encoded) != part['sha256']:
            raise ValueError('source part encoded hash: ' + part['name'])
        try:
            chunk = base64.b64decode(encoded, validate=True)
        except ValueError as error:
            raise ValueError('source part base64: ' + part['name']) from error
        if len(chunk) != part['bytes'] or sha(chunk) != part['decoded_sha256']:
            raise ValueError('source part decoded hash/size: ' + part['name'])
        chunks.append(chunk)
    packed = b''.join(chunks)
    if len(packed) != packed_size or sha(packed) != packed_sha:
        raise ValueError('source packed pool hash/size')
    decoder = zlib.decompressobj()
    try:
        raw = decoder.decompress(packed, raw_size + 1)
    except zlib.error as error:
        raise ValueError('source pool decompression') from error
    if (len(raw) != raw_size or not decoder.eof or decoder.unconsumed_tail or
            decoder.unused_data or sha(raw) != raw_sha):
        raise ValueError('source decoded pool hash/size or bounds')
    files = {}
    for entry in entries:
        data = raw[entry['offset']:entry['offset'] + entry['bytes']]
        if sha(data) != entry['sha256']:
            raise ValueError('source resource hash: ' + entry['path'])
        files[entry['path']] = data
    return files


def write_files(output_root, files):
    root = Path(output_root).resolve()
    for name in files:
        _path(name)
        if not (root / name).resolve().is_relative_to(root):
            raise ValueError('source resource destination escapes output root')
    written = 0
    for name, data in files.items():
        path = root / name
        if path.is_file() and path.read_bytes() == data:
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary = None
        try:
            with tempfile.NamedTemporaryFile(dir=path.parent, prefix='.source-pool-', delete=False) as stream:
                temporary = Path(stream.name)
                stream.write(data)
            os.replace(temporary, path)
            written += 1
        finally:
            if temporary is not None:
                temporary.unlink(missing_ok=True)
    return written


def pack(files, bundle):
    """Author ordered parts from exact supplied bytes, retaining all identities."""
    if not files or len(files) > 1024:
        raise ValueError('invalid source resource inventory')
    unique, entries = {}, []
    for name, data in sorted(files.items()):
        _path(name)
        if not data:
            raise ValueError('empty source resource: ' + name)
        digest = sha(data)
        unique[digest] = data
        entries.append({'path': name, 'bytes': len(data), 'sha256': digest})
    offsets, pool, offset = {}, [], 0
    for digest, data in sorted(unique.items()):
        offsets[digest] = offset
        pool.append(data)
        offset += len(data)
    raw = b''.join(pool)
    packed = zlib.compress(raw, 9)
    _size(len(raw), 'decoded', MAX_RAW)
    _size(len(packed), 'packed', MAX_PACKED)
    bundle = Path(bundle)
    bundle.mkdir(parents=True, exist_ok=True)
    parts = []
    for number, start in enumerate(range(0, len(packed), PART_BYTES), 1):
        chunk = packed[start:start + PART_BYTES]
        encoded = base64.b64encode(chunk)
        name = f'pool.zlib.{number:03d}.b64'
        (bundle / name).write_bytes(encoded)
        parts.append({'name': name, 'bytes': len(chunk), 'encoded_bytes': len(encoded),
                      'sha256': sha(encoded), 'decoded_sha256': sha(chunk)})
    for entry in entries:
        entry['offset'] = offsets[entry['sha256']]
    manifest = {'version': 1, 'encoding': 'zlib-base64-pool', 'raw_bytes': len(raw),
                'raw_sha256': sha(raw), 'packed_bytes': len(packed),
                'packed_sha256': sha(packed), 'parts': parts, 'files': entries}
    (bundle / 'manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    expected_names = {part['name'] for part in parts}
    for stale in bundle.glob('pool.zlib.*.b64'):
        if stale.name not in expected_names:
            stale.unlink()
    return manifest
