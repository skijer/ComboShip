"""Matching quest equipment resources, canonical axes, native envelope and parity."""
import importlib.util
from pathlib import Path
import sys
import unittest
import numpy as np

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/nei_gi/SOURCE'))

class QuestHeldGeometry(unittest.TestCase):
    def test_matching_canonical_grips_and_resource_parity(self):
        path=ROOT/'tools/nei_held/SOURCE/quest_held.py'
        self.assertTrue(path.exists(),'Quest GIs have no matching held builders')
        spec=importlib.util.spec_from_file_location('quest_held',path)
        source=importlib.util.module_from_spec(spec);spec.loader.exec_module(source)
        for slug,builder in source.BUILDERS.items():
            with self.subTest(slug=slug):
                m=builder();p=np.concatenate([part['p'] for part in m.parts])*m.native_scale*16
                grip=m.markers['grip_native']
                if slug in source.source.WANDS:
                    np.testing.assert_allclose(grip,[0,-45.5,0],atol=.001)
                    np.testing.assert_allclose(p[:,1].min(),-175,atol=.01)
                    np.testing.assert_allclose(p[:,1].max(),175,atol=.01)
                    self.assertLessEqual(np.abs(p[:,0]).max(),36.01)
                    self.assertLessEqual(np.abs(p[:,2]).max(),24.01)
                    self.assertEqual(m.markers['shaft_axis_native'],[0,1,0])
                else:
                    np.testing.assert_allclose(grip,[0,45,0],atol=.001)
                    np.testing.assert_allclose(p[:,1].min(),-48,atol=.01)
                    np.testing.assert_allclose(p[:,1].max(),48,atol=.01)
                for part in m.parts:
                    self.assertEqual(m.materials[part['mat']]['alpha'],1)
                resource=ROOT/'soh/assets/custom'/m.prefix
                self.assertTrue((resource/'gi_dl').exists())
                self.assertFalse((resource/'gi_xlu_dl').exists())
                mm=ROOT/'mm/assets/custom'/m.prefix
                self.assertEqual({x.name:x.read_bytes() for x in resource.iterdir() if x.is_file()},
                                 {x.name:x.read_bytes() for x in mm.iterdir() if x.is_file()})
                self.assertTrue((ROOT/'tools/nei_held/CHECKPOINTS'/slug/'checkpoint.json').exists())

    def test_serialized_triangles_equal_review_meshes(self):
        spec=importlib.util.spec_from_file_location('verifier',ROOT/'tools/nei_gi/verify_assets.py')
        verifier=importlib.util.module_from_spec(spec);spec.loader.exec_module(verifier)
        slugs=['elemental_wand','sand_rod','tornado_rod','water_rod','meteor_rod','storm_rod','shadow_scepter','sheikah_slate']
        self.assertEqual(len(verifier.verify(slugs,'objects/nei_held_redesign/',ROOT/'tools/nei_held/CHECKPOINTS')),8)

if __name__=='__main__':unittest.main()
