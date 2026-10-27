"""Blouse art safety regressions: connected fabric, strict UV ownership, alpha."""
from pathlib import Path
import unittest
import os
import tempfile

import numpy as np

MODULE = Path(__file__).with_name('repair_malon_blouse_art.py')
if MODULE.exists():
    import repair_malon_blouse_art as repair
else:
    repair = None


class BlouseMaskTests(unittest.TestCase):
    def test_native_linear_filter_footprint_protects_the_real_arm_witness(self):
        uv = np.array([[2883,654],[2954,595],[2900,577]], float)/16384
        protected = repair.native_protection_mask(uv, np.array([[0,1,2]]), (4096,4096))
        self.assertTrue(protected[166,725], 'Native linear filtering reaches this non-chest texel')

    def setUp(self):
        self.assertIsNotNone(repair, 'The archive-only blouse repair is not implemented')

    def target(self):
        size = 128
        image = np.full((size, size, 4), [155, 80, 20, 255], np.uint8)
        # Registered world coordinates, same bounded chest frame as the parent.
        yy, xx = np.indices((size, size))
        x = -950+xx/(size-1)*1600; y = 5100-yy/(size-1)*1600
        shirt = (x > -630) & (x < 220) & (y > 3840) & (y < 4450)
        image[shirt] = [232, 231, 211, 255]
        gray = shirt & (x > -530) & (x < -370) & (y > 4050) & (y < 4200)
        image[gray] = [112, 113, 98, 255]
        yellow = shirt & (abs(x+200) < 100) & (y > 4140)
        image[yellow] = [220, 180, 15, 255]
        blue = shirt & (x < -580) & (y > 4250)
        image[blue] = [25, 39, 100, 255]
        skin = shirt & (x > 120) & (y > 4350)
        image[skin] = [244, 184, 126, 255]
        owners = np.zeros((size, size), int)
        return image, owners, gray, yellow | blue | skin

    def test_gray_fold_is_editable_but_colored_details_are_protected(self):
        image, owners, gray, colored = self.target()
        weight = repair.semantic_blouse_mask(image, owners, np.array([10]), repair.BOUNDS)
        self.assertGreater(np.count_nonzero(weight[gray] > .5), 40)
        self.assertFalse(np.any(weight[colored]))
        self.assertTrue(np.any((weight > 0) & (weight < 1)), 'Fabric boundary must blend')

    def test_disconnected_white_and_other_limb_pixels_are_protected(self):
        image, owners, _, _ = self.target()
        image[72:78, 8:14] = [230, 230, 214, 255]
        owners[85:92, 70:76] = 1
        weight = repair.semantic_blouse_mask(image, owners, np.array([10, 11]), repair.BOUNDS)
        self.assertFalse(np.any(weight[72:78, 8:14]))
        self.assertFalse(np.any(weight[85:92, 70:76]))

    def test_shared_non_blouse_uv_pixels_cannot_change(self):
        # A white front quad shares its UVs with a back/other-limb quad. A
        # purely spatial paint mask would accidentally repaint both surfaces.
        p = np.array([[-620, 3860, 100], [200, 3860, 100],
                      [200, 4440, 100], [-620, 4440, 100]], float)
        uv = np.array([[.05, .95], [.95, .95], [.95, .05], [.05, .05]])
        points = np.concatenate([p, p-[0, 0, 50]])
        tex = np.full((128, 128, 4), [225, 224, 205, 173], np.uint8)
        art = np.full((128, 128, 4), [243, 242, 223, 255], np.uint8)
        faces = np.array([[0, 1, 2], [0, 2, 3], [4, 5, 6], [4, 6, 7]])
        revised, allowed, _ = repair.transfer_blouse_art(points, np.tile(uv, (2, 1)), tex,
            faces, np.array([10, 10, 11, 11]), art, repair.BOUNDS)
        np.testing.assert_array_equal(revised, tex)
        self.assertFalse(allowed.any())

    def test_alpha_and_pixels_outside_owned_fabric_are_exact(self):
        p = np.array([[-620, 3860, 100], [200, 3860, 100],
                      [200, 4440, 100], [-620, 4440, 100]], float)
        uv = np.array([[.05, .95], [.95, .95], [.95, .05], [.05, .05]])
        tex = np.full((128, 128, 4), [225, 224, 205, 173], np.uint8)
        tex[40:60, 40:60, :3] = [30, 45, 120]
        art = np.full((128, 128, 4), [243, 242, 223, 255], np.uint8)
        revised, allowed, _ = repair.transfer_blouse_art(p, uv, tex,
            np.array([[0, 1, 2], [0, 2, 3]]), np.array([10, 10]), art, repair.BOUNDS)
        self.assertGreater(np.count_nonzero(np.any(revised != tex, axis=2)), 1000)
        np.testing.assert_array_equal(revised[~allowed], tex[~allowed])
        np.testing.assert_array_equal(revised[:, :, 3], tex[:, :, 3])
        np.testing.assert_array_equal(revised[40:60, 40:60], tex[40:60, 40:60])


    def test_hidden_chest_surface_and_authored_colored_drift_are_protected(self):
        front = np.array([[-620, 3860, 100], [200, 3860, 100],
                          [200, 4440, 100], [-620, 4440, 100]], float)
        front_uv = np.array([[.02, .95], [.45, .95], [.45, .05], [.02, .05]])
        back_uv = front_uv+[.51, 0]
        tex = np.full((128, 128, 4), [225, 224, 205, 255], np.uint8)
        art = np.full((128, 128, 4), [243, 242, 223, 255], np.uint8)
        art[55:75, 35:50, :3] = [220, 180, 15]
        revised, allowed, _ = repair.transfer_blouse_art(
            np.concatenate([front, front-[0, 0, 100]]),
            np.concatenate([front_uv, back_uv]), tex,
            np.array([[0, 1, 2], [0, 2, 3], [4, 5, 6], [4, 6, 7]]),
            np.array([10, 10, 10, 10]), art, repair.BOUNDS)
        self.assertGreater(np.count_nonzero(np.any(revised[:, :60] != tex[:, :60], axis=2)), 1000)
        np.testing.assert_array_equal(revised[:, 64:], tex[:, 64:])
        # A generator's stray yellow scarf color must never enter white fabric.
        self.assertTrue(np.all(repair.cloth_color(revised[allowed])))


    def test_disconnected_uv_charts_do_not_expose_original_triangle_lines(self):
        from native_npc_face import bake
        corner = np.array([[-620, 3860, 100], [200, 3860, 100],
                           [200, 4440, 100], [-620, 4440, 100]], float)
        p = corner[[0, 1, 2, 0, 2, 3]]
        uv = np.array([[.05, .95], [.45, .95], [.45, .05],
                       [.55, .95], [.95, .05], [.55, .05]])
        faces = np.array([[0, 1, 2], [3, 4, 5]])
        tex = np.full((128, 128, 4), [225, 224, 205, 255], np.uint8)
        art = np.full_like(tex, [243, 242, 223, 255])
        revised, _, _ = repair.transfer_blouse_art(p, uv, tex, faces, np.array([10, 10]), art)
        projection, _, valid = bake(p, uv, revised, faces, repair.BOUNDS, size=128)
        yy, xx = np.indices((128, 128))
        x = -950+xx/127*1600; y = 5100-yy/127*1600
        interior = valid & (x > -500) & (x < 80) & (y > 3970) & (y < 4320)
        error = abs(projection[interior, :3].astype(int)-[243, 242, 223]).max()
        self.assertLessEqual(error, 2, 'UV chart gutters must not leave original fold-colored triangle seams')


    def test_dark_gray_fold_can_clean_while_scarf_ink_rim_stays_exact(self):
        from scipy.ndimage import binary_dilation
        image, owners, gray, _ = self.target()
        image[gray] = [42, 43, 28, 255]
        yellow = (image[:, :, 0] > 200) & (image[:, :, 2] < 30)
        rim = binary_dilation(yellow, iterations=2) & ~yellow
        image[rim] = [40, 38, 18, 255]
        weight = repair.semantic_blouse_mask(image, owners, np.array([10]), repair.BOUNDS)
        self.assertGreater(np.count_nonzero(weight[gray] > .5), 40)
        self.assertFalse(np.any(weight[rim]), 'The real scarf ink rim is a protected detail')


class BlouseDestinationTests(unittest.TestCase):
    def test_hardlinked_output_cannot_replace_parent(self):
        with tempfile.TemporaryDirectory() as folder:
            folder = Path(folder)
            parent = folder/'parent.o2r'; parent.write_bytes(b'protected parent')
            output = folder/'output.o2r'; os.link(parent, output)
            with self.assertRaisesRegex(ValueError, 'Destination must be separate'):
                repair.build(folder/'native', parent, folder/'art.png', output)
            self.assertEqual(parent.read_bytes(), b'protected parent')

    def test_report_and_artwork_aliases_are_rejected_before_parsing(self):
        with tempfile.TemporaryDirectory() as folder:
            folder = Path(folder)
            artwork = folder/'blouse-source-projection.png'; artwork.write_bytes(b'artwork')
            with self.assertRaisesRegex(ValueError, 'Destination must be separate'):
                repair.build(folder/'native', folder/'parent', artwork, folder/'output', folder)
            with self.assertRaisesRegex(ValueError, 'Destination must be separate'):
                repair.build(folder/'native', folder/'parent', folder/'art.png',
                             folder/'blouse-report.json', folder)


if __name__ == '__main__':
    unittest.main()
