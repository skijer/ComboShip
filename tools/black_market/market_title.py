#!/usr/bin/env python3
"""Audit and pack the original default market wordmark without image libraries.

The immutable geometry bundle owns no title pixels. This preparation pass adds
only its separately authored, overrideable native IA8 location-card resource.
"""
import hashlib
import json
from pathlib import Path
import re
import struct
import zlib

ASSETS = Path(__file__).parent / 'assets/title'
RESOURCE = 'custom/skiejer_market/title'
WIDTH, HEIGHT = 192, 24
PNG = 'black-market-wordmark.png'
RAW = 'black-market-title.ia8'
PIXELS = WIDTH * HEIGHT


def resource_paths():
    return {RESOURCE}


def _png(data, expected_dimensions=None):
    """Check the complete PNG stream, including ancillary chunk checksums."""
    if not data.startswith(b'\x89PNG\r\n\x1a\n'):
        raise ValueError('market title PNG signature is invalid')
    offset, dimensions, image_data = 8, None, False
    while offset < len(data):
        if len(data) - offset < 12:
            raise ValueError('market title PNG chunk is truncated')
        size = struct.unpack_from('>I', data, offset)[0]
        if size > len(data) - offset - 12:
            raise ValueError('market title PNG chunk size is invalid')
        kind = data[offset + 4:offset + 8]
        body = data[offset + 8:offset + 8 + size]
        crc = struct.unpack_from('>I', data, offset + 8 + size)[0]
        if zlib.crc32(kind + body) != crc:
            raise ValueError('market title PNG chunk CRC mismatch')
        if dimensions is None and kind != b'IHDR':
            raise ValueError('market title PNG must begin with IHDR')
        if kind == b'IHDR':
            if dimensions is not None or size != 13:
                raise ValueError('market title PNG IHDR is invalid')
            width, height, depth, color, compression, filtering, interlace = struct.unpack('>IIBBBBB', body)
            if (not width or not height or compression != 0 or filtering != 0 or interlace not in (0, 1) or
                    depth not in {0: (1, 2, 4, 8, 16), 2: (8, 16), 3: (1, 2, 4, 8),
                                  4: (8, 16), 6: (8, 16)}.get(color, ())):
                raise ValueError('market title PNG dimensions/encoding are invalid')
            dimensions = (width, height)
            if expected_dimensions is not None and dimensions != expected_dimensions:
                raise ValueError('market title PNG dimensions changed')
        elif kind == b'IDAT':
            image_data |= bool(body)
        elif kind == b'IEND':
            if size != 0 or not image_data or offset + 12 != len(data):
                raise ValueError('market title PNG end or image data is invalid')
            return
        offset += size + 12
    raise ValueError('market title PNG has no complete IEND')


def _context(value):
    return bool(value.strip()) if isinstance(value, str) else isinstance(value, dict) and bool(value)


def _name(value):
    if (not isinstance(value, str) or not value or value in ('.', '..') or
            '/' in value or '\\' in value or Path(value).name != value):
        raise ValueError('market title provenance contains an unsafe asset path')
    return value


def _assets(directory):
    directory = Path(directory)
    try:
        provenance = json.loads((directory / 'provenance.json').read_text(encoding='utf-8'))
    except (OSError, UnicodeError, ValueError) as error:
        raise ValueError('market title provenance cannot be read') from error
    if (not isinstance(provenance, dict) or type(provenance.get('version')) is not int or
            provenance['version'] != 1 or provenance.get('format') != 'IA8' or
            type(provenance.get('logical_width')) is not int or provenance['logical_width'] != WIDTH or
            type(provenance.get('logical_height')) is not int or provenance['logical_height'] != HEIGHT or
            not _context(provenance.get('authorship', provenance.get('authoring'))) or
            not _context(provenance.get('source'))):
        raise ValueError('market title provenance version, dimensions, encoding or authorship/source changed')
    files = provenance.get('files')
    if not isinstance(files, dict) or not {PNG, RAW} <= files.keys():
        raise ValueError('market title provenance must identify the PNG and raw IA8 assets')
    source = provenance['source']
    references = [source['file']] if isinstance(source, dict) and 'file' in source else []
    if 'originalPNG' in provenance:
        original = provenance['originalPNG']
        references.append(original.get('file') if isinstance(original, dict) else original)
    if any(_name(name) not in files for name in references):
        raise ValueError('market title provenance source has no audited file record')
    result = {}
    for name, record in files.items():
        path = directory / _name(name)
        if (not isinstance(record, dict) or type(record.get('bytes')) is not int or record['bytes'] < 0 or
                not isinstance(record.get('sha256'), str) or not re.fullmatch('[0-9a-fA-F]{64}', record['sha256']) or
                not isinstance(record.get('crc32'), str) or not re.fullmatch('[0-9a-fA-F]{8}', record['crc32'])):
            raise ValueError('market title provenance file hash/CRC/size record is invalid: ' + name)
        try:
            if path.resolve().parent != directory.resolve():
                raise ValueError('market title provenance asset path leaves its directory')
            data = path.read_bytes()
        except OSError as error:
            raise ValueError('market title provenance asset cannot be read: ' + name) from error
        if len(data) != record['bytes'] or hashlib.sha256(data).hexdigest() != record['sha256'].lower():
            raise ValueError('market title asset hash/size mismatch: ' + name)
        if f'{zlib.crc32(data):08x}' != record['crc32'].lower():
            raise ValueError('market title asset CRC mismatch: ' + name)
        if name.lower().endswith('.png'):
            _png(data, (WIDTH, HEIGHT) if name == PNG else None)
        result[name] = data
    return result


def _alpha(raw):
    if len(raw) != PIXELS:
        raise ValueError('market title raw IA8 size changed')
    if (not any(value & 15 == 0 for value in raw) or not any(value & 15 == 15 for value in raw) or
            not any(value >> 4 and value & 15 for value in raw)):
        raise ValueError('market title IA8 alpha must contain transparent background and solid visible lettering')
    perimeter = raw[:WIDTH] + raw[-WIDTH:] + raw[::WIDTH] + raw[WIDTH - 1::WIDTH]
    if any(value & 15 for value in perimeter):
        raise ValueError('market title IA8 alpha must retain its transparent outer perimeter')


def validate_texture(data):
    """Reject a malformed native texture record or an empty/opaque alpha mask."""
    if len(data) != 80 + PIXELS:
        raise ValueError('market title texture size changed')
    if data[0] != 0 or data[4:8] != b'XETO' or struct.unpack_from('<I', data, 8)[0] != 0:
        raise ValueError('market title texture header must use native texture version zero')
    if struct.unpack_from('<IIII', data, 64) != (8, WIDTH, HEIGHT, PIXELS):
        raise ValueError('market title texture must contain the native IA8 dimensions and byte count')
    _alpha(data[80:])


def texture(asset_directory=ASSETS):
    files = _assets(asset_directory)
    raw = files[RAW]
    _alpha(raw)
    # V0 reads type, width, height and byte count directly before native pixels.
    header = struct.pack('<B3xIIQ', 0, 0x4F544558, 0, 0xDEADBEEFDEADBEEF).ljust(64, b'\0')
    data = header + struct.pack('<IIII', 8, WIDTH, HEIGHT, PIXELS) + raw
    validate_texture(data)
    return data


def apply(resources, asset_directory=ASSETS):
    title = texture(asset_directory)
    result = dict(resources)
    result[RESOURCE] = title
    return result
