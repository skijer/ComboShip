#!/usr/bin/env python3
"""Prepare built-in native OoT title cards into both ordinary port archives."""
import argparse
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/location_titles'))
import native_assets


def prepare(output_root=ROOT):
    written = 0
    for host in ('soh', 'mm'):
        for name, data in sorted(native_assets.resources().items()):
            target = Path(output_root) / host / 'assets/custom' / name
            if target.is_file() and target.read_bytes() == data:
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(dir=target.parent, prefix='.native-title-', delete=False) as stream:
                    temporary = Path(stream.name)
                    stream.write(data)
                os.replace(temporary, target)
                written += 1
            finally:
                if temporary is not None:
                    temporary.unlink(missing_ok=True)
    print(f'PASS: 69 built-in native title resources for both hosts ({written} files written)')
    return written


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-root', type=Path, default=ROOT)
    args = parser.parse_args()
    prepare(args.output_root)
