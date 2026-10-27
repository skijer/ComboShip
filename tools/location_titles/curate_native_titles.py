#!/usr/bin/env python3
"""Offline curation from user-owned OoT art; never needed by a normal build.

Uses Pillow only while authoring. No desktop font or image generation is used.
The native IA8 cards are the single source of both normal and optional HD art.
"""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import struct
from zipfile import ZipFile
import zlib
from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[2]
PREFIX = 'custom/oot_location_titles/v1/'
# Native logical pair spacing is shared with the optional HD compositor via
# the manifest. The y crop retains its donor's leading bearing; fa needs
# optical overlap beneath the f hook, with no change to either glyph.
PAIR_KERNING = {'ar': -1, 'et': -1, 'fa': -2,
                'ay': -3, 'ey': -3, 'gy': -3, 'hy': -3, 'ny': -3, 'ry': -3, 'ty': -3}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def native_record(raw, width):
    header = struct.pack('<B3xIIQ', 0, 0x4F544558, 0, 0xDEADBEEFDEADBEEF).ljust(64, b'\0')
    return header + struct.pack('<IIII', 8, width, 24, len(raw)) + raw


def native_halo(face):
    # Fit against alpha in the actual English place-card inventory: radius
    # sqrt(5), 1.5x coverage and sigma0.5 give mean alpha error 4.607/255.
    # Applying the same native-derived profile to every glyph avoids retaining
    # the next donor letter's shadow as a rectangular black fragment.
    boosted = face.point(lambda value: min(255, round(value * 1.5)))
    grown = Image.new('L', face.size)
    for dy in range(-2, 3):
        for dx in range(-2, 3):
            if dx * dx + dy * dy <= 5:
                shifted = Image.new('L', face.size)
                shifted.paste(boosted, (dx, dy))
                grown = ImageChops.lighter(grown, shifted)
    return ImageChops.lighter(face, grown.filter(ImageFilter.GaussianBlur(0.5)))


