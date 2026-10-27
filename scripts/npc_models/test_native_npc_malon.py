"""Anatomical arm and prop-contact regression checks for Malon's adapters."""
import os
from pathlib import Path
import unittest
import numpy as np

import native_npc_pose_fit as poses


class MalonElbowTests(unittest.TestCase):
    def test_child_idle_hangs_from_the_shoulder_with_downward_inward_facing_hands(self):
        from native_npc_malon import child_idle_pose
        shoulder=np.array([.3,2.,0.])
        cloth=np.array([[x,y,z] for x in [.25,.3,.35] for y in [.8,1.,1.2] for z in [-.05,.13]])
        hand=np.array([[0.,-.02,-.01],[.2,.02,.01]])
        wrist,matrix,pole=child_idle_pose(shoulder,1.,cloth,hand,np.array([0.,0.,1.]),1.)
        self.assertLess(abs(wrist[0]-shoulder[0]),.07)
        self.assertLess(wrist[1],shoulder[1]-.95)
        np.testing.assert_allclose(matrix[:,0],[0.,-1.,0.],atol=1e-12)
        np.testing.assert_allclose(matrix[:,2],[1.,0.,0.],atol=1e-12)
        self.assertAlmostEqual(np.linalg.norm(wrist-shoulder),.9995)
        np.testing.assert_allclose(matrix.T@matrix,np.eye(3),atol=1e-12)
        self.assertLess(abs(wrist[2]-shoulder[2]),.035)
        self.assertGreater((wrist+hand@matrix.T)[:,0].min(),.35)

    def test_basket_exposes_the_hand_backs_without_changing_finger_direction(self):
        from native_npc_malon import basket_hand_matrix
        from scipy.spatial.transform import Rotation
        for side,sign in [('left',1),('right',-1)]:
            original=Rotation.from_euler('xyz',[25,16,78],degrees=True).as_matrix()
            forward=np.array([0.,0.,1.])
            revised=basket_hand_matrix(original,forward,side)
            np.testing.assert_allclose(revised[:,0],original[:,0],atol=1e-12)
            self.assertGreater(np.dot(revised[:,2]*sign,forward),.95)
            np.testing.assert_allclose(revised.T@revised,np.eye(3),atol=1e-12)
            self.assertAlmostEqual(np.linalg.det(revised),1.)

    def test_sole_leveling_preserves_the_inward_toe_heading(self):
        from native_npc_malon import level_sole
        from scipy.spatial.transform import Rotation
        matrix=Rotation.from_euler('yxz',[-48,12,7],degrees=True).as_matrix()
        sole=np.array([[-1.,-.6,-1.],[-1.,-.6,2.],[1.,-.6,-1.],[1.,-.6,2.]])
        revised,height=level_sole(matrix,sole,np.array([0.,0.,1.]))
        p=sole@revised.T+[0.,height,0.]
        self.assertLess(np.ptp(p[:,1]),1e-10)
        self.assertAlmostEqual(p[:,1].min(),0.)
        before=matrix[:,2][[0,2]];after=revised[:,2][[0,2]]
        np.testing.assert_allclose(before/np.linalg.norm(before),after/np.linalg.norm(after),atol=1e-12)

    def test_cloth_clearance_stays_fixed_when_a_shorter_arm_cannot_reach_down(self):
        import importlib.util
        self.assertIsNotNone(importlib.util.find_spec('native_npc_malon'))
        from native_npc_malon import reachable_wrist
        shoulder=np.zeros(3);target=np.array([1.,-3.,.4])
        revised=reachable_wrist(shoulder,target,2.)
        np.testing.assert_allclose(revised[[0,2]],target[[0,2]])
        self.assertGreater(revised[1],target[1])
        self.assertLessEqual(np.linalg.norm(revised),2.)

    def test_explicit_bend_pole_controls_elbow_after_lateral_wrist_clearance(self):
        # Moving a wrist sideways must not reuse the old shoulder-to-elbow
        # vector as a new bend pole: it puts the elbow on the wrong side.
        self.assertIn('pole', __import__('inspect').signature(poses.elbow_point).parameters)
        shoulder=np.zeros(3); elbow=np.array([0.,-1.,-.1]); wrist=np.array([0.,-2.,0.])
        target=np.array([1.,-1.5,0.]);pole=np.array([0.,0.,-1.])
        revised,tip=poses.elbow_point(shoulder,elbow,wrist,target,pole=pole)
        self.assertLess(revised[2],-.2)
        self.assertAlmostEqual(np.linalg.norm(revised),np.linalg.norm(elbow))
        self.assertAlmostEqual(np.linalg.norm(tip-revised),np.linalg.norm(wrist-elbow))
        np.testing.assert_allclose(tip,target)


