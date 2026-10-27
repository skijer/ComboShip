"""The visible rear-shorts fin is removed without changing accepted surfaces."""
import copy
import hashlib
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import numpy as np

try:
    import native_npc_leg as leg
except ModuleNotFoundError:
    leg = None


# Hand-audited complete fin, including the neck; hidden debris starts at 2120.
VISIBLE_FIN_FACES = list(range(2068, 2120))
CAP_FACES = [[1158, 1175, 1162], [1158, 1162, 1120],
             [1158, 1120, 1105], [1158, 1105, 1116]]
HEM_RING = [1158, 1175, 1162, 1120, 1105, 1116]


def fin_fixture():
    """Give the visible fin and hidden debris the same below-hem height."""
    faces = np.arange(2304*3).reshape(-1, 3)
    positions = np.zeros((2304*3, 3))
    positions[:, 0] = -.063
    positions[:, 1] = .46
    positions[:, 2] = -.05
    positions[faces[VISIBLE_FIN_FACES].ravel(), 1] = .442
    below_hem = [2073, 2074, 2075, 2076, 2077, 2078, 2079, 2080, 2081, 2082,
                 2083, 2084, 2085, 2086, 2087, 2088, 2089, 2090, 2091, 2092,
                 2093, 2094, 2095, 2097, 2098, 2099, 2100, 2101, 2102, 2103,
                 2104, 2105, 2106, 2107, 2108, 2109, 2110, 2112, 2116, 2117,
                 2118, 2119]
    positions[faces[below_hem].ravel(), 1] = .42
    positions[faces[2120:].ravel(), 1] = .42
    positions[faces[2120:].ravel(), 2] = 0.
    # Actual audited root coordinates: their fan closes the downward hem.
    positions[HEM_RING] = [
        [-.06404094, .44906622, -.05957795],
        [-.06219468, .44870001, -.06095886],
        [-.05991734, .44854742, -.05987166],
        [-.05985249, .44866949, -.05517960],
        [-.06179794, .44903570, -.05424119],
        [-.06372815, .44915777, -.05503847],
    ]
    unrelated = np.array([[0., .70, .03], [.01, .70, .03], [0., .71, .03]])
    meshes = [dict(name='unrelated', positions=unrelated,
                   faces=np.array([[0, 1, 2]]))]
    meshes.extend(dict(name=f'piece{i}', positions=np.empty((0, 3)),
                       faces=np.empty((0, 3), int)) for i in range(1, 21))
    meshes.append(dict(name='tripo_part_6', positions=positions, faces=faces))
    source = SimpleNamespace(raw=b'traditional shorts fin fixture', meshes=meshes)
    p = np.concatenate([unrelated, positions])
    model = dict(source_positions=p.copy(), posed_source_positions=p.copy(),
                 positions=p*10, normals=np.tile([0., 0., -1.], (len(p), 1)),
                 posed_source_normals=np.tile([0., 0., -1.], (len(p), 1)),
                 uv=np.arange(len(p)*2).reshape(-1, 2)/10000.,
                 weights=np.tile([.25, .75], (len(p), 1)),
                 mesh_ids=np.array([0]*3+[21]*len(positions)),
                 bones=np.zeros(len(p), int),
                 faces=np.concatenate([np.array([[0, 1, 2]]), faces+3]),
                 hosts=np.arange(2305) % 16,
                 removed_segment_faces=np.array([99999]))
    return source, model, SimpleNamespace(name='saria')


class TraditionalRearShortsFinTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(leg, 'The source-gated visible rear-shorts fin repair is missing')

    def test_removes_only_the_visible_fin_and_preserves_hidden_debris_and_vertex_fields(self):
        source, model, rig = fin_fixture()
        before = copy.deepcopy(model)
        source_before = copy.deepcopy(source.meshes)
        with patch.object(leg, 'TRADITIONAL_SOURCE_SHA256', hashlib.sha256(source.raw).hexdigest()):
            report = leg.remove_traditional_saria_leg_fin(source, model, rig)
        removed_rows = np.array(VISIBLE_FIN_FACES)+1
        expected_faces = np.concatenate([np.delete(before['faces'], removed_rows, axis=0),
                                         np.asarray(CAP_FACES)+3])
        expected_hosts = np.r_[np.delete(before['hosts'], removed_rows), [0]*4]
        np.testing.assert_array_equal(model['faces'], expected_faces)
        np.testing.assert_array_equal(model['hosts'], expected_hosts)
        for key in before:
            if key not in ('faces', 'hosts'):
                np.testing.assert_array_equal(model[key], before[key], err_msg=key)
        np.testing.assert_array_equal(model['faces'][-188:-4], before['faces'][-184:])
        for original, current in zip(source_before, source.meshes):
            np.testing.assert_array_equal(current['positions'], original['positions'])
            np.testing.assert_array_equal(current['faces'], original['faces'])
        self.assertEqual(report['source_mesh_id'], 21)
        self.assertEqual(report['source_mesh_name'], 'tripo_part_6')
        self.assertEqual(report['removed_source_mesh_faces'], VISIBLE_FIN_FACES)
        self.assertEqual(report['removed_source_faces'], removed_rows.tolist())
        self.assertEqual(report['removed_triangles'], 52)
        self.assertEqual(report['added_triangles'], 4)
        self.assertEqual(report['added_source_mesh_vertex_triangles'], CAP_FACES)
        self.assertEqual(report['added_source_vertex_triangles'], (np.asarray(CAP_FACES)+3).tolist())
        self.assertTrue(report['vertices_unchanged'])
        self.assertTrue(report['uvs_unchanged'])
        self.assertTrue(report['weights_unchanged'])
        self.assertTrue(report['normals_unchanged'])

    def test_other_source_and_other_rig_are_noops_before_model_fields_are_required(self):
        model = dict(protected=np.array([1., 2., 3.]))
        before = copy.deepcopy(model)
        self.assertIsNone(leg.remove_traditional_saria_leg_fin(
            SimpleNamespace(raw=b'unrelated'), model, SimpleNamespace(name='saria')))
        self.assertIsNone(leg.remove_traditional_saria_leg_fin(
            SimpleNamespace(), model, SimpleNamespace(name='twins')))
        self.assertEqual(set(model), set(before))
        np.testing.assert_array_equal(model['protected'], before['protected'])

    def test_changed_visible_mask_fails_before_any_model_mutation(self):
        source, model, rig = fin_fixture()
        source.meshes[21]['positions'][source.meshes[21]['faces'][2067], 1] = .42
        before = copy.deepcopy(model)
        with patch.object(leg, 'TRADITIONAL_SOURCE_SHA256', hashlib.sha256(source.raw).hexdigest()):
            with self.assertRaises(ValueError):
                leg.remove_traditional_saria_leg_fin(source, model, rig)
        for key in before:
            np.testing.assert_array_equal(model[key], before[key], err_msg=key)

    def test_cap_restores_exact_six_edge_ring_with_downward_winding(self):
        source, model, rig = fin_fixture()
        with patch.object(leg, 'TRADITIONAL_SOURCE_SHA256', hashlib.sha256(source.raw).hexdigest()):
            report = leg.remove_traditional_saria_leg_fin(source, model, rig)
        cap = model['faces'][-4:]-3
        p = source.meshes[21]['positions']
        normal = np.cross(p[cap[:, 1]]-p[cap[:, 0]], p[cap[:, 2]]-p[cap[:, 0]])
        self.assertTrue((normal[:, 1] < 0).all(), 'The underside cap must face downward')
        edges = np.sort(np.concatenate([cap[:, [0, 1]], cap[:, [1, 2]], cap[:, [2, 0]]]), axis=1)
        edge, count = np.unique(edges, axis=0, return_counts=True)
        boundary = {tuple(e) for e in edge[count == 1]}
        self.assertEqual(boundary, {(1158, 1175), (1162, 1175), (1120, 1162),
                                    (1105, 1120), (1105, 1116), (1116, 1158)})
        self.assertTrue((count <= 2).all())
        self.assertEqual(report['hem_ring_source_mesh_vertices'], HEM_RING)

    def test_changed_shorts_face_order_fails_before_any_model_mutation(self):
        source, model, rig = fin_fixture()
        model['faces'][[2074, 2075]] = model['faces'][[2075, 2074]]
        before = copy.deepcopy(model)
        with patch.object(leg, 'TRADITIONAL_SOURCE_SHA256', hashlib.sha256(source.raw).hexdigest()):
            with self.assertRaises(ValueError):
                leg.remove_traditional_saria_leg_fin(source, model, rig)
        for key in before:
            np.testing.assert_array_equal(model[key], before[key], err_msg=key)


if __name__ == '__main__':
    unittest.main()
