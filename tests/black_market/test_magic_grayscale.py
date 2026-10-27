"""Neutral BM magic bases preserve source detail and unrelated prepared art."""
import hashlib
import importlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / 'tools/black_market'), str(ROOT / 'scripts/assets')]
import market_interior
import prepare_native_market
import room_magic

SHELF = 'custom/skiejer_market/texture/arcane_stone'
ROOM = 'custom/skiejer_market/room_magic_v1/texture/arcane_stone'
PIXELS_SHA = '08fc11578f90b1b834b6fb026bf6defbad4374a0013a448d3fcc7822df355237'
# Prepared resource inventories captured before this bounded two-texture change.
NONMAGIC_SHA = {
    'oot': '8e5a62f28b037a55e849ea58b8f0b4039fead95bbc345ce82b0f9c0e86e076ab',
    'mm': '44ac84a1ba05eada7f828ce69f94fff6eadb41f0068aa213de5f1bfd2e9c45cd',
}


class MagicGrayscale(unittest.TestCase):
    def test_each_pixel_retains_rec709_code_value_luminance_and_alpha(self):
        self.assertTrue(hasattr(market_interior, 'grayscale_magic_texture'),
                        'neutral Black Market magic upload helper is missing')
        original = market_interior.texture('arcane_stone', 64)
        neutral = market_interior.grayscale_magic_texture()
        self.assertEqual(neutral[:92], original[:92])
        self.assertEqual(struct.unpack_from('<IIIIffI', neutral, 64),
                         (1,512,512,1,16.,8.,1048576))
        self.assertEqual(len(neutral), len(original))
        for offset in range(92, len(original), 4):
            red, green, blue, alpha = original[offset:offset+4]
            # Integer Rec.709 coefficients on existing encoded RGB values;
            # no exposure/contrast adjustment, resampling or alpha conversion.
            expected = (2126*red + 7152*green + 722*blue + 5000) // 10000
            if neutral[offset:offset+4] != bytes((expected, expected, expected, alpha)):
                self.fail('luminance or alpha changed at pixel ' + str((offset-92)//4))
        self.assertEqual(hashlib.sha256(neutral[92:]).hexdigest(), PIXELS_SHA)

    def test_colored_source_api_used_by_temple_is_unchanged(self):
        original = market_interior.texture('arcane_stone', 64)
        self.assertEqual(hashlib.sha256(original[92:]).hexdigest(),
                         '3e2bac601d1a9c3e0317d159aa6b9f5354abaac982a24848edfec5e97dbb3231')
        self.assertEqual(original[92:], market_interior.shelf_pixels())

    def test_both_host_exports_change_only_the_two_magic_texture_payloads(self):
        for host in ('oot', 'mm'):
            with self.subTest(host=host):
                resources = prepare_native_market.prepared_resources(host=host)
                for name in (SHELF, ROOM):
                    self.assertEqual(hashlib.sha256(resources[name][92:]).hexdigest(), PIXELS_SHA)
                unchanged = {name:hashlib.sha256(data).hexdigest()
                             for name, data in resources.items() if name not in (SHELF, ROOM)}
                digest = hashlib.sha256(json.dumps(unchanged, sort_keys=True,
                                                   separators=(',',':')).encode()).hexdigest()
                self.assertEqual(digest, NONMAGIC_SHA[host], 'nonmagic resource changed')

    def test_override_is_four_closed_deterministic_texture_entries(self):
        path = ROOT / 'tools/black_market/build_magic_grayscale.py'
        self.assertTrue(path.exists(), 'standalone grayscale override builder is missing')
        builder = importlib.import_module('build_magic_grayscale')
        with tempfile.TemporaryDirectory() as directory:
            first, second = (Path(directory) / name for name in ('one.o2r', 'two.o2r'))
            receipt = builder.build(first)
            builder.build(second)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            self.assertEqual(receipt['sha256'], hashlib.sha256(first.read_bytes()).hexdigest())
            with zipfile.ZipFile(first) as archive:
                self.assertEqual(set(archive.namelist()), {SHELF, ROOM, 'alt/'+SHELF, 'alt/'+ROOM})
                for name in archive.namelist():
                    self.assertEqual(archive.read(name), room_magic.texture())
                    self.assertEqual(archive.getinfo(name).date_time, (1980,1,1,0,0,0))
            cli = subprocess.run([sys.executable, '-S', str(path), '--output', str(second)],
                                 text=True, capture_output=True)
            self.assertEqual(cli.returncode, 0, cli.stderr)
            self.assertEqual(first.read_bytes(), second.read_bytes())


if __name__ == '__main__':
    unittest.main()
