#!/usr/bin/env python3
"""Build optional smooth HD replacements from one restored native-font atlas."""
import base64
import hashlib
import json
from pathlib import Path
import struct
import zlib
import native_assets

ASSETS = Path(__file__).parent / 'assets/hd-v1'


def require(value, message):
    if not value:
        raise ValueError('HD title assets: ' + message)


def _source(directory=ASSETS):
    directory = Path(directory)
    manifest = json.loads((directory / 'manifest.json').read_text())
    native_manifest = native_assets.ASSETS / 'manifest.json'
    native = json.loads(native_manifest.read_text())
    require(manifest['native_manifest_sha256'] == hashlib.sha256(native_manifest.read_bytes()).hexdigest(),
            'native source identity')
    order = manifest['order']
    require(manifest['version'] == 2 and manifest['tile'] == [96, 96] and len(order) == len(set(order)) == 49 and
            all(33 <= ord(ch) < 127 for ch in order) and set(order) == set(manifest['glyphs']), 'font inventory')
    require(1 <= len(manifest['parts']) <= 16, 'part count')
    packed = bytearray()
    for index, part in enumerate(manifest['parts'], 1):
        require(part['name'] == f'glyphs.zlib.{index:03}.b64', 'part path/order')
        encoded = (directory / part['name']).read_bytes()
        require(len(encoded) == part['encoded_bytes'] <= 131072 and
                hashlib.sha256(encoded).hexdigest() == part['encoded_sha256'], 'encoded size/hash')
        body = base64.b64decode(encoded, validate=True)
        require(len(body) == part['bytes'] <= 98304 and hashlib.sha256(body).hexdigest() == part['sha256'], 'part hash')
        packed.extend(body)
    require(len(packed) == manifest['packed_bytes'] and hashlib.sha256(packed).hexdigest() == manifest['packed_sha256'],
            'compressed framing')
    require(manifest['decoded_bytes'] == 12 + 49 * (1 + 96 * 96 * 2) + 576 * 96 * 2, 'decoded size')
    decoder = zlib.decompressobj()
    raw = decoder.decompress(packed, manifest['decoded_bytes'] + 1)
    require(len(raw) == manifest['decoded_bytes'] and decoder.eof and not decoder.unused_data and
            not decoder.unconsumed_tail and hashlib.sha256(raw).hexdigest() == manifest['decoded_sha256'], 'decode/hash')
    require(raw[:8] == b'HDGLYPH2' and struct.unpack_from('<I', raw, 8)[0] == 49, 'stream header')
    offset, result = 12, {}
    for ch in order:
        require(raw[offset] == ord(ch), 'glyph order')
        offset += 1
        face, alpha = raw[offset:offset + 9216], raw[offset + 9216:offset + 18432]
        offset += 18432
        metric = manifest['glyphs'][ch]
        require(metric['source_face_sha256'] == native['glyph_layout'][ch]['face_coverage_sha256'] and
                metric['source_halo_sha256'] == native['glyph_layout'][ch]['halo_coverage_sha256'],
                'native glyph source identity')
        require(hashlib.sha256(face).hexdigest() == metric['face_sha256'] and
                hashlib.sha256(alpha).hexdigest() == metric['alpha_sha256'], 'glyph hash')
        require(any(face) and any(alpha) and all(a >= f for f, a in zip(face, alpha)), 'face/outline coverage')
        require(type(metric['advance']) is int and 1 <= metric['advance'] <= 24, 'advance bound')
        result[ch] = (face, alpha, metric['advance'])
    face, alpha = raw[offset:offset + 55296], raw[offset + 55296:]
    require(len(face) == len(alpha) == 55296 and
            manifest['forest']['native_resource_sha256'] == native['forest_donor_sha256'] and any(face) and
            hashlib.sha256(face).hexdigest() == manifest['forest']['face_sha256'] and
            hashlib.sha256(alpha).hexdigest() == manifest['forest']['alpha_sha256'] and
            all(a >= f for f, a in zip(face, alpha)), 'native forest planes')
    return result, (face, alpha)


def glyphs(directory=ASSETS):
    return _source(directory)[0]


def _record(face, alpha, width):
    require(len(face) == len(alpha) == width * 4 * 96, 'record plane size')
    pixels = bytearray()
    for f, a in zip(face, alpha):
        intensity = min(255, (f * 255 + a // 2) // a) if a else 0
        pixels.extend(bytes((intensity, intensity, intensity, a)))
    header = struct.pack('<B3xIIQ', 0, 0x4F544558, 1, 0xDEADBEEFDEADBEEF).ljust(64, b'\0')
    return header + struct.pack('<IIIIffI', 1, width * 4, 96, 1, 16, 4, len(pixels)) + pixels


def _card(text, width, font, extra_kerning=None):
    advance = lambda ch: 4 if ch == ' ' else font[ch][2]
    kerning = extra_kerning or {}
    length = sum(map(advance, text)) + sum(kerning.get(text[i:i + 2], 0) for i in range(len(text) - 1))
    require(length <= width - 8, 'unwarped title bound')
    physical_width = width * 4
    face, alpha = bytearray(physical_width * 96), bytearray(physical_width * 96)
    x = ((width - length) // 2 - 3) * 4
    for index, ch in enumerate(text):
        if ch != ' ':
            f, a, _ = font[ch]
            for y in range(96):
                for xx in range(96):
                    if 0 <= x + xx < physical_width:
                        target, source = y * physical_width + x + xx, y * 96 + xx
                        face[target] = max(face[target], f[source])
                        alpha[target] = max(alpha[target], a[source])
        x += (advance(ch) + kerning.get(text[index:index + 2], 0)) * 4
    # Match the native composed-card frame. The source rows are already
    # transparent before interpolation; remove any outer antialiasing ring.
    for plane in (face, alpha):
        plane[:physical_width] = plane[-physical_width:] = bytes(physical_width)
        for y in range(96):
            plane[y * physical_width] = plane[(y + 1) * physical_width - 1] = 0
    return _record(face, alpha, width)


def resources(directory=ASSETS):
    font, forest = _source(directory)
    manifest = json.loads((native_assets.ASSETS / 'manifest.json').read_text())
    require(set(font) == set(manifest['glyph_layout']) and
            all(font[ch][2] == manifest['glyph_layout'][ch]['advance'] for ch in font), 'native metrics parity')
    kerning = manifest['pair_kerning']
    result = {native_assets.PREFIX + '1/' + row['id']: _card(row['text'], 240, font, kerning)
              for row in manifest['titles']}
    result[native_assets.PREFIX + 'market'] = _card("Skiejer's Black Market", 240, font,
                                                 {**kerning, **manifest.get('market_pair_kerning', {})})
    result[native_assets.PREFIX + 'lost_woods'] = _record(*forest, 144)
    require(set(result) == set(native_assets.resources()), 'native resource name parity')
    return result
