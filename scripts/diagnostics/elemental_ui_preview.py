#!/usr/bin/env python3
"""Inspect the production selector via its real submitted GBI, without running a game.

This is an offline visual approximation of filtering/blending, not runtime proof.
The native sword icon is omitted unless --sword-icon supplies its actual PNG.
"""
from pathlib import Path
import argparse
import json
import os
import subprocess
import tempfile

import numpy as np
from PIL import Image, ImageDraw

from sword_element_kaleido_tests import ROOT, flags, compile_source


def render(prefix, sword_icon=None, scale=4):
    data = json.loads(Path(str(prefix) + '.json').read_text())
    textures = []
    for meta in data['textures']:
        size = (meta['width'], meta['height'])
        if meta['native']:
            image = Image.open(sword_icon).convert('RGBA').resize(size) if sword_icon else Image.new('RGBA', size)
        else:
            pixels = Path(f'{prefix}-t{meta["index"]:02}.bin').read_bytes()
            if meta['format'] == 0:
                image = Image.frombytes('RGBA', size, pixels)
            else:
                packed = np.frombuffer(pixels, np.uint8)
                rgba = np.zeros((len(packed), 4), np.uint8)
                rgba[:, :3] = ((packed >> 4) * 17)[:, None]
                rgba[:, 3] = (packed & 15) * 17
                image = Image.fromarray(rgba.reshape((size[1], size[0], 4)), 'RGBA')
        textures.append(image)
    canvas = Image.new('RGBA', (320 * scale, 240 * scale))
    draw = ImageDraw.Draw(canvas)
    for y in range(canvas.height):
        t = y / canvas.height
        draw.line((0, y, canvas.width, y), fill=(int(20 - 10 * t), int(29 - 13 * t), int(47 - 20 * t), 255))
    for rect in data['rectangles']:
        texture = textures[rect['texture']]
        x1, y1, x2, y2 = [round(x * scale / 4) for x in rect['box4']]
        if x2 <= x1 or y2 <= y1:
            continue
        s, t, dx, dy = rect['uv']
        # N64 texture coordinates are S10.5; texture steps are S5.10.
        bounds = (s / 32, t / 32,
                  s / 32 + (x2 - x1) / scale * dx / 1024,
                  t / 32 + (y2 - y1) / scale * dy / 1024)
        patch = texture.transform((x2 - x1, y2 - y1), Image.Transform.EXTENT, bounds,
                                  resample=Image.Resampling.BILINEAR)
        values = np.asarray(patch).astype(np.float32)
        color = np.array([(rect['color'] >> bits) & 255 for bits in (24, 16, 8, 0)])
        if rect['gray']:
            luminance = (values[:, :, :3] * np.array([0.2126, 0.7152, 0.0722])).sum(axis=2)
            values[:, :, :3] = luminance[:, :, None] * np.array([123, 137, 154]) / 255
        values *= color / 255
        patch = Image.fromarray(np.clip(values, 0, 255).astype(np.uint8), 'RGBA')
        canvas.alpha_composite(patch, (x1, y1))
    return canvas


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--host', choices=('soh', 'mm'), default='soh')
    parser.add_argument('--sword-icon', type=Path)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    overlay = 'overlays/misc/ovl_kaleido_scope' if args.host == 'soh' else 'overlays/kaleido_scope/ovl_kaleido_scope'
    with tempfile.TemporaryDirectory(prefix='elemental-ui-preview-') as temp:
        prefix = Path(temp) / 'selector'
        binary = Path(temp) / 'render'
        options = flags(args.host)
        if args.sanitize:
            options += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
        compile_source([*options, str(ROOT / 'tests/sword_element_kaleido_render_test.c'),
                        str(ROOT / args.host / 'src' / overlay / 'sword_element_kaleido.c'),
                        '-Wl,--gc-sections', '-lm', '-o', str(binary)])
        subprocess.run([str(binary), str(prefix)], check=True,
                       env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
        canvas = render(prefix, args.sword_icon)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        canvas.crop((40 * 4, 112 * 4, 280 * 4, 230 * 4)).save(args.output)
    print(f'{args.output}: actual GBI export; native sword ' + ('provided' if args.sword_icon else 'omitted'))


if __name__ == '__main__':
    main()
