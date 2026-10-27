#!/usr/bin/env python3
"""Execute the real WoM shimmer draw and compare its centers with the real black cap."""
from pathlib import Path
import argparse
import math
import os
import struct
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from


def vertices(data):
    if data.startswith(b'<'):
        return [tuple(int(v.get(k)) for k in ('X', 'Y', 'Z')) for v in ET.fromstring(data)]
    kind, count = struct.unpack_from('<II', data, 64)
    assert kind == 25
    version = struct.unpack_from('<I', data, 8)[0]
    return [struct.unpack_from('<iii' if version else '<hhh', data, 72 + i * (24 if version else 16))
            for i in range(count)]


def black_triangles(get, display, texture):
    cache, result, black = {}, [], False
    for element in ET.fromstring(get(display)):
        a = element.attrib
        if element.tag == 'SetTextureImage':
            black = a['Path'] == texture
        elif element.tag == 'LoadVertices':
            points = vertices(get(a['Path']))
            start, dest = int(a.get('VertexOffset', 0)), int(a.get('VertexBufferIndex', 0))
            for i in range(int(a['Count'])):
                cache[dest + i] = points[start + i]
        elif black and element.tag in ('Triangle1', 'Triangle2'):
            result.append(tuple(cache[int(a[f'V0{i}'])] for i in range(3)))
            if element.tag == 'Triangle2':
                result.append(tuple(cache[int(a[f'V1{i}'])] for i in range(3)))
    return result


def projected(point):
    return ((point[0] + point[2]) / math.sqrt(2), point[1])


def contains(point, triangle):
    p, a, b, c = projected(point), *(projected(v) for v in triangle)
    cross = lambda u, v, w: (v[0] - u[0]) * (w[1] - u[1]) - (v[1] - u[1]) * (w[0] - u[0])
    if abs(cross(a, b, c)) < 1e-9:
        return False
    edges = [cross(a, b, p), cross(b, c, p), cross(c, a, p)]
    return min(edges) >= -1e-6 or max(edges) <= 1e-6


def distance(point, triangle):
    # The cap's quantized triangles are almost coplanar. Project onto each
    # actual triangle plane, then include edge distance outside its footprint.
    sub = lambda a, b: tuple(x - y for x, y in zip(a, b))
    dot = lambda a, b: sum(x * y for x, y in zip(a, b))
    a, b, c = triangle
    u, v = sub(b, a), sub(c, a)
    normal = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
    norm = dot(normal, normal)
    if norm:
        ratio = dot(sub(point, a), normal) / norm
        on_plane = tuple(x - ratio * n for x, n in zip(point, normal))
        if contains(on_plane, triangle):
            return abs(ratio) * math.sqrt(norm)
    distances = []
    for a, b in ((a, b), (b, c), (c, a)):
        edge = sub(b, a)
        denom = dot(edge, edge)
        t = min(1, max(0, dot(sub(point, a), edge) / denom)) if denom else 0
        nearest = tuple(x + t * d for x, d in zip(a, edge))
        distances.append(math.sqrt(dot(sub(point, nearest), sub(point, nearest))))
    return min(distances)


def clearing_depth(point, triangle):
    a, b, c = triangle
    u, v = [tuple(q - p for p, q in zip(a, end)) for end in (b, c)]
    normal = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
    # This cap is in the +X/-Z corner. The room's clearing lies in the
    # -X/+Z direction; orient each actual triangle normal toward that approach.
    if -normal[0] + normal[2] < 0:
        normal = tuple(-n for n in normal)
    length = math.sqrt(sum(n * n for n in normal))
    assert length > 0  # contains() already excludes degenerate projections.
    return sum((p - q) * n for p, q, n in zip(point, a, normal)) / length


def placement_failures(points, triangles):
    distances = [min(distance(p, t) for t in triangles) for p in points]
    outside = [i for i, p in enumerate(points) if not any(contains(p, t) for t in triangles)]
    failures = []
    if max(distances) > 12:
        failures.append('shimmer is suspended away from the actual black portal surface')
    if outside:
        failures.append('emitter centers leak beyond the black arch')
    signed_depths = []
    for point in points:
        supporting_faces = [t for t in triangles if contains(point, t)]
        if supporting_faces:
            signed_depths.append(min(clearing_depth(point, t) for t in supporting_faces))
    if signed_depths and min(signed_depths) <= 0:
        failures.append('emitter centers are behind the clearing-facing black cap')
    return failures, {'distances': distances, 'outside': outside, 'signed_depths': signed_depths}


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--accepted', type=Path, help='Also verify the immutable accepted POC5 portal cap')
parser.add_argument('--shipped', type=Path, help='Verify the matching 2ship.o2r uses these exact bridge resources')
args = parser.parse_args()
assets = ROOT / 'mm/assets/custom'
cap = black_triangles(lambda name: (assets / name).read_bytes(), 'custom/cor_woods_bridge/bridge',
                      'custom/cor_woods_bridge/textures/black')
