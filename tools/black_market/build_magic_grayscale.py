#!/usr/bin/env python3
"""Build only Black Market magic base overrides; existing tint controls remain.

    python3 -B tools/black_market/build_magic_grayscale.py --output candidate.prelude.o2r

Load after competing Black Market texture overrides in each active host, then
restart to clear pinned texture caches. Requires an existing compatible native
Black Market installation. No executable, geometry or other artwork is included.
"""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

import market_interior
import room_magic


def build(output):
    """Write a deterministic four-entry ZIP and return its hash receipt."""
    data = market_interior.grayscale_magic_texture()
    paths = (market_interior.SHELF, room_magic.TEXTURE)
    resources = {prefix + path: data for prefix in ('', 'alt/') for path in paths}
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, payload in sorted(resources.items()):
            info = zipfile.ZipInfo(name, (1980,1,1,0,0,0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, payload, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    return dict(output=str(output), sha256=hashlib.sha256(output.read_bytes()).hexdigest(),
                pixels_sha256=hashlib.sha256(data[92:]).hexdigest(),
                resources={name:hashlib.sha256(payload).hexdigest()
                           for name, payload in sorted(resources.items())},
                runtime_verified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(build(args.output), indent=2, sort_keys=True))
