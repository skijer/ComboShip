#!/usr/bin/env python3
"""Check the shop against native Granny's customer entrance and staff side."""
import argparse
import ast
import hashlib
import json
from pathlib import Path
import struct
import sys
import zipfile

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/black_market'))
from keeper_lean import keeper_contract, keeper_placement, room_faces
from native_preview import NativeKeeper
from counter import keeper_report

MM_SCENE = 'scenes/nonmq/skiejer_market/skiejer_market'
MM_ROOM = MM_SCENE + '_room_0'
OOT_SCENE = 'scenes/shared/skiejer_market_oot/skiejer_market_oot'
MANIFEST = 'custom/skiejer_market/manifest.json'


def actor_records(data):
    """Decode native scene command fields, stopping at actor/start positions."""
    at, records = 68, {}
    def take(fmt):
        nonlocal at
        value = struct.unpack_from('<' + fmt, data, at)
        at += struct.calcsize('<' + fmt)
        return value
    def string():
        size, = take('I')
        return take(str(size) + 's')[0]
    while at < len(data):
        cmd, = take('I')
        if cmd == 0x15: take('3B')
        elif cmd == 0x04:
            count, = take('I')
            for _ in range(count): string(); take('II')
        elif cmd == 0x0E:
            count, = take('I')
            records['doors'] = [take('4B5hH') for _ in range(count)]
        elif cmd in (0x19, 0x08): take('bi')
        elif cmd == 0x03: string()
        elif cmd == 0x06:
            count, = take('I'); take(str(2 * count) + 'b')
        elif cmd == 0x07: take('bh')
        elif cmd in (0x00, 0x01):
            count, = take('I')
            records['actors'] = [take('H6hH') for _ in range(count)]
            return records
        elif cmd == 0x16: take('b')
        elif cmd == 0x12: take('bb')
        elif cmd == 0x10: take('3b')
        elif cmd == 0x0A: take('4b'); string(); string()
        elif cmd == 0x0B:
            count, = take('I'); take(str(count) + 'H')
        else: raise ValueError((hex(cmd), at))
    raise AssertionError('Native actor/start-position list missing')


def floor_heights(faces, points):
    horizontal = faces[np.ptp(faces[:, :, 1], axis=1) < 1e-6]
    horizontal = horizontal[horizontal[:, 0, 1] <= 1.01]
    xz = horizontal[:, :, (0, 2)]
    a, b = xz[:, 1] - xz[:, 0], xz[:, 2] - xz[:, 0]
    determinant = a[:, 0] * b[:, 1] - a[:, 1] * b[:, 0]
    valid = abs(determinant) > 1e-8
    xz, a, b, determinant, horizontal = [v[valid] for v in (xz, a, b, determinant, horizontal)]
    heights = []
    for point in points:
        delta = point - xz[:, 0]
        u = (delta[:, 0] * b[:, 1] - delta[:, 1] * b[:, 0]) / determinant
        v = (a[:, 0] * delta[:, 1] - a[:, 1] * delta[:, 0]) / determinant
        hits = (u >= -1e-6) & (v >= -1e-6) & (u + v <= 1.000001)
        assert hits.any(), ('No physical floor under room position', point.tolist())
        heights.append(horizontal[hits, 0, 1].max())
    return np.array(heights)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('pack', type=Path)
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--native-oot', type=Path, required=True)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    before = hashlib.sha256(args.baseline.read_bytes()).hexdigest()
    assert before == 'bca46926d8474b3503653bdc243ee53ff0ab598c3db2b094827ad902a5ada4bf'
    with zipfile.ZipFile(args.native_oot) as native:
        vanilla = actor_records(native.read('scenes/shared/mahouya_scene/mahouya_scene'))
        native_keeper = actor_records(native.read('scenes/shared/mahouya_scene/mahouya_room_0'))['actors'][0]
    spawn, door = vanilla['actors'][0], vanilla['doors'][0]
    with zipfile.ZipFile(args.pack) as pack, zipfile.ZipFile(args.baseline) as baseline:
        for path in (MM_SCENE, OOT_SCENE):
            actual = actor_records(pack.read(path))['actors'][0]
            assert actual[1:4] == spawn[1:4] and actual[5] == spawn[5], \
                (path, 'Player must enter from the native customer doorway, facing the counter', actual, spawn)
        contract = keeper_contract()
        placement = keeper_placement(pack.read(MM_ROOM))
        assert placement == {key: contract[key] for key in ('position', 'yaw', 'scale')}
        assert contract['position'][2] < 93 < 123 < spawn[3] < door[7]
        assert contract['yaw'] == native_keeper[5] == 0, 'Keeper must face customers, not the back room'
        names = baseline.namelist()
        assert names == pack.namelist(), 'Resource set and order must be preserved'
        changed = [n for n in names if baseline.read(n) != pack.read(n)]
        assert set(changed) == {MM_SCENE, MM_ROOM, OOT_SCENE, MANIFEST}, changed
        assert json.loads(pack.read(MANIFEST))['keeper'] == contract
        counter = keeper_report(pack)
        faces = np.array(room_faces(pack))
        keeper = NativeKeeper(pack)
        min_gap, max_gap = float('inf'), 0
        for frame in range(keeper.frame_count):
            points = keeper.position_triangles(frame).reshape(-1, 3)
            soles = np.unique(points[points[:, 1] <= 2], axis=0)
            assert len(soles)
            gaps = soles[:, 1] - floor_heights(faces, soles[:, (0, 2)])
            assert gaps.min() >= 0 and gaps.max() <= 2, 'Keeper soles must stay close to supported floor'
            min_gap, max_gap = min(min_gap, float(gaps.min())), max(max_gap, float(gaps.max()))
        walkway = np.array([[spawn[1], z] for z in np.linspace(160, door[7], 53)])
        floor_heights(faces, walkway)
    constants = {}
    for node in ast.parse((ROOT / 'tools/black_market/preview.py').read_text()).body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == 'OVERVIEW_VIEW':
                    constants[target.id] = ast.literal_eval(node.value)
    eye, target = constants['OVERVIEW_VIEW']
    assert eye[2] > 123 and target[2] < 93, 'Preview must look from the customer side toward the keeper'
    assert hashlib.sha256(args.baseline.read_bytes()).hexdigest() == before
    report = {'pack_sha256': hashlib.sha256(args.pack.read_bytes()).hexdigest(),
              'native_player_spawn': spawn, 'native_door': door, 'native_keeper': native_keeper,
              'keeper': contract, 'keeper_frames': keeper.frame_count, 'counter': counter,
              'sole_floor_gap_range': [min_gap, max_gap], 'walkway_samples': len(walkway),
              'changed_resources': changed, 'preserved_resources': len(names) - len(changed),
              'preview_eye': eye, 'preview_target': target, 'native_runtime_tested': False}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS room orientation: native customer spawn/yaw in both engines; staff-side keeper; 32 lean/counter/floor checks; doorway approach; exactly four header/manifest changes')


if __name__ == '__main__': main()
