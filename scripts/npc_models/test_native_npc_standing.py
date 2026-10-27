"""Standing arm direction and protected native channels on the supplied rigs."""
from pathlib import Path
import unittest
import numpy as np
import native_npc_pose_fit as fit
from native_npc_models import NativeRig,animation_frames,unit
from verify_native_npc_models import Archive

class StandingTests(unittest.TestCase):
    def test_standing_forearms_and_fingers_point_down_without_moving_elbows(self):
        self.assertTrue(callable(getattr(fit,'fit_saria_standing_arms',None)), 'Standing arm fit is missing')
        root=Path(__file__).resolve().parents[3]
        native=root/'native-reference/oot.o2r'
        if not native.exists(): self.skipTest('Native/source archives unavailable')
        for label in ('Anime','Render'):
            archive=Archive(native,root/f'native-npc-poc9/Saria_{label}_Native_NPC_POC9.o2r')
            rig=NativeRig(archive,'saria',private=True)
            frames=animation_frames(archive.native.read('objects/object_sa/gSariaWaitArmsToSideAnim'))
            for original in frames:
                frame=original.copy();rig.pose(frame)
                elbows={s:rig.limbs[rig.profile[s][-2]].T.copy() for s in ('left','right')}
                fit.fit_saria_standing_arms(rig,frame)
                changed=np.flatnonzero(np.any(frame!=original,axis=1))
                self.assertTrue(set(changed).issubset({11,12,14,15}))
                for side in elbows:
                    fore,hand=rig.profile[side][-2:]
                    np.testing.assert_allclose(rig.limbs[fore].T,elbows[side],atol=1e-7)
                    self.assertGreater(np.dot(unit(rig.limbs[hand].T-rig.limbs[fore].T),[0,-1,0]),.9999)
                    self.assertGreater(np.dot(rig.limbs[hand].M[:,0],[0,-1,0]),.9999)
