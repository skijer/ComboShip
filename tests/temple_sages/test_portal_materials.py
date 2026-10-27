"""POC3 portal ownership, actual native sampler and membrane regressions."""
import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
import zipfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/temple_sages'))
import resource_geometry as G
NAMES = ('forest', 'fire', 'water', 'spirit', 'shadow')
PREFIX = 'custom/temple_sages/portals/'


class PortalMaterials(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path = ROOT / 'tools/temple_sages/portals.py'
        if path.exists():
            spec = importlib.util.spec_from_file_location('portal_materials', path)
            cls.module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(cls.module)

    def data(self):
        self.assertTrue(hasattr(self, 'module'), 'five independently bound scrolling portal resources are missing')
        return self.module.resources()

    def test_five_complete_independent_bindings_and_alt(self):
        data = self.data()
        payloads = []
        for name in NAMES:
            path = PREFIX + name + '/'
            effect = ET.fromstring(data[path + 'effect'])
            self.assertEqual([e.get('Path') for e in effect if e.tag == 'CallDisplayList'],
                             [path + 'material', path + 'geometry'])
            material = ET.fromstring(data[path + 'material'])
            self.assertEqual([e.get('Path') for e in material if e.tag == 'SetTextureImage'],
                             [path + 'texture_flow', path + 'texture_detail'])
            for suffix in ('effect', 'material', 'texture_flow', 'texture_detail'):
                self.assertEqual(data[path + suffix], data['alt/' + path + suffix])
            payloads.extend(data[path + x] for x in ('texture_flow', 'texture_detail'))
        self.assertTrue(all(p == payloads[0] for p in payloads), 'all defaults use identical initial art')
        edited = dict(data)
        edited[PREFIX + 'forest/texture_flow'] = payloads[0][:-1] + b'\0'
        for name in NAMES[1:]:
            self.assertEqual(edited[PREFIX + name + '/texture_flow'], payloads[0])
        self.assertNotEqual(edited[PREFIX + 'forest/texture_flow'], data[PREFIX + 'forest/texture_flow'])

    def test_raw_art_is_high_resolution_strict_gray_with_native_logical_32(self):
        payload = self.data()[PREFIX + 'forest/texture_flow']
        self.assertEqual(payload[4:8], b'XETO')
        self.assertEqual(struct.unpack_from('<I', payload, 8)[0], 1)
        kind, w, h, flags, sx, sy, count = struct.unpack_from('<IIIIffI', payload, 64)
        self.assertEqual((kind, w, h, flags, sx, sy, count), (1, 512, 512, 1, 32., 16., 512*512*4))
        # Mirror ImportTextureRaw's direct-upload branch for a logical RGBA16
        # tile: native line bytes * HByteScale == RGBA8 upload row bytes, and
        # loaded logical rows * VPixelScale == source height.
        native_line_bytes, native_loaded_bytes = 8*8, 32*32*2
        self.assertEqual(native_line_bytes*sx, 4*w)
        self.assertEqual(native_loaded_bytes/native_line_bytes*sy, h)
        native_types = (ROOT/'libultraship/include/fast/resource/type/Texture.h').read_text()
        self.assertIn('RGBA32bpp = 1', native_types)
        pixels = payload[92:]
        self.assertEqual(len(pixels), count)
        self.assertEqual(pixels[0::4], pixels[1::4])
        self.assertEqual(pixels[1::4], pixels[2::4])
        self.assertGreater(len(set(pixels[0::4])), 64)
        self.assertEqual(set(pixels[3::4]), {255}, 'default art uses vertex/native opacity without squaring luminance')

    def test_actual_two_layer_pipeline_sampler_and_dependency_closure(self):
        data = self.data()
        factory = (ROOT/'libultraship/src/fast/resource/factory/DisplayListFactory.cpp').read_text()
        for name in NAMES:
            path = PREFIX + name + '/'
            tree = ET.fromstring(data[path + 'material'])
            self.assertIsNone(tree.find('SetPrimColor'), 'native caller owns RGB/opacity')
            self.assertEqual(tree.find('Texture').get('On'), '1')
            self.assertTrue(any(e.get('G_CYC_2CYCLE') == '1' for e in tree))
            self.assertTrue(any(e.get('G_RM_AA_ZB_XLU_SURF2') == '1' for e in tree))
            tiles = [e for e in tree if e.tag == 'SetTile' and e.get('Tile') in ('0', '1')]
            self.assertEqual([(e.get('Tile'), e.get('TMem'), e.get('Line'), e.get('ShiftS')) for e in tiles],
                             [('0', '0', '8', '0'), ('1', '256', '8', '15')])
            self.assertEqual([(e.get('Tile'), e.get('SpeedX'), e.get('SpeedY')) for e in tree if e.tag == 'ScrollTexture'],
                             [('0', '-1', '2'), ('1', '2', '-3')])
            self.assertEqual(len(tree.findall('LoadBlock')), 2)
            combine = tree.find('SetCombineLERP').attrib
            self.assertIn('G_CCMUX_TEXEL1', combine.values())
            self.assertIn('G_CCMUX_TEXEL0', combine.values())
            self.assertEqual(combine['Ac1'], 'G_ACMUX_PRIMITIVE')
            self.assertEqual(combine['Ac0'], 'G_ACMUX_SHADE')
            for resource in (path + 'effect', path + 'material', path + 'geometry'):
                for e in ET.fromstring(data[resource]):
                    self.assertIn('childName == "' + e.tag + '"', factory, e.tag)
                    if e.get('Path'):
                        self.assertIn(e.get('Path'), data)
                        self.assertFalse(e.get('Path').startswith('>'))

    def test_membrane_spans_vertical_interior_with_real_uvs_and_edge_fade(self):
        data = self.data()
        rows = G.vertex_rows(data[PREFIX + 'forest/geometry_vtx'])
        self.assertGreater(max(v[1] for v in rows), 140)
        self.assertEqual(min(v[1] for v in rows), 0)
        face_rows = [v for v in rows if v[2] == 0 and 16 <= v[1] <= 136]
        center = [v for v in face_rows if v[0] == 0 and v[1] == 76]
        self.assertTrue(center)
        self.assertGreaterEqual(max(v[9] for v in center), 220)
        self.assertGreater(len({(v[4], v[5]) for v in face_rows}), 40)
        self.assertGreater(max(v[5] for v in face_rows) - min(v[5] for v in face_rows), 900)
        self.assertTrue(any(v[9] == 0 for v in face_rows))


    def test_preview_samples_actual_layers_and_scroll_without_opacity_animation(self):
        import numpy as np
        from PIL import Image
        import preview_portals as preview
        data = self.data()
        with tempfile.TemporaryDirectory(prefix='portal-preview-') as tmp:
            path = Path(tmp) / 'source.o2r'
            with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as archive:
                for name, raw in data.items():
                    if name.startswith(PREFIX + 'forest/'):
                        archive.writestr(name, raw)
            model = preview.mesh(path)
            self.assertEqual(len(model['layers']), 2)
            np.testing.assert_array_equal(model['layers'][0]['speed'], (-1, 2))
            np.testing.assert_array_equal(model['layers'][1]['speed'], (2, -3))
            uv = np.array([[100., 200.], [500., 600.]])
            np.testing.assert_allclose(preview.sample(model['layers'][0], uv, 128),
                                       preview.sample(model['layers'][0], uv, 0))
            self.assertFalse(np.allclose(preview.sample(model['layers'][0], uv, 0),
                                        preview.sample(model['layers'][1], uv, 0)))
            frames = []
            for tick in (0, 20):
                image = Image.new('RGB', (320, 420), preview.BACKGROUND)
                preview.portal(image, model, (105, 255, 145), 155, (160, 210), 2.4, tick)
                frames.append(np.asarray(image, dtype=int))
            delta = np.abs(frames[0] - frames[1])
            self.assertGreater(np.count_nonzero(delta), 1000, 'actual textures must move even with fixed opacity')
            self.assertGreater(delta.max(), 10)
            # A later archive can replace just flow's alpha while retaining RGB.
            # The real sampler and the source combiner must still honor it.
            flow_path = PREFIX + 'forest/texture_flow'
            custom = bytearray(data[flow_path])
            custom[95::4] = bytes(len(custom[95::4]))
            with zipfile.ZipFile(path, 'w', zipfile.ZIP_DEFLATED) as archive:
                for name, raw in data.items():
                    if name.startswith(PREFIX + 'forest/'):
                        archive.writestr(name, bytes(custom) if name == flow_path else raw)
            override = preview.mesh(path)
            np.testing.assert_array_equal(preview.sample(override['layers'][0], uv, 20)[:, 3], 0)
            np.testing.assert_array_equal(preview.sample(override['layers'][0], uv, 20)[:, :3],
                                          preview.sample(model['layers'][0], uv, 20)[:, :3])
            image = Image.new('RGB', (320, 420), preview.BACKGROUND)
            preview.portal(image, override, (105, 255, 145), 155, (160, 210), 2.4, 20)
            np.testing.assert_array_equal(np.asarray(image),
                np.broadcast_to(np.array(preview.BACKGROUND, dtype=np.uint8), (420, 320, 3)))


if __name__ == '__main__':
    unittest.main()
