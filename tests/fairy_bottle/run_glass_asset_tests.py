#!/usr/bin/env python3
"""Verify both shipped glass graphs and reproduce their resource bytes."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    manifest = json.loads((ROOT / 'tools/song_bottle_20261008/bottle_assets.json').read_text())
    resources = manifest['resources']
    assert manifest['checks']['fairy_glass_alpha'] == 80
    for host in ('soh', 'mm'):
        for resource, expected in resources.items():
            blob = (ROOT / host / 'assets/custom' / resource).read_bytes()
            assert hashlib.sha256(blob).hexdigest() == expected, (host, resource, 'shipped resource drift')
    source = ROOT / manifest['source']
    assert hashlib.sha256(source.read_bytes()).hexdigest() == manifest['source_sha256']
    with tempfile.TemporaryDirectory(prefix='fairy-glass-rebuild-') as temporary:
        build = Path(temporary)
        copied_source = build / 'source.o2r'
        shutil.copyfile(source, copied_source)
        subprocess.run([sys.executable, '-B', str(ROOT / 'scripts/mods/build_bottle_contents_gi.py'),
                        '--source-o2r', str(copied_source), '--repo', str(build)], check=True)
        rebuilt = json.loads((build / 'tools/song_bottle_20261008/bottle_assets.json').read_text())
        assert rebuilt['resources'] == resources and rebuilt['checks'] == manifest['checks']
        for host in ('soh', 'mm'):
            for resource in resources:
                assert ((build / host / 'assets/custom' / resource).read_bytes() ==
                        (ROOT / host / 'assets/custom' / resource).read_bytes()), (host, resource)
    print('PASS both-owner closed bottle resource graphs, source hashes and deterministic fairy-glass rebuild')


if __name__ == '__main__':
    main()
