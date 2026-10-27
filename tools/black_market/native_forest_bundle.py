"""Read one exact native forest ZIP from verified storage parts or a legacy file."""
import hashlib
import json
from pathlib import Path
import re


MAX_PART_BYTES = 8 * 1024 * 1024


def _identity(entry, label, limit=None):
    size, digest = entry.get('bytes'), entry.get('sha256')
    if type(size) is not int or size <= 0 or (limit is not None and size > limit):
        raise ValueError(f'Invalid native forest bundle {label} size/limit')
    if not isinstance(digest, str) or not re.fullmatch('[0-9a-f]{64}', digest):
        raise ValueError(f'Invalid native forest bundle {label} hash')
    return size, digest


def _read_file(path, entry, label):
    if not path.is_file():
        raise ValueError(f'Missing native forest bundle {label}: {path}')
    if path.stat().st_size != entry['bytes']:
        raise ValueError(f'Native forest bundle {label} size mismatch: {path}')
    data = path.read_bytes()
    if len(data) != entry['bytes'] or hashlib.sha256(data).hexdigest() != entry['sha256']:
        raise ValueError(f'Native forest bundle {label} hash/size mismatch: {path}')
    return data


def read_bundle(directory, bundle, bundle_path=None):
    """Validate all storage identities before returning the canonical ZIP bytes.

    Explicit canonical-file overrides and old single-ZIP manifests keep their
    existing contract. Otherwise a parts manifest always reads its parts; an
    ignored local ZIP cannot hide missing, corrupt or reordered source files.
    """
    directory = Path(directory)
    name = bundle.get('name', '')
    if not isinstance(name, str) or not re.fullmatch('[A-Za-z0-9_-][A-Za-z0-9_.-]*', name):
        raise ValueError('Invalid native forest bundle filename/path')
    size, digest = _identity(bundle, 'aggregate')
    if bundle_path is not None:
        return _read_file(Path(bundle_path), bundle, 'aggregate')
    if 'parts' not in bundle:
        return _read_file(directory / name, bundle, 'aggregate')
    parts = bundle['parts']
    if not isinstance(parts, list) or not parts or len(parts) > 999:
        raise ValueError('Invalid native forest bundle parts order/count')
    total = 0
    for number, part in enumerate(parts, 1):
        if not isinstance(part, dict) or part.get('name') != f'parts/{name}.part{number:03d}':
            raise ValueError('Invalid native forest bundle part order/path')
        total += _identity(part, 'part', MAX_PART_BYTES)[0]
    if total != size:
        raise ValueError('Native forest bundle parts aggregate size mismatch')
    data = b''.join(_read_file(directory / part['name'], part, 'part') for part in parts)
    if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
        raise ValueError('Native forest bundle aggregate hash/size mismatch')
    return data


def load_bundle(directory: Path) -> bytes:
    """Load directory/manifest.json and return its exact, verified source ZIP."""
    directory = Path(directory)
    metadata = json.loads((directory / 'manifest.json').read_text())
    return read_bundle(directory, metadata.get('bundle', {}))
