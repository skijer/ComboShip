"""Authored equipment must match native grip/axis frames, not GI display pose."""
import sys
import unittest
from pathlib import Path
import numpy as np
ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT/'tools/nei_held/SOURCE'), str(ROOT/'tools/nei_gi/SOURCE')]
from equipment import BUILDERS

class EquipmentFit(unittest.TestCase):
    def test_frames_and_envelopes(self):
        self.assertEqual(set(BUILDERS), {'divine_shield','sheikah_shield','shield_of_ikana',
            'cane_of_byrna','trident','four_sword_blade','four_sword_hilt','iron_knuckle_axe'})
        for slug, builder in BUILDERS.items():
            with self.subTest(slug=slug):
                m=builder(); p=np.concatenate([v['p'] for v in m.parts])*m.native_scale*16
                self.assertTrue(np.isfinite(p).all())
                self.assertEqual(m.entry, 'objects/nei_held_redesign/'+slug+'/gi_dl')
                self.assertFalse(m.markers.get('presentation_only',False))
                self.assertGreater(len(m.parts),0)
                if 'shield' in slug:
                    # New shell remains in the legacy custom shield plane (+Y up, +Z width).
                    self.assertLess(np.ptp(p,axis=0)[0],20)
                    self.assertGreater(np.ptp(p,axis=0)[1],60)
                    self.assertLess(np.ptp(p,axis=0)[1],85)
                elif slug=='cane_of_byrna':
                    self.assertAlmostEqual(p[:,1].min(),-416,delta=.6)
                    self.assertAlmostEqual(p[:,1].max(),223,delta=.6)
                elif slug=='trident':
                    self.assertAlmostEqual(p[:,2].max(),8520,delta=4)
                    self.assertEqual(m.markers['grip_native'],[0,0,2049.5])
                    self.assertLess(np.ptp(p,axis=0)[0],4000)
                elif slug=='four_sword_blade':
                    self.assertAlmostEqual(p[:,0].min(),719,delta=2)
                    self.assertAlmostEqual(p[:,0].max(),3389,delta=2)
                elif slug=='four_sword_hilt':
                    self.assertLess(p[:,0].min(),0)
                    self.assertLess(p[:,0].max(),1000)
                elif slug=='iron_knuckle_axe':
                    self.assertLess(p[:,2].min(),-5150)
                    self.assertGreater(p[:,2].max(),877)
                    self.assertAlmostEqual(m.markers['source_axis_native'][0],395)
    def test_four_sword_parts_are_complete_and_disjoint(self):
        b=BUILDERS['four_sword_blade'](); h=BUILDERS['four_sword_hilt']()
        self.assertFalse({p['name'] for p in b.parts}&{p['name'] for p in h.parts})
        self.assertGreater(sum(len(p['tri']) for p in b.parts),30)
        self.assertGreater(sum(len(p['tri']) for p in h.parts),100)

if __name__=='__main__':unittest.main()
