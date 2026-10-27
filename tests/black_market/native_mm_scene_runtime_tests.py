#!/usr/bin/env python3
"""Run the production MM scene-readiness boundary with actual native resources.

NativeShopSceneReady is compiled verbatim. Scene/camera/cutscene resource
classes, factories, binary readers and native cutscene-chain lookup are actual
production source. Only ResourceLoad/OwnRMScope selection and destructor trace
logging are controlled. This is source integration evidence, not gameplay proof.
"""
import argparse
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
sys.path.insert(0, str(ROOT / 'scripts/assets'))
from run_time_pedestal_tests import block_from
import prepare_native_market
import mm_native_shop


def function(source, signature):
    match = re.search(r'^' + re.escape(signature) + r'[^;{]*\{', source, re.M)
    if match is None:
        raise AssertionError('production function missing: ' + signature)
    return block_from(source, match.start())


def native_chain_source():
    manager = (ROOT / 'mm/src/code/z_eventmgr.c').read_text()
    actor = (ROOT / 'mm/src/overlays/actors/ovl_En_Ossan/z_en_ossan.c').read_text()
    source = '#include "global.h"\n#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"\n'
    source += block_from(manager, manager.index('ActorCutscene sGlobalCutsceneList[]')) + ';\n'
    for name in ('ActorCutscene* sSceneCutsceneList;', 's16 sSceneCutsceneCount;'):
        if name not in manager:
            raise AssertionError('native manager global changed: ' + name)
        source += name + '\n'
    source += '\n'.join(function(manager, signature) for signature in (
        'ActorCutscene* CutsceneManager_GetCutsceneEntryImpl(',
        'ActorCutscene* CutsceneManager_GetCutsceneEntry(', 's16 CutsceneManager_GetAdditionalCsId('))
    return source + '\n' + function(actor, 'void EnOssan_GetCutscenes(')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp')
    args = parser.parse_args()
    production = args.source.read_text()
    body = '\n'.join(re.search(r'^constexpr char k' + name + r'\[\] = [^;]+;', production, re.M)[0]
                     for name in ('Scene', 'Room')) + '\n'
    body += re.search(r'^std::shared_ptr<SOH::Scene> sNativeShopSceneResource;', production, re.M)[0] + '\n'
    body += function(production, 'bool NativeShopSceneReady(')
    fixture = (ROOT / 'tests/black_market/native_mm_scene_runtime_test.cpp').read_text().replace(
        '/* PRODUCTION READINESS */', body)
    resources = prepare_native_market.prepared_resources(host='mm')
    commands = mm_native_shop.native_commands(resources[mm_native_shop.SCENE])
    sources = [
        'mm/2s2h/resource/type/Scene.cpp',
        'mm/2s2h/resource/importer/scenecommand/SceneCommandFactory.cpp',
        'mm/2s2h/resource/importer/scenecommand/SetCsCameraFactory.cpp',
        'mm/2s2h/resource/importer/scenecommand/SetActorCutsceneListFactory.cpp',
        'mm/2s2h/resource/type/scenecommand/SetCsCamera.cpp',
        'mm/2s2h/resource/type/scenecommand/SetActorCutsceneList.cpp',
        'mm/2s2h/resource/type/scenecommand/SetMesh.cpp',
        'mm/2s2h/resource/importer/scenecommand/SetMeshFactory.cpp',
        'libultraship/src/ship/utils/binarytools/BinaryReader.cpp',
        'libultraship/src/ship/utils/binarytools/MemoryStream.cpp',
        'libultraship/src/ship/utils/binarytools/Stream.cpp',
        'libultraship/src/ship/resource/Resource.cpp',
    ]
    with tempfile.TemporaryDirectory(prefix='native-mm-scene-runtime-') as directory:
        temporary = Path(directory)
        cpp, native, native_object, binary = (temporary / name for name in ('test.cpp', 'chain.c', 'chain.o', 'test'))
        cpp.write_text(fixture)
        native.write_text(native_chain_source())
        logger = temporary / 'spdlog/spdlog.h'
        logger.parent.mkdir()
        logger.write_text('#pragma once\n#define SPDLOG_TRACE(...)\n#define SPDLOG_ERROR(...)\n')
        paths = ('.', 'mm', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets', 'mm/2s2h',
                 'libultraship/include', 'combo')
        include = ['-I' + str(temporary), *['-I' + str(ROOT / path) for path in paths]]
        flags = ['-g', '-w', '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-fsanitize=address,undefined',
                 '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-no-pie']
        subprocess.run([os.environ.get('CC', 'cc'), '-std=c11', *flags, *include,
                        '-c', str(native), '-o', str(native_object)], check=True)
        # Context.h reaches spdlog/async.h beyond the logger stub. External
        # distro fmt needs the same header-only mode as spdlog's bundled fmt.
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DFMT_HEADER_ONLY', *flags, *include,
                        str(cpp), *[str(ROOT / source) for source in sources], str(native_object),
                        '-o', str(binary)], check=True)
        arguments = []
        for identity in (2, 27):
            path = temporary / f'command-{identity}.bin'
            path.write_bytes(struct.pack('<I', identity) + commands[identity])
            arguments.append(str(path))
        room = mm_native_shop.native_commands(resources[mm_native_shop.ROOM])
        path = temporary / 'mesh.bin'
        path.write_bytes(struct.pack('<I', 10) + room[10])
        arguments.append(str(path))
        subprocess.run([str(binary), *arguments], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


if __name__ == '__main__':
    main()
