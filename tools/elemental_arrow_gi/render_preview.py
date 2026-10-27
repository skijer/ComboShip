"""Render production aura triangles and unchanged medallion pixels in Mesa.

Standard alpha blending; the arrow is a representative size guide. Actual native
and Alt arrow display lists, GI pose, occlusion and lighting require game review.
"""
import ctypes as C
from pathlib import Path
import struct
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/nei_gi/runtime_preview'))
from gl_context import context, gl, ptr, integer, uint

source, destination = Path(sys.argv[1]), Path(sys.argv[2])
destination.mkdir(parents=True, exist_ok=True)
raw = source.read_bytes()
frame_count, = struct.unpack_from('<I', raw)
at = 4
dtype = np.dtype([('p', '<f4', 3), ('rgba', 'u1', 4), ('uv', '<f4', 2)])
samples = []
for frame in range(frame_count):
    profiles = []
    for profile in range(3):
        layers = []
        for layer in range(3):
            count, = struct.unpack_from('<I', raw, at)
            at += 4
            layers.append(np.frombuffer(raw, dtype, count, at))
            at += count * dtype.itemsize
        profiles.append(layers)
    samples.append(profiles)
assert at == len(raw) and frame_count == 360

W, H = 1080, 560
context(W, H)
F, D = C.c_float, C.c_double
ClearColor = gl('glClearColor', None, F, F, F, F)
Clear = gl('glClear', None, uint)
Enable, Disable = gl('glEnable', None, uint), gl('glDisable', None, uint)
DepthMask = gl('glDepthMask', None, C.c_ubyte)
Blend = gl('glBlendFunc', None, uint, uint)
Viewport = gl('glViewport', None, integer, integer, integer, integer)
MatrixMode = gl('glMatrixMode', None, uint)
Identity = gl('glLoadIdentity', None)
Ortho = gl('glOrtho', None, D, D, D, D, D, D)
Rotate = gl('glRotatef', None, F, F, F, F)
Begin, End = gl('glBegin', None, uint), gl('glEnd', None)
Vertex, Color = gl('glVertex3f', None, F, F, F), gl('glColor4f', None, F, F, F, F)
EnableClient = gl('glEnableClientState', None, uint)
DisableClient = gl('glDisableClientState', None, uint)
VertexPointer = gl('glVertexPointer', None, integer, uint, integer, ptr)
ColorPointer = gl('glColorPointer', None, integer, uint, integer, ptr)
TexCoordPointer = gl('glTexCoordPointer', None, integer, uint, integer, ptr)
DrawArrays = gl('glDrawArrays', None, uint, integer, integer)
GenTextures = gl('glGenTextures', None, integer, C.POINTER(uint))
Bind = gl('glBindTexture', None, uint, uint)
TexParameteri = gl('glTexParameteri', None, uint, uint, integer)
TexImage = gl('glTexImage2D', None, uint, integer, integer, integer, integer, integer, uint, uint, ptr)
Read = gl('glReadPixels', None, integer, integer, integer, integer, uint, uint, ptr)

textures = []
for slug in ('fire', 'ice', 'light'):
    resource = (ROOT / 'soh/assets/custom/objects/nei_elemental_arrow_gi' / slug).read_bytes()
    kind, width, height, flags, hs, vs, size = struct.unpack_from('<4I2fI', resource, 64)
    assert kind == 1 and flags == 3 and size == width * height * 4
    pixels = np.frombuffer(resource, np.uint8, size, 92)
    texture = uint()
    GenTextures(1, C.byref(texture))
    Bind(0x0DE1, texture.value)
    for parameter in (0x2800, 0x2801):
        TexParameteri(0x0DE1, parameter, 0x2601)
    for parameter in (0x2802, 0x2803):
        TexParameteri(0x0DE1, parameter, 0x812F)
    TexImage(0x0DE1, 0, 0x1908, width, height, 0, 0x1908, 0x1401, pixels.ctypes.data)
    textures.append(texture.value)
Enable(0x0BE2)
Blend(0x0302, 0x0303)
ClearColor(.040, .052, .075, 1)
font = lambda size: ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', size)


