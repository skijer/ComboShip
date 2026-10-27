"""Regression checks for the traditional Saria gaze and eyelid boundaries."""
import inspect
import unittest

import numpy as np

import native_npc_face as face


class ExpressionSeamTests(unittest.TestCase):
    def test_face_paint_changes_only_covered_atlas_texels(self):
        self.assertTrue(callable(getattr(face,'reproject_face_paint',None)))
        p=np.array([[.2,.2,.1],[.8,.2,.1],[.2,.8,.1]])
        atlas=np.full((16,16,4),[30,40,50,255],np.uint8)
        art=np.full((8,8,4),[220,170,130,255],np.uint8)
        mask=np.full((8,8),255,np.uint8)
        out=face.reproject_face_paint(p,p[:,:2],atlas,np.array([[0,1,2]]),art,mask,[0,0,1,1])
        np.testing.assert_array_equal(out[5,5],art[0,0])
        np.testing.assert_array_equal(out[0],atlas[0])
        np.testing.assert_array_equal(out[14],atlas[14])
        np.testing.assert_array_equal(atlas,np.full_like(atlas,[30,40,50,255]))
        unchanged=face.reproject_face_paint(p,p[:,:2],atlas,np.array([[0,1,2]]),art,np.zeros_like(mask),[0,0,1,1])
        np.testing.assert_array_equal(unchanged,atlas)

    def test_mouth_refinement_keeps_native_perimeter_edges_unsplit(self):
        self.assertTrue(callable(getattr(face, 'expression_subdivision', None)),
                        'Mouth detail must not insert rounded vertices into an adjoining unsplit edge')
        p = np.array([[0., 0., 0.], [113., 27., 15.], [21., 117., 43.]])
        original = np.array([[0, 1, 2]])
        bary, source_faces = face.expression_subdivision(original)
        triangles = np.rint(np.einsum('fij,fjk->fik', bary, p[source_faces])).astype(int)
        edges = np.concatenate([triangles[:, [0, 1]], triangles[:, [1, 2]], triangles[:, [2, 0]]])
        from collections import Counter
        counts = Counter(tuple(sorted(map(tuple, edge))) for edge in edges)
        boundary = {edge for edge, count in counts.items() if count == 1}
        expected = {tuple(sorted([tuple(p[a].astype(int)), tuple(p[b].astype(int))]))
                    for a, b in [(0, 1), (1, 2), (2, 0)]}
        self.assertEqual(boundary, expected)
        self.assertGreater(len(triangles), 16, 'Talking detail must retain enough interior samples')

    def test_lid_can_preserve_concave_source_skin_topology(self):
        self.assertIn('preserve_topology', inspect.signature(face.make_lid_surface).parameters,
                      'Unconstrained lid triangulation must not cut off source boundary corners')
        p = np.array([[0., 0., .1], [.04, 0., .1], [.04, .01, .1],
                      [.01, .01, .1], [.01, .04, .1], [0., .04, .1]])
        fs = np.array([[0, 1, 3], [1, 2, 3], [0, 3, 5], [3, 4, 5]])
        model = dict(source_positions=p, positions=p*10000, normals=np.tile([0., 0., 1.], (6, 1)),
                     faces=fs, head_transform=np.column_stack([np.eye(3)*10000, np.zeros(3)]))
        out = face.make_lid_surface(model, fs, np.array([.1, 0, 0, 0, 0, 0]),
                                    (0., 0.), [0, 0, .04, .04], 15,
                                    preserve_vertices=np.arange(6), preserve_topology=True)
        signature = lambda points, triangles: {tuple(sorted(map(tuple, np.rint(points[f]).astype(int)))) for f in triangles}
        self.assertEqual(signature(out['positions'], out['faces']), signature(model['positions'], fs))

    def test_closed_saria_lid_uses_skin_instead_of_overlapping_eye_shells(self):
        p = np.array([[-1., -1., .04], [1., -1., .04], [1., 1., .04], [-1., 1., .04],
                      [-.2, -.2, .08], [.2, -.2, .08], [0., .2, .08]])
        fs = np.array([[0, 1, 2], [0, 2, 3], [4, 5, 6]])
        model = dict(source_positions=p, positions=p.copy(), normals=np.tile([0., 0., 1.], (7, 1)),
                     mesh_ids=np.array([6]*4+[14]*3), faces=fs,
                     head_transform=np.column_stack([np.eye(3), np.zeros(3)]))
        lid, retained = face.make_front_lid_surface(model, fs, np.array([.04, 0, 0, 0, 0, 0]),
                                                   (0., 0.), [-1, -1, 1, 1], 15)
        self.assertEqual(len(lid['faces']), 2)
        self.assertEqual(len(lid['positions']), 4)
        self.assertEqual(len(retained), 0)


