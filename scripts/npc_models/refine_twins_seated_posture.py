"""Fit the complete seated upper body; retain the native local head nod."""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

import numpy as np
from scipy.spatial.transform import Rotation

from native_npc_models import NativeRig, animation_frames, unit
from native_npc_pose_fit import parents, set_global
from edit_native_npc_pose import encode_animation
from verify_native_npc_models import Archive

PARENT_SHA256 = 'c57501ff2848a33579fe125d73ba8d662a8e4d96b6b57da4afa4e7108a26015f'
CLIP = 'objects/object_os_anime/gKokiriSittingHeadOnHandAnim'


def posture_angle(archive, rig, private):
    """Measure the rendered head's area center over the seated hip anchor."""
    path = rig.ns+'/Faces/L0R0M0' if private else rig.near[14]
    data = archive.geometry(path, rig, 14)
    matrices = archive.matrices(rig)
    p = np.zeros_like(data['local'])
    for bone in np.unique(data['bones']):
        mask = data['bones'] == bone
        p[mask] = data['local'][mask]@matrices[bone, :3, :3].T+matrices[bone, :3, 3]
    triangles = p[data['faces']]
    areas = np.linalg.norm(np.cross(triangles[:, 1]-triangles[:, 0],
                                    triangles[:, 2]-triangles[:, 0]), axis=1)
    center = np.average(triangles.mean(1), axis=0, weights=areas)
    axis = unit(rig.limbs[8].T-rig.limbs[11].T)
    up = unit(np.array([0., 1., 0.])-axis*axis[1])
    forward = np.cross(axis, up)
    delta = center-rig.limbs[0].T
    return float(np.arctan2(delta@forward, delta@up)), axis


def fit_posture(archive):
    """Change only the torso joint; its entire child chain stays connected."""
    native, custom = NativeRig(archive, 'twins'), NativeRig(archive, 'twins', True)
    original = animation_frames(archive.native.read(CLIP))
    fitted = original.copy()
    parent = parents(custom)
    corrections, errors = [], []
    for i, frame in enumerate(fitted):
        native.pose(original[i]); custom.pose(frame)
        target, _ = posture_angle(archive, native, False)
        current, axis = posture_angle(archive, custom, True)
        amount = target-current
        if not -20 < np.degrees(amount) < 0:
            raise ValueError('The seated correction escaped the reviewed forward-hunch range')
        # Chest and hip anchors coincide in this native pose. Rotating the
        # chest therefore balances the silhouette over the original seat.
        np.testing.assert_allclose(custom.limbs[7].T, custom.limbs[0].T, atol=1e-6)
        turn = Rotation.from_rotvec(axis*amount).as_matrix()
        set_global(custom, frame, 7, turn@custom.limbs[7].M, parent)
        actual, _ = posture_angle(archive, custom, True)
        corrections.append(float(np.degrees(amount)))
        errors.append(float(abs(np.degrees(actual-target))))
    unchanged = sorted(set(range(original.shape[1]))-{8})
    np.testing.assert_array_equal(fitted[:, unchanged], original[:, unchanged])
    if max(errors) > .02:
        raise ValueError('The connected upper body did not reach the native seat alignment')
    return fitted, dict(parent_sha256=PARENT_SHA256, native_animation=CLIP,
                       torso_joint=8, corrections_degrees=corrections,
                       max_seat_alignment_error_degrees=max(errors),
                       native_local_head_nod_and_all_other_channels_preserved=True,
                       method='Rendered head area center over the original seated hip anchor',
                       runtime_tested=False)


def validate_posture(archive, report, fitted):
    """Authorize this exact bounded torso fit, without relaxing other clips."""
    if report.get('parent_sha256') != PARENT_SHA256 or report.get('native_animation') != CLIP or report.get('torso_joint') != 8:
        raise ValueError('Unrecognized seated posture correction')
    expected, details = fit_posture(archive)
    if report != details or not np.array_equal(fitted, expected):
        raise ValueError('Seated posture differs from its bounded connected-body fit')


def refine(native, candidate, output):
    native, candidate, output = map(Path, (native, candidate, output))
    if output.resolve() in (native.resolve(), candidate.resolve()):
        raise ValueError('Preserve both source archives')
    if hashlib.file_digest(open(candidate, 'rb'), 'sha256').hexdigest() != PARENT_SHA256:
        raise ValueError('Use the reviewed POC8 parent with its skirt/arm repairs')
    archive = Archive(native, candidate)
    info = 'twins_NativeNpcModelInfo.json'
    manifest = json.loads(archive.candidate.read(info))
    if manifest['native_sha256'] != hashlib.file_digest(open(native, 'rb'), 'sha256').hexdigest():
        raise ValueError('The native reference differs from the reviewed model')
    frames, report = fit_posture(archive)
    manifest.setdefault('geometry_repairs', {})['pose3_posture'] = report
    manifest.setdefault('fitted_poses', {})[CLIP.rsplit('/', 1)[-1]] = dict(
        frames=len(frames), native_root_preserved=True, arms_only=False,
        native_clip_identical=False, torso_posture_only=True,
        native_local_head_nod_and_all_other_channels_preserved=True)
    namespace = manifest['namespace']
    changes = {'alt/'+namespace+'/PoseAnimations/'+CLIP: encode_animation(frames),
               namespace+'/SeatedPostureInfo.json': json.dumps(report, indent=2).encode(),
               info: json.dumps(manifest, indent=2).encode()}
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for name in sorted(set(archive.candidate.namelist()) | set(changes)):
            raw = changes[name] if name in changes else archive.candidate.read(name)
            entry = zipfile.ZipInfo(name, (2026, 10, 19, 0, 0, 0)); entry.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(entry, raw)
    return dict(report, changed_resources=sorted(changes),
                output_sha256=hashlib.file_digest(open(output, 'rb'), 'sha256').hexdigest())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('native', 'candidate', 'output'): parser.add_argument(name)
    args = parser.parse_args()
    print(json.dumps(refine(args.native, args.candidate, args.output), indent=2))
