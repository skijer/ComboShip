#!/usr/bin/env python3
"""Audit the original market wordmark through the actual two-port preparation."""
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
import zlib

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / 'tools/black_market'
sys.path.insert(0, str(TOOLS))
RESOURCE = 'custom/skiejer_market/title'
WIDTH, HEIGHT = 192, 24


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))


def png(width, height, pixels):
    rows = b''.join(b'\0' + pixels[row * width * 4:(row + 1) * width * 4] for row in range(height))
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)) +
            chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b''))


def record(data):
    return {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(), 'crc32': f'{zlib.crc32(data):08x}'}


def fixture(directory):
    """Small original test pixels, encoded without image-authoring dependencies."""
    directory = Path(directory)
    directory.mkdir()
    raw = bytes(0xFF if 8 < x < WIDTH - 9 and 7 < y < HEIGHT - 8 else 0
                for y in range(HEIGHT) for x in range(WIDTH))
    preview = png(WIDTH, HEIGHT, b''.join(bytes((255, 255, 255, (value & 15) * 17)) for value in raw))
    files = {'black-market-wordmark.png': preview, 'black-market-title.ia8': raw,
             'black-market-wordmark-original.png': png(2, 2, bytes((255, 255, 255, 255)) * 4)}
    for name, data in files.items():
        (directory / name).write_bytes(data)
    metadata = {'version': 1, 'format': 'IA8', 'logical_width': WIDTH, 'logical_height': HEIGHT,
                'authoring': 'Original test mask', 'source': {'file': 'black-market-wordmark-original.png'},
                'files': {name: record(data) for name, data in files.items()}}
    (directory / 'provenance.json').write_text(json.dumps(metadata))
    return directory


def refresh(directory, filename):
    path = directory / 'provenance.json'
    metadata = json.loads(path.read_text())
    metadata['files'][filename] = record((directory / filename).read_bytes())
    path.write_text(json.dumps(metadata))


