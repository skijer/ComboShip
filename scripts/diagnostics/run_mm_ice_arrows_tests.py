#!/usr/bin/env python3
"""Run production OoT ice-arrow code with native headers; no game runtime proof."""
import os
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--include', type=Path, action='append', default=[])
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    run_env = {**os.environ, 'ASAN_OPTIONS': os.environ.get('ASAN_OPTIONS','') + ':detect_leaks=0'}
    flags = ['-std=gnu2x', '-O1', '-g', '-DNDEBUG', '-DF3DEX_GBI_2', '-DLOG_LEVEL_GAME_PRINTS=0',
             '-ffunction-sections', '-fdata-sections', '-Werror=implicit-function-declaration',
             '-Wno-incompatible-pointer-types']
    for path in ('soh/include', 'soh/src', 'soh/assets', 'soh', 'libultraship/include', 'combo'):
        flags.append('-I' + str(ROOT / path))
    flags += ['-I' + str(path) for path in args.include]
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        # ASan global metadata would retain the uncalled native En_Arrow profile and
        # its entire renderer/init graph despite --gc-sections. Instrument the actual
        # executed code/heap/stack without turning this fixture into a full game link.
        compiler = subprocess.check_output([os.environ.get('CC','cc'),'--version'],text=True)
        flags += ['-mllvm','-asan-globals=0'] if 'clang' in compiler.lower() else ['--param=asan-globals=0']
    for path in ('CMake/soh-cvars.cmake', 'CMake/lus-cvars.cmake'):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            flags.append(f'-D{key}="{value}"')
    sources = ['soh/tests/mm_ice_arrows_test.c', 'soh/src/overlays/actors/ovl_En_Arrow/z_en_arrow.c',
               'soh/src/overlays/actors/ovl_Bg_MmIcefloe/z_bg_mm_icefloe.c']
    with tempfile.TemporaryDirectory(prefix='mm-ice-arrows-') as temp:
        for mode in ('solo','combo'):
            binary = Path(temp) / ('behavior-' + mode)
            subprocess.run([os.environ.get('CC', 'cc'), *flags, *(['-DCOMBO_BUILD'] if mode == 'combo' else []),
                            *sources, '-Wl,--gc-sections', '-lm', '-o', binary],cwd=ROOT,check=True)
            subprocess.run([binary],check=True,env=run_env)
        # Compile/run the actual C++ bridge and native owning resource classes in both builds.
        if args.include or os.environ.get('CPATH'):
            cppflags = [f for f in flags if f not in ('-std=gnu2x','-Werror=implicit-function-declaration',
                                                     '-Wno-incompatible-pointer-types','-O1','-g')]
            cppflags += ['-std=c++20', '-O0', '-DFMT_HEADER_ONLY', '-DSPDLOG_ACTIVE_LEVEL=6']
            resources = ['soh/tests/mm_ice_arrows_resources_test.cpp',
                         'soh/soh/Enhancements/Items/MmIceArrows.cpp',
                         'soh/soh/resource/type/CollisionHeader.cpp',
                         'libultraship/src/fast/resource/type/DisplayList.cpp',
                         'libultraship/src/ship/resource/Resource.cpp']
            for mode in ('solo','combo'):
                output = Path(temp) / ('resources-' + mode)
                subprocess.run([os.environ.get('CXX','c++'), *cppflags,
                                *(['-DCOMBO_BUILD'] if mode == 'combo' else []), *resources,
                                '-Wl,--gc-sections','-pthread','-o',output],cwd=ROOT,check=True)
                subprocess.run([output],check=True,env=run_env)
        else:
            print('SKIP native C++ resource bridge: pass public dependency roots with --include or CPATH')

if __name__ == '__main__':
    main()
