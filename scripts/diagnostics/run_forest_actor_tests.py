#!/usr/bin/env python3
"""Focused forest actor checks; no game executable or runtime proof implied."""
import os
import re
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    match = re.search(r'^[^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError(name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'

def run(*args):
    subprocess.run([str(x) for x in args], cwd=ROOT, check=True)

def main():
    for bits in [32, 64]:
        run('cc', '-std=c11', f'-m{bits}', '-ffreestanding', '-fsyntax-only', '-Icombo',
            'combo/tests/placed_saria_abi_layout_test.c')
        print(f'PASS frozen Saria ABI layout: {bits}-bit V1 and V2 sizes/offsets', flush=True)
    run('python3', '-B', 'tests/monkey_audio/run_tests.py')
    with tempfile.TemporaryDirectory(prefix='forest-actors-') as temp:
        work = Path(temp)
        folder = 'soh/src/overlays/actors/ovl_En_Viewer/'
        run('cc', '-std=c11', '-Isoh/include', 'soh/tests/static_story_monkey_test.c',
            *(folder + 'static_story_' + name + '.c' for name in ['actor', 'mm_actor', 'dialogue']),
            '-lm', '-o', work/'monkey')
        run(work/'monkey')
        viewer = (ROOT/'mm/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c').read_text()
        fixture = (ROOT/'mm/tests/placed_saria_viewer_test.c').read_text()
        names = ['EnViewer_SetupAction', 'EnViewer_Init', 'EnViewer_Destroy', 'EnViewer_Update', 'EnViewer_Draw']
        (work/'viewer.c').write_text(fixture.replace('/* PRODUCTION_VIEWER_FUNCTIONS */',
                                                   '\n'.join(function(viewer, name) for name in names)))
        # Standalone fixtures also support distro spdlog using external fmt.
        flags = ['-std=gnu11', '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-DNDEBUG', '-D_LANGUAGE_C', '-DFMT_HEADER_ONLY',
                 '-DCONTROLLERBUTTONS_T=uint32_t', '-DLOG_LEVEL_GAME_PRINTS=6',
                 '-Imm/include', '-Imm/include/PR', '-Imm/src', '-Imm/assets', '-Imm',
                 '-Imm/2s2h', '-Ilibultraship/include', '-Icombo', '-Icombo/menu']
        run('cc', *flags, work/'viewer.c', '-lm', '-o', work/'viewer')
        run(work/'viewer')
        run('c++', '-std=c++20', *flags[1:], 'mm/tests/forest_epona_test.cpp', '-lm', '-o', work/'epona')
        run(work/'epona')
        skull = (ROOT/'mm/2s2h/Enhancements/Gameplay/ComboForestSkullKid.cpp').read_text()
        spawn = skull.split('namespace {', 1)[1].split('void RegisterForestSkullKid()', 1)[0]
        fixture = (ROOT/'mm/tests/forest_skull_kid_test.cpp').read_text()
        (work/'skull.cpp').write_text(fixture.replace('/* PRODUCTION_SKULL_KID_SPAWN */',
                                                      'namespace {\n' + spawn + '}\n'))
        run('c++', '-std=c++20', *flags[1:], work/'skull.cpp', '-lm', '-o', work/'skull')
        run(work/'skull')
        # Real resource classes and accessors; only the archive lookup boundary
        # is supplied by the fixture. This checks malformed packs and retention.
        skeleton = (ROOT/'soh/soh/resource/type/Skeleton.cpp').read_text()
        (work/'skeleton.cpp').write_text('#include "soh/resource/type/Skeleton.h"\nnamespace SOH {\n' +
            function(skeleton, 'Skeleton::GetPointer') + function(skeleton, 'Skeleton::GetPointerSize') + '}\n')
        provider = (ROOT/'soh/soh/Enhancements/cosmetics/PlacedSariaResources.cpp').read_text()
        providerNames = ['OOT_PlacedSariaAltEnabled', 'OOT_PlacedSariaLoad', 'OOT_PlacedSariaRelease']
        if 'int OOT_PlacedSariaLoadV2(' in provider:
            providerNames.insert(1, 'OOT_PlacedSariaLoadV2')
        fixture = (ROOT/'soh/tests/placed_saria_resources_test.cpp').read_text()
        (work/'saria-provider.cpp').write_text(fixture.replace('/* PRODUCTION_PLACED_SARIA_PROVIDER */',
            '\n'.join(function(provider, name) for name in providerNames)))
        sources = [str(work/'saria-provider.cpp'), str(work/'skeleton.cpp'),
                   'soh/soh/resource/type/SkeletonLimb.cpp', 'soh/soh/resource/type/Animation.cpp',
                   'soh/soh/resource/type/Array.cpp',
                   'libultraship/src/ship/resource/Resource.cpp', 'libultraship/src/fast/resource/type/Texture.cpp',
                   'libultraship/src/fast/resource/type/DisplayList.cpp']
        run('c++', '-std=c++20', '-DF3DEX_GBI_2', '-DNDEBUG', '-DFMT_HEADER_ONLY', '-Ilibultraship/include',
            '-Isoh/include', '-Isoh', '-Isoh/assets', '-Isoh/mods', '-Icombo', *sources, '-o', work/'resources')
        run(work/'resources')
        runtime = (ROOT/'mm/2s2h/Enhancements/Gameplay/ComboSaria.cpp').read_text()
        core = runtime.split('namespace {', 1)[1].split('void SpawnForestSaria()', 1)[0]
        public = runtime[runtime.index('extern "C" int ComboSaria_IsParam'):]
        bodies = '\n'.join(function(public, name) for name in
                           ['ComboSaria_IsParam', 'ComboSaria_Init', 'ComboSaria_Destroy', 'ComboSaria_Update', 'ComboSaria_Draw'])
        fixture = (ROOT/'mm/tests/placed_saria_lifecycle_test.cpp').read_text()
        receipt = (ROOT/'mm/2s2h/Rando/ItemReceiptText.cpp').read_text()
        fixture = fixture.replace('/* PRODUCTION_RECEIPT_FINALIZER */',
                                  function(receipt, 'Rando::AppendReceiptSource'))
        (work/'lifecycle.cpp').write_text(fixture.replace('/* PRODUCTION_SARIA_RUNTIME */',
            function(runtime, 'DrawShimmer') + 'namespace {\n' + core + function(runtime, 'SpawnForestSaria') + function(runtime, 'SpawnForestEpona') + '}\n' + bodies))
        run('c++', '-std=c++20', *flags[1:], work/'lifecycle.cpp', '-ldl', '-lm', '-o', work/'lifecycle')
        run(work/'lifecycle')
        run('c++', '-std=c++20', *flags[1:], 'mm/tests/placed_saria_save_test.cpp', '-o', work/'save')
        run(work/'save')

if __name__ == '__main__':
    main()
