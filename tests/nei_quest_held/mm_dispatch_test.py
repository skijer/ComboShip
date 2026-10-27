"""The real player's world draw callback must reach matching MM held equipment."""
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
class NativeWandDispatch(unittest.TestCase):
    def test_wand_draw_keeps_world_effects_and_draws_held(self):
        s=(ROOT/'mm/mods/items/logic/item_elemental_wand.c').read_text()
        start=s.index('void Wand_Draw(');body=s[start:s.index('\n}',start)+2]
        self.assertIn('CustomItems_DrawElementalWand(player, play);',body)
        for call in ['WandShadow_Draw(play);','WandStorm_Draw(play);','WandWind_Draw(player, play);']:
            self.assertIn(call,body)
            self.assertLess(body.index(call),body.index('CustomItems_DrawElementalWand'))
        self.assertIn('#include "../objects/object_elemental_wand.c"',s)
if __name__=='__main__':unittest.main()
