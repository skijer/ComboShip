#!/usr/bin/env python3
"""Compare exact Mirror cast meshes/materials with standard alpha; no bloom."""
import argparse
import ctypes as C
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools/nei_used_fx'))
import render as native

DTYPE = np.dtype([('p', '<f4', 3), ('rgba', 'u1', 4), ('uv', '<f4', 2)])
original_texture = native.texture
textures = {}


def texture(material):
    if material < 11:
        return original_texture(material)
    if material not in textures:
        name = 'forest_seal' if material == 11 else 'forest_wisp'
        raw = (ROOT/'soh/assets/custom/objects/magic_mirror'/name).read_bytes()
        width, height = struct.unpack_from('<II', raw, 68)
        pixels = np.frombuffer(raw, np.uint8, offset=92)
        handle = native.uint()
        native.GenTextures(1, C.byref(handle))
        native.BindTexture(0x0DE1, handle.value)
        for parameter in (0x2800, 0x2801):
            native.TexParameteri(0x0DE1, parameter, 0x2601)
        native.TexParameteri(0x0DE1, 0x2802, 0x2901 if material == 12 else 0x812F)
        native.TexParameteri(0x0DE1, 0x2803, 0x812F)
        native.TexImage(0x0DE1, 0, 0x1908, width, height, 0, 0x1908, 0x1401, pixels.ctypes.data)
        textures[material] = handle.value
    return textures[material]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='mirror-cast-preview-') as directory:
        executable = Path(directory)/'export'
        samples = Path(directory)/'samples.bin'
        subprocess.run(['c++', '-std=c++17', '-O2', str(Path(__file__).with_name('export_cast.cpp')),
                        '-o', str(executable)], check=True)
        subprocess.run([str(executable), str(samples)], check=True)
        raw = samples.read_bytes()
    native.texture = texture
    native.Clear(0x00004000 | 0x00000100)
    at = 0
    for panel in range(2):
        native.Viewport(panel*750, 90, 750, 790)
        native.MatrixMode(0x1701)
        native.LoadIdentity()
        native.Ortho(-105, 105, -34, 140, -400, 400)
        native.MatrixMode(0x1700)
        native.LoadIdentity()
        native.Rotate(24, 1, 0, 0)
        native.Rotate(-18, 0, 1, 0)
        native.Disable(0x0B71)
        native.Color(.08, .1, .13, 1)
        native.Begin(0x0001)
        for axis in range(-100, 101, 20):
            native.Vertex(-100, -2, axis)
            native.Vertex(100, -2, axis)
            native.Vertex(axis, -2, -100)
            native.Vertex(axis, -2, 100)
        native.End()
        layers, = struct.unpack_from('<I', raw, at)
        at += 4
        for _ in range(layers):
            material, count = struct.unpack_from('<II', raw, at)
            at += 8
            vertices = np.frombuffer(raw, DTYPE, count, at)
            at += count*24
            native.drawmesh(vertices, material)
    assert at == len(raw)
    pixels = np.zeros((native.H, native.W, 4), np.uint8)
    native.ReadPixels(0, 0, native.W, native.H, 0x1908, 0x1401, pixels.ctypes.data)
    image = Image.fromarray(pixels[::-1].copy()).convert('RGB')
    draw = ImageDraw.Draw(image)
    draw.text((32, 24), 'MAGIC MIRROR / FOREST CAST TEXTURE PROBE', font=native.font(30), fill='#e4f5e8')
    draw.text((32, 70), 'Exact production meshes and native textures; standard alpha; offline asset preview',
              font=native.font(19), fill='#a5b6ad')
    draw.text((32, 125), 'Previous: green ritual + shared wisp', font=native.font(23), fill='#bdc9c1')
    draw.text((782, 125), 'Probe: botanical seal + forest wisps', font=native.font(23), fill='#a4edb7')
    draw.line((750, 120, 750, 930), fill='#2d3d34', width=2)
    draw.text((32, 950), 'Farore motion, existing green ritual and cast timing retained. New artwork is Mirror-only.',
              font=native.font(20), fill='#a5b6ad')
    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output)
    print(args.output)


if __name__ == '__main__':
    main()
