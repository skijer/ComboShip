#!/usr/bin/env python3
"""Offline native-contour HD restoration; no substitute font or generated glyphs.

Pillow, NumPy and potracer are authoring dependencies, never build dependencies.
"""
import argparse
import base64
import hashlib
from importlib.metadata import version
import json
from pathlib import Path
import struct
import zlib
from PIL import Image, ImageChops, ImageDraw
import potrace
import native_assets


def sha(data):
    return hashlib.sha256(data).hexdigest()


def restore(coverage, size):
    """Fit cubic native half-coverage contours, preserving counters and corners."""
    image = Image.frombytes('L', size, coverage)
    sample = image.resize((size[0] * 2, size[1] * 2), Image.Resampling.LANCZOS)
    paths = potrace.Bitmap(ImageChops.invert(sample)).trace(turdsize=0, alphamax=1.3,
                                                          opticurve=True, opttolerance=0.05)
    high = Image.new('1', (size[0] * 16, size[1] * 16))
    for curve in paths:
        start = (curve.start_point.x, curve.start_point.y)
        points = [start]
        for segment in curve:
            end = (segment.end_point.x, segment.end_point.y)
            if segment.is_corner:
                points.extend([(segment.c.x, segment.c.y), end])
            else:
                a, b = (segment.c1.x, segment.c1.y), (segment.c2.x, segment.c2.y)
                for step in range(1, 33):
                    t = step / 32
                    points.append(tuple((1 - t) ** 3 * start[i] + 3 * (1 - t) ** 2 * t * a[i] +
                                        3 * (1 - t) * t ** 2 * b[i] + t ** 3 * end[i] for i in range(2)))
            start = end
        contour = Image.new('1', high.size)
        ImageDraw.Draw(contour).polygon([(x * 8, y * 8) for x, y in points], fill=1)
        high = ImageChops.logical_xor(high, contour)
    return high.convert('L').resize((size[0] * 4, size[1] * 4), Image.Resampling.LANCZOS)


def planes(face_source, halo_source, size, clear_rows=False, fractional_face=False):
    if clear_rows:
        # Composed native cards clear their first/last logical rows. Apply
        # that framing before interpolation so descender halos fade to zero
        # rather than ending at the physical texture edge.
        width = size[0]
        face_source = bytes(width) + face_source[width:-width] + bytes(width)
        halo_source = bytes(width) + halo_source[width:-width] + bytes(width)
    if fractional_face:
        # The resampled native E has a partially covered top bar. A binary
        # half-coverage trace severs it and turns its tip into a white dot.
        # Retain its actual fractional coverage with smooth interpolation.
        face = Image.frombytes('L', size, face_source).resize((size[0] * 4, size[1] * 4),
                                                            Image.Resampling.BICUBIC)
    else:
        face = restore(face_source, size)
    # Preserve the native halo's fractional opacity/softness. Turning it
    # into a filled contour makes the accepted border visibly too heavy.
    halo = Image.frombytes('L', size, halo_source).resize(face.size, Image.Resampling.BICUBIC)
    alpha = ImageChops.lighter(face, halo)
    return face.tobytes(), alpha.tobytes()


def curate(destination):
    native_manifest = native_assets.ASSETS / 'manifest.json'
    native = json.loads(native_manifest.read_text())
    order = ''.join(sorted(native['glyph_layout']))
    assert len(order) == 49
    stream = bytearray(b'HDGLYPH2' + struct.pack('<I', 49))
    records = {}
    for ch in order:
        metric = native['glyph_layout'][ch]
        face_source = base64.b64decode(metric['face_coverage_b64'], validate=True)
        halo_source = base64.b64decode(metric['halo_coverage_b64'], validate=True)
        assert len(face_source) == len(halo_source) == 576
        assert sha(face_source) == metric['face_coverage_sha256']
        assert sha(halo_source) == metric['halo_coverage_sha256']
        face, alpha = planes(face_source, halo_source, (24, 24), clear_rows=True, fractional_face=ch == 'E')
        stream.extend(bytes([ord(ch)]) + face + alpha)
        records[ch] = {'native_core_bbox': metric['core_bbox'], 'advance': metric['advance'],
                       'source_face_sha256': sha(face_source), 'source_halo_sha256': sha(halo_source),
                       'face_sha256': sha(face), 'alpha_sha256': sha(alpha)}
    # Preserve the exact native Lost Woods wordmark's own layout and letters.
    forest = native_assets.resources()[native_assets.PREFIX + 'lost_woods']
    raw = forest[80:]
    face_source = bytes(round((v >> 4) * (v & 15) * 255 / 225) for v in raw)
    halo_source = bytes((v & 15) * 17 for v in raw)
    face, alpha = planes(face_source, halo_source, (144, 24))
    stream.extend(face + alpha)
    forest_record = {'native_resource_sha256': sha(forest), 'face_sha256': sha(face), 'alpha_sha256': sha(alpha)}
    packed = zlib.compress(bytes(stream), 9)
    destination.mkdir(parents=True, exist_ok=True)
    parts = []
    for index, offset in enumerate(range(0, len(packed), 98304), 1):
        body = packed[offset:offset + 98304]
        encoded = base64.b64encode(body)
        name = f'glyphs.zlib.{index:03}.b64'
        (destination / name).write_bytes(encoded)
        parts.append({'name': name, 'bytes': len(body), 'sha256': sha(body), 'encoded_bytes': len(encoded),
                      'encoded_sha256': sha(encoded)})
    manifest = {'version': 2, 'order': order, 'tile': [96, 96], 'parts': parts,
                'decoded_bytes': len(stream), 'decoded_sha256': sha(bytes(stream)),
                'packed_bytes': len(packed), 'packed_sha256': sha(packed), 'glyphs': records, 'forest': forest_record,
                'native_manifest_sha256': sha(native_manifest.read_bytes()),
                'authoring_dependencies': {name: version(name) for name in ('potracer', 'Pillow', 'numpy')},
                'authorship': 'Native OoT face/halo coverage only: 48 cubic-contour glyphs and fractional native E interpolation. No generated glyph geometry.',
                'recipe': 'Clear composed native top/bottom rows before interpolation; native face coverage at 2x Lanczos; Potrace half-coverage (turdsize0, alphamax1.3, tolerance0.05); 16x supersampling to 4x output. E preserves its partial top-bar coverage at bicubic4x to avoid a detached trace artifact. Native fractional halo at bicubic4x. Original forest donor framing is retained. Native metrics and counters retained.'}
    (destination / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({'native_contour_glyphs': 48, 'fractional_native_glyphs': 'E', 'exact_layout_forest': True,
                      'compressed_bytes': len(packed), 'parts': len(parts), 'native_metrics': 'unchanged'}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--destination', type=Path, default=Path(__file__).parent / 'assets/hd-v1')
    curate(parser.parse_args().destination)
