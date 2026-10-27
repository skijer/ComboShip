#!/usr/bin/env python3
"""Exercise MM's production Midna draw, routing, mixer and resource adapter without a ROM."""
import os
from pathlib import Path
import subprocess
import tempfile
from run_child_ruto_face_test import function

ROOT = Path(__file__).resolve().parents[2]
ACTOR = ROOT / 'mm/src/overlays/actors/ovl_En_Elf/z_en_elf.c'
HUD = ROOT / 'mm/src/code/z_parameter.c'
FLAGS = ['-std=gnu11', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DMM_BUILD_DLL',
         '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING', '-DNDEBUG',
         '-DLOG_LEVEL_GAME_PRINTS=0', '-Werror=implicit-function-declaration',
         '-Wno-int-conversion', '-Wno-incompatible-pointer-types', '-Wno-discarded-qualifiers']
FLAGS += ['-I' + str(ROOT / p) for p in ('mm/include', 'mm/include/PR', 'mm/src', 'mm',
           'mm/2s2h', 'mm/assets', 'libultraship/include', 'libultraship/src', 'combo')]

def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)

def main():
    source = ACTOR.read_text()
    lights = (ROOT / 'mm/src/code/z_lights.c').read_text()
    draw = ''.join(function(lights, name) for name in ('Lights_PointSetInfo', 'Lights_PointNoGlowSetInfo',
                     'Lights_PointGlowSetInfo', 'Lights_PointSetColorAndRadius'))
    draw += function(source, 'EnElf_BlackMarketQuiet')
    for name in ('EnElf_UpdateMidnaBlink', 'func_8088E60C', 'EnElf_Update', 'EnElf_DrawMidnaShimmer',
                 'EnElf_GetMidnaBlinkModel', 'EnElf_TryDrawMidna', 'EnElf_Draw'):
        if name + '(' in source:
            draw += function(source, name)
    flags = FLAGS + (['-DMIDNA_VISIBLE_CLOCK'] if 'midnaBlinkTimer' in source else [])
    with tempfile.TemporaryDirectory(prefix='mm-midna-') as folder:
        folder = Path(folder)
        fixture = (ROOT / 'mm/tests/midna_tatl_draw_test.c').read_text()
        cfile = folder / 'draw.c'
        cfile.write_text(fixture.replace('/* PRODUCTION_MIDNA_DRAW */', draw))
        run(os.environ.get('CC', 'cc'), *flags, str(cfile), '-lm', '-o', str(folder / 'draw'))
        run(str(folder / 'draw'))
        routing = function(HUD.read_text(), 'Interface_SetTatlCall')
        routing += ''.join(function(source, name) for name in ('EnElf_UpdateMidnaIdleAudio',
                           'EnElf_PlayTatlSound', 'EnElf_Destroy', 'func_8088EF18', 'func_8088EFA4'))
        cfile = folder / 'routing.c'
        cfile.write_text((ROOT / 'mm/tests/midna_audio_routing_test.c').read_text().replace(
            '/* PRODUCTION_MIDNA_AUDIO_ROUTING */', routing))
        run(os.environ.get('CC', 'cc'), *flags, str(cfile), '-lm', '-o', str(folder / 'routing'))
        run(str(folder / 'routing'))
        run(os.environ.get('CC', 'cc'), *flags, '-fsyntax-only', str(ACTOR), str(HUD))
        run(os.environ.get('CXX', 'c++'), '-std=c++17', '-pthread', '-Imm', '-Imm/tests',
            'mm/tests/midna_audio_test.cpp', 'mm/2s2h/Enhancements/Companion/MidnaAudio.cpp',
            '-o', str(folder / 'mixer'))
        result = subprocess.run([str(folder / 'mixer')], cwd=ROOT, capture_output=True, text=True)
        if result.returncode:
            print(result.stderr)
            result.check_returncode()
        print(result.stdout.strip())
        resources = (ROOT / 'mm/2s2h/Enhancements/Companion/MidnaAudioResources.cpp').read_text()
        adapter = ''.join(function(resources, name) for name in ('OwnManager', 'MMMidnaResources_Exists',
                                                                'MMMidnaResources_Load'))
        adapter += 'namespace MMMidnaAudioResources {\n'
        adapter += ''.join(function(resources, name) for name in ('ListClips', 'ReadAssignment', 'WriteAssignment',
                                                                'Enabled', 'HasModel', 'ReadClip', 'Gain'))
        adapter += '}\n'
        cppfile = folder / 'resources.cpp'
        cppfile.write_text((ROOT / 'mm/tests/midna_audio_resources_test.cpp').read_text().replace(
            '/* PRODUCTION_MIDNA_AUDIO_RESOURCES */', adapter))
        run(os.environ.get('CXX', 'c++'), '-std=c++17', '-DCOMBO_BUILD', '-Imm', '-Imm/tests',
            str(cppfile), '-o', str(folder / 'resources'))
        run(str(folder / 'resources'))
        print('PASS: complete MM fairy actor and HUD compile against production MM headers')

if __name__ == '__main__':
    main()
