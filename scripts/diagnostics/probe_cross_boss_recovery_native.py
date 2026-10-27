#!/usr/bin/env python3
"""Exercise linked boss recovery exports without ROMs, a window or player saves."""
import argparse
import ctypes
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_dir', type=Path)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
modules = [ctypes.CDLL(str(args.game_dir.resolve() / name), mode=ctypes.RTLD_GLOBAL)
           for name in ['libsoh.so', 'lib2ship.so']]


def call(game, name, signature=(), values=(), restype=ctypes.c_int32):
    function = getattr(modules[game], name)
    function.argtypes = list(signature)
    function.restype = restype
    return function(*values)


def json_call(game, name, value=None):
    raw = call(game, name, () if value is None else (ctypes.c_char_p,),
               () if value is None else (json.dumps(value).encode(),), ctypes.c_char_p)
    assert raw is not None, name
    return json.loads(raw)


for game, prefix in enumerate(['SOH', 'MM']):
    call(game, prefix + '_InitRandoHeadless', restype=None)
options = {'dungeons': False, 'bosses': True, 'interiors': False, 'grottos': False, 'expanded': False}
plain = [json_call(game, prefix + '_DumpCrossDungeonCatalogueForOptions', options)
         for game, prefix in enumerate(['SOH', 'MM'])]
gohma, fire, water = plain[0][0], plain[0][4], plain[0][5]
settings = json_call(0, 'SOH_DumpRandoSettings')
# Native table types are ChildBoss=10 / AdultBoss=12. Destination is the
# immutable source's reverse, as recorded by native CreateEntranceOverrides.
overrides = [
    {'type': 12, 'index': fire['entrance']['index'], 'destination': fire['exit']['index'],
     'override': gohma['entrance']['index'], 'overrideDestination': -1},
    {'type': 10, 'index': gohma['exit']['index'], 'destination': gohma['entrance']['index'],
     'override': water['exit']['index'], 'overrideDestination': -1},
]
consolidated = {'oot': {'settings': settings}, 'entrances': {'oot': overrides},
                'crossDungeonEntrances': {'options': options}}
entries = json_call(0, 'SOH_DumpCrossDungeonCatalogueForSeed', consolidated) + plain[1]
assert 'deathCheckpoint' in entries[0], (len(entries), entries[:2], plain[0][:2])
assert entries[0]['deathCheckpoint'] == fire['deathCheckpoint']
assert entries[0]['returnTo'] == water['exit']


def install(swaps):
    permutation = list(range(len(entries)))
    for left, right in swaps:
        permutation[left], permutation[right] = permutation[right], permutation[left]
    seed = {'version': 2, 'enabled': True, 'options': options, 'dungeons': entries, 'permutation': permutation}
    for game, prefix in enumerate(['SOH', 'MM']):
        assert call(game, prefix + '_SetCrossDungeonRuntimeSeed', (ctypes.c_char_p,),
                    (json.dumps(seed).encode(),)) == 1
    return seed


parent = fire['deathCheckpoint']['index']
seed = install([(0, 9)])  # Gohma's logical source -> physical Goht; inverse returns MM -> OoT.
assert call(0, 'SOH_QueueCrossBossRecovery', (ctypes.c_int, ctypes.c_int32),
            (1, entries[9]['entrance']['index'])) == 1
assert call(0, 'SOH_QueueCrossBossRecovery', (ctypes.c_int, ctypes.c_int32),
            (0, entries[0]['entrance']['index'])) == 0
assert call(1, 'MM_QueueCrossBossRecovery', (ctypes.c_int, ctypes.c_int32),
            (0, entries[0]['entrance']['index'])) == 1
assert call(1, 'MM_QueueCrossBossRecovery', (ctypes.c_int, ctypes.c_int32), (0, 0xFFFF)) == 0
bad = json.loads(json.dumps(seed))
bad['dungeons'][0]['deathCheckpoint']['index'] = 0xFFFF
assert call(0, 'SOH_SetCrossDungeonRuntimeSeed', (ctypes.c_char_p,), (json.dumps(bad).encode(),)) == 0
assert call(0, 'SOH_QueueCrossBossRecovery', (ctypes.c_int, ctypes.c_int32),
            (1, entries[9]['entrance']['index'])) == 1  # Rejection preserved the installed map.
result = {'passed': True, 'ootDeathParent': parent, 'independentReverse': entries[0]['returnTo']['index'],
          'cases': ['native decoupled catalogue', 'runtime metadata accepted by both modules',
                    'foreign source requests validated in both directions', 'unknown request rejected',
                    'malformed recovery metadata rejected transactionally'],
          'limits': 'Native title/death functions are internal; actual spawn/consume is covered by production-body fixtures.'}
args.output.write_text(json.dumps(result, indent=2) + '\n')
print('linked native boss recovery PASS', json.dumps(result))