class TraditionalSariaEyeTests(unittest.TestCase):
    def test_projection_labels_follow_depth_and_leave_uncovered_pixels_unowned(self):
        self.assertIn("face_labels", inspect.signature(face.bake).parameters,
                      "Gaze masks must use the same depth winner as the source bake")
        p = np.array([[0., 0., .1], [1., 0., .1], [0., 1., .1],
                      [.2, .2, .2], [.8, .2, .2], [.2, .8, .2],
                      [.3, .3, .3], [.6, .3, .3], [.3, .6, .3]])
        fs = np.arange(9).reshape(-1, 3)
        atlas = np.full((2, 2, 4), 255, np.uint8)
        _, _, valid, owners = face.bake(p, p[:, :2], atlas, fs, [0, 0, 1, 1],
                                        size=21, face_labels=np.array([6, 14, 0]))
        self.assertEqual(owners[19, 1], 6)
        self.assertEqual(owners[15, 5], 14)
        self.assertEqual(owners[13, 7], 0)
        self.assertFalse(valid[1, 19])
        self.assertEqual(owners[1, 19], -1,
                         "Nearest-filled background pixels are not visible eyeball pixels")
        self.assertEqual(len(face.bake(p, p[:, :2], atlas, fs, [0, 0, 1, 1], size=21)), 3)

    def test_gaze_copy_keeps_two_pixel_lash_border_and_occluding_hair(self):
        self.assertTrue(hasattr(face, "compose_visible_gaze"),
                        "The gaze repaint must be confined to the visible iris aperture")
        base = np.full((18, 18, 4), [205, 170, 100, 255], np.uint8)
        base[13, 3:15] = [9, 12, 3, 255]  # Original lower lash.
        authored = np.full_like(base, [245, 250, 255, 255])
        authored[8, 10] = [10, 45, 160, 255]  # Centered pupil/iris correction.
        owners = np.full((18, 18), 6)
        owners[3:14, 3:15] = 14
        owners[6:9, 6] = 0  # A nearer bang crossing the eye.
        result = face.compose_visible_gaze(base, authored, owners, (14, 15), inset=2)
        np.testing.assert_array_equal(result[13, 3:15], base[13, 3:15])
        np.testing.assert_array_equal(result[3:5, 3:15], base[3:5, 3:15])
        np.testing.assert_array_equal(result[owners != 14], base[owners != 14])
        np.testing.assert_array_equal(result[8, 10], authored[8, 10])
        np.testing.assert_array_equal(result[6:9, 6], base[6:9, 6])

    def test_blink_preserves_visible_skin_rim_depth_and_shading(self):
        self.assertTrue(hasattr(face, "visible_skin_vertices"),
                        "Closing an eyeball must retain the surrounding source face contour")
        p = np.array([[-2., -2., .12], [2., -2., .09], [2., 2., .13],
                      [-2., 2., .08], [0., 0., .10],
                      [-.5, -.5, .30], [.5, -.5, .30], [.5, .5, .30], [-.5, .5, .30]])
        fs = np.array([[0, 1, 4], [1, 2, 4], [2, 3, 4], [3, 0, 4],
                       [5, 6, 7], [5, 7, 8]])
        normals = np.array([[.2, 0., .98], [0., .2, .98], [-.2, 0., .98],
                            [0., -.2, .98]] + [[0., 0., 1.]]*5)
        model = dict(source_positions=p, positions=p.copy(), normals=normals.copy(),
                     mesh_ids=np.array([6]*5+[14]*4), faces=fs,
                     head_transform=np.column_stack([np.eye(3), np.zeros(3)]))
        original = model["positions"].copy()
        keep = face.visible_skin_vertices(model, fs, 6, (14, 15))
        np.testing.assert_array_equal(keep, [0, 1, 2, 3])
        out = face.make_lid_surface(model, fs, np.array([.15, 0, 0, 0, 0, 0]),
                                    (0., 0.), [-2, -2, 2, 2], 15, preserve_vertices=keep)
        for i in keep:
            index = np.flatnonzero(np.all(np.isclose(out["positions"][:, :2], p[i, :2]), axis=1))[0]
            np.testing.assert_array_equal(out["positions"][index], p[i])
            np.testing.assert_array_equal(out["normals"][index], normals[i])
        center = np.flatnonzero(np.all(out["positions"][:, :2] == 0, axis=1))[0]
        self.assertAlmostEqual(out["positions"][center, 2], .15)
        np.testing.assert_array_equal(model["positions"], original)

    def test_gaze_expands_into_connected_visible_face_eye_pixels_without_copying_lashes(self):
        base = np.full((20, 20, 4), [205, 170, 100, 255], np.uint8)
        base[3:15, 3:17] = [190, 205, 225, 255]
        base[8, 12] = [10, 65, 190, 255]  # Old iris on the underlying face mesh.
        base[7:9, 8] = [3, 7, 10, 255]  # A pupil enclosed by the aperture.
        base[15, 3:17] = [3, 7, 2, 255]  # Lower lash, open to surrounding skin.
        base[1, 1] = [20, 70, 190, 255]  # Disconnected blue fragment.
        base[6:13, 17:] = [20, 70, 190, 255]  # Connected pixels beyond the authored eye bounds.
        owners = np.full((20, 20), 6)
        owners[6:11, 6:11] = 14
        owners[7:9, 8] = 6  # Part of the dark pupil is painted on the face too.
        owners[7:9, 3] = 0  # Hair cannot become aperture skin.
        authored = np.full_like(base, [245, 250, 255, 255])
        authored[8, 8] = [15, 45, 160, 255]
        authored[10:13, 9:13] = [205, 170, 100, 255]  # Repaint shrank the eye and inserted skin.
        bounds = np.zeros((20, 20), bool)
        bounds[2:17, 2:17] = True
        out = face.compose_visible_gaze(base, authored, owners, (14, 15), aperture_mask=bounds)
        np.testing.assert_array_equal(out[8, 12], authored[8, 12],
                                      "An old iris crescent must not survive on visible face6")
        np.testing.assert_array_equal(out[8, 8], authored[8, 8])
        self.assertGreater(out[11, 11, 2], out[11, 11, 1]*.72,
                           "Authored skin inside the old aperture must not detach the lower lash")
        np.testing.assert_array_equal(out[15, 3:17], base[15, 3:17])
        np.testing.assert_array_equal(out[owners == 0], base[owners == 0])
        np.testing.assert_array_equal(out[1, 1], base[1, 1])
        np.testing.assert_array_equal(out[9, 17], base[9, 17])
        np.testing.assert_array_equal(out[17:], base[17:])

    def test_blink_keeps_side_skin_topology_out_of_the_projected_lid(self):
        self.assertTrue(hasattr(face, 'make_front_lid_surface'),
                        'Side faces outside the bake support must not be joined to a frontal lash')
        p = np.array([[-1., 0., .03], [1., 0., .04], [1., 1., .05], [-1., 1., .045],
                      [-.9, .3, -.02], [-.3, .3, .06], [.3, .3, .06], [0., .7, .06]])
        fs = np.array([[0, 1, 2], [0, 2, 3], [0, 3, 4], [5, 6, 7]])
        normals = np.tile([0., 0., 1.], (len(p), 1))
        normals[0] = [.2, 0., .98]
        model = dict(source_positions=p, positions=p.copy(), normals=normals.copy(),
                     mesh_ids=np.array([6]*5+[14]*3), faces=fs,
                     head_transform=np.column_stack([np.eye(3), np.zeros(3)]))
        before = model['positions'].copy()
        lid, retained = face.make_front_lid_surface(
            model, fs, np.array([.04, 0, 0, 0, 0, 0]), (0., .5), [-1, 0, 1, 1], 15)
        np.testing.assert_array_equal(retained, fs[2:3])
        self.assertFalse(np.any(np.all(np.isclose(lid['positions'][:, :2], p[4, :2]), axis=1)),
                         'A new lash triangle must not connect to skin behind the eye')
        for original in (0, 3):
            index = np.flatnonzero(np.all(np.isclose(lid['positions'][:, :2], p[original, :2]), axis=1))[0]
            np.testing.assert_array_equal(lid['positions'][index], p[original])
            np.testing.assert_array_equal(lid['normals'][index], normals[original])
        np.testing.assert_array_equal(model['positions'], before)


if __name__ == "__main__":
    unittest.main()
