#!/usr/bin/env python3
"""Exercise production Granny donor selection across native host/archive seams."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

source = (ROOT / 'combo/BlackMarketGrannyResources.h').read_text()
signatures = ['inline std::pair<std::string, std::shared_ptr<Ship::ResourceManager>> MapsOwner(',
              'inline std::pair<std::shared_ptr<Ship::ResourceManager>, std::shared_ptr<Ship::Archive>> NativeOwner(',
              'inline bool DependenciesAvailable(']
if 'inline bool MapsDependenciesAvailable(' in source:
    signatures.insert(0, 'inline bool MapsDependenciesAvailable(')
production = '\n'.join(block_from(source, source.index(signature)) for signature in signatures)
with tempfile.TemporaryDirectory(prefix='granny-availability-') as directory:
    cpp = Path(directory) / 'availability.cpp'
    cpp.write_text((ROOT / 'tests/black_market/granny_availability_test.cpp').read_text().replace(
        '/* PRODUCTION AVAILABILITY */', production))
    binary = Path(directory) / 'availability'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DF3DEX_GBI_2', '-g',
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
                    '-I' + str(ROOT), '-I' + str(ROOT / 'libultraship/include'),
                    str(cpp), '-o', str(binary)], check=True)
    failed = []
    for scenario in ('controls', 'native-shadow', 'partial-maps-owner'):
        result = subprocess.run([str(binary), scenario], env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
        if result.returncode:
            failed.append(scenario)
    if failed:
        raise SystemExit('Failed Granny dependency selections: ' + ', '.join(failed))
