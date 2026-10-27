"""Regression guards for the narrowly scoped oblique-eye/hair-root pass."""
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
import numpy as np


class HairOwnershipTests(unittest.TestCase):
    def test_root_paint_cannot_touch_a_face_or_body_island(self):
        self.assertIsNotNone(importlib.util.find_spec('refine_young_malon_eye_hair'),
                             'Hair root paint needs a mesh-owned UV transfer')
        import refine_young_malon_eye_hair as repair
        atlas=np.full((32,32,4),[220,174,145,255],np.uint8)
        p=np.array([[0,.94,0],[.1,.94,0],[0,.90,0],
                    [0,.94,0],[.1,.94,0],[0,.90,0]],float)
        uv=np.array([[.05,.05],[.40,.05],[.05,.40],
                     [.60,.60],[.95,.60],[.60,.95]])
        faces=np.array([[0,1,2],[3,4,5]])
        mesh=np.array([24,24,24,25,25,25])
        art=np.full((32,32,4),[180,65,25,255],np.uint8)
        patch=dict(bounds=[0,0,32,32],meshes=[24],artwork=art,min_y=.89)
        out,mask=repair.paint_hair_roots(p,uv,faces,mesh,atlas,[patch])
        self.assertGreater(mask.sum(),5)
        self.assertLess(int(out[4,4,1]),100)
        np.testing.assert_array_equal(out[18:,18:],atlas[18:,18:])
        np.testing.assert_array_equal(out[mask==0],atlas[mask==0])
        np.testing.assert_array_equal(out[:,:,3],atlas[:,:,3])
        np.testing.assert_array_equal(atlas,np.full_like(atlas,[220,174,145,255]))

    def test_hair_lower_than_the_root_band_remains_unchanged(self):
        self.assertIsNotNone(importlib.util.find_spec('refine_young_malon_eye_hair'))
        import refine_young_malon_eye_hair as repair
        atlas=np.full((16,16,4),[220,174,145,255],np.uint8)
        p=np.array([[0,.80,0],[.1,.80,0],[0,.82,0]],float)
        uv=np.array([[0,0],[1,0],[0,1]],float)
        patch=dict(bounds=[0,0,16,16],meshes=[1],
                   artwork=np.full((16,16,4),[180,65,25,255],np.uint8),min_y=.89)
        out,mask=repair.paint_hair_roots(p,uv,np.array([[0,1,2]]),np.ones(3,int),atlas,[patch])
        np.testing.assert_array_equal(out,atlas)
        self.assertFalse(mask.any())

    def test_authored_root_palette_warms_gray_roots_but_keeps_gold_highlights(self):
        import refine_young_malon_eye_hair as repair
        atlas=np.full((16,16,4),[220,174,149,255],np.uint8)
        atlas[3,3]=[240,180,72,255]
        p=np.array([[0,.94,0],[.1,.94,0],[0,.94,0]])
        uv=np.array([[0,0],[1,0],[0,1]],float)
        patch=dict(bounds=[0,0,16,16],meshes=[12],min_y=.89,
                   artwork=atlas,chroma=[1.,.422,.152])
        out,mask=repair.paint_hair_roots(p,uv,np.array([[0,1,2]]),np.full(3,12),atlas,[patch])
        self.assertLess(int(out[4,4,1]),115,'The opposite root must match the authored copper hair')
        np.testing.assert_array_equal(out[3,3],atlas[3,3])