def curate(oot, coverage, destination, preview):
    metrics = json.loads((ROOT / 'tools/location_titles/assets/native-oot-v2/native-title-pack.json').read_text())['native_metrics']
    isolation_path = Path(__file__).parent / 'assets/native-v3/glyph-isolation.json'
    isolation = json.loads(isolation_path.read_text())
    assert isolation['version'] == 1
    isolation = isolation['glyphs']
    glyphs, provenance = {}, {}
    with ZipFile(oot) as archive:
        names = archive.namelist()
        for ch, metric in metrics.items():
            name = next(n for n in names if '/' + metric['card'] + '/' in n and n.endswith('ENGTex'))
            record = archive.read(name)
            source = record[80:]
            start, _, end, _ = metric['crop']
            width = end - start
            face = Image.new('L', (width, 24))
            values = [round((source[y * 144 + x] >> 4) * (source[y * 144 + x] & 15) * 255 / 225)
                      for y in range(24) for x in range(start, end)]
            for x, y, _ in metric['removed_neighbor_pixels']:
                values[y * width + x - start] = 0
            # Check the original audited crop before applying this addendum.
            # Crop windows can include the next/previous donor letter's face,
            # not just its shadow. Remove only explicit, source-checked pixels;
            # component filtering would also erase real i/j dots and edge AA.
            assert sha(bytes(values)) == metric['face_sha256']
            removed = list(metric['removed_neighbor_pixels'])
            if ch in isolation:
                fix = isolation[ch]
                assert (fix['source'] == name and fix['source_sha256'] == sha(record) and
                        fix['source_crop'] == metric['crop'] and
                        fix['pre_sweep_face_sha256'] == metric['face_sha256'])
                for x, y, value in fix['removed_neighbor_pixels']:
                    assert start <= x < end and 0 <= y < 24
                    assert values[y * width + x - start] == value
                    values[y * width + x - start] = 0
                removed.extend(fix['removed_neighbor_pixels'])
                assert sha(bytes(values)) == fix['face_sha256']
            face.putdata(values)
            tile = Image.new('L', (24, 24))
            tile.paste(face, (3, 0))
            native_face = Image.new('L', (24, 24))
            for y in range(24):
                for x in range(width):
                    if face.getpixel((x, y)):
                        native_face.putpixel((x + 3, y), source[y * 144 + start + x])
            glyphs[ch] = (tile, native_halo(tile), metric['advance'], native_face)
            provenance[ch] = {'source': name, 'crop': metric['crop'], 'face_sha256': sha(bytes(values)),
                              'isolated_neighbor_pixels': removed}
        # Native OoT font source supplies the five characters absent from its
        # English place wordmarks. Explicit provenance avoids a false claim of
        # place-bitmap identity for these resampled native-font characters.
        for ch, width, height in [('E', 9, 15), ('N', 12, 15), ('O', 14, 15), ('&', 14, 15), ('j', 6, 18)]:
            name = next(n for n in names if n.startswith(f'textures/nes_font_static/gMsgChar{ord(ch):02X}'))
            source = archive.read(name)[80:]
            values = bytes(v for b in source for v in ((b >> 4) * 17, (b & 15) * 17))
            original = Image.frombytes('L', (16, 16), values)
            crop = original.getbbox()
            face = original.crop(crop).resize((width, height), Image.Resampling.LANCZOS)
            tile = Image.new('L', (24, 24))
            tile.paste(face, (3, 3))
            # Same native cap height/baseline; pre-baked full halo, not the
            # thin cardinal-offset approximation in the old presenter.
            native_face = Image.frombytes('L', tile.size,
                                         bytes(round(value / 17) * 16 + 15 if round(value / 17) else 0
                                               for value in tile.tobytes()))
            glyphs[ch] = (tile, native_halo(tile), width + 1, native_face)
            provenance[ch] = {'source': name, 'native_source_sha256': sha(source), 'native_crop': list(crop),
                              'place_card_size': [width, height], 'resampling': 'Lanczos, native OoT font only'}

        def card(text, width=240, extra_kerning=None):
            # Crop advances stay native. Four-pixel spaces match the Forest
            # Temple donor; only the audited pair positions change.
            advance = lambda ch: 4 if ch == ' ' else glyphs[ch][2]
            kern = dict(PAIR_KERNING)
            kern.update(extra_kerning or {})
            length = sum(map(advance, text)) + sum(kern.get(text[i:i + 2], 0) for i in range(len(text) - 1))
            assert length <= width - 8, (text, length)
            face, alpha = Image.new('L', (width, 24)), Image.new('L', (width, 24))
            native_faces = Image.new('L', (width, 24))
            x = (width - length) // 2 - 3
            for index, ch in enumerate(text):
                if ch != ' ':
                    f, a, _, packed_face = glyphs[ch]
                    for yy in range(24):
                        for xx in range(24):
                            if 0 <= x + xx < width:
                                p = (x + xx, yy)
                                if f.getpixel((xx, yy)) > face.getpixel(p):
                                    native_faces.putpixel(p, packed_face.getpixel((xx, yy)))
                                face.putpixel(p, max(face.getpixel(p), f.getpixel((xx, yy))))
                                alpha.putpixel(p, max(alpha.getpixel(p), a.getpixel((xx, yy))))
                x += advance(ch) + kern.get(text[index:index + 2], 0)
            raw = bytearray()
            for native_face, a in zip(native_faces.tobytes(), alpha.tobytes()):
                raw.append(native_face if native_face else round(a / 17))
            # Native place sprites have a transparent one-pixel outer frame.
            for xx in range(width):
                raw[xx] = raw[(24 - 1) * width + xx] = 0
            for yy in range(24):
                raw[yy * width] = raw[yy * width + width - 1] = 0
            return native_record(bytes(raw), width)

        titles = json.loads(Path(coverage).read_text())
        assert len(titles) == 67 and all(not set(row['text']) - (set(glyphs) | {' '}) for row in titles)
        resources = {PREFIX + '1/' + row['id']: card(row['text']) for row in titles}
        # The narrow native j begins two pixels into its tile. Tighten the
        # conspicuous gap before it in this shop name, keeping every scene's
        # accepted layout and the actual spelling unchanged.
        market_pair_kerning = {'ej': -2}
        resources[PREFIX + 'market'] = card("Skiejer's Black Market", extra_kerning=market_pair_kerning)
        donor_name = 'textures/g_pn_36/gLostWoodsTitleCardENGTex'
        resources[PREFIX + 'lost_woods'] = archive.read(donor_name)
        assert len(resources[PREFIX + 'lost_woods']) == 3536
        forest_control = archive.read('textures/g_pn_01/gForestTempleTitleCardENGTex')
        market_control = archive.read('textures/g_pn_17/gMarketTitleCardENGTex')

    destination.mkdir(parents=True, exist_ok=True)
    stream = bytearray(b'OTITLES1' + struct.pack('<I', len(resources)))
    for name, data in sorted(resources.items()):
        encoded = name.encode('ascii')
        stream.extend(struct.pack('<H', len(encoded)) + encoded + struct.pack('<I', len(data)) + data)
    packed = zlib.compress(bytes(stream), 9)
    encoded = base64.b64encode(packed)
    assert len(packed) <= 98304 and len(encoded) <= 131072
    (destination / 'cards.zlib.b64').write_bytes(encoded)
    manifest = {'version': 1, 'titles': [{'id': row['id'], 'text': row['text']} for row in titles],
                'resources': {name: {'bytes': len(data), 'sha256': sha(data)} for name, data in sorted(resources.items())},
                'decoded_bytes': len(stream), 'decoded_sha256': sha(bytes(stream)),
                'packed_bytes': len(packed), 'packed_sha256': sha(packed),
                'encoded_bytes': len(encoded), 'encoded_sha256': sha(encoded), 'glyph_provenance': provenance,
                'glyph_layout': {ch: {'advance': values[2],
                                      'core_bbox': list(values[0].point(lambda a: 255 if a >= 128 else 0).getbbox()),
                                      'face_coverage_b64': base64.b64encode(values[0].tobytes()).decode('ascii'),
                                      'halo_coverage_b64': base64.b64encode(values[1].tobytes()).decode('ascii'),
                                      'face_coverage_sha256': sha(values[0].tobytes()),
                                      'halo_coverage_sha256': sha(values[1].tobytes())}
                                 for ch, values in sorted(glyphs.items())},
                'forest_donor_sha256': sha(resources[PREFIX + 'lost_woods']),
                'glyph_isolation_sha256': sha(isolation_path.read_bytes()),
                'pair_kerning': PAIR_KERNING,
                'market_pair_kerning': market_pair_kerning,
                'native_market_reference_sha256': sha(market_control),
                'notes': 'Native source artwork, baked halo and 24px baseline. E/N/O/&/j use native OoT NES-font masks, not invented place-card pixels. Optional HD traces these native face contours; generated substitute glyph shapes are rejected.'}
    (destination / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')

    def rgba(record):
        _, w, h, _ = struct.unpack_from('<IIII', record, 64)
        image = Image.new('RGBA', (w, h))
        image.putdata([((v >> 4) * 17,) * 3 + ((v & 15) * 17,) for v in record[80:]])
        return image

    preview.mkdir(parents=True, exist_ok=True)
    # One canonical glyph atlas is the reference for the optional smooth HD
    # restoration; every title then uses the same letters and native metrics.
    order = ''.join(sorted(glyphs))
    reference = Image.new('RGBA', (7 * 192, 7 * 192))
    for index, ch in enumerate(order):
        tile, alpha, _, packed_face = glyphs[ch]
        image = Image.new('RGBA', (24, 24))
        image.putdata([((p >> 4) * 17,) * 3 + ((p & 15) * 17,) if p else (0, 0, 0, a)
                       for p, a in zip(packed_face.tobytes(), alpha.tobytes())])
        image = image.resize((192, 192), Image.Resampling.NEAREST)
        reference.alpha_composite(image, ((index % 7) * 192, (index // 7) * 192))
    reference.save(preview / 'native-glyph-atlas-reference.png')
    (preview / 'native-glyph-atlas-order.json').write_text(json.dumps({'columns': 7, 'rows': 7, 'order': order}))
    rows = [('Native OoT Market reference', market_control), ('Native OoT Forest Temple reference', forest_control),
            ('Curated Forest Temple control', card('Forest Temple')),
            ('Built-in South Clock Town', resources[PREFIX + '1/0104']),
            ('Built-in Black Market', resources[PREFIX + 'market'])]
    canvas = Image.new('RGB', (1020, len(rows) * 135), (38, 43, 38))
    draw = ImageDraw.Draw(canvas)
    for index, (label, record) in enumerate(rows):
        draw.text((18, index * 135 + 8), label, fill='white')
        image = rgba(record); image = image.resize((image.width * 4, 96), Image.Resampling.NEAREST)
        canvas.paste(image, (18, index * 135 + 30), image)
    canvas.save(preview / 'native-title-parity-controls.png')
    height = ((len(titles) + 1) // 2) * 84
    canvas = Image.new('RGB', (1470, height), (38, 43, 38)); draw = ImageDraw.Draw(canvas)
    for index, row in enumerate(titles):
        x, y = (index % 2) * 735, (index // 2) * 84
        draw.text((x + 6, y + 3), row['id'] + '  ' + row['text'], fill='white')
        image = rgba(resources[PREFIX + '1/' + row['id']]).resize((720, 72), Image.Resampling.NEAREST)
        canvas.paste(image, (x + 5, y + 15), image)
    canvas.save(preview / 'all-native-location-cards.png')
    print(json.dumps({'native_cards': len(resources), 'English_title_ids': len(titles), 'compressed_bytes': len(packed),
                      'extra_native_font_characters': 'ENO&j', 'verdict': 'offline candidate; runtime/visual acceptance pending'}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--oot', type=Path, required=True)
    parser.add_argument('--coverage', type=Path, required=True)
    parser.add_argument('--destination', type=Path, default=Path(__file__).parent / 'assets/native-v3')
    parser.add_argument('--preview', type=Path, required=True)
    args = parser.parse_args()
    curate(args.oot, args.coverage, args.destination, args.preview)