class NativeMarketTitle(unittest.TestCase):
    def title(self):
        path = TOOLS / 'market_title.py'
        self.assertTrue(path.is_file(), 'Default native preparation has no audited original title asset wrapper')
        import market_title
        return market_title

    def preparation(self):
        return load(ROOT / 'scripts/assets/prepare_native_market.py', 'prepare_market_title_test')

    def test_default_wordmark_has_native_ia8_framing_and_exact_raw_pixels(self):
        title = self.title()
        self.assertEqual((title.RESOURCE, title.WIDTH, title.HEIGHT), (RESOURCE, WIDTH, HEIGHT))
        self.assertEqual(title.resource_paths(), {RESOURCE})
        data = title.texture()
        title.validate_texture(data)
        self.assertEqual(data[4:8], b'XETO')
        self.assertEqual(struct.unpack_from('<I', data, 8), (0,))
        self.assertEqual(struct.unpack_from('<IIII', data, 64), (8, WIDTH, HEIGHT, WIDTH * HEIGHT))
        self.assertEqual(data[80:], (title.ASSETS / 'black-market-title.ia8').read_bytes())
        self.assertEqual(len(data), 80 + WIDTH * HEIGHT)
        raw = data[80:]
        self.assertEqual(hashlib.sha256(raw).hexdigest(), '5991dcef52bd5817b06ceb142e3274ff278f3e740b6a40558c03a3992a688fac')
        self.assertTrue(any(value >> 4 == 0 and value & 15 for value in raw), 'Crisp native lettering requires its black IA8 edge')
        self.assertIn(0xFF, raw, 'Native title must retain solid white letter faces')
        perimeter = raw[:WIDTH] + raw[-WIDTH:] + raw[::WIDTH] + raw[WIDTH - 1::WIDTH]
        self.assertTrue(all(value & 15 == 0 for value in perimeter), 'Title edge must not touch its texture boundary')

    def test_apply_preserves_the_immutable_geometry_bundle_and_only_adds_title(self):
        title = self.title()
        import native_market
        base = native_market.read_bundle(self.preparation().SOURCE)
        before = dict(base)
        result = title.apply(base)
        self.assertIsNot(result, base)
        self.assertEqual(base, before)
        self.assertEqual(set(result) - set(base), {RESOURCE})
        self.assertEqual({name: result[name] for name in base}, before)
        self.assertEqual(result[RESOURCE], title.texture())
        self.assertEqual(len(base), 125)

    def test_actual_preparation_ships_the_same_title_to_both_ports(self):
        title, prepare = self.title(), self.preparation()
        resources = prepare.prepared_resources()
        self.assertIn(RESOURCE, set(resources), 'Default preparation still omits the original native title')
        self.assertEqual(resources[RESOURCE], title.texture())
        with tempfile.TemporaryDirectory(prefix='market-title-prepare-') as directory:
            output = Path(directory) / 'output'
            prepare.prepare(output)
            targets = [output / game / 'assets/custom' / RESOURCE for game in ('soh', 'mm')]
            self.assertEqual([path.read_bytes() for path in targets], [title.texture()] * 2)

    def test_default_preparation_uses_only_the_standard_library(self):
        self.title()
        with tempfile.TemporaryDirectory(prefix='market-title-stdlib-') as directory:
            result = subprocess.run([sys.executable, '-S', str(ROOT / 'scripts/assets/prepare_native_market.py'),
                                     '--output-root', directory], text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            paths = [Path(directory) / game / 'assets/custom' / RESOURCE for game in ('soh', 'mm')]
            self.assertTrue(all(path.is_file() for path in paths), 'Standard-library preparation omitted title')
            self.assertEqual(paths[0].read_bytes(), paths[1].read_bytes())

    def test_each_png_chunk_crc_is_checked_even_when_provenance_hashes_match(self):
        title = self.title()
        for kind in (b'IHDR', b'IDAT', b'IEND'):
            with self.subTest(chunk=kind), tempfile.TemporaryDirectory(prefix='market-title-crc-') as directory:
                assets = fixture(Path(directory) / 'assets')
                path = assets / 'black-market-wordmark.png'
                data = bytearray(path.read_bytes())
                offset = data.index(kind)
                length = struct.unpack_from('>I', data, offset - 4)[0]
                data[offset + 4 + length] ^= 1
                path.write_bytes(data)
                refresh(assets, path.name)
                with self.assertRaisesRegex(ValueError, 'PNG.*CRC|CRC.*PNG'):
                    title.texture(assets)

    def test_png_signature_dimensions_and_complete_stream_are_required(self):
        title = self.title()
        for damage in ('signature', 'dimensions', 'truncated', 'trailing'):
            with self.subTest(damage=damage), tempfile.TemporaryDirectory(prefix='market-title-png-') as directory:
                assets = fixture(Path(directory) / 'assets')
                path = assets / 'black-market-wordmark.png'
                data = path.read_bytes()
                if damage == 'signature':
                    data = b'bad PNG!' + data[8:]
                elif damage == 'dimensions':
                    data = data[:8] + chunk(b'IHDR', struct.pack('>IIBBBBB', 191, HEIGHT, 8, 6, 0, 0, 0)) + data[33:]
                elif damage == 'truncated':
                    data = data[:-1]
                else:
                    data += b'extra'
                path.write_bytes(data)
                refresh(assets, path.name)
                with self.assertRaisesRegex(ValueError, 'PNG'):
                    title.texture(assets)

    def test_raw_hash_crc_length_and_nonblank_alpha_are_required(self):
        title = self.title()
        for damage in ('hash', 'crc', 'length', 'transparent', 'opaque', 'perimeter'):
            with self.subTest(damage=damage), tempfile.TemporaryDirectory(prefix='market-title-raw-') as directory:
                assets = fixture(Path(directory) / 'assets')
                path = assets / 'black-market-title.ia8'
                if damage in ('hash', 'crc'):
                    metadata_path = assets / 'provenance.json'
                    metadata = json.loads(metadata_path.read_text())
                    metadata['files'][path.name]['sha256' if damage == 'hash' else 'crc32'] = '0' * (64 if damage == 'hash' else 8)
                    metadata_path.write_text(json.dumps(metadata))
                elif damage == 'perimeter':
                    path.write_bytes(b'\xFF' + path.read_bytes()[1:])
                    refresh(assets, path.name)
                else:
                    path.write_bytes(path.read_bytes()[:-1] if damage == 'length' else
                                     bytes(WIDTH * HEIGHT) if damage == 'transparent' else b'\xFF' * (WIDTH * HEIGHT))
                    refresh(assets, path.name)
                with self.assertRaisesRegex(ValueError, 'IA8|raw|hash|CRC|alpha|size'):
                    title.texture(assets)

    def test_source_context_can_be_a_string_and_native_black_edges_remain_valid(self):
        title = self.title()
        with tempfile.TemporaryDirectory(prefix='market-title-outline-') as directory:
            assets = fixture(Path(directory) / 'assets')
            path = assets / 'black-market-title.ia8'
            raw = bytearray(path.read_bytes())
            raw[9 * WIDTH + 9] = 0x0F  # Native IA8 black outline, fully opaque alpha.
            path.write_bytes(raw)
            refresh(assets, path.name)
            metadata_path = assets / 'provenance.json'
            metadata = json.loads(metadata_path.read_text())
            metadata['source'] = 'Original checked-in test artwork'
            metadata_path.write_text(json.dumps(metadata))
            data = title.texture(assets)
            title.validate_texture(data)
            self.assertEqual(data[80:], bytes(raw))

    def test_invalid_provenance_and_referenced_original_fail_before_any_port_output(self):
        title, prepare = self.title(), self.preparation()
        real_texture = title.texture
        for damage in ('png_crc', 'raw', 'version', 'format', 'dimensions', 'authorship', 'source', 'path', 'original'):
            with self.subTest(damage=damage), tempfile.TemporaryDirectory(prefix='market-title-reject-') as directory:
                root = Path(directory)
                assets = fixture(root / 'assets')
                metadata_path = assets / 'provenance.json'
                metadata = json.loads(metadata_path.read_text())
                if damage == 'png_crc':
                    image = assets / 'black-market-wordmark.png'
                    data = bytearray(image.read_bytes())
                    data[-1] ^= 1
                    image.write_bytes(data)
                    metadata['files'][image.name] = record(data)
                elif damage == 'raw':
                    (assets / 'black-market-title.ia8').write_bytes(b'changed raw pixels')
                elif damage == 'version':
                    metadata['version'] = 2
                elif damage == 'format':
                    metadata['format'] = 'RGBA'
                elif damage == 'dimensions':
                    metadata['logical_width'] = 193
                elif damage == 'authorship':
                    metadata['authoring'] = ''
                elif damage == 'source':
                    metadata['source'] = {}
                elif damage == 'path':
                    metadata['files']['../outside.png'] = record(b'outside')
                else:
                    (assets / 'black-market-wordmark-original.png').write_bytes(b'corrupt original')
                metadata_path.write_text(json.dumps(metadata))
                output = root / 'output'
                # Inject a local asset input; actual validation and prepare run unchanged.
                with patch.object(title, 'texture', side_effect=lambda *args, **kwargs: real_texture(assets)):
                    with self.assertRaises(ValueError):
                        prepare.prepare(output)
                self.assertFalse(output.exists(), 'A rejected title must not leave either port partly prepared')

    def test_texture_validator_rejects_bad_native_records_and_blank_pixels(self):
        title = self.title()
        valid = title.texture()
        for damage in ('header', 'version', 'kind', 'width', 'height', 'count', 'truncated', 'extra', 'blank'):
            with self.subTest(damage=damage):
                data = bytearray(valid)
                if damage == 'header':
                    data[4:8] = b'BAD!'
                elif damage == 'version':
                    struct.pack_into('<I', data, 8, 2)
                elif damage in ('kind', 'width', 'height', 'count'):
                    offset = {'kind': 64, 'width': 68, 'height': 72, 'count': 76}[damage]
                    struct.pack_into('<I', data, offset, 0)
                elif damage == 'truncated':
                    data.pop()
                elif damage == 'extra':
                    data.append(0)
                else:
                    data[80:] = bytes(WIDTH * HEIGHT)
                with self.assertRaisesRegex(ValueError, 'texture|IA8|alpha|size|header'):
                    title.validate_texture(bytes(data))


if __name__ == '__main__':
    unittest.main()
