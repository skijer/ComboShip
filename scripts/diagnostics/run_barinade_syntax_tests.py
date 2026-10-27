#!/usr/bin/env python3
"""Compile boss callbacks, native MM dispatch and freeze against actual engine headers."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'combo/menu/ComboForeignAnim.h').read_text()
shim = source[source.index('#ifdef COMBO_FOREIGN_ANIM_HOST_MM'):source.index('#include <ship/Context.h>')]
callbacks = '\n'.join(function(source, name) for name in
                       ['CfaBarinadeOverride', 'CfaBarinadePost', 'CfaBarinadeBegin',
                        'CfaOverrideLimbDrawOpa', 'CfaPostLimbDrawOpa', 'CfaTwinmoldMatrices'])
freeze = re.search(r'if \(info->freezeLastFrame\) \{[^}]+}', source).group()
prefix = '''
#include <cstdint>
#ifdef HOST_MM
#include "ultra64.h"
#undef BTN_A
#undef BTN_B
#undef BTN_Z
#undef BTN_START
#undef BTN_DUP
#undef BTN_DDOWN
#undef BTN_DLEFT
#undef BTN_DRIGHT
#undef BTN_L
#undef BTN_R
#undef BTN_CUP
#undef BTN_CDOWN
#undef BTN_CLEFT
#undef BTN_CRIGHT
#endif
#ifdef HOST_MM
#include "global.h"
#else
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "z64math.h"
#endif
#include "ComboItemDrawABI.h"
extern const CwItemAnimDrawInfo* sCfaInfo;
const CwAnimLimbDL* CfaFindLimbDL(s32);
Gfx* CfaRouteLimbDList(Gfx*);
'''
with tempfile.TemporaryDirectory(prefix='barinade-syntax-') as temporary:
    build = Path(temporary)
    for host, game in [('mm', 'mm'), ('oot', 'soh')]:
        extra = '#define COMBO_FOREIGN_ANIM_HOST_MM\n' if host == 'mm' else ''
        unit = prefix + extra + shim + callbacks
        unit += '\nstruct Entry { SkelAnime skelAnime; };\n'
        unit += 'void Freeze(Entry& e, const CwItemAnimDrawInfo* info, AnimationHeader* anim) {' + freeze + '}\n'
        if host == 'mm':
            unit += '\n#include <string>\n#include "Rando/StaticData/StaticData.h"\n'
            unit += 'extern "C" int32_t MM_GetItemAnimDrawInfo(const char*, CwItemAnimDrawInfo*);\n'
            unit += 'int32_t ComboForeignAnim_Draw(const CwItemAnimDrawInfo*, const char*, PlayState*);\n'
            drawitem = (ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text()
            if 'int32_t ComboDrawNativeMmBossSoul(' in drawitem:
                unit += function(drawitem, 'ComboDrawNativeMmBossSoul')
            unit += function(drawitem, 'ComboDrawNativeTwinmoldSoul')
            unit += '\n#include "objects/gameplay_keep/gameplay_keep.h"\n// Rando.h only references json by reference here; its external dependency is opaque.\nnamespace nlohmann { class json; }\n#include "Rando/DrawFuncs.h"\n'
            native = (ROOT / 'mm/2s2h/Rando/DrawFuncs.cpp').read_text()
            unit += function(native, 'DrawSoulFlame') + '\n' + function(native, 'DrawMmSoulFlame')
            for object_name in ['object_boss_hakugin', 'object_boss03', 'object_boss01', 'object_boss02']:
                unit += '\n#include "objects/' + object_name + '/' + object_name + '.h"\n'
            unit += native[native.index('#define SETUP_DRAW('):native.index('// Soul Effects\nstatic void')]
            unit += '\n'.join(function(native, name) for name in ['DrawGoht', 'DrawGyorg', 'DrawOdolwa', 'DrawTwinmold'])

        if host == 'oot':
            native = (ROOT / 'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
            unit += '\n#include "objects/object_bv/object_bv.h"\nextern "C" int32_t OOT_MagicJarUsesCustomAsset(const char*);\n'
            unit += re.search(r'^#define M_PIf .+$', (ROOT / 'soh/soh/OTRGlobals.h').read_text(), re.M).group() + '\n'
            unit += '\n'.join(function(native, name) for name in ['OverrideLimbDrawBarinade', 'PostLimbDrawBarinade'])
        path = build / (host + '.cpp')
        path.write_text(unit)
        includes = [game, game + '/include', game + '/include/PR', game + '/src', game + '/assets', game + '/2s2h', game + '/soh',
                    'libultraship/include', 'libultraship/src', 'combo/menu']
        flags = ['-std=gnu++20', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DLOG_LEVEL_GAME_PRINTS=0',
                 '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING', '-Wno-comma-subscript', '-fsyntax-only']
        if host == 'mm': flags += ['-DHOST_MM']
        subprocess.run([os.environ.get('CXX', 'c++'), *flags, *['-I' + str(ROOT / p) for p in includes], str(path)], check=True)
        print('PASS production boss callbacks/native MM dispatch and Animation_Change/GetLastFrame against ' + host + ' headers')
