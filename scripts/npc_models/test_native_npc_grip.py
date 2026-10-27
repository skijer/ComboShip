"""Regression checks for moving a complete arm to an instrument grip."""
import unittest
import struct
from types import SimpleNamespace
import numpy as np
from scipy.spatial.transform import Rotation
import native_npc_pose_fit as pose_fit


class WristSeatTests(unittest.TestCase):
    def test_approach_clearance_can_hold_through_a_late_native_wrist_turn(self):
        hands = {'Left': {'approach_wrist_offset': [0., 0., 60.],
                          'approach_start': .7, 'approach_end': 1.}}
        for blend, expected in [(0., 60.), (.7, 60.), (.85, 30.), (1., 0.)]:
            targets = {'left': np.zeros(3), 'right': np.zeros(3)}
            pose_fit.seat_wrist_targets(targets, {'left': np.eye(3), 'right': np.eye(3)}, hands, blend)
            np.testing.assert_allclose(targets['left'], [0., 0., expected])
            np.testing.assert_array_equal(targets['right'], np.zeros(3))

    def test_approach_seat_keeps_the_playing_grip_and_other_arm_exact(self):
        hands = {'Left': {'wrist_offset': [20., 30., 40.],
                          'approach_wrist_offset': [-10., 0., 8.], 'approach_end': .8},
                 'Right': {'wrist_offset': [1., 2., 3.]}}
        matrices = {'left': np.array([[0., -1., 0.], [1., 0., 0.], [0., 0., 1.]]),
                    'right': np.eye(3)}
        for blend, extra in [(0., [-10., 0., 8.]), (.4, [-5., 0., 4.]),
                             (.8, [0., 0., 0.]), (1., [0., 0., 0.])]:
            targets = {'left': np.zeros(3), 'right': np.zeros(3)}
            pose_fit.seat_wrist_targets(targets, matrices, hands, blend)
            np.testing.assert_allclose(targets['left'], matrices['left']@(np.array([20., 30., 40.])+extra))
            np.testing.assert_array_equal(targets['right'], [1., 2., 3.])

    def test_lowered_hold_moves_both_hands_together_and_fades_before_playing(self):
        self.assertTrue(callable(getattr(pose_fit, 'support_carried_ocarina', None)))
        hands = {'Right': {'carried_offset': [0., 400., 0.], 'carried_support_end': .25}}
        body = np.array([[0., -1., 0.], [1., 0., 0.], [0., 0., 1.]])
        for blend, expected in [(0., [-400., 0., 0.]), (.125, [-200., 0., 0.]),
                                (.25, [0., 0., 0.]), (1., [0., 0., 0.])]:
            targets = {'left': np.array([10., 20., 30.]), 'right': np.array([-10., 20., 30.])}
            original = {s: p.copy() for s, p in targets.items()}
            pose_fit.support_carried_ocarina(targets, body, hands, blend)
            for side in targets:
                np.testing.assert_allclose(targets[side]-original[side], expected)
            np.testing.assert_array_equal(targets['left']-targets['right'], [20., 0., 0.])

    def test_grip_prop_does_not_move_the_shared_prop_in_other_poses(self):
        from native_assets import o2r as asset
        self.assertTrue(callable(getattr(pose_fit, 'export_ocarina', None)),
                        'Shared and seated ocarinas need separate exported resources')
        prop = bytearray(696)
        struct.pack_into('<II', prop, 384, 0xE7000000, 0)
        texture_hash = asset.crc64('objects/object_sa/gSariaFairyOcarinaTex')
        struct.pack_into('<IIII', prop, 112, 0x20100000, 0,
                         texture_hash >> 32, texture_hash & 0xffffffff)
        records = [(100, 200, 300, 0, 0, 0, 0, 0, 127, 255)]*20
        data = {'objects/object_sa/gSariaRightHandAndOcarinaDL': bytes(prop),
                'objects/object_sa/object_saVtx_006948': asset.build_vertex_array(records)}
        archive = asset.O2R()
        pose_fit.export_ocarina(archive, SimpleNamespace(read=data.__getitem__),
                                'objects/fixture', {'Right': {'wrist_offset': [20., 30., 40.]}})
        base = asset.read_vertex_array(archive.files['alt/objects/fixture/Props/OcarinaVertices'])
        seated = asset.read_vertex_array(archive.files['alt/objects/fixture/Props/OcarinaGripVertices'])
        self.assertEqual(tuple(base[0][:3]), (80, 160, 240))
        self.assertEqual(tuple(seated[0][:3]), (60, 130, 200))
        for before, after in zip(base, seated):
            self.assertEqual(tuple(before[3:]), tuple(after[3:]))

    def test_fitter_rejects_a_translation_that_detaches_the_hand_surface(self):
        from native_npc_models import fit_model
        from test_native_npc_models import proportion_fixture
        source, rig = proportion_fixture()
        with self.assertRaisesRegex(ValueError, 'wrist_offset'):
            fit_model(source, rig, hand_grip={'Left': {'native_offset': [0., 0., 150.]}})

    def test_seating_moves_both_wrists_and_keeps_the_instrument_in_place(self):
        # Baking this translation into the hand surface used to detach the
        # wrist. The arm target must move, with an inverse prop-local shift.
        self.assertTrue(callable(getattr(pose_fit, 'seat_wrist_targets', None)),
                        'A grip must seat whole arms instead of translating hand vertices')
        self.assertTrue(callable(getattr(pose_fit, 'seat_ocarina', None)))
        matrices = {'right': Rotation.from_euler('xyz', [17, -31, 23], degrees=True).as_matrix(),
                    'left': Rotation.from_euler('xyz', [-25, 12, -9], degrees=True).as_matrix()}
        targets = {'right': np.array([900., 3200., 100.]),
                   'left': np.array([-700., 3100., 80.])}
        old_targets = {s: t.copy() for s, t in targets.items()}
        hands = {'Right': {'wrist_offset': [30., -20., 40.]},
                 'Left': {'wrist_offset': [-15., 25., 35.]}}
        prop = np.array([[491.2, -104., 360.], [-120., 40., 90.]])
        seated_prop = pose_fit.seat_ocarina(prop, hands)
        pose_fit.seat_wrist_targets(targets, matrices, hands)
        for side, offset in [('right', [30., -20., 40.]), ('left', [-15., 25., 35.])]:
            np.testing.assert_allclose(targets[side], old_targets[side]+matrices[side]@offset)
        np.testing.assert_allclose(seated_prop@matrices['right'].T+targets['right'],
                                   prop@matrices['right'].T+old_targets['right'])
        np.testing.assert_array_equal(prop[0], [491.2, -104., 360.])


if __name__ == '__main__':
    unittest.main()
