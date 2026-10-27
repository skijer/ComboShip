#!/usr/bin/env python3
"""Run the production B-sword badge with real engine structs, bundled art and GBI."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    subprocess.run([sys.executable, '-B', str(ROOT / 'scripts/assets/build_sword_element_ui.py'), '--check'],
                   cwd=ROOT, check=True)
    with tempfile.TemporaryDirectory(prefix='sword-hud-') as directory:
        build = Path(directory)
        for host in ('soh', 'mm'):
            source = (ROOT / host / 'src/code/z_parameter.c').read_text()
            production = functions(source, {'Interface_DrawSwordElementBadge'})
            assert 'Interface_DrawSwordElementBadge' in production, host + ': missing B-sword element badge'
            (build / 'sword_badge_production.inc').write_text(production['Interface_DrawSwordElementBadge'])
            command = [os.environ.get('CC', 'cc'), '-std=gnu2x', '-DNDEBUG', '-DF3DEX_GBI_2',
                       '-w', '-DCONTROLLERBUTTONS_T=uint32_t', '-Werror=implicit-function-declaration',
                       '-Wno-incompatible-pointer-types']
            if host == 'mm':
                command += ['-DTEST_MM']
            for prefix in ('CMake/soh-cvars.cmake', 'CMake/2ship-cvars.cmake', 'CMake/lus-cvars.cmake'):
                for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)',
                                             (ROOT / prefix).read_text()):
                    command += [f'-D{key}="{value}"']
            roots = [ROOT / host / path for path in ('', 'include', 'include/PR', 'src', 'assets', '2s2h')]
            roots += [ROOT / 'libultraship/include', build]
            command += ['-I' + str(path) for path in roots]
            if args.sanitize:
                command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
            binary = build / host
            command += [str(ROOT / 'tests/elemental_swords/hud_badge_test.c'), '-o', str(binary)]
            subprocess.run(command, cwd=ROOT, check=True)
            subprocess.run([str(binary)], check=True,
                           env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
            legacy = command.copy()
            legacy[-3] = str(ROOT / 'tests/elemental_swords/hud_legacy_integration_test.c')
            legacy[-1] = str(build / (host + '-legacy'))
            legacy += ['-DSWORD_ELEMENT_TEST_COMBAT', '-ffunction-sections', '-fdata-sections',
                       '-Wl,--gc-sections', str(ROOT / host / 'src/code/sword_element.c'),
                       str(ROOT / host / 'src/code/din_fire_sword.c'), '-lm']
            if host == 'mm':
                legacy += ['-DSWORD_ELEMENT_TEST_MM']
            subprocess.run(legacy, cwd=ROOT, check=True)
            subprocess.run([str(build / (host + '-legacy'))], check=True,
                           env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
            if host == 'mm':
                icon = functions(source, {'Interface_DrawItemIconTexture'})['Interface_DrawItemIconTexture']
                editor = functions((ROOT / 'mm/2s2h/BenGui/HudEditor.cpp').read_text().replace('extern "C" ', ''),
                                   {'HudEditor_ShouldOverrideDraw', 'HudEditor_SetActiveElement',
                                    'HudEditor_ModifyRectPosValuesFloat', 'HudEditor_ModifyRectPosValues',
                                    'HudEditor_ModifyDrawValues'})
                integration = '\n'.join(editor.values()) + '\n'
                for name in ('sBCButtonXPositions', 'sBCButtonYPositions', 'sItemIconTextureScales'):
                    integration += re.search(r's16 ' + name + r'\[\] = \{.*?\};', source, re.S)[0] + '\n'
                (build / 'sword_badge_icon_production.inc').write_text(integration + icon)
                fixture = str(ROOT / 'tests/elemental_swords/mm_hud_integration_test.c')
                command[-3] = fixture
                command[-1] = str(build / 'mm-integration')
                subprocess.run(command, cwd=ROOT, check=True)
                subprocess.run([command[-1]], check=True,
                               env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))


if __name__ == '__main__':
    main()
