"""Lossless bounded source storage matching the established morning workflow."""
import base64
import hashlib
from pathlib import Path, PurePosixPath
import zlib

MAX_RAW = 64 * 1024 * 1024
MAX_PACKED = 16 * 1024 * 1024
PART_BYTES = 96 * 1024
MAX_PARTS = 256


def sha(data):
    return hashlib.sha256(data).hexdigest()


def safe_path(name):
    if not isinstance(name, str) or '\\' in name or '\0' in name:
        return False
    path = PurePosixPath(name)
    return (not path.is_absolute() and str(path) == name and
            not any(part in ('.', '..') for part in path.parts) and
            name.removeprefix('alt/').startswith(('custom/temple_sages/', 'scenes/shared/tokinoma_scene/')))


def pack(resources, directory):
    """Pool exact byte sequences, compress once, and emit ordered text parts."""
    unique = {}
    for name, data in resources.items():
        if not safe_path(name) or not data:
            raise ValueError('Invalid chamber resource path or empty resource: ' + name)
        unique[sha(data)] = data
    offsets, pool, offset = {}, [], 0
    for digest, data in sorted(unique.items()):
        offsets[digest] = offset
        pool.append(data)
        offset += len(data)
    raw = b''.join(pool)
    packed = zlib.compress(raw, 9)
    if not 0 < len(raw) <= MAX_RAW or not 0 < len(packed) <= MAX_PACKED:
        raise ValueError('Chamber source pool size limit')
    directory = Path(directory)
    (directory / 'parts').mkdir(parents=True, exist_ok=True)
    parts = []
    for number, start in enumerate(range(0, len(packed), PART_BYTES), 1):
        name = f'parts/pool.zlib.{number:03d}.b64'
        encoded = base64.b64encode(packed[start:start + PART_BYTES])
        (directory / name).write_bytes(encoded)
        parts.append({'name': name, 'encoded_bytes': len(encoded), 'sha256': sha(encoded)})
    if len(parts) > MAX_PARTS:
        raise ValueError('Chamber source part-count limit')
    entries = [{'path': name, 'offset': offsets[sha(data)], 'bytes': len(data), 'sha256': sha(data)}
               for name, data in sorted(resources.items())]
    return {'version': 1, 'name': 'temple_sages_pool',
            'raw_bytes': len(raw), 'raw_sha256': sha(raw),
            'packed_bytes': len(packed), 'packed_sha256': sha(packed),
            'parts': parts, 'files': entries}


def decode(directory, manifest):
    """Check every layer before returning any reconstructed runtime resource."""
    directory = Path(directory)
    if manifest.get('version') != 1 or not 0 < manifest.get('raw_bytes', 0) <= MAX_RAW:
        raise ValueError('Chamber decoded-size limit or source version')
    if not 0 < manifest.get('packed_bytes', 0) <= MAX_PACKED:
        raise ValueError('Chamber packed-size limit')
    parts = manifest.get('parts', [])
    if not 0 < len(parts) <= MAX_PARTS:
        raise ValueError('Chamber source part-count limit')
    chunks = []
    for number, part in enumerate(parts, 1):
        if part.get('name') != f'parts/pool.zlib.{number:03d}.b64':
            raise ValueError('Invalid chamber source part order/path')
        path = directory / part['name']
        if not path.is_file():
            raise ValueError('Missing chamber source part: ' + part['name'])
        if path.stat().st_size > 128 * 1024:
            raise ValueError('Chamber encoded part size limit')
        encoded = path.read_bytes()
        if len(encoded) != part['encoded_bytes'] or sha(encoded) != part['sha256']:
            raise ValueError('Chamber source part hash/size mismatch: ' + part['name'])
        try:
            chunk = base64.b64decode(encoded, validate=True)
        except ValueError as error:
            raise ValueError('Chamber source part is not valid base64') from error
        if len(chunk) > PART_BYTES:
            raise ValueError('Chamber decoded part size limit')
        chunks.append(chunk)
    packed = b''.join(chunks)
    if len(packed) != manifest['packed_bytes'] or sha(packed) != manifest['packed_sha256']:
        raise ValueError('Chamber packed pool hash/size mismatch')
    decoder = zlib.decompressobj()
    raw = decoder.decompress(packed, manifest['raw_bytes'] + 1)
    if (len(raw) != manifest['raw_bytes'] or not decoder.eof or decoder.unconsumed_tail or
            decoder.unused_data or sha(raw) != manifest['raw_sha256']):
        raise ValueError('Chamber decoded pool integrity or size limit')
    resources = {}
    for entry in manifest['files']:
        name = entry['path']
        if not safe_path(name) or name in resources:
            raise ValueError('Invalid or duplicate chamber resource path')
        start, size = entry['offset'], entry['bytes']
        if type(start) is not int or type(size) is not int or not 0 <= start < len(raw) or not 0 < size <= len(raw) - start:
            raise ValueError('Chamber resource pool range')
        data = raw[start:start + size]
        if sha(data) != entry['sha256']:
            raise ValueError('Chamber resource hash/size mismatch: ' + name)
        resources[name] = data
    return resources
