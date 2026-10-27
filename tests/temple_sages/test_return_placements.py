#!/usr/bin/env python3
"""Verify return effects against accepted native collision (never edit inputs).

python tests/temple_sages/test_return_placements.py --base /path/oot.o2r \
    --djipi "/path/Djipi's 3DE - 20 Scenes Map.otr"
PYTHONPATH must include mpyq when the Djipi archive is MPQ.
"""
import argparse
import hashlib
import math
from pathlib import Path
import re
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools/temple_sages'))
import resource_geometry as geo

SCENES = ('spot05', 'spot17', 'spot06', 'spot11', 'spot02')
SCENE_IDS = (0x56, 0x61, 0x57, 0x5c, 0x53)
ROOMS = (0, 1, 0, 0, 0)
PADS = {1: (0, 421, 0, 3), 2: (-1045, -1243, 7457, 4), 3: (-847, 0, -2067, 5)}


def read_placements():
    text = (Path(__file__).resolve().parents[2] / 'combo/entrances/TempleSagesReturnPlacements.h').read_text()
    rows = re.findall(r'\{ (0x[0-9A-Fa-f]+), (\d+), \{ \{ ([^}]+) \}, (-?\d+) \} \}', text)
    assert len(rows) == 5
    return [(int(s, 16), int(r), tuple(float(x.strip().rstrip('f')) for x in pos.split(',')), int(yaw))
            for s, r, pos, yaw in rows]


def floor_hits(vertices, polys, x, z):
    hits = []
    for index, p in enumerate(polys):
        if p[5] < 23170 or p[1] & 0x4000:  # Entity-ignored is not support.
            continue
        a, b, c = [vertices[i & 8191] for i in p[1:4]]
        den = (b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
        if not den:
            continue
        u = ((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2]))/den
        v = ((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2]))/den
        if min(u, v, 1-u-v) >= -1e-7:
            hits.append((index, u*a[1]+v*b[1]+(1-u-v)*c[1]))
    return hits


def clip_height(poly, height, direction):
    out = []
    for a, b in zip(poly, poly[1:]+poly[:1]):
        da, db = (a[1]-height)*direction, (b[1]-height)*direction
        if da >= 0:
            out.append(a)
        if da*db < 0:
            t = da/(da-db)
            out.append(tuple(a[k]+t*(b[k]-a[k]) for k in range(3)))
    return out


def segment_distance(x, z, a, b):
    dx, dz = b[0]-a[0], b[2]-a[2]
    den = dx*dx+dz*dz
    t = max(0, min(1, ((x-a[0])*dx+(z-a[2])*dz)/den)) if den else 0
    return math.hypot(x-a[0]-t*dx, z-a[2]-t*dz)


