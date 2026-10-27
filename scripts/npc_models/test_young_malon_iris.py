import unittest
import numpy as np
from refine_young_malon_iris import iris_depth

class IrisDepthTests(unittest.TestCase):
    def test_removes_interior_depth_ridge_without_moving_xy_or_seam(self):
        uv=np.array([[0,0],[1,0],[1,1],[0,1],[.5,.5]],float)
        p=np.column_stack([uv,np.array([1,2,3,4,9.])]);curve=np.array([.08,0,0,0,0,0])
        out=iris_depth(p,uv,uv[:4],curve,0)
        np.testing.assert_array_equal(out[:4],p[:4])
        np.testing.assert_array_equal(out[:,:2],p[:,:2])
        self.assertAlmostEqual(out[-1,2],.08)
        np.testing.assert_array_equal(p[:,-1],[1,2,3,4,9])

if __name__=='__main__':unittest.main()
