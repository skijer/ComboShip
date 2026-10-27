"""Verify the standalone Temple magic resource graph without donor archives."""
import hashlib
import importlib.util
from pathlib import Path
import struct
import unittest
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[2]
NAMESPACE = 'custom/temple_sages/'
THEMES = {
    'forest': (105, 255, 145), 'fire': (255, 105, 40),
    'water': (90, 180, 255), 'spirit': (255, 195, 85),
    'shadow': (190, 105, 255), 'light': (255, 242, 180),
}


class TempleMaterials(unittest.TestCase):
    def setUp(self):
        path = ROOT / 'tools/temple_sages/materials.py'
        self.assertTrue(path.exists(), 'standalone Temple magic materials are missing')
        spec = importlib.util.spec_from_file_location('temple_materials', path)
        self.module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.module)
        self.resources = self.module.bridge_resources()

    def materials(self):
        return {name: ET.fromstring(data) for name, data in self.resources.items()
                if data.lstrip().startswith(b'<')}

    def test_canonical_texture_is_indexed_before_alt_resolution(self):
        # POC1 emitted only the Alt texture; native hash lookup then kept stone.
        path = NAMESPACE + 'texture/arcane_stone'
        self.assertIn(path, self.resources)
        self.assertEqual(self.resources[path], self.resources['alt/' + path])
        for name, tree in self.materials().items():
            references = [node.get('Path') for node in tree if node.get('Path')]
            self.assertEqual(references, [path], name)
            self.assertTrue(all(reference in self.resources for reference in references))

    def test_exact_black_market_pixels_keep_correct_raw_upload_contract(self):
        data = self.resources[NAMESPACE + 'texture/arcane_stone']
        self.assertEqual(data[4:8], b'XETO')
        self.assertEqual(struct.unpack_from('<I', data, 8)[0], 1)
        self.assertEqual(struct.unpack_from('<IIIIffI', data, 64),
                         (1, 512, 512, 1, 16.0, 8.0, 1048576))
        self.assertEqual(hashlib.sha256(data[92:]).hexdigest(),
                         '3e2bac601d1a9c3e0317d159aa6b9f5354abaac982a24848edfec5e97dbb3231')
        self.assertEqual(set(data[95::4]), {255})

    def test_materials_own_the_pipeline_after_arbitrary_previous_draw(self):
        for name, tree in self.materials().items():
            self.assertEqual(tree.tag, 'DisplayList')
            self.assertEqual(tree[0].tag, 'PipeSync')
            clear = tree.find('ClearGeometryMode')
            for flag in ('G_LIGHTING', 'G_FOG', 'G_CULL_BOTH',
                         'G_TEXTURE_GEN', 'G_TEXTURE_GEN_LINEAR'):
                self.assertEqual(clear.get(flag), '1', (name, flag))
            modes = {node.get('Cmd'): node for node in tree.findall('SetOtherMode')}
            high, low = modes['G_SETOTHERMODE_H'], modes['G_SETOTHERMODE_L']
            self.assertEqual((high.get('Sft'), high.get('Length')), ('4', '20'))
            for flag in ('G_CYC_2CYCLE', 'G_TL_TILE', 'G_TF_BILERP', 'G_TP_PERSP'):
                self.assertEqual(high.get(flag), '1', (name, flag))
            self.assertEqual((low.get('Sft'), low.get('Length')), ('0', '32'))
            for flag in ('G_RM_PASS', 'G_RM_AA_ZB_OPA_SURF2', 'G_AC_NONE', 'G_ZS_PIXEL'):
                self.assertEqual(low.get(flag), '1', (name, flag))
            self.assertEqual(tree.find('SetTextureLUT').get('Mode'), 'G_TT_NONE')
            self.assertEqual(tree.find('Texture').attrib,
                             dict(S='65535', T='65535', Level='0', Tile='0', On='1'))
            self.assertIsNone(tree.find('CallDisplayList'))
            self.assertEqual(tree[-1].tag, 'EndDisplayList')

    def test_six_theme_materials_multiply_texture_shade_and_owned_tint(self):
        for theme, color in THEMES.items():
            name = NAMESPACE + 'bridge_material_' + theme
            for prefix in ('', 'alt/'):
                tree = ET.fromstring(self.resources[prefix + name])
                primitive = tree.find('SetPrimColor')
                self.assertEqual(tuple(int(primitive.get(c)) for c in 'RGBA'), (*color, 255))
                combine = tree.find('SetCombineLERP')
                # Stage zero keeps side-face vertex shading; stage one applies
                # the material's tint. Alpha is opaque in both stages.
                self.assertEqual([combine.get(key) for key in ('A0','B0','C0','D0')],
                                 ['G_CCMUX_TEXEL0','G_CCMUX_0','G_CCMUX_SHADE','G_CCMUX_0'])
                self.assertEqual([combine.get(key) for key in ('A1','B1','C1','D1')],
                                 ['G_CCMUX_COMBINED','G_CCMUX_0','G_CCMUX_PRIMITIVE','G_CCMUX_0'])
                self.assertEqual(combine.get('Ad0'), 'G_ACMUX_1')
                self.assertEqual(combine.get('Ad1'), 'G_ACMUX_1')
                scroll = tree.find('ScrollTexture')
                self.assertIsNotNone(scroll)
                self.assertEqual(scroll.get('Tile'), '0')
                self.assertNotEqual((int(scroll.get('SpeedX')), int(scroll.get('SpeedY'))), (0, 0))

    def test_logical_sampler_matches_proven_black_market_flow(self):
        for name, tree in self.materials().items():
            texture = tree.find('LoadTextureBlock')
            self.assertIsNotNone(texture, name)
            for key, value in dict(Format='0', Size='2', Width='64', Height='64',
                                   MaskS='6', MaskT='6', ShiftS='0', ShiftT='0',
                                   CMS_TXWrap='1', CMT_TXWrap='1').items():
                self.assertEqual(texture.get(key), value, (name, key))
            self.assertLess(list(tree).index(texture), list(tree).index(tree.find('ScrollTexture')))

    def test_shell_is_dark_opaque_and_namespace_is_private(self):
        shell = ET.fromstring(self.resources[NAMESPACE + 'shell_material'])
        color = tuple(int(shell.find('SetPrimColor').get(c)) for c in 'RGBA')
        self.assertLessEqual(max(color[:3]), 42)
        self.assertGreater(min(color[:3]), 0)
        self.assertEqual(color[3], 255)
        other = self.module.bridge_resources('custom/temple_candidate/')
        self.assertEqual(len(other), 16)
        for name, data in other.items():
            self.assertTrue(name.startswith(('custom/temple_candidate/', 'alt/custom/temple_candidate/')))
            if data.lstrip().startswith(b'<'):
                for node in ET.fromstring(data):
                    if node.get('Path'):
                        self.assertIn(node.get('Path'), other)
        self.assertEqual(self.resources, self.module.bridge_resources())


if __name__ == '__main__':
    unittest.main()
