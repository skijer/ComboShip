#!/usr/bin/env python3
"""Optionally pack an authored title into native IA8; normal builds use its bytes.

ImageMagick is needed only for this authoring step. The source art is retained;
native preparation consumes the verified raw sprite without image libraries.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import zlib

WIDTH, HEIGHT = 192, 24


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def png_chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))


def preview_png(sprite):
    rows = bytearray()
    for y in range(HEIGHT):
        rows.append(0)
        for value in sprite[y * WIDTH:(y + 1) * WIDTH]:
            intensity = (value >> 4) * 17
            rows.extend((intensity, intensity, intensity, (value & 15) * 17))
    return (b'\x89PNG\r\n\x1a\n' +
            png_chunk(b'IHDR', struct.pack('>IIBBBBB', WIDTH, HEIGHT, 8, 6, 0, 0, 0)) +
            png_chunk(b'IDAT', zlib.compress(bytes(rows), 9)) + png_chunk(b'IEND', b''))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--prompt-file', type=Path, required=True)
    args = parser.parse_args()
    source = args.source.read_bytes()
    command = ['convert', str(args.source), '-alpha', 'extract', '-trim', '+repage',
               '-filter', 'Lanczos', '-resize', '188x20!', '-gravity', 'center',
               '-background', 'black', '-extent', '192x24', '-depth', '8', 'gray:-']
    alpha = subprocess.check_output(command)
    if len(alpha) != WIDTH * HEIGHT:
        raise ValueError('unexpected native title alpha dimensions')
    # Native title art carries its dark keyline in IA8 intensity; the engine
    # multiplies that image by the same primitive color/alpha during its fade.
    # Inset the white face before a one-native-pixel black outline, retaining
    # a fully transparent perimeter for texture filtering.
    pixels = bytearray()
    for y in range(HEIGHT):
        for x in range(WIDTH):
            face = alpha[y * WIDTH + x]
            edge = max(alpha[yy * WIDTH + xx]
                       for yy in range(max(0, y - 1), min(HEIGHT, y + 2))
                       for xx in range(max(0, x - 1), min(WIDTH, x + 2)))
            combined = face * 255 + edge * (255 - face)
            opacity = (combined * 15 + 32512) // 65025
            intensity = (face * 255 * 15 + combined // 2) // combined if combined else 0
            pixels.append((intensity << 4) | opacity if opacity else 0)
    sprite = bytes(pixels)
    if not any(sprite) or not any(value == 255 for value in sprite):
        raise ValueError('title must have visible solid lettering')
    if any(sprite[:WIDTH]) or any(sprite[-WIDTH:]) or any(sprite[::WIDTH]) or any(sprite[WIDTH - 1::WIDTH]):
        raise ValueError('native sprite must retain a transparent one-pixel border')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(args.source, args.output_dir / 'black-market-wordmark-original.png')
    png = preview_png(sprite)
    (args.output_dir / 'black-market-wordmark.png').write_bytes(png)
    (args.output_dir / 'black-market-title.ia8').write_bytes(sprite)
    files = {}
    for name, data in (('black-market-wordmark-original.png', source),
                       ('black-market-wordmark.png', png), ('black-market-title.ia8', sprite)):
        files[name] = {'bytes': len(data), 'sha256': sha256(data), 'crc32': f'{zlib.crc32(data):08x}'}
    provenance = {
        'version': 1, 'resource': 'custom/skiejer_market/title',
        'text': "Skiejer's Black Market", 'logical_width': WIDTH, 'logical_height': HEIGHT,
        'format': 'IA8', 'encoding': 'native high intensity nibble, low alpha nibble',
        'authoring': 'Original built-in imagegen lettering; no external font or modpack pixels',
        'source': {'file': 'black-market-wordmark-original.png'},
        'prompts': json.loads(args.prompt_file.read_text()),
        'packing': 'Alpha-only crop and Lanczos fit to 188x20, centered in 192x24; white face over one-native-pixel black keyline, independently quantized 4-bit intensity/alpha; clear outer perimeter',
        'authoring_tool': subprocess.check_output(['convert', '-version'], text=True).splitlines()[0],
        'files': files,
        'ordinary_build': 'Consumes checked-in raw bytes with the standard library; no ImageMagick or font dependency',
    }
    (args.output_dir / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n')
    print(json.dumps({key: provenance[key] for key in ('resource', 'text', 'logical_width', 'logical_height', 'files')}, indent=2))


if __name__ == '__main__':
    main()
