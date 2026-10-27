#!/usr/bin/env python3
"""Prove the built bridge retains accepted wood and only cuts the native corner."""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import math
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

import numpy as np

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / 'tools/woods_mystery'))
from build_candidate import ROOT, SCENE, WALL, Resources
from scene_assets import vertices

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--native', type=Path, default=REPO.parent / 'inputs/Woods-of-Mystery-Native-Resources.zip')
parser.add_argument('--accepted', type=Path, default=REPO.parent / 'poc5/zzzzz_Woods_of_Mystery_OoT_Makeover_POC5.prelude.o2r')
args = parser.parse_args()
ASSET_ROOT = REPO / 'mm/assets/custom'
PREFIX = 'custom/cor_woods_bridge/'
directory = ASSET_ROOT / PREFIX
assert directory.is_dir(), 'The accepted bridge must be built into 2ship assets without a makeover archive'
files = {p.relative_to(ASSET_ROOT).as_posix(): p.read_bytes() for p in directory.rglob('*') if p.is_file()}
expected = {'bridge', 'bridge_vtx', 'wall', 'wall_vtx', 'canopy_vtx', 'textures/bark', 'textures/planks', 'textures/black'}
assert set(files) == {PREFIX + name for name in expected}, sorted(files)
native = Resources(args.native)
accepted = Resources(args.native, args.accepted)
built = Resources(args.native)
built.files.update(files)
built.reindex()
native_canopy_vertices=SCENE+'Z2_26SARUNOMORI_room_01Vtx_002708'
original_vertices=native.v(native_canopy_vertices)
corrected_vertices=vertices(files[PREFIX+'canopy_vtx'])
assert original_vertices.shape==corrected_vertices.shape==(38,9)
expected_vertices=original_vertices.copy()
assert np.array_equal(expected_vertices[[0,27],:3],[[213,88,-214],[213,88,-214]])
expected_vertices[[0,27],1]=119
assert np.array_equal(corrected_vertices,expected_vertices), 'Canopy changed beyond the two authorized pivot Y fields'
old_vertex_bytes=native.get(native_canopy_vertices)
new_vertex_bytes=files[PREFIX+'canopy_vtx']
assert len(old_vertex_bytes)==len(new_vertex_bytes)==680
assert {i for i,(a,b) in enumerate(zip(old_vertex_bytes,new_vertex_bytes)) if a!=b}<={74,75,506,507}, 'Native vertex header, bounds or other attributes changed'
assert built.get(native_canopy_vertices)==old_vertex_bytes, 'The original native vertex path must remain untouched'
assert hashlib.sha256(args.accepted.read_bytes()).hexdigest() == 'cc1963d9dae25cd684cb0fc79948a398a611f1f49fad82ccd45f455f6f4c9257'
origin = np.array([110, -110])
axis = np.array([1, -1]) / math.sqrt(2)
side = np.array([1, 1]) / math.sqrt(2)
source_path = ROOT + 'room_1/geometry'
approved = []
for tri in accepted.mesh(source_path):
    v = tri['v']
    along = (v[:, [0, 2]] - origin) @ axis
    across = (v[:, [0, 2]] - origin) @ side
    if tri['path'] == source_path and along.min() >= -1 and along.max() <= 152 and abs(across).max() <= 70 and v[:, 1].max() <= 130:
        approved.append(tri)
assert len(approved) == 194
texture_map = {
    'custom/cor_mm3d/lost_woods_poc1/textures/lw_tutu_01_2A000': PREFIX + 'textures/planks',
    'custom/cor_mm3d/lost_woods_poc1/textures/lw_wood_02_2CA00': PREFIX + 'textures/bark',
    ROOT + 'textures/black': PREFIX + 'textures/black',
}
def signature(tri, remap=None):
    v=tri['v'].copy()
    if tri['texture'] in texture_map or tri['texture'] in texture_map.values():
        low=np.isin(v[:,1],[-3,-2]);v[low,1]+=4
    # The native mesh preview reader associates tile0's64x64 intensity extent
    # with the last image,A1C0. Its actual RGBA16 sampler1 is32x32.
    tile = (32, 32) if tri['texture'] == WALL else tuple(tri['tile'])
    return ((remap or {}).get(tri['texture'], tri['texture']), tile, bool(tri['lit']), tuple(v.reshape(-1)))
