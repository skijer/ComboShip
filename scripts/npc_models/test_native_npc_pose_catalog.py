"""Review every cutscene entry and animations outside the old three-frame sheet."""
import unittest
import preview_native_npc_models as preview

class PoseCatalogTests(unittest.TestCase):
    def test_all_six_kokiri_cutscene_entries_have_their_actual_animation(self):
        self.assertTrue(callable(getattr(preview,'animation_for_mode',None)),
                        'The preview still exposes only the three old Twins poses')
        expected=['gKokiriIdleAnim','gKokiriStandingArmsBehindBackAnim','gKokiriStandingHandsOnHipsAnim',
                  'gKokiriSittingHeadOnHandAnim','gKokiriSittingCrossedLegsAnim','gKokiriSittingCrossedArmsLegsAnim']
        for i,name in enumerate(expected):
            self.assertEqual(preview.animation_for_mode('twins','object_os_anime',f'static{i}'),
                             ('objects/object_os_anime/'+name,i,i))
        self.assertEqual(preview.animation_for_mode('twins','object_os_anime','pose2')[0],
                         'objects/object_os_anime/gKokiriSittingCrossedArmsLegsAnim')
        self.assertEqual(preview.animation_for_mode('twins','object_os_anime','pose0',
                         '__OTR__objects/object_ec/gDemoEcDancingKokiriAnim')[0],
                         'objects/object_ec/gDemoEcDancingKokiriAnim')

if __name__=='__main__':unittest.main()
