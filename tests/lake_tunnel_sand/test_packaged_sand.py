"""The source package must own the dedicated surface, without a scene mod."""
from pathlib import Path
import base64
import importlib.util
import json
import struct
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
PATH = 'textures/comboship/regional/lake_hylia/sand'
PREPARE = ROOT / 'scripts/assets/prepare_lake_tunnel_sand.py'


class PackagedSandTests(unittest.TestCase):
    def test_source_payload_uses_bounded_base64_text_parts(self):
        module = self.prepare_module()
        manifest = json.loads(module.DEFAULT_MANIFEST.read_text())
        self.assertEqual(manifest['bundle'].get('encoding'), 'zlib-base64-pool')
        bundle = module.DEFAULT_MANIFEST.parent / manifest['bundle']['directory']
        for part in manifest['bundle']['parts']:
            data = (bundle / part['name']).read_bytes()
            self.assertLessEqual(len(data), 131072)
            self.assertTrue(part['name'].endswith('.b64'))
            self.assertLessEqual(len(base64.b64decode(data, validate=True)), 98304)

    def prepare_module(self):
        self.assertTrue(PREPARE.is_file(), 'normal source build has no dedicated sand preparation')
        spec = importlib.util.spec_from_file_location('prepare_lake_tunnel_sand', PREPARE)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module

    def test_default_package_has_same_dedicated_sand_in_native_and_alt_paths(self):
        module = self.prepare_module()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            module.prepare(root)
            resources = sorted(p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file())
            self.assertEqual(resources, ['soh/assets/custom/alt/' + PATH, 'soh/assets/custom/' + PATH])
            native = (root / 'soh/assets/custom' / PATH).read_bytes()
            self.assertEqual(native, (root / 'soh/assets/custom/alt' / PATH).read_bytes())
            typ, width, height, flags, horizontal, vertical, size = struct.unpack_from('<IIIIffI', native, 64)
            self.assertEqual((typ, flags), (2, 1))
            self.assertEqual((width * 4 / horizontal, height / vertical), (32 * 2, 32))
            self.assertEqual(size, width * height * 4)
            self.assertEqual(len(native), 92 + size)

    def test_preparation_preserves_scene_collision_water_and_grass_atlas(self):
        module = self.prepare_module()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            protected = root / 'soh/assets/custom/scenes/shared/spot06_scene/spot06_sceneTex_00FEB8'
            protected.parent.mkdir(parents=True)
            protected.write_bytes(b'untouched-grass-trim-atlas')
            module.prepare(root)
            self.assertEqual(protected.read_bytes(), b'untouched-grass-trim-atlas')
            self.assertEqual(len([p for p in root.rglob('*') if p.is_file()]), 3)

    def test_corrupt_source_does_not_partially_replace_installed_sand(self):
        module = self.prepare_module()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            output = root / 'output'
            module.prepare(output)
            native = output / 'soh/assets/custom' / PATH
            expected = native.read_bytes()
            manifest = json.loads(module.DEFAULT_MANIFEST.read_text())
            manifest['resources'][PATH]['sha256'] = '0' * 64
            (root / 'manifest.json').write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, 'hash'):
                module.prepare(output, root / 'manifest.json', source_directory=module.DEFAULT_MANIFEST.parent)
            self.assertEqual(native.read_bytes(), expected)


if __name__ == '__main__':
    unittest.main()