bridge = built.mesh(PREFIX + 'bridge')
assert collections.Counter(signature(t) for t in bridge) == collections.Counter(signature(t, texture_map) for t in approved), 'Wood changed beyond the authorized low-floor displacement'
assert collections.Counter(t['texture'].split('/')[-1] for t in bridge) == {'planks': 142, 'bark': 24, 'black': 28}
for old, new in texture_map.items():
    assert files[new] == accepted.get(old), 'Accepted texture bytes changed: ' + old
assert len(files[PREFIX + 'textures/bark']) == 131164
assert len(files[PREFIX + 'textures/planks']) == 65628
wall_path = SCENE + 'Z2_26SARUNOMORI_room_01DL_000B90'
original = native.mesh(wall_path)
replacement = built.mesh(PREFIX + 'wall')
wall = [t for t in replacement if t['path'] == PREFIX + 'wall']
assert all(t['texture'] == WALL and t['lit'] for t in wall), 'Native wall texture and lighting must survive'
assert all(t['tile'] == (32, 32) for t in wall), 'A1C0 must retain its native32x32 sampler extent'
present = collections.Counter(signature(t) for t in wall)
protected = [t for t in original if not ((t['v'][:, 0] >= 80) & (t['v'][:, 2] <= -80)).all()]
assert len(protected) == 80
assert all(present[signature(t)] for t in protected), 'A native wall face outside the bridge corner changed'
for direction in (np.array([1, 0]), np.array([-1, 0]), np.array([0, 1]), np.array([0, -1])):
    corridor = [t for t in original if ((t['v'][:, [0, 2]] @ direction) >= 239).all()]
    assert corridor and all(present[signature(t)] for t in corridor), 'An original cardinal corridor changed'
assert sum(t['path'] == PREFIX + 'bridge' for t in replacement) == 194, 'Native wall must own exactly one bridge'
assert all(np.linalg.norm(t['v'][:, :3] - [0, 87, 0], axis=1).max() < 456 for t in replacement), 'Generated bridge escaped original wall cull bounds'
xml = ET.fromstring(files[PREFIX + 'bridge'])
elements = list(xml)
black_index = next(i for i, e in enumerate(elements) if e.tag == 'SetTextureImage' and e.get('Path') == PREFIX + 'textures/black')
assert any(e.tag == 'ClearGeometryMode' and 'G_FOG' in e.attrib for e in elements[black_index - 4:black_index]), 'Dark cap must be explicitly unfogged'
assert any(e.tag == 'SetOtherMode' and 'G_RM_AA_ZB_OPA_SURF2' in e.attrib for e in elements[black_index - 4:black_index]), 'Dark cap must be opaque'
for name, data in files.items():
    if not data.startswith(b'<'):
        continue
    for element in ET.fromstring(data):
        path = element.get('Path')
        assert path is None or path.startswith(PREFIX) or path == WALL, (name, path)
manifest_path = REPO / 'tools/woods_mystery/native_bridge_manifest.json'
manifest = json.loads(manifest_path.read_text())
assert manifest['accepted_sha256'] == hashlib.sha256(args.accepted.read_bytes()).hexdigest()
assert manifest['native_sha256'] == hashlib.sha256(args.native.read_bytes()).hexdigest()
assert manifest['bridge_triangles'] == 194
assert manifest['canopy_clearance']['pivot_new_xyz']==[213,119,-214]
assert manifest['canopy_clearance']['vertex_indices']==[0,27]
assert manifest['canopy_clearance']['vertex_count']==38
assert manifest['asset_sha256'] == {name: hashlib.sha256(data).hexdigest() for name, data in sorted(files.items())}
with tempfile.TemporaryDirectory(prefix='woods-native-assets-') as td:
    output = Path(td) / 'assets'
    report = Path(td) / 'manifest.json'
    subprocess.run([sys.executable, '-B', str(REPO / 'tools/woods_mystery/build_native_bridge.py'), '--native', str(args.native), '--accepted', str(args.accepted), '--output', str(output), '--manifest', str(report)], check=True)
    regenerated = {p.relative_to(output).as_posix(): p.read_bytes() for p in output.rglob('*') if p.is_file()}
    assert regenerated == files, 'Native asset recipe is not byte reproducible'
    assert report.read_bytes() == manifest_path.read_bytes(), 'Native provenance is not reproducible'
print('PASS: native38-vertex canopy changes only indices0/27 Y88 to119; wood upper geometry/textures and all native corridors stay exact; corrected assets reproduce deterministically')
