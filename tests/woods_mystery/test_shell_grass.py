"""Close-camera shoulder slits and a uniform detailed grass material."""
from pathlib import Path
import argparse,collections,sys,unittest,xml.etree.ElementTree as ET
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/woods_mystery'))
from build_candidate import Resources,ROOT,SCENE,FLOOR,read_mesh_header

p=argparse.ArgumentParser();p.add_argument('candidate');p.add_argument('native');p.add_argument('previous')
a,rest=p.parse_known_args();sys.argv=[sys.argv[0],*rest]
current=Resources(a.native,a.candidate);native=Resources(a.native);previous=Resources(a.native,a.previous)

def room_mesh(r,room):
    _,_,entries=read_mesh_header(r.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
    return [t for path in dict.fromkeys(p for _,o,x in entries for p in (o,x) if p) for t in r.mesh(path)]

def nearest(triangles,eye,direction):
    result=np.inf
    for t in triangles:
        p,q,r=t['v'][:,:3];a=q-p;b=r-p;h=np.cross(direction,b);det=a@h
        if abs(det)<1e-8:continue
        s=eye-p;u=(s@h)/det;c=np.cross(s,a);v=(direction@c)/det;distance=(b@c)/det
        if u>=-1e-7 and v>=-1e-7 and u+v<=1+1e-7 and distance>1e-5:result=min(result,distance)
    return result

class ShellGrass(unittest.TestCase):
    def test_every_near_shoulder_has_a_local_opaque_join(self):
        for room in range(9):
            center=np.array([-880*(room//3),0,880*(1-room%3)])
            triangles=[t for t in room_mesh(current,room) if t['texture'] in
                {ROOT+'textures/'+m for m in ['rock','vines','grass','black']}
                or (t['texture'] or '').endswith(('/lw_tutu_01_2A000','/lw_wood_02_2CA00'))]
            for x,z in [(1,0),(-1,0),(0,1),(0,-1)]:
                d=np.array([x,0,z]);side=np.array([-z,0,x])
                for sign in [-1,1]:
                    for height in [8,25,50,80,105,125]:
                        eye=center+190*d+100*sign*side+[0,height,0]
                        ray=18*d+14.5*sign*side
                        self.assertLessEqual(nearest(triangles,eye,ray),2,
                            f'Close shoulder exposes the sky: room{room}, direction{(x,z)}, side{sign}, y{height}')

    def test_main_floor_and_added_coverage_use_the_same_detail_sampler(self):
        for room in range(9):
            before=room_mesh(native,room)
            paths={t['path'] for t in before if t['texture']==FLOOR}
            self.assertEqual(len(paths),1)
            path=next(iter(paths));data=current.get(path)
            self.assertTrue(data.startswith(b'<DisplayList'),'Main grass still uses the native distance-blended two-stage material')
            for draw_path in [path,ROOT+f'room_{room}/backing']:
                xml=ET.fromstring(current.get(draw_path));active_texture=None;combine=None;tile=None;groups=[]
                for c in xml:
                    if c.tag=='SetTextureImage':active_texture=c.get('Path')
                    if c.tag=='SetCombineLERP':combine=c.attrib
                    if c.tag=='SetTile' and c.get('Tile')=='0':tile=c.attrib
                    if c.tag.startswith('Triangle') and active_texture==ROOT+'textures/grass':
                        groups.append((combine,tile))
                self.assertTrue(groups)
                for combine,tile in groups:
                    self.assertNotIn('G_CCMUX_TEXEL1',combine.values(),'A second layer blurs the detailed grass')
                    self.assertEqual(combine['A0'],'G_CCMUX_TEXEL0')
                    self.assertEqual((tile['ShiftS'],tile['ShiftT']),('0','0'),'Grass density changes at a material boundary')
            # Keep every original grass position and normal; only the material changes.
            shape=lambda ts:collections.Counter(tuple(sorted(tuple(v[[0,1,2,5,6,7,8]]) for v in t['v'])) for t in ts)
            self.assertEqual(shape([t for t in before if t['path']==path]),shape(current.mesh(path)))
            all_grass=[t for t in room_mesh(current,room) if t['texture']==ROOT+'textures/grass']
            for t in all_grass:
                v=t['v'];expected=-6.4*v[:,[0,2]]
                self.assertLessEqual(abs(expected-v[:,3:5]).max(),.51,'Grass tile phase changes across a room or material boundary')

if __name__=='__main__':unittest.main()
