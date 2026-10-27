#!/usr/bin/env python3
"""Exercise verified dual-tree preparation and the real archive-only CMake gate."""
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import zipfile

from test_generator import COMBINED, MM, OOT, PREFIX, ROOT, fixture, load_module

PREPARE = ROOT / 'scripts/assets/prepare_native_forest.py'
CMAKE = ROOT / 'CMake/NativeForestAssets.cmake'


def load_prepare(test):
    test.assertTrue(PREPARE.is_file(), 'Verified dual-tree native forest preparation is missing')
    spec = importlib.util.spec_from_file_location('prepare_native_forest', PREPARE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def source_bundle(test, folder):
    generator = load_module(test)
    resources, report = generator.build_resources(*fixture(generator))
    bundle = folder / 'resources.zip'
    manifest = folder / 'manifest.json'
    metadata = generator.write_bundle(resources, report, bundle)
    manifest.write_text(json.dumps(metadata))
    return resources, manifest, bundle


def split_source_bundle(test, folder):
    resources, manifest, bundle = source_bundle(test, folder)
    payload = bundle.read_bytes()
    metadata = json.loads(manifest.read_text())
    width = (len(payload) + 2) // 3
    parts = []
    (folder / 'parts').mkdir()
    for number, offset in enumerate(range(0, len(payload), width), 1):
        name = f'parts/resources.zip.part{number:03d}'
        data = payload[offset:offset + width]
        (folder / name).write_bytes(data)
        parts.append({'name': name, 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()})
    metadata['bundle']['parts'] = parts
    manifest.write_text(json.dumps(metadata))
    bundle.unlink()
    return resources, manifest, metadata, payload


class SplitBundleTests(unittest.TestCase):
    def reader(self):
        path = ROOT / 'tools/black_market/native_forest_bundle.py'
        self.assertTrue(path.is_file(), 'Native forest needs the verified split-bundle reader')
        spec = importlib.util.spec_from_file_location('native_forest_bundle', path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module

    def test_tracked_parts_reproduce_the_exact_canonical_zip_without_a_cache(self):
        metadata = json.loads((ROOT / 'tools/native_forest/manifest.json').read_text())
        self.assertIn('parts', metadata['bundle'], 'The oversized canonical ZIP must be stored as bounded parts')
        self.assertEqual(metadata['bundle']['bytes'], 49704908)
        self.assertEqual(metadata['bundle']['sha256'],
                         'b0a33016b32d7e5415090038cff4cb8f4328cc8fd8a4b3f09d55918318be9fb5')
        source = ROOT / 'tools/native_forest'
        with tempfile.TemporaryDirectory(prefix='native-forest-source-only-') as name:
            directory = Path(name)
            (directory / 'parts').mkdir()
            (directory / 'manifest.json').write_text(json.dumps(metadata))
            for part in metadata['bundle']['parts']:
                shutil.copyfile(source / part['name'], directory / part['name'])
            self.assertFalse((directory / 'resources.zip').exists())
            payload = self.reader().load_bundle(directory)
        self.assertEqual(hashlib.sha256(payload).hexdigest(), metadata['bundle']['sha256'])
        self.assertEqual(len(payload), metadata['bundle']['bytes'])
        for part in metadata['bundle']['parts']:
            self.assertLessEqual(part['bytes'], 8 * 1024 * 1024)

    def test_split_bundle_prepares_identical_resources_and_supports_the_shared_reader(self):
        prepare = load_prepare(self)
        with tempfile.TemporaryDirectory(prefix='native-forest-split-') as name:
            directory = Path(name)
            expected, manifest, _, payload = split_source_bundle(self, directory)
            self.assertEqual(self.reader().load_bundle(directory), payload)
            prepare.prepare(directory / 'output', manifest)
            for game in ('soh', 'mm'):
                root = directory / 'output' / game / 'assets/custom'
                self.assertEqual({path.relative_to(root).as_posix(): path.read_bytes()
                                  for path in root.rglob('*') if path.is_file()}, expected)

    def test_invalid_parts_fail_before_any_existing_output_is_changed(self):
        prepare = load_prepare(self)
        for damage in ('missing', 'corrupt', 'reordered', 'part-hash', 'part-size', 'aggregate-hash', 'aggregate-size',
                       'part-too-large', 'unsafe-path'):
            with self.subTest(damage=damage), tempfile.TemporaryDirectory(prefix='native-forest-bad-parts-') as name:
                directory = Path(name)
                _, manifest, metadata, payload = split_source_bundle(self, directory)
                # Even an exact local cache must not hide a broken tracked part.
                (directory / 'resources.zip').write_bytes(payload)
                output = directory / 'output'
                sentinel = output / 'soh/assets/custom/retained'
                sentinel.parent.mkdir(parents=True)
                sentinel.write_bytes(b'previous verified output')
                part = metadata['bundle']['parts'][0]
                if damage == 'missing':
                    (directory / part['name']).unlink()
                elif damage == 'corrupt':
                    raw = bytearray((directory / part['name']).read_bytes())
                    raw[0] ^= 1
                    (directory / part['name']).write_bytes(raw)
                elif damage == 'reordered':
                    metadata['bundle']['parts'].reverse()
                elif damage == 'part-hash':
                    part['sha256'] = '0' * 64
                elif damage == 'part-size':
                    part['bytes'] += 1
                    metadata['bundle']['parts'][1]['bytes'] -= 1
                elif damage == 'aggregate-hash':
                    metadata['bundle']['sha256'] = '0' * 64
                elif damage == 'aggregate-size':
                    metadata['bundle']['bytes'] += 1
                elif damage == 'part-too-large':
                    part['bytes'] = 8 * 1024 * 1024 + 1
                else:
                    part['name'] = '../protected'
                manifest.write_text(json.dumps(metadata))
                with self.assertRaisesRegex(ValueError, '(part|bundle).*(missing|hash|size|order|path|limit)|Missing.*part'):
                    prepare.prepare(output, manifest)
                self.assertEqual({path.relative_to(output).as_posix(): path.read_bytes()
                                  for path in output.rglob('*') if path.is_file()},
                                 {'soh/assets/custom/retained': b'previous verified output'})
                self.assertFalse((output / 'mm').exists())

    def test_legacy_single_zip_and_explicit_canonical_override_remain_supported(self):
        prepare = load_prepare(self)
        with tempfile.TemporaryDirectory(prefix='native-forest-single-') as name:
            directory = Path(name)
            expected, manifest, bundle = source_bundle(self, directory)
            self.assertEqual(self.reader().load_bundle(directory), bundle.read_bytes())
            self.assertEqual(prepare.verified_resources(manifest, bundle), expected)


class PreparationTests(unittest.TestCase):
    def test_verified_native_geometry_prepares_identical_trees_and_repairs_only_stale_files(self):
        module = load_prepare(self)
        with tempfile.TemporaryDirectory(prefix='native-forest-prepare-') as name:
            directory = Path(name)
            resources, manifest, _ = source_bundle(self, directory)
            output = directory / 'source'
            module.prepare(output, manifest)
            paths = [output / game / 'assets/custom' / key for game in ('soh', 'mm') for key in resources]
            before = {path: path.stat().st_mtime_ns for path in paths}
            for game in ('soh', 'mm'):
                for key, data in resources.items():
                    self.assertEqual((output / game / 'assets/custom' / key).read_bytes(), data)
            module.prepare(output, manifest)
            self.assertEqual(before, {path: path.stat().st_mtime_ns for path in paths})
            damaged = paths[0]
            damaged.write_bytes(b'broken geometry')
            module.prepare(output, manifest)
            for game in ('soh', 'mm'):
                root = output / game / 'assets/custom'
                self.assertEqual({path.relative_to(root).as_posix(): path.read_bytes()
                                  for path in root.rglob('*') if path.is_file()}, resources)
            self.assertTrue(all(path == damaged or before[path] == path.stat().st_mtime_ns for path in paths))

    def test_bundle_corruption_and_missing_real_donor_fail_before_any_output_write(self):
        module = load_prepare(self)
        with tempfile.TemporaryDirectory(prefix='native-forest-invalid-') as name:
            directory = Path(name)
            _, manifest, bundle = source_bundle(self, directory)
            damaged = bytearray(bundle.read_bytes())
            damaged[len(damaged) // 2] ^= 1
            bundle.write_bytes(damaged)
            output = directory / 'source'
            with self.assertRaisesRegex(ValueError, 'bundle.*(hash|SHA)'):
                module.prepare(output, manifest)
            self.assertFalse(output.exists())
            manifest.write_text(json.dumps({'schema': 1, 'status': 'blocked_missing_native_mm',
                                            'required_native_mm_sha256': 'f10167'}))
            with self.assertRaisesRegex(ValueError, 'native MM|native-mm'):
                module.prepare(output, manifest)
            self.assertFalse(output.exists())

    def test_resource_hash_and_texture_exclusion_are_checked_beyond_the_zip_hash(self):
        module = load_prepare(self)
        with tempfile.TemporaryDirectory(prefix='native-forest-members-') as name:
            directory = Path(name)
            resources, manifest, bundle = source_bundle(self, directory)
            metadata = json.loads(manifest.read_text())
            first = next(iter(resources))
            with zipfile.ZipFile(bundle, 'w') as archive:
                for key, data in resources.items():
                    archive.writestr(key, b'changed bytes' if key == first else data)
            metadata['bundle']['sha256'] = hashlib.sha256(bundle.read_bytes()).hexdigest()
            metadata['bundle']['bytes'] = bundle.stat().st_size
            manifest.write_text(json.dumps(metadata))
            with self.assertRaisesRegex(ValueError, 'resource.*(hash|SHA|length|size)'):
                module.prepare(directory / 'output', manifest)
            key, data = PREFIX + 'forbidden_image', bytes(4) + b'XETO' + bytes(60)
            resources[key] = data
            with zipfile.ZipFile(bundle, 'w') as archive:
                for key, data in resources.items():
                    archive.writestr(key, data)
            metadata['bundle'].update(sha256=hashlib.sha256(bundle.read_bytes()).hexdigest(), bytes=bundle.stat().st_size)
            metadata['resources'][key] = {'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data)}
            manifest.write_text(json.dumps(metadata))
            with self.assertRaisesRegex(ValueError, 'texture'):
                module.prepare(directory / 'output', manifest)
            self.assertFalse((directory / 'output').exists())

    def test_resource_contracts_include_only_each_engines_headers_and_common_stock_dependencies(self):
        module = load_module(self)
        resources, receipt = module.build_resources(*fixture(module))
        text = module.native_resource_header(resources, receipt)
        mm, oot = text.split('kNativeForestOotResources')
        self.assertIn('kNativeForestMmResources', mm)
        self.assertIn(COMBINED, mm)
        self.assertIn(MM, mm)
        self.assertNotIn(OOT, mm)
        self.assertIn(OOT, oot)
        self.assertNotIn(COMBINED, oot)
        self.assertNotIn(MM, oot)
        for path in receipt['external_resources']:
            self.assertIn('"' + path + '"', mm)
            self.assertIn('"' + path + '"', oot)

    def test_real_cmake_gate_blocks_only_archive_generation_until_a_verified_bundle_exists(self):
        self.assertTrue(CMAKE.is_file(), 'Native archive preparation dependency is missing')
        self.assertTrue(PREPARE.is_file())
        with tempfile.TemporaryDirectory(prefix='native-forest-cmake-') as name:
            directory = Path(name)
            source, build = directory / 'source', directory / 'build'
            source.mkdir()
            manifest = directory / 'manifest.json'
            manifest.write_text(json.dumps({'schema': 1, 'status': 'blocked_missing_native_mm'}))
            (source / 'ready.py').write_text(
                'from pathlib import Path\n'
                "assert (Path(__file__).parent/'soh/assets/custom/custom/comboship/native_forest/materials_v1').is_file()\n"
                "assert (Path(__file__).parent/'mm/assets/custom/custom/comboship/native_forest/materials_v1').is_file()\n")
            (source / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.22)\nproject(NativeForestPackaging NONE)\n'
                'find_package(Python3 REQUIRED COMPONENTS Interpreter)\n'
                'add_custom_target(CheckAssetCollisions)\n'
                'add_custom_target(GenerateSohOtr COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/ready.py")\n'
                'add_custom_target(Generate2ShipOtr COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/ready.py")\n'
                f'set(COMBO_NATIVE_FOREST_MANIFEST "{manifest.as_posix()}")\n'
                f'include("{CMAKE.as_posix()}")\n')
            configured = subprocess.run(['cmake', '-S', str(source), '-B', str(build)], capture_output=True, text=True)
            self.assertEqual(configured.returncode, 0, configured.stdout + configured.stderr)
            ordinary = subprocess.run(['cmake', '--build', str(build), '--target', 'CheckAssetCollisions'], capture_output=True, text=True)
            self.assertEqual(ordinary.returncode, 0, ordinary.stdout + ordinary.stderr)
            blocked = subprocess.run(['cmake', '--build', str(build), '--target', 'GenerateSohOtr'], capture_output=True, text=True)
            self.assertNotEqual(blocked.returncode, 0)
            self.assertIn('native MM', blocked.stdout + blocked.stderr)
            self.assertFalse((source / 'soh').exists())
            resources, _, _ = source_bundle(self, directory)
            for target in ('GenerateSohOtr', 'Generate2ShipOtr'):
                result = subprocess.run(['cmake', '--build', str(build), '--target', target], capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            for game in ('soh', 'mm'):
                for path, data in resources.items():
                    self.assertEqual((source / game / 'assets/custom' / path).read_bytes(), data)


if __name__ == '__main__':
    unittest.main()