@unittest.skipUnless(os.environ.get('MALON_SOURCE_DIR'),'Set MALON_SOURCE_DIR for supplied-GLB checks')
class MalonFingerTests(unittest.TestCase):
    def test_child_idle_clearance_does_not_jump_between_nearby_dress_facets(self):
        import zipfile
        from native_npc_models import SourceModel,NativeRig,VARIANTS,fit_model
        from native_npc_malon import child_idle_profile,clear_arm_targets
        source=SourceModel(Path(os.environ['MALON_SOURCE_DIR'])/VARIANTS['young-malon']['file'])
        native=Path(os.environ.get('MALON_NATIVE_ARCHIVE','/workspace/scratch/7e8140561f6b/native-reference/oot.o2r'))
        if not native.is_file():self.skipTest('Native Malon skeleton fixture is unavailable')
        with zipfile.ZipFile(native) as z:
            rig=NativeRig(z,'young-malon')
            contact={side:dict(curls={'Index':.10,'Middle':.12,'Ring':.14,'Pinky':.16,'Thumb':.08}) for side in ('Left','Right')}
            model=fit_model(source,rig,hand_curls=.12,hand_grip=contact)
        bm=np.array([l.M.copy() for l in rig.limbs]);bt=np.array([l.T.copy() for l in rig.limbs])
        local={side:(model['positions'][model['bones']==rig.profile[side][-1]]-bt[rig.profile[side][-1]])@bm[rig.profile[side][-1]] for side in ('left','right')}
        profile=child_idle_profile(source,model,rig,rig.frames,bm,bt,local)
        offsets={side:[] for side in ('left','right')}
        for frame in rig.frames:
            rig.pose(frame)
            targets={side:rig.limbs[rig.profile[side][-1]].T.copy() for side in offsets}
            matrices={side:rig.limbs[rig.profile[side][-1]].M.copy() for side in offsets}
            clear_arm_targets(source,model,rig,rig,'gMalonChildIdleAnim',bm,bt,targets,matrices,local,profile)
            for side in offsets:
                arm,fore,hand=rig.profile[side][3:]
                s,e,w=[rig.limbs[i].T for i in (arm,fore,hand)]
                offsets[side].append(targets[side]-s)
        for side,points in offsets.items():
            step=np.linalg.norm(np.diff(points,axis=0),axis=1).max()
            self.assertLess(step,.005*model['scale'],side+' clearance flips between dress facets')

    def test_child_shoulder_reanchor_does_not_shorten_the_source_arm_chain(self):
        import zipfile
        from native_npc_models import SourceModel,NativeRig,VARIANTS,fit_model
        source=SourceModel(Path(os.environ['MALON_SOURCE_DIR'])/VARIANTS['young-malon']['file'])
        native=Path(os.environ.get('MALON_NATIVE_ARCHIVE','/workspace/scratch/7e8140561f6b/native-reference/oot.o2r'))
        if not native.is_file():self.skipTest('Native Malon skeleton fixture is unavailable')
        with zipfile.ZipFile(native) as z:
            rig=NativeRig(z,'young-malon');model=fit_model(source,rig)
        named={n:source.world[i][:3,3] for n,i in zip(source.names,source.joints)}
        for side in ['left','right']:
            a,e,w=[rig.limbs[i].T for i in rig.profile[side][3:]]
            actual=np.linalg.norm(e-a)+np.linalg.norm(w-e)
            original=sum(np.linalg.norm(named[side.title()+b]-named[side.title()+a]) for a,b in [('Arm','ForeArm'),('ForeArm','Hand')])*model['scale']
            self.assertLess(abs(actual-original),1.5,side+' shoulder repair shortened the arm')

    def test_carrying_fingers_curl_toward_the_palm_for_both_hands(self):
        from native_npc_models import SourceModel,VARIANTS,unit
        from native_npc_fit import source_hand_pose
        source=SourceModel(Path(os.environ['MALON_SOURCE_DIR'])/VARIANTS['malon']['file'])
        skin=source_hand_pose(source,.85)
        for side in ['Left','Right']:
            node=dict(zip(source.names,source.joints))
            wrist=source.world[node[side+'Hand']][:3,3]
            middle=source.world[node[side+'HandMiddle1']][:3,3]
            thumb=source.world[node[side+'HandThumb2']][:3,3]
            direction=unit(middle-wrist);lateral=unit(thumb-wrist-direction*np.dot(thumb-wrist,direction))
            palm=unit(np.cross(direction,lateral))*(1 if side=='Left' else -1)
            name=side+'HandMiddle3';joint=source.names.index(name)
            tip=source.world[node[side+'HandMiddle4']][:3,3]
            changed=skin[joint][:3,:3]@tip+skin[joint][:3,3]
            self.assertGreater(np.dot(changed-tip,palm),.015,side+' fingers bend away from the basket grip')


if __name__=='__main__':unittest.main()
