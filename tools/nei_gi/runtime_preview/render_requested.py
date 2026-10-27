"""Turntables of the exact new Cojiro/Mario GI checkpoints, using the existing Mesa renderer."""
import argparse
from pathlib import Path

from PIL import Image, ImageDraw

import render as r
from render_slate import basis


def frame(yaw):
    r.R = basis(12, yaw)
    models = [r.model(slug) for slug in ('cojiro', 'mario_mask')]
    r.DepthMask(1)
    r.Clear(0x4000 | 0x0100)
    for index, (opaque, translucent) in enumerate(models):
        r.Viewport(index*600, 95, 600, 780)
        r.MatrixMode(0x1701)
        r.LoadIdentity()
        r.Ortho(-30, 30, -33, 45, -300, 300)
        r.MatrixMode(0x1700)
        r.LoadIdentity()
        r.Rotate(12, 1, 0, 0)
        r.Rotate(yaw, 0, 1, 0)
        r.DepthMask(1)
        r.CallList(opaque)
        r.DepthMask(0)
        r.CallList(translucent)
    im = r.pixels()
    draw = ImageDraw.Draw(im)
    draw.text((35, 24), 'Cojiro', font=r.font(30), fill='#eff6ff')
    draw.text((635, 24), 'Mario Mask', font=r.font(30), fill='#eff6ff')
    draw.text((35, 75), 'New GI candidates • exact exported geometry', font=r.font(20), fill='#afbed0')
    draw.text((35, 939), 'Offline model review • native actors, held items and player models are separate',
              font=r.font(20), fill='#afbed0')
    return im


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--frames', type=int, default=48)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    frames = [frame(index*360/args.frames) for index in range(args.frames)]
    frames[0].save(args.output/'cojiro-mario-front.png')
    for name, fraction in [('side', .25), ('back', .5), ('other-side', .75)]:
        frames[round(fraction*args.frames) % args.frames].save(args.output/f'cojiro-mario-{name}.png')
    frames[0].save(args.output/'cojiro-mario-turntable.gif', save_all=True,
                   append_images=frames[1:], duration=90, loop=0, disposal=2)
    sheet = Image.new('RGB', (1200, 1000), '#111923')
    for index, sample in enumerate([0, args.frames//4, args.frames//2, 3*args.frames//4]):
        sheet.paste(frames[sample].resize((600, 500), Image.Resampling.LANCZOS),
                    ((index%2)*600, (index//2)*500))
    sheet.save(args.output/'cojiro-mario-views.png')
    print(f'Rendered {len(frames)} frames and four views of the exact checkpoint GLBs to {args.output}')


if __name__ == '__main__':
    main()
