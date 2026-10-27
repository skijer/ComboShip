#!/usr/bin/env python3
"""Correct POC13's native player/keeper headers; retain all room/model assets."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

from build import room_spawn_contract, scene_resources, oot_scene_resources
from keeper_lean import MANIFEST, ROOM, keeper_contract, keeper_placement

BASELINE_SHA256 = 'bca46926d8474b3503653bdc243ee53ff0ab598c3db2b094827ad902a5ada4bf'
MM_SCENE = 'scenes/nonmq/skiejer_market/skiejer_market'
OOT_SCENE = 'scenes/shared/skiejer_market_oot/skiejer_market_oot'


def patch(source, output, report_path):
    source, output, report_path = map(Path, (source, output, report_path))
    before = hashlib.sha256(source.read_bytes()).hexdigest()
    assert before == BASELINE_SHA256, 'Expected the exact retained POC13 archive'
    assert len({p.resolve() for p in (source, output, report_path)}) == 3
    if output.exists() or report_path.exists():
        raise FileExistsError('Refusing to overwrite a retained input, candidate or report')
    with zipfile.ZipFile(source) as baseline:
        names = baseline.namelist()
        assert len(names) == len(set(names))
        resources = {name: baseline.read(name) for name in names}
        assert keeper_placement(resources[ROOM]) == {'position': [0, 1, 130], 'yaw': -32768, 'scale': .01}
        resources.update(scene_resources())
        resources.update(oot_scene_resources())
        manifest = json.loads(resources[MANIFEST])
        manifest['keeper'] = keeper_contract()
        manifest['orientation'] = {'customer_side': 'positive Z', 'native_door': [-10, 0, 420],
                                   'player_spawn': room_spawn_contract(), 'exit_z': 400,
                                   'source': 'Native Granny scene door/player and keeper actor records'}
        resources[MANIFEST] = (json.dumps(manifest, indent=2) + '\n').encode()
        changed = [n for n in names if resources[n] != baseline.read(n)]
        assert set(resources) == set(names)
        assert set(changed) == {MM_SCENE, ROOM, OOT_SCENE, MANIFEST}, changed
        output.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(output, 'x') as candidate:
            for info in baseline.infolist():
                candidate.writestr(info, resources[info.filename])
    with zipfile.ZipFile(source) as baseline, zipfile.ZipFile(output) as candidate:
        assert candidate.namelist() == names
        assert candidate.testzip() is None
        for name in names:
            assert (baseline.read(name) != candidate.read(name)) == (name in changed), name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == before
    report = {'baseline_sha256': before, 'candidate_sha256': hashlib.sha256(output.read_bytes()).hexdigest(),
              'candidate': str(output.resolve()), 'changed_resources': changed,
              'preserved_resources': len(names) - len(changed), 'keeper': keeper_contract(),
              'player_spawn': room_spawn_contract(),
              'preservation': 'Every model/room vertex, display list, texture, collision, animation, cosmetic and exterior payload is byte-identical. Only two scene spawns, the MM keeper actor list and manifest change.',
              'native_runtime_tested': False}
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(patch(args.source, args.output, args.report), indent=2))


if __name__ == '__main__': main()
