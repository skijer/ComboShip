"""Review exact production song triangles using standard alpha blending.

python tools/nei_gi/runtime_preview/render_songs.py --before-ref c10fd9a5 \
    --out /tmp/song-effects-review.png --gif /tmp/song-effects-review.gif

The native music note and shared shimmer are omitted. This is an effects-only
offline review, not a captured game framebuffer or a claim of runtime approval.
"""
import argparse
import ctypes as C
import os
from pathlib import Path
import struct
import subprocess
import tempfile

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from gl_context import context, gl, integer, ptr, uint

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
POLICY = 'soh/soh/Enhancements/randomizer/NeiGiSongEffectPolicy.h'
TILE_W, TILE_H = 380, 300
context(TILE_W, TILE_H)
F, D = C.c_float, C.c_double
clear_color = gl('glClearColor', None, F, F, F, F)
clear = gl('glClear', None, uint)
enable = gl('glEnable', None, uint)
blend_func = gl('glBlendFunc', None, uint, uint)
viewport = gl('glViewport', None, integer, integer, integer, integer)
matrix_mode = gl('glMatrixMode', None, uint)
load_identity = gl('glLoadIdentity', None)
ortho = gl('glOrtho', None, D, D, D, D, D, D)
rotate = gl('glRotatef', None, F, F, F, F)
enable_client = gl('glEnableClientState', None, uint)
disable_client = gl('glDisableClientState', None, uint)
vertex_pointer = gl('glVertexPointer', None, integer, uint, integer, ptr)
color_pointer = gl('glColorPointer', None, integer, uint, integer, ptr)
draw_arrays = gl('glDrawArrays', None, uint, integer, integer)
read_pixels = gl('glReadPixels', None, integer, integer, integer, integer, uint, uint, ptr)
enable(0x0BE2)
blend_func(0x0302, 0x0303)
clear_color(.035, .045, .06, 1)
font = lambda size: ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', size)


def export(temp, name, before_ref=None):
    include = ROOT / POLICY
    if before_ref:
        include = temp / name / 'NeiGiSongEffectPolicy.h'
        include.parent.mkdir()
        include.write_bytes(subprocess.check_output(['git', 'show', before_ref + ':' + POLICY], cwd=ROOT))
    binary, output = temp / (name + '-export'), temp / (name + '.bin')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-O2',
                    '-I' + str(include.parent), '-I' + str((ROOT / POLICY).parent),
                    '-I' + str(ROOT / 'combo/menu'), str(HERE / 'export_songs.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary), str(output)], check=True)
    packed = output.read_bytes()
    frames, offset = [], 0
    for _ in range(180):
        songs = []
        for _ in range(2):
            count = struct.unpack_from('<I', packed, offset)[0]
            offset += 4
            songs.append(np.frombuffer(packed[offset:offset + 16 * count], dtype=np.uint8).copy())
            offset += 16 * count
        frames.append(songs)
    assert offset == len(packed)
    return frames


def render(data):
    clear(0x4000)
    viewport(0, 0, TILE_W, TILE_H)
    matrix_mode(0x1701)
    load_identity()
    ortho(-44, 44, -34, 36, -200, 200)
    matrix_mode(0x1700)
    load_identity()
    rotate(12, 1, 0, 0)
    rotate(25, 0, 1, 0)
    if len(data):
        enable_client(0x8074)
        enable_client(0x8076)
        vertex_pointer(3, 0x1406, 16, data.ctypes.data)
        color_pointer(4, 0x1401, 16, data.ctypes.data + 12)
        draw_arrays(0x0004, 0, len(data) // 16)
        disable_client(0x8074)
        disable_client(0x8076)
    pixels = np.zeros((TILE_H, TILE_W, 4), dtype=np.uint8)
    read_pixels(0, 0, TILE_W, TILE_H, 0x1908, 0x1401, pixels.ctypes.data)
    image = Image.fromarray(pixels[::-1].copy()).convert('RGB')
    if not len(data):
        ImageDraw.Draw(image).text((62, 140), 'Native note + shimmer only', fill='#7f96a8', font=font(17))
    return image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before-ref', default='c10fd9a5')
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--gif', type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='song-effects-review-') as tmp:
        temp = Path(tmp)
        before, after = export(temp, 'before', args.before_ref), export(temp, 'after')
    header, labels = 90, 30
    sheet = Image.new('RGB', (TILE_W * 4, header + (TILE_H + labels) * 4), '#131d27')
    draw = ImageDraw.Draw(sheet)
    draw.text((18, 12), 'Bolero and Serenade · production effect triangles', font=font(25), fill='#eef4fa')
    draw.text((18, 50), 'Effects only · native note and shared shimmer omitted · standard alpha · offline Mesa',
              font=font(17), fill='#a7bccd')
    for row, (title, version, song) in enumerate([
        ('Bolero before', before, 0), ('Bolero after', after, 0),
        ('Serenade before', before, 1), ('Serenade after', after, 1)]):
        for column, frame in enumerate([0, 42, 89, 179]):
            x, y = column * TILE_W, header + row * (TILE_H + labels)
            draw.text((x + 12, y + 5), f'{title} · frame {frame}', font=font(17), fill='#edf4fb')
            sheet.paste(render(version[frame][song]), (x, y + labels))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(args.out)
    if args.gif:
        animation = []
        for frame in range(0, 180, 3):
            image = Image.new('RGB', (TILE_W * 2, TILE_H + labels), '#131d27')
            for column, (title, version) in enumerate([('Bolero before', before), ('Bolero after', after)]):
                image.paste(render(version[frame][0]), (column * TILE_W, labels))
                ImageDraw.Draw(image).text((column * TILE_W + 12, 5), title, font=font(17), fill='#edf4fb')
            animation.append(image)
        args.gif.parent.mkdir(parents=True, exist_ok=True)
        animation[0].save(args.gif, save_all=True, append_images=animation[1:], duration=100, loop=0)
    print('Exported production before/after song effects:', args.out)


if __name__ == '__main__':
    main()
