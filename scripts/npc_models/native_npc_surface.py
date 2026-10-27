"""Small, source-gated normal repairs for the approved Saria Anime model.

Call once on a freshly fitted model, after source_bone_transforms are available
and before the optional skin palette is prepared. Geometry, UVs and weights are
never edited here; the head, tunic, hands and boots are outside these masks.
"""
import hashlib

import numpy as np
from scipy.spatial import cKDTree


ANIME_SOURCE_SHA256 = 'dddeaed512e7b4aaedf960e1ddab13e74c480897e0f44404fb980c0c9ce5d63a'


def _unit(values):
    values = np.asarray(values, float)
    return values / np.maximum(np.linalg.norm(values, axis=-1, keepdims=True), 1e-12)


def _smooth(value):
    value = np.clip(value, 0., 1.)
    return value*value*(3.-2.*value)


def _points(positions, normals):
    """UV copies must contribute one geometric sample to the normal filter."""
    points, inverse = np.unique(np.round(positions, 6), axis=0, return_inverse=True)
    total = np.zeros_like(points)
    np.add.at(total, inverse, normals)
    return points, _unit(total), inverse


def _blend_elbow_rims(positions, normals, pieces, upper, lower, center, axis):
    """Share only nearby, outward-facing sleeve rim shading across two pieces."""
    result = np.asarray(normals, float).copy()
    normal = _unit(normals)
    delta = positions-center
    along = delta@axis
    radial = delta-along[:, None]*axis
    radius = np.linalg.norm(radial, axis=1)
    outer = ((abs(along) < .025) & (radius > .014) & (radius < .045) &
             ((normal*_unit(radial)).sum(1) > .5) & (abs(normal@axis) < .65))
    ids = [np.flatnonzero(outer & (pieces == piece)) for piece in (upper, lower)]
    if any(len(group) == 0 for group in ids):
        return result, 0
    grouped = [_points(positions[group], normal[group]) for group in ids]
    point = [group[0] for group in grouped]
    current = [group[1] for group in grouped]
    total = [np.zeros_like(group) for group in current]
    weights = [np.zeros(len(group)) for group in current]
    strength = [np.zeros(len(group)) for group in current]
    matches = cKDTree(point[0]).query_ball_tree(cKDTree(point[1]), .006)
    count = 0
    for first, neighbors in enumerate(matches):
        for second in neighbors:
            # Interior caps can lie almost on an outer surface. Proximity
            # alone must not average opposing cap and garment normals.
            if np.dot(current[0][first], current[1][second]) < .26:
                continue
            distance = np.linalg.norm(point[0][first]-point[1][second])
            weight = float(_smooth((.006-distance)/.003))
            if weight <= 0:
                continue
            for side, own, other in ((0, first, second), (1, second, first)):
                total[side][own] += current[1-side][other]*weight
                weights[side][own] += weight
                strength[side][own] = max(strength[side][own], weight)
            count += 1
    for side in (0, 1):
        active = weights[side] > 0
        target = current[side].copy()
        target[active] = _unit(current[side][active] +
                               total[side][active]/weights[side][active, None])
        blend = .9*strength[side][:, None]
        revised = _unit(current[side]*(1-blend)+target*blend)
        inverse = grouped[side][2]
        chosen = active[inverse]
        result[ids[side][chosen]] = revised[inverse[chosen]]
    return result, count


def _filter_normals(positions, normals, strength):
    """A weak local filter with a caller-owned smooth spatial envelope."""
    result = np.asarray(normals, float).copy()
    points, normal, inverse = _points(positions, normals)
    amount = np.zeros(len(points))
    np.maximum.at(amount, inverse, strength)
    tree = cKDTree(points)
    revised = normal.copy()
    for index in np.flatnonzero(amount > 0):
        neighbors = np.asarray(tree.query_ball_point(points[index], .020), int)
        alignment = normal[neighbors]@normal[index]
        neighbors = neighbors[alignment > .65]
        distance = np.linalg.norm(points[neighbors]-points[index], axis=1)
        weight = np.exp(-(distance/.010)**2)
        target = _unit((normal[neighbors]*weight[:, None]).sum(0))
        revised[index] = _unit(normal[index]*(1-amount[index])+target*amount[index])
    active = strength > 0
    result[active] = revised[inverse[active]]
    return result


