"""Exercise authoring and stdlib decoding of small, verifiable texture parts."""
import base64
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
import market_interior as interior


class TextureBundles(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        try:
            from PIL import Image
        except ImportError as error:
            raise unittest.SkipTest('Pillow is required for optional texture authoring fixtures') from error
        cls.sources, cls.pixels = {}, {}
        for name in (*interior.BASE_NAMES, *interior.PROP_NAMES):
            rgb = hashlib.shake_256(name.encode()).digest(512 * 512 * 3)
            image = Image.frombytes('RGB', (512, 512), rgb)
            stream = io.BytesIO()
            image.save(stream, format='PNG', compress_level=0)
            cls.sources[name] = stream.getvalue()
            cls.pixels[name] = image.convert('RGBA').tobytes()

    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix='market-parts-')
        self.addCleanup(temporary.cleanup)
        self.assets = Path(temporary.name)
        hashes = {}
        for name, data in self.sources.items():
            (self.assets / (name + '.png')).write_bytes(data)
            hashes[name] = hashlib.sha256(data).hexdigest()
        for name, value in (('ASSETS', self.assets), ('SOURCE_HASHES', hashes)):
            patcher = patch.object(interior, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)
        interior.encoded_pixels.cache_clear()
        self.addCleanup(interior.encoded_pixels.cache_clear)

    def bake(self):
        interior.rebake()
        path = self.assets / 'encoded.json'
        self.assertTrue(path.is_file(), 'Authoring must recreate the missing architectural bundle too')
        return json.loads(path.read_text())

    def test_round_trip_recreates_both_bundles_in_small_parts(self):
        architecture = self.bake()
        props = json.loads((self.assets / 'props-encoded.json').read_text())
        for manifest, stem in ((architecture, 'textures'), (props, 'props')):
            self.assertGreater(len(manifest['parts']), 1)
            for index, part in enumerate(manifest['parts'], 1):
                self.assertEqual(part['name'], f'{stem}.zlib.{index:03}.b64')
                self.assertLessEqual((self.assets / part['name']).stat().st_size, 128 * 1024)
        decoded = interior.encoded_pixels()
        self.assertEqual(set(decoded), set(self.pixels) | set(interior.DERIVED_NAMES))
        for name, pixels in self.pixels.items():
            self.assertEqual(decoded[name], pixels, name)

    def test_corrupt_compressed_part_is_rejected(self):
        manifest = self.bake()
        path = self.assets / manifest['parts'][0]['name']
        data = bytearray(base64.b64decode(path.read_bytes()))
        data[len(data) // 2] ^= 1
        path.write_bytes(base64.b64encode(data))
        with self.assertRaisesRegex(ValueError, 'part.*(hash|size)|(hash|size).*part'):
            interior.encoded_pixels()

    def test_manifest_cannot_select_a_foreign_part_path(self):
        manifest = self.bake()
        manifest['parts'][0]['name'] = '../outside.b64'
        (self.assets / 'encoded.json').write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'order|path'):
            interior.encoded_pixels()

    def test_oversized_part_is_rejected_before_decoding(self):
        manifest = self.bake()
        path = self.assets / manifest['parts'][0]['name']
        path.write_bytes(path.read_bytes() + b'AAAA')
        with self.assertRaisesRegex(ValueError, 'publication limit'):
            interior.encoded_pixels()


if __name__ == '__main__':
    unittest.main()
