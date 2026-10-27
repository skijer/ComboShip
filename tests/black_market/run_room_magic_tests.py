#!/usr/bin/env python3
"""Exercise the production room graph and each host's native attachment functions."""
from pathlib import Path
import os
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[2]
assert (ROOT / 'combo/BlackMarketRoomMagic.h').exists(), 'missing production optional room-material implementation'
with tempfile.TemporaryDirectory(prefix='room-magic-') as temp:
    binary = Path(temp) / 'room_magic'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DF3DEX_GBI_2', '-g',
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
                    '-I' + str(ROOT), '-I' + str(ROOT / 'libultraship/include'),
                    str(ROOT / 'tests/black_market/room_magic_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})

    sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
    from run_time_pedestal_tests import block_from
    for host in ('mm', 'soh'):
        source = (ROOT / host / ('2s2h' if host == 'mm' else 'soh') / 'Enhancements/Gameplay' /
                  ('BlackMarket.cpp' if host == 'mm' else 'ComboBlackMarketNative.cpp')).read_text()
        assert 'void BeginRoomMagic(' in source, f'{host}: missing native room attachment'
        start = source.index('BlackMarketRoomMagic::Resources sRoomMagicResources;')
        end = source.index('void RestoreRoomMagic(', start)
        production = source[start:end] + '\n'.join(block_from(source, source.index(signature)) for signature in
            ('void RestoreRoomMagic(', 'void ClearRoomMagic(', 'void BeginRoomMagic('))
        resources = (ROOT / 'combo/BlackMarketRoomMagicResources.h').read_text()
        palette = block_from(resources, resources.index('inline Palette ReadPalette('))
        fixture = (ROOT / 'tests/black_market/room_magic_native_test.cpp').read_text()
        fixture = fixture.replace('/* PALETTE */', palette).replace('/* PRODUCTION */', production)
        cpp = Path(temp) / (host + '.cpp'); cpp.write_text(fixture)
        binary = Path(temp) / host
        includes = [host, host + '/include', host + '/include/PR', host + '/src', host + '/assets',
                    host + ('/2s2h' if host == 'mm' else '/soh'), 'libultraship/include', 'combo']
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DCOMBO_BUILD', '-DF3DEX_GBI_2',
                        *(['-DOOT_ROOM_MAGIC_TEST'] if host == 'soh' else []), '-g',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie', '-I' + str(ROOT),
                        *['-I' + str(ROOT / p) for p in includes], str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})

    resources = (ROOT / 'combo/BlackMarketRoomMagicResources.h').read_text()
    body = block_from(resources, resources.index('class Resources {')) + ';'
    fixture = (ROOT / 'tests/black_market/room_magic_resources_test.cpp').read_text().replace('/* RESOURCES */', body)
    cpp = Path(temp) / 'resources.cpp'; cpp.write_text(fixture)
    binary = Path(temp) / 'resources'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DF3DEX_GBI_2', '-g',
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
                    '-I' + str(ROOT), '-I' + str(ROOT / 'libultraship/include'),
                    '-I' + str(ROOT / 'tests/black_market'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
