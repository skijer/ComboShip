#!/usr/bin/env python3
"""Syntax-check receipt production translation units with actual game headers."""
from pathlib import Path
import os
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]


def main():
    sources = (
        'mm/src/code/z_message.c', 'mm/src/code/z_message_nes.c',
        'soh/src/code/z_message_PAL.c', 'mm/2s2h/Rando/ItemReceiptText.cpp',
        'mm/2s2h/CustomMessage/CustomMessage.cpp', 'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp',
        'mm/2s2h/Rando/StaticData/Items.cpp',
        'soh/soh/Enhancements/custom-message/CustomMessageManager.cpp',
        'soh/soh/Enhancements/randomizer/hook_handlers.cpp',
        'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp', 'mm/2s2h/Rando/DrawItem.cpp',
        'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c',
        'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope_NES.c',
        'soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c',
        'mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipLearningEponasSong.cpp',
        'mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipLearningSongOfHealing.cpp',
        'mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipLearningSongOfSoaring.cpp',
        'mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipLearningSongOfStorms.cpp',
        'mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipLearningSongOfTime.cpp',
    )
    if len(sys.argv) > 1:
        sources = tuple(sys.argv[1:])
    prefixes = ('COSMETIC', 'ENHANCEMENT', 'SETTING', 'REMOTE', 'RANDOMIZER_SETTING',
                'RANDOMIZER_ENHANCEMENT', 'CHEAT', 'DEVELOPER_TOOLS', 'AUDIO', 'WINDOW',
                'TRACKER', 'GENERAL', 'GAMEPLAY_STATS', 'TIME_DISPLAY')
    for filename in sources:
        game = filename.split('/')[0]
        cpp = filename.endswith('.cpp')
        compiler = shlex.split(os.environ.get('CXX' if cpp else 'CC', 'c++' if cpp else 'cc'))
        includes = (f'{game}/include', f'{game}/include/PR', f'{game}/src', game, f'{game}/assets',
                    f'{game}/2s2h' if game == 'mm' else 'soh/soh', 'libultraship/include', 'libultraship/src',
                    'combo', 'combo/menu')
        flags = ['-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DCONTROLLERBUTTONS_T=uint32_t', '-DLOG_LEVEL_GAME_PRINTS=0',
                 '-DNON_EQUIVALENT', '-DNON_MATCHING', '-DMM_BUILD_DLL' if game == 'mm' else '-DSOH_BUILD_DLL']
        flags += ['-I' + str(ROOT / directory) for directory in includes]
        if game == 'soh':
            flags += [f'-DCVAR_PREFIX_{prefix}="gReceiptSyntax{prefix}"' for prefix in prefixes]
        if not cpp:
            flags += ['-Werror=implicit-function-declaration', '-Wno-int-conversion', '-Wno-incompatible-pointer-types']
        else:
            flags += ['-DIMGUI_DEFINE_MATH_OPERATORS']
        if cpp and game == 'mm':
            # These are real target_precompile_headers entries in mm/CMakeLists.txt.
            flags += ['-include', 'libultraship/bridge/consolevariablebridge.h',
                      '-include', 'ship/Context.h', '-include', 'ship/window/Window.h']
        result = subprocess.run([*compiler, '-std=gnu++20' if cpp else '-std=gnu11', *flags,
                                 '-fsyntax-only', str(ROOT / filename)], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(filename + '\n' + result.stdout + result.stderr)
        print('PASS real-header receipt syntax:', filename,
              f'({result.stderr.count("warning:")} compiler warnings)')


if __name__ == '__main__':
    main()
