"""Regressions from cor's play-renderer clip: solid joins and the approved bridge mouth."""
from pathlib import Path
import argparse,collections,sys,unittest,xml.etree.ElementTree as ET
import numpy as np

root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools/woods_mystery'))
from scene_assets import Resources
from build_candidate import ROOT,SCENE,WALL,FLOOR,read_mesh_header
from build_candidate import recover,tunnel_at

parser=argparse.ArgumentParser();parser.add_argument('candidate');parser.add_argument('native');parser.add_argument('approved')
parser.add_argument('--donor',type=Path,required=True)
args,rest=parser.parse_known_args();candidate_path,native_path,approved_path=args.candidate,args.native,args.approved
sys.argv=[sys.argv[0],*rest]
native=Resources(native_path);candidate=Resources(native_path,candidate_path);approved=Resources(native_path,approved_path)
donor=recover(Resources(args.donor))
lining=SCENE+'Z2_26SARUNOMORITex_00B9C0'
axis=np.array([1,-1])/np.sqrt(2);side=np.array([1,1])/np.sqrt(2)

def key(tri,columns=slice(None)):
    vertices=tri['v'].copy()
    if (tri['texture'] or '').endswith(('/lw_tutu_01_2A000','/lw_wood_02_2CA00','/textures/black')):
        # Undo only the authorized submerged floor displacement for historical
        # shape comparisons. test_floor_contact checks its exact actual delta.
        low=np.isin(vertices[:,1],[-3,-2]);vertices[low,1]+=4
    return (tri['texture'],tuple(sorted(tuple(np.rint(row[columns]).astype(int)) for row in vertices)))

def preserved_key(tri):
    # The requested interior repair changes UVs and baked RGB only. Exterior
    # rim attributes and every inward/outward triangle position stay approved.
    return key(tri,slice(0,3) if 'lw_tutu_01_2A000' in tri['texture'] else slice(None))

def room_paths(room):
    _,_,entries=read_mesh_header(native.get(SCENE+f'Z2_26SARUNOMORI_room_{room:02d}'))
    return list(dict.fromkeys(p for _,opa,xlu in entries for p in (opa,xlu) if p))

def walls(room):
    return next(p for p in room_paths(room) if any(t['texture']==WALL for t in native.mesh(p)))

def ray_hit(triangles,origin,direction):
    for tri in triangles:
        a,b,c=tri['v'][:,:3];e1=b-a;e2=c-a;h=np.cross(direction,e2);det=e1@h
        if abs(det)<1e-8:continue
        q=origin-a;u=(q@h)/det
        if u<-1e-8 or u>1+1e-8:continue
        cross=np.cross(q,e1);v=(direction@cross)/det;distance=(e2@cross)/det
        if v>=-1e-8 and u+v<=1+1e-8 and distance>0:return True
    return False

def fitted_interior(actual,expected):
    shape={key(t,slice(0,3)) for t in expected if 'lw_tutu_01_2A000' in t['texture']}
    return [t for t in actual if key(t,slice(0,3)) in shape]


