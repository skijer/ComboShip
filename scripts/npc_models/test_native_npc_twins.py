"""Regression coverage for the twins' preserved face and joint attachments."""
import unittest
import numpy as np
from native_npc_fit import smooth_twin_face

class TwinsFaceTests(unittest.TestCase):
    def test_normal_repair_preserves_source_face_depth(self):
        # A curved face plus two eye regions and the forehead support ring.
        x,y=np.meshgrid(np.linspace(-.075,.075,25),np.linspace(.795,.915,25))
        z=.09-2*x*x-1.5*(y-.855)**2+.014*np.exp(-((x-.04)/.018)**2-((y-.84)/.019)**2)
        p=np.column_stack([x.ravel(),y.ravel(),z.ravel()])
        n=np.tile([0.,0.,1.],(len(p),1));rgb=np.tile([215.,180.,130.],(len(p),1))
        model=dict(source_positions=p,positions=p.copy(),normals=n,bones=np.full(len(p),14),
                   mesh_ids=np.zeros(len(p),int),head_transform=np.column_stack([np.eye(3),np.zeros(3)]))
        before=model['positions'].copy()
        smooth_twin_face(model,14,rgb)
        np.testing.assert_array_equal(model['positions'],before,
            'A forehead normal repair must not flatten the face or open-eye shells')

    def test_blink_curve_fit_preserves_neutral_head_normals(self):
        # A smooth source head has its own orbital and cheek curvature. Blink
        # support polynomials must not replace that visible normal field.
        x,y=np.meshgrid(np.linspace(-.075,.075,25),np.linspace(.795,.915,25))
        orbital=np.exp(-((x-.04)/.018)**2-((y-.84)/.019)**2)
        z=.09-2*x*x-1.5*(y-.855)**2+.014*orbital
        p=np.column_stack([x.ravel(),y.ravel(),z.ravel()])
        n=np.column_stack([(4*x+.028*(x-.04)/.018**2*orbital).ravel(),
                           (3*(y-.855)+.028*(y-.84)/.019**2*orbital).ravel(),np.ones(x.size)])
        n/=np.linalg.norm(n,axis=1)[:,None]
        model=dict(source_positions=p,positions=p.copy(),normals=n.copy(),bones=np.full(len(p),14),
                   mesh_ids=np.zeros(len(p),int),head_transform=np.column_stack([np.eye(3),np.zeros(3)]))
        smooth_twin_face(model,14,np.tile([215.,180.,130.],(len(p),1)))
        np.testing.assert_array_equal(model['normals'],n)
        self.assertIn('twin_eye_curve_L',model)
        self.assertIn('twin_eye_curve_R',model)

    def test_armhole_contact_is_fixed_while_outer_cap_blends_to_arm(self):
        import native_npc_twins as repair
        self.assertTrue(hasattr(repair,'shoulder_blend'))
        points=np.array([[0.,0.,0.],[.02,0.,0.],[.045,0.,0.],[.10,0.,0.]])
        t=repair.shoulder_blend(points,np.zeros(3),np.array([.2,0.,0.]),np.array([[0.,0.,0.]]))
        self.assertEqual(t[0],0.)
        self.assertTrue(0<t[1]<t[2]<=t[3])
        self.assertEqual(t[-1],1.)

    def test_closed_lid_fills_aperture_without_retriangulating_visible_skin(self):
        import native_npc_face as face
        self.assertTrue(hasattr(face,'make_aperture_lid'))
        # An annular skin patch with an actual four-edge eye aperture.
        p=np.array([[-2,-2,.1],[2,-2,.1],[2,2,.1],[-2,2,.1],
                    [-1,-1,.1],[1,-1,.1],[1,1,.1],[-1,1,.1]],float)
        fs=np.array([[0,1,5],[0,5,4],[1,2,6],[1,6,5],[2,3,7],[2,7,6],[3,0,4],[3,4,7]])
        model=dict(source_positions=p,positions=p.copy(),normals=np.tile([0.,0.,1.],(8,1)),
                   mesh_ids=np.zeros(8,int),faces=fs,head_transform=np.column_stack([np.eye(3),np.zeros(3)]))
        lid=face.make_aperture_lid(model,fs,0,(),(0.,0.),np.array([.1,0,0,0,0,0]),[-2,-2,2,2],14)
        np.testing.assert_array_equal(lid['positions'][:8],p)
        np.testing.assert_array_equal(lid['faces'][:len(fs)],fs)
        self.assertGreater(len(lid['faces']),len(fs))
        # Original perimeter edges remain whole after native integer rounding.
        edges={tuple(sorted((int(a),int(b)))) for f in lid['faces'] for a,b in [(f[0],f[1]),(f[1],f[2]),(f[2],f[0])]}
        self.assertTrue({(4,5),(5,6),(6,7),(4,7)}<=edges)

    def test_blink_keeps_aperture_and_internal_brow_joins_welded(self):
        import native_npc_face as face
        p=np.array([[-.02,.80,.05],[.10,.80,.05],[.10,.92,.05],[-.02,.92,.05],
                    [.02,.822,.085],[.06,.822,.085],[.06,.858,.085],[.02,.858,.085]])
        fs=np.array([[0,1,5],[0,5,4],[1,2,6],[1,6,5],[2,3,7],[2,7,6],[3,0,4],[3,4,7]])
        # A retained brow is welded to an interior skin vertex, with all its
        # triangles inside the expression patch. It is not an exterior seam.
        middle=p[fs[0]].mean(0)
        p=np.vstack([p,middle,middle,middle+[.003,0,0],middle+[0,.003,0]])
        fs=np.vstack([[[0,1,8],[1,5,8],[5,0,8]],fs[1:],[[9,10,11]]])
        free=fs[5].copy();p=np.vstack([p,p[free].mean(0)])
        fs=np.vstack([fs[:5],[[free[0],free[1],12],[free[1],free[2],12],[free[2],free[0],12]],fs[6:]])
        model=dict(source_positions=p,positions=p.copy(),normals=np.tile([0.,0.,1.],(13,1)),
                   mesh_ids=np.array([0]*9+[1]*3+[0]),faces=fs,
                   head_transform=np.column_stack([np.eye(3),np.zeros(3)]))
        out=face.make_aperture_lid(model,fs,0,(),(.04,.84),np.array([.055,1.,0,0,0,0]),
                                   [-.02,.80,.10,.92],14)
        np.testing.assert_array_equal(out['positions'][4:8],p[4:8],
                                      'Blink must retain the real aperture boundary')
        np.testing.assert_array_equal(out['positions'][8],p[8],
                                      'Internal skin/brow welds must not open during a blink')
        np.testing.assert_array_equal(out['positions'][8],out['positions'][9])
        np.testing.assert_array_equal(out['positions'][:len(p)],p,
                                      'Fill the aperture without deforming retained skin')
        np.testing.assert_allclose(out['normals'],np.tile([0.,0.,1.],(len(out['normals']),1)),
                                   err_msg='Generated lid shading must continue its real rim field')
        np.testing.assert_array_equal(model['positions'],p)

if __name__=='__main__':unittest.main()