def arrow_guide():
    """Neutral representative core; it is never installed as a game asset."""
    Disable(0x0DE1)
    Enable(0x0B71)
    DepthMask(1)
    Begin(0x0004)
    def tri(a, b, c, color):
        Color(*color, 1)
        for point in (a, b, c): Vertex(*point)
    for i in range(6):
        a, b = i * np.pi / 3, (i + 1) * np.pi / 3
        lo = (.65 * np.cos(a), -23, .65 * np.sin(a))
        nxt = (.65 * np.cos(b), -23, .65 * np.sin(b))
        hi = (lo[0], 9, lo[2]); nxt_hi = (nxt[0], 9, nxt[2])
        shade = .35 + .055 * (i % 3)
        tri(lo, nxt, nxt_hi, (shade * .9, shade, shade * 1.1))
        tri(lo, nxt_hi, hi, (shade * .9, shade, shade * 1.1))
        tri((2.4 * np.cos(a), 8, 2.4 * np.sin(a)), (2.4 * np.cos(b), 8, 2.4 * np.sin(b)),
            (0, 16, 0), (.57 + .07 * (i % 3), .62 + .07 * (i % 3), .70 + .06 * (i % 3)))
    for angle in (0, 2 * np.pi / 3, 4 * np.pi / 3):
        p = (3.2 * np.cos(angle), -22, 3.2 * np.sin(angle))
        q = (2.6 * np.cos(angle), -16, 2.6 * np.sin(angle))
        tri((0, -24, 0), p, q, (.34, .38, .43))
        tri((0, -24, 0), q, (0, -14, 0), (.45, .49, .55))
    End()
    DepthMask(0)


def draw(frame):
    DepthMask(1)
    Clear(0x4000 | 0x100)
    for profile, layers in enumerate(samples[frame]):
        Viewport(profile * 360, 74, 360, 380)
        MatrixMode(0x1701); Identity()
        Ortho(-33.6, 33.6, -32, 39, -150, 150)
        MatrixMode(0x1700); Identity()
        Rotate(12, 1, 0, 0); Rotate(25, 0, 1, 0)
        arrow_guide()
        for layer, mesh in enumerate(layers):
            if layer == 0:
                Enable(0x0DE1); Bind(0x0DE1, textures[profile])
                EnableClient(0x8078)
                TexCoordPointer(2, 0x1406, 24, mesh.ctypes.data + 16)
            else:
                Disable(0x0DE1); DisableClient(0x8078)
            EnableClient(0x8074); EnableClient(0x8076)
            VertexPointer(3, 0x1406, 24, mesh.ctypes.data)
            ColorPointer(4, 0x1401, 24, mesh.ctypes.data + 12)
            DrawArrays(0x0004, 0, len(mesh))
        DisableClient(0x8074); DisableClient(0x8076); DisableClient(0x8078)
    pixels = np.empty((H, W, 4), np.uint8)
    Read(0, 0, W, H, 0x1908, 0x1401, pixels.ctypes.data)
    image = Image.fromarray(pixels[::-1].copy()).convert('RGB')
    d = ImageDraw.Draw(image)
    d.text((24, 16), 'ELEMENTAL ARROWS  /  GI POC 1', font=font(25), fill='#EDF1F8')
    for i, (name, note) in enumerate((('FIRE', 'Curling flames + rising embers'),
                                     ('ICE', 'Frost veils + tumbling crystals'),
                                     ('LIGHT', 'Rotating halos + white-gold rays'))):
        d.text((i * 360 + 24, 68), name, font=font(20), fill='#EDF1F8')
        d.text((i * 360 + 24, 490), note, font=font(14), fill='#B5C1D1')
    d.text((24, 531), 'Production aura animation + medallion artwork · representative arrow core · in-game review pending',
           font=font(14), fill='#93A5BC')
    return image


poster = draw(47)
poster.save(destination / 'Elemental_Arrow_GI_POC1.png')
if '--still' not in sys.argv:
    movie = destination / 'Elemental_Arrow_GI_POC1.mp4'
    encoder = subprocess.Popen(['ffmpeg', '-y', '-loglevel', 'error', '-f', 'rawvideo', '-pix_fmt', 'rgb24',
                                '-s', f'{W}x{H}', '-r', '20', '-i', 'pipe:0', '-c:v', 'libx264',
                                '-crf', '18', '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(movie)],
                               stdin=subprocess.PIPE)
    for frame in range(frame_count): encoder.stdin.write(draw(frame).tobytes())
    encoder.stdin.close()
    assert encoder.wait() == 0
    gif = destination / 'Elemental_Arrow_GI_POC1.gif'
    subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', str(movie),
                    '-filter_complex', '[0:v]fps=10,scale=900:-1:flags=lanczos,split[a][b];'
                    '[a]palettegen=stats_mode=diff[p];[b][p]paletteuse=dither=sierra2_4a', str(gif)], check=True)
print('Rendered production aura preview to', destination)
