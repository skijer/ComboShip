#!/usr/bin/env python3
"""Decode immutable external Djipi data with the real native XML factories."""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
from build import archive

p = argparse.ArgumentParser()
for key in ('maps', 'textures', 'stormlib'):
    p.add_argument('--' + key, type=Path, required=True)
args = p.parse_args()
hashes = {path: hashlib.sha256(path.read_bytes()).hexdigest() for path in (args.maps, args.textures)}
resources = archive(args.maps, args.stormlib)
resources.update(archive(args.textures, args.stormlib))
with tempfile.TemporaryDirectory(prefix='granny-real-xml-') as directory:
    folder = Path(directory)
    (folder / 'spdlog').mkdir()
    (folder / 'spdlog/spdlog.h').write_text('#pragma once\n#define SPDLOG_ERROR(...) ((void)0)\n#define SPDLOG_TRACE(...) ((void)0)\n')
    texture_paths = []
    for path, data in resources.items():
        if not path.startswith('alt/scenes/shared/mahouya_scene/'):
            continue
        if data.lstrip().startswith(b'<'):
            target = folder / path; target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(data)
        else:
            texture_paths.append(path)
    (folder / 'textures.txt').write_text('\n'.join(texture_paths))
    sources = ['tests/black_market/granny_xml_loader_test.cpp',
               'libultraship/src/fast/resource/factory/DisplayListFactory.cpp',
               'libultraship/src/fast/resource/factory/VertexFactory.cpp',
               'libultraship/src/fast/resource/type/DisplayList.cpp',
               'libultraship/src/fast/resource/type/Vertex.cpp',
               'libultraship/src/ship/resource/ResourceFactoryXML.cpp',
               'libultraship/src/ship/resource/ResourceFactoryBinary.cpp',
               'libultraship/src/ship/resource/Resource.cpp']
    tinyxml = ROOT / 'ZAPDTR/lib/tinyxml2'; binary = folder / 'loader'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DF3DEX_GBI_2',
                    '-ffunction-sections', '-fdata-sections', '-include', 'unordered_map',
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie',
                    '-I' + str(folder), '-I' + str(ROOT), '-I' + str(ROOT / 'libultraship/include'),
                    '-I' + str(tinyxml), *(str(ROOT / source) for source in sources), str(tinyxml / 'tinyxml2.cpp'),
                    '-Wl,--gc-sections', '-o', str(binary)], check=True)
    for host in ('oot', 'mm'):
        subprocess.run([str(binary), str(folder), host], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
for path, digest in hashes.items():
    assert hashlib.sha256(path.read_bytes()).hexdigest() == digest
