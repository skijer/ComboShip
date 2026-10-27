"""Behavior checks for continuous native joint deformation."""
import unittest
import numpy as np
from types import SimpleNamespace
try:
    import native_npc_skin as skin
except ModuleNotFoundError:
    skin = None


class JointSkinTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(skin, 'The export still discards joint blending')

    def test_bent_shared_ring_follows_both_bones(self):
        bind = np.tile(np.eye(4), (2, 1, 1))
        bind[1, 0, 3] = 1
        weights = np.array([[.5, .5], [.5, .5], [1., 0.], [0., 1.]])
        palette = skin.Palette(bind, [0, 1], weights)
        pose = bind.copy()
        pose[1, :3, :3] = [[0, -1, 0], [1, 0, 0], [0, 0, 1]]
        matrices = palette.matrices(pose)
        p = np.array([1., .2, 0., 1.])
        a = matrices[palette.vertex_slots[0]]@p
        b = matrices[palette.vertex_slots[1]]@p
        np.testing.assert_allclose(a, [.9, .1, 0., 1.])
        np.testing.assert_array_equal(a, b)

    def test_palette_roundtrip_and_rest_shape(self):
        bind = np.tile(np.eye(4), (2, 1, 1)); bind[1, 0, 3] = 123
        p = skin.Palette(bind, [0, 1], np.array([[.25, .75], [.75, .25]]))
        restored = skin.decode(p.resource())
        np.testing.assert_allclose(restored.matrices(bind)[2:], np.tile(np.eye(4), (2, 1, 1)))
        with self.assertRaises(ValueError):
            skin.decode(p.resource()[:-2])

    def test_nonmonotonic_native_slots_keep_influence_weights(self):
        bind=np.tile(np.eye(4),(2,1,1))
        palette=skin.Palette(bind,[1,0],np.array([[.25,.75]]))
        restored=skin.decode(palette.resource())
        pose=bind.copy();pose[0,0,3]=4.;pose[1,1,3]=8.
        result=restored.matrices(pose[[1,0]])[palette.vertex_slots[0]]@np.array([0.,0.,0.,1.])
        np.testing.assert_allclose(result,[1.,6.,0.,1.])

    def test_preview_lighting_uses_native_transposed_light_direction(self):
        from preview_native_npc_models import native_shade
        normals=np.array([[1.,0.,0.],[.5,0.,0.]])
        result=native_shade(normals,np.diag([2.,1.,1.]),np.array([1.,1.,0.]))
        np.testing.assert_allclose(result,.45+.55*np.array([1.,.5])*2/np.sqrt(5))

    def test_invalid_weights_fail_instead_of_becoming_rigid(self):
        bind = np.tile(np.eye(4), (2, 1, 1))
        for weights in ([[float('nan'), 0.]], [[-.1, 1.1]], [[0., 0.]]):
            with self.assertRaises(ValueError):
                skin.Palette(bind, [0, 1], np.array(weights))

    def test_contact_measures_triangle_surface_and_detects_penetration(self):
        from native_npc_contact import clearance
        p=np.array([[0.,0.,0.],[1.,0.,0.],[0.,1.,0.],[0.,0.,1.]])
        faces=np.array([[0,2,1],[0,1,3],[0,3,2],[1,2,3]])
        distance=clearance(np.array([[.1,.1,.1],[-.2,.1,.1],[0.,.1,.1]]),p[faces])
        np.testing.assert_allclose(distance,[-.1,.2,0.],atol=1e-8)
        from native_npc_contact import surface_crossings
        cut=np.array([[[-.1,.2,.2],[.8,.2,.2],[.2,.8,.2]],
                      [[2.,2.,2.],[3.,2.,2.],[2.,3.,2.]]])
        np.testing.assert_array_equal(surface_crossings(cut,p[faces]),[0])
        coplanar=np.array([[[.1,.1,0.],[.6,.1,0.],[.1,.6,0.]],
                          [[2.,2.,0.],[3.,2.,0.],[2.,3.,0.]]])
        np.testing.assert_array_equal(surface_crossings(coplanar,p[faces]),[0])

    def test_independent_fingers_curl_towards_palm_and_keep_space(self):
        from native_npc_fit import source_hand_pose
        names, world, parent, nodes = [], {}, {}, []
        for side, sign in [('Left', 1), ('Right', -1)]:
            hand = len(names); names.append(side+'Hand'); parent[hand] = None
            world[hand] = np.eye(4); nodes.append({'name': names[-1]})
            for finger, z in [('Index', .02), ('Middle', 0), ('Ring', -.02), ('Pinky', -.04), ('Thumb', .04)]:
                previous = hand
                for k in range(1, 5):
                    i = len(names); names.append(side+'Hand'+finger+str(k)); parent[i] = previous
                    world[i] = np.eye(4); world[i][:3, 3] = [sign*(.03+k*.02), 0., z]
                    nodes.append({'name': names[-1]}); previous = i
        source = SimpleNamespace(names=names, joints=list(range(len(names))), world=world, parent=parent,
                                 inverse=[np.linalg.inv(world[i]) for i in range(len(names))], g={'nodes': nodes})
        posed = source_hand_pose(source, .85, separated=True)
        for side in ['Left', 'Right']:
            j = names.index(side+'HandMiddle4')
            tip = posed[j]@world[j][:, 3]
            self.assertLess(tip[1], -.01, side+' fingers bend away from the palm')
        offset=np.array([.01,.02,-.01])
        fitted=source_hand_pose(source,.85,separated=True,grip={
            'Left':{'offset':offset,'curls':{'Index':.2}}})
        for name in ('LeftHand','LeftHandMiddle4','RightHand','RightHandIndex4'):
            j=names.index(name)
            np.testing.assert_allclose(fitted[j][:3,:3],posed[j][:3,:3])
            delta=offset if name.startswith('Left') else np.zeros(3)
            np.testing.assert_allclose(fitted[j][:3,3],posed[j][:3,3]+delta,atol=1e-9)
        j=names.index('LeftHandIndex4')
        self.assertGreater(np.linalg.norm(fitted[j][:3,:3]-posed[j][:3,:3]),.1)

    def test_welded_attachment_copies_receive_identical_weights(self):
        import native_npc_models as compiler
        from test_native_npc_models import proportion_fixture
        source, rig = proportion_fixture()
        model = compiler.fit_model(source, rig)
        model['positions'][-2] = model['positions'][-3]
        weights = skin.joint_weights(source, model, rig)
        np.testing.assert_array_equal(weights[-2], weights[-3])

    def test_authored_rigid_cloth_keeps_its_fitted_surface(self):
        import native_npc_models as compiler
        from test_native_npc_models import proportion_fixture
        source,rig=proportion_fixture();model=compiler.fit_model(source,rig)
        expected=model['positions'][-3:].copy()
        attached=np.full(len(model['positions']),-1,int)
        attached[-3:]=rig.profile['chest'];model['rigid_attachments']=attached
        palette=skin.prepare(source,model,rig)
        self.assertTrue((palette.vertex_slots[-3:]==palette.slots.index(rig.profile['chest'])).all())
        np.testing.assert_allclose(palette.geometry_for(model).rest_positions[-3:],expected)

    def test_authored_sleeve_weights_override_the_generic_shoulder_band(self):
        import native_npc_models as compiler
        from test_native_npc_models import proportion_fixture
        source,rig=proportion_fixture();model=compiler.fit_model(source,rig)
        baseline=skin.joint_weights(source,model,rig).copy()
        chosen=np.zeros(len(baseline),bool);chosen[-3:]=True
        authored=np.zeros_like(baseline);chest=rig.profile['chest'];arm=rig.profile['left'][3]
        authored[chosen,chest]=.75;authored[chosen,arm]=.25
        model['authored_joint_mask']=chosen;model['authored_joint_weights']=authored
        actual=skin.joint_weights(source,model,rig)
        np.testing.assert_array_equal(actual[chosen],authored[chosen])
        np.testing.assert_array_equal(actual[~chosen],baseline[~chosen])
        self.assertTrue(model['skin_joint_mask'][chosen].all())

    def test_forehead_normals_follow_a_smooth_surface(self):
        from native_npc_fit import smooth_twin_face
        from native_npc_face_shading import twin_face_normal_field
        xx, yy = np.meshgrid(np.linspace(-.07,.07,12),np.linspace(.875,.905,8))
        p=np.column_stack([xx.ravel(),yy.ravel(),(.08-2*xx*xx-(yy-.89)**2).ravel()])
        count=len(p)
        overlay=p.copy();overlay[:,2]+=.006
        p=np.concatenate([p,overlay])
        model=dict(source_positions=p,positions=p.copy(),normals=np.tile([1.,0.,0.],(len(p),1)),
                   bones=np.zeros(len(p),int),mesh_ids=np.repeat([0,1],count),
                   head_transform=np.column_stack([np.eye(3),np.zeros(3)]))
        faces=[]
        for row in range(7):
            for col in range(11):
                a=row*12+col;faces.extend(((a,a+1,a+13),(a,a+13,a+12)))
        model['faces']=np.asarray(faces)
        smooth_twin_face(model,0,np.tile([210.,175.,125.],(len(p),1)))
        model['normals']=twin_face_normal_field(model,model['normals'])
        self.assertTrue((model['normals'][:,2]>.9).all())
        np.testing.assert_allclose(model['normals'][:count],model['normals'][count:],atol=1e-8)
        forehead=p[:,1]>.89
        np.testing.assert_array_equal(model['positions'][forehead],p[forehead])
        np.testing.assert_array_equal(model['source_positions'],p)

    def test_joint_blend_uses_each_source_to_bone_map_once(self):
        import native_npc_models as compiler
        from test_native_npc_models import proportion_fixture
        source,rig=proportion_fixture()
        rig.frames[0,rig.profile['left'][3]+1,2]=-8192
        rig.pose(rig.frames[0])
        model=compiler.fit_model(source,rig);pal=skin.prepare(source,model,rig)
        native_count=len(pal.slots)
        ids=np.flatnonzero(pal.vertex_slots>=native_count);i=int(ids[0]);slot=pal.vertex_slots[i]
        a,b,q=pal.blends[slot-native_count];wa,wb=(16-q)/16,q/16
        bind=pal.bind.copy();pose=bind.copy();arm=rig.profile['left'][3]
        rotation=np.array([[0.,-1,0],[1,0,0],[0,0,1]])
        pose[arm,:3,:3]=rotation@pose[arm,:3,:3]
        named={n:source.world[j][:3,3] for n,j in zip(source.names,source.joints)}
        body=np.eye(4);body[:3]=model['head_transform'];body[:3,3]=bind[rig.profile['hips'],:3,3]-body[:3,:3]@named['Hips']
        u=compiler.unit(named['LeftForeArm']-named['LeftArm']);fore=rig.profile['left'][4]
        ratio=np.linalg.norm(bind[fore,:3,3]-bind[arm,:3,3])/np.linalg.norm(named['LeftForeArm']-named['LeftArm'])
        scale=float(model['scale'])
        aa=compiler.align(u,bind[fore,:3,3]-bind[arm,:3,3])@(scale*np.eye(3)+(ratio-scale)*np.outer(u,u))
        arm_map=np.eye(4);arm_map[:3,:3]=aa;arm_map[:3,3]=bind[arm,:3,3]-aa@named['LeftArm']
        point=np.r_[model['source_positions'][i],1.]
        maps={rig.profile['chest']:body,arm:arm_map}
        expected=sum(weight*(pose[bone]@np.linalg.inv(bind[bone])@maps[bone]@point)
                     for weight,bone in [(wa,pal.slots[a]),(wb,pal.slots[b])])
        encoded=getattr(pal,'vertex_positions',model['positions'])[i]
        actual=pal.matrices(pose)[slot]@np.r_[encoded,1.]
        np.testing.assert_allclose(actual,expected,atol=1e-4)

    def test_rest_space_blends_share_the_rigid_endpoint_normal_basis(self):
        # Nonuniform source retargeting must not enter the native normal
        # matrix only for q=1..15 and disappear abruptly at a rigid endpoint.
        bind=np.tile(np.eye(4),(2,1,1));weights=np.array([[.5,.5]])
        p=skin.Palette(bind,[0,1],weights);p.rest_space=True
        p.canonical=np.eye(4);p.seam_groups=[];p.skin_weights=weights;p.skin_mask=np.array([True])
        p.transforms=bind.copy();p.transforms[1,:3,:3]=np.diag([.4,1.8,1.])
        model=dict(source_positions=np.array([[1.,2.,3.]]),positions=np.array([[.7,2.8,3.]]),
                   normals=np.array([[.6,.8,0.]]),posed_source_normals=np.array([[.6,.8,0.]]))
        out=p.geometry_for(model);matrix=p.matrices(bind)[p.vertex_slots[0]]
        np.testing.assert_allclose(matrix@np.r_[out.vertex_positions[0],1],np.r_[out.rest_positions[0],1])
        actual=matrix[:3,:3]@out.vertex_normals[0];actual/=np.linalg.norm(actual)
        np.testing.assert_allclose(actual,out.rest_normals[0],atol=1e-8)


if __name__ == '__main__':
    unittest.main()
