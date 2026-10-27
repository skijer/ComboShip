#!/usr/bin/env python3
"""Run the real SoH seasonal controller and native rain resolver with native headers."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile


def extract(path, names):
    source = path.read_text()
    output = []
    for name in names:
        match = re.search(r'^[\w* ]+\b' + name + r'\([^;]*?\)\s*\{.*?^}', source, re.M | re.S)
        assert match, name
        line = source.count('\n', 0, match.start()) + 1
        output.append(f'#line {line} "{path}"\n{match[0]}\n')
    return '\n'.join(output)


def run(out, cc, cxx):
    root = Path(__file__).resolve().parents[2]
    out.mkdir(parents=True, exist_ok=True)
    # Use the unchanged gameplay ownership/getter functions and actual save
    # structure, so querying an unowned saved season would really mutate it.
    inventory = root / 'soh/mods/extended_inventory.c'
    getters = '#include "z64.h"\n#include "mods/extended_inventory.h"\n'
    getters += extract(inventory, ['Seasons_HasRod', 'Seasons_SeasonOwned', 'Seasons_SeasonCount',
                                  'Seasons_SeasonAt', 'Seasons_GetSeason', 'Seasons_SetSeason'])
    (out / 'season_weather_saved_getters.c').write_text(getters)
    rod = root / 'soh/mods/items/logic/item_rod_of_seasons.c'
    bridge = '#include "z64.h"\n#include "functions.h"\n#include "mods/extended_inventory.h"\n'
    bridge += '#include "soh/Enhancements/audio/SeasonWeather.h"\n'
    bridge += extract(rod, ['Seasons_PrecipTint', 'Seasons_ApplyWeather'])
    bridge += '\nvoid SeasonWeather_TestRodBridge(PlayState* play) { Seasons_ApplyWeather(play); }\n'
    bridge += extract(root / 'soh/src/code/code_8006C3A0.c', ['Flags_SetEnv', 'Flags_GetEnv'])
    bridge += '#include "overlays/actors/ovl_Door_Ana/z_door_ana.h"\n'
    bridge += 'void DoorAna_WaitOpen(DoorAna*, PlayState*);\n'
    bridge += extract(root / 'soh/src/overlays/actors/ovl_Door_Ana/z_door_ana.c',
                      ['DoorAna_SetupAction', 'DoorAna_WaitClosed'])
    # The environment functions are unchanged production code, not a second
    # implementation of density interpolation or sky-restoration behavior.
    bridge += '#include "macros.h"\n#include "variables.h"\n#include "vt.h"\n#include "libc/math.h"\n'
    bridge += '#include "soh/Enhancements/audio/GlobalOutdoorRainBridge.h"\n'
    bridge += '#include "soh/Enhancements/audio/WeatherSamplePlayer.h"\n#include "code/concurrent_weather_audio.h"\n'
    bridge += '#include "libultraship/bridge/consolevariablebridge.h"\n'
    bridge += 'extern s16 sLightningFlashAlpha; extern s32 sLightningBolts[3];\n'
    bridge += 'void Environment_DrawLightningFlash(PlayState*, u8, u8, u8, u8);\n'
    environment = root / 'soh/src/code/z_kankyo.c'
    bridge += extract(environment, ['func_800766C4', 'func_8006FB94', 'Environment_UpdateLightningStrike'])
    bridge += extract(root / 'soh/src/code/z_actor.c', ['Flags_GetEventChkInf'])
    environment_source = environment.read_text()
    # Exercise the real outdoor time/overcast palette setup before the final
    # clamp, rather than assigning bright synthetic colors straight to it.
    time_type = re.search(r'typedef struct \{[^{}]*\} struct_8011FB48;', environment_source, re.S)
    time_table = re.search(r'struct_8011FB48 D_8011FB48\[\]\[7\] = \{.*?^};',
                           environment_source, re.M | re.S)
    assert time_type and time_table
    bridge += time_type[0] + '\n' + time_table[0] + '\n'
    bridge += 'u8 D_8011FDCC, D_8011FDD0; f32 D_8011FDD4;\n'
    bridge += extract(environment, ['Environment_LerpWeight'])
    bridge += extract(root / 'soh/src/code/z_lib.c', ['Math_CosS', 'Math_SinS'])
    bridge += '#define TIME_ENTRY_1F (D_8011FB48[envCtx->unk_1F][i])\n'
    bridge += '#define TIME_ENTRY_20 (D_8011FB48[envCtx->unk_20][i])\n'
    outdoor_start = environment_source.index('                for (i = 0; i < ARRAY_COUNT(D_8011FB48')
    outdoor_end = outdoor_start
    depth = 0
    for index in range(environment_source.index('{', outdoor_start), len(environment_source)):
        depth += (environment_source[index] == '{') - (environment_source[index] == '}')
        if depth == 0:
            outdoor_end = index + 1
            break
    bridge += '\nvoid SeasonWeather_TestNativeOutdoorLighting(PlayState* play) {\n'
    bridge += ('EnvironmentContext* envCtx = &play->envCtx; '
               'EnvLightSettings* lightSettingsList = envCtx->lightSettingsList; '
               'f32 sp8C, sp88 = 0; u16 i, j;\n')
    bridge += f'#line {environment_source.count(chr(10), 0, outdoor_start) + 1} "{environment}"\n'
    bridge += environment_source[outdoor_start:outdoor_end] + '\n}\n'
    lighting_start = environment_source.index('        for (i = 0; i < 3; i++) {',
                                               environment_source.index('// Apply lighting adjustments'))
    lighting_end = environment_source.index('        // Set both directional light directions', lighting_start)
    bridge += '\nvoid SeasonWeather_TestNativeLighting(PlayState* play) {\n'
    bridge += 'EnvironmentContext* envCtx = &play->envCtx; LightContext* lightCtx = &play->lightCtx; s16 i;\n'
    bridge += environment_source[lighting_start:lighting_end] + '}\n'
    # Use the native palette-transition countdown too: clearing gloomySkyMode
    # is insufficient while native tags still see unk_21 or light row one.
    blend_start = environment_source.index('                        if (envCtx->unk_21) {',
                                           environment_source.index('void Environment_Update('))
    blend_end = blend_start
    depth = 0
    for index in range(environment_source.index('{', blend_start), len(environment_source)):
        depth += (environment_source[index] == '{') - (environment_source[index] == '}')
        if depth == 0:
            blend_end = index + 1
            break
    bridge += '\nvoid SeasonWeather_TestNativeSkyBlend(EnvironmentContext* envCtx) { f32 sp88;\n'
    bridge += environment_source[blend_start:blend_end] + '\n}\n'
    bomb = (root / 'soh/src/overlays/actors/ovl_En_Bom/z_en_bom.c').read_text()
    flash_start = bomb.index('            play->envCtx.adjLight1Color[0] =')
    flash_end = bomb.index('            Camera_AddQuake(', flash_start)
    bridge += '\nvoid SeasonWeather_TestBombFlash(PlayState* play) {\n' + bomb[flash_start:flash_end] + '}\n'
    fade_start = bomb.index('    if (play->envCtx.adjLight1Color[0] != 0)', bomb.index('void EnBom_Explode('))
    fade_end = bomb.index('    if (this->timer == 0)', fade_start)
    bridge += '\nvoid SeasonWeather_TestBombFade(PlayState* play) {\n' + bomb[fade_start:fade_end] + '}\n'
    (out / 'season_weather_rod_bridge.c').write_text(bridge)
    common = ['-g', '-O1', '-ffunction-sections', '-fdata-sections', '-DLOG_LEVEL_GAME_PRINTS=0',
              '-DCVAR_PREFIX_AUDIO="gAudioEditor"', '-DCVAR_PREFIX_SETTING="gSettings"',
              '-Isoh/tests/rain_engine_stubs', '-Isoh/tests', '-Isoh/include', '-Isoh/src',
              '-Isoh/assets', '-Isoh', '-Ilibultraship/include', '-Icombo']
    sources = [root / 'soh/tests/season_weather_runtime_test.cpp',
               root / 'soh/soh/Enhancements/audio/GlobalOutdoorRain.cpp',
               root / 'soh/src/code/concurrent_weather_audio.c',
               root / 'soh/src/libultra/gu/sins.c',
               root / 'soh/src/libultra/gu/coss.c',
               root / 'soh/src/overlays/actors/ovl_En_Weather_Tag/z_en_weather_tag.c',
               out / 'season_weather_saved_getters.c',
               out / 'season_weather_rod_bridge.c']
    controller = root / 'soh/soh/Enhancements/audio/SeasonWeather.cpp'
    if controller.exists():
        sources.append(controller)
    objects = []
    for source in sources:
        cpp = source.suffix == '.cpp'
        obj = out / (source.name + '.o')
        flags = ['-std=c++20'] if cpp else ['-std=gnu11']
        subprocess.run([cxx if cpp else cc, *flags, *common, '-c', str(source), '-o', str(obj)],
                       cwd=root, check=True)
        objects.append(str(obj))
    binary = out / 'season_weather_runtime_test'
    subprocess.run([cxx, *objects, '-Wl,--gc-sections', '-lm', '-o', str(binary)], cwd=root, check=True)
    subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', nargs='?', type=Path)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    args = parser.parse_args()
    if args.output:
        run(args.output.resolve(), args.cc, args.cxx)
    else:
        with tempfile.TemporaryDirectory(prefix='soh-season-weather-') as directory:
            run(Path(directory), args.cc, args.cxx)
