"""Geometry/animation contracts for the child-only final review repair."""
import importlib.util
import os
from pathlib import Path
import unittest

import numpy as np


class FaceRepairTests(unittest.TestCase):
    @staticmethod
    def aperture():
        yy,xx=np.indices((512,512))
        x=-.095+xx/511*.19;y=.920-yy/511*.152
        return (((x-.030)/.023)**2+((y-.855)/.014)**2<1)|(((x+.032)/.023)**2+((y-.855)/.014)**2<1)

    def module(self):
        self.assertIsNotNone(importlib.util.find_spec('refine_young_malon_face_idle'),
                             'Child Malon needs a source-gated face/idle asset repair')
        import refine_young_malon_face_idle
        return refine_young_malon_face_idle

    def test_eye_dent_is_removed_without_moving_the_surrounding_face(self):
        repair = self.module()
        x, y = np.meshgrid(np.linspace(-.07, .07, 39), np.linspace(.79, .92, 37))
        p = np.column_stack([x.ravel(), y.ravel(), (.095-6*x*x).ravel()])
        radius = ((p[:, 0]-.030)/.024)**2+((p[:, 1]-.855)/.018)**2
        p[:, 2] -= .012*np.maximum(1-radius, 0)**2
        original = p.copy()
        import inspect
        self.assertIn('aperture',inspect.signature(repair.repair_eye_depth).parameters,
                      'Eye correction must require the actual visible aperture, not a broad orbital ellipse')
        out = repair.repair_eye_depth(p,aperture=self.aperture())
        center = np.argmin(radius)
        self.assertGreater(out[center, 2], original[center, 2]+.008)
        np.testing.assert_array_equal(out[:, :2], original[:, :2])
        outside = (radius >= 1.4)&(p[:, 0] > 0)
        np.testing.assert_array_equal(out[outside], original[outside])
        np.testing.assert_array_equal(p, original)

    def test_eye_repair_retains_eyelid_and_socket_depth_outside_the_painted_aperture(self):
        import inspect
        from scipy.ndimage import map_coordinates
        repair=self.module()
        self.assertIn('aperture',inspect.signature(repair.repair_eye_depth).parameters,
                      'The surrounding eyelid skin must retain its original shape')
        x,y=np.meshgrid(np.linspace(-.075,.075,81),np.linspace(.815,.895,61))
        # Include a raised lid ridge that lies inside the old broad ellipse.
        z=.08+.012*np.exp(-((y-.874)/.004)**2)
        p=np.column_stack([x.ravel(),y.ravel(),z.ravel()]);original=p.copy()
        mask=self.aperture()
        pixel=[(.920-p[:,1])/.152*511,(p[:,0]+.095)/.19*511]
        visible=map_coordinates(mask.astype(float),pixel,order=1,mode='constant')>.5
        out=repair.repair_eye_depth(p,aperture=mask)
        np.testing.assert_array_equal(out[~visible],original[~visible],
                                      err_msg='No cheek, brow, eye corner or lid ridge may be flattened')
        np.testing.assert_array_equal(out[:,:2],original[:,:2])
        self.assertLess(float(abs(out[:,2]-p[:,2]).max()),.008)

    def test_uv_split_skin_normals_share_a_continuous_geometric_field(self):
        repair = self.module()
        x, y = np.meshgrid(np.linspace(-.06, .06, 19), np.linspace(.78, .92, 19))
        p = np.column_stack([x.ravel(), y.ravel(), (.09-5*x*x-2*(y-.85)**2).ravel()])
        faces=[]
        for row in range(18):
            for col in range(18):
                a=row*19+col
                faces.extend(((a,a+1,a+20),(a,a+20,a+19)))
        p=np.vstack([p,p[180]])
        faces=np.asarray(faces)
        normals=np.tile([0.,0.,1.],(len(p),1))
        normals[-1]=[.8,0.,.6]
        out=repair.skin_normal_field(p,faces)
        np.testing.assert_allclose(out[-1],out[180],atol=1e-12)
        self.assertLess(np.linalg.norm(out[180]-out[181]),.09)
        np.testing.assert_allclose(np.linalg.norm(out,axis=1),1.,atol=1e-12)

    def test_soft_nose_does_not_introduce_a_sharp_filter_boundary(self):
        repair=self.module()
        x,y=np.meshgrid(np.linspace(-.045,.045,101),np.linspace(.79,.86,65))
        z=.09+.018*np.exp(-(x/.018)**2-((y-.824)/.016)**2)
        p=np.column_stack([x.ravel(),y.ravel(),z.ravel()])
        faces=[]
        for row in range(64):
            for col in range(100):
                a=row*101+col
                faces.extend(((a,a+1,a+102),(a,a+102,a+101)))
        n=repair.skin_normal_field(p,np.asarray(faces)).reshape(65,101,3)
        cosine=(n[:,1:]*n[:,:-1]).sum(2)
        angle=np.degrees(np.arccos(np.clip(cosine,-1,1)))
        self.assertLess(float(angle.max()),4.,'A smooth surface must not gain an artificial shading crease')


