"""Check the scene boundary: visuals may change; collision and routes may not."""
from pathlib import Path
import argparse, hashlib, json, re, struct, sys, zipfile
import xml.etree.ElementTree as ET
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/woods_mystery'))
from scene_assets import Resources

p=argparse.ArgumentParser();p.add_argument('candidate',type=Path);p.add_argument('native',type=Path)
a=p.parse_args()
with zipfile.ZipFile(a.candidate) as z:
    assert z.testzip() is None
    files={n:z.read(n) for n in z.namelist()}
report_key=next((n for n in ['woods-mystery-poc6.json','woods-mystery-poc5.json','woods-mystery-poc4.json','woods-mystery-poc3.json','woods-mystery-poc2.json','woods-mystery-poc1.json'] if n in files),None)
assert report_key, 'Woods of Mystery visual candidate is not installed'
report=json.loads(files[report_key])
assert report['donor_faces']==246 and report['donor_vertices']==321
assert report['collision']=='Vanilla; not overridden'
assert not any('collision' in n.lower() or 'actorentry' in n.lower() or 'pathway' in n.lower() for n in files)
assert not any('/Z2_LOST_WOODS/' in n or '/skiejer' in n for n in files), 'Other forest/shop resources changed'
with zipfile.ZipFile(a.native) as z:
    native={n:z.read(n) for n in z.namelist()}
native_res=Resources(a.native)
candidate_res=Resources(a.native,a.candidate)
def signed(normals):return np.where(normals>=128,normals-256,normals)
def on_native_corner_face(vertex,triangles):
    for tri in triangles:
        v=tri['v'];basis=(v[1:,:3]-v[0,:3]).T
        weights=np.linalg.lstsq(basis,vertex[:3]-v[0,:3],rcond=None)[0]
        w=np.array([1-weights.sum(),*weights])
        if w.min()<-.02 or w.max()>1.02 or np.linalg.norm(w@v[:,:3]-vertex[:3])>.9:continue
        if (np.max(abs(w@v[:,3:5]-vertex[3:5]))<=7 and
            np.max(abs(w@signed(v[:,5:8])-signed(vertex[5:8])))<=5 and
            abs(w@v[:,8]-vertex[8])<=4):return True
        # Rounded XYZ need not project back to the original UV at the nearest
        # point on a sloped face. Recover from UV and require the corresponding
        # XYZ to remain within its integer-quantization cell instead.
        uv_basis=(v[1:,3:5]-v[0,3:5]).T
        if abs(np.linalg.det(uv_basis))<1e-6:continue
        weights=np.linalg.solve(uv_basis,vertex[3:5]-v[0,3:5]);w=np.r_[1-weights.sum(),weights]
        if (w.min()>=-.02 and w.max()<=1.02 and np.max(abs(w@v[:,:3]-vertex[:3]))<=.65 and
            np.max(abs(w@signed(v[:,5:8])-signed(vertex[5:8])))<=5 and
            abs(w@v[:,8]-vertex[8])<=4):return True
    return False
for name,data in files.items():
    key=name.removeprefix('alt/')
    if key not in native or not data.startswith(b'<DisplayList'):continue
    before=native_res.mesh(key);after=candidate_res.mesh(key)
    if not any(t['texture']=='scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00A1C0' for t in before):continue
    assert before and all(t['lit'] for t in before), 'Unexpected native wall material'
    assert after and all(t['lit'] for t in after), 'Native wall normals are being interpreted as RGB'
    original_vertices={tuple(v.astype(int)) for tri in before for v in tri['v']}
    for tri in after:
        for v in tri['v']:
            if tuple(v.astype(int)) in original_vertices:continue
            match=re.fullmatch(r'scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORI_room_(0[0-8])DL_[0-9A-F]+',key)
            assert match, 'Unrelated native wall resource changed'
            room=int(match[1]);local=v[:3]-[-880*(room//3),0,880*(1-room%3)]
            assert max(abs(local[[0,2]]))>=149 or (room==1 and local[0]>=79 and local[2]<=-79), 'Wall edit escaped the tunnel surrounds'
            assert on_native_corner_face(v,before), 'New tunnel-surround vertex lost native UV or signed-normal interpolation'
for room in range(9):
    key=f'scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORI_room_{room:02d}'
    original=native[key];candidate=files['alt/'+key]
    info=report['rooms'][room]
    before=info['mesh_offset'];end=info['mesh_end'];extra=info['appended_entry_bytes']
    restored=bytearray(candidate[:end]+candidate[end+extra:])
    restored[before+6]-=1
    assert bytes(restored)==original, f'Room {room}: non-visual header fields changed'
    assert struct.unpack_from('<I',candidate,64)==struct.unpack_from('<I',original,64)
    assert info['tunnel_count']==4
    assert info['center']==[-880*(room//3),0,880*(1-room%3)], 'Room grid placement changed'
    assert f'alt/custom/cor_woods_mystery/poc1/room_{room}/geometry' in files
assert report['rooms'][1]['bridge_tunnel']
assert len(report['rooms'])==9
for room in range(9):
    display_list=ET.fromstring(files[f'alt/custom/cor_woods_mystery/poc1/room_{room}/geometry'])
    assert all('G_LIGHTING' not in command.attrib for command in display_list if command.tag=='SetGeometryMode'), 'Donor bark must not enable lighting: the native XML factory uses attribute presence'
def vertices(room):
    root=ET.fromstring(files[f'alt/custom/cor_woods_mystery/poc1/room_{room}/geometry_vtx'])
    return np.array([[int(v.get(k)) for k in ['X','Y','Z']] for v in root])
for room in [0,2,3,4,5,6,7,8]:
    local=vertices(room)-np.array(report['rooms'][room]['center'])
    assert np.max(abs(local[:,[0,2]]))<750
    assert np.min(local[:,1])>=-3, 'Geometry extends below the authorized submerged wood floor'
    assert np.array_equal(local,vertices(0)-np.array(report['rooms'][0]['center'])), 'Maze rooms use inconsistent local placements'
print('PASS: nine room meshes, complete donor, preserved non-mesh header bytes, no collision/actor/path/other-scene overrides')
