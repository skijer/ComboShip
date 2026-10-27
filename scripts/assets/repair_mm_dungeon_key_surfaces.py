#!/usr/bin/env python3
"""Repair the confirmed POC3 key surface overlaps without replacing key bodies.

Uses the exact recovered archive as its source. Coordinates are native quarter
units; runtime draw scale, resource paths, materials and cosmetic bindings stay
unchanged. This is offline geometry verification, not in-game acceptance.
"""
from collections import Counter, defaultdict
from itertools import combinations
from pathlib import Path
import argparse
import hashlib
import json
import math
import xml.etree.ElementTree as ET
import zipfile

SOURCE_SHA256 = '2b4948ff7d784dbce5943f264ed8049b1e7e2a57b8744071fcf30b39c06b83c5'
# Small integer relief selected with cap-depth ordering constraints. Every
# occurrence of a shared point moves together, including adjacent bevels.
CAP_RELIEF = {
    'StoneTowerBossKey': {(25, 41): 1, (29, 43): 1, (30, 48): 4, (32, 46): 1, (39, 52): 1},
    'StoneTowerSmallKey': {(28, 45): 1, (32, 47): 1, (34, 52): 4, (36, 51): 1,
                         (38, 51): 1, (43, 57): 1},
}


def xml(root):
    ET.indent(root, space='  ')
    return ET.tostring(root) + b'\n'


def positions(data):
    return [tuple(int(v.get(k)) for k in ('X', 'Y', 'Z')) for v in ET.fromstring(data)]


def subtract(a, b):
    return tuple(x - y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0])


