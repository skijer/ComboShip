"""Render exported production triangles with native alpha blending; no added glow.

Portal outlines and Saria's silhouette are schematic guides, not game captures.
"""
import argparse
import ctypes as C
import math
from pathlib import Path
import struct
import subprocess

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from gl_context import context, gl, ptr, integer, uint

W, H = 1440, 1040
F, D = C.c_float, C.c_double
context(W, H)
ClearColor = gl('glClearColor', None, F, F, F, F)
Clear = gl('glClear', None, uint)
Enable = gl('glEnable', None, uint)
Disable = gl('glDisable', None, uint)
Viewport = gl('glViewport', None, integer, integer, integer, integer)
MatrixMode = gl('glMatrixMode', None, uint)
LoadIdentity = gl('glLoadIdentity', None)
Ortho = gl('glOrtho', None, D, D, D, D, D, D)
Begin = gl('glBegin', None, uint)
End = gl('glEnd', None)
Vertex = gl('glVertex3f', None, F, F, F)
Color = gl('glColor4f', None, F, F, F, F)
BlendFunc = gl('glBlendFunc', None, uint, uint)
DepthMask = gl('glDepthMask', None, C.c_ubyte)
ReadPixels = gl('glReadPixels', None, integer, integer, integer, integer, uint, uint, ptr)
EnableClient = gl('glEnableClientState', None, uint)
DisableClient = gl('glDisableClientState', None, uint)
VertexPointer = gl('glVertexPointer', None, integer, uint, integer, ptr)
ColorPointer = gl('glColorPointer', None, integer, uint, integer, ptr)
DrawArrays = gl('glDrawArrays', None, uint, integer, integer)
Enable(0x0BE2)
BlendFunc(0x0302, 0x0303)
ClearColor(.045, .065, .075, 1)
FONT = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'


def read_frames(path):
    raw = path.read_bytes()
    at, frames = 0, []
    dtype = np.dtype([('p', '<f4', 3), ('rgba', 'u1', 4)])
    for _ in range(180):
        row = []
        for _ in range(6):
            count = struct.unpack_from('<I', raw, at)[0]
            at += 4
            row.append(np.frombuffer(raw, dtype=dtype, count=count, offset=at))
            at += count * 16
        frames.append(row)
    assert at == len(raw)
    return frames


def pose(x, y, w, h, low, high):
    Viewport(x, y, w, h)
    MatrixMode(0x1701)
    LoadIdentity()
    half = (high - low) * w / h / 2
    Ortho(-half, half, low, high, -300, 300)
    MatrixMode(0x1700)
    LoadIdentity()


def draw(mesh):
    Disable(0x0B44)
    Disable(0x0DE1)
    EnableClient(0x8074)
    EnableClient(0x8076)
    VertexPointer(3, 0x1406, 16, mesh.ctypes.data)
    ColorPointer(4, 0x1401, 16, mesh.ctypes.data + 12)
    DrawArrays(0x0004, 0, len(mesh))
    DisableClient(0x8074)
    DisableClient(0x8076)


def polygon(points, color):
    Color(*color)
    Begin(0x0009)
    for x, y in points:
        Vertex(x, y, -25)
    End()


def doorway(radius, center):
    # Only a neutral boundary guide. The actual scrolling texture is not mocked.
    points = [(-radius, 0), (radius, 0)]
    points += [(radius * math.cos(i * math.pi / 48), center + radius * math.sin(i * math.pi / 48))
               for i in range(49)]
    polygon(points, (.20, .24, .23, 1))
    r = radius - 1.5
    polygon([(-r, 1.5), (r, 1.5)] + [(r * math.cos(i * math.pi / 48), center + r * math.sin(i * math.pi / 48))
                                       for i in range(49)], (.003, .004, .006, 1))


