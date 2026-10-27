#!/usr/bin/env python3
"""Sample the addon's actual portal mesh, two textures, UVs and scroll pipeline.

Offline orthographic render only: no scene camera, bloom or invented particles.
The material is deliberately rejected if its combiner differs from this supported
POC3 pipeline. Texture dimensions, tile offsets/shifts and scroll speeds are read
from the archive, never replaced with a decorative preview animation.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import subprocess
import xml.etree.ElementTree as ET
import zipfile

import numpy as np
from PIL import Image, ImageDraw, ImageFont

THEMES = (('Forest', (105, 255, 145)), ('Fire', (255, 105, 40)), ('Water', (90, 180, 255)),
          ('Spirit', (255, 195, 85)), ('Shadow', (190, 105, 255)))
BACKGROUND = (10, 14, 23)


def font(size):
    path = Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
    return ImageFont.truetype(str(path), size) if path.exists() else ImageFont.load_default()


def mesh(path, route='forest'):
    base = 'custom/temple_sages/portals/' + route + '/'
    with zipfile.ZipFile(path) as archive:
        data = archive.read(base + 'geometry_vtx')
        material = ET.fromstring(archive.read(base + 'material'))
        combine = material.find('SetCombineLERP').attrib
        expected = dict(A0='G_CCMUX_TEXEL1', B0='G_CCMUX_TEXEL0', C0='G_CCMUX_ENV_ALPHA', D0='G_CCMUX_TEXEL0',
                        Aa0='G_ACMUX_TEXEL0', Ab0='G_ACMUX_0', Ac0='G_ACMUX_SHADE', Ad0='G_ACMUX_0',
                        A1='G_CCMUX_COMBINED', B1='G_CCMUX_0', C1='G_CCMUX_PRIMITIVE', D1='G_CCMUX_0',
                        Aa1='G_ACMUX_COMBINED', Ab1='G_ACMUX_0', Ac1='G_ACMUX_PRIMITIVE', Ad1='G_ACMUX_0')
        if combine != expected:
            raise ValueError('preview only supports the actual POC3 two-layer combiner')
        model = dict(rows=np.array([[int(v.get(a)) for a in ('X', 'Y', 'Z', 'S', 'T', 'R', 'G', 'B', 'A')]
                                    for v in ET.fromstring(data)], dtype=float), layers=[], raster={})
        images = material.findall('SetTextureImage')
        tiles = [e for e in material if e.tag == 'SetTile' and e.get('Tile') in ('0', '1')]
        sizes = material.findall('SetTileSize')
        scrolls = material.findall('ScrollTexture')
        if len(images) != 2 or len(tiles) != 2 or len(scrolls) != 2:
            raise ValueError('preview requires two explicit owned texture layers')
        for image, tile, size, scroll in zip(images, tiles, sizes, scrolls):
            raw = archive.read(image.get('Path'))
            kind, w, h, flags, sx, sy, count = struct.unpack_from('<IIIIffI', raw, 64)
            if kind != 1 or flags != 1 or count != w*h*4 or len(raw) != 92 + count:
                raise ValueError('preview requires the native RGBA RAW1 replacement resource')
            model['layers'].append(dict(pixels=np.frombuffer(raw[92:], dtype=np.uint8).reshape(h, w, 4)/255,
                shift=np.array([int(tile.get('ShiftS')), int(tile.get('ShiftT'))]),
                origin=np.array([int(size.get('Uls')), int(size.get('Ult'))]),
                logical=np.array([round(w*2/sx), round(h/sy)]),
                speed=np.array([int(scroll.get('SpeedX')), int(scroll.get('SpeedY'))])))
        model['env_alpha'] = int(material.find('SetEnvColor').get('A')) / 255
    if len(model['rows']) % 3:
        raise ValueError('authored triangle rows are incomplete')
    return model


def _raster(model, center, scale, dimensions):
    key = (center, scale, dimensions)
    if key in model['raster']:
        return model['raster'][key]
    rows = model['rows']
    yaw, pitch = math.radians(18), math.radians(12)
    depth = -math.sin(yaw)*rows[:, 0] + math.cos(yaw)*rows[:, 2]
    points = np.column_stack((center[0] + scale*(math.cos(yaw)*rows[:, 0] + math.sin(yaw)*rows[:, 2]),
        center[1] - scale*(math.cos(pitch)*(rows[:, 1]-70) - math.sin(pitch)*depth)))
    triangles = []
    for offset in range(0, len(rows), 3):
        a, b, c = points[offset:offset+3]
        den = (b[1]-c[1])*(a[0]-c[0]) + (c[0]-b[0])*(a[1]-c[1])
        if abs(den) < 1e-7:
            continue
        low = np.maximum(np.floor(np.min((a, b, c), axis=0)).astype(int), 0)
        high = np.minimum(np.ceil(np.max((a, b, c), axis=0)).astype(int)+1, dimensions)
        if np.any(high <= low):
            continue
        yy, xx = np.mgrid[low[1]:high[1], low[0]:high[0]]
        x, y = xx+.5, yy+.5
        u = ((b[1]-c[1])*(x-c[0]) + (c[0]-b[0])*(y-c[1])) / den
        v = ((c[1]-a[1])*(x-c[0]) + (a[0]-c[0])*(y-c[1])) / den
        w = 1-u-v
        mask = (u >= 0) & (v >= 0) & (w >= 0)
        if mask.any():
            weights = np.stack((u[mask], v[mask], w[mask]), axis=1)
            attributes = weights @ rows[offset:offset+3, 3:]
            triangles.append((yy[mask], xx[mask], attributes))
    model['raster'][key] = triangles
    return triangles


def sample(layer, uv, tick):
    """Native S10.5 UV, tile shift, quarter-texel scroll, bilinear repeat.

    Mirrors interpreter.cpp's shift before subtracting uls/4, then +0.5
    bilinear texel-center adjustment. Raw replacement scales preserve logical UV.
    """
    shifts = layer['shift']
    factor = np.where(shifts <= 10, 2.**(-shifts), 2.**(16-shifts))
    logical_uv = uv / 32 * factor - (layer['origin'] + tick*layer['speed']) / 4 + .5
    pixels = layer['pixels']
    h, w = pixels.shape[:2]
    xy = logical_uv / layer['logical'] * np.array([w, h]) - .5
    lo = np.floor(xy).astype(int)
    fraction = xy-lo
    x, y = lo[:, 0] % w, lo[:, 1] % h
    x1, y1 = (x+1) % w, (y+1) % h
    fx, fy = fraction[:, 0, None], fraction[:, 1, None]
    return ((pixels[y, x]*(1-fx) + pixels[y, x1]*fx)*(1-fy)
            + (pixels[y1, x]*(1-fx) + pixels[y1, x1]*fx)*fy)


def portal(image, model, color, alpha, center, scale, tick=0):
    canvas = np.asarray(image, dtype=float).copy()
    for yy, xx, attributes in _raster(model, center, scale, image.size):
        flow, detail = [sample(layer, attributes[:, :2], tick) for layer in model['layers']]
        shade_alpha = attributes[:, 5:6] / 255
        rgb = (flow[:, :3]*(1-model['env_alpha']) + detail[:, :3]*model['env_alpha']) * np.array(color)
        opacity = flow[:, 3:4] * shade_alpha * alpha/255
        canvas[yy, xx] = canvas[yy, xx]*(1-opacity) + rgb*opacity
    image.paste(Image.fromarray(np.clip(canvas, 0, 255).astype('uint8')))


def opacity(earned, frame):
    period = 240 if earned else 480
    wave = math.sin((frame % period)*math.tau/period)
    return int(155+35*wave) if earned else int(52+10*wave)


def comparison(models):
    image = Image.new('RGB', (1460, 820), BACKGROUND)
    draw = ImageDraw.Draw(image)
    draw.text((38, 26), 'Scrolling portal membranes', font=font(29), fill='#edf3ff')
    draw.text((38, 69), 'Offline source-mesh render; actual two-layer textures / UVs / native scroll. No added bloom.',
              font=font(17), fill='#a8b5cb')
    for col, (name, color) in enumerate(THEMES):
        draw.text((280+col*245, 120), name, font=font(19), fill=color, anchor='mm')
    for earned, y in ((False, 290), (True, 610)):
        draw = ImageDraw.Draw(image)
        draw.text((32, y-18), 'EARNED' if earned else 'LOCKED', font=font(19), fill='#dbe6f7')
        draw.text((32, y+13), 'Travel enabled' if earned else 'Visible, inactive', font=font(14), fill='#94a6bf')
        for col, (_, color) in enumerate(THEMES):
            portal(image, models[col], color, opacity(earned, 60), (280+col*245, y), 1.62, 60)
    draw = ImageDraw.Draw(image)
    draw.line((32, 455, 1428, 455), fill='#243046', width=1)
    draw.text((38, 783), 'Locked: faint slow pulse.  Earned: brighter pulse; matching medallion permits travel.',
              font=font(17), fill='#b6c6dc')
    return image


def video_frame(model, tick):
    image = Image.new('RGB', (960, 640), BACKGROUND)
    draw = ImageDraw.Draw(image)
    draw.text((26, 18), 'Forest portal / actual two-layer membrane flow', font=font(24), fill='#e8f1ff')
    draw.text((26, 58), 'Offline source-mesh preview. Same art, UVs, native scroll and opacity; no bloom.',
              font=font(16), fill='#9cafc8')
    for earned, x in ((False, 260), (True, 700)):
        portal(image, model, THEMES[0][1], opacity(earned, tick), (x, 340), 2.8, tick)
    draw = ImageDraw.Draw(image)
    draw.text((260, 586), 'Locked / visible, inactive', font=font(20), fill='#afc1d8', anchor='mm')
    draw.text((700, 586), 'Earned / travel enabled', font=font(20), fill='#d5eadf', anchor='mm')
    return image


def motion(model, path, seconds=10):
    path.parent.mkdir(parents=True, exist_ok=True)
    command = ['ffmpeg', '-loglevel', 'error', '-y', '-f', 'rawvideo', '-vcodec', 'rawvideo',
               '-pix_fmt', 'rgb24', '-s', '960x640', '-r', '10', '-i', '-', '-an',
               '-vf', 'fps=30', '-c:v', 'libx264', '-preset', 'fast', '-crf', '18',
               '-pix_fmt', 'yuv420p', '-movflags', '+faststart', str(path)]
    encoder = subprocess.Popen(command, stdin=subprocess.PIPE)
    try:
        # OOT native update tick is 20 Hz; input frames sample it at 10 fps.
        for i in range(seconds*10):
            encoder.stdin.write(video_frame(model, i*2).tobytes())
        encoder.stdin.close()
        if encoder.wait():
            raise RuntimeError('ffmpeg failed')
    finally:
        if encoder.poll() is None:
            encoder.kill()
            encoder.wait()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--addon', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--mp4', type=Path)
    args = parser.parse_args()
    models = [mesh(args.addon, name.lower()) for name, _ in THEMES]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    comparison(models).save(args.output)
    if args.mp4:
        motion(models[0], args.mp4)
    print(json.dumps(dict(mesh_triangles=len(models[0]['rows'])//3,
        addon_sha256=hashlib.sha256(args.addon.read_bytes()).hexdigest(), png=str(args.output),
        mp4=str(args.mp4) if args.mp4 else None, in_game_capture=False, layers=2,
        game_tick_hz=20, input_fps=10, output_fps=30)))


if __name__ == '__main__':
    main()
