#!/usr/bin/env python3
"""Actual donor emitted triangles versus authored Lake vault and optional final valley.

CPU asset/geometry proof, not a renderer or configured gameplay test. No asset is modified.
"""
from pathlib import Path
import argparse
import json
import math
import struct
import subprocess
import sys
import tempfile
import zipfile
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_pose_behavior import Rig, animation
from render_anju_preview import commands, crc64


def posed_mesh(resources, skeleton, clip, prop=None):
    rig = Rig(resources, skeleton)
    hashes = {crc64(name): name for name in resources}
    slots = [i for i, dl in enumerate(rig.dlists) if dl]
    order = []
    def visit(i):
        order.append(i)
        if rig.children[i] != 255:
            visit(rig.children[i])
        if rig.siblings[i] != 255:
            visit(rig.siblings[i])
    visit(0)
    draws = [(i, rig.dlists[i]) for i in order]
    if prop:
        # Native limb callbacks are one-based; archive limb arrays zero-based.
        index, path = prop
        draws.insert(next(n for n, row in enumerate(draws) if row[0] == index - 1) + 1,
                     (index - 1, path))
    def display(path):
        for op, w0, w1, hashed in commands(resources[path]):
            if op == 0x31:
                yield from display(hashes[hashed])
            else:
                yield op, w0, w1, hashed
    meshes = []
    for frame in animation(resources[clip]):
        matrices = np.asarray(rig.world(frame))
        cache, triangles = {}, []
        for joint, path in draws:
            if not path:
                continue
            for op, w0, w1, hashed in display(path):
                if op == 0xDA:
                    assert w1 >> 24 == 13
                    joint = slots[(w1 & 0xffffff) // 64]
                elif op == 0x32:
                    count = (w0 >> 12) & 255
                    start = ((w0 >> 1) & 127) - count
                    data = resources[hashes[hashed]]
                    for k in range(count):
                        v = (*struct.unpack_from('<hhh', data, 72 + w1 + k * 16), 1)
                        cache[start + k] = (matrices[joint] @ v)[:3] * .01
                elif op in (5, 6):
                    for word in (w0, w1) if op == 6 else (w0,):
                        triangles.append([cache[((word >> s) & 255) // 2] for s in (16, 8, 0)])
                elif op in (1, 0xD8):
                    raise AssertionError(f'Unimplemented geometry command {op:x}')
        meshes.append(np.asarray(triangles))
    return meshes


def geometry(terrain_root):
    source = r'''
#include "actors/RegionalActorPlacement.h"
#include "entrances/LakeHyliaBridge.h"
#include <cstdio>
#ifdef VALLEY
#include "FINAL_SNOWHEAD_HEADER"
#endif
int main() {
    const auto row = ComboRegionalActors::Rows(6, true)[0];
    const double angle = ComboLakeHylia::kYaw * (6.28318530717958647692 / 65536.0);
    printf("{\"lake\":[%g,%g,%g,%g,%g,%g,%g],\"valley\":[",
      ComboLakeHylia::kMouth.x + std::sin(angle)*row.front + std::cos(angle)*row.across,
      ComboLakeHylia::kMouth.y,
      ComboLakeHylia::kMouth.z + std::cos(angle)*row.front - std::sin(angle)*row.across, ComboLakeHylia::kBackX,
      ComboLakeHylia::kFacadeX, ComboLakeHylia::kMouth.z, float(ComboLakeHylia::kYaw));
#ifdef VALLEY
    bool firstValley = true;
    for (auto t : ComboFountainValley::kTriangles) {
        printf("%s[[%g,%g,%g],[%g,%g,%g],[%g,%g,%g]]", firstValley ? "" : ",",
          t.a.x,t.a.y,t.a.z,t.b.x,t.b.y,t.b.z,t.c.x,t.c.y,t.c.z); firstValley=false;
    }
#endif
#ifdef VALLEY
    printf("],\"endpoint\":[%g,%g,%g,%d],\"bridge\":[", ComboFountainSnowhead::kMouth.x,
      ComboFountainSnowhead::kMouth.y, ComboFountainSnowhead::kMouth.z, ComboFountainSnowhead::kYaw);
    auto bridge = ComboFountainSnowhead::BuildMesh();
    for (size_t i=0; i<bridge.count; i++) {
        auto t = bridge.triangles[i];
        printf("%s[[%g,%g,%g],[%g,%g,%g],[%g,%g,%g]]", i ? "," : "",
          t.a.x,t.a.y,t.a.z,t.b.x,t.b.y,t.b.z,t.c.x,t.c.y,t.c.z);
    }
#endif
    const auto ghost = ComboRegionalActors::Rows(7, true)[0];
    printf("],\"ghost\":[%g,%g,%g],\"opening\":[", ghost.across,ghost.front,ghost.lift);
    bool first = true;
    for (auto p : ComboLakeHylia::kOpeningSection) {
        printf("%s[%g,%g]", first ? "" : ",",p.z,p.y); first=false;
    }
    puts("]}");
}
'''
    if terrain_root:
        source = source.replace('FINAL_SNOWHEAD_HEADER',
                                str(terrain_root / 'combo/entrances/FountainSnowheadBridge.h'))
    with tempfile.TemporaryDirectory(prefix='regional-geometry-') as d:
        path = Path(d) / 'geometry.cpp'
        path.write_text(source)
        flags = ['-DVALLEY', '-I' + str(terrain_root / 'combo')] if terrain_root else []
        subprocess.run(['c++', '-std=c++20', '-I' + str(ROOT / 'combo'), *flags,
                        str(path), '-o', d + '/geometry'], check=True)
        return json.loads(subprocess.check_output([d + '/geometry']))


def world(mesh, origin, yaw):
    angle = yaw * math.pi / 32768
    s, c = math.sin(angle), math.cos(angle)
    return mesh @ np.array([[c, 0, -s], [0, 1, 0], [s, 0, c]]) + origin


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--terrain-root', type=Path)
    args = parser.parse_args()
    with zipfile.ZipFile(args.archive) as archive:
        resources = {n: archive.read(n) for n in archive.namelist()
                     if n.startswith(('objects/object_zog/', 'objects/object_gg/'))}
    g = geometry(args.terrain_root)
    x, y, z, back, front, center, yaw = g['lake']
    frames = posed_mesh(resources, 'objects/object_zog/object_zog_Skel_029170',
                        'objects/object_zog/object_zog_Anim_00931C',
                        (17, 'objects/object_zog/object_zog_DL_0280A8'))
    points = np.concatenate([world(m, [x, y, z], yaw).reshape(-1, 3) for m in frames])
    print('Lake guitar emitted bounds', points.min(axis=0), points.max(axis=0), flush=True)
    assert (points[:, 0] > back).all() and (points[:, 0] < front).all()
    # Convex vault section: containing every emitted vertex contains every triangle.
    opening = np.array(g['opening'])
    across = points[:, 2] - center
    assert (across >= opening[:, 0].min() + 1).all(), 'guitar penetrates north vault wall'
    assert (across <= opening[:, 0].max() - 1).all(), 'guitar penetrates south vault wall'
    top = np.interp(across, opening[1:-1, 0], opening[1:-1, 1])
    assert (points[:, 1] < y + top - 1).all(), 'guitar/body penetrates vault roof'
    # Original native toes dip .515 units below their nominal floor anchor.
    assert (points[:, 1] >= y - 1).all(), 'guitar/body penetrates dry floor'
    assert (points[:, 2] <= center - 18).all(), 'guitar/body blocks adult center approach/return lane'
    print(f'PASS Lake: all {len(frames)} guitar frames/{len(frames[0])} triangles, vault/floor and adult lane')
    if g['valley']:
        check_ghost(resources, g)


def check_ghost(resources, g):
    terrain = np.array(g['valley'])
    def floor(x, z):
        found = []
        for t in terrain:
            m = np.array([[t[0, 0]-t[2, 0], t[1, 0]-t[2, 0]],
                          [t[0, 2]-t[2, 2], t[1, 2]-t[2, 2]]])
            if abs(np.linalg.det(m)) < .001:
                continue
            a, b = np.linalg.solve(m, [x-t[2, 0], z-t[2, 2]])
            if min(a, b, 1-a-b) >= -1e-5:
                found.append(a*t[0, 1]+b*t[1, 1]+(1-a-b)*t[2, 1])
        return max(found) if found else None
    across, front, lift = g['ghost']
    ex, ey, ez, yaw = g['endpoint']
    angle = yaw * math.pi / 32768
    x = ex + math.sin(angle)*front + math.cos(angle)*across
    z = ez + math.cos(angle)*front - math.sin(angle)*across
    origin = [x, floor(x, z)+lift, z]
    frames = posed_mesh(resources, 'objects/object_gg/object_gg_Skel_00F6C0',
                        'objects/object_gg/object_gg_Anim_00F578')
    points = np.concatenate([world(m, origin, yaw).reshape(-1, 3) for m in frames])
    # Conservative whole-animation axis-aligned body prism; clip every actual
    # terrain triangle to its X/Z rectangle, then bound its highest surface.
    lo, hi = points.min(axis=0), points.max(axis=0)
    def clip(poly, axis, edge, sign):
        output = []
        for a, b in zip(poly, poly[1:] + poly[:1]):
            da, db = sign*(a[axis]-edge), sign*(b[axis]-edge)
            if da >= 0:
                output.append(a)
            if (da >= 0) != (db >= 0):
                output.append(a + (b-a)*da/(da-db))
        return output
    heights = []
    for tri in np.concatenate([terrain, np.array(g['bridge'])]):
        poly = list(tri)
        for axis, edge, sign in [(0, lo[0], 1), (0, hi[0], -1), (2, lo[2], 1), (2, hi[2], -1)]:
            poly = clip(poly, axis, edge, sign) if poly else []
        heights.extend(p[1] for p in poly)
    clearance = lo[1]-max(heights)
    print('Snowhead wait envelope', lo, hi, 'terrain clearance', clearance, flush=True)
    assert clearance > 1, 'Darmani body envelope intersects final terrain'
    assert lo[2] > 18, 'Darmani blocks center approach/return lane'
    print(f'PASS Snowhead: all {len(frames)} wait frames, conservative body prism above actual terrain')

if __name__ == '__main__':
    main()