def plane(triangle):
    normal = cross(subtract(triangle[1], triangle[0]), subtract(triangle[2], triangle[0]))
    divisor = math.gcd(*normal)
    if not divisor:
        return None
    normal = tuple(v // divisor for v in normal)
    return normal + (sum(a*b for a, b in zip(normal, triangle[0])),)


def signed_area(polygon):
    return sum(a[0]*b[1] - a[1]*b[0] for a, b in zip(polygon, polygon[1:] + polygon[:1])) / 2


def intersection(a, b):
    """Convex clipping; only positive-area intersections count as overlaps."""
    if signed_area(b) < 0:
        b = b[::-1]
    polygon = list(a)
    for v, w in zip(b, b[1:] + b[:1]):
        def side(q):
            return (w[0]-v[0])*(q[1]-v[1]) - (w[1]-v[1])*(q[0]-v[0])
        output = []
        for x, y in zip(polygon, polygon[1:] + polygon[:1]):
            sx, sy = side(x), side(y)
            if sx >= -1e-9:
                output.append(x)
            if (sx >= 0) != (sy >= 0):
                t = sx / (sx - sy)
                output.append((x[0] + t*(y[0]-x[0]), x[1] + t*(y[1]-x[1])))
        polygon = output
        if not polygon:
            break
    return polygon


def analyze(data):
    p = positions(data)
    assert len(p) % 3 == 0
    triangles = [p[i:i+3] for i in range(0, len(p), 3)]
    edges, planes = defaultdict(list), defaultdict(list)
    parents = list(range(len(triangles)))

    def root(i):
        while parents[i] != i:
            i = parents[i]
        return i

    for i, triangle in enumerate(triangles):
        assert plane(triangle), 'degenerate native triangle'
        planes[plane(triangle)].append(i)
        for a, b in zip(triangle, triangle[1:] + triangle[:1]):
            edges[tuple(sorted((a, b)))].append(i)
    for ids in edges.values():
        for i in ids[1:]:
            parents[root(i)] = root(ids[0])
    components = defaultdict(list)
    for i in range(len(triangles)):
        components[root(i)].append(i)
    conflicts = []
    for key, ids in planes.items():
        axis = max(range(3), key=lambda a: abs(key[a]))
        axes = [a for a in range(3) if a != axis]
        for i, j in combinations(ids, 2):
            a = [tuple(q[k] for k in axes) for q in triangles[i]]
            b = [tuple(q[k] for k in axes) for q in triangles[j]]
            polygon = intersection(a, b)
            if polygon and abs(signed_area(polygon)) > 1e-6:
                conflicts.append((i, j, root(i), root(j)))
    topology = {'boundaryEdges': sum(len(v) == 1 for v in edges.values()),
                'nonmanifoldEdges': sum(len(v) > 2 for v in edges.values())}
    return triangles, components, conflicts, topology


def height_at(triangle, p):
    a, b, c = triangle
    denominator = (b[1]-c[1])*(a[0]-c[0]) + (c[0]-b[0])*(a[1]-c[1])
    wa = ((b[1]-c[1])*(p[0]-c[0]) + (c[0]-b[0])*(p[1]-c[1])) / denominator
    wb = ((c[1]-a[1])*(p[0]-c[0]) + (a[0]-c[0])*(p[1]-c[1])) / denominator
    return wa*a[2] + wb*b[2] + (1-wa-wb)*c[2]


def verify_cap_clearance(old, new, conflicts):
    for a, b, *_ in conflicts:
        if plane(old[a]) != (0, 0, 1, 5):
            continue
        polygon = intersection([q[:2] for q in old[a]], [q[:2] for q in old[b]])
        differences = [height_at(new[a], p) - height_at(new[b], p) for p in polygon]
        # A linear depth difference reaches its extrema at intersection corners.
        # Adjacent edges may meet; overlapping interiors must have depth order.
        assert min(differences) >= -1e-7 or max(differences) <= 1e-7, 'cap faces still cross'
        center = tuple(sum(p[k] for p in polygon)/len(polygon) for k in range(2))
        assert abs(height_at(new[a], center) - height_at(new[b], center)) >= .1-1e-7


def rebuild_batches(blobs, vertex_path, count):
    path = vertex_path[:-4]
    root = ET.fromstring(blobs[path])
    first = next(i for i, e in enumerate(root) if e.tag == 'LoadVertices')
    end = next(i for i, e in enumerate(root) if e.tag == 'EndDisplayList')
    del root[first:end]
    at = first
    for offset in range(0, count, 30):
        size = min(30, count-offset)
        root.insert(at, ET.Element('LoadVertices', {'Path': vertex_path, 'VertexBufferIndex': '0',
                                                   'VertexOffset': str(offset), 'Count': str(size)}))
        at += 1
        for j in range(0, size, 3):
            root.insert(at, ET.Element('Triangle1', {'V00': str(j), 'V01': str(j+1), 'V02': str(j+2)}))
            at += 1
    blobs[path] = xml(root)


def validate_archive(blobs):
    vertices = triangles = 0
    for path, data in blobs.items():
        if not data.startswith(b'<'):
            continue
        root = ET.fromstring(data)
        for node in root:
            if 'Path' in node.attrib:
                assert node.get('Path') in blobs, 'missing resource dependency'
        if root.tag == 'Vertex':
            vertices += len(root)
            for v in root:
                assert all(-32768 <= int(v.get(k)) <= 32767 for k in ('X', 'Y', 'Z'))
            assert not analyze(data)[2], path + ' has overlapping coplanar triangles'
        elif root.tag == 'DisplayList':
            loaded = 0
            for node in root:
                if node.tag == 'LoadVertices':
                    loaded = int(node.get('Count'))
                    size = len(ET.fromstring(blobs[node.get('Path')]))
                    assert 0 < loaded <= 30 and int(node.get('VertexOffset')) + loaded <= size
                elif node.tag == 'Triangle1':
                    assert all(0 <= int(node.get(k)) < loaded for k in ('V00', 'V01', 'V02'))
                    triangles += 1
    assert vertices == triangles*3
    return {'resources': len(blobs), 'vertices': vertices, 'triangles': triangles}


def repair(source, target):
    assert hashlib.sha256(source.read_bytes()).hexdigest() == SOURCE_SHA256, 'unexpected key baseline'
    with zipfile.ZipFile(source) as archive:
        original = {n: archive.read(n) for n in archive.namelist()}
    blobs, report = dict(original), {}
    for name, data in original.items():
        if not name.endswith('_vtx'):
            continue
        old, components, conflicts, old_topology = analyze(data)
        if not conflicts:
            continue
        nodes = ET.fromstring(data)
        info = {'coplanarOverlapsBefore': len(conflicts)}
        if 'GreatBayBossKeyEmblemDL_eye' in name:
            seen, kept = set(), []
            for i, triangle in enumerate(old):
                key = tuple(sorted(triangle)) + (plane(triangle),)
                if key not in seen:
                    seen.add(key)
                    kept.extend(list(nodes)[i*3:i*3+3])
            nodes[:] = kept
            info['duplicateTrianglesRemoved'] = len(old) - len(kept)//3
            rebuild_batches(blobs, name, len(nodes))
        elif 'StoneTower' in name:
            key = next(key for key in CAP_RELIEF if key in name)
            relief = CAP_RELIEF[key]
            for vertex in nodes:
                x, y, z = (int(vertex.get(k)) for k in ('X', 'Y', 'Z'))
                if abs(z) == 5 and (x, y) in relief:
                    vertex.set('Z', str(z + (1 if z > 0 else -1)*relief[x, y]))
            info['capReliefNativeUnits'] = {f'{x},{y}': d for (x, y), d in relief.items()}
        else:
            shifted = set()
            for _, _, ca, cb in conflicts:
                assert ca != cb, 'overlap within a single closed part'
                shifted.add(max((ca, cb), key=lambda c: min(components[c])))
            for component in shifted:
                for i in components[component]:
                    for vertex in list(nodes)[i*3:i*3+3]:
                        vertex.set('Z', str(int(vertex.get('Z'))+2))
            info['closedPartDepthShiftNativeUnits'] = 2
            info['shiftedTriangles'] = sum(len(components[c]) for c in shifted)
        blobs[name] = xml(nodes)
        new, _, remaining, new_topology = analyze(blobs[name])
        assert not remaining, 'surface overlap remains'
        assert new_topology == old_topology, 'surface repair changed open/nonmanifold edge counts'
        old_points, new_points = positions(data), positions(blobs[name])
        assert all(min(p[k] for p in old_points) == min(p[k] for p in new_points) and
                   max(p[k] for p in old_points) == max(p[k] for p in new_points) for k in range(3)), 'bounds changed'
        if 'StoneTower' in name:
            verify_cap_clearance(old, new, conflicts)
        info.update(coplanarOverlapsAfter=0, boundsPreserved=True, topology=new_topology)
        report[name] = info
    # All eight original donor bodies/teeth and normals must be identical.
    for name in original:
        if 'MetalDL_metal_vtx' in name:
            count = (126 if 'BossKey' in name else 78)*3
            assert [v.attrib for v in ET.fromstring(original[name])][:count] == \
                   [v.attrib for v in ET.fromstring(blobs[name])][:count], 'donor body changed'
    assert set(blobs) == set(original)
    # Only one eye display list needs new triangle batches. Every material,
    # root selector, texture and other dependency remains exactly as recovered.
    for name, data in original.items():
        if not name.endswith('_vtx') and not name.endswith('GreatBayBossKeyEmblemDL_eye'):
            assert blobs[name] == data, 'material or resource binding changed'
    counts = validate_archive(blobs)
    target.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(target, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(blobs.items()):
            info = zipfile.ZipInfo(name, (2026, 10, 7, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
    return {'sourceSHA256': SOURCE_SHA256, 'candidateSHA256': hashlib.sha256(target.read_bytes()).hexdigest(),
            'status': 'offline verified; runtime acceptance pending', 'counts': counts, 'repairs': report,
            'originalBodiesAndNormalsPreserved': True, 'cosmeticBindingsPreserved': True}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('target', type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    result = repair(args.source, args.target)
    text = json.dumps(result, indent=2) + '\n'
    if args.report:
        args.report.write_text(text)
    print(text, end='')
