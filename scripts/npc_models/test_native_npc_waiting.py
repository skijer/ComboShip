from pathlib import Path
from types import SimpleNamespace
import json,unittest
import numpy as np
from scipy.spatial.transform import Rotation
import native_npc_pose_fit as fit
from verify_native_npc_models import Archive
from native_npc_models import NativeRig,animation_frames

class WaitingTests(unittest.TestCase):
    def test_waiting_adjustment_preserves_right_arm_and_attached_left_bone_lengths(self):
        self.assertTrue(callable(getattr(fit,'apply_waiting_hand_fit',None)),'Waiting hand fit is missing')
        root=Path(__file__).resolve().parents[3]
        p=root/'native-npc-poc11/Saria_Anime_Native_NPC_POC11.o2r'
        if not p.exists():self.skipTest('Review candidate unavailable')
        a=Archive(root/'native-reference/oot.o2r',p);r=NativeRig(a,'saria',private=True)
        spec=json.loads(Path(__file__).with_name('pose_art').joinpath('saria-anime-waiting.json').read_text())
        frames=animation_frames(a.read(r.ns+'/PoseAnimations/objects/object_sa/gSariaHandsBehindBackWaitAnim'))
        for original in frames:
            frame=original.copy();r.pose(frame);body=fit.body_frame(r)[0]
            target=r.limbs[11].T+body@np.array(spec['body_offset'])
            matrix=r.limbs[11].M@Rotation.from_euler('xyz',spec['local_rotation_degrees'],degrees=True).as_matrix()
            lengths=[np.linalg.norm(r.limbs[b].T-r.limbs[c].T) for b,c in [(9,10),(10,11)]]
            fit.apply_waiting_hand_fit(r,frame,spec)
            changed=np.flatnonzero(np.any(frame!=original,axis=1))
            self.assertTrue(set(changed).issubset({10,11,12}))
            self.assertLess(np.linalg.norm(r.limbs[11].T-target),.1)
            np.testing.assert_allclose(r.limbs[11].M,matrix,atol=.0002)
            np.testing.assert_allclose([np.linalg.norm(r.limbs[b].T-r.limbs[c].T) for b,c in [(9,10),(10,11)]],lengths,atol=1e-6)

    def test_traditional_saria_has_no_waiting_correction_and_wrong_source_is_rejected(self):
        self.assertTrue(callable(getattr(fit,'waiting_hand_profile',None)),'Source-gated waiting profile is missing')
        source=SimpleNamespace(raw=b'wrong source')
        self.assertIsNone(fit.waiting_hand_profile(source,'saria-render'))
        with self.assertRaisesRegex(ValueError,'source'):
            fit.waiting_hand_profile(source,'saria-anime')

    def test_elbow_clearance_moves_the_arm_route_without_moving_the_seated_wrist(self):
        self.assertTrue(callable(getattr(fit,'apply_waiting_hand_fit',None)))
        root=Path(__file__).resolve().parents[3]
        p=root/'native-npc-poc11/Saria_Anime_Native_NPC_POC11.o2r'
        if not p.exists():self.skipTest('Review candidate unavailable')
        a=Archive(root/'native-reference/oot.o2r',p);r=NativeRig(a,'saria',private=True)
        original=animation_frames(a.read(r.ns+'/PoseAnimations/objects/object_sa/gSariaHandsBehindBackWaitAnim'))[12]
        spec=json.loads(Path(__file__).with_name('pose_art').joinpath('saria-anime-waiting.json').read_text())
        base=original.copy();fit.apply_waiting_hand_fit(r,base,dict(spec,elbow_swivel_degrees=0))
        elbow=r.limbs[10].T.copy();wrist=r.limbs[11].T.copy()
        alternate=original.copy();fit.apply_waiting_hand_fit(r,alternate,dict(spec,elbow_swivel_degrees=-110))
        self.assertGreater(np.linalg.norm(r.limbs[10].T-elbow),100)
        self.assertLess(np.linalg.norm(r.limbs[11].T-wrist),.1)
