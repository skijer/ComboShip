"""Exact source bytes must survive the bounded morning publication workflow."""
import base64
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
REFERENCE = json.loads(Path(__file__).with_name('reference_hashes.json').read_text())['files']


def loader():
    path = ROOT / 'scripts/assets/prepare_cumulative_assets.py'
    if not path.is_file():
        raise AssertionError('normal source build lacks bounded cumulative asset preparation')
    spec = importlib.util.spec_from_file_location('prepare_cumulative_assets', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class EncodedAssets(unittest.TestCase):
    def test_public_build_metadata_omits_private_input_handles(self):
        paths = ('docs/integration/20261026/assets.json',
                 'tools/temple_sages_bundle/manifest.json',
                 'tools/temple_sages/assets/chamber_hd/provenance.json',
                 'tools/lake_tunnel_sand/manifest.json',
                 'tools/magic_mirror/reflection/forest-scene.json')
        private_fields = ('libfile_', 'library_file_id', 'native_donor_sha256',
                          'map_donor_sha256', 'source_library_file_id')
        for name in paths:
            with self.subTest(path=name):
                text = (ROOT / name).read_text()
                self.assertTrue(all(token not in text for token in private_fields), name)

    def test_exact_bytes_restore_repair_and_repeat_without_changes(self):
        module = loader()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            self.assertEqual(module.prepare(root), 18)
            for entry in REFERENCE:
                data = (root / entry['path']).read_bytes()
                self.assertEqual(len(data), entry['bytes'], entry['path'])
                self.assertEqual(hashlib.sha256(data).hexdigest(), entry['sha256'], entry['path'])
            self.assertEqual(module.prepare(root), 0)
            witness = root / 'soh/assets/custom/objects/magic_mirror/forest_seal'
            witness.write_bytes(b'damaged')
            self.assertEqual(module.prepare(root), 1)
            self.assertEqual(hashlib.sha256(witness.read_bytes()).hexdigest(),
                             '090aa134f050e55e7a8f49d9461c1c506dc08fb09e4058dc4fc0e174616576d8')

    def test_only_bounded_base64_parts_carry_asset_payloads(self):
        module = loader()
        manifest = json.loads((module.BUNDLE / 'manifest.json').read_text())
        self.assertEqual(manifest['encoding'], 'zlib-base64-pool')
        for part in manifest['parts']:
            self.assertTrue(part['name'].endswith('.b64'))
            data = (module.BUNDLE / part['name']).read_bytes()
            self.assertLessEqual(len(data), 131072)
            decoded = base64.b64decode(data, validate=True)
            self.assertLessEqual(len(decoded), 98304)
        for path in module.BUNDLE.iterdir():
            if path.is_file():
                self.assertTrue(path.name == 'manifest.json' or path.suffix == '.b64', path.name)

    def test_corrupt_or_reordered_parts_cannot_change_installed_output(self):
        module = loader()
        manifest = json.loads((module.BUNDLE / 'manifest.json').read_text())
        for reorder in (False, True):
            with self.subTest(reorder=reorder), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                bundle = root / 'bundle'
                bundle.mkdir()
                candidate = json.loads(json.dumps(manifest))
                if reorder:
                    candidate['parts'][0], candidate['parts'][1] = candidate['parts'][1], candidate['parts'][0]
                else:
                    name = candidate['parts'][0]['name']
                    data = bytearray((module.BUNDLE / name).read_bytes())
                    data[0] = ord('A') if data[0] != ord('A') else ord('B')
                    (bundle / name).write_bytes(data)
                (bundle / 'manifest.json').write_text(json.dumps(candidate))
                output = root / 'output'
                witness = output / 'soh/assets/custom/objects/magic_mirror/forest_seal'
                witness.parent.mkdir(parents=True)
                witness.write_bytes(b'installed')
                with self.assertRaisesRegex(ValueError, 'part'):
                    module.prepare(output, bundle)
                self.assertEqual(witness.read_bytes(), b'installed')
                self.assertEqual(len([p for p in output.rglob('*') if p.is_file()]), 1)

    def test_invalid_bounds_and_output_paths_fail_before_reading_parts(self):
        module = loader()
        manifest = json.loads((module.BUNDLE / 'manifest.json').read_text())
        for change in ('oversized', 'traversal', 'unlisted'):
            with self.subTest(change=change), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                candidate = json.loads(json.dumps(manifest))
                if change == 'oversized':
                    candidate['raw_bytes'] = 64 * 1024 * 1024 + 1
                elif change == 'traversal':
                    candidate['files'][0]['path'] = '../outside'
                else:
                    candidate['files'][0]['path'] = 'soh/assets/custom/objects/unrelated'
                (root / 'manifest.json').write_text(json.dumps(candidate))
                with self.assertRaises(ValueError):
                    module.prepare(root / 'output', root)
                self.assertFalse((root / 'output').exists())

    def test_corrupt_morning_override_preserves_all_installed_files(self):
        module = loader()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            overlay = root / 'overlay'
            shutil.copytree(module.OVERLAY, overlay)
            manifest = json.loads((overlay / 'manifest.json').read_text())
            part = overlay / manifest['parts'][0]['name']
            data = bytearray(part.read_bytes())
            data[0] = ord('A') if data[0] != ord('A') else ord('B')
            part.write_bytes(data)
            output = root / 'output'
            witness = output / 'soh/assets/custom/objects/magic_mirror/forest_seal'
            witness.parent.mkdir(parents=True)
            witness.write_bytes(b'installed')
            with self.assertRaisesRegex(ValueError, 'part'):
                module.prepare(output, overlay=overlay)
            self.assertEqual(witness.read_bytes(), b'installed')
            self.assertEqual(len([p for p in output.rglob('*') if p.is_file()]), 1)

    def test_cmake_target_prepares_exact_assets_before_archive_target(self):
        module = loader()
        cmake = shutil.which('cmake')
        fallback = ROOT.parent / 'deps/cmake/cmake/data/bin/cmake'
        if not cmake and fallback.is_file():
            cmake = str(fallback)
        self.assertIsNotNone(cmake, 'CMake is required to check the real preparation target')
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            checker = root / 'check.py'
            checker.write_text('from pathlib import Path\n'
                               'import hashlib\n'
                               'p=Path(__file__).parent/"soh/assets/custom/objects/magic_mirror/forest_seal"\n'
                               'assert hashlib.sha256(p.read_bytes()).hexdigest()=="090aa134f050e55e7a8f49d9461c1c506dc08fb09e4058dc4fc0e174616576d8"\n')
            root.joinpath('CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.20)\nproject(EncodedAssets NONE)\n'
                'find_package(Python3 REQUIRED COMPONENTS Interpreter)\n'
                'add_custom_target(GenerateSohOtr COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/check.py")\n'
                'add_custom_target(Generate2ShipOtr)\n'
                f'include("{ROOT / "CMake/CumulativeAssets.cmake"}")\n')
            subprocess.run([cmake, '-S', str(root), '-B', str(root / 'build')], check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
            subprocess.run([cmake, '--build', str(root / 'build'), '--target', 'GenerateSohOtr'], check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
            self.assertEqual(module.prepare(root), 0)


if __name__ == '__main__':
    unittest.main()
