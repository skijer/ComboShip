"""Physical held Rod of Seasons, derived from its GI mesh without presentation lean.

The existing OoT wrist pose is retained: native shaft +Y, 96-unit total height,
and the same model origin. This creates geometry/material resources only.
"""
import argparse
from copy import deepcopy
import hashlib
import importlib.util
from pathlib import Path
import shutil
import sys

import numpy as np

HELD_ROOT = Path(__file__).resolve().parents[1]
GI_SOURCE = HELD_ROOT.parent / 'nei_gi' / 'SOURCE'
sys.path.insert(0, str(GI_SOURCE))
import meshkit
from meshkit import Q, rotation, unit
import preview

GI_LEAN_DEGREES = -12.0
LEGACY_NATIVE_HEIGHT = 96.0
LEGACY_NATIVE_CENTER_Y = 0.0
POSE = (0.0, 5.977, -3.218, 109.655, 72.414, 0.0, 0.4)


def _gi_source():
    """Load the current shared recipe, including later scope simplifications."""
    path = GI_SOURCE / 'quest_revamp.py'
    spec = importlib.util.spec_from_file_location('nei_quest_rod_held_source', path)
    source = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(source)
    return source.BUILDERS['rod_of_seasons'](), path


def rod_of_seasons():
    source, path = _gi_source()
    m = deepcopy(source)
    m.name = 'Rod of Seasons / physical held model'
    m.slug = 'rod_of_seasons'
    m.prefix = 'objects/nei_held_redesign/rod_of_seasons/'
    m.entry = m.prefix + 'gi_dl'

    # The GI has a baked display lean, unlike the existing held staff. Undo it
    # before measuring, without introducing GI spin or changing wrist rotations.
    m.transform(rotation('z', -GI_LEAN_DEGREES))
    positions = np.concatenate([part['p'] for part in m.parts])
    lo, hi = positions[:, 1].min(), positions[:, 1].max()
    source_height = float(hi-lo)
    center = float((lo+hi)*.5)
    m.transform(offset=[0, -center, 0])
    positions = np.concatenate([part['p'] for part in m.parts])
    centered_height = float(np.ptp(positions[:, 1]))

    # The resource matrix converts the 16-times quantized mesh to the legacy
    # native envelope; the caller's existing .4 is still applied exactly once.
    native_factor = LEGACY_NATIVE_HEIGHT / centered_height
    m.native_scale = round(native_factor / Q * 65536) / 65536
    m.draw_scale = POSE[-1]
    m.effective_scale = m.native_scale * Q * m.draw_scale
    native_factor_actual = m.native_scale * Q

    # A final quantization may collapse or reverse the smallest crown accents.
    # Reorient from the preserved source normals after all author transforms.
    for part in m.parts:
        part['p'] = np.rint(part['p'] * Q) / Q
        part['n'] = unit(part['n'])
        f = part['tri']; p = part['p']
        fn = np.cross(p[f[:, 1]]-p[f[:, 0]], p[f[:, 2]]-p[f[:, 0]])
        good = np.linalg.norm(fn, axis=1) > 1e-7
        f = f[good]; fn = fn[good]
        flip = np.einsum('ij,ij->i', fn, part['n'][f].sum(axis=1)) < 0
        f[flip] = f[flip][:, [0, 2, 1]]
        part['tri'] = f
    m.parts = [part for part in m.parts if len(part['tri'])]
    used = {part['mat'] for part in m.parts}
    m.materials = {name:mat for name,mat in m.materials.items() if name in used}

    positions = np.concatenate([part['p'] for part in m.parts])
    native_bounds = [list(v*native_factor_actual)
                     for v in (positions.min(axis=0), positions.max(axis=0))]
    # Model origin is the old staff origin. The helper translates AFTER wrist
    # rotation and BEFORE .4 scaling; rebasing to a zero grip would break it.
    m.markers = dict(
        model_origin_native=[0, 0, 0], canonical_shaft_axis=[0, 1, 0],
        native_bounds=native_bounds, native_height=LEGACY_NATIVE_HEIGHT,
        source_height_author=source_height, removed_gi_lean_degrees=GI_LEAN_DEGREES,
        source_center_y_author=center, native_factor=native_factor_actual,
        world_factor=m.effective_scale,
        hand_pose_offsets=list(POSE[:3]), hand_pose_rotations_degrees=list(POSE[3:6]),
        hand_pose_scale=POSE[6], hand_pose_change_required=False,
        wrist_in_model_native=(-np.array(POSE[:3])/POSE[6]).tolist(),
        source_recipe_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        presentation_only=False, weather_vfx=False, gameplay_changes=False,
    )
    m.notes = [
        'Same physical Rod of Seasons triangles/materials as the shared GI recipe; baked -12-degree GI lean removed.',
        'Legacy native staff bounds Y=-48..48 and shaft +Y are preserved by an internal resource matrix.',
        'Existing sRodPose stays [0,5.977,-3.218,109.655,72.414,0,.4]; origin is not rebased to the grip.',
        'Helper order remains captured wrist / unscale / RY / RX / RZ / local translation / .4 scale.',
        'Crown width is an art change; no geometry evidence requires changing the established wrist pose.',
        'Physical geometry/material only: no weather effects, seasonal GI variants, selection or gameplay edits; no MM equip port.',
        'Hand clearance and runtime appearance remain untested.',
    ]
    return m


BUILDERS = {'rod_of_seasons': rod_of_seasons}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--install', action='store_true')
    args = parser.parse_args()
    old_mesh_root, old_preview_root = meshkit.ROOT, preview.ROOT
    try:
        meshkit.ROOT = preview.ROOT = HELD_ROOT
        m = rod_of_seasons()
        stats = meshkit.export_resources(m)
        preview.checkpoint(m, stats)
        if args.install:
            shutil.copytree(HELD_ROOT/'RESOURCES'/m.prefix,
                            HELD_ROOT.parents[1]/'soh/assets/custom'/m.prefix,
                            dirs_exist_ok=True)
    finally:
        meshkit.ROOT, preview.ROOT = old_mesh_root, old_preview_root


if __name__ == '__main__':
    main()
