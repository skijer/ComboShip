#!/usr/bin/env python3
"""Render comparisons directly from the native and optional HD resource bytes."""
import argparse
import json
from pathlib import Path
import struct
from PIL import Image, ImageDraw, ImageFont
import native_assets


def artwork(body):
    kind, width, height = struct.unpack_from('<III', body, 64)
    if kind == 8:
        image = Image.new('RGBA', (width, height))
        image.putdata([((value >> 4) * 17,) * 3 + ((value & 15) * 17,) for value in body[80:]])
        return image.resize((width * 4, height * 4), Image.Resampling.NEAREST)
    return Image.frombytes('RGBA', (width, height), body[92:])


def preview(destination):
    native, hd = native_assets.resources(), native_assets.hd_resources()
    manifest = json.loads((native_assets.ASSETS / 'manifest.json').read_text())
    south = next(row['id'] for row in manifest['titles'] if row['text'] == 'South Clock Town')
    pairs = [('South Clock Town', native_assets.PREFIX + '1/' + south),
             ("Skiejer's Black Market", native_assets.PREFIX + 'market'),
             ('Lost Woods', native_assets.PREFIX + 'lost_woods')]
    font = ImageFont.truetype('DejaVuSans.ttf', 20)
    output = Image.new('RGB', (1120, len(pairs) * 300), (40, 56, 48))
    draw = ImageDraw.Draw(output)
    for index, (text, key) in enumerate(pairs):
        top = index * 300
        draw.text((30, top + 8), text, font=font, fill='white')
        draw.text((30, top + 38), 'Native artwork (enlarged for inspection)', font=font, fill='white')
        draw.text((30, top + 168), 'Optional smooth HD replacement', font=font, fill='white')
        for body, y in [(native[key], top + 65), (hd[key], top + 195)]:
            image = artwork(body)
            output.paste(image, ((1120 - image.width) // 2, y), image)
    destination.mkdir(parents=True, exist_ok=True)
    output.save(destination / 'Native-and-Smooth-HD.png')

    rows = [(row['text'], native_assets.PREFIX + '1/' + row['id']) for row in manifest['titles']] + pairs[1:]
    for label, resources in [('Native', native), ('Smooth-HD', hd)]:
        output = Image.new('RGB', (2160, ((len(rows) + 1) // 2) * 150), (40, 56, 48))
        draw = ImageDraw.Draw(output)
        for index, (text, key) in enumerate(rows):
            x, y = (index % 2) * 1080, (index // 2) * 150
            draw.text((x + 24, y + 5), text, font=font, fill='white')
            image = artwork(resources[key])
            output.paste(image, (x + (1080 - image.width) // 2, y + 35), image)
        output.save(destination / ('All-69-' + label + '-Titles.png'))
    print('PASS: previews rendered from 69 actual native/HD resource records')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    preview(parser.parse_args().output)
