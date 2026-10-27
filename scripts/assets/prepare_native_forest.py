#!/usr/bin/env python3
"""Verify the one native forest source ZIP, then prepare both port trees."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
import native_forest
from native_forest_bundle import read_bundle

DEFAULT_MANIFEST = ROOT / 'tools/native_forest/manifest.json'


def verified_resources(manifest_path, bundle_path=None):
    manifest_path = Path(manifest_path)
    metadata = json.loads(manifest_path.read_text())
    if metadata.get('schema') != 1 or metadata.get('status') != 'generated':
        raise ValueError('Native forest source is blocked: the immutable native MM archive is still required; '
                         'run tools/black_market/native_forest.py --native-mm with the real donor first')
    bundle = metadata.get('bundle', {})
    payload = read_bundle(manifest_path.parent, bundle, bundle_path)
    expected = metadata.get('resources', {})
    if not expected:
        raise ValueError('Native forest source has no resource inventory')
    resources = {}
    # Validate every member before creating an output directory. A corrupt
    # source must never partially replace a previously prepared candidate.
    with zipfile.ZipFile(io.BytesIO(payload)) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)) or set(names) != set(expected):
            raise ValueError('Native forest bundle resource membership mismatch')
        for name in names:
            if not native_forest.safe_resource_path(name):
                raise ValueError(f'Unsafe native forest resource path: {name}')
            entry = expected[name]
            if archive.getinfo(name).file_size != entry['bytes']:
                raise ValueError(f'Native forest resource size mismatch: {name}')
            data = archive.read(name)
            if hashlib.sha256(data).hexdigest() != entry['sha256']:
                raise ValueError(f'Native forest resource hash mismatch: {name}')
            resources[name] = data
    native_forest.validate_resources(resources)
    return resources


def prepared_resources(manifest_path, bundle_path=None):
    # Authoring rebakes consume the original verified source; normal archive
    # preparation consumes the derived presentation. Keep those contracts apart.
    resources = verified_resources(manifest_path, bundle_path)
    metadata = json.loads(Path(manifest_path).read_text())
    polish = metadata.get('presentation_polish')
    if polish is not None:
        if polish.get('version') != 2 or not isinstance(polish.get('resources'), dict):
            raise ValueError('Unsupported native forest presentation polish')
        from forest_presentation import apply
        polished = apply(resources)
        changed = {name for name in resources if resources[name] != polished[name]}
        if changed != set(polish['resources']):
            raise ValueError('Native forest presentation polish membership mismatch')
        for name in changed:
            entry, data = polish['resources'][name], polished[name]
            if len(data) != entry['bytes'] or native_forest.sha(data) != entry['sha256']:
                raise ValueError(f'Native forest presentation polish hash/size mismatch: {name}')
        native_forest.validate_resources(polished)
        resources = polished
    return resources


def prepare(output_root, manifest_path=DEFAULT_MANIFEST, bundle_path=None):
    resources = prepared_resources(manifest_path, bundle_path)
    restored = 0
    for game in ('soh', 'mm'):
        root = Path(output_root) / game / 'assets/custom'
        for name, data in resources.items():
            target = root / name
            if target.is_file() and target.stat().st_size == len(data) and native_forest.file_sha(target) == native_forest.sha(data):
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(dir=target.parent, prefix='.native-forest-', delete=False) as stream:
                    temporary = Path(stream.name)
                    stream.write(data)
                os.replace(temporary, target)
                restored += 1
            finally:
                if temporary:
                    temporary.unlink(missing_ok=True)
    textures = sum(data[4:8] == b'XETO' for data in resources.values())
    print(f'PASS native forest source: {len(resources)} resources in both port trees, '
          f'{textures} authored textures ({restored} restored)')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-root', type=Path, default=ROOT)
    parser.add_argument('--manifest', type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument('--bundle', type=Path)
    args = parser.parse_args()
    try:
        prepare(args.output_root, args.manifest, args.bundle)
    except (ValueError, OSError, zipfile.BadZipFile) as error:
        parser.exit(1, f'{error}\n')


if __name__ == '__main__':
    main()
