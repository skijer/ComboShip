#!/usr/bin/env python3
"""Restore the exact authored 4K resources for normal port-archive generation."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
import zipfile

HERE = Path(__file__).resolve().parent


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest()


def unpack(destination, other_assets_root=None):
    manifest = json.loads((HERE / 'material_manifest.json').read_text())
    archive_path = HERE / 'assets.zip'
    if sha256(archive_path) != manifest['pack']['sha256']:
        raise ValueError('Reward asset bundle does not match the authored manifest')
    restored = 0
    with zipfile.ZipFile(archive_path) as archive:
        expected = set(manifest['resource_scope'])
        if set(archive.namelist()) != expected or len(archive.infolist()) != len(expected):
            raise ValueError('Reward asset bundle must contain exactly the seven private resources')
        if other_assets_root is not None:
            for key in expected:
                if (other_assets_root / key).exists():
                    raise ValueError(f'Reward private resource collision: {key}')
        for entry in manifest['entries'].values():
            key = entry['target']
            target = destination / key
            info = archive.getinfo(key)
            if info.file_size != entry['resource_bytes']:
                raise ValueError(f'Incorrect resource length: {key}')
            if target.is_file() and target.stat().st_size == info.file_size and sha256(target) == entry['resource_sha256']:
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(dir=target.parent, delete=False) as output:
                    temporary = Path(output.name)
                    digest = hashlib.sha256()
                    with archive.open(info) as incoming:
                        for block in iter(lambda: incoming.read(1024 * 1024), b''):
                            digest.update(block)
                            output.write(block)
                if digest.hexdigest() != entry['resource_sha256']:
                    raise ValueError(f'Incorrect resource hash: {key}')
                os.replace(temporary, target)
                restored += 1
            finally:
                if temporary is not None:
                    temporary.unlink(missing_ok=True)
    print(f'Reward assets verified: 7 exact 4K resources ({restored} restored)')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assets-root', required=True, type=Path)
    parser.add_argument('--other-assets-root', type=Path)
    args = parser.parse_args()
    unpack(args.assets_root, args.other_assets_root)
