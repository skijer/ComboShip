#!/usr/bin/env python3
"""Exercise the supplied real Wolf archive through production MM loader and SSBB.

The archive is an explicit external input; this harness never generates a mesh,
installs it into a game, or loads its native plugin. Scratch extraction is temporary.
"""
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions


def run(command):
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        print(result.stdout + result.stderr)
        raise SystemExit(result.returncode)
    if result.stdout:
        print(result.stdout, end='')


def read_asset(path):
    asset = Path(path).resolve()
    supplied = asset.read_bytes()
    if zipfile.is_zipfile(asset):
        with zipfile.ZipFile(asset) as archive:
            resource = archive.read('objects/forms/wolf_link/gWolfLinkData')
        if len(resource) < 68 or resource[4:8] != b'BLBO' or struct.unpack_from('<I', resource, 8)[0] != 0:
            raise SystemExit('Invalid Blob resource envelope')
        length = struct.unpack_from('<I', resource, 64)[0]
        if length != len(resource) - 68:
            raise SystemExit('Invalid Blob payload length')
        payload = resource[68:]
        resource_envelope = resource
    elif len(supplied) >= 68 and supplied[4:8] == b'BLBO':
        resource_envelope = supplied
        payload = supplied[68:]
        if struct.unpack_from('<I', supplied, 64)[0] != len(payload):
            raise SystemExit('Invalid Blob payload length')
    else:
        payload = supplied
        resource_envelope = None
    print(f'Real Wolf input SHA256 {hashlib.sha256(supplied).hexdigest()}')
    print(f'Real Wolf payload SHA256 {hashlib.sha256(payload).hexdigest()} ({len(payload)} bytes)')
    return payload, resource_envelope


def main():
    if len(sys.argv) not in (2, 3):
        raise SystemExit('Usage: run_real_asset_tests.py <standard Wolf asset> [HD Wolf asset]')
    payload, resource_envelope = read_asset(sys.argv[1])
    hd_payload, hd_envelope = read_asset(sys.argv[2]) if len(sys.argv) == 3 else (None, None)
    with tempfile.TemporaryDirectory(prefix='real-wolf-') as td:
        build = Path(td)
        (build / 'real.bin').write_bytes(payload)
        if resource_envelope is not None:
            (build / 'resource.bin').write_bytes(resource_envelope)
        extra_args = []
        if hd_payload is not None:
            (build / 'hd.bin').write_bytes(hd_payload)
            extra_args = [str(build / 'hd.bin')]
        if hd_envelope is not None:
            (build / 'hd-resource.bin').write_bytes(hd_envelope)
        graph = functions((ROOT / 'mm/src/code/graph.c').read_text())
        (build / 'wolf-native-graph.inc').write_text(graph['Graph_OpenDisps'] + '\n' + graph['Graph_CloseDisps'])
        for mode_name, mode in [('sanitized', ['-O1', '-g', '-fsanitize=undefined,float-cast-overflow', '-fno-sanitize-recover=all']),
                                ('fast-math', ['-O2', '-ffast-math'])]:
            objects = []
            for i, path in enumerate(['mm/expansions/ssbb/ssbb_character.c', 'mm/expansions/ssbb/ssbb_skin.c',
                                      'mm/src/code/z_skin_matrix.c', 'mm/src/code/z_lib.c']):
                obj = str(build / f'{mode_name}-{i}.o')
                run([os.environ.get('CC', 'cc'), '-std=gnu11', *flags(),
                     '-include', str(ROOT / 'mm/include/z64malloc.h'),
                     '-include', str(ROOT / 'mm/expansions/ssbb/ssbb_anim.h'),
                     '-include', str(ROOT / 'mm/include/functions.h'), '-include', str(ROOT / 'mm/include/variables.h'),
                     '-include', str(ROOT / 'libultraship/include/libultraship/bridge/consolevariablebridge.h'),
                     '-ffunction-sections', '-fdata-sections', *mode, '-c', str(ROOT / path), '-o', obj])
                objects.append(obj)
            binary = str(build / mode_name)
            run([os.environ.get('CXX', 'c++'), '-std=c++20', *flags(), '-I' + td,
                 '-DMM_WOLF_HOST', '-DMM_WOLF_NATIVE_HANDOFF', '-DFMT_HEADER_ONLY',
                 '-DWOLF_IMPLEMENTATION="' + str(ROOT / 'mm/mods/transformation_masks/wolf_link_form.cpp') + '"',
                 '-ffunction-sections', '-fdata-sections', *mode,
                 str(ROOT / 'tests/mm_wolf/real_asset_runtime_test.cpp'), *objects, '-Wl,--gc-sections', '-o', binary])
            run([binary, td, str(build / 'real.bin'), *extra_args])
            # Execute OoT's complete loader on native OoT structures too.
            sys.path.insert(0, str(ROOT / 'tests/nei_held'))
            from run_articulated_tests import flags as oot_flags
            oot_options = oot_flags()[1:]
            oot_object = str(build / f'{mode_name}-oot-character.o')
            run([os.environ.get('CC', 'cc'), '-std=gnu11', *oot_options,
                 '-include', str(ROOT / 'soh/include/global.h'),
                 '-include', str(ROOT / 'soh/expansions/ssbb/ssbb_anim.h'),
                 '-include', str(ROOT / 'soh/include/functions.h'),
                 '-include', str(ROOT / 'soh/include/variables.h'),
                 '-include', str(ROOT / 'libultraship/include/libultraship/bridge/consolevariablebridge.h'),
                 '-ffunction-sections', '-fdata-sections', *mode, '-c',
                 str(ROOT / 'soh/expansions/ssbb/ssbb_character.c'), '-o', oot_object])
            oot_binary = str(build / f'{mode_name}-oot')
            run([os.environ.get('CXX', 'c++'), '-std=c++20', *oot_options, '-DFMT_HEADER_ONLY',
                 '-DWOLF_IMPLEMENTATION="' + str(ROOT / 'soh/mods/transformation_masks/wolf_link_form.cpp') + '"',
                 '-ffunction-sections', '-fdata-sections', *mode,
                 str(ROOT / 'tests/mm_wolf/oot_real_asset_test.cpp'), oot_object, '-Wl,--gc-sections', '-o', oot_binary])
            run([oot_binary, td, str(build / 'real.bin'), *extra_args])
        if resource_envelope is not None:
            binary = str(build / 'resource-boundary')
            lus_sources = ['ship/resource/CrossRMRegistry.cpp', 'ship/resource/Resource.cpp',
                           'ship/resource/type/Blob.cpp', 'ship/resource/ResourceFactoryBinary.cpp',
                           'ship/resource/factory/BlobFactory.cpp', 'ship/utils/binarytools/Stream.cpp',
                           'ship/utils/binarytools/MemoryStream.cpp', 'ship/utils/binarytools/BinaryReader.cpp']
            run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DCOMBO_BUILD', '-DFMT_HEADER_ONLY',
                 '-I' + str(ROOT / 'libultraship/include'), '-O1', '-g', '-fsanitize=undefined',
                 '-fno-sanitize-recover=all', '-ffunction-sections', '-fdata-sections',
                 str(ROOT / 'tests/mm_wolf/resource_boundary_test.cpp'),
                 *[str(ROOT / 'libultraship/src' / path) for path in lus_sources],
                 '-Wl,--gc-sections', '-o', binary])
            run([binary, str(build / 'resource.bin'), *([str(build / 'hd-resource.bin')] if hd_envelope else [])])


if __name__ == '__main__':
    main()
