#!/usr/bin/env python3
"""Preview the shared shelf material using its exact packed room texture."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import zipfile

from PIL import ImageDraw, ImageFont
import preview

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pack', type=Path, required=True)
    parser.add_argument('--stock', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--frames', type=int, default=40)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    stock = json.loads(args.stock.read_text())['stock']
    shelves = preview.presentation.shelf_positions()
    sys.path.insert(0, str(ROOT / 'tests/black_market'))
    from check_browsing_frame import camera_views
    camera = camera_views()['right']
    font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 17)

    def label(image):
        draw = ImageDraw.Draw(image)
        draw.text((18, 12), 'Magical shelves · ethereal material POC2', font=font,
                  fill='#f3eafc', stroke_width=2, stroke_fill='#15101b')
        draw.text((18, image.height - 28), 'Offline source/assets preview · lighting approximate',
                  font=font, fill='#e2d3eb', stroke_width=2, stroke_fill='#15101b')
        return image

    with zipfile.ZipFile(args.pack) as archive:
        rows, textures = preview.streams(archive)
        keeper = preview.NativeKeeper(archive)
        placement = preview.presentation.keeper_placement(archive)
        for name in archive.namelist():
            if '/keeper/' in name and 'Texture' in name:
                import struct
                import numpy as np
                data = archive.read(name)
                _, width, height, flags, _, _, _ = struct.unpack_from('<IIIIffI', data, 64)
                assert flags & 1
                textures[name] = preview.presentation.upload(preview.gpu, np.frombuffer(data[92:], dtype=np.uint8), width, height)
        models = {item['slug']: preview.presentation.checkpoint(preview.gpu, item['slug'])[0] for item in stock}
        options = dict(keeper=keeper, placement=placement, stock=stock, shelves=shelves,
                       models=models, clears=3)
        def frame(tick=0, **kwargs):
            return preview.render(rows, textures, tick, None, **options, **kwargs)
        front = args.output / 'Black-Market-Ethereal-Shelves-POC2.png'
        label(frame(view=preview.OVERVIEW_VIEW, fov=preview.OVERVIEW_FOV)).save(front)
        bank = args.output / 'Black-Market-Ethereal-Shelves-POC2-Browsing.png'
        label(frame(view=(camera['eye'], camera['at']), fov=camera['fov'], selected=0)).save(bank)
        print('Rendered front and native browsing camera', flush=True)
        video = args.output / 'Black-Market-Ethereal-Shelves-POC2.mp4'
        if args.frames:
            process = subprocess.Popen(['ffmpeg', '-loglevel', 'error', '-y', '-f', 'rawvideo',
                '-vcodec', 'rawvideo', '-pix_fmt', 'rgb24', '-s', f'{preview.gpu.W}x{preview.gpu.H}',
                '-r', '10', '-i', '-', '-an', '-c:v', 'libx264', '-preset', 'fast', '-crf', '21',
                '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(video)], stdin=subprocess.PIPE)
            for index in range(args.frames):
                process.stdin.write(label(frame(tick=index*3, view=preview.OVERVIEW_VIEW,
                                                fov=preview.OVERVIEW_FOV)).tobytes())
            process.stdin.close()
            assert process.wait() == 0
            print('Rendered motion preview', flush=True)
        paths = ['combo/BlackMarketShelves.h', 'combo/BlackMarketShelvesNative.h',
                 'tools/black_market/shelves_preview.cpp', 'tools/black_market/presentation_preview.py',
                 'tools/black_market/preview.py', 'tools/black_market/ethereal_shelves_preview.py']
        outputs = [front, bank] + ([video] if args.frames else [])
        evidence = dict(runtime_proven=False, pack_sha256=hashlib.sha256(args.pack.read_bytes()).hexdigest(),
            source_sha256={p: hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},
            output_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in outputs},
            material=preview.presentation.shelf_fixture()['flow_texture'],
            texture='Exact existing 512x512 packed arcane_stone; no new image asset',
            notes=['Shared native geometry and material UVs; distinct frame/backing scroll.',
                   'All eight illustrative offers unlocked to expose backing material.',
                   'Native browsing camera and selected-item scale.',
                   'No gameplay capture or full executable build.'])
        (args.output/'preview-evidence.json').write_text(json.dumps(evidence, indent=2)+'\n')


if __name__ == '__main__':
    main()
