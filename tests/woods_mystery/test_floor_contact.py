"""Grass meets native collision; the wood threshold stays beneath it.

The byte/vertex oracle permits only this user-authorized floor correction.
Collision is decoded independently using CollisionHeaderFactoryV0's layout.
"""
from pathlib import Path
import argparse,hashlib,struct,sys,unittest,zipfile,xml.etree.ElementTree as ET
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/woods_mystery'))
from build_candidate import Resources,ROOT,SCENE,read_mesh_header

p=argparse.ArgumentParser();p.add_argument('candidate');p.add_argument('native');p.add_argument('previous')
p.add_argument('--previous-native-assets',type=Path)
a,rest=p.parse_known_args();sys.argv=[sys.argv[0],*rest]
r=Resources(a.native,a.candidate);native=Resources(a.native)

def room_mesh(room):
    _,_,entries=read_mesh_header(r.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
    return [t for path in dict.fromkeys(p for _,o,x in entries for p in(o,x) if p) for t in r.mesh(path)]

def surface_heights(triangles,points):
    result=np.full(len(points),-np.inf)
    for tri in triangles:
        xz=tri[:,[0,2]];e1=xz[1]-xz[0];e2=xz[2]-xz[0];d=points-xz[0]
        det=e1[0]*e2[1]-e1[1]*e2[0]
        if abs(det)<1e-8:continue
        u=(d[:,0]*e2[1]-d[:,1]*e2[0])/det;v=(e1[0]*d[:,1]-e1[1]*d[:,0])/det
        ix=np.where((u>=-1e-7)&(v>=-1e-7)&(u+v<=1+1e-7))[0]
        y=tri[0,1]+u[ix]*(tri[1,1]-tri[0,1])+v[ix]*(tri[2,1]-tri[0,1])
        result[ix]=np.maximum(result[ix],y)
    return result

def expected_vertex_bytes(data,backing=False):
    xml=ET.fromstring(data)
    for v in xml:
        y=int(v.get('Y'))
        if y in (1,2):v.set('Y',str(y-4))
        elif backing and y==10:v.set('Y','0')
    return ET.tostring(xml,encoding='utf-8')

class FloorContact(unittest.TestCase):
    def test_grass_follows_native_walk_height_on_every_route(self):
        b=native.get(SCENE+'Z2_26SARUNOMORICollisionHeader_009190')
        count=struct.unpack_from('<I',b,76)[0]
        vertices=np.frombuffer(b,dtype='<i2',count=count*3,offset=80).reshape(-1,3).astype(float)
        offset=80+6*count;count=struct.unpack_from('<I',b,offset)[0]
        polygons=np.frombuffer(b,dtype='<i2',count=count*8,offset=offset+4).reshape(-1,8)
        floors=vertices[polygons[polygons[:,5]>16000,1:4].astype('uint16')&8191]
        samples=0
        for room in range(9):
            center=np.array([-880*(room//3),880*(1-room%3)])
            points=np.array([center+depth*np.array(direction)+across*np.array([-direction[1],direction[0]])
                for direction in[(1,0),(-1,0),(0,1),(0,-1)]
                for depth in[180,240,271,300,400,439] for across in[-40,0,40]])
            collision=surface_heights(floors,points)
            grass=surface_heights([t['v'][:,:3] for t in room_mesh(room) if t['texture']==ROOT+'textures/grass'],points)
            self.assertTrue(np.isfinite(collision).all(),f'Missing vanilla walk surface in room{room}')
            self.assertTrue(np.isfinite(grass).all(),f'Missing grass in room{room}')
            self.assertLessEqual((grass-collision).max(),.01,
                f'Raised grass cuts into Link: room{room}, maximum excess height{(grass-collision).max()}')
            # Native prop collision can sit above the base near a shoulder.
            # It stays native; the corridor's flat walk surface must align.
            flat=abs(collision)<.01
            self.assertLessEqual(abs(grass[flat]-collision[flat]).max(),.01)
            samples+=len(points)
        self.assertEqual(samples,648)

    def test_wood_floor_is_below_the_visible_grass(self):
        count=0
        for room in range(9):
            for t in r.mesh(ROOT+f'room_{room}/geometry'):
                if not (t['texture'] or '').endswith('/lw_tutu_01_2A000'):continue
                v=t['v'][:,:3];normal=np.cross(v[1]-v[0],v[2]-v[0])
                if v[:,1].max()<3 and abs(normal[1])>.9*np.linalg.norm(normal):
                    self.assertLessEqual(v[:,1].max(),-2,'Wood threshold still protrudes through the grass')
                    count+=1
        self.assertGreater(count,300)

    def test_scene_changes_only_the_authorized_low_vertex_heights(self):
        changed={f'alt/{ROOT}room_{room}/{name}_vtx' for room in range(9) for name in['geometry','backing']}
        changed.add(f'alt/{ROOT}room_1/bridge_surround_vtx')
        prior=Resources(a.native,a.previous)
        wall_resources=set()
        for room in range(9):
            _,_,entries=read_mesh_header(prior.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
            for path in dict.fromkeys(p for _,o,x in entries for p in(o,x) if p):
                if any(t['path'].startswith(SCENE) and t['texture'] in {ROOT+'textures/rock',ROOT+'textures/vines'} for t in prior.mesh(path)):
                    wall_resources.update(['alt/'+path,'alt/'+path+'_vtx'])
        with zipfile.ZipFile(a.previous) as old,zipfile.ZipFile(a.candidate) as new:
            self.assertEqual(set(old.namelist()),set(new.namelist()))
            for name in old.namelist():
                # These wall removals have their own fascia/volume/local-closure
                # checks in test_wall_fins. Every other resource stays exact.
                if name=='woods-mystery-poc6.json' or name in wall_resources:continue
                expected=old.read(name)
                if name in changed:expected=expected_vertex_bytes(expected,name.endswith('/backing_vtx'))
                if name=='alt/'+SCENE+'Z2_26SARUNOMORI_room_01DL_002968_vtx':
                    xml=ET.fromstring(expected)
                    for v in xml:
                        if (v.get('X'),v.get('Y'),v.get('Z'))==('213','88','-214'):v.set('Y','119')
                    expected=ET.tostring(xml,encoding='utf-8')
                self.assertEqual(new.read(name),expected,'Unexpected scene change: '+name)

    @unittest.skipUnless(a.previous_native_assets,'Needs protected built-in bridge assets')
    def test_native_bridge_has_the_same_narrow_floor_correction(self):
        current=Path(__file__).resolve().parents[2]/'mm/assets/custom'
        for before in a.previous_native_assets.rglob('*'):
            if not before.is_file():continue
            name=before.relative_to(a.previous_native_assets);expected=before.read_bytes()
            if name.name in['bridge_vtx','wall_vtx']:expected=expected_vertex_bytes(expected)
            self.assertEqual((current/name).read_bytes(),expected,'Unexpected native asset change: '+str(name))

if __name__=='__main__':unittest.main()
