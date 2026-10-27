#!/usr/bin/env python3
"""Exercise the complete foreign animation header with the actual native limb drawers.

Resource I/O, animation initialization and matrix/GBI primitives are modeled. Native
rigid/flex limb traversal and foreign loading/cache/dispatch/routing are production code.
No TP archive or live scene is emulated. --baseline demonstrates the missing matrices.
"""
import os
import re
from pathlib import Path
import subprocess
import sys
import tempfile
from run_mm_scene_randomization_tests import function as production_function

ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    start = re.search(r'^(?:static |extern )?void ' + re.escape(name) + r'\(', source, re.M).start()
    token = source.index(name + '(', start)
    brace = source.index('{', token)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

header = (ROOT / 'combo/menu/ComboForeignAnim.h').read_text()
if '--baseline-barinade' in sys.argv:
    header = subprocess.check_output(['git', 'show', '4542e9a:combo/menu/ComboForeignAnim.h'], cwd=ROOT, text=True)
if '--baseline-aura' in sys.argv:
    header = subprocess.check_output(['git', 'show', '9e3a2a6006c8d77f6e733da1987dd44681566ba7:combo/menu/ComboForeignAnim.h'], cwd=ROOT, text=True)
if '--baseline' in sys.argv:
    header = subprocess.check_output(['git', 'show', 'd1965180:combo/menu/ComboForeignAnim.h'], cwd=ROOT, text=True)
    # Fixture's factory-layout mirror was introduced with the correction.
    header = header.replace('struct CfaSkelEntry {', 'struct CfaLoadedSkeletonHeader { void** segment; uint8_t limbCount, skeletonType; };\nstruct CfaLoadedFlexSkeletonHeader { CfaLoadedSkeletonHeader sh; uint8_t dListCount; };\nstruct CfaSkelEntry {')
