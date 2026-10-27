"""Skin-only normal smoothing must preserve facial landmarks and geometry."""
import unittest
import numpy as np

class ShadingTests(unittest.TestCase):
    def test_overlapping_orbital_layers_share_one_normal_field(self):
        import native_npc_face_shading as shading
        x,y=np.meshgrid(np.linspace(-.07,.07,15),np.linspace(.78,.91,14))
        z=.09-2*x*x-2*(y-.85)**2
        first=np.column_stack([x.ravel(),y.ravel(),z.ravel()]);count=len(first)
        raised=first.copy()
        raised[:,2]+=.008*np.exp(-((raised[:,0]-.04)/.025)**2-((raised[:,1]-.84)/.025)**2)
        p=np.vstack([first,raised,[[.03,.85,.10],[.04,.85,.10],[.03,.86,.10]]])
        fs=[]
        for offset in (0,count):
            for row in range(13):
                for col in range(14):
                    a=offset+row*15+col;fs.extend(((a,a+1,a+16),(a,a+16,a+15)))
        fs.append((count*2,count*2+1,count*2+2))
        model=dict(source_positions=p,positions=p.copy(),faces=np.asarray(fs),
                   mesh_ids=np.r_[np.zeros(count*2,int),np.ones(3,int)])
        before=p.copy()
        out=shading.twin_face_normal_field(model,np.tile([0.,0.,1.],(len(p),1)))
        np.testing.assert_allclose(out[:count],out[count:count*2],atol=1e-8,
            err_msg='Overlapping orbital skin must not expose a different shading plane')
        self.assertIn('twin_surface_normal_coefficients',model)
        np.testing.assert_array_equal(model['positions'],before)

    def test_twins_skin_field_repairs_split_normals_without_touching_eye_shells(self):
        import native_npc_face_shading as shading
        self.assertTrue(callable(getattr(shading,'twin_skin_normal_field',None)))
        x,y=np.meshgrid(np.linspace(-.06,.06,13),np.linspace(.78,.91,14))
        p=np.column_stack([x.ravel(),y.ravel(),(.09-4*x*x-2*(y-.85)**2).ravel()])
        fs=[]
        for row in range(13):
            for col in range(12):
                a=row*13+col;fs.extend(((a,a+1,a+14),(a,a+14,a+13)))
        n=np.tile([.5,0.,.8660254],(len(p),1));n[p[:,0]<0,0]=-.5
        p=np.vstack([p,[.04,.84,.11]]);n=np.vstack([n,[.1,.2,.9746794]])
        pieces=np.r_[np.zeros(len(p)-1,int),1];before=p.copy()
        result=shading.twin_skin_normal_field(p,n,np.asarray(fs),pieces,0)
        row=7*13
        self.assertLess(np.linalg.norm(result[row+5]-result[row+6]),.15)
        np.testing.assert_array_equal(result[-1],n[-1])
        np.testing.assert_array_equal(p,before)

    def test_forehead_split_normal_softens_without_touching_nose_or_eyes(self):
        import native_npc_face_shading as shading
        p=np.array([[-.01,.91,.06],[0.,.91,.06],[.01,.91,.06],
                    [0.,.85,.10],[.04,.855,.08],[0.,.803,.07]])
        n=np.array([[0.,0.,1.],[.5,0.,.8660254],[0.,0.,1.],
                    [0.,0.,1.],[0.,0.,1.],[0.,0.,1.]])
        before=p.copy();old=n.copy()
        out=shading.soften_face_normals(p,n,np.ones(len(p),bool),'young-malon')
        self.assertLess(out[1,0],old[1,0]-.05)
        np.testing.assert_array_equal(out[3:],old[3:])
        np.testing.assert_array_equal(p,before);np.testing.assert_array_equal(n,old)

    def test_non_skin_samples_and_other_variants_are_preserved(self):
        import native_npc_face_shading as shading
        p=np.array([[0.,.91,.06],[.01,.91,.06]])
        n=np.array([[0.,0.,1.],[.5,0.,.8660254]])
        np.testing.assert_array_equal(shading.soften_face_normals(p,n,np.zeros(2,bool),'twins'),n)
        np.testing.assert_array_equal(shading.soften_face_normals(p,n,np.ones(2,bool),'saria-anime'),n)

    def test_painted_eye_lids_retain_source_topology_and_shading(self):
        import native_npc_face as face
        self.assertTrue(hasattr(face,'source_eye_surface'))
        p=np.array([[-.02,.84,.08],[.02,.84,.08],[0.,.87,.1]])
        n=np.array([[0.,0.,1.],[0.,0.,1.],[0.,.2,.98]])
        fs=np.array([[0,1,2]])
        out=face.source_eye_surface(dict(positions=p,normals=n,source_positions=p),fs,[-.1,.8,.1,.9],14)
        np.testing.assert_array_equal(out['positions'],p)
        np.testing.assert_array_equal(out['normals'],n)
        np.testing.assert_array_equal(out['faces'],fs)

if __name__=='__main__':unittest.main()
