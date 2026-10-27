"""Verify actual spin bytes survive bounded publication and archive preparation."""
import importlib.util
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
BUNDLE = ROOT / 'tools/spin_styles/bundled'


def loader():
    path = ROOT / 'scripts/assets/prepare_spin_styles.py'
    assert path.is_file(), 'Spin resources need a lossless bounded source-bundle loader'
    spec = importlib.util.spec_from_file_location('prepare_spin_styles', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class BundledSpinAssets(unittest.TestCase):
    def test_real_pool_restores_both_hosts_repairs_corruption_and_is_idempotent(self):
        module = loader()
        expected = json.loads((ROOT / 'tools/spin_styles/manifest.json').read_text())['outputs']
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary)
            count = sum(len(entries) for entries in expected.values())
            self.assertEqual(module.prepare(output), count)
            for host, entries in expected.items():
                for name, digest in entries.items():
                    path = output / host / 'assets/custom' / name
                    self.assertEqual(hashlib.sha256(path.read_bytes()).hexdigest(), digest, str(path))
            self.assertEqual(module.prepare(output), 0)
            witness = output / 'soh/assets/custom/objects/combo_spin/fire/burst0'
            witness.write_bytes(b'broken')
            self.assertEqual(module.prepare(output), 1)
            self.assertEqual(hashlib.sha256(witness.read_bytes()).hexdigest(), expected['soh']['objects/combo_spin/fire/burst0'])
        for part in BUNDLE.glob('*.b64'):
            self.assertLessEqual(part.stat().st_size, 128 * 1024)

    def test_corrupt_part_is_rejected_before_any_output_is_written(self):
        module = loader()
        manifest = json.loads((BUNDLE / 'manifest.json').read_text())
        with tempfile.TemporaryDirectory() as temporary:
            bundle = Path(temporary) / 'bundle'
            bundle.mkdir()
            (bundle / 'manifest.json').write_text(json.dumps(manifest))
            name = manifest['parts'][0]['name']
            part = bytearray((BUNDLE / name).read_bytes())
            part[0] = ord('A') if part[0] != ord('A') else ord('B')
            (bundle / name).write_bytes(part)
            output = Path(temporary) / 'output'
            with self.assertRaisesRegex(ValueError, 'part'):
                module.prepare(output, bundle)
            self.assertFalse(output.exists())

    def test_oversized_decode_is_rejected_before_reading_parts(self):
        module = loader()
        manifest = json.loads((BUNDLE / 'manifest.json').read_text())
        manifest['raw_bytes'] = 64 * 1024 * 1024 + 1
        with tempfile.TemporaryDirectory() as temporary:
            bundle = Path(temporary) / 'bundle'
            bundle.mkdir()
            (bundle / 'manifest.json').write_text(json.dumps(manifest))
            output = Path(temporary) / 'output'
            with self.assertRaisesRegex(ValueError, 'limit'):
                module.prepare(output, bundle)
            self.assertFalse(output.exists())

    @unittest.skipUnless(shutil.which('cmake'), 'CMake is required for the real asset preparation target')
    def test_real_cmake_preparation_runs_before_asset_consumer(self):
        with tempfile.TemporaryDirectory() as temporary:
            project = Path(temporary)
            output = project / 'output'
            check = project / 'check.py'
            check.write_text('import hashlib,json,sys\nfrom pathlib import Path\n'
                             'root=Path(sys.argv[1])\n'
                             'for host,entries in json.loads(Path(sys.argv[2]).read_text())["outputs"].items():\n'
                             ' for name,digest in entries.items():\n'
                             '  assert hashlib.sha256((root/host/"assets/custom"/name).read_bytes()).hexdigest()==digest\n')
            (project / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.16)\nproject(SpinBundle NONE)\n'
                f'set(Python3_EXECUTABLE "{sys.executable}")\n'
                f'set(COMBO_SPIN_STYLE_OUTPUT_ROOT "{output}")\n'
                f'include("{ROOT}/CMake/SpinStyleAssets.cmake")\n'
                f'add_custom_target(AssetConsumer COMMAND "{sys.executable}" "{check}" "{output}" '
                f'"{ROOT}/tools/spin_styles/manifest.json")\n'
                'add_dependencies(AssetConsumer PrepareSpinStyleAssets)\n')
            subprocess.run(['cmake', '-S', str(project), '-B', str(project / 'build'), '-G', 'Ninja'], check=True)
            subprocess.run(['cmake', '--build', str(project / 'build'), '--target', 'AssetConsumer'], check=True)


if __name__ == '__main__':
    unittest.main()
