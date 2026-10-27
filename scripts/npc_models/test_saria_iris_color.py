"""Saria's palette pass may alter iris hue without changing any drawn detail."""
import colorsys
import json
import os
import unittest
import zipfile

import numpy as np

from recolor_saria_iris import recolor_pixels, select_iris_mask, map_projection_mask


class SariaIrisColorTests(unittest.TestCase):
    def setUp(self):
        self.image = np.array([[[20, 180, 55, 255], [10, 80, 25, 255], [3, 15, 8, 255]],
                               [[250, 250, 250, 255], [225, 180, 120, 255], [20, 170, 45, 255]]], np.uint8)
        self.iris = np.array([[True, True, True], [True, False, False]])
        self.hue = 230/360

    def test_green_iris_reaches_native_blue_hue_without_losing_shading(self):
        fixed, changed = recolor_pixels(self.image, self.iris, self.hue)
        self.assertEqual(int(changed.sum()), 2)
        for row, column in ((0, 0), (0, 1)):
            old = colorsys.rgb_to_hsv(*(self.image[row, column, :3]/255))
            new = colorsys.rgb_to_hsv(*(fixed[row, column, :3]/255))
            self.assertAlmostEqual(new[0], self.hue, delta=.004)
            self.assertAlmostEqual(new[1], old[1], delta=.008)
            self.assertEqual(max(fixed[row, column, :3]), max(self.image[row, column, :3]))

    def test_pupil_catchlight_skin_hair_and_alpha_remain_exact(self):
        fixed, changed = recolor_pixels(self.image, self.iris, self.hue)
        np.testing.assert_array_equal(fixed[~changed], self.image[~changed])
        np.testing.assert_array_equal(fixed[0, 2], self.image[0, 2])
        np.testing.assert_array_equal(fixed[1], self.image[1])
        np.testing.assert_array_equal(fixed[..., 3], self.image[..., 3])

    def test_existing_blue_iris_is_byte_identical(self):
        blue = self.image.copy()
        blue[0, :2, :3] = [[20, 55, 180], [10, 25, 80]]
        fixed, changed = recolor_pixels(blue, self.iris, self.hue)
        np.testing.assert_array_equal(fixed, blue)
        self.assertFalse(changed.any())


class IrisOwnershipTests(unittest.TestCase):
    def test_projection_mask_excludes_green_hair_and_closed_lids(self):
        from native_npc_face import face_bounds
        bounds = face_bounds('saria-anime')
        yy, xx = np.indices((512, 512))
        sx = bounds[0]+xx/511*(bounds[2]-bounds[0])
        sy = bounds[3]-yy/511*(bounds[3]-bounds[1])
        image = np.full((512, 512, 4), [225, 180, 120, 255], np.uint8)
        expected = np.zeros((512, 512), bool)
        for center in (-.031, .031):
            iris = ((sx-center)/.007)**2+((sy-.847)/.010)**2 < 1
            image[iris] = [20, 180, 55, 255]
            expected |= iris
        hair = (sy > .866) & (abs(sx-.031) < .024)
        image[hair] = [20, 180, 55, 255]
        mask, records = select_iris_mask(image, 'saria-anime', bounds)
        self.assertEqual(len(records), 2)
        self.assertFalse((mask & hair).any())
        np.testing.assert_array_equal(mask, expected)
        image[expected] = [225, 180, 120, 255]
        closed, records = select_iris_mask(image, 'saria-anime', bounds)
        self.assertFalse(closed.any())
        self.assertEqual(records, [])


class AtlasOwnershipTests(unittest.TestCase):
    def test_projection_to_uv_keeps_the_top_left_quadrant_in_place(self):
        positions = np.array([[0., 1., 1.], [1., 1., 1.], [1., 0., 1.], [0., 0., 1.]])
        uv = np.array([[0., 0.], [1., 0.], [1., 1.], [0., 1.]])
        faces = np.array([[0, 1, 2], [0, 2, 3]])
        mask = np.zeros((8, 8), bool)
        mask[:4, :4] = True
        mapped = map_projection_mask(positions, uv, faces, mask, [0., 0., 1., 1.], (8, 8))
        np.testing.assert_array_equal(mapped, mask)


class ExportedSariaIrisTests(unittest.TestCase):
    @unittest.skipUnless(os.environ.get('SARIA_IRIS_ARCHIVE'), 'Set SARIA_IRIS_ARCHIVE for exported color proof')
    def test_visible_projected_irises_have_no_green_core(self):
        from native_assets import o2r as asset
        from native_npc_face import face_bounds
        with zipfile.ZipFile(os.environ['SARIA_IRIS_ARCHIVE']) as archive:
            manifest = next(name for name in archive.namelist() if name.endswith('_NativeNpcModelInfo.json'))
            metadata = json.loads(archive.read(manifest))
            for name in archive.namelist():
                if not (name.endswith('/Textures/EyesHalf') or name.endswith('/Textures/EyesNeutral')):
                    continue
                with self.subTest(resource=name):
                    _, w, h, raw = asset.read_texture(archive.read(name))
                    image = np.frombuffer(raw, np.uint8).reshape(h, w, 4)
                    mask, owners = select_iris_mask(image, metadata['variant'], face_bounds(metadata['variant']))
                    self.assertEqual(len(owners), 2)
                    self.assertEqual(int(recolor_pixels(image, mask, 230/360)[1].sum()), 0,
                                     'A visible authored iris still contains its green core')


if __name__ == '__main__':
    unittest.main()
