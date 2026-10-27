#!/usr/bin/env python3
"""Restore verified built-in chamber resources into the normal port trees."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/temple_sages_bundle'))
from temple_sages_pool import decode, safe_path
from packaged_resources import missing_dependencies

DEFAULT_MANIFEST = ROOT / 'tools/temple_sages_bundle/manifest.json'


def validate_dependencies(resources, manifest_path=DEFAULT_MANIFEST):
    metadata = json.loads(Path(manifest_path).read_text())
    return missing_dependencies(resources, metadata['native_dependencies'])


def verified_resources(manifest_path=DEFAULT_MANIFEST):
    manifest_path = Path(manifest_path)
    metadata = json.loads(manifest_path.read_text())
    if metadata.get('schema') != 1 or metadata.get('external_mod_dependencies') != []:
        raise ValueError('Invalid packaged chamber manifest')
    resources = decode(manifest_path.parent, metadata['bundle'])
    if set(resources) != set(metadata['resources']):
        raise ValueError('Packaged chamber resource membership mismatch')
    for name, data in resources.items():
        entry = metadata['resources'][name]
        if len(data) != entry['bytes'] or hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise ValueError('Packaged chamber hash/size mismatch: ' + name)
    retired = metadata.get('retired_resources', [])
    if (not isinstance(retired, list) or len(retired) > 65536 or
            any(not safe_path(name) or name in resources for name in retired) or
            len(set(retired)) != len(retired)):
        raise ValueError('Invalid retired chamber resource')
    missing = validate_dependencies(resources, manifest_path)
    if missing:
        raise ValueError('Missing packaged chamber dependencies: ' + repr(missing[:8]))
    return resources


def prepare(output_root, manifest_path=DEFAULT_MANIFEST):
    # Validate the entire source before touching either game's prior assets.
    resources = verified_resources(manifest_path)
    metadata = json.loads(Path(manifest_path).read_text())
    expected = set(resources)
    output_root = Path(output_root).resolve()
    # Validate every future destination and swept namespace before touching
    # either game's existing generated assets, including symlink resolutions.
    for game in ('soh', 'mm'):
        root = output_root / game / 'assets/custom'
        resolved = root.resolve()
        if not resolved.is_relative_to(output_root):
            raise ValueError('Chamber asset root escapes output root')
        if resolved != root:
            raise ValueError('Chamber asset root is redirected by a symlink')
        for name in (*expected, *metadata.get('retired_resources', [])):
            destination = root / name
            actual = destination.resolve()
            if not actual.is_relative_to(resolved):
                raise ValueError('Chamber resource destination escapes output root')
            if actual != destination:
                raise ValueError('Chamber resource destination is redirected by a symlink')
        for prefix in ('custom/temple_sages', 'alt/custom/temple_sages'):
            directory = root / prefix
            actual = directory.resolve()
            if not actual.is_relative_to(resolved):
                raise ValueError('Chamber private namespace escapes output root')
            if actual != directory:
                raise ValueError('Chamber private namespace is redirected by a symlink')
    restored = 0
    for game in ('soh', 'mm'):
        root = Path(output_root) / game / 'assets/custom'
        # These two private namespaces belong only to this preparer. An older
        # expanded visual bundle must not leak into a newly native-art build.
        # Ordinary scene paths are never swept; only exact retired members
        # recorded by the source builder may be removed there.
        for prefix in ('custom/temple_sages', 'alt/custom/temple_sages'):
            directory = root / prefix
            if directory.is_dir():
                for path in directory.rglob('*'):
                    if path.is_file() and path.relative_to(root).as_posix() not in expected:
                        path.unlink()
        for name in metadata.get('retired_resources', []):
            path = root / name
            if path.is_file():
                path.unlink()
        for name, data in resources.items():
            path = root / name
            if path.is_file() and path.read_bytes() == data:
                continue
            path.parent.mkdir(parents=True, exist_ok=True)
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(dir=path.parent, prefix='.temple-sages-', delete=False) as stream:
                    temporary = Path(stream.name)
                    stream.write(data)
                os.replace(temporary, path)
                restored += 1
            finally:
                if temporary is not None:
                    temporary.unlink(missing_ok=True)
    print(f'PASS packaged chamber: {len(resources)} resources in both port trees ({restored} restored)')
    return resources


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-root', type=Path, default=ROOT)
    parser.add_argument('--manifest', type=Path, default=DEFAULT_MANIFEST)
    args = parser.parse_args()
    try:
        prepare(args.output_root, args.manifest)
    except (ValueError, OSError) as error:
        parser.exit(1, str(error) + '\n')
