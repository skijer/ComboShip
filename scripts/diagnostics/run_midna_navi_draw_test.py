#!/usr/bin/env python3
"""Compile real Navi draw logic against production headers, without a ROM."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
from run_child_ruto_face_test import function
from run_time_pedestal_tests import block_from

ROOT = Path(__file__).resolve().parents[2]
source_path = ROOT / 'soh/src/overlays/actors/ovl_En_Elf/z_en_elf.c'
source = source_path.read_text()
lights = (ROOT / 'soh/src/code/z_lights.c').read_text()
production = ''.join(function(lights, name) for name in (
    'Lights_PointSetInfo', 'Lights_PointNoGlowSetInfo', 'Lights_PointGlowSetInfo', 'Lights_PointSetColorAndRadius'))
production += function(source, 'EnElf_BlackMarketQuiet')
production += function(source, 'EnElf_UpdateLights')
if 'static void EnElf_UpdateMidnaBlink(' in source:
    production += function(source, 'EnElf_UpdateMidnaBlink')
production += function(source, 'EnElf_Update')
if 'static void EnElf_DrawMidnaShimmer(' in source:
    production += function(source, 'EnElf_DrawMidnaShimmer')
if 'static Gfx* EnElf_GetMidnaBlinkModel(' in source:
    production += function(source, 'EnElf_GetMidnaBlinkModel')
if 'static void EnElf_TraceMidnaFrame(' in source:
    production += function(source, 'EnElf_TraceMidnaFrame')
if 'static s32 EnElf_TryDrawMidna(' in source:
    production += function(source, 'EnElf_TryDrawMidna')
production += function(source, 'EnElf_Draw')
fixture = (ROOT / 'soh/tests/midna_navi_draw_test.c').read_text().replace(
    '/* PRODUCTION_MIDNA_DRAW */', production)
flags = ['-DCOMBO_BUILD', '-Icombo', '-std=gnu2x', '-DF3DEX_GBI_2', '-DLOG_LEVEL_GAME_PRINTS=0', '-DNDEBUG',
         '-Werror=implicit-function-declaration', '-Wno-int-conversion',
         '-Wno-incompatible-pointer-types', '-Wno-discarded-qualifiers',
         '-Ilibultraship/include', '-Isoh/include', '-Isoh/src', '-Isoh/assets', '-Isoh', '-Isoh/mods']
if 'midnaBlinkTimer' in source:
    flags.append('-DMIDNA_VISIBLE_CLOCK')
    # Shop companions are hidden and freeze movement/blink updates. Their timer
    # and explicit idle-audio disable still run; ordinary paths retain one tick.
    update = function(source, 'func_80A053F0')
    quiet = block_from(update, update.index('if (EnElf_BlackMarketQuiet(this, play))'))
    ordinary = update.replace(quiet, '', 1)
    for branch in (quiet, ordinary):
        assert branch.count('this->timer++;') == 1
    assert quiet.count('EnElf_UpdateMidnaBlink(this);') == 0
    assert quiet.count('this->actionFunc(this, play);') == 0
    assert quiet.count('Lights_PointNoGlowSetInfo(') == 2
    assert ordinary.count('EnElf_UpdateMidnaBlink(this);') == 1
    assert quiet.rstrip().endswith('return;\n    }')
    assert 'MidnaAudio_UpdateIdle(false);' in quiet
    assert 'EnElf_UpdateMidnaIdleAudio(this, play);' in ordinary
    assert source.count('EnElf_UpdateMidnaBlink(this);') == 3
    for name in ('func_80A04F94', 'EnElf_Update'):
        update = function(source, name)
        assert update.count('this->timer++;') == 1
        assert update.count('EnElf_UpdateMidnaBlink(this);') == 1
    assert 'this->midnaBlinkTimer = 0;' in function(source, 'EnElf_Init')
for path in ('CMake/soh-cvars.cmake', 'CMake/lus-cvars.cmake'):
    for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT/path).read_text()):
        flags.append(f'-D{key}="{value}"')
with tempfile.TemporaryDirectory(prefix='midna-navi-test-') as folder:
    folder = Path(folder)
    cfile, binary = folder/'draw.c', folder/'draw'
    cfile.write_text(fixture)
    subprocess.run([os.environ.get('CC','cc'), *flags, str(cfile), '-lm', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], cwd=ROOT, check=True)
    subprocess.run([os.environ.get('CC','cc'), *flags, '-fsyntax-only', str(source_path)], cwd=ROOT, check=True)
    print('PASS: entire z_en_elf.c compiles against current integration headers')
    # Reuse the native fixture's explicit service boundaries, executing the
    # complete production update body. Blink rendering itself is tested above;
    # here its call counter catches missed/double ticks or shop fallthrough.
    clock = (ROOT / 'tests/black_market/fairy_quiet_native_test.cpp').read_text().split('/* PRODUCTION */')[0]
    clock = clock.replace('static int blinks, movementUpdates;',
                          'static int blinks, movementUpdates, idleDisabled, idleUpdates;')
    clock = clock.replace('static void Bob(EnElf *, PlayState *) {}',
                          'static void Bob(EnElf *, PlayState *) { ++movementUpdates; }')
    clock = clock.replace('#define MidnaAudio_UpdateIdle(...) ((void)0)',
                          '#define MidnaAudio_UpdateIdle(active) (idleDisabled += !(active))')
    clock = clock.replace('#define EnElf_UpdateMidnaIdleAudio(...) ((void)0)',
                          '#define EnElf_UpdateMidnaIdleAudio(...) (++idleUpdates)')
    clock += function(source, 'EnElf_BlackMarketQuiet')
    # Match the existing native fixture's isolation of an unrelated pre-existing
    # debug mirror: sceneFlags[127] is outside its native 124-element type.
    clock += function(source, 'func_80A053F0').replace('gSaveContext.sceneFlags[127].chest', '0')
    clock += '''
int main() {
    static PlayState play;
    static Player player;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    EnElf fairy{};
    fairy.actor.params = FAIRY_NAVI;
    fairy.actionFunc = Bob;
    for (int mode : {COMBO_GAME_BOTH, COMBO_GAME_OOT, COMBO_GAME_MM}) {
        gComboGameMode = mode;
        for (int inside : {0, 1, 0}) {
            play.sceneNum = inside ? SCENE_SKIEJER_MARKET : SCENE_MARKET_DAY;
            const bool quiet = inside && mode != COMBO_GAME_MM;
            for (int frame = 0; frame < 230; ++frame) {
                const auto timer = fairy.timer;
                const int blink = blinks, movement = movementUpdates, disabled = idleDisabled, idle = idleUpdates;
                func_80A053F0(&fairy.actor, &play);
                assert(fairy.timer == u16(timer + 1));
                assert(blinks == blink + !quiet);
                assert(movementUpdates == movement + !quiet);
                assert(idleDisabled == disabled + quiet);
                assert(idleUpdates == idle + !quiet);
            }
        }
    }
    std::puts("PASS: native Navi shop suppresses movement/blink; ordinary/exit/reentry tick once and preserve idle-audio routing");
}
'''
    cppfile, clock_binary = folder/'clock.cpp', folder/'clock'
    cppfile.write_text(re.sub(r'\bthis\b', 'self', clock))
    includes = ('.', 'soh', 'soh/include', 'soh/include/PR', 'soh/src', 'soh/assets',
                'soh/soh', 'libultraship/include', 'combo')
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DCOMBO_BUILD',
                    '-DFAIRY_QUIET_OOT', '-DF3DEX_GBI_2', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-no-pie',
                    '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', '-DCVAR_PREFIX_SETTING="gSettings"',
                    *['-I' + str(ROOT / path) for path in includes], str(cppfile), '-o', str(clock_binary)],
                   cwd=ROOT, check=True)
    subprocess.run([str(clock_binary)], cwd=ROOT, check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