def wall_clearance(vertices, polys, point, height=75):
    x, y, z = point
    closest = math.inf
    for p in polys:
        if p[5] >= 23170 or p[1] & 0x4000:
            continue
        clipped = [vertices[i & 8191] for i in p[1:4]]
        clipped = clip_height(clipped, y+5, 1)
        clipped = clip_height(clipped, y+height, -1)
        if len(clipped) < 3:
            continue
        ds = [segment_distance(x, z, a, b) for a, b in zip(clipped, clipped[1:]+clipped[:1])]
        # Projection containing center (including ceilings) has zero clearance.
        cross = [(b[0]-a[0])*(z-a[2])-(b[2]-a[2])*(x-a[0])
                 for a, b in zip(clipped, clipped[1:]+clipped[:1])]
        area = abs(sum(a[0]*b[2]-b[0]*a[2] for a, b in zip(clipped, clipped[1:]+clipped[:1])))
        if area > .001 and (min(cross) >= -1e-6 or max(cross) <= 1e-6):
            closest = 0
        else:
            closest = min(closest, *ds)
    return closest


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--base', type=Path, required=True)
    parser.add_argument('--djipi', type=Path, required=True)
    args = parser.parse_args()
    prefixes = tuple('scenes/shared/'+s+'_scene/' for s in SCENES)
    base, _ = geo.archive(args.base, prefixes + ('objects/object_mjin/',))
    donor, names = geo.archive(args.djipi, tuple('alt/'+p for p in prefixes))
    placements = read_placements()
    for route, (scene, room, point, yaw) in enumerate(placements):
        assert (scene, room) == (SCENE_IDS[route], ROOMS[route])
        stem = SCENES[route]
        name, data = next((n, d) for n, d in base.items() if stem in n and d[4:8] == b'LOCO')
        assert 'alt/'+name not in donor, 'Revalidate against replacement collision'
        assert not any(stem in n and ('Collision' in n or n.endswith('_scene')) for n in names), 'Scene override requires review'
        vertices, polys, _, _ = geo.collision_parts(data)
        x, y, z = point
        support = set()
        for radius in (0, 20, 64, 84):
            for index in range(64):
                px = x+radius*math.cos(index*math.tau/64)
                pz = z+radius*math.sin(index*math.tau/64)
                hits = [(i, h) for i, h in floor_hits(vertices, polys, px, pz) if abs(h-y) < 12]
                assert hits, (stem, 'unsupported effect/avatar disc', px, pz)
                support.update(i for i, _ in hits)
        clearance = wall_clearance(vertices, polys, point)
        assert clearance > 84, (stem, 'effect plus avatar wall clearance', clearance)
        portal_clearance = wall_clearance(vertices, polys, point, height=146)
        assert portal_clearance > 64, (stem, '146-unit portal headroom/width', portal_clearance)
        if route in PADS:
            px, py, pz, params = PADS[route]
            actor = struct.pack('<4h', 0x6e, px, py, pz)
            assert any(actor in d and struct.pack('<8h', 0x6e, px, py, pz, 0,
                       16384 if route == 3 else 0, 0, params) in d
                       for n, d in base.items() if f'{stem}_room_{room}' in n)
            # Real dynamic pad collision is scale1, rotated 90 degrees for Spirit.
            pad_vertices, _, _, _ = geo.collision_parts(base['objects/object_mjin/gWarpPadCol'])
            half_x = max(abs(v[0]) for v in pad_vertices)
            half_z = max(abs(v[2]) for v in pad_vertices)
            if route == 3:
                half_x, half_z = half_z, half_x
            nearest = math.hypot(max(0, abs(x-px)-half_x), max(0, abs(z-pz)-half_z))
            assert nearest > 84, (stem, 'native raised pad overlap', nearest)
        print(f'{stem} room={room} point={point} floor_polys={sorted(support)} wall_clearance={clearance:.2f} portal146_clearance={portal_clearance:.2f} collision_sha256={hashlib.sha256(data).hexdigest()}')
    # Source room0 has the circular torch chamber, including its dynamic door.
    shadow_room = base['scenes/shared/spot02_scene/spot02_room_0']
    actors = next(info['actors'] for cmd, _, _, info in geo.room_commands(shadow_room) if cmd == 1)
    entries = [struct.unpack_from('<8h', actors, i) for i in range(0, len(actors), 16)]
    torches = [a for a in entries if a[0] == 0x5e]
    assert len(torches) == 24
    assert any(a[:4] == (0x9c, 2676, 100, 80) and a[7] == 0x1f00 for a in entries)
    sx, _, sz = placements[4][2]
    nearest_torch = min(math.hypot(sx-a[1], sz-a[3]) for a in torches)
    assert nearest_torch > 84+15, nearest_torch  # Flame15, stand12.
    print(f'shadow nearest_torch_center={nearest_torch:.2f}; flame_radius=15; effect/avatar_disc=84')
    # Remain on the door-facing right (+Z), with the portal front aimed at the room center.
    assert placements[4][2][2] > 80
    shadow_yaw = placements[4][3]*math.tau/65536
    to_center = (2136-sx, 80-sz)
    assert (math.sin(shadow_yaw)*to_center[0]+math.cos(shadow_yaw)*to_center[1])/math.hypot(*to_center) > 0.9999
    for file in (args.base, args.djipi):
        print(file.name, hashlib.sha256(file.read_bytes()).hexdigest())
    print('PASS: five return effects; native pad offsets; 84-unit support discs; 75-unit avatar height; no collision overrides')

if __name__ == '__main__':
    main()
