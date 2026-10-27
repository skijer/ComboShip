#!/usr/bin/env python3
"""Restore exact cumulative runtime/authoring assets from bounded source parts."""
import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'tools/cumulative_assets'
BUNDLE = SOURCE / 'bundled'
OVERLAY = SOURCE / 'morning-20261027'
BASELINE = SOURCE / 'baseline_source_files.json'
sys.path.insert(0, str(SOURCE))
from cumulative_source_pool import decode, pack, sha, write_files


def inventory(path=SOURCE / 'source_files.json'):
    entries = json.loads(path.read_text())['files']
    result = {entry['path']: entry for entry in entries}
    if len(result) != len(entries):
        raise ValueError('duplicate cumulative source identity')
    return result


def verified_files(bundle=BUNDLE, overlay=OVERLAY):
    baseline = inventory(BASELINE)
    current = inventory()
    files = decode(bundle, baseline)
    updates = {name: entry for name, entry in current.items() if entry != baseline.get(name)}
    files.update(decode(overlay, updates))
    if set(files) != set(current):
        raise ValueError('cumulative source inventory membership mismatch')
    return files


def prepare(output_root=ROOT, bundle=BUNDLE, overlay=OVERLAY):
    files = verified_files(bundle, overlay)
    count = write_files(output_root, files)
    print(f'PASS exact cumulative assets ({len(files)} paths; {count} restored)')
    return count


def pack_source():
    files = {}
    baseline = inventory(BASELINE)
    for name, entry in inventory().items():
        data = (ROOT / name).read_bytes()
        if len(data) != entry['bytes'] or sha(data) != entry['sha256']:
            raise ValueError('Refusing changed cumulative source bytes: ' + name)
        if entry != baseline.get(name):
            files[name] = data
    manifest = pack(files, OVERLAY)
    print(f'Packed {len(files)} exact cumulative overrides in {len(manifest["parts"])} bounded text parts')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pack', action='store_true', help='author parts only from pinned source bytes')
    parser.add_argument('--output-root', type=Path, default=ROOT)
    parser.add_argument('--bundle', type=Path, default=BUNDLE)
    parser.add_argument('--overlay', type=Path, default=OVERLAY)
    args = parser.parse_args()
    try:
        if args.pack:
            pack_source()
        else:
            prepare(args.output_root, args.bundle, args.overlay)
    except (ValueError, OSError) as error:
        parser.exit(1, str(error) + '\n')
