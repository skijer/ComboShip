#!/usr/bin/env python3
"""Load actual prepared materials through the native XML and binary factories."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/assets'))
from prepare_native_forest import prepared_resources


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tinyxml2-root', type=Path, default=ROOT / 'ZAPDTR/lib/tinyxml2')
    args = parser.parse_args()
    resources = prepared_resources(ROOT / 'tools/native_forest/manifest.json')
    with tempfile.TemporaryDirectory(prefix='forest-material-loader-') as temporary:
        folder = Path(temporary)
        # Only logging is replaced; the actual resource factories/GBI macros
        # compile every command, including the tagged dynamic-segment address.
        (folder / 'spdlog').mkdir()
        (folder / 'spdlog/spdlog.h').write_text(
            '#pragma once\n#define SPDLOG_ERROR(...) ((void)0)\n#define SPDLOG_TRACE(...) ((void)0)\n')
        for name in ('mirror_access_root', 'mirror_access_material',
                     'geometry/Z2_LOST_WOODS_room_00DL_002548'):
            path = 'custom/comboship/native_forest/' + name
            target = folder / 'assets' / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(resources[path])
        sources = ['tests/native_forest/material_loader_test.cpp',
                   'libultraship/src/fast/resource/factory/DisplayListFactory.cpp',
                   'libultraship/src/fast/resource/type/DisplayList.cpp',
                   'libultraship/src/ship/resource/ResourceFactoryXML.cpp',
                   'libultraship/src/ship/resource/ResourceFactoryBinary.cpp',
                   'libultraship/src/ship/resource/Resource.cpp',
                   *('libultraship/src/ship/utils/binarytools/' + name + '.cpp'
                     for name in ('BinaryReader', 'MemoryStream', 'Stream'))]
        binary = folder / 'loader'
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DF3DEX_GBI_2',
                        '-ffunction-sections', '-fdata-sections', '-include', 'unordered_map',
                        '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                        '-I' + str(folder), '-I' + str(ROOT / 'libultraship/include'),
                        '-I' + str(args.tinyxml2_root),
                        *(str(ROOT / source) for source in sources), str(args.tinyxml2_root / 'tinyxml2.cpp'),
                        '-Wl,--gc-sections', '-o', str(binary)], check=True)
        subprocess.run([str(binary), str(folder / 'assets')], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


if __name__ == '__main__':
    main()