class EyeSheetTests(unittest.TestCase):
    def test_repair_rejects_resolved_and_hardlinked_input_output_aliases(self):
        import refine_young_malon_eye_hair as repair
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);native=root/'native.o2r';parent=root/'parent.o2r';source=root/'source.glb'
            for p in (native,parent,source):p.write_bytes(b'protected original')
            alias=root/'alias.o2r';os.link(parent,alias)
            for output in (native,parent,source,alias):
                with self.subTest(output=output.name):
                    with self.assertRaisesRegex(ValueError,'protected input'):
                        repair.refine(native,parent,source,root/'art',output)
            for p in (native,parent,source):self.assertEqual(p.read_bytes(),b'protected original')

    def test_concave_rim_is_filled_once_without_crossed_triangles(self):
        from types import SimpleNamespace
        import refine_young_malon_eye_hair as repair
        xy=np.array([[0,0],[3,0],[3,1],[1,1],[1,3],[0,3]],float)
        local=np.column_stack([xy,np.zeros(len(xy))])
        normal=np.tile([0.,0.,1.],(len(xy),1))
        transform=np.column_stack([np.eye(3),np.zeros(3)])
        p,n,uv,faces=repair.eye_disk(local,normal,xy,local,np.zeros(6),0,
                                   transform,SimpleNamespace(T=np.zeros(3),M=np.eye(3)))
        np.testing.assert_array_equal(p[:len(local)],local)
        np.testing.assert_array_equal(n[:len(local)],normal)
        q=uv[faces];edge=q[:,1:]-q[:,0,None,:]
        area=edge[:,0,0]*edge[:,1,1]-edge[:,0,1]*edge[:,1,0]
        self.assertTrue((area>1e-10).all() or (area< -1e-10).all(),
                        'The iris surface must not fold over itself inside a concave lid rim')
        self.assertAlmostEqual(float(abs(area).sum()/2),5.,places=7)


@unittest.skipUnless(os.environ.get('MALON_EYE_HAIR_CANDIDATE'),'Select the exact eye/hair archive')
class ArchiveEyeOwnershipTests(unittest.TestCase):
    def test_oblique_iris_return_uses_iris_color_instead_of_front_lid_skin(self):
        from scipy.spatial.transform import Rotation
        from native_npc_models import NativeRig,animation_frames
        from verify_native_npc_models import Archive
        from native_assets.render import Tex,_sample_bilinear
        root=Path(os.environ['MALON_REVIEW_FIXTURES'])
        a=Archive(root/'native-reference/oot.o2r',os.environ['MALON_EYE_HAIR_CANDIDATE'])
        rig=NativeRig(a,'young-malon',True)
        clip=rig.ns+'/PoseAnimations/objects/object_ma1/gMalonChildIdleAnim'
        rig.pose(animation_frames(a.read(clip))[0])
        data=a.geometry(rig.ns+'/Faces/L0R0M0',rig,14)
        camera=Rotation.from_euler('yx',[45,0],degrees=True).as_matrix()
        p=(data['local']@rig.limbs[14].M.T+rig.limbs[14].T)@camera.T
        head=camera@rig.limbs[14].T;span=p[:,1].max()-head[1]+120
        cy=head[1]-60+span/2;ppu=700/(span*1.2)
        xy=np.column_stack([350+(p[:,0]-head[0])*ppu,350-(p[:,1]-cy)*ppu])
        cache={}
        for probe in ((408,367),(414,373)):
            hits=[]
            for face,texture in zip(data['faces'],data['textures']):
                q=xy[face];m=np.vstack([q.T,np.ones(3)])
                if abs(np.linalg.det(m))<1e-8:continue
                bary=np.linalg.solve(m,[probe[0]+.5,probe[1]+.5,1])
                if bary.min() < -1e-8:continue
                if texture not in cache:
                    kind,w,h,raw=a.texture(texture)
                    self.assertEqual(kind,1)
                    tex=Tex(np.frombuffer(raw,np.uint8).reshape(h,w,4),2,2)
                    tex.scale_s,tex.scale_t=a.texture_scale(texture);cache[texture]=tex
                tex=cache[texture];uv=bary@(data['uv'][face]*512)
                rgba=_sample_bilinear(tex,np.asarray(uv[0]*tex.scale_s),np.asarray(uv[1]*tex.scale_t),2,2)
                if rgba[3]>8:hits.append((float(bary@p[face,2]),rgba))
            self.assertTrue(hits)
            rgba=max(hits,key=lambda hit:hit[0])[1]
            self.assertGreater(rgba[2],rgba[0]*1.1,
                               f'Oblique iris probe {probe} contains eyelid skin: {rgba.tolist()}')


if __name__=='__main__':unittest.main()
