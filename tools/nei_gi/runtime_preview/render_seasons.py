"""Render exported production weather; a presentation preview, not gameplay proof."""
import ctypes as C
from pathlib import Path
import struct
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from gl_context import context, gl, ptr, integer, uint

source, destination = Path(sys.argv[1]), Path(sys.argv[2])
destination.mkdir(parents=True, exist_ok=True)
raw = source.read_bytes()
frames, tile_size = struct.unpack_from('<II', raw)
at = 8
intensity = np.frombuffer(raw, np.uint8, tile_size * tile_size, at).reshape(tile_size, tile_size)
at += tile_size * tile_size
dtype = np.dtype([('p', '<f4', 3), ('rgba', 'u1', 4), ('uv', '<f4', 2)])
samples = []
for frame in range(frames):
    row = []
    for profile in range(4):
        passes = []
        for _ in range(2):
            count, = struct.unpack_from('<I', raw, at)
            at += 4
            passes.append(np.frombuffer(raw, dtype, count, at))
            at += count * dtype.itemsize
        row.append(passes)
    samples.append(row)
assert at == len(raw) and frames == 240 and tile_size == 32

width, height = 1120, 400
context(width, height)
F, D = C.c_float, C.c_double
ClearColor = gl('glClearColor', None, F, F, F, F)
Clear = gl('glClear', None, uint)
Enable, Disable = gl('glEnable', None, uint), gl('glDisable', None, uint)
Blend = gl('glBlendFunc', None, uint, uint)
Viewport = gl('glViewport', None, integer, integer, integer, integer)
MatrixMode = gl('glMatrixMode', None, uint)
Identity = gl('glLoadIdentity', None)
Ortho = gl('glOrtho', None, D, D, D, D, D, D)
Rotate = gl('glRotatef', None, F, F, F, F)
Begin, End = gl('glBegin', None, uint), gl('glEnd', None)
Vertex, Color = gl('glVertex3f', None, F, F, F), gl('glColor4f', None, F, F, F, F)
TexCoord = gl('glTexCoord2f', None, F, F)
GenTextures = gl('glGenTextures', None, integer, C.POINTER(uint))
Bind = gl('glBindTexture', None, uint, uint)
TexParameteri = gl('glTexParameteri', None, uint, uint, integer)
TexImage = gl('glTexImage2D', None, uint, integer, integer, integer, integer, integer, uint, uint, ptr)
Read = gl('glReadPixels', None, integer, integer, integer, integer, uint, uint, ptr)
texture = uint()
GenTextures(1, C.byref(texture))
Bind(0x0DE1, texture.value)
for parameter in (0x2800, 0x2801):
    TexParameteri(0x0DE1, parameter, 0x2601)
TexParameteri(0x0DE1, 0x2802, 0x2901)  # repeat S
TexParameteri(0x0DE1, 0x2803, 0x812F)  # clamp T
pixels = np.repeat(intensity[:, :, None], 4, axis=2).copy()
TexImage(0x0DE1, 0, 0x1908, 32, 32, 0, 0x1908, 0x1401, pixels.ctypes.data)
Enable(0x0BE2)
Blend(0x0302, 0x0303)
ClearColor(.055, .072, .105, 1)
font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
font = ImageFont.truetype(font_path, 20)
small = ImageFont.truetype(font_path, 13)
names = ['Spring · Rain', 'Summer · Sunshine', 'Autumn · Leaves', 'Winter · Snow']
images = []
for frame in range(0, frames, 2):
    Clear(0x4000)
    for profile, passes in enumerate(samples[frame]):
        Viewport(profile * 280, 20, 280, 350)
        MatrixMode(0x1701)
        Identity()
        Ortho(-43, 43, -50, 50, -100, 100)
        MatrixMode(0x1700)
        Identity()
        Rotate(12, 1, 0, 0)
        Rotate(25, 0, 1, 0)
        for textured, mesh in enumerate(passes):
            (Enable if textured else Disable)(0x0DE1)
            Begin(0x0004)
            for vertex in mesh:
                Color(*(vertex['rgba'].astype(float) / 255))
                if textured:
                    TexCoord(vertex['uv'][0] - ((frame // 4) % 32) / 32, vertex['uv'][1])
                Vertex(*vertex['p'])
            End()
    output = np.empty((height, width, 4), np.uint8)
    Read(0, 0, width, height, 0x1908, 0x1401, output.ctypes.data)
    image = Image.fromarray(output[::-1].copy(), 'RGBA').convert('RGB')
    draw = ImageDraw.Draw(image)
    for profile, name in enumerate(names):
        draw.text((profile * 280 + 22, 18), name, font=font, fill='#F0F3F9')
    draw.text((22, 375), 'Production geometry preview · weather only · no rod models · not captured gameplay',
              font=small, fill='#9CA9BD')
    images.append(image)
images[23].save(destination / 'Season_GIs_Weather_Only_Preview_20261003.png')
images[0].save(destination / 'Season_GIs_Weather_Only_Preview_20261003.gif', save_all=True,
               append_images=images[1:], duration=100, loop=0, optimize=True)
video = destination / 'Season_GIs_Weather_Only_Preview_20261003.mp4'
encoder = subprocess.Popen(['ffmpeg', '-y', '-loglevel', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24',
                            '-s', f'{width}x{height}', '-r', '10', '-i', 'pipe:0', '-c:v', 'libx264',
                            '-crf', '18', '-pix_fmt', 'yuv420p', str(video)], stdin=subprocess.PIPE)
for image in images:
    encoder.stdin.write(image.tobytes())
encoder.stdin.close()
assert encoder.wait() == 0
print(f'Rendered 120 frames from the production samplers to {destination}')
