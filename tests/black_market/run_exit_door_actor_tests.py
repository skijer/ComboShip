#!/usr/bin/env python3
"""Exercise actual native door actor bodies with the shared exit contract."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

with tempfile.TemporaryDirectory(prefix='market-exit-door-') as temporary:
    for host in ('mm', 'soh'):
        actor = (ROOT / host / 'src/code/z_actor.c').read_text()
        native = (ROOT / host / 'src/overlays/actors/ovl_En_Door/z_en_door.c').read_text()
        bodies = [block_from(actor, actor.index(signature)) for signature in (
            'void Actor_WorldToActorCoords(', 'void Actor_SpawnTransitionActors(')]
        bodies += [block_from(native, native.index(signature)) for signature in (
            'void EnDoor_Destroy(Actor* thisx, PlayState* play) {',
            'void EnDoor_Idle(EnDoor* this, PlayState* play) {', 's32 EnDoor_OverrideLimbDraw(')]
        fixture = (ROOT / 'tests/black_market/exit_door_actor_test.cpp').read_text()
        if host == 'mm':
            start = actor.index('s16 D_801AED48[] = {')
            fixture = fixture.replace('/* NATIVE HALF DAYS */', actor[start:actor.index('};', start) + 2])
        fixture = fixture.replace('/* NATIVE FUNCTIONS */', re.sub(r'\bthis\b', 'current', '\n'.join(bodies)))
        cpp = Path(temporary) / (host + '.cpp'); cpp.write_text(fixture)
        binary = Path(temporary) / host
        includes = [host, host + '/include', host + '/include/PR', host + '/src', host + '/assets',
                    host + ('/2s2h' if host == 'mm' else '/soh'), 'libultraship/include', 'combo']
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DCOMBO_BUILD', '-DF3DEX_GBI_2',
                        *(['-DEXIT_DOOR_OOT'] if host == 'soh' else []), '-g',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
                        *['-I' + str(ROOT / path) for path in includes], str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
