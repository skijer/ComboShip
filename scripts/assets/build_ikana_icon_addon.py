#!/usr/bin/env python3
"""Build only Shield of Ikana's private HD icon and authored-font pause title.

The raw RGBA texture envelope follows the existing NEI HD icon pack. IA4 is the
pause-title draw format; raw RGBA preserves antialiasing before its IA combiner.
Unmodified MM 32px pixels are retained in both port archives as the no-addon
fallback. The optional addon never owns either ordinary Mirror Shield key.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[2]
ICON = 'textures/icon_item_custom/gItemIconShieldOfIkanaTex'
TITLE = 'textures/item_name_custom/gShieldOfIkanaNameTex'
STOCK_KEY = 'icon_item_static_yar/gItemIconMirrorShieldTex'
STOCK_SHA256 = 'f145ac21320f65594a2383db430c65c6570c0509bdf47be4b69581c56f607524'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def texture(image, kind, base):
    image = image.convert('RGBA')
    header = bytearray(64)
    struct.pack_into('<BB2xIIQ', header, 0, 0, 1, 0x4F544558, 1, 0xDEADBEEFDEADBEEF)
    w, h = image.size
    # IA4 native tile consumes .5 bytes/pixel; RGBA raw replacement consumes 4.
    # Therefore title HByteScale includes 8x pixel-format conversion as in the
    # exact recovered companion. RGBA32 icons use the ordinary dimension ratio.
    byte_ratio = 8 if kind == 7 else 1
    pixels = image.tobytes()
    body = struct.pack('<4I2fI', kind, w, h, 1, w / base[0] * byte_ratio, h / base[1], len(pixels))
    return bytes(header) + body + pixels


def title(font_path):
    # Match the recovered companion: Century Gothic Bold, 96px, centered within
    # 1024x128, white antialiased core with an 8px dark outline. All other titles
    # remain in their original packs. The shader supplies the native navy tone.
    text = 'Shield of Ikana'
    font = ImageFont.truetype(str(font_path), 96)
    assert font.getlength(text) <= 1008
    bounds = font.getbbox(text)
    mask = Image.new('L', (1024, 128), 0)
    ImageDraw.Draw(mask).text((round((1024 - font.getlength(text)) / 2),
                              (128 - (bounds[3] - bounds[1])) // 2 - bounds[1] - 2),
                             text, font=font, fill=255)
    alpha = mask.filter(ImageFilter.MaxFilter(17))
    return Image.merge('RGBA', (mask, mask, mask, alpha))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--icon', type=Path, required=True, help='unmodified approved art master with alpha/shadow')
    parser.add_argument('--stock-mm', type=Path, required=True)
    parser.add_argument('--companion', type=Path, required=True, help='exact existing companion, provenance only')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--write-stock-sources', action='store_true')
    args = parser.parse_args()
    args.evidence.mkdir(parents=True, exist_ok=True)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(args.stock_mm) as source:
        stock = source.read(STOCK_KEY)
    assert sha(stock) == STOCK_SHA256, 'unexpected native MM shield pixels'
    assert struct.unpack_from('<4I', stock, 64) == (1, 32, 32, 4096)
    native = Image.frombytes('RGBA', (32, 32), stock[80:])
    if args.write_stock_sources:
        for host in ('mm', 'soh'):
            native.save(ROOT / host / 'assets/custom' / (ICON + '.rgba32.png'))
    for host in ('mm', 'soh'):
        fallback = Image.open(ROOT / host / 'assets/custom' / (ICON + '.rgba32.png')).convert('RGBA')
        assert fallback.size == (32, 32) and fallback.tobytes() == native.tobytes()
    art = Image.open(args.icon).convert('RGBA')
    assert art.size[0] == art.size[1] and art.getchannel('A').getextrema()[0] == 0
    # Texture import only. Keep authored alpha and its shadow; no redraw/composite.
    hd_icon = art.resize((512, 512), Image.Resampling.LANCZOS)
    font_path = ROOT / 'soh/assets/custom/fonts/CenturyGothicBold.ttf'
    hd_title = title(font_path)
    hd_icon.save(args.evidence / 'ikana-icon-512.png')
    hd_title.save(args.evidence / 'ikana-title-1024.png')
    data = {ICON: texture(hd_icon, 1, (32, 32)), TITLE: texture(hd_title, 7, (128, 16))}
    entries = {prefix + key: payload for prefix in ('', 'alt/') for key, payload in data.items()}
    with ZipFile(args.output, 'w', compression=ZIP_DEFLATED, compresslevel=9) as archive:
        for key, payload in sorted(entries.items()):
            info = ZipInfo(key, (2026, 10, 25, 0, 0, 0))
            info.compress_type = ZIP_DEFLATED
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            archive.writestr(info, payload, compress_type=ZIP_DEFLATED, compresslevel=9)
    with ZipFile(args.output) as archive:
        assert set(archive.namelist()) == set(entries)
        for key, payload in entries.items():
            assert archive.read(key) == payload
            kind, w, h, flags, sx, sy, length = struct.unpack_from('<4I2fI', payload, 64)
            assert len(payload) == length + 92 and flags == 1 and length == w * h * 4
            assert (kind, w, h, sx, sy) in ((1, 512, 512, 16., 16.), (7, 1024, 128, 64., 8.))
    manifest = {
        'addon': args.output.name, 'sha256': sha(args.output.read_bytes()),
        'native_mm_archive_sha256': sha(args.stock_mm.read_bytes()),
        'native_mm_key': STOCK_KEY, 'native_mm_resource_sha256': STOCK_SHA256,
        'recovered_companion_name': args.companion.name, 'recovered_companion_sha256': sha(args.companion.read_bytes()),
        'icon_master_sha256': sha(args.icon.read_bytes()), 'icon_conversion': 'RGBA 512x512 Lanczos; alpha/shadow retained',
        'title': 'Shield of Ikana; authored Century Gothic Bold 96px, 1024x128, 8px native dark outline',
        'font_sha256': sha(font_path.read_bytes()),
        'resources': [{ 'key': key, 'sha256': sha(payload), 'bytes': len(payload),
                       'kind_dimensions_flags_scales_length': list(struct.unpack_from('<4I2fI',payload,64))}
                      for key, payload in sorted(entries.items())],
        'installation': 'Requires a new executable including the private Ikana icon path and rebuilt soh.o2r/2ship.o2r. Copy the same addon to mods/soh and mods/2ship for combined play, mods/soh for Solo OoT, or mods/2ship for Solo MM. Alt off/on supported.',
        'runtime': 'Unproven in game; native fixture and metadata verification only.'
    }
    args.output.with_suffix('.manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
