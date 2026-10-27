#!/usr/bin/env python3
"""Fit frozen authored forest cast art into private native RGBA32 resources."""
import hashlib
import json
from pathlib import Path
import struct
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
ART = Path(__file__).with_name('cast')


def build():
    manifest = {}
    for name in ('forest_seal', 'forest_wisp'):
        source = ART / (name + '.png')
        image = Image.open(source).convert('RGBA')
        # Transparent padding avoids clamp-edge streaks and wrap seams. Preserve
        # authored RGB and alpha; only fit/resize for native texture consumption.
        margin = 24 if name == 'forest_seal' else 16
        fitted = image.resize((512 - 2 * margin, 512 - 2 * margin), Image.Resampling.LANCZOS)
        tile = Image.new('RGBA', (512, 512))
        tile.paste(fitted, (margin, margin))
        header = bytearray(64)
        struct.pack_into('<IIIQQ', header, 0, 0, 0x4F544558, 1, 0xDEADBEEFDEADBEEF, 1 << 32)
        raw = bytes(header) + struct.pack('<IIIIffI', 1, 512, 512, 1, 16., 16., 512 * 512 * 4) + tile.tobytes()
        for host in ('soh', 'mm'):
            target = ROOT / host / 'assets/custom/objects/magic_mirror' / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(raw)
        manifest[name] = dict(source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                              resource_sha256=hashlib.sha256(raw).hexdigest(),
                              source_pixels=list(image.size), resource_pixels=[512, 512],
                              logical_tile=[32, 32], margin=margin,
                              origin='Built-in ImageGen; original botanical forest VFX, transparent RGBA')
    (ART / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print('Built two private forest cast textures for both native hosts')


if __name__ == '__main__':
    build()
