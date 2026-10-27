"""Independent readback checks for the bounded Young Fado shoulder candidate.

Set YOUNG_FADO_NATIVE, YOUNG_FADO_PARENT and YOUNG_FADO_CANDIDATE to run the
asset checks. The fixtures are user-owned archives and are never committed.
"""
import os
from collections import Counter
import struct
import unittest
import zipfile
from pathlib import Path
from types import SimpleNamespace

import numpy as np

from native_assets import skeleton
from native_npc_models import animation_frames, read_limb, read_string, unit
from verify_native_npc_models import Archive
from repair_young_fado_shoulders import preserve_orientation


NS = 'objects/object_fa/YoungFadoNpcPOC1'
CLIPS = (
    'gKokiriIdleAnim', 'gKokiriStandingArmsBehindBackAnim',
    'gKokiriStandingHandsOnHipsAnim', 'gKokiriSittingHeadOnHandAnim',
    'gKokiriSittingCrossedLegsAnim', 'gKokiriSittingCrossedArmsLegsAnim',
)


def fixture_rig(archive):
    raw = archive.read(NS+'/Skel')
    count, drawable = struct.unpack_from('<II', raw, 66)
    limbs, offset = [], 79
    for index in range(count):
        path, offset = read_string(raw, offset)
        values, near, _ = read_limb(archive.read(path))
        limbs.append(skeleton.Limb(index, *values, int(bool(near))))
    skel = skeleton.Skeleton(limbs, count, drawable)
    skeleton._pose(skel, None)
    return SimpleNamespace(ns=NS, limbs=limbs, skel=skel, dlist_count=drawable)


def fixture_frames(archive, pose):
    if pose == 5:
        return animation_frames(archive.read(NS+'/Animations/Seated5'))
    return animation_frames(archive.native.read('objects/object_os_anime/'+CLIPS[pose]))


def shoulder_path(archive, pose, host):
    for path in (
        NS+'/PoseAnimations/objects/object_os_anime/'+CLIPS[pose]+f'/Skin/Limbs/Limb{host:02d}',
        NS+f'/Skin/Limbs/Limb{host:02d}',
        NS+f'/Body/Limb{host:02d}',
    ):
        if 'alt/'+path in archive.candidate.namelist():
            return path
    raise AssertionError('No shoulder display list')


def pose_geometry(archive, rig, path, host, frame):
    skeleton._pose(rig.skel, frame.ravel().tolist())
    data = archive.geometry(path, rig, host)
    matrices = archive.matrices(rig)
    positions, normals = np.zeros_like(data['local']), np.zeros_like(data['normals'])
    for bone in np.unique(data['bones']):
        mask = data['bones'] == bone
        matrix = matrices[bone]
        positions[mask] = data['local'][mask]@matrix[:3, :3].T+matrix[:3, 3]
        normals[mask] = unit(data['normals'][mask]@matrix[:3, :3].T)
    return data, positions, normals


def frame_samples(frames):
    for index, current in enumerate(frames):
        yield index, current
        following = frames[(index+1) % len(frames)]
        delta = (following-current+32768) % 65536-32768
        half = current.astype(float)+delta*.5
        half[0] = (current[0]+following[0])*.5
        yield index+.5, half


class ShoulderOrientationTests(unittest.TestCase):
    def test_fold_is_corrected_by_moving_surface_with_fixed_seam(self):
        points = np.array([[0., 0., 0.], [10., 0., 0.], [5., -1., 0.]])
        normals = np.tile([0., 0., 1.], (3, 1))
        result = preserve_orientation(points, normals, np.array([[0, 1, 2]]),
                                      np.array([True, True, False]))
        np.testing.assert_array_equal(result[:2], points[:2])
        self.assertGreater(np.cross(result[1]-result[0], result[2]-result[0])[2], 0)
        np.testing.assert_array_equal(normals, np.tile([0., 0., 1.], (3, 1)))

    def test_impossible_fixed_fold_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'fixed shoulder boundary'):
            preserve_orientation(np.array([[0., 0., 0.], [10., 0., 0.], [5., -1., 0.]]),
                                 np.tile([0., 0., 1.], (3, 1)), np.array([[0, 1, 2]]),
                                 np.ones(3, bool))


class YoungFadoShoulderArchiveTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        names = ('YOUNG_FADO_NATIVE', 'YOUNG_FADO_PARENT', 'YOUNG_FADO_CANDIDATE')
        if any(not os.environ.get(name) for name in names):
            raise unittest.SkipTest('Set the three YOUNG_FADO archive fixture paths')
        cls.native, cls.parent, cls.candidate = [Path(os.environ[name]) for name in names]
        cls.archive = Archive(cls.native, cls.candidate)
        cls.rig = fixture_rig(cls.archive)

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, 'archive'):
            cls.archive.native.close()
            cls.archive.candidate.close()

    def test_cap_faces_remain_outward_in_all_six_static_poses(self):
        """Catch the parent's folded chest/upper-arm triangles after emission."""
        for pose, clip in enumerate(CLIPS):
            frames = fixture_frames(self.archive, pose)
            for frame_index, frame in frame_samples(frames):
                for host in (8, 11):
                    with self.subTest(clip=clip, frame=frame_index, host=host):
                        data, p, normals = pose_geometry(
                            self.archive, self.rig,
                            shoulder_path(self.archive, pose, host), host, frame)
                        f = data['faces']
                        cross = np.cross(p[f[:, 1]]-p[f[:, 0]], p[f[:, 2]]-p[f[:, 0]])
                        self.assertGreater(float(np.linalg.norm(cross, axis=1).min()), 1e-6,
                                           'The shoulder has a collapsed surface triangle')
                        alignment = (unit(cross)*unit(normals[f].mean(1))).sum(1)
                        self.assertEqual(int((alignment < -.05).sum()), 0,
                                         'The shoulder has folded/back-facing surface triangles')

    def test_native_fado_laughter_retains_outward_shoulders(self):
        frames = animation_frames(self.archive.native.read('objects/object_os_anime/gKokiriLaughingAnim'))
        for index, frame in frame_samples(frames):
            for host in (8, 11):
                with self.subTest(frame=index, host=host):
                    path = NS+'/PoseAnimations/objects/object_os_anime/gKokiriLaughingAnim'+f'/Skin/Limbs/Limb{host:02d}'
                    if 'alt/'+path not in self.archive.candidate.namelist():
                        path = NS+f'/Skin/Limbs/Limb{host:02d}'
                    if 'alt/'+path not in self.archive.candidate.namelist():
                        path = NS+f'/Body/Limb{host:02d}'
                    data, p, normals = pose_geometry(self.archive, self.rig, path, host, frame)
                    f = data['faces']
                    cross = np.cross(p[f[:, 1]]-p[f[:, 0]], p[f[:, 2]]-p[f[:, 0]])
                    alignment = (unit(cross)*unit(normals[f].mean(1))).sum(1)
                    self.assertEqual(int((alignment < -.05).sum()), 0)

    def test_original_winding_and_both_shoulder_boundaries_are_retained(self):
        parent = Archive(self.native, self.parent)
        try:
            parent_rig = fixture_rig(parent)
            chest = parent.geometry(NS+'/Body/Limb07', parent_rig, 7)
            body_edge = {tuple(row) for row in chest['local'][chest['bones'] == 7]}
            for host in (8, 11):
                before = parent.geometry(NS+f'/Body/Limb{host:02d}', parent_rig, host)
                old_uv = np.rint(before['uv']*16384).astype(int)
                topology = Counter(tuple(map(tuple, old_uv[f])) for f in before['faces'])
                seam = {tuple(local) for local in before['local'][before['bones'] == 7]}
                self.assertTrue(seam <= body_edge)
                forearm = parent.geometry(NS+f'/Body/Limb{host+1:02d}', parent_rig, host+1)
                distal = ({tuple(row) for row in before['local'][before['bones'] == host]} &
                          {tuple(row) for row in forearm['local'][forearm['bones'] == host]})
                self.assertGreater(len(distal), 3)
                for pose in range(6):
                    after = self.archive.geometry(shoulder_path(self.archive, pose, host), self.rig, host)
                    uv = np.rint(after['uv']*16384).astype(int)
                    self.assertEqual(topology, Counter(tuple(map(tuple, uv[f])) for f in after['faces']))
                    new_seam = {tuple(local) for local in after['local'][after['bones'] == 7]}
                    self.assertTrue(seam <= new_seam)
                    new_distal = {tuple(local) for local in after['local'][after['bones'] == host]}
                    self.assertTrue(distal <= new_distal)
        finally:
            parent.native.close()
            parent.candidate.close()

    def test_original_resources_except_manifest_remain_byte_identical(self):
        with zipfile.ZipFile(self.parent) as before:
            after = self.archive.candidate
            self.assertFalse(set(before.namelist())-set(after.namelist()))
            for path in before.namelist():
                if path != 'YoungFadoNpcInfo.json':
                    with self.subTest(resource=path):
                        self.assertEqual(before.read(path), after.read(path))


if __name__ == '__main__':
    unittest.main()
