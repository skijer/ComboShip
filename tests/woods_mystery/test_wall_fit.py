"""Renderer-observed regressions: exposed wood rims and unobstructed inward sleeves."""
from pathlib import Path
import argparse,collections,sys,unittest,xml.etree.ElementTree as ET
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/woods_mystery'))
from build_candidate import Resources,ROOT,SCENE,read_mesh_header

p=argparse.ArgumentParser();p.add_argument('candidate');p.add_argument('native');p.add_argument('previous')
a,rest=p.parse_known_args();sys.argv=[sys.argv[0],*rest]
native=Resources(a.native);current=Resources(a.native,a.candidate);previous=Resources(a.native,a.previous)

def room_mesh(resources,room):
    _,_,entries=read_mesh_header(resources.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
    return [tri for path in dict.fromkeys(p for _,opa,xlu in entries for p in (opa,xlu) if p)
            for tri in resources.mesh(path)]

def first_hits(triangles,eye,directions):
    distances=np.full(len(directions),np.inf);ids=np.full(len(directions),-1)
    for i,tri in enumerate(triangles):
        p,q,r=tri['v'][:,:3];e1=q-p;e2=r-p;h=np.cross(directions,e2);det=h@e1
        valid=abs(det)>1e-8;inv=np.zeros(len(det));inv[valid]=1/det[valid]
        s=eye-p;u=(h@s)*inv;cross=np.cross(s,e1);v=(directions@cross)*inv;t=(e2@cross)*inv
        hit=valid&(u>=-1e-7)&(v>=-1e-7)&(u+v<=1+1e-7)&(t>1e-5)&(t<distances)
        distances[hit]=t[hit];ids[hit]=i
    return ids

class WallFit(unittest.TestCase):
    def test_every_maze_sleeve_reveals_wood_on_sides_and_ceiling(self):
        for room in range(9):
            triangles=room_mesh(current,room);center=np.array([-880*(room//3),0,880*(1-room%3)])
            for x,z in [(1,0),(-1,0),(0,1),(0,-1)]:
                direction=np.array([x,0,z]);side=np.array([-z,0,x])
                for depth in [260,340,420]:
                    eye=center+depth*direction+[0,55,0]
                    hits=first_hits(triangles,eye,np.stack([side,-side,[0,1,0]]))
                    for hit in hits:
                        self.assertGreaterEqual(hit,0,'Open sleeve exposes the sky')
                        self.assertIn('custom/cor_mm3d/',triangles[hit]['texture'],
                                      f'Native stone cuts through wood in room {room}, direction {(x,z)}, depth {depth}')

    def test_full_maze_rims_are_visible_from_the_clearings(self):
        for room in range(9):
            # Native props (including room 2's fern) may stand in front of a
            # mouth. Isolate the wall/tunnel surfaces responsible for burying it.
            triangles=[t for t in room_mesh(current,room) if (t['texture'] or '').startswith('custom/cor_mm3d/')
                       or t['texture'] in {ROOT+'textures/'+m for m in ['rock','vines','black']}]
            center=np.array([-880*(room//3),0,880*(1-room%3)])
            for x,z in [(1,0),(-1,0),(0,1),(0,-1)]:
                direction=np.array([x,0,z]);side=np.array([-z,0,x]);eye=center+[0,65,0]
                targets=np.asarray([center+direction*265+side*across+[0,height,0]
                                    for across,height in [(-97,48),(97,48),(-78,22),(78,22)]])
                hits=first_hits(triangles,eye,targets-eye)
                for hit in hits:
                    self.assertGreaterEqual(hit,0)
                    self.assertIn('custom/cor_mm3d/',triangles[hit]['texture'],
                                  f'Wall still buries the lower wood rim in room {room}, direction {(x,z)}')

    def test_bridge_crown_has_no_foreign_wall_strip(self):
        triangles=room_mesh(current,1);origin=np.array([110,-110]);axis=np.array([1,-1])/np.sqrt(2);side=np.array([1,1])/np.sqrt(2)
        xz=origin-axis*40;eye=np.array([xz[0],85,xz[1]])
        across,height=np.meshgrid(np.linspace(-50,50,201),np.linspace(95,125,61))
        xz=origin+axis*100+across.ravel()[:,None]*side
        hits=first_hits(triangles,eye,np.column_stack([xz[:,0],height.ravel(),xz[:,1]])-eye)
        stone={ROOT+'textures/'+m for m in ['rock','vines','ruins']}
        count=sum(i>=0 and triangles[i]['texture'] in stone for i in hits)
        self.assertEqual(count,0,f'{count} crown rays hit the protruding forest wall')

    def test_widened_backing_feet_remain_covered(self):
        triangles=room_mesh(current,0);center=np.array([0,0,880]);eye=center+[0,60,0]
        targets=[]
        for x,z in [(1,0),(-1,0),(0,1),(0,-1)]:
            direction=np.array([x,0,z]);side=np.array([-z,0,x])
            targets.extend(center+265*direction+across*side+[0,height,0]
                           for across in [-112,-100,-82,82,100,112] for height in [0,6,12])
        directions=np.asarray(targets)-eye
        self.assertTrue(np.all(first_hits(triangles,eye,directions)>=0),'Sky visible below widened backing')
        def wing(t):
            if t['texture']!=ROOT+'textures/grass' or t['v'][:,1].max()>10:return False
            q=t['v'][:,:3]-center
            return np.any(np.min(abs(q[:,[0,2]]),axis=1)>81)
        without=[t for t in triangles if not wing(t)]
        self.assertLess(len(without),len(triangles))
        self.assertTrue(np.any(first_hits(without,eye,directions)<0),'Negative control failed to expose missing floor wings')

    def test_far_surround_seals_backing_and_keeps_the_wood_passage_open(self):
        center=np.array([0,0,880]);triangles=[t for t in room_mesh(current,0)
            if t['path']==ROOT+'room_0/backing']
        for x,z in [(1,0),(-1,0),(0,1),(0,-1)]:
            d=np.array([x,0,z]);side=np.array([-z,0,x])
            for across,height,open_passage in [(0,30,True),(0,60,True),(0,100,True),(-20,60,True),(20,60,True),
                                               (0,140,False),(-90,140,False),(90,140,False),(-110,40,False),(110,40,False)]:
                eye=center+430*d+across*side+[0,height,0]
                hit=first_hits(triangles,eye,np.asarray([d]))[0]
                self.assertEqual(hit<0,open_passage,f'Far surround blocks the route or leaves a backing gap at {(x,z,across,height)}')

    def test_expanded_backing_uses_the_enclosing_visual_cull_entry(self):
        for room in range(9):
            path=ROOT+f'room_{room}/geometry';backing=ROOT+f'room_{room}/backing'
            xml=ET.fromstring(current.get(path))
            self.assertTrue(any(c.tag=='CallDisplayList' and c.get('Path')==backing for c in xml))
            _,_,entries=read_mesh_header(current.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
            bound=next(bound for bound,opa,xlu in entries if opa==path)
            vertices=np.concatenate([t['v'][:,:3] for t in current.mesh(backing)])
            self.assertTrue(np.all(np.linalg.norm(vertices-np.asarray(bound[:3]),axis=1)<bound[3]))
            self.assertEqual(bound[3],1600)

    def test_approved_bridge_and_poc3_materials_remain_exact(self):
        axis=np.array([1,-1])/np.sqrt(2);side=np.array([1,1])/np.sqrt(2)
        def key(t):
            v=t['v'].copy();low=np.isin(v[:,1],[-3,-2]);v[low,1]+=4
            return t['texture'],tuple(sorted(tuple(row) for row in v))
        before=[]
        for t in previous.mesh(ROOT+'room_1/geometry'):
            v=t['v'];q=v[:,[0,2]]-[110,-110]
            if np.all(q@axis>=-1) and np.all(q@axis<=152) and np.all(abs(q@side)<=70) and v[:,1].max()<=130:
                before.append(t)
        actual=collections.Counter(key(t) for t in current.mesh(ROOT+'room_1/geometry'))
        self.assertEqual(len(before),194)
        self.assertFalse(collections.Counter(key(t) for t in before)-actual,'Approved bridge/cap geometry or attributes changed')
        for path,data in previous.files.items():
            if path.startswith('alt/'+ROOT+'textures/'):
                self.assertEqual(current.get(path),data,'Protected texture changed')

if __name__=='__main__':unittest.main()
