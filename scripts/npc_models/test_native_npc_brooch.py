"""Regression checks for the complete, chest-bound young Malon clasp."""
import hashlib
import unittest
from types import SimpleNamespace

import numpy as np

import native_npc_brooch as brooch


class BroochRepairTests(unittest.TestCase):
    def test_clasp_seats_into_the_curved_scarf_without_changing_the_emblem(self):
        original=brooch.clasp_geometry()
        self.assertTrue(hasattr(brooch,'seat_clasp'),'The brooch must follow the scarf instead of floating in front')
        seated=brooch.seat_clasp(original)
        before=original['positions'];after=seated['positions']
        np.testing.assert_array_equal(after[:,:2],before[:,:2])
        np.testing.assert_array_equal(seated['uv'],original['uv'])
        np.testing.assert_array_equal(seated['faces'],original['faces'])
        pull=before[:,2]-after[:,2]
        self.assertGreater(pull[before[:,1]>.730].min(),.035)
        self.assertLess(pull[before[:,1]<.665].max(),.007)
        self.assertGreater(pull.min(),.003)
        self.assertTrue(np.isfinite(seated['normals']).all())
        used=np.unique(seated['faces'])
        np.testing.assert_allclose(np.linalg.norm(seated['normals'][used],axis=1),1.,atol=1e-12)
        # Recovering the seat plane restores the complete original relief.
        recovered=after[:,2]+.005+.5*(after[:,1]-.662)
        np.testing.assert_allclose(recovered,before[:,2],atol=1e-12)

    def test_source_mask_removes_lower_jaw_and_crossing_triangles(self):
        # The old centroid window began at .675, leaving the jaw and spurs.
        pieces = ['tripo_part_11', 'tripo_part_15', 'collar', 'shirt']
        faces = np.arange(18).reshape(-1, 3)
        p = np.array([
            [-.01, .665, .12], [.01, .665, .12], [0, .672, .125],
            [-.01, .658, .12], [.01, .658, .12], [0, .665, .125],
            [-.01, .635, .12], [.01, .635, .12], [0, .660, .125],
            [-.04, .72, .09], [.04, .72, .09], [0, .75, .09],
            [-.01, .69, .10], [.01, .69, .10], [0, .73, .10],
            [-.01, .71, .12], [.01, .71, .12], [0, .73, .12],
        ])
        source = SimpleNamespace(meshes=[{'name': s} for s in pieces])
        model = dict(source_positions=p, faces=faces,
                     mesh_ids=np.repeat([0, 1, 0, 2, 3, 1], 3))
        before = p.copy()
        np.testing.assert_array_equal(brooch.source_mask(source, model),
                                      [True, True, False, False, False, True],
                                      'All original jaw/spur triangles must be removed while collar/tails remain')
        np.testing.assert_array_equal(p, before)

    def test_source_mask_selects_only_inner_collar_tip_for_reemission(self):
        source = SimpleNamespace(meshes=[{'name': 'tripo_part_14'}, {'name': 'neck'}])
        p = np.array([[.02112, .74103, .06871], [.01971, .74261, .06350], [.00883, .72314, .07615],
                      [.04, .741, .069], [.07, .75, .064], [.05, .723, .076],
                      [.02112, .74103, .06871], [.01971, .74261, .06350], [.00883, .72314, .07615]])
        model = dict(source_positions=p, faces=np.arange(9).reshape(-1, 3),
                     mesh_ids=np.repeat([0, 0, 1], 3))
        np.testing.assert_array_equal(brooch.source_mask(source, model), [True, False, False],
                                      'Replace inner tab faces, preserving outer collar and skin')

    def test_collar_gap_cleanup_retains_the_low_original_scarf_join(self):
        source = SimpleNamespace(meshes=[{'name': 'tripo_part_14'}])
        p = np.array([[.003, .712, .08], [.014, .718, .08], [.020, .720, .08]])
        model = dict(source_positions=p, faces=np.array([[0, 1, 2]]), mesh_ids=np.zeros(3, int))
        np.testing.assert_array_equal(brooch.source_mask(source, model), [False],
                                      'The collar must still join behind the brooch below the horns')

    def test_collar_join_adjustment_preserves_complete_faces_and_uvs(self):
        self.assertTrue(hasattr(brooch, 'collar_join_geometry'),
                        'The curved scarf must remain whole, with its inner tab shifted behind the horn')
        source = SimpleNamespace(meshes=[{'name': 'tripo_part_14'}])
        p = np.array([[.04, .74, .07], [.01, .73, .08], [.02, .75, .06]])
        model = dict(source_positions=p, positions=p*1000, faces=np.array([[0, 1, 2]]),
                     uv=p[:, :2]*5, normals=np.tile([0, 0, 1.], (3, 1)),
                     mesh_ids=np.zeros(3, int), bones=np.full(3, 7),
                     head_transform=np.column_stack([np.eye(3)*1000, np.zeros(3)]))
        joined = brooch.collar_join_geometry(source, model)
        np.testing.assert_array_equal(joined['faces'], model['faces'])
        np.testing.assert_array_equal(joined['uv'], model['uv'])
        np.testing.assert_array_equal(joined['positions'][:, 1:], model['positions'][:, 1:])
        shift = joined['positions'][:, 0]-model['positions'][:, 0]
        self.assertTrue((shift >= 0).all())
        self.assertGreater(shift.max(), 1.)
        self.assertLessEqual(shift.max(), 8.5)
        np.testing.assert_array_equal(joined['bones'], np.full(3, 7))
        self.assertTrue(np.isfinite(joined['normals']).all())

    def test_reference_pixels_and_provenance_are_retained(self):
        self.assertTrue(hasattr(brooch, 'reference_art'),
                        'Use the complete supplied child concept emblem instead of the adult projection')
        pixels, report = brooch.reference_art()
        self.assertEqual(pixels.shape[2], 4)
        self.assertEqual(report['reference_crop'], [466, 426, 551, 523])
        self.assertEqual(hashlib.sha256(pixels.tobytes()).hexdigest(), report['crop_pixel_sha256'])
        self.assertIn('Refined young Malon 3D T-pose.png', report['source_artwork'])
        # Pixel landmarks cover both horns, forehead, eyes, muzzle, open mouth and chin.
        for x, y in [(20, 11), (64, 12), (43, 14), (25, 43), (59, 43),
                     (30, 55), (55, 55), (43, 69), (43, 91)]:
            self.assertGreater(pixels[y, x, 3], 0)

    def test_relief_contains_intact_landmarks_and_real_edge_thickness(self):
        self.assertTrue(hasattr(brooch, 'clasp_geometry'),
                        'The badge must have shaped relief and thickness rather than one flat depth')
        geometry = brooch.clasp_geometry()
        p, f, uv = geometry['positions'], geometry['faces'], geometry['uv']
        self.assertGreater(np.ptp(p[:, 2]), .010)
        self.assertGreater(p[:, 1].max(), .735)
        self.assertLess(p[:, 1].min(), .665)
        self.assertGreater(p[:, 0].max()-p[:, 0].min(), .063)
        self.assertTrue(np.isfinite(p).all())
        self.assertTrue(np.isfinite(geometry['normals']).all())
        self.assertTrue(((uv >= 0) & (uv <= 1)).all())
        for pixel in [(20, 11), (64, 12), (43, 14), (25, 43), (59, 43),
                      (30, 55), (55, 55), (43, 69), (43, 91)]:
            q = np.array(pixel)/[84, 96]
            t = uv[f]
            a, b = t[:, 0]-t[:, 2], t[:, 1]-t[:, 2]
            d = q-t[:, 2]
            det = a[:, 0]*b[:, 1]-a[:, 1]*b[:, 0]
            ok = abs(det)>1e-10
            u = np.full(len(f), -np.inf); v = u.copy()
            u[ok] = (d[ok, 0]*b[ok, 1]-d[ok, 1]*b[ok, 0])/det[ok]
            v[ok] = (a[ok, 0]*d[ok, 1]-a[ok, 1]*d[ok, 0])/det[ok]
            self.assertTrue(np.any(ok & (u >= -1e-6) & (v >= -1e-6) & (u+v <= 1+1e-6)),
                            f'Complete emblem landmark {pixel} must lie on exported geometry')
        # Texture/normal seams may split vertices, but the geometric shell stays closed.
        _, welded = np.unique(np.round(p, 10), axis=0, return_inverse=True)
        wf = welded[f]
        edges = np.sort(np.concatenate([wf[:, [0, 1]], wf[:, [1, 2]], wf[:, [2, 0]]]), axis=1)
        _, count = np.unique(edges, axis=0, return_counts=True)
        self.assertTrue(np.all(count == 2), 'The relief must be a closed clasp with no open border')

    def test_relief_has_a_thin_gold_rim_without_stretched_background_pixels(self):
        self.assertTrue(hasattr(brooch, 'clasp_geometry'))
        g = brooch.clasp_geometry()
        p, f, uv = g['positions'], g['faces'], g['uv']
        self.assertLess(np.ptp(p[:, 2]), .022, 'A small clasp must not become a thick slab')
        fn = np.cross(p[f[:, 1]]-p[f[:, 0]], p[f[:, 2]]-p[f[:, 0]])
        side = abs(fn[:, 2]) < 1e-12
        self.assertTrue(side.any())
        self.assertTrue(np.all(np.ptp(uv[f[side]], axis=1) < 1e-12),
                        'Rim faces need a gold sample, not stretched perimeter/background texels')


if __name__ == '__main__':
    unittest.main()
