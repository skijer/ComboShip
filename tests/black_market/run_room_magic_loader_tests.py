#!/usr/bin/env python3
"""Validate the private room copy against real loaded generated resources."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/assets'))
from prepare_native_market import prepared_resources

with tempfile.TemporaryDirectory(prefix='room-magic-loader-') as temporary:
    folder = Path(temporary)
    (folder / 'spdlog').mkdir()
    (folder / 'spdlog/spdlog.h').write_text(
        '#pragma once\n#define SPDLOG_ERROR(...) ((void)0)\n#define SPDLOG_TRACE(...) ((void)0)\n')
    sources = ['tests/black_market/room_magic_loader_test.cpp',
               'libultraship/src/fast/resource/factory/DisplayListFactory.cpp',
               'libultraship/src/fast/resource/factory/VertexFactory.cpp',
               'libultraship/src/fast/resource/type/DisplayList.cpp',
               'libultraship/src/fast/resource/type/Vertex.cpp',
               'libultraship/src/ship/resource/ResourceFactoryXML.cpp',
               'libultraship/src/ship/resource/ResourceFactoryBinary.cpp',
               'libultraship/src/ship/resource/Resource.cpp']
    binary = folder / 'loader'
    tinyxml = ROOT / 'ZAPDTR/lib/tinyxml2'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DF3DEX_GBI_2',
                    '-ffunction-sections', '-fdata-sections', '-include', 'unordered_map',
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie',
                    '-I' + str(folder), '-I' + str(ROOT), '-I' + str(ROOT / 'libultraship/include'),
                    '-I' + str(tinyxml), *(str(ROOT / source) for source in sources), str(tinyxml / 'tinyxml2.cpp'),
                    '-Wl,--gc-sections', '-o', str(binary)], check=True)
    for host in ('oot', 'mm'):
        for name, data in prepared_resources(host=host).items():
            if name.startswith('custom/skiejer_market/'):
                target = folder / host / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(data)
        for mode in ('private', 'source'):
            subprocess.run([str(binary), str(folder / host), host, mode], check=True,
                           env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
