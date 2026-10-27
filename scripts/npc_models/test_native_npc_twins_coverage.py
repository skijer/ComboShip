"""Every animation must inherit the reviewed joint surface without replacing it."""
import importlib
import importlib.util
import struct
import unittest
from native_npc_models import asset

class SharedShoulderTests(unittest.TestCase):
    def test_default_skin_calls_reviewed_idle_geometry_and_preserves_inputs(self):
        self.assertIsNotNone(importlib.util.find_spec('expand_twins_pose_coverage'),
                             'Twins shoulder repair still covers only three named animations')
        coverage=importlib.import_module('expand_twins_pose_coverage')
        ns='objects/object_kw1/NativeNpcV1';idle=ns+'/PoseAnimations/objects/object_os_anime/gKokiriIdleAnim'
        records={'accepted-face':b'face','accepted-palette':b'palette'}
        for host in (7,8,11):
            records['alt/'+idle+f'/Skin/Limbs/Limb{host:02d}']=b'accepted shoulder '+bytes([host])
            records['alt/'+ns+f'/Skin/Limbs/Limb{host:02d}']=b'old protruding cap'
        before=records.copy();changed=coverage.shared_shoulders(records,ns)
        self.assertEqual(records,before,'The accepted input must stay immutable')
        self.assertEqual(set(changed),{'alt/'+ns+f'/Skin/Limbs/Limb{i:02d}' for i in (7,8,11)})
        for host in (7,8,11):
            target=idle+f'/Skin/Limbs/Limb{host:02d}';hashed=asset.crc64(target)
            self.assertIn(struct.pack('<II',hashed>>32,hashed&0xffffffff),changed['alt/'+ns+f'/Skin/Limbs/Limb{host:02d}'])
        self.assertEqual(coverage.shared_shoulders(dict(records,**changed),ns),changed)

if __name__=='__main__':unittest.main()