class TunnelSeams(unittest.TestCase):
    def assert_continuous_interior(self,triangles):
        vertices=collections.defaultdict(list)
        for tri in triangles:
            for v in tri['v']:vertices[tuple(v[:3])].append(v)
        for xyz,rows in vertices.items():
            a=np.asarray(rows)
            self.assertEqual(len(set(a[:,4])),1,'Non-wrapping T seam at '+str(xyz))
            self.assertEqual(len(set(a[:,3]%1024)),1,'S phase seam at '+str(xyz))
            self.assertEqual(len({tuple(v[5:8]) for v in a}),1,'Isolated wood shading wedge at '+str(xyz))
            self.assertTrue(np.all((a[:,4]>=16)&(a[:,4]<=800)),'Interior samples the bark strip in the plank atlas')

    def test_native_fading_lining_becomes_solid_tiled_surfaces(self):
        count=0
        for room in range(9):
            for path in room_paths(room):
                before=native.mesh(path)
                if not any(t['texture']==lining for t in before):continue
                backing=ROOT+f'room_{room}/backing'
                after=candidate.mesh(backing);count+=1
                self.assertFalse(candidate.mesh(path),'Native short cull sphere still owns the expanded backing')
                self.assertTrue(all(t['texture'] in {ROOT+'textures/'+m for m in ['grass','rock','vines']} for t in after),
                                'The black IA lining still exposes the skybox')
                self.assertGreater(len(after),len(before)+24,'Missing grass wings or the open exit surround')
                xyz=lambda ts:collections.Counter(tuple(sorted(tuple(v[[0,2]]) for v in t['v'])) for t in ts)
                self.assertFalse(xyz([t for t in before if t['v'][:,1].max()<=10])-
                                 xyz([t for t in after if t['v'][:,1].max()<=10]),'Vanilla grass corridor footprint moved')
                center=np.array([-880*(room//3),0,880*(1-room%3)])
                for t in after:
                    if t['v'][:,1].max()<=10:continue
                    q=t['v'][:,:3]-center
                    exit_face=any(np.all(q[:,a]==sign*450) for a in [0,2] for sign in [-1,1])
                    if not exit_face:self.assertTrue(np.all((q[:,1]<90)|(q[:,1]>=145)))
                    self.assertTrue(np.all(np.min(abs(q[:,[0,2]]),axis=1)<=116))
                self.assertTrue(all(t['lit'] and np.all(t['v'][:,8]==255) for t in after))
                xml=ET.fromstring(candidate.get(backing))
                self.assertTrue(any('G_RM_AA_ZB_OPA_SURF2' in command.attrib for command in xml))
                for tri in after:
                    uv=tri['v'][:,3:5];self.assertGreater(abs(np.linalg.det(uv[1:]-uv[0])),10,'Wall tiling collapsed')
        self.assertEqual(count,9)

    def test_native_wall_above_each_maze_mouth_stays_closed(self):
        count=0
        for room in range(9):
            center=np.array([-880*(room//3),0,880*(1-room%3)])
            triangles=candidate.mesh(walls(room))
            for x,z in [(1,0),(-1,0),(0,1),(0,-1)]:
                direction=np.array([x,0,z]);side=np.array([-z,0,x])
                for across in [-100,0,100]:
                    count+=1;eye=center+100*direction+across*side+[0,145,0]
                    self.assertTrue(ray_hit(triangles,eye,direction),'Missing wall above the wood rim')
        self.assertEqual(count,108)

    def test_all_maze_mouths_move_forward_without_separating_far_joins(self):
        for room in range(9):
            center=np.array([-880*(room//3),0,880*(1-room%3)])
            triangles=candidate.mesh(ROOT+f'room_{room}/geometry')
            actual=collections.Counter(preserved_key(t) for t in triangles)
            for d in [np.array(x) for x in [(1,0),(-1,0),(0,1),(0,-1)]]:
                expected=tunnel_at(donor,center[[0,2]]+175*d,d,length=275)
                self.assertFalse(collections.Counter(preserved_key(t) for t in expected)-actual,
                                 'Maze mouth placement or far sleeve join differs from the short fitted passage')
                interior=fitted_interior(triangles,expected)
                self.assertEqual(len(interior),sum('lw_tutu_01_2A000' in t['texture'] for t in expected))
                self.assert_continuous_interior(interior)

    def test_approved_bridge_wood_geometry_is_unchanged(self):
        expected=[]
        for tri in approved.mesh(ROOT+'room_1/geometry'):
            v=tri['v'];q=v[:,[0,2]]-[110,-110];a=q@axis;c=q@side
            if tri['texture'].startswith('custom/cor_mm3d/') and np.all(a>=-1) and np.all(a<=121) and np.all(abs(c)<=70) and v[:,1].max()<=130:
                expected.append(tri)
        self.assertEqual(len(expected),110)
        actual=collections.Counter(preserved_key(t) for t in candidate.mesh(ROOT+'room_1/geometry'))
        self.assertFalse(collections.Counter(preserved_key(t) for t in expected)-actual,'Approved wood shape or exterior rim changed')

    def test_interior_wood_has_continuous_uvs_and_vertex_shading(self):
        expected=[]
        for tri in approved.mesh(ROOT+'room_1/geometry'):
            v=tri['v'];q=v[:,[0,2]]-[110,-110]
            if 'lw_tutu_01_2A000' in tri['texture'] and np.all(q@axis>=-1) and np.all(q@axis<=121) and np.all(abs(q@side)<=70):
                expected.append(tri)
        self.assertEqual(len(expected),86)
        actual=fitted_interior(candidate.mesh(ROOT+'room_1/geometry'),expected)
        self.assertEqual(len(actual),86,'Interior shape changed during the material correction')
        self.assert_continuous_interior(actual)

    def test_serialized_bridge_end_has_no_unpaired_edges(self):
        edges=collections.Counter()
        for tri in candidate.mesh(ROOT+'room_1/geometry'):
            if not ('lw_tutu_01_2A000' in tri['texture'] or tri['texture']==ROOT+'textures/black'):continue
            v=tri['v'][:,:3];q=v[:,[0,2]]-[110,-110]
            if not (np.all(q@axis>=-1) and np.all(q@axis<=152) and np.all(abs(q@side)<=70)):continue
            for i,a in enumerate(v):
                b=v[(i+1)%3]
                if min((p[[0,2]]-[110,-110])@axis for p in [a,b])<119:continue
                edges[tuple(sorted([tuple(a),tuple(b)]))]+=1
        self.assertGreater(len(edges),40)
        self.assertTrue(all(count==2 for count in edges.values()),'Unwelded integer sleeve/cap boundary')

    def test_quantized_bridge_joins_block_sky_rays(self):
        triangles=[]
        for p in [*room_paths(1),ROOT+'room_1/geometry']:triangles.extend(candidate.mesh(p))
        def point(depth,across,height):
            xz=np.array([110,-110])+axis*depth+side*across
            return np.array([xz[0],height,xz[1]])
        eye=point(-50,0,60)
        for across,height in [(-41,79),(-40,41),(43,76)]:
            self.assertTrue(ray_hit(triangles,eye,point(100,across,height)-eye),
                            f'Sky crack at quantized sleeve/cap join toward {(across,height)}')
        for eye_across,across,height in [(-80,-67,78),(80,74,35)]:
            eye=point(-40,eye_across,60)
            self.assertTrue(ray_hit(triangles,eye,point(100,across,height)-eye),
                            f'Sky crack in clipped native surround from {eye_across}')
        for eye_across,across,height in [(-80,-87,15),(-80,-58,6),(80,87,15),(80,58,6)]:
            eye=point(-40,eye_across,60)
            self.assertTrue(ray_hit(triangles,eye,point(100,across,height)-eye),
                            f'Opened shoulder/lower aperture exposes sky toward {(eye_across,across,height)}')

    def test_bridge_corner_is_a_forest_wall_with_an_open_tunnel_cutout(self):
        ts=candidate.mesh(walls(1))
        # A trimmed shoulder is now owned by the existing forest backing.
        # Test the actual opaque surround rather than its former draw owner.
        ts+=[t for path in [ROOT+'room_1/backing',ROOT+'room_1/bridge_surround'] for t in candidate.mesh(path)
             if t['texture'] in {ROOT+'textures/rock',ROOT+'textures/vines'}]
        self.assertFalse(any(t['texture']==ROOT+'textures/ruins' for t in ts),'Detached stone panels remain')
        direction=np.array([axis[0],0,axis[1]])
        for across,height in [(0,145),(-75,75),(75,75)]:
            p=np.array([110,-110])+side*across-axis*30
            self.assertTrue(ray_hit(ts,np.array([p[0],height,p[1]]),direction),'Sky gap in the bridge surround')
        for height in [25,60,90]:
            p=np.array([110,-110])-axis*30
            self.assertFalse(ray_hit(ts,np.array([p[0],height,p[1]]),direction),'New forest wall blocks the approved tunnel')

    def test_dark_cap_is_recessed_and_follows_the_tunnel_profile(self):
        black=[t for t in candidate.mesh(ROOT+'room_1/geometry')
               if t['texture']==ROOT+'textures/black' and t['path']==ROOT+'room_1/geometry']
        self.assertEqual(len(black),28,'Bridge cap must retain every terminal edge')
        vertices=np.concatenate([t['v'][:,:3] for t in black]);q=vertices[:,[0,2]]-[110,-110]
        self.assertTrue(np.all(abs(q@axis-150)<1),'Dark cap must be at the recessed sleeve end')
        across=q@side;top=vertices[:,1]>vertices[:,1].max()-1
        self.assertLess(max(abs(across[top])),max(abs(across))-5,'Black corners protrude past the curved crown')

    def test_unrevised_assets_and_room_headers_stay_byte_identical(self):
        allowed={ROOT+f'room_{room}/geometry'+suffix for room in range(9) for suffix in ['', '_vtx']}
        for room in range(9):
            for p in room_paths(room):
                # Floor material/UV phase are explicitly revised by POC6;
                # original positions/normals are protected in test_shell_grass.
                if any(t['texture'] in [WALL,lining,FLOOR] for t in native.mesh(p)):allowed.update([p,p+'_vtx'])
                # Only the existing low curtain receives the fitted forest strip;
                # test_foliage protects its XYZ/S/normals/alpha and artwork.
                if any(t['texture']==ROOT+'textures/henriko_forest_edge' and t['v'][:,1].min()<200
                       for t in candidate.mesh(p)):allowed.update([p,p+'_vtx'])
        checked=0
        for name,data in approved.files.items():
            if name.startswith('alt/'):
                path=name[4:]
                if path in allowed:continue
                self.assertEqual(candidate.get(path),data,'Unrelated POC2 asset changed: '+path);checked+=1
        self.assertGreaterEqual(checked,72,'Unrevised texture, high foliage and header protection was lost')


if __name__=='__main__':unittest.main()