def _reconstruct_calf_normals(positions, normals, knee):
    """Bridge the narrow rear ridge with tangents fitted to its surroundings.

    The quadratic retains the calf's transverse bulge and longitudinal curve.
    Its samples exclude the ridge core; only a soft strip uses the new normal
    field. Positions, the boot cuff and the outer muscle boundary stay intact.
    """
    result = np.asarray(normals, float).copy()
    sign = 1. if knee[0] > 0 else -1.
    center_x = sign*(abs(knee[0])+.022+(knee[1]-positions[:, 1])*.15)
    u = positions[:, 0]-center_x
    v = positions[:, 1]-.255
    # Scale the polynomial coordinates before solving the small tangent fit.
    x, y = u/.040, v/.080
    design = np.column_stack([np.ones(len(x)), x, y, x*x, y*y, x*y])
    support = ((positions[:, 1] > .18) & (positions[:, 1] < .35) &
               (positions[:, 2] < -.015) & (normals[:, 2] < -.35) &
               (abs(u) > .009) & (abs(u) < .045))
    # UV duplicates must not give one side of the fit extra influence.
    _, unique = np.unique(np.round(positions, 6), axis=0, return_index=True)
    ids = unique[support[unique]]
    if len(ids) < 12:
        return result
    coefficient, _, rank, _ = np.linalg.lstsq(design[ids], positions[ids, 2], rcond=None)
    if rank < 6:
        return result
    residual = positions[ids, 2]-design[ids]@coefficient
    deviation = np.median(abs(residual-np.median(residual)))
    retained = abs(residual-np.median(residual)) < max(.004, 3*deviation)
    if retained.sum() >= 12:
        coefficient = np.linalg.lstsq(design[ids[retained]], positions[ids[retained], 2], rcond=None)[0]
    du = (coefficient[1]+2*coefficient[3]*x+coefficient[5]*y)/.040
    dy = ((coefficient[2]+2*coefficient[4]*y+coefficient[5]*x)/.080 +
          du*sign*.15)
    target = _unit(np.column_stack([du, dy, -np.ones(len(du))]))
    envelope = (_smooth((positions[:, 1]-.20)/.020)*
                _smooth((.33-positions[:, 1])/.020)*
                _smooth((.026-abs(u))/.016)*
                _smooth((-positions[:, 2]-.025)/.015)*
                _smooth((-normals[:, 2]-.20)/.30))
    active = envelope > 0
    amount = .95*envelope[active, None]
    result[active] = _unit(normals[active]*(1-amount)+target[active]*amount)
    return result


def repair_anime_surface_normals(source, model, rig):
    """Repair the diagnosed Anime elbow rims and rear calf highlights in place.

    Return a serializable report, or None for every other source/rig. The two
    edited normal fields stay in their existing source and fitted coordinate
    systems; palette preparation can therefore use either field normally.
    """
    if getattr(rig, 'name', None) != 'saria':
        return None
    digest = hashlib.sha256(getattr(source, 'raw', b'')).hexdigest()
    if digest != ANIME_SOURCE_SHA256:
        return None

    positions = model['source_positions']
    original = model['posed_source_normals'].copy()
    normal = original.copy()
    pieces = model['mesh_ids']
    names = [mesh.get('name', '') for mesh in source.meshes]
    named = {name: source.world[joint][:3, 3]
             for name, joint in zip(source.names, source.joints)}
    pair_count = 0
    for side, upper, lower in (('Right', 'tripo_part_7', 'tripo_part_9'),
                               ('Left', 'tripo_part_8', 'tripo_part_11')):
        center = named[side+'ForeArm']
        axis = _unit(center-named[side+'Arm'])
        normal, count = _blend_elbow_rims(
            positions, normal, pieces, names.index(upper), names.index(lower), center, axis)
        pair_count += count
    elbow_changed = np.linalg.norm(normal-original, axis=1) > 1e-10

    calf_changed = np.zeros(len(normal), bool)
    for side, piece in (('Left', 'tripo_part_5'), ('Right', 'tripo_part_6')):
        ids = np.flatnonzero(pieces == names.index(piece))
        p = positions[ids]
        knee = named[side+'Leg']
        revised = _reconstruct_calf_normals(p, normal[ids], knee)
        calf_changed[ids] = np.linalg.norm(revised-normal[ids], axis=1) > 1e-10
        normal[ids] = revised

    changed = elbow_changed | calf_changed
    fitted = np.zeros((np.count_nonzero(changed), 3))
    # This is the same covector mapping used by fit_model for these limbs.
    # Preserve its source native influences instead of rotating a fitted
    # normal as if every retarget map were an orthonormal matrix.
    weights = model['weights'][changed]
    for bone in np.flatnonzero((weights > 0).any(0)):
        inverse = np.linalg.inv(model['source_bone_transforms'][bone, :3, :3])
        fitted += (normal[changed]@inverse)*weights[:, bone, None]
    model['posed_source_normals'][changed] = normal[changed]
    model['normals'][changed] = _unit(fitted)
    turn = np.degrees(np.arccos(np.clip((_unit(original[changed])*normal[changed]).sum(1), -1, 1)))
    return dict(source_sha256=digest, normals_only=True, elbow_pairs=pair_count,
                elbow_vertices=int(elbow_changed.sum()), calf_vertices=int(calf_changed.sum()),
                max_source_normal_turn_degrees=round(float(turn.max(initial=0)), 3))
