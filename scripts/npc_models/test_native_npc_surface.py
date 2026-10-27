"""Bounded source-normal repairs must not blend garment interiors or faces."""
import copy
from types import SimpleNamespace
import unittest

import numpy as np

try:
    import native_npc_surface as surface
except ModuleNotFoundError:
    surface = None


class SurfaceNormalTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(surface, 'The bounded normal repair helper is missing')

    def test_outer_elbow_pair_converges_without_touching_inward_cap(self):
        positions = np.array([[0., 0., .025], [.001, .001, .025],
                              [.001, 0., .025], [.001, 0., .025]])
        normals = np.array([[0., -.342, .940], [0., .342, .940],
                            [0., 0., -1.], [1., 0., 0.]])
        normals /= np.linalg.norm(normals, axis=1)[:, None]
        original = normals.copy()
        repaired, count = surface._blend_elbow_rims(
            positions, normals, np.array([0, 1, 1, 1]), 0, 1,
            np.zeros(3), np.array([1., 0., 0.]))
        self.assertGreater(count, 0)
        self.assertGreater(np.dot(repaired[0], repaired[1]), .98)
        np.testing.assert_array_equal(repaired[2:], original[2:])
        np.testing.assert_array_equal(normals, original)

    def test_calf_filter_is_local_and_reduces_a_normal_spike(self):
        x, y = np.meshgrid(np.linspace(-.008, .008, 5), np.linspace(-.008, .008, 5))
        positions = np.column_stack([x.ravel(), y.ravel(), np.zeros(x.size)])
        normals = np.tile([0., 0., -1.], (len(positions), 1))
        middle = len(normals) // 2
        normals[middle] = [.35, 0., -np.sqrt(1-.35**2)]
        strength = np.zeros(len(normals)); strength[middle] = .55
        repaired = surface._filter_normals(positions, normals, strength)
        self.assertLess(abs(repaired[middle, 0]), abs(normals[middle, 0])*.75)
        np.testing.assert_array_equal(repaired[strength == 0], normals[strength == 0])
        np.testing.assert_allclose(np.linalg.norm(repaired, axis=1), 1.)

    def test_other_source_hash_is_a_complete_noop(self):
        source = SimpleNamespace(raw=b'an unrelated model')
        rig = SimpleNamespace(name='saria')
        model = dict(positions=np.array([[1., 2., 3.]]),
                     normals=np.array([[0., 0., 1.]]),
                     posed_source_normals=np.array([[0., 0., 1.]]))
        before = copy.deepcopy(model)
        self.assertIsNone(surface.repair_anime_surface_normals(source, model, rig))
        for name in model:
            np.testing.assert_array_equal(model[name], before[name])

    def test_calf_tangent_field_removes_a_ridge_without_changing_outer_normals(self):
        self.assertTrue(hasattr(surface, '_reconstruct_calf_normals'),
                        'The calf ridge still has only a weak neighbor filter')
        x, y = np.meshgrid(np.linspace(.015, .105, 21), np.linspace(.18, .35, 29))
        z = -.055+6*(x-.055)**2+4*(y-.25)**2
        positions = np.column_stack([x.ravel(), y.ravel(), z.ravel()])
        expected = np.column_stack([12*(x.ravel()-.055), 8*(y.ravel()-.25), -np.ones(x.size)])
        expected /= np.linalg.norm(expected, axis=1)[:, None]
        knee = np.array([.048, .308, .002])
        center = knee[0]+.022+(knee[1]-positions[:, 1])*.15
        core = ((abs(positions[:, 0]-center) < .005) &
                (positions[:, 1] > .23) & (positions[:, 1] < .29))
        normals = expected.copy()
        normals[core] = np.array([.7, .1, -1.])/np.linalg.norm([.7, .1, -1.])
        repaired = surface._reconstruct_calf_normals(positions, normals, knee)
        before_error = np.linalg.norm(normals[core]-expected[core], axis=1).mean()
        after_error = np.linalg.norm(repaired[core]-expected[core], axis=1).mean()
        self.assertLess(after_error, before_error*.25)
        protected = (positions[:, 1] <= .20) | (positions[:, 1] >= .33)
        np.testing.assert_array_equal(repaired[protected], normals[protected])
        np.testing.assert_allclose(np.linalg.norm(repaired, axis=1), 1.)


if __name__ == '__main__':
    unittest.main()