def label(image, frame):
    d = ImageDraw.Draw(image)
    font = lambda size: ImageFont.truetype(FONT, size)
    d.rectangle((0, 0, W, 90), fill='#101c22')
    d.text((30, 16), 'NEI twinkle • production-code preview', font=font(30), fill='#edf7f0')
    d.text((30, 58), 'Exact triangles, pulse, spin, NEI palettes and alpha. Standard blending; no bloom.',
           font=font(18), fill='#b1c9bc')
    for x, y, title, note in [
        (30, 108, 'Original NEI shimmer', 'Reference sampler, unchanged'),
        (745, 108, 'Both portal entrances', 'Twinkles across the face of each black opening'),
        (30, 565, 'Saria emitter', 'Same NEI twinkles around the actor; silhouette is a guide'),
        (745, 565, 'Uncollected Magic Mirror', 'Pale blue and white NEI twinkles match its glass and casing'),
    ]:
        d.text((x, y), title, font=font(25), fill='#edf7f0')
        d.text((x, y + 37), note, font=font(16), fill='#a6c0b2')
    d.line((720, 100, 720, 983), fill='#294039', width=2)
    d.text((845, 183), 'Lost Woods', font=font(18), fill='#b1c9bc')
    d.text((1150, 183), 'Woods of Mystery', font=font(18), fill='#b1c9bc')
    d.line((20, 548, 1420, 548), fill='#294039', width=2)
    d.rectangle((0, 992, W, H), fill='#101c22')
    d.text((30, 1006), 'Offline effect preview. Portal borders / actor guide are schematic; this is not captured gameplay.',
           font=font(18), fill='#b1c9bc')
    return image


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('triangles', type=Path)
    p.add_argument('output', type=Path)
    a = p.parse_args()
    a.output.mkdir(parents=True, exist_ok=True)
    frames = read_frames(a.triangles)
    movie = a.output / 'ComboShip-NEI-Twinkle-Preview-20261021.mp4'
    proc = subprocess.Popen(['ffmpeg', '-loglevel', 'error', '-y', '-f', 'rawvideo', '-pix_fmt', 'rgb24',
                             '-s', f'{W}x{H}', '-r', '20', '-i', '-', '-an', '-c:v', 'libx264',
                             '-preset', 'fast', '-crf', '18', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(movie)],
                            stdin=subprocess.PIPE)
    for f, row in enumerate(frames):
        DepthMask(1)
        Clear(0x4000 | 0x0100)
        Disable(0x0B71)
        pose(35, 495, 650, 360, -36, 36)
        draw(row[0])
        for x, index, radius, center in [(755, 1, 68, 66), (1090, 2, 60, 66)]:
            pose(x, 495, 315, 360, -8, 146)
            doorway(radius, center)
            draw(row[index])
        pose(35, 55, 650, 360, -7, 64)
        polygon([(-5, 0), (-7, 25), (-12, 21), (-13, 24), (-7, 30), (-5, 36), (5, 36),
                 (7, 30), (13, 24), (12, 21), (7, 25), (5, 0)], (.13, .19, .15, 1))
        polygon([(7 * math.cos(i * math.tau / 32), 43 + 7 * math.sin(i * math.tau / 32))
                 for i in range(32)], (.13, .19, .15, 1))
        draw(row[3])
        pose(750, 55, 650, 360, -30, 32)
        Enable(0x0B71)
        DepthMask(1)
        draw(row[5])
        DepthMask(0)
        draw(row[4])
        pixels = np.zeros((H, W, 4), np.uint8)
        ReadPixels(0, 0, W, H, 0x1908, 0x1401, pixels.ctypes.data)
        image = label(Image.fromarray(pixels[::-1].copy()).convert('RGB'), f)
        if f == 36:
            image.save(a.output / 'ComboShip-NEI-Twinkle-Preview-20261021.png')
        proc.stdin.write(image.tobytes())
    proc.stdin.close()
    assert proc.wait() == 0
    gif = a.output / 'ComboShip-NEI-Twinkle-Preview-20261021.gif'
    subprocess.run(['ffmpeg', '-loglevel', 'error', '-y', '-i', str(movie), '-filter_complex',
                    '[0:v]fps=15,scale=1080:-1:flags=lanczos,split[a][b];[a]palettegen=max_colors=192[p];[b][p]paletteuse', str(gif)], check=True)
    print(movie)
    print(gif)


if __name__ == '__main__':
    main()
