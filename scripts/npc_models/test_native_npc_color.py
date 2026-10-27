"""Bounded diffuse repair: source identity, atlas ownership and luminance."""
import hashlib
from pathlib import Path
from types import SimpleNamespace
import unittest

import numpy as np
from PIL import Image

try:
    import native_npc_color as color
except ModuleNotFoundError:
    color = None


def luminance(rgb):
    value = np.asarray(rgb, float) / 255
    linear = np.where(value <= .04045, value / 12.92, ((value + .055) / 1.055) ** 2.4)
    return linear @ [.2126, .7152, .0722]


class DiffuseColorTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(color, "The bounded Traditional Saria color repair is missing")

    def test_unrecognized_source_is_an_unchanged_independent_copy(self):
        pixels = np.full((5, 6, 4), [78, 99, 4, 173], np.uint8)
        source = SimpleNamespace(raw=b"different GLB", image=Image.fromarray(pixels))
        result, report = color.repair_traditional_saria_diffuse(source)
        np.testing.assert_array_equal(result, pixels)
        result[0, 0, 0] = 0
        np.testing.assert_array_equal(source.image, pixels)
        self.assertFalse(report["applied"])

    def test_green_chroma_uses_clean_skin_and_retains_luminance_detail(self):
        pixels = np.array([[[104, 120, 13, 255], [78, 99, 4, 200], [50, 87, 7, 99],
                            [180, 150, 85, 255], [210, 175, 100, 255], [196, 166, 92, 255]]], np.uint8)
        target = np.array([[True, True, True, False, False, False]])
        reference = ~target
        before = pixels.copy()
        result, details = color.correct_green_chroma(pixels, target, np.zeros_like(target), reference)
        rgb = result[0, :3, :3].astype(float)
        self.assertTrue(np.all(rgb[:, 0] > rgb[:, 1]))
        self.assertTrue(np.all(rgb[:, 1] > rgb[:, 2]))
        np.testing.assert_allclose(luminance(result[..., :3]), luminance(pixels[..., :3]), atol=.002)
        np.testing.assert_array_equal(result[0, 3:], before[0, 3:])
        np.testing.assert_array_equal(result[..., 3], before[..., 3])
        np.testing.assert_array_equal(pixels, before)
        self.assertEqual(details["changed_texels"], 3)

    def test_non_target_and_overlapping_other_geometry_stay_byte_identical(self):
        pixels = np.array([[[78, 99, 4, 255], [78, 99, 4, 255], [78, 99, 4, 255],
                            [200, 165, 90, 255]]], np.uint8)
        target = np.array([[True, True, False, False]])
        protected = np.array([[False, True, False, False]])
        reference = np.array([[False, False, False, True]])
        result, _ = color.correct_green_chroma(pixels, target, protected, reference)
        self.assertFalse(np.array_equal(result[0, 0], pixels[0, 0]))
        np.testing.assert_array_equal(result[0, 1:], pixels[0, 1:])

    def test_green_threshold_feathers_continuously(self):
        ratios = np.array([.99, 1., 1.001, 1.025, 1.05, 1.075, 1.1, 1.2])
        rgb = np.column_stack([np.full(len(ratios), 100.), ratios * 100., np.full(len(ratios), 20.)])
        weight = color.green_weight(rgb)
        self.assertTrue(np.all(np.diff(weight) >= 0))
        np.testing.assert_array_equal(weight[:2], [0., 0.])
        self.assertGreater(weight[2], 0.)
        self.assertLess(weight[2], .001)
        self.assertEqual(weight[-1], 1.)

    def test_uv_footprint_includes_filtered_edges(self):
        uv = np.array([[.25, .25], [.5, .25], [.25, .5]])
        mask = color.uv_footprint(uv, np.array([[0, 1, 2]]), (32, 32), padding=2)
        self.assertTrue(mask[8, 6])
        self.assertFalse(mask[0, 0])
        self.assertFalse(mask[30, 30])


class RealDiffuseColorTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(color, "The bounded Traditional Saria color repair is missing")
        self.path = Path(__file__).resolve().parents[3] / "upload/Saria+Render+GLB.glb"
        if not self.path.exists():
            self.skipTest("Supplied source GLB is unavailable")
        from native_npc_models import SourceModel
        self.source = SourceModel(self.path)
        self.before = np.asarray(self.source.image).copy()
        self.result, self.report = color.repair_traditional_saria_diffuse(self.source)

    def sample(self, pixels, mesh_id, local_ids):
        uv = self.source.meshes[mesh_id]["uv"][local_ids]
        return pixels[(uv[:, 1] * pixels.shape[0]).astype(int),
                      (uv[:, 0] * pixels.shape[1]).astype(int), :3]

    def test_diagnosed_thumb_chart_seams_do_not_gain_a_skin_color_boundary(self):
        # These coincident source points were nearly the same yellow-green;
        # the green-only gate turned just one chart brown in POC5.
        for mesh_id, ids in ((4, [113, 114]), (8, [106, 108])):
            with self.subTest(mesh_id=mesh_id):
                rgb = self.sample(self.result, mesh_id, ids)
                value = rgb.astype(float) / 255
                linear = np.where(value <= .04045, value / 12.92, ((value + .055) / 1.055) ** 2.4)
                chroma = linear / luminance(rgb)[:, None]
                self.assertLess(np.linalg.norm(chroma[0] - chroma[1]), .06)

    def test_mustard_hand_texels_without_green_dominance_receive_skin_chroma(self):
        for mesh_id, ids in ((4, [114]), (8, [126])):
            with self.subTest(mesh_id=mesh_id):
                original = self.sample(self.before, mesh_id, ids)[0]
                self.assertLessEqual(int(original[1]), int(original[0]))
                rgb = self.sample(self.result, mesh_id, ids)[0]
                value = rgb.astype(float) / 255
                linear = np.where(value <= .04045, value / 12.92, ((value + .055) / 1.055) ** 2.4)
                # Clean source cheek blue/linear-luminance ratio is .241;
                # the mustard source texels have insufficient blue chroma.
                self.assertGreater(linear[2] / luminance(rgb), .20)
                self.assertLess(linear[2] / luminance(rgb), .29)

    def test_baked_dark_hand_tone_moves_toward_clean_skin_without_flattening_detail(self):
        for mesh_id in (4, 8):
            with self.subTest(mesh_id=mesh_id):
                mesh = self.source.meshes[mesh_id]
                mask = color.uv_footprint(mesh["uv"], mesh["faces"], self.source.image.size, padding=0)
                before = luminance(self.before[mask, :3])
                after = luminance(self.result[mask, :3])
                # Audited clean cheek median Y=.443; the hands were .146/.185.
                self.assertLess(abs(np.median(after) - .443), abs(np.median(before) - .443) * .65)
                self.assertGreater(np.corrcoef(before, after)[0, 1], .995)
                self.assertGreater(np.std(after), np.std(before) * .35)

    def test_supplied_traditional_saria_is_narrowly_repaired_without_source_mutation(self):
        source, before, result, report = self.source, self.before, self.result, self.report
        original_hash = hashlib.sha256(source.raw).hexdigest()
        self.assertTrue(report["applied"])
        self.assertGreater(report["changed_texels"], 50_000)
        self.assertEqual(report["target_mesh_ids"], [4, 7, 8, 11])
        self.assertLess(report["remaining_strong_green_texels"], report["original_strong_green_texels"] * .01)
        changed = np.any(result != before, axis=-1)
        protected = np.zeros(before.shape[:2], bool)
        for index, mesh in enumerate(source.meshes):
            if index not in report["target_mesh_ids"]:
                protected |= color.uv_footprint(mesh["uv"], mesh["faces"], source.image.size, padding=2)
        self.assertFalse(np.any(changed & protected))
        ears = np.zeros(before.shape[:2], bool)
        for index in (7, 11):
            mesh = source.meshes[index]
            ears |= color.uv_footprint(mesh["uv"], mesh["faces"], source.image.size, padding=2)
        self.assertLess(np.max(np.abs(luminance(result[ears, :3]) - luminance(before[ears, :3]))), .003)
        np.testing.assert_array_equal(result[..., 3], before[..., 3])
        np.testing.assert_array_equal(np.asarray(source.image), before)
        self.assertEqual(hashlib.sha256(self.path.read_bytes()).hexdigest(), original_hash)


if __name__ == "__main__":
    unittest.main()

class LegDiffuseTests(unittest.TestCase):
    def test_leg_chroma_is_warm_and_preserves_other_owned_texels_and_luminance(self):
        self.assertTrue(callable(getattr(color,'repair_traditional_saria_legs',None)), 'Leg skin calibration is missing')
        path=Path(__file__).resolve().parents[3]/'upload/Saria+Render+GLB.glb'
        if not path.exists():self.skipTest('Supplied GLB unavailable')
        from native_npc_models import SourceModel
        source=SourceModel(path)
        before,report=color.repair_traditional_saria_diffuse(source)
        after,details=color.repair_traditional_saria_legs(source,before,report['reference_linear_chroma'])
        protected=color._mesh_footprint([m for i,m in enumerate(source.meshes) if i not in (18,23)],source.image.size,2)
        np.testing.assert_array_equal(after[protected],before[protected])
        np.testing.assert_array_equal(after[...,3],before[...,3])
        changed=np.any(after!=before,axis=-1)
        self.assertGreater(changed.sum(),50000)
        rgb=after[changed,:3].astype(float)
        self.assertTrue(np.all(rgb[:,0]>=rgb[:,1]))
        self.assertLess(np.max(np.abs(luminance(after[changed,:3])-luminance(before[changed,:3]))),.005)
        np.testing.assert_array_equal(np.asarray(source.image),np.asarray(SourceModel(path).image))
