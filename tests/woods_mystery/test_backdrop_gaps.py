"""Clip regressions: boundary-mask silhouettes, real floor holes and black state."""
from pathlib import Path
import argparse,collections,sys,unittest,xml.etree.ElementTree as ET
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools/woods_mystery'))
from build_candidate import Resources,ROOT,SCENE,read_mesh_header,section_profile

p=argparse.ArgumentParser();p.add_argument('candidate');p.add_argument('native');p.add_argument('previous')
a,rest=p.parse_known_args();sys.argv=[sys.argv[0],*rest]
native=Resources(a.native);current=Resources(a.native,a.candidate);previous=Resources(a.native,a.previous)

def room_mesh(resources,room):
    _,_,entries=read_mesh_header(resources.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
    return [t for path in dict.fromkeys(p for _,opa,xlu in entries for p in (opa,xlu) if p)
            for t in resources.mesh(path)]

def hits(triangles,eyes,directions):
    distances=np.full(len(directions),np.inf)
    for tri in triangles:
        p,q,r=tri['v'][:,:3];e1=q-p;e2=r-p;h=np.cross(directions,e2);det=h@e1
        valid=abs(det)>1e-8;inv=np.zeros(len(det));inv[valid]=1/det[valid]
        s=eyes-p;u=np.einsum('ij,ij->i',h,s)*inv;cross=np.cross(s,e1)
        v=np.einsum('ij,ij->i',directions,cross)*inv;t=(cross@e2)*inv
        ok=valid&(u>=-1e-7)&(v>=-1e-7)&(u+v<=1+1e-7)&(t>1e-5)&(t<distances)
        distances[ok]=t[ok]
    return distances

class BackdropGaps(unittest.TestCase):
    def test_bridge_upper_wall_has_opaque_coverage_beneath_cutout_foliage(self):
        triangles=[t for t in room_mesh(current,1) if t['texture'] in
            {ROOT+'textures/'+m for m in ['rock','vines','grass','black']}
            or (t['texture'] or '').endswith(('/lw_tutu_01_2A000','/lw_wood_02_2CA00'))]
        axis=np.array([1,-1])/np.sqrt(2);side=np.array([1,1])/np.sqrt(2);origin=np.array([110,-110])
        for camera_across,camera_height,target_across,target_height in [
            (-130,100,-4,156),(-80,110,-24,160),(-80,125,-22,160),(130,110,22,158)]:
            xz=origin-40*axis+camera_across*side;eye=np.array([xz[0],camera_height,xz[1]])
            xz=origin+100*axis+target_across*side;target=np.array([xz[0],target_height,xz[1]])
            self.assertTrue(np.isfinite(hits(triangles,eye[None,:],(target-eye)[None,:])[0]),
                            f'Upper wall silhouette opens behind transparent foliage from {camera_across,camera_height}')

    def test_bridge_cap_perimeter_is_welded_to_the_exterior_backing(self):
        cap=[t for t in current.mesh(ROOT+'room_1/geometry')
             if t['path']==ROOT+'room_1/geometry' and t['texture']==ROOT+'textures/black']
        surround=current.mesh(ROOT+'room_1/bridge_surround') if current.get(ROOT+'room_1/bridge_surround') else []
        def edges(triangles):
            return collections.Counter(tuple(sorted((tuple(v[:3]),tuple(t['v'][(i+1)%3,:3]))))
                for t in triangles for i,v in enumerate(t['v']))
        boundary={edge for edge,n in edges(cap).items() if n==1}
        self.assertEqual(len(boundary),28)
        self.assertFalse(boundary-set(edges(surround)),'Bridge exterior is not welded to every actual cap edge')
        self.assertTrue(surround and all(t['lit'] and np.all(t['v'][:,8]==255) for t in surround))

    def test_all_added_masks_and_backing_fit_their_actual_cull_entry(self):
        for room in range(9):
            geometry=ROOT+f'room_{room}/geometry'
            _,_,entries=read_mesh_header(current.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
            bound=next(b for b,opa,xlu in entries if opa==geometry)
            xml=ET.fromstring(current.get(geometry))
            calls={c.get('Path') for c in xml if c.tag=='CallDisplayList'}
            if room==1:self.assertIn(ROOT+'room_1/bridge_surround',calls)
            vertices=np.concatenate([t['v'][:,:3] for t in current.mesh(geometry)])
            self.assertTrue(np.all(np.linalg.norm(vertices-bound[:3],axis=1)<bound[3]))

    def test_only_original_boundary_exits_receive_fitted_black_masks(self):
        count=0
        for room in range(9):
            center=np.array([-880*(room//3),0,880*(1-room%3)])
            old=[t for t in room_mesh(native,room) if t['texture'] is None]
            directions=set()
            for t in old:
                mean=(t['v'][:,:3]-center).mean(0)[[0,2]];axis=int(abs(mean).argmax())
                d=np.zeros(2,int);d[axis]=int(np.sign(mean[axis]));directions.add(tuple(d))
            triangles=room_mesh(current,room)
            masks=[t for t in triangles if t['texture'] in {None,ROOT+'textures/black'}]
            wood=[t for t in triangles if (t['texture'] or '').startswith('custom/cor_mm3d/')]
            for d in [(1,0),(-1,0),(0,1),(0,-1)]:
                d=np.asarray(d);side=np.array([-d[1],d[0]])
                caps=[t for t in masks if np.max(abs((t['v'][:,[0,2]]-center[[0,2]])@d-440))<1]
                self.assertEqual(bool(caps),tuple(d) in directions,'Boundary masks moved to a daily route')
                if not caps:continue
                count+=1;profile=section_profile(wood,center[[0,2]],d,440)
                # Probe near every actual arch edge, where the old pentagon left crescents.
                midpoint=np.array([0,60]);points=[]
                for i,start in enumerate(profile):
                    for f in np.linspace(0,1,17):
                        point=start*(1-f)+profile[(i+1)%len(profile)]*f
                        points.append(midpoint+.99*(point-midpoint))
                xz=center[[0,2]]+430*d+np.asarray(points)[:,0,None]*side
                eyes=np.column_stack([xz[:,0],np.asarray(points)[:,1],xz[:,1]])
                rays=np.tile([d[0],0,d[1]],(len(eyes),1))
                self.assertTrue(np.isfinite(hits(caps,eyes,rays)).all(),f'Native exit has uncovered arch edges: room {room}, {d}')
        self.assertEqual(count,12)

    def test_wall_foot_holes_have_grass_directly_below_them(self):
        # Downward rays cannot be falsely satisfied by distant wall/backdrop triangles.
        x,z=np.meshgrid(np.arange(-260,261,5),np.arange(-260,261,5))
        offsets=np.column_stack([x.ravel(),np.full(x.size,.5),z.ravel()])
        rays=np.tile([0,-1,0],(len(offsets),1))
        for room in range(9):
            center=np.array([-880*(room//3),0,880*(1-room%3)])
            grass=[t for t in room_mesh(current,room) if t['texture']==ROOT+'textures/grass']
            distances=hits(grass,offsets+center,rays)
            self.assertTrue(np.isfinite(distances).all(),f'Uncovered clearing grass footprint in room {room}: {np.sum(~np.isfinite(distances))} samples')
            self.assertTrue((distances<=1.6).all(),'Ground backing is too deeply recessed')

    def test_black_caps_explicitly_disable_fog_and_use_opaque_blending(self):
        black_triangles=0
        for path,data in current.files.items():
            if not path.startswith('alt/'+ROOT) or not data.startswith(b'<DisplayList'):continue
            texture=None;fog=None;opaque=False
            for c in ET.fromstring(data):
                if c.tag=='SetTextureImage':texture=c.get('Path')
                if c.tag=='SetGeometryMode' and 'G_FOG' in c.attrib:fog=True
                if c.tag=='ClearGeometryMode' and 'G_FOG' in c.attrib:fog=False
                if c.tag=='SetOtherMode' and c.get('Cmd')=='G_SETOTHERMODE_L':
                    opaque='G_RM_AA_ZB_OPA_SURF' in c.attrib and 'G_RM_AA_ZB_OPA_SURF2' in c.attrib
                if c.tag.startswith('Triangle') and texture==ROOT+'textures/black':
                    black_triangles+=1
                    self.assertIs(fog,False,'Solid black backdrop inherits fog shading')
                    self.assertTrue(opaque,'Solid black backdrop inherits fog blending')
        self.assertGreater(black_triangles,28,'Native boundary backdrops were not fitted')

    def test_poc4_wood_textures_foliage_and_floor_shape_remain_exact(self):
        def key(t):
            v=t['v'].copy()
            if (t['texture'] or '').endswith(('/lw_tutu_01_2A000','/lw_wood_02_2CA00','/textures/black')):
                low=np.isin(v[:,1],[-3,-2]);v[low,1]+=4
            return t['texture'],tuple(sorted(tuple(row) for row in v))
        for room in range(9):
            before=previous.mesh(ROOT+f'room_{room}/geometry');after=current.mesh(ROOT+f'room_{room}/geometry')
            # Backing near-edges intentionally move in POC6 to close the
            # close-camera shoulder seam. The accepted wood/cap list is exact.
            protected=[t for t in before if t['path']==ROOT+f'room_{room}/geometry']
            missing=collections.Counter(map(key,protected))-collections.Counter(map(key,after))
            self.assertFalse(bool(missing),f'{sum(missing.values())} accepted wood/cap faces changed')
        for name,data in previous.files.items():
            if name.startswith('alt/'+ROOT+'textures/'):
                self.assertEqual(current.get(name),data)
        for room in range(9):
            before=room_mesh(previous,room);after=room_mesh(current,room)
            old_grass=[t for t in before if t['texture']==ROOT+'textures/grass']
            grass_shape=lambda t:tuple(sorted(tuple(v[[0,2,5,6,7,8]]) for v in t['v']))
            self.assertFalse(collections.Counter(map(grass_shape,old_grass))-
                collections.Counter(grass_shape(t) for t in after if t['texture']==ROOT+'textures/grass'))
            foliage=[t for t in before if (t['texture'] or '').startswith(ROOT+'textures/henriko_')]
            low_paths={t['path'] for t in foliage if t['v'][:,1].min()<200}
            def foliage_key(t):
                if t['path'] in low_paths:
                    v=t['v'].copy()
                    if room==1:v[np.all(v[:,:3]==[213,119,-214],axis=1),1]=88
                    return t['path'],tuple(sorted(tuple(row[[0,1,2,3,5,6,7,8]]) for row in v))
                return key(t)
            self.assertEqual(collections.Counter(map(foliage_key,foliage)),collections.Counter(foliage_key(t) for t in after
                if (t['texture'] or '').startswith(ROOT+'textures/henriko_')))

if __name__=='__main__':unittest.main()
