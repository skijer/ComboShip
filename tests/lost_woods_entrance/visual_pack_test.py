#!/usr/bin/env python3
"""Check shipped vertex UVs against the original artist model, not the repair tool."""
import argparse
from pathlib import Path
import struct
import xml.etree.ElementTree as ET
import zipfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--baseline', type=Path, required=True)
parser.add_argument('--candidate', type=Path, required=True)
parser.add_argument('--dae', type=Path, required=True)
args = parser.parse_args()
NS = {'c': 'http://www.collada.org/2005/11/COLLADASchema'}
model = ET.parse(args.dae).getroot()
geometries = {g.get('id'): g for g in model.findall('.//c:library_geometries/c:geometry', NS)}
prefix = 'alt/custom/cor_mm3d/lost_woods_poc1/'
errors = []
with zipfile.ZipFile(args.baseline) as before, zipfile.ZipFile(args.candidate) as after:
    assert after.testzip() is None
    assert len(after.namelist()) == len(set(after.namelist()))
    tested = count = 0
    for name in before.namelist():
        if '/Geometry' not in name or not name.endswith('_vtx'):
            continue
        geom = geometries[Path(name).name[:-4]]
        sources = {}
        for source in geom.findall('c:mesh/c:source', NS):
            access = source.find('c:technique_common/c:accessor', NS)
            stride = int(access.get('stride'))
            values = list(map(float, source.find('c:float_array', NS).text.split()))
            sources[source.get('id')] = [values[i:i+stride] for i in range(0, len(values), stride)]
        bindings = {x.get('semantic'): sources[x.get('source')[1:]]
                    for x in geom.findall('c:mesh/c:vertices/c:input', NS)}
        dl = ET.fromstring(before.read(name[:-4]))
        tile = dl.find('SetTileSize')
        period_s = (int(tile.get('Lrs')) // 4 + 1) * 32
        period_t = (int(tile.get('Lrt')) // 4 + 1) * 32
        # The source images use image-row UVs. Negative V wraps from the bottom
        # of the image; the center of its quarter-ring atlas is the top left.
        authored = {(*[round(x * 2.54) for x in xyz], round(uv[0] * period_s) % period_s,
                     round((1 + uv[1]) * period_t) % period_t)
                    for xyz, uv in zip(bindings['POSITION'], bindings['TEXCOORD'])}
        old = ET.fromstring(before.read(name))
        new = ET.fromstring(after.read(name))
        assert len(old) == len(new)
        bad = 0
        for a, b in zip(old, new):
            assert {k:v for k,v in a.attrib.items() if k != 'T'} == {
                k:v for k,v in b.attrib.items() if k != 'T'}, name
            key = (*[int(b.get(c)) for c in ('X','Y','Z')],
                   int(b.get('S')) % period_s, int(b.get('T')) % period_t)
            bad += key not in authored
        if bad:
            errors.append(f'{Path(name).name}: {bad}/{len(new)} UVs do not match the artist atlas')
        tested += 1
        count += len(new)
    assert tested == 22
    root = prefix + 'room_0/'
    mouth = ET.fromstring(after.read(root + 'ReturnTunnelMouth_vtx'))
    xyz = [[int(v.get(c)) for c in ('X','Y','Z')] for v in mouth]
    if not all(-415 <= p[0] <= -414 and 52 <= p[2] <= 188 for p in xyz):
        errors.append('doorway is not at the east face of the cut corner stump')
    stump_roots = []
    for geometry in ('Geometry6','Geometry7','Geometry8'):
        vertices = ET.fromstring(after.read(root + geometry + '_vtx'))
        stump_roots += [[int(v.get(c)) for c in ('X','Y','Z')] for v in vertices
                        if -520 <= int(v.get('X')) < -400 and 52 <= int(v.get('Z')) <= 188
                        and 0 <= int(v.get('Y')) <= 134]
    if max(p[0] for p in stump_roots) >= min(p[0] for p in xyz):
        errors.append('native stump roots protrude through the visible doorway')
    active = 'custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_corner_v3'
    for name in before.namelist():
        if name.startswith('alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00'):
            token = struct.pack('<I', len(active)) + active.encode()
            if after.read(name).count(token) != 1:
                errors.append(name + ': active mesh is not the corner doorway')
    if errors:
        raise AssertionError('\n'.join(errors))
    print(f'PASS artist atlas: {tested} scenery resources, {count} vertices; XYZ/S/color preserved; corner mouth and all four setup bindings')
