#!/usr/bin/env python3
"""Check changed Wolf translation units with actual project/dependency headers."""
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags

for path in ('mm/mods/transformation_masks/wolf_link_form.cpp',
             'mm/mods/forms/wolf_link_host.cpp',
             'mm/expansions/ssbb/ssbb_skin.c',
             'mm/src/overlays/actors/ovl_player_actor/z_player.c'):
    cpp = path.endswith('.cpp')
    command = [os.environ.get('CXX' if cpp else 'CC', 'c++' if cpp else 'cc'),
               '-std=c++20' if cpp else '-std=gnu11', *flags(), '-fsyntax-only', str(ROOT / path)]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        print(result.stdout + result.stderr)
        raise SystemExit(result.returncode)
    warnings = result.stderr.count('warning:')
    print(f'PASS real-header syntax: {path} ({warnings} header/unity warnings)')
