"""Regressions for audited distal finger influences in the supplied Saria GLBs."""
from pathlib import Path
from types import SimpleNamespace
import unittest

import numpy as np

try:
    from native_npc_hand_weights import repair_saria_hand_weights
except ModuleNotFoundError:
    repair_saria_hand_weights = None

from native_npc_models import SourceModel, unit
from native_npc_fit import source_hand_pose


NATURAL_CURLS = dict(Index=.64, Middle=.59, Ring=.54, Pinky=.56, Thumb=.72)


def posed(source, mesh, piece, skin):
    p = np.zeros_like(mesh["positions"], dtype=float)
    n = np.zeros_like(mesh["normals"], dtype=float)
    for slot in range(piece["joints"].shape[1]):
        for joint in np.unique(piece["joints"][:, slot]):
            ids = np.flatnonzero(piece["joints"][:, slot] == joint)
            weight = piece["weights"][ids, slot, None]
            p[ids] += (mesh["positions"][ids] @ skin[joint][:3, :3].T + skin[joint][:3, 3]) * weight
            n[ids] += (mesh["normals"][ids] @ skin[joint][:3, :3].T) * weight
    return p, n


def normal_angles(p, n, faces):
    geometric = unit(np.cross(p[faces[:, 1]] - p[faces[:, 0]], p[faces[:, 2]] - p[faces[:, 0]]))
    supplied = unit(n[faces].mean(axis=1))
    return np.rad2deg(np.arccos(np.clip((geometric * supplied).sum(axis=1), -1, 1)))


class HandWeightCopyTests(unittest.TestCase):
    def test_other_source_returns_independent_unchanged_influence_arrays(self):
        self.assertIsNotNone(repair_saria_hand_weights)
        joints = np.array([[1, 2, 3, 4]])
        weights = np.array([[.1, .2, .3, .4]], np.float32)
        source = SimpleNamespace(raw=b"unrecognized source", meshes=[dict(joints=joints, weights=weights)])
        pieces, report = repair_saria_hand_weights(source)
        np.testing.assert_array_equal(pieces[0]["joints"], joints)
        np.testing.assert_array_equal(pieces[0]["weights"], weights)
        self.assertFalse(report["applied"])
        self.assertFalse(np.shares_memory(pieces[0]["joints"], joints))
        self.assertFalse(np.shares_memory(pieces[0]["weights"], weights))


class SuppliedHandWeightTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = Path(__file__).resolve().parents[3] / "upload"
        files = {"anime": "Saria+Anime+custom+npc+++player+model.glb", "traditional": "Saria+Render+GLB.glb"}
        if any(not (root / name).exists() for name in files.values()):
            raise unittest.SkipTest("Supplied Saria sources are unavailable")
        cls.sources = {key: SourceModel(root / name) for key, name in files.items()}

    def setUp(self):
        self.assertIsNotNone(repair_saria_hand_weights)

    def test_diagnosed_middle_tip_keeps_middle_joint_blend_and_rejects_ring_mass(self):
        source = self.sources["anime"]
        pieces, _ = repair_saria_hand_weights(source)
        mesh, repaired, vertex = source.meshes[4], pieces[4], 632
        names = np.array([source.names[joint] for joint in mesh["joints"][vertex]])
        own = np.char.startswith(names, "LeftHandMiddle")
        foreign = np.char.startswith(names, "LeftHandRing")
        self.assertGreater(mesh["weights"][vertex, foreign].sum(), .60)
        self.assertLess(repaired["weights"][vertex, foreign].sum(), 1e-7)
        np.testing.assert_allclose(
            repaired["weights"][vertex, own] / repaired["weights"][vertex, own].sum(),
            mesh["weights"][vertex, own] / mesh["weights"][vertex, own].sum(), atol=1e-7)
        self.assertGreater(np.count_nonzero(repaired["weights"][vertex, own]), 1)

    def test_natural_grip_fixes_the_diagnosed_wedge_without_new_opposed_faces(self):
        for variant, hand_ids in (("anime", (2, 4)), ("traditional", (4, 8))):
            source = self.sources[variant]
            pieces, _ = repair_saria_hand_weights(source)
            for scale in (.8, 1., 1.15):
                grip = {side: {"curls": {key: value * scale for key, value in NATURAL_CURLS.items()}}
                        for side in ("Left", "Right")}
                skin = source_hand_pose(source, .85, separated=True, grip=grip)
                for mesh_id in hand_ids:
                    with self.subTest(variant=variant, mesh_id=mesh_id, scale=scale):
                        mesh = source.meshes[mesh_id]
                        old = normal_angles(*posed(source, mesh, mesh, skin), mesh["faces"])
                        new = normal_angles(*posed(source, mesh, pieces[mesh_id], skin), mesh["faces"])
                        self.assertFalse(np.any((old <= 90) & (new > 90)))
                        if variant == "anime" and mesh_id == 4 and scale == 1.:
                            self.assertGreater(old[1102], 120.)
                            self.assertLess(new[1102], 80.)
                            self.assertLess(np.count_nonzero(new > 90), np.count_nonzero(old > 90))

    def test_sources_nonhand_influences_and_identity_skin_geometry_are_preserved(self):
        for variant, hand_ids in (("anime", (2, 4)), ("traditional", (4, 8))):
            source = self.sources[variant]
            snapshots = [{key: value.copy() for key, value in mesh.items() if isinstance(value, np.ndarray)}
                         for mesh in source.meshes]
            pieces, _ = repair_saria_hand_weights(source)
            identity = np.tile(np.eye(4), (len(source.joints), 1, 1))
            for index, (mesh, repaired, snapshot) in enumerate(zip(source.meshes, pieces, snapshots)):
                for key, before in snapshot.items():
                    np.testing.assert_array_equal(mesh[key], before)
                self.assertFalse(np.shares_memory(repaired["joints"], mesh["joints"]))
                self.assertFalse(np.shares_memory(repaired["weights"], mesh["weights"]))
                np.testing.assert_allclose(repaired["weights"].sum(axis=1), 1., atol=1e-6)
                if index not in hand_ids:
                    np.testing.assert_array_equal(repaired["joints"], mesh["joints"])
                    np.testing.assert_array_equal(repaired["weights"], mesh["weights"])
                np.testing.assert_allclose(posed(source, mesh, repaired, identity)[0], mesh["positions"], atol=1e-7)

    def test_coincident_uv_copies_keep_identical_grip_positions(self):
        for variant, hand_ids in (("anime", (2, 4)), ("traditional", (4, 8))):
            source = self.sources[variant]
            pieces, _ = repair_saria_hand_weights(source)
            grip = {side: {"curls": NATURAL_CURLS} for side in ("Left", "Right")}
            skin = source_hand_pose(source, .85, separated=True, grip=grip)
            for mesh_id in hand_ids:
                mesh = source.meshes[mesh_id]
                p, _ = posed(source, mesh, pieces[mesh_id], skin)
                _, groups = np.unique(np.round(mesh["positions"], 6), axis=0, return_inverse=True)
                for group in np.flatnonzero(np.bincount(groups) > 1):
                    ids = np.flatnonzero(groups == group)
                    self.assertLess(np.linalg.norm(p[ids] - p[ids[0]], axis=1).max(), 1e-7)


if __name__ == "__main__":
    unittest.main()