with tempfile.TemporaryDirectory(prefix='foreign-soul-') as td:
    build = Path(td)
    (build / 'foreign_anim.h').write_text(header)
    oot = (ROOT / 'combo/menu/ComboItemDrawOOT.h').read_text()
    native = (ROOT / 'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
    assets = ['object_goma', 'object_kingdodongo', 'object_bv', 'object_gnd', 'object_fd', 'object_sst', 'object_tw', 'object_ganon2', 'object_gi_fire']
    recipe = '\n'.join('#include "' + str(ROOT / 'soh/assets/objects' / n / (n + '.h')) + '"' for n in assets)
    recipe += '\n#define RANDO_ENUM_BEGIN(name) enum name {\n#define RANDO_ENUM_ITEM(name) name,\n#define RANDO_ENUM_END(name) };\n'
    recipe += '#include "' + str(ROOT / 'soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h') + '"\n'
    recipe += '#define CVAR_RANDOMIZER_ENHANCEMENT(name) name\nstatic int CVarGetInteger(const char*, int) { return 0; }\n'
    recipe += 'static bool barinadeCustom = false;\nextern "C" int32_t OOT_MagicJarUsesCustomAsset(const char*) { return barinadeCustom; }\n'
    recipe += '#define BOSSGOMA_LIMB_EYE 5\n#define BOSSGOMA_LIMB_IRIS 39\n'
    recipe += re.search(r'static const uint8_t kBossSoulFlameColors\[[\s\S]+?\n};', oot).group()
    recipe += '\n' + '\n'.join(production_function(oot, n) for n in ['OOT_AnimBossSoulFlame', 'OOT_AnimSeg', 'OOT_AnimLimbEnv', 'OOT_BossSoulUsesSkeleton', 'OOT_FillBossSoulAnim'])
    recipe += '\n' + '\n'.join(production_function(native, n) for n in ['OverrideLimbDrawBarinade', 'PostLimbDrawBarinade'])
    (build / 'barinade_production.inc').write_text(recipe)
    mm = (ROOT / 'combo/menu/ComboItemDrawMM.h').read_text()
    mmrecipe = '#include "' + str(ROOT / 'mm/assets/objects/object_boss02/object_boss02.h') + '"\n'
    mmrecipe += '#include "' + str(ROOT / 'mm/assets/objects/object_boss01/object_boss01.h') + '"\n'
    mmrecipe += '#include "' + str(ROOT / 'mm/assets/objects/object_boss03/object_boss03.h') + '"\n'
    mmrecipe += '#include "' + str(ROOT / 'mm/assets/objects/object_boss_hakugin/object_boss_hakugin.h') + '"\n'
    mmrecipe += 'enum RandoItemId { RI_SOUL_BOSS_GOHT, RI_SOUL_BOSS_GYORG, RI_SOUL_BOSS_ODOLWA, RI_SOUL_BOSS_TWINMOLD };\n'
    mmrecipe += '\n'.join(production_function(mm, n) for n in ['MM_AnimSeg', 'MM_AnimTexSeg', 'MM_AnimSoulFlame', 'MM_FillBossSoulAnim'])
    mmrecipe += '\nnamespace Rando::StaticData { const std::string& GetItemDisplayName(RandoItemId id) { static const std::string names[] = {"Soul of Goht", "Soul of Gyorg", "Soul of Odolwa", "Soul of Twinmold"}; assert(id >= RI_SOUL_BOSS_GOHT && id <= RI_SOUL_BOSS_TWINMOLD); return names[id]; } }\n'
    mmrecipe += 'static bool twinmoldRecipeAvailable = true;\nextern "C" int32_t MM_GetItemAnimDrawInfo(const char* name, CwItemAnimDrawInfo* out) { for (int id = RI_SOUL_BOSS_GOHT; id <= RI_SOUL_BOSS_TWINMOLD; ++id) { if (Rando::StaticData::GetItemDisplayName((RandoItemId)id) == name) return twinmoldRecipeAvailable ? MM_FillBossSoulAnim((RandoItemId)id, out) : 0; } assert(false); return 0; }\n'
    drawitem = (ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text()
    native_bosses = (ROOT / 'mm/2s2h/Rando/DrawFuncs.cpp').read_text()
    if '--baseline-mm-bosses' in sys.argv or '--baseline-mm-bosses-type' in sys.argv:
        drawitem = subprocess.check_output(['git', 'show', '13901c677d0084e0305eec4672c0557f4434aea7:mm/2s2h/Rando/DrawItem.cpp'], cwd=ROOT, text=True)
        native_bosses = subprocess.check_output(['git', 'show', '13901c677d0084e0305eec4672c0557f4434aea7:mm/2s2h/Rando/DrawFuncs.cpp'], cwd=ROOT, text=True)
    if 'int32_t ComboDrawNativeMmBossSoul(' in drawitem:
        mmrecipe += production_function(drawitem, 'ComboDrawNativeMmBossSoul')
    mmrecipe += production_function(drawitem, 'ComboDrawNativeTwinmoldSoul')
    mmrecipe += '\nstatic int nativeTwinmoldFallbacks = 0;\n#define SETUP_DRAW(n) PlayState* play = gPlayState; SkelAnime skelAnime{}; ++nativeTwinmoldFallbacks; OPEN_DISPS(gPlayState->state.gfxCtx);\n#define SETUP_FLEX_SKEL(...) ((void)0)\nstatic void DrawEnLight(Color_RGB8 color, Vec3f scale) {\n#ifdef HOST_MM\nDrawSoulFlame(gPlayState, color, scale);\n#endif\n}\n'
    mmrecipe += production_function(native_bosses, 'DrawTwinmold')
    mmrecipe += '\n#undef SETUP_DRAW\n#undef SETUP_FLEX_SKEL\n'
    # Execute the native boss entry points with their real static initialization
    # macros. The local play alias adapts only the fixture's OPEN_DISPS boundary.
    macros = native_bosses[native_bosses.index('#define SETUP_DRAW('):native_bosses.index('// Soul Effects\nstatic void')]
    macros = macros.replace('#define SETUP_DRAW(LIMB_MAX)', '#define SETUP_DRAW(LIMB_MAX) PlayState* play = gPlayState;')
    mmrecipe += '\n#ifdef HOST_MM\n' + macros
    mmrecipe += '\n'.join(production_function(native_bosses, name) for name in ['DrawGoht', 'DrawGyorg', 'DrawOdolwa'])
    mmrecipe += '\n#undef SETUP_DRAW\n#undef SETUP_DRAW_TYPE\n#undef SETUP_SKEL\n#undef SETUP_FLEX_SKEL\n#endif\n'
    (build / 'twinmold_production.inc').write_text(mmrecipe)
    for path in ['ship/Context.h', 'ship/resource/ResourceManager.h', 'ship/resource/ResourceManagerScope.h', 'ship/resource/CrossRMRegistry.h']:
        p = build / path
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text('#pragma once\n')
    for host in ['mm', 'oot']:
        engine = (ROOT / ('mm/src/code/z_skelanime.c' if host == 'mm' else 'soh/src/code/z_skelanime.c')).read_text()
        draw_names = ['SkelAnime_DrawLimbOpa', 'SkelAnime_DrawOpa', 'SkelAnime_DrawFlexLimbOpa']
        if 'static void SkelAnime_ApplyNpcSkin(' in engine:
            draw_names.insert(0, 'SkelAnime_ApplyNpcSkin')
        if 'static void SkelAnime_DrawFlexOpaImpl(' in engine:
            draw_names += ['SkelAnime_DrawFlexOpaImpl']
        draw_names += ['SkelAnime_DrawFlexOpa']
        if 'void SkelAnime_DrawFlexOpaWithXlu(' in engine:
            draw_names += ['SkelAnime_DrawFlexOpaWithXlu']
        production = '\n'.join(function(engine, n) for n in draw_names)
        # C's implicit void-pointer conversions need explicit casts in this C++ harness.
        production = production.replace('= Lib_SegmentedToVirtual(skeleton[', '= (StandardLimb*)Lib_SegmentedToVirtual(skeleton[')
        production = production.replace('Mtx* mtx = GRAPH_ALLOC(', 'Mtx* mtx = (Mtx*)GRAPH_ALLOC(')
        production = production.replace('Mtx* mtx = Graph_Alloc(', 'Mtx* mtx = (Mtx*)Graph_Alloc(')
        (build / 'native_draw.inc').write_text(production)
        soul = (ROOT / 'mm/2s2h/Rando/DrawFuncs.cpp').read_text()
        (build / 'native_soul.inc').write_text('\n'.join(function(soul, n) for n in ['DrawSoulFlame', 'DrawMmSoulFlame', 'DrawOotSoulFlame']))
        binary = build / host
        flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-Wno-unused-variable', '-Wno-unused-but-set-variable']
        flags += ['-DCOMBO_BUILD']
        if host == 'mm': flags += ['-DHOST_MM']
        if '--baseline-mm-bosses-type' in sys.argv or '--native-mm-bosses-alt-first' in sys.argv:
            flags += ['-DNATIVE_MM_BOSS_ALT_FIRST']
        if '--baseline' in sys.argv or '--baseline-aura' in sys.argv: flags += ['-DSKIP_BARINADE_TESTS', '-DSKIP_TWINMOLD_TESTS']
        if '--baseline-barinade' in sys.argv: flags += ['-DSKIP_TWINMOLD_TESTS']
        if '--sanitize' in sys.argv: flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
        subprocess.run([os.environ.get('CXX', 'c++'), *flags, '-I'+str(build), '-I'+str(ROOT/'combo/menu'), '-I'+str(ROOT/'soh/assets'), '-I'+str(ROOT/'soh/include'), str(ROOT/'tests/foreign_soul/skeleton_test.cpp'), '-o', str(binary)], check=True)
        env = os.environ.copy()
        env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS', '') + ':detect_leaks=0'
        subprocess.run([str(binary)], check=True, env=env)
