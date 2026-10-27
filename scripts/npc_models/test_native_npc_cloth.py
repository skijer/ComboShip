"""A shared cloth shading field must preserve shape and native normal bases."""
import copy
import hashlib
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import numpy as np

try:
    import native_npc_cloth as cloth
except ModuleNotFoundError:
    cloth = None


def cloth_fixture():
    x, y = np.meshgrid(np.linspace(-.09, .09, 13), np.linspace(.58, .78, 17))
    plane = np.column_stack([x.ravel(), y.ravel(), (.05+.1*x+.08*(y-.68)).ravel()])
    faces = []
    for row in range(16):
        for column in range(12):
            a = row*13+column
            faces += [[a, a+1, a+14], [a, a+14, a+13]]
    faces = np.asarray(faces)
    count = len(plane)
    lower = plane.copy(); lower[:, 2] -= .001
    protected = np.array([[0., .86, .06], [0., .64, -.05], [.15, .74, .03],
                          [0., .73, .04], [0., .79, .02]])
    p = np.concatenate([plane, lower, protected])
    n = np.tile([-.1, -.08, 1.], (len(p), 1))
    n /= np.linalg.norm(n, axis=1)[:, None]
    waist = (p[:count, 1] > .625) & (p[:count, 1] < .655)
    n[:count][waist] = [0., 1., 0.]
    n[count:2*count][waist] = [0., -.6, -.8]
    n[-5:] = [[1., 0., 0.], [0., 0., -1.], [1., 0., 0.], [0., 1., 0.], [1., 0., 0.]]
    maps = np.tile(np.eye(4), (16, 1, 1))
    maps[8, :3, :3] = np.diag([2., 3., 4.])
    maps[9, :3, :3] = [[2., .3, 0.], [0., 1., 0.], [0., 0., 3.]]
    weights = np.zeros((len(p), 16)); weights[:, 8] = 1.
    middle = count//2
    weights[middle, 8:10] = [.25, .75]
    model = dict(source_positions=p.copy(), posed_source_positions=p.copy(),
                 positions=p*10, normals=n.copy(), posed_source_normals=n.copy(),
                 mesh_ids=np.array([3]*count+[20]*count+[6, 3, 24, 16, 24]),
                 bones=np.array([8]*(2*count)+[15, 8, 8, 8, 15]), weights=weights,
                 uv=np.arange(len(p)*2).reshape(-1, 2)/1000.,
                 faces=np.concatenate([faces, faces+count]), source_bone_transforms=maps,
                 scale=np.array(10.))
    return SimpleNamespace(raw=b'traditional cloth regression fixture'), model, SimpleNamespace(
        name='saria', profile=dict(chest=8, head=15)), count, waist


class TraditionalClothNormalTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(cloth, 'The source-gated traditional cloth normal field is missing')

    def test_overlapping_waist_shells_share_shading_without_geometry_or_uv_changes(self):
        source, model, rig, count, waist = cloth_fixture()
        before = copy.deepcopy(model)
        with patch.object(cloth, 'RENDER_SOURCE_SHA256', hashlib.sha256(source.raw).hexdigest()):
            report = cloth.repair_traditional_cloth_normals(source, model, rig)
        self.assertTrue(report['normals_only'])
        self.assertGreater(report['front_vertices'], 0)
        revised = model['posed_source_normals']
        np.testing.assert_allclose(revised[:count], revised[count:2*count], atol=1e-12)
        self.assertTrue((revised[:count][waist, 2] > .9).all(),
                        'Open cut-edge normals must not leave a jagged dark waist join')
        expected = np.array([-.1, -.08, 1.]); expected /= np.linalg.norm(expected)
        np.testing.assert_allclose(revised[count//2], expected, atol=.003)
        for key in before:
            if key not in ('normals', 'posed_source_normals'):
                np.testing.assert_array_equal(model[key], before[key], err_msg=key)
        np.testing.assert_array_equal(revised[-5:], before['posed_source_normals'][-5:])
        np.testing.assert_array_equal(model['normals'][-5:], before['normals'][-5:])

    def test_fitted_normals_use_the_same_covector_maps_as_the_skin_palette(self):
        source, model, rig, count, _ = cloth_fixture()
        with patch.object(cloth, 'RENDER_SOURCE_SHA256', hashlib.sha256(source.raw).hexdigest()):
            cloth.repair_traditional_cloth_normals(source, model, rig)
        index = count//2
        source_normal = model['posed_source_normals'][index]
        expected = sum(model['weights'][index, bone] *
                       (source_normal@np.linalg.inv(model['source_bone_transforms'][bone, :3, :3]))
                       for bone in (8, 9))
        expected /= np.linalg.norm(expected)
        np.testing.assert_allclose(model['normals'][index], expected, atol=1e-12)
        np.testing.assert_allclose(np.linalg.norm(model['normals'][:2*count], axis=1), 1.)

    def test_other_source_is_a_complete_noop_before_any_model_field_is_required(self):
        model = dict(normals=np.array([[0., 0., 1.]]))
        original = model['normals'].copy()
        self.assertIsNone(cloth.repair_traditional_cloth_normals(
            SimpleNamespace(raw=b'an unrelated source'), model, SimpleNamespace(name='saria')))
        np.testing.assert_array_equal(model['normals'], original)
        self.assertEqual(set(model), {'normals'})


if __name__ == '__main__':
    unittest.main()
