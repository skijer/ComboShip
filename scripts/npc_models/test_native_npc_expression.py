"""A warmer neutral smile must leave the accepted face proportions intact."""
import unittest
import numpy as np
try:
    import native_npc_expression as expression
except ModuleNotFoundError:
    expression = None


class SmileTests(unittest.TestCase):
    def test_only_mouth_corners_lift_without_spreading_the_face(self):
        self.assertIsNotNone(expression, 'The bounded mouth-corner geometry repair is missing')
        points = np.array([[-.018, .815, .06], [.018, .815, .06], [0., .815, .06],
                           [.031, .853, .06], [.08, .815, .06], [.018, .815, -.04]])
        normals = np.tile([0., 0., 1.], (len(points), 1))
        original = points.copy()
        revised, normal = expression.smile_warp(points, normals)
        for i in (0, 1):
            self.assertGreater(revised[i, 1]-points[i, 1], .0035)
            self.assertLessEqual(revised[i, 1]-points[i, 1], .0046)
        np.testing.assert_array_equal(revised[2:], points[2:])
        np.testing.assert_array_equal(revised[:, [0, 2]], points[:, [0, 2]])
        np.testing.assert_array_equal(points, original)
        np.testing.assert_allclose(np.linalg.norm(normal, axis=1), 1.)


if __name__ == '__main__':
    unittest.main()
