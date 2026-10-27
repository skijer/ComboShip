#!/usr/bin/env python3
"""Prepare the retained market and original default surfaces for both ports."""
import argparse
import hashlib
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/black_market"))
import native_market
import market_interior
import market_title
import exit_door
import mm_native_shop
import mm_shop_presentation
import room_magic

SOURCE = ROOT / "tools/native_market/bundled"


def prepared_resources(bundle_directory=SOURCE, host=None):
    if host not in (None, 'oot', 'mm'):
        raise ValueError('native market host must be mm or oot')
    resources = exit_door.apply(market_title.apply(market_interior.apply(native_market.read_bundle(bundle_directory))))
    if host == 'mm':
        resources = mm_shop_presentation.apply(mm_native_shop.apply(resources))
    return room_magic.apply(resources, 'mm' if host == 'mm' else 'oot')


def prepare(output_root=ROOT, bundle_directory=SOURCE):
    resources_by_game = {game: prepared_resources(bundle_directory, host=host)
                         for game, host in (('soh', 'oot'), ('mm', 'mm'))}
    written = 0
    for game, resources in resources_by_game.items():
        for name, data in sorted(resources.items()):
            target = Path(output_root) / game / "assets/custom" / name
            if target.is_file() and hashlib.sha256(target.read_bytes()).digest() == hashlib.sha256(data).digest():
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            temporary = None
            try:
                with tempfile.NamedTemporaryFile(dir=target.parent, prefix=".native-market-", delete=False) as stream:
                    temporary = Path(stream.name)
                    stream.write(data)
                os.replace(temporary, target)
                written += 1
            finally:
                if temporary is not None:
                    temporary.unlink(missing_ok=True)
    print(f"PASS: {len(resources)} native market resources prepared for soh.o2r and 2ship.o2r ({written} files written)")
    return written


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-root", type=Path, default=ROOT)
    parser.add_argument("--bundle-dir", type=Path, default=SOURCE)
    args = parser.parse_args()
    prepare(args.output_root, args.bundle_dir)
