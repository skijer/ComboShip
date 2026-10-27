"""Source-hash-bounded repairs for diagnosed Saria distal finger weights.

Only audited distal points are eligible. Nearest-chain pruning across an entire
hand creates new folds at ambiguous digit boundaries, so the spatial and topology
checks below validate the two proven patches rather than select additional ones.
Source arrays remain untouched; callers receive independent influence copies.
Apply these only to explicit grip/curl fits, preserving accepted relaxed hands.
"""
import hashlib

import numpy as np


_ANIME = "dddeaed512e7b4aaedf960e1ddab13e74c480897e0f44404fb980c0c9ce5d63a"
_TRADITIONAL = "902dfca93f4640a65a72e4266f855966fa708b412e0742789552f55f85c94aff"
_FINGERS = ("Thumb", "Index", "Middle", "Ring", "Pinky")
# mesh, source name, side, audited (local vertex, actual finger) patches.
# All four hands were inspected. Other candidate changes introduced local
# folds or involved ambiguous webs and are deliberately left unchanged.
_AUDITS = {
    _ANIME: ((2, "tripo_part_10", "Right", ()),
             (4, "tripo_part_12", "Left", ((632, "Middle"), (641, "Middle")))),
    _TRADITIONAL: ((4, "tripo_part_12", "Right", ()),
                   (8, "tripo_part_16", "Left", ())),
}


def _chain_proximity(source, mesh, side):
    """Return nearest raw-source digit, longitudinal progress and distance gap."""
    named = {name: source.world[joint][:3, 3] for name, joint in zip(source.names, source.joints)}
    points = mesh["positions"]
    distances, progress = [], []
    for finger in _FINGERS:
        chain = np.array([named[side + "Hand" + finger + str(k)] for k in (1, 2, 3, 4)])
        axis = chain[1:] - chain[:-1]
        length = np.linalg.norm(axis, axis=1)
        t = np.clip(np.einsum("vsi,si->vs", points[:, None] - chain[:-1], axis) / length**2, 0, 1)
        distance = np.linalg.norm(points[:, None] - (chain[:-1] + t[..., None] * axis), axis=2)
        segment = distance.argmin(axis=1)
        row = np.arange(len(points))
        distances.append(distance[row, segment])
        start = np.r_[0., np.cumsum(length)]
        progress.append((start[segment] + t[row, segment] * length[segment]) / length.sum())
    distances = np.asarray(distances).T
    order = distances.argsort(axis=1)
    row = np.arange(len(points))
    nearest = order[:, 0]
    return nearest, np.asarray(progress).T[row, nearest], distances[row, nearest], (
        distances[row, order[:, 1]] - distances[row, nearest])


def _surface_groups(mesh):
    """Share topology votes between coincident copies at UV chart boundaries."""
    _, groups = np.unique(np.round(mesh["positions"], 6), axis=0, return_inverse=True)
    adjacent = [set() for _ in range(groups.max() + 1)]
    faces = mesh["faces"]
    for a, b in np.concatenate([faces[:, [0, 1]], faces[:, [1, 2]], faces[:, [2, 0]]]):
        a, b = int(groups[a]), int(groups[b])
        if a != b:
            adjacent[a].add(b)
            adjacent[b].add(a)
    return groups, adjacent


def repair_saria_hand_weights(source):
    """Return ``(pieces, report)`` aligned to ``source.meshes``, without mutation.

    Each piece contains independent ``joints`` and ``weights`` arrays. The two
    audited Middle-tip points retain their valid Middle2/Middle3 blend while
    misplaced Ring influences are redistributed proportionally to that blend.
    Palm, web, other digit, nonhand and unsupported-source data stays identical.
    """
    pieces = [{"joints": mesh["joints"].copy(), "weights": mesh["weights"].copy()}
              for mesh in source.meshes]
    digest = hashlib.sha256(source.raw).hexdigest()
    report = dict(applied=False, recognized_source=digest in _AUDITS,
                  source_sha256=digest, changed_vertices=0, removed_cross_digit_mass=0., hands=[])
    if digest not in _AUDITS:
        return pieces, report
    for mesh_id, name, side, patches in _AUDITS[digest]:
        if mesh_id >= len(source.meshes) or source.meshes[mesh_id]["name"] != name:
            raise ValueError("Audited Saria hand-piece identity changed")
        mesh, repaired = source.meshes[mesh_id], pieces[mesh_id]
        hand = dict(mesh_id=mesh_id, mesh_name=name, side=side, changed_vertices=0,
                    changed_local_vertices=[], removed_cross_digit_mass=0., patches=[])
        report["hands"].append(hand)
        if not patches:
            continue
        nearest, progress, distance, gap = _chain_proximity(source, mesh, side)
        groups, adjacent = _surface_groups(mesh)
        first = np.array([np.flatnonzero(groups == group)[0] for group in range(len(adjacent))])
        digit_joints = {finger: [source.names.index(side + "Hand" + finger + str(k)) for k in (1, 2, 3, 4)]
                        for finger in _FINGERS}
        repaired_groups = set()
        for vertex, finger in patches:
            group = int(groups[vertex])
            if group in repaired_groups:
                continue
            copies = np.flatnonzero(groups == group)
            digit = _FINGERS.index(finger)
            neighbors = adjacent[group]
            support = sum(nearest[first[index]] == digit for index in neighbors) / max(len(neighbors), 1)
            if (np.any(nearest[copies] != digit) or np.any(progress[copies] <= .7) or
                    np.any(distance[copies] >= .013) or np.any(gap[copies] <= .00075) or support < .9):
                raise ValueError("Audited distal finger point no longer has unambiguous source geometry")
            removed = 0.
            for index in copies:
                row = repaired["weights"][index].astype(float)
                own = np.isin(repaired["joints"][index], digit_joints[finger])
                foreign = np.isin(repaired["joints"][index], [joint for other in _FINGERS if other != finger
                                                              for joint in digit_joints[other]])
                own_mass, foreign_mass = float(row[own].sum()), float(row[foreign].sum())
                if own_mass < .1 or foreign_mass < .1:
                    raise ValueError("Audited distal finger influence pattern changed")
                row[own] *= (own_mass + foreign_mass) / own_mass
                row[foreign] = 0.
                repaired["weights"][index] = row
                removed += foreign_mass
            hand["changed_local_vertices"].extend(map(int, copies))
            hand["removed_cross_digit_mass"] += removed
            hand["patches"].append(dict(local_vertices=copies.tolist(), finger=finger,
                source_position=mesh["positions"][vertex].tolist(),
                nearest_chain_progress=float(progress[vertex]), nearest_chain_distance=float(distance[vertex]),
                second_chain_distance_gap=float(gap[vertex]), topology_support=float(support),
                removed_cross_digit_mass=removed))
            repaired_groups.add(group)
        hand["changed_vertices"] = len(hand["changed_local_vertices"])
        report["changed_vertices"] += hand["changed_vertices"]
        report["removed_cross_digit_mass"] += hand["removed_cross_digit_mass"]
    report.update(applied=report["changed_vertices"] > 0,
                  method="audited distal chain weights; proportional same-digit longitudinal blend retained",
                  source_arrays_unchanged=True, neutral_geometry_unchanged=True)
    return pieces, report
