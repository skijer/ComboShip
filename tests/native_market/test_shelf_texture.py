"""The animated fixture must use its original art, independently of stone floors."""
import base64
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
import market_interior as interior

PIXELS_SHA = '3e2bac601d1a9c3e0317d159aa6b9f5354abaac982a24848edfec5e97dbb3231'


class ShelfTexture(unittest.TestCase):
    def tearDown(self):
        interior.shelf_pixels.cache_clear()

    def test_original_art_pixels_and_valid_native_framing_without_other_bundles(self):
        interior.shelf_pixels.cache_clear()
        with patch.object(interior, 'encoded_pixels', side_effect=AssertionError('floor/prop bundle read')):
            data = interior.texture('arcane_stone', 64)
        self.assertEqual(data[4:8], b'XETO')
        self.assertEqual(data[1], 1)
        self.assertEqual(struct.unpack_from('<IIIIffI', data, 64), (1,512,512,1,16.,8.,1048576))
        self.assertEqual(hashlib.sha256(data[92:]).hexdigest(), PIXELS_SHA)
        self.assertEqual(len(data), 92 + 1048576)
        self.assertEqual(set(data[95::4]), {255})

    def fixture(self, directory):
        root = Path(directory)
        manifest = json.loads((interior.ASSETS / 'shelf-encoded.json').read_text())
        for part in manifest['parts']:
            text = (interior.ASSETS / part['name']).read_bytes()
            self.assertLessEqual(len(text), 128 * 1024)
            self.assertLessEqual(part['bytes'], 96 * 1024)
            (root / part['name']).write_bytes(text)
        return root, manifest

    def reject(self, root, manifest):
        (root / 'shelf-encoded.json').write_text(json.dumps(manifest))
        interior.shelf_pixels.cache_clear()
        with patch.object(interior, 'ASSETS', root), self.assertRaises(ValueError):
            interior.shelf_pixels()

    def test_foreign_part_path_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root, manifest = self.fixture(directory)
            manifest['parts'][0]['name'] = '../outside.b64'
            self.reject(root, manifest)

    def test_bad_part_and_oversized_text_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root, manifest = self.fixture(directory)
            name = manifest['parts'][0]['name']
            (root / name).write_bytes(b'AAAA')
            self.reject(root, manifest)
            (root / name).write_bytes(b'A' * (128 * 1024 + 4))
            self.reject(root, manifest)

    def test_trailing_zlib_data_rejected_after_valid_hashes(self):
        with tempfile.TemporaryDirectory() as directory:
            root, manifest = self.fixture(directory)
            packed = b''.join(base64.b64decode((root / p['name']).read_bytes()) for p in manifest['parts'])
            last = manifest['parts'][-1]
            data = base64.b64decode((root / last['name']).read_bytes()) + b'junk'
            (root / last['name']).write_bytes(base64.b64encode(data))
            last['bytes'], last['sha256'] = len(data), hashlib.sha256(data).hexdigest()
            manifest['packed_bytes'] += 4
            manifest['packed_sha256'] = hashlib.sha256(packed + b'junk').hexdigest()
            self.reject(root, manifest)


if __name__ == '__main__':
    unittest.main()
