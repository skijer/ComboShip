"""The cap must follow real armhole edges, including concave corners."""
import unittest
import numpy as np
from refine_native_npc_shoulders import perimeter_angles


class ShoulderBoundaryTests(unittest.TestCase):
    def test_concave_armhole_preserves_every_boundary_edge(self):
        points=np.array([[2,0],[1.8,1],[.5,.2],[1,1.8],[0,2],[-2,0],[0,-2]],float)
        groups=np.arange(len(points))+10
        graph={int(g):{int(groups[(i-1)%len(groups)]),int(groups[(i+1)%len(groups)])}
               for i,g in enumerate(groups)}
        approximate=np.arctan2(points[:,1],points[:,0])
        angles=perimeter_angles(points,groups,graph,approximate)
        order=groups[np.argsort(angles)]
        actual={tuple(sorted((int(a),int(b)))) for a,b in zip(order,np.roll(order,-1))}
        expected={tuple(sorted((a,b))) for a,neighbors in graph.items() for b in neighbors}
        self.assertEqual(actual,expected)
        self.assertTrue(np.all(np.diff(np.sort(angles))>0))

    def test_pinched_corner_is_rejected(self):
        graph={0:{1,2,3},1:{0,2},2:{0,1},3:{0,4},4:{0,3}}
        with self.assertRaisesRegex(ValueError,'simple closed boundary'):
            perimeter_angles(np.zeros((5,3)),np.arange(5),graph,np.arange(5,dtype=float))

    def test_disconnected_armhole_is_rejected(self):
        graph={i:{(i//3)*3+(i+1)%3,(i//3)*3+(i+2)%3} for i in range(6)}
        with self.assertRaisesRegex(ValueError,'disconnected'):
            perimeter_angles(np.zeros((6,3)),np.arange(6),graph,np.arange(6,dtype=float))


if __name__=='__main__':
    unittest.main()
