"""Regression checks for the bounded archive-authoritative face lighting POC."""
import importlib.util
from pathlib import Path
import unittest

import numpy as np


class SkinFieldTests(unittest.TestCase):
    def repair(self):
        path = Path(__file__).with_name('repair_npc_face_torso.py')
        self.assertTrue(path.exists(), 'The face normal field repair is missing')
        spec = importlib.util.spec_from_file_location('repair_npc_face_torso', path)
        module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
        self.assertTrue(callable(getattr(module, 'smooth_skin_field', None)))
        return module

    def curved_skin(self):
        x, y = np.meshgrid(np.linspace(-.07, .07, 25), np.linspace(.79, .925, 25))
        p = np.c_[x.ravel(), y.ravel(), (.08-3*x*x-2*(y-.86)**2).ravel()]
        n = np.c_[6*x.ravel(), 4*(y-.86).ravel(), np.ones(x.size)]
        n /= np.linalg.norm(n, axis=1, keepdims=True)
        return p, n

    def test_angular_skin_lighting_contrast_is_reduced_with_curvature_retained(self):
        module = self.repair(); p, expected = self.curved_skin()
        n = expected.copy(); n[::2, 0] += .45; n[1::2, 0] -= .45
        n /= np.linalg.norm(n, axis=1, keepdims=True)
        out = module.smooth_skin_field(p, n, p, n, np.ones(len(p)))
        # Actual F3DEX directional shade uses max(0, dot(n, normalized light)).
        light = np.array([.8, .2, .4]); light /= np.linalg.norm(light)
        before = np.maximum(n@light, 0).reshape(25, 25)
        after = np.maximum(out@light, 0).reshape(25, 25)
        self.assertLess(np.abs(np.diff(after, axis=1)).mean(),
                        np.abs(np.diff(before, axis=1)).mean()*.2)
        self.assertLess(np.linalg.norm(out-expected, axis=1).mean(), .05)
        self.assertGreater(out[p[:, 0]>.05, 0].mean(), .2)

    def test_overlapping_skin_layers_receive_the_same_normal_field(self):
        module = self.repair(); p, n = self.curved_skin()
        targets = np.vstack([p, p]); old = np.vstack([n, -n])
        out = module.smooth_skin_field(p, n, targets, old, np.ones(len(targets)))
        np.testing.assert_allclose(out[:len(p)], out[len(p):], atol=1e-12)

    def test_zero_envelope_keeps_protected_normals_and_every_position(self):
        module = self.repair(); p, n = self.curved_skin(); before = p.copy(); old = n.copy()
        strength = np.ones(len(p)); strength[::3] = 0
        out = module.smooth_skin_field(p, n, p, n, strength)
        np.testing.assert_array_equal(out[::3], n[::3])
        np.testing.assert_array_equal(p, before); np.testing.assert_array_equal(n, old)


class EyeArtworkTests(unittest.TestCase):
    repair = SkinFieldTests.repair
    def test_eye_mask_excludes_brows_hair_nose_and_mouth(self):
        module = self.repair()
        self.assertTrue(callable(getattr(module, 'eye_art_mask', None)), 'The bounded eye artwork mask is missing')
        bounds = [-.075, .795, .075, .90]
        mask = module.eye_art_mask(bounds, 1000)
        def at(x, y):
            return mask[round((bounds[3]-y)/(bounds[3]-bounds[1])*999),
                        round((x-bounds[0])/(bounds[2]-bounds[0])*999)]
        self.assertGreater(at(.031, .853), .99); self.assertGreater(at(-.031, .853), .99)
        for point in [(0, .853), (.031, .883), (0, .82), (.07, .87), (0, .90)]:
            self.assertEqual(at(*point), 0, f'Protected source point {point} must remain outside the eye mask')

    def test_eye_art_transfer_preserves_all_other_atlas_pixels(self):
        module = self.repair()
        self.assertTrue(callable(getattr(module, 'transfer_eye_art', None)), 'The bounded UV artwork transfer is missing')
        bounds = [-.075, .795, .075, .90]
        points = np.array([[-.075,.795,.08],[.075,.795,.08],[.075,.90,.08],[-.075,.90,.08]])
        uv = np.array([[0,1],[1,1],[1,0],[0,0]], float)
        atlas = np.full((200,200,4), [120,100,80,255], np.uint8)
        art = np.full((200,200,4), [20,220,40,255], np.uint8)
        out, allowed = module.transfer_eye_art(points, uv, atlas, [[0,1,2],[0,2,3]], art, bounds)
        self.assertGreater(np.count_nonzero((out != atlas).any(2)), 100)
        np.testing.assert_array_equal(out[~allowed], atlas[~allowed])
        np.testing.assert_array_equal(out[:10], atlas[:10])
        np.testing.assert_array_equal(out[:,90:110], atlas[:,90:110])
        np.testing.assert_array_equal(out[...,3], atlas[...,3])

    def test_green_brow_or_hair_texels_outside_the_iris_are_protected(self):
        module = self.repair(); bounds = [-.075, .795, .075, .90]
        points = np.array([[-.075,.795,.08],[.075,.795,.08],[.075,.90,.08],[-.075,.90,.08]])
        uv = np.array([[0,1],[1,1],[1,0],[0,0]], float)
        atlas = np.full((200,200,4), [15,120,35,255], np.uint8)
        art = np.full((200,200,4), [230,190,130,255], np.uint8)
        out, _ = module.transfer_eye_art(points, uv, atlas, [[0,1,2],[0,2,3]], art, bounds)
        def pixel(x, y):
            return round((bounds[3]-y)/(bounds[3]-bounds[1])*199), round((x-bounds[0])/(bounds[2]-bounds[0])*199)
        self.assertFalse(np.array_equal(out[pixel(.031,.853)], atlas[pixel(.031,.853)]))
        np.testing.assert_array_equal(out[pixel(.055,.864)], atlas[pixel(.055,.864)])

    def test_projection_skin_outside_actual_apertures_is_not_repainted(self):
        module = self.repair(); bounds = [-.075, .795, .075, .90]
        points = np.array([[-.075,.795,.08],[.075,.795,.08],[.075,.90,.08],[-.075,.90,.08]])
        uv = np.array([[0,1],[1,1],[1,0],[0,0]], float)
        atlas = np.full((200,200,4), [240,180,120,255], np.uint8)
        art = np.full((200,200,4), [230,165,110,255], np.uint8)
        yy, xx = np.indices((200,200)); x = -.075+xx/199*.15; y = .90-yy/199*.105
        for cx in (-.031,.031):
            eye = ((x-cx)/.016)**2+((y-.853)/.010)**2 < 1
            atlas[eye] = [180,190,190,255]; art[eye] = [20,120,40,255]
        out, _ = module.transfer_eye_art(points, uv, atlas, [[0,1,2],[0,2,3]], art, bounds)
        def pixel(x, y):
            return round((bounds[3]-y)/(bounds[3]-bounds[1])*199), round((x-bounds[0])/(bounds[2]-bounds[0])*199)
        self.assertFalse(np.array_equal(out[pixel(.031,.853)], atlas[pixel(.031,.853)]))
        np.testing.assert_array_equal(out[pixel(.031,.835)], atlas[pixel(.031,.835)])


if __name__ == '__main__':
    unittest.main()