assert len(cap) == 28
surfaces = [('native bridge', cap)]
if args.shipped:
    with ZipFile(args.shipped) as archive:
        for name in ('custom/cor_woods_bridge/bridge', 'custom/cor_woods_bridge/bridge_vtx'):
            assert archive.read(name) == (assets / name).read_bytes(), name + ' differs from the supplied build'
    print('PASS matching shipped bridge display list and vertices are exact')
if args.accepted:
    with ZipFile(args.accepted) as archive:
        names = set(archive.namelist())
        get = lambda name: archive.read('alt/' + name if 'alt/' + name in names else name)
        accepted = black_triangles(get, 'custom/cor_woods_mystery/poc1/room_1/geometry',
                                   'custom/cor_woods_mystery/poc1/textures/black')
    # The accepted room also has a separate side-building portal. Select only
    # the same diagonal bridge cluster that the native baking recipe retains.
    accepted = [t for t in accepted if all(-1 <= (p[0] - p[2] - 220) / math.sqrt(2) <= 152 and
                                           abs((p[0] + p[2]) / math.sqrt(2)) <= 70 for p in t)]
    assert len(accepted) == 28
    surfaces.append(('accepted POC5 bridge', accepted))

source = (ROOT / 'mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp').read_text()
draw = block_from(source, source.index('static void DrawWoodsMysteryHexes('))
fixture = (ROOT / 'tests/woods_mystery/shimmer_placement_test.cpp').read_text().replace('/* PRODUCTION */', draw)
with tempfile.TemporaryDirectory(prefix='woods-shimmer-placement-') as temporary:
    cpp, binary = Path(temporary) / 'test.cpp', Path(temporary) / 'test'
    cpp.write_text(fixture)
    includes = ['.', 'mm', 'mm/2s2h', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets',
                'libultraship/include', 'combo']
    command = [os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DCOMBO_BUILD', '-DF3DEX_GBI_2',
               '-DFMT_HEADER_ONLY', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
               *['-I' + str(ROOT / p) for p in includes], str(cpp), '-o', str(binary)]
    subprocess.run(command, check=True)
    output = subprocess.check_output([str(binary)], text=True,
                                     env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
    behind_output = subprocess.check_output([str(binary), '--behind-cap'], text=True,
                                            env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
points = [tuple(map(float, line.split()[2:])) for line in output.splitlines()]
behind_points = [tuple(map(float, line.split()[2:])) for line in behind_output.splitlines()]
assert len(points) == 60  # 20 motes, native/Alt plus the accepted pack-owned path.
assert len(behind_points) == 60
failures = []
for label, triangles in surfaces:
    violations, result = placement_failures(points, triangles)
    distances, outside = result['distances'], result['outside']
    depths = result['signed_depths']
    depth_range = f'{min(depths):.3f}..{max(depths):.3f}' if depths else 'no projected face support'
    print(f'{label}: 60 centers, cap distance {min(distances):.3f}..{max(distances):.3f}, '
          f'{len(outside)} outside projected black face, '
          f'clearing-facing depth {depth_range}', flush=True)
    failures.extend(label + ': ' + reason for reason in violations)
    rejected, control = placement_failures(behind_points, triangles)
    assert max(control['distances']) <= 12 and not control['outside'], 'negative control must isolate the cap-side gap'
    assert rejected == ['emitter centers are behind the clearing-facing black cap'], \
        label + ': behind-cap placement must fail despite passing unsigned distance and footprint checks'
    assert len(control['signed_depths']) == 60 and max(control['signed_depths']) < 0
    print(f'PASS {label} behind-cap negative control: signed depth '
          f'{min(control["signed_depths"]):.3f}..{max(control["signed_depths"]):.3f}', flush=True)
assert not failures, '; '.join(failures)
print('PASS actual WoM draw: native, Alt and pack-owned shimmer centers stay against their real portal cap')
