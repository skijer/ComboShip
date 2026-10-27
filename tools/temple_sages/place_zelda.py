#!/usr/bin/env python3
"""Append age-specific cutscene Zelda placements to the exact accepted POC3.

This makes a full replacement addon. It never regenerates accepted scenery,
edits the input archives, or adds assets for the source-owned native aura.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import resource_geometry as geo

TEMPLE = 'scenes/shared/tokinoma_scene/'
MAIN = TEMPLE + 'tokinoma_room_0'
ADULT = TEMPLE + 'tokinoma_room_0ZeldaAdult'
SCENE = TEMPLE + 'tokinoma_scene'
COLLISION = TEMPLE + 'tokinoma_sceneCollisionHeader_0032F8'
MANIFEST = 'custom/temple_sages/manifest.json'
PARENT_SHA256 = '2ae8cdd5ea919e299eefcf0a9730b97d6badd15d20b01692dcc4df4556e54e42'
BASE_SHA256 = 'ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b'


def sha256(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def leading_alternates(data):
    """Decode the actual native roots' first command, including empty refs."""
    if data[4:8] != b'MORO' or struct.unpack_from('<I', data, 68)[0] != 0x18:
        raise ValueError('expected a native scene/room alternate-header root')
    count = struct.unpack_from('<I', data, 72)[0]
    offset, paths = 76, []
    for _ in range(count):
        path, offset = geo.read_string(data, offset)
        paths.append(path)
    return paths


def center_floor(data):
    """Choose the topmost entity-supporting collision triangle at (0,-2400)."""
    vertices, polygons, _, _ = geo.collision_parts(data)
    hits = []
    for index, polygon in enumerate(polygons):
        if polygon[5] < 23170 or polygon[1] & 0x4000:
            continue
        a, b, c = [vertices[i & 0x1fff] for i in polygon[1:4]]
        denominator = (b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
        if not denominator:
            continue
        u = ((b[2]-c[2])*(-c[0])+(c[0]-b[0])*(-2400-c[2]))/denominator
        v = ((c[2]-a[2])*(-c[0])+(a[0]-c[0])*(-2400-c[2]))/denominator
        if min(u, v, 1-u-v) >= -1e-7:
            hits.append((index, u*a[1]+v*b[1]+(1-u-v)*c[1]))
    if not hits:
        raise ValueError('accepted center has no entity-supporting collision')
    height = max(y for _, y in hits)
    if abs(height-round(height)) > 1e-7:
        raise ValueError('center floor cannot be represented by an integer actor position')
    return round(height), [index for index, y in hits if abs(y-height) < 1e-7], hits


def string(value):
    encoded = value.encode()
    return struct.pack('<I', len(encoded)) + encoded


def place_headers(data, height):
    """Split the shared normal header; retain each accepted non-target slice."""
    paths = leading_alternates(data)
    if paths[:3] != ['', '', '']:
        raise ValueError('accepted normal room setups no longer share their main header')
    paths[1] = ADULT  # Setup 3 intentionally keeps native setup-2 fallback.
    new_dispatch = struct.pack('<II', 0x18, len(paths)) + b''.join(map(string, paths))
    child, adult = [], []
    actor_count = 0
    commands = geo.room_commands(data)
    for command, start, end, info in commands:
        raw = data[start:end]
        if command == 0x18:
            child.append(new_dispatch)
            continue  # The new adult leaf must not dispatch recursively.
        if command == 1:
            actor_count += 1
            original = info['actors']
            if any(struct.unpack_from('<H', original, offset)[0] == 0x2a
                   for offset in range(0, len(original), 16)):
                raise ValueError('accepted room already contains a cutscene Zelda actor')
            prefix = struct.pack('<II', 1, len(original)//16+1) + original
            for target, params in ((child, 0x7D23), (adult, 0x7F34)):
                target.append(prefix + struct.pack('<HhhhhhhH', 0x2a, 0, height, -2400, 0, 0, 0, params))
        else:
            child.append(raw)
            adult.append(raw)
    if actor_count != 1:
        raise ValueError('expected exactly one room actor list')
    return (data[:64] + struct.pack('<I', len(child)) + b''.join(child),
            data[:64] + struct.pack('<I', len(adult)) + b''.join(adult))


def build(input_path, base_path, output_path):
    """Write deterministic POC4 from immutable accepted POC3 and native input."""
    input_path, base_path, output_path = map(Path, (input_path, base_path, output_path))
    if output_path.resolve() in (input_path.resolve(), base_path.resolve()):
        raise ValueError('output must not overwrite an input archive')
    if sha256(input_path) != PARENT_SHA256:
        raise ValueError('input is not the exact accepted POC3 addon')
    if sha256(base_path) != BASE_SHA256:
        raise ValueError('base is not the exact accepted native oot.o2r')
    with zipfile.ZipFile(input_path) as pack:
        names = pack.namelist()
        if len(names) != 1986 or len(set(names)) != len(names):
            raise ValueError('accepted addon membership changed')
        resources = {name: pack.read(name) for name in names}
    with zipfile.ZipFile(base_path) as base:
        scene = base.read(SCENE)
    if leading_alternates(scene)[:3] != ['', '', '']:
        raise ValueError('native normal scene resolution changed')
    if resources[MAIN] != resources['alt/'+MAIN] or resources[COLLISION] != resources['alt/'+COLLISION]:
        raise ValueError('accepted canonical/Alt room or collision mismatch')
    height, support, hits = center_floor(resources[COLLISION])
    main, adult = place_headers(resources[MAIN], height)
    for prefix in ('', 'alt/'):
        resources[prefix+MAIN] = main
        resources[prefix+ADULT] = adult

    manifest = json.loads(resources[MANIFEST])
    manifest['zelda_placement'] = dict(
        version=1, candidate='Zelda_POC4', parent_filename=input_path.name,
        parent_sha256=PARENT_SHA256, base_sha256=BASE_SHA256,
        actor_id=0x2a, position=[0, height, -2400], yaw=0,
        floor_collision=COLLISION, floor_support_polygons=support,
        floor_hits=[dict(polygon=index, height=y) for index, y in hits],
        native_aura_resources_added=False, runtime_verified=False,
        added_resources=[ADULT, 'alt/'+ADULT],
        changed_parent_resources=[MAIN, 'alt/'+MAIN, MANIFEST],
        normal_setups=[dict(setup=setup, age='child' if setup < 2 else 'adult',
                            time='day' if setup % 2 == 0 else 'night', scene_header=SCENE,
                            room_header=MAIN if setup < 2 else ADULT,
                            params=0x7D23 if setup < 2 else 0x7F34,
                            resolution=('main', 'empty_child_night_uses_main', 'explicit_adult_day',
                                        'empty_adult_night_uses_adult_day')[setup])
                       for setup in range(4)],
        preserved_cutscene_room_headers=leading_alternates(resources[MAIN])[3:])
    manifest['resources'] = {name: hashlib.sha256(data).hexdigest()
                             for name, data in resources.items() if name != MANIFEST}
    resources[MANIFEST] = json.dumps(manifest, sort_keys=True, separators=(',', ':')).encode()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output_path, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as pack:
        for name, data in sorted(resources.items()):
            info = zipfile.ZipInfo(name, (2026, 10, 27, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            pack.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    return manifest


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--base', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = build(args.input, args.base, args.output)
    print(json.dumps(dict(output=str(args.output), sha256=sha256(args.output),
                          members=len(result['resources'])+1, placement=result['zelda_placement']), sort_keys=True))