@unittest.skipUnless(os.environ.get('MALON_REVIEW_FIXTURES'), 'Set MALON_REVIEW_FIXTURES for archive checks')
class IdleReferenceTests(unittest.TestCase):
    def test_idle_arms_and_hand_roll_follow_vanilla_with_other_channels_preserved(self):
        import refine_young_malon_face_idle as repair
        from native_npc_models import NativeRig, animation_frames, unit
        from verify_native_npc_models import Archive
        root=Path(os.environ['MALON_REVIEW_FIXTURES'])
        a=Archive(root/'native-reference/oot.o2r',root/'young-malon-poc8/Young_Malon_Native_NPC_POC8.o2r')
        out=repair.fit_idle(a)
        native=animation_frames(a.native.read(repair.IDLE))
        reference,custom=NativeRig(a,'young-malon'),NativeRig(a,'young-malon',True)
        arms={b+1 for side in ('left','right') for b in custom.profile[side][3:]}
        other=sorted(set(range(native.shape[1]))-arms)
        np.testing.assert_array_equal(out[:,other],native[:,other])
        for i in range(len(out)):
            reference.pose(native[i]);custom.pose(out[i])
            for side in ('left','right'):
                upper,fore,hand=custom.profile[side][3:]
                for b,c in ((upper,fore),(fore,hand)):
                    expected=unit(reference.limbs[c].T-reference.limbs[b].T)
                    actual=unit(custom.limbs[c].T-custom.limbs[b].T)
                    self.assertGreater(float(expected@actual),.999)
                np.testing.assert_allclose(custom.limbs[hand].M,reference.limbs[hand].M,atol=1e-4)

    @unittest.skipUnless(os.environ.get('MALON_REVIEW_CANDIDATE'), 'Set MALON_REVIEW_CANDIDATE for preservation checks')
    def test_candidate_preserves_body_skin_brooch_and_other_poses_byte_for_byte(self):
        import json
        import zipfile
        root=Path(os.environ['MALON_REVIEW_FIXTURES'])
        with zipfile.ZipFile(root/'young-malon-poc8/Young_Malon_Native_NPC_POC8.o2r') as old, zipfile.ZipFile(os.environ['MALON_REVIEW_CANDIDATE']) as new:
            info='young_malon_NativeNpcModelInfo.json'
            ns=json.loads(old.read(info))['namespace']
            allowed=lambda name: (name==info or name=='alt/'+ns+'/PoseAnimations/objects/object_ma1/gMalonChildIdleAnim' or
                                  name.startswith(('alt/'+ns+'/FaceParts/','alt/'+ns+'/Body/Limb14','alt/'+ns+'/Textures/Eyes')))
            protected=[name for name in old.namelist() if not allowed(name)]
            self.assertGreater(len(protected),250)
            self.assertTrue(any('/Skin/' in name for name in protected))
            self.assertTrue(any('gMalonChildRaiseHandsAnim' in name for name in protected))
            self.assertTrue(any('gMalonChildSingAnim' in name for name in protected))
            for name in protected:
                self.assertEqual(old.read(name),new.read(name),name)
            self.assertEqual(old.read('alt/'+ns+'/Textures/EyesClosed'),new.read('alt/'+ns+'/Textures/EyesClosed'))


if __name__=='__main__':
    unittest.main()
