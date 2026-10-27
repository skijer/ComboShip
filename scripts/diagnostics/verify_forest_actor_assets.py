#!/usr/bin/env python3
"""Read-only native/pack contract check. This is not an in-game visual test."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import zipfile

def string(raw, offset):
    size, = struct.unpack_from('<I', raw, offset)
    return raw[offset+4:offset+4+size].decode().removeprefix('__OTR__'), offset+4+size

def limb(raw):
    _, offset = string(raw, 66)
    _, offset = string(raw, offset + 6)
    offset += 18
    for _ in range(2):
        _, offset = string(raw, offset)
    near, offset = string(raw, offset)
    _, offset = string(raw, offset)
    return struct.unpack_from('<hhhBB', raw, offset), near

def rig(raw, load, count, matrices):
    assert struct.unpack_from('<BBIIBI', raw, 64) == (1, 1, count, matrices, 1, count)
    paths, offset = [], 79
    for _ in range(count):
        path, offset = string(raw, offset)
        paths.append(path)
    limbs = [limb(load(path)) for path in paths]
    assert sum(bool(display) for _, display in limbs) == matrices
    hierarchy = [(values[3], values[4]) for values, _ in limbs]
    assert all(index == 255 or index < count for row in hierarchy for index in row)
    return hierarchy

def animation(raw, joints):
    assert raw[4:8] == b'MNAO' and struct.unpack_from('<I', raw, 64)[0] == 0
    frames, count = struct.unpack_from('<hI', raw, 68)
    assert frames > 0
    values = struct.unpack_from('<' + 'h'*count, raw, 74)
    offset = 74 + count*2
    assert struct.unpack_from('<I', raw, offset)[0] == joints
    indices = [struct.unpack_from('<HHH', raw, offset+4+i*6) for i in range(joints)]
    limit, = struct.unpack_from('<H', raw, offset+4+joints*6)
    assert limit <= count
    assert all(index + (1 if index < limit else frames) <= count for row in indices for index in row)
    root = [[values[index + (0 if index < limit else frame)] for index in indices[0]] for frame in range(frames)]
    return frames, root

def floor_at(raw, x, z, ceiling):
    """Read native collision triangles; probe only upward facing floors."""
    count, = struct.unpack_from('<I', raw, 76)
    vertices = [struct.unpack_from('<hhh', raw, 80 + i*6) for i in range(count)]
    offset = 80 + count*6
    count, = struct.unpack_from('<I', raw, offset)
    floors = []
    for i in range(count):
        poly = struct.unpack_from('<HHHHhhhh', raw, offset + 4 + i*16)
        if poly[5] <= 0:
            continue
        a, b, c = [vertices[index & 0x1FFF] for index in poly[1:4]]
        determinant = (b[2]-c[2])*(a[0]-c[0]) + (c[0]-b[0])*(a[2]-c[2])
        if not determinant:
            continue
        u = ((b[2]-c[2])*(x-c[0]) + (c[0]-b[0])*(z-c[2])) / determinant
        v = ((c[2]-a[2])*(x-c[0]) + (a[0]-c[0])*(z-c[2])) / determinant
        height = u*a[1] + v*b[1] + (1-u-v)*c[1]
        if min(u, v, 1-u-v) >= -1e-6 and height <= ceiling:
            floors.append(height)
    assert floors, 'No native floor at the requested placement'
    return max(floors)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--oot', type=Path, required=True)
    parser.add_argument('--mm', type=Path, required=True)
    parser.add_argument('--saria', type=Path, action='append', default=[])
    args = parser.parse_args()
    report = {'inputs': {str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                         for p in [args.oot, args.mm, *args.saria]}, 'checks': []}
    with zipfile.ZipFile(args.mm) as mm, zipfile.ZipFile(args.oot) as oot:
        rig(mm.read('objects/object_stk/gSkullKidSkel'), mm.read, 21, 20)
        frames, root = animation(mm.read('objects/object_stk2/gSkullKidRecliningFloatAnim'), 22)
        assert frames == 40
        for part in ['NormalHead', 'NormalEyes', 'MajorasMask1', 'OpenLeftHand']:
            mm.read('objects/object_stk/gSkullKid' + part + 'DL')
        skullFloor = floor_at(mm.read('scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODSCollisionHeader_005DF8'),
                              2200, 200, 500)
        assert 89 < skullFloor < 90
        report['native_forest_skull_kid'] = {'x': 2200, 'floor_y': skullFloor, 'z': 200,
            'actor_y': skullFloor + 35, 'yaw': -0x5000, 'clip_frames': frames, 'limbs': 21, 'matrices': 20,
            'root_y_range': [min(v[1] for v in root), max(v[1] for v in root)]}
        report['checks'].append('native Skull Kid rig/reclining clip/head/mask/hands; room-one raised outer-edge floor')
        rig(mm.read('objects/object_mnk/gMonkeySkel'), mm.read, 22, 20)
        for clip, count in [('0105DC', 12), ('00B864', 71), ('008814', 16)]:
            frames, root = animation(mm.read('objects/object_mnk/object_mnk_Anim_'+clip), 23)
            assert frames == count
            if clip == '008814':
                assert max(row[1] for row in root) > min(row[1] for row in root)
                report['jump_world_lift'] = (max(row[1] for row in root) - min(row[1] for row in root)) * .012
        report['checks'].append('native monkey rig and all three clips; jumping vertical root motion retained')
        horse = mm.read('objects/object_horse_link_child/gEponaSkel')
        assert struct.unpack_from('<BBIIBI', horse, 64) == (0, 1, 46, 0, 1, 46)
        offset = 79
        for _ in range(46):
            path, offset = string(horse, offset)
            part = mm.read(path)
            assert part[64] == 3 and part[65] in {0, 4, 5, 11}
            display, _ = string(part, 66)
            if display:
                mm.read(display)
        assert animation(mm.read('objects/object_horse_link_child/gEponaIdleAnim'), 47)[0] == 70
        for eye in ['Open', 'Half', 'Closed']:
            mm.read('objects/object_horse_link_child/gEponaEye'+eye+'Tex')
        angle = 12288 * math.tau / 65536
        x = -695 + 145*math.cos(angle) - 90*math.sin(angle)
        z = -313 - 145*math.sin(angle) - 90*math.cos(angle)
        floor = floor_at(mm.read('scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODSCollisionHeader_005DF8'), x, z, 120)
        assert floor == 0
        report['native_forest_epona'] = {'x': x, 'floor_y': floor, 'z': z, 'idle_frames': 70, 'skin_limbs': 46}
        report['checks'].append('native Epona Skin limbs/idle/blink assets; opposite portal flank has native floor')
        native = rig(oot.read('objects/object_sa/gSariaSkel'), oot.read, 16, 15)
        frames, root = animation(oot.read('objects/object_sa/gSariaSitting3Anim'), 17)
        assert frames == 24 and root[0] == [0, 7903, -350]
        report['seated_root_y_world'] = [(row[1]-7003)*.01 for row in root]
        floor = floor_at(mm.read('scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUCollisionHeader_00DCE8'), -720, 2000, 300)
        assert floor == -77
        report['native_field_placement'] = {'x': -720, 'floor_y': floor, 'z': 2000}
        ns = 'objects/object_sa/NativeNpcV1'
        for path in args.saria:
            with zipfile.ZipFile(path) as pack:
                def load(name):
                    for candidate in ['alt/'+name, name] if not name.startswith('alt/') else [name]:
                        if candidate in pack.namelist(): return pack.read(candidate)
                        if candidate in oot.namelist(): return oot.read(candidate)
                    raise KeyError(name)
                assert rig(pack.read(ns+'/Skel'), load, 16, 15) == native
                assert rig(pack.read('alt/'+ns+'/Skel'), load, 16, 15) == native
                for clip in ['gSariaWaitArmsToSideAnim', 'gSariaHandsBehindBackWaitAnim', 'gSariaPlayingOcarinaAnim', 'gSariaSitting3Anim']:
                    original = 'objects/object_sa/'+clip
                    frames, root = animation(oot.read(original), 17)
                    assert frames == (65 if clip == 'gSariaPlayingOcarinaAnim' else 24)
                    fitted = 'alt/'+ns+'/PoseAnimations/'+original
                    if fitted in pack.namelist():
                        count, fittedRoot = animation(pack.read(fitted), 17)
                        assert count == frames and fittedRoot == root
                heads = [pack.read('alt/'+ns+'/Faces/L'+str(i)+'R'+str(i)+'M0') for i in range(3)]
                assert len(set(heads)) == 3
                for clip in ['gSariaHandsBehindBackWaitAnim', 'gSariaSitting3Anim']:
                    head = 'alt/'+ns+'/PoseAnimations/objects/object_sa/'+clip+'/Limbs/Limb15'
                    assert head not in pack.namelist(), 'Pose head override needs separate blink validation'
                report['checks'].append(path.name + ': native/private hierarchy, matrix counts, blink heads and fitted root parity')
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
