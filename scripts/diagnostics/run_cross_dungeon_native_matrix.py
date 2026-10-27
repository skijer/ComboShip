#!/usr/bin/env python3
"""Exercise linked engines: generation, exact replay, native ER and disabled parity.

Build comborando from the accepted baseline separately and supply its executable.
Both executables load the candidate game modules from game_dir; the baseline runs
with cross routing disabled. No window, player saves or ROM archives are required.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('game_dir', type=Path)
parser.add_argument('--baseline-comborando', required=True, type=Path)
parser.add_argument('--comborando', type=Path)
parser.add_argument('--output', type=Path, default=Path('cross-dungeon-native-matrix'))
args = parser.parse_args()
game = args.game_dir.resolve()
exe = (args.comborando or game / ('comborando.exe' if os.name == 'nt' else 'comborando')).resolve()
control = args.baseline_comborando.resolve()
out = args.output.resolve()
out.mkdir(parents=True, exist_ok=True)


def run(name, argv, binary=exe):
    begin = time.monotonic()
    with (out / (name + '.log')).open('w') as log:
        result = subprocess.run([str(binary), *argv], cwd=game, stdout=log, stderr=subprocess.STDOUT)
    print(name, 'exit', result.returncode, 'seconds', round(time.monotonic() - begin, 2), flush=True)
    if result.returncode:
        raise SystemExit('FAILED ' + name + '; see ' + str(out / (name + '.log')))
    if '--playthrough' in argv:
        return None
    seed = json.loads((game / 'saves/combo/comborando.spoiler.json').read_text())
    (out / (name + '.json')).write_text(json.dumps(seed, indent=2) + '\n')
    return seed


common = ['--seed', 'cross-dungeon-native-off', '--starting-game', 'oot', '--shared-items', 'none']
first = run('enabled-native-off-oot', [*common, '--cross-dungeon-entrances=1'])
assert first.get('crossDungeonEntrances', {}).get('enabled') is True
assert len(first['crossDungeonEntrances']['dungeons']) == 16
second = run('enabled-native-off-determinism', [*common, '--cross-dungeon-entrances=1'])
assert first == second, 'same-seed determinism mismatch'
replay = run('enabled-recorded-replay', ['--settings', str(out / 'enabled-native-off-oot.json'),
                                        '--master-seed', str(first['masterSeed']), *common])
assert replay == first, 'recorded map / native layout / regeneration mismatch'
run('enabled-native-off-playthrough', ['--playthrough', str(out / 'enabled-native-off-oot.json')])
run('enabled-native-off-mm', ['--seed', 'cross-dungeon-mm-start', '--starting-game', 'mm',
                              '--shared-items', 'none', '--cross-dungeon-entrances=1'])
run('enabled-native-off-mm-playthrough', ['--playthrough', str(out / 'enabled-native-off-mm.json')])


def fixture(name, values):
    data = json.loads(json.dumps(first))
    data.pop('crossDungeonEntrances', None)
    settings = data['oot']['settings']
    for key, value in values.items():
        resolved = [k for k in settings if k.endswith('.' + key)]
        assert len(resolved) == 1, (key, resolved)
        settings[resolved[0]] = value
    path = out / (name + '-settings.json')
    path.write_text(json.dumps(data, indent=2) + '\n')
    return path


mixed = {'ShuffleDungeonsEntrances': 2, 'ShuffleBossEntrances': 2,
         'ShuffleGrottosEntrances': 1, 'MixedEntrances': 1,
         'MixDungeons': 1, 'MixBosses': 1, 'MixGrottos': 1}
# The original coupled+Ganon fixture also fails the baseline generator with cross
# routing disabled. Keep that rejection documented in the POC ledger; use the
# passing coupled 16-pool control here, with Ganon covered by the other profiles.
for name, values, pool_size, seed_name in [
    ('native-dungeon-ganon', {'ShuffleDungeonsEntrances': 2}, 17, 'cross-dungeon-native-dungeon-ganon'),
    ('native-mixed-coupled-no-ganon', {**mixed, 'ShuffleDungeonsEntrances': 1, 'DecoupleEntrances': 0},
     16, 'cross-dungeon-native-mixed-coupled'),
    ('native-mixed-decoupled', {**mixed, 'DecoupleEntrances': 1}, 17, 'cross-dungeon-native-mixed-decoupled'),
]:
    path = fixture(name, values)
    seed = run(name, ['--settings', str(path), '--seed', seed_name,
                      '--starting-game', 'oot', '--shared-items', 'none',
                      '--cross-dungeon-entrances=1'])
    assert len(seed['crossDungeonEntrances']['dungeons']) == pool_size, (name, 'unexpected dungeon pool')
    run(name + '-playthrough', ['--playthrough', str(out / (name + '.json'))])

disabled = ['--seed', 'cross-dungeon-disabled-control', '--starting-game', 'oot', '--shared-items', 'none']
baseline = run('disabled-baseline', disabled, control)
candidate = run('disabled-candidate', [*disabled, '--cross-dungeon-entrances=0'])
assert baseline == candidate, 'disabled seed differs from original baseline generator'
assert 'crossDungeonEntrances' not in candidate
(out / 'RESULT.json').write_text(json.dumps({
    'sameSeedDeterminism': True,
    'recordedMapReplay': True,
    'disabledBaselineExactMatch': True,
    'nativeMatrix': ['off/OOT', 'off/MM', 'dungeon+Ganon',
                     'mixed+boss+grotto/coupled, Ganon excluded', 'mixed+boss+grotto/decoupled+Ganon'],
    'knownRejectedControl': 'coupled+Ganon seed cross-dungeon-native-mixed-coupled also fails baseline generation',
}, indent=2) + '\n')
print('native generation/replay/disabled-baseline matrix PASS', flush=True)
