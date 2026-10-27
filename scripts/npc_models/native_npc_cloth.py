"""One source-gated front shading field for traditional Saria's split tunic.

The supplied chest and waist shells have jagged open rims and sculpted normal
creases. Only their normals are repaired; source shape, UVs and native fitting
stay unchanged. Call after fit_saria_cloth and before skin palette preparation.
"""
import hashlib

import numpy as np
from scipy.ndimage import distance_transform_edt, gaussian_filter, map_coordinates


RENDER_SOURCE_SHA256 = '902dfca93f4640a65a72e4266f855966fa708b412e0742789552f55f85c94aff'
PIECES = (3, 20, 24)
BOUNDS = (-.11, .56, .11, .80)
RESOLUTION = 256
SMOOTH_RADIUS = .012


def _unit(values):
    values = np.asarray(values, float)
    return values/np.maximum(np.linalg.norm(values, axis=-1, keepdims=True), 1e-12)


def _front_normals(positions, faces, ids):
    """Sample one smooth front-envelope gradient across all three source pieces."""
    bounds = np.asarray(BOUNDS)
    step = (bounds[2:]-bounds[:2])/(RESOLUTION-1)
    xy = (positions[:, :2]-bounds[:2])/step
    depth = np.full((RESOLUTION, RESOLUTION), -np.inf)
    for face in faces:
        q = xy[face]
        low = np.maximum(np.floor(q.min(0)).astype(int), 0)
        high = np.minimum(np.ceil(q.max(0)).astype(int), RESOLUTION-1)
        if np.any(high < low):
            continue
        matrix = np.column_stack([q[0]-q[2], q[1]-q[2]])
        if abs(np.linalg.det(matrix)) < 1e-12:
            continue
        xx, yy = np.meshgrid(np.arange(low[0], high[0]+1), np.arange(low[1], high[1]+1))
        pair = (np.stack([xx, yy], -1)-q[2])@np.linalg.inv(matrix).T
        weights = np.dstack([pair, 1-pair.sum(-1)])
        z = weights@positions[face, 2]
        visible = (weights.min(-1) >= -1e-6) & (z > depth[yy, xx])
        depth[yy[visible], xx[visible]] = z[visible]
    valid = np.isfinite(depth)
    if not valid.any():
        return None
    nearest = distance_transform_edt(~valid, return_distances=False, return_indices=True)
    smooth = gaussian_filter(depth[tuple(nearest)], sigma=[SMOOTH_RADIUS/step[1], SMOOTH_RADIUS/step[0]])
    dy, dx = np.gradient(smooth, step[1], step[0])
    coordinates = [xy[ids, 1], xy[ids, 0]]
    return _unit(np.column_stack([-map_coordinates(dx, coordinates, order=1, mode='nearest'),
                                  -map_coordinates(dy, coordinates, order=1, mode='nearest'),
                                  np.ones(len(ids))]))


def repair_traditional_cloth_normals(source, model, rig):
    """Update source/fitted normal fields consistently; return a repair report.

    Front depth is used only to derive a continuous shading field. No vertex
    positions, triangle indices, UVs, scale, bindings or native weights change.
    """
    if getattr(rig, 'name', None) != 'saria':
        return None
    digest = hashlib.sha256(getattr(source, 'raw', b'')).hexdigest()
    if digest != RENDER_SOURCE_SHA256:
        return None
    p = model['source_positions']
    eligible = np.isin(model['mesh_ids'], PIECES) & (model['bones'] != rig.profile['head'])
    target = (eligible & (p[:, 2] > 0) & (abs(p[:, 0]) < .10) &
              (p[:, 1] > BOUNDS[1]) & (p[:, 1] < BOUNDS[3]))
    ids = np.flatnonzero(target)
    faces = model['faces'][eligible[model['faces']].all(1)]
    report = dict(source_sha256=digest, normals_only=True, mesh_ids=list(PIECES),
                  front_vertices=0, field_bounds=list(BOUNDS),
                  field_resolution=RESOLUTION, smooth_radius=SMOOTH_RADIUS)
    if not len(ids) or not len(faces):
        return report
    normal = _front_normals(p, faces, ids)
    if normal is None:
        return report
    original = model['posed_source_normals'][ids].copy()
    fitted = np.zeros_like(normal)
    weights = model['weights'][ids]
    for bone in np.flatnonzero((weights > 0).any(0)):
        inverse = np.linalg.inv(model['source_bone_transforms'][bone, :3, :3])
        fitted += (normal@inverse)*weights[:, bone, None]
    model['posed_source_normals'][ids] = normal
    model['normals'][ids] = _unit(fitted)
    turn = np.degrees(np.arccos(np.clip((_unit(original)*normal).sum(1), -1, 1)))
    report.update(front_vertices=int(len(ids)), max_source_normal_turn_degrees=round(float(turn.max()), 3))
    return report
