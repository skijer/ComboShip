#!/usr/bin/env python3
"""Check the production cutscene-only Zelda aura without executing the game."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_child_ruto_face_test import function


def main():
    env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'}
    if env.get('COMBO_NEI_FIXTURE_CPATH'):
        env['CPATH'] = os.pathsep.join((env['COMBO_NEI_FIXTURE_CPATH'], env.get('CPATH', '')))
    flags = ['-std=c++20', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DFMT_HEADER_ONLY',
             '-DCONTROLLERBUTTONS_T=uint32_t', '-w', '-fsanitize=address,undefined',
             '-fno-sanitize-recover=all', '-no-pie',
             *['-I' + str(ROOT / p) for p in ('.', 'combo', 'combo/menu', 'soh', 'soh/include',
                 'soh/include/PR', 'soh/assets', 'soh/src', 'soh/soh', 'libultraship/include')]]
    if '--fast-math' in sys.argv:
        flags += ['-O3', '-ffast-math']
    source = (ROOT / 'soh/soh/Enhancements/Gameplay/ComboTempleSages.cpp').read_text()
    fixture = (ROOT / 'tests/temple_sages/zelda_aura_native_test.cpp').read_text().replace(
        '/* PRODUCTION_AURA_BRIDGE */', function(source, 'OotSession') +
        function(source, 'ComboTempleSages_DrawZeldaAuraAt') + function(source, 'ComboTempleSages_DrawZeldaAura'))
    with tempfile.TemporaryDirectory(prefix='sages-zelda-aura-') as temporary:
        work = Path(temporary)
        for name in ('policy', 'native'):
            path = ROOT / 'tests/temple_sages/zelda_aura_policy_test.cpp'
            if name == 'native':
                path = work / 'native.cpp'; path.write_text(fixture)
            subprocess.run([os.environ.get('CXX', 'c++'), *flags, str(path), '-o', str(work / name)],
                           cwd=ROOT, env=env, check=True)
            subprocess.run([str(work / name)], cwd=ROOT, env=env, check=True)


if __name__ == '__main__':
    main()
