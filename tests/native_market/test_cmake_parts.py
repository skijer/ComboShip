"""The ordinary asset target must consume the checked-in split bundles."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class NativeMarketBuild(unittest.TestCase):
    def test_real_asset_target_builds_with_split_texture_sources(self):
        cmake = shutil.which('cmake')
        if not cmake:
            self.skipTest('CMake is required for the native asset-target integration test')
        with tempfile.TemporaryDirectory(prefix='market-cmake-parts-') as directory:
            source = Path(directory)
            script = (ROOT / 'CMake/NativeMarketAssets.cmake').as_posix()
            python = Path(sys.executable).as_posix()
            (source / 'CMakeLists.txt').write_text(
                'cmake_minimum_required(VERSION 3.12)\n'
                'project(NativeMarketParts NONE)\n'
                f'set(Python3_EXECUTABLE "{python}")\n'
                f'include("{script}")\n')
            build = source / 'build'
            configured = subprocess.run([cmake, '-S', str(source), '-B', str(build)],
                                        text=True, capture_output=True)
            self.assertEqual(configured.returncode, 0, configured.stdout + configured.stderr)
            prepared = subprocess.run([cmake, '--build', str(build), '--target', 'PrepareNativeMarketAssets'],
                                      text=True, capture_output=True)
            self.assertEqual(prepared.returncode, 0, prepared.stdout + prepared.stderr)
            self.assertIn('141 native market resources prepared', prepared.stdout)


if __name__ == '__main__':
    unittest.main()
