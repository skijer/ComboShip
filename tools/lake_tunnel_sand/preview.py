#!/usr/bin/env python3
"""Render the evaluated source floor geometry, independently of game lighting."""
import argparse
import importlib.util
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import zipfile
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
EXPORTER = r'''
#include "entrances/LakeHyliaSurface.h"
#include <cstdio>
int main() {
 const auto mesh = ComboLakeHylia::BuildSurfaceGeometry();
 puts("["); bool first = true;
 for (bool sand : {false, true})
  for (const auto& q : sand ? mesh.sand : mesh.stone) {
   bool floor = true;
   for (auto p : {q.a,q.b,q.c,q.d}) floor &= p.y == ComboLakeHylia::kMouth.y;
   if (!floor) continue;
   if (!first) puts(","); first = false;
   printf("{\"sand\":%s,\"shade\":%u,\"points\":[",sand ? "true" : "false",q.shade);
   int i = 0;
   for (auto p : {q.a,q.b,q.c,q.d}) {
    if (i++) printf(",");
    printf("[%ld,%ld]",std::lround(p.x),std::lround(p.z));
   }
   printf("]}");
  }
 puts("]");
}
'''


def geometry():
    with tempfile.TemporaryDirectory(prefix='lake-sand-preview-') as temporary:
        source = Path(temporary) / 'geometry.cpp'
        binary = Path(temporary) / 'geometry'
        source.write_text(EXPORTER)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-I' + str(ROOT / 'combo'),
                        str(source), '-o', str(binary)], check=True)
        return json.loads(subprocess.check_output([str(binary)], text=True))


def raw_texture(data):
    typ, width, height, flags = struct.unpack_from('<IIII', data, 64)
    if typ != 2 or flags != 1:
        raise ValueError('Preview requires the selected RAW RGBA surface resource')
    return Image.frombytes('RGBA', (width, height), data[92:]).convert('RGB')


def area(points):
    return abs(sum(a[0] * b[1] - b[0] * a[1]
                   for a, b in zip(points, points[1:] + points[:1]))) * .5


def preview(output, stone_archive=None):
    spec = importlib.util.spec_from_file_location('sand_prepare', ROOT / 'scripts/assets/prepare_lake_tunnel_sand.py')
    prepare = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(prepare)
    sand = raw_texture(prepare.verified_resources()[prepare.SAND])
    stone = Image.new('RGB', (32, 32), (108, 105, 97))
    if stone_archive:
        with zipfile.ZipFile(stone_archive) as archive:
            stone = raw_texture(archive.read('scenes/shared/spot06_scene/spot06_sceneTex_010EB8'))
    quads = geometry()
    width, height, scale = 1236, 600, 3
    origin_x, origin_z = -2237, 3615
    canvas = Image.new('RGB', (width, height), (32, 89, 109))
    coverage = Image.new('L', (width, height))
    draw = ImageDraw.Draw(coverage)
    shade = Image.new('L', (width, height))
    shades = ImageDraw.Draw(shade)
    for q in quads:
        polygon = [((x - origin_x) * scale, (z - origin_z) * scale) for x, z in q['points']]
        draw.polygon(polygon, fill=2 if q['sand'] else 1)
        shades.polygon(polygon, fill=q['shade'])
    pixels, masks, lighting = canvas.load(), coverage.load(), shade.load()
    sand_pixels, stone_pixels = sand.load(), stone.load()
    for y in range(height):
        z = origin_z + y / scale
        for x in range(width):
            if not masks[x, y]:
                continue
            world_x = origin_x + x / scale
            image = sand if masks[x, y] == 2 else stone
            image_pixels = sand_pixels if masks[x, y] == 2 else stone_pixels
            # Actual custom 32x32 world-coordinate sampler: tc=world*4,
            # then /32 N64 units per pixel and image/logical rescale.
            u = math.floor((world_x + 2185) * image.width / 256) % image.width
            v = math.floor((z - 3717) * image.height / 256) % image.height
            color = image_pixels[u, v]
            pixels[x, y] = tuple(int(c * (.68 + .32 * lighting[x, y] / 255)) for c in color)
    total = sum(area(q['points']) for q in quads)
    sand_area = sum(area(q['points']) for q in quads if q['sand'])
    output_image = Image.new('RGB', (width + 64, height + 142), (20, 25, 31))
    output_image.paste(canvas, (32, 85))
    labels = ImageDraw.Draw(output_image)
    labels.text((32, 18), f'Lake Hylia tunnel floor - source geometry: {sand_area / total:.1%} sand', fill='white')
    labels.text((32, 42), 'Top-down material preview; evaluated, rounded source vertices. In-game lighting and acceptance remain untested.', fill=(190, 200, 210))
    labels.text((32, height + 103), 'Cave / portal end  <---                                      --->  Lake / outflow', fill='white')
    labels.text((32, height + 122), 'Gold: dedicated sand  |  Gray: stone  |  Blue: stream / area outside platform (schematic)', fill=(190, 200, 210))
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output_image.save(output)
    print(json.dumps({'preview': str(output), 'evaluated_quads': len(quads),
                      'dry_floor_area': total, 'sand_area': sand_area, 'sand_fraction': sand_area / total}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--stone-archive', type=Path)
    args = parser.parse_args()
    preview(args.output, args.stone_archive)
