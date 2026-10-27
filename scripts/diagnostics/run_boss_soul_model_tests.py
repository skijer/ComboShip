#!/usr/bin/env python3
"""Execute production boss recipes and MM command consumer with real MM headers.

Matrices/resource loading are fixture boundaries; this is not archive/runtime proof.
"""
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
oot = (ROOT / 'combo/menu/ComboItemDrawOOT.h').read_text()
mm = (ROOT / 'combo/menu/ComboItemDrawMM.h').read_text()
consumer = (ROOT / 'combo/menu/ComboForeignDrawMM.h').read_text()
soul = (ROOT / 'mm/2s2h/Rando/DrawFuncs.cpp').read_text()
# This is the complete production soul branch, including its real CVar decision.
start = oot.index('    if (rg >= RG_GOHMA_SOUL && rg <= RG_GANON_SOUL) {')
end = oot.index('    // Dungeon maps:', start)
recipe = 'static int32_t BossRecipe(RandomizerGet rg, CwItemDrawInfo* out) {\n' + oot[start:end] + '\nreturn 0;\n}'
draw = function(consumer, 'MM_DrawForeignBossSoul')
if 'inline void MM_DrawForeignMorphaSoul(' in consumer:
    draw += '\n' + function(consumer, 'MM_DrawForeignMorphaSoul')
else:
    draw += '\ninline void MM_DrawForeignMorphaSoul(const ComboForeignDrawInfoOOT* i) { MM_DrawForeignBossSoul(i); }'
mmrecipe = '\n'.join(function(mm, n) for n in ['MM_AnimSeg', 'MM_AnimTexSeg', 'MM_AnimSoulFlame'])
if 'static int32_t MM_FillBossSoulAnim(' in mm:
    mmrecipe += '\n' + function(mm, 'MM_FillBossSoulAnim')
mmrecipe += '\n' + function(mm, 'MM_FillAnimDrawInfo')
mmrecipe += '\n' + function(mm, 'MM_HasAnimDraw')
recipe += '\n' + function(oot, 'OOT_BossSoulUsesSkeleton')
recipe += '\n' + function(oot, 'OOT_IsStateDependentDraw')
recipe += '\n' + function(oot, 'OOT_DrawDependency')
export = function(oot, 'OOT_GetItemDrawInfo')
# Extract the complete dependency decision, including its surrounding guards.
result_guard = export.index('if (result != 1)')
dependency_start = export.index('}', result_guard) + 1
dependency = export[dependency_start:export.index('return 1;', dependency_start)]
recipe += '\nstatic int ExportedDependency(RandomizerGet rg) { CwItemDrawInfo info{}; auto* out = &info; ' + dependency + ' return info.stateDependent; }'
native = '\n'.join(function(soul, n) for n in ['DrawSoulFlame', 'DrawOotSoulFlame'])
flags = ['-std=gnu++20', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DLOG_LEVEL_GAME_PRINTS=0',
         '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING']
if '--sanitize' in sys.argv:
    flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
includes = ['mm', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets', 'mm/2s2h',
            'libultraship/include', 'libultraship/src', 'combo/menu']
with tempfile.TemporaryDirectory(prefix='boss-soul-models-') as td:
    build = Path(td)
    template = (ROOT / 'tests/foreign_soul/boss_model_test.cpp').read_text()
    for mark, value in [('PRODUCTION_RECIPE', recipe), ('PRODUCTION_DRAW', draw),
                        ('PRODUCTION_MM_RECIPE', mmrecipe), ('PRODUCTION_NATIVE_FLAME', native)]:
        template = template.replace('/* ' + mark + ' */', value)
    (build / 'test.cpp').write_text(template)
    binary = build / 'test'
    subprocess.run([os.environ.get('CXX', 'c++'), *flags, *['-I' + str(ROOT / p) for p in includes],
                    str(build / 'test.cpp'), '-o', str(binary)], check=True)
    env = os.environ.copy()
    env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS', '') + ':detect_leaks=0'
    subprocess.run([str(binary)], check=True, env=env)
