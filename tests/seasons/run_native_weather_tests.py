"""Exercise native MM season scheduling, selector timing and actual snow actor rendering."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_mm_weather_tests import production_function


def weather_flags():
    # These weather/selector bodies use real MM symbols and do not need the
    # generated OoT item redirect header used by the broader NEI fixture.
    result = flags()
    redirect = str(ROOT / 'mm/soh/_nei_compat_core.h')
    index = result.index(redirect)
    del result[index - 1:index + 1]
    return [*result, '-DCOMBO_BUILD']


def source(path):
    baseline = next((arg.split('=', 1)[1] for arg in sys.argv if arg.startswith('--preview-baseline=')), None)
    if baseline and path.endswith('/z_object_kankyo.c'):
        return subprocess.check_output(['git', 'show', baseline + ':' + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def body(path, name):
    return re.sub(r'\bthis\b', 'self', production_function(source(path), name))


with tempfile.TemporaryDirectory(prefix='native-season-weather-') as temporary:
    build = Path(temporary)
    snow = 'mm/src/overlays/actors/ovl_Object_Kankyo/z_object_kankyo.c'
    environment = 'mm/src/code/z_kankyo.c'
    rod = 'mm/mods/items/logic/item_rod_of_seasons.c'
    rcp = (ROOT / 'mm/src/code/z_rcp.c').read_text()
    setup = re.search(r'\{\s*/\* SETUPDL_0 \*/[\s\S]+?\n    \},', rcp)
    if setup is None:
        raise RuntimeError('Cannot find production SETUPDL_0 material commands')
    (build / 'native_particle_setup.inc').write_text('static Gfx sNativeParticleSetup[] = ' + setup[0][:-1] + ';\n')
    parts = [body('mm/src/audio/sequence.c', 'AudioSeq_QueueSeqCmd')]
    parts += [body('mm/2s2h/gu_pc.c', 'guMtxIdentF'), body('mm/src/libultra/gu/lookat.c', 'guLookAtF')]
    parts += [body(snow, name) for name in (
        'ObjectKankyo_SetupAction', 'ObjectKankyo_IsAutumnOwner', 'ObjectKankyo_AutumnBand',
        'ObjectKankyo_IsCompactAutumnScene', 'ObjectKankyo_AutumnViewBasis', 'ObjectKankyo_InitAutumnParticle',
        'ObjectKankyo_UpdateAutumnParticles', 'ObjectKankyo_RestoreAutumnParticle', 'func_808DC454',
        'ObjectKankyo_UpdateSnowTarget', 'ObjectKankyo_UpdateSeasonSnowParticles', 'func_808DCB7C', 'func_808DCBF8',
        'func_808DBEB0', 'func_808DBFB0', 'ObjectKankyo_Init', 'ObjectKankyo_Update', 'func_808DD3C8')
        if name in source(snow)]
    rod_source = (ROOT / rod).read_text()
    owner = '// Seasonal particles' + rod_source.split('// Seasonal particles')[1].split('#include "../objects')[0]
    parts.append(owner)
    owner_c = build / 'native_season_owner.c'
    owner_c.write_text('#include "z64.h"\n#include "2s2h/Enhancements/Audio/MMWeather.h"\n'
                       '#include "mods/items/helpers/season_particles.h"\n' + owner)
    for source_path in (ROOT / snow, owner_c):
        syntax = subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu17', '-fsyntax-only',
                                 '-DNDEBUG', '-DCOMBO_BUILD', '-DMM_BUILD_DLL',
                                 '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING',
                                 '-Wno-int-conversion', '-Wno-incompatible-pointer-types',
                                 *weather_flags(), str(source_path)], capture_output=True, text=True)
        if syntax.returncode:
            raise RuntimeError(syntax.stdout + syntax.stderr)
        print('PASS real-header seasonal C syntax:', source_path.name, flush=True)
    parts += [body('mm/mods/extended_inventory.c', name) for name in
              ('Seasons_HasRod', 'Seasons_SeasonOwned', 'Seasons_SeasonAt', 'Seasons_GetSeason', 'Seasons_SetSeason')]
    parts.append(body(rod, 'Seasons_OnWheelConfirm'))
    parts += [body('mm/mods/items/helpers/box_menu.c', name) for name in ('BoxMenu_Close', 'BoxMenu_Update')]
    parts += [body(environment, name) for name in
              ('func_800F6CEC', 'Environment_UpdateRain', 'Environment_DrawRain', 'Environment_Update')]
    play_source = (ROOT / 'mm/src/code/z_play.c').read_text()
    gate = re.search(r'if \([^;{}]*precipitation\[PRECIP_RAIN_CUR\][^;{}]*\)\s*\{\s*'
                     r'Environment_DrawRain\(this, &this->view, gfxCtx\);\s*\}', play_source)
    if gate is None:
        raise RuntimeError('Cannot find the native Play rain draw gate')
    parts.append('void DrawRainFromPlay(PlayState* play) { GraphicsContext* gfxCtx = play->state.gfxCtx;\n' +
                 gate[0].replace('this', 'play') + '\n}')
    parts += [body('mm/src/overlays/actors/ovl_En_Test4/z_en_test4.c', name) for name in
              ('EnTest4_UpdateWeatherClear', 'EnTest4_UpdateWeatherRainy')]
    tag = 'mm/src/overlays/actors/ovl_En_Weather_Tag/z_en_weather_tag.c'
    parts += [body(tag, name) for name in ('EnWeatherTag_SetupAction', 'func_80966758', 'func_80966E84')]
    fog = body(environment, 'Environment_DrawSandstorm')
    gate = re.search(r'if \(play->envCtx.sandstormPrimA != 0[^\{]*\)', fog)
    if gate is None:
        raise RuntimeError('Cannot find native sandstorm draw gate')
    # Execute actual native fog alpha/state evolution and the actual render gate;
    # omit only subsequent textured fog geometry/resource submission.
    parts.append(fog[:gate.start()].replace('void Environment_DrawSandstorm', 'bool NativeFogVisible') +
                 'return ' + gate[0][3:] + ';\n}')
    (build / 'native_weather.inc').write_text('\n'.join(parts))
    binary = build / 'native-weather'
    sanitizer = ['-fsanitize=address,undefined,bounds', '-fno-sanitize-recover=all'] if '--sanitize' in sys.argv else []
    result = subprocess.run([
        os.environ.get('CXX', 'c++'), '-std=c++20', '-O2', '-w', *sanitizer, *weather_flags(),
        '-I' + str(build), str(ROOT / 'tests/seasons/native_weather_test.cpp'),
        str(ROOT / 'mm/2s2h/Enhancements/Audio/MMWeather.cpp'),
        str(ROOT / 'mm/2s2h/Enhancements/Audio/MMWeatherState.cpp'), '-o', str(binary)
    ], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    output = next((arg.split('=', 1)[1] for arg in sys.argv if arg.startswith('--preview-output=')), None)
    subprocess.run([str(binary), *([output] if output else [])], check=True,
                   env={**os.environ, 'ASAN_OPTIONS': os.environ.get('ASAN_OPTIONS', '') + ':detect_leaks=0'})
