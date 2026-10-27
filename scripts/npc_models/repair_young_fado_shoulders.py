"""Refit only the rejected Young Fado archive's connected shoulder caps.

The exact parent, GLB and fitted mesh are required. Existing resources remain
byte-identical except for the manifest; shoulder-only v1 skin lists are added.
This is an offline candidate builder, not evidence of runtime acceptance.
"""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import struct
from types import SimpleNamespace
import zipfile

import numpy as np
from scipy.sparse import coo_matrix, diags
from scipy.sparse.linalg import factorized, spsolve
from scipy.spatial import cKDTree

from native_assets import skeleton
from native_assets.path_guard import require_separate_destination
from native_npc_models import SourceModel, animation_frames, asset, read_limb, read_string, unit, wrapper
from native_npc_skin import Palette, STEPS
from verify_native_npc_models import Archive


PARENT_SHA256 = '8f4d8994fc8c7f29b460dc0217281e389b24db7684acc7b840c0472149127649'
SOURCE_SHA256 = '5c478e60657434a788e1d4d90eef9bfeec647cba2666be7f4f16f0d2524e0157'
MESH_SHA256 = '7de4be516a39d5c2e4b0be2b93bfd2656c30e1b26fcdbe97c82dc85f47668639'
NS = 'objects/object_fa/YoungFadoNpcPOC1'
CLIPS = (
    'gKokiriIdleAnim', 'gKokiriStandingArmsBehindBackAnim',
    'gKokiriStandingHandsOnHipsAnim', 'gKokiriSittingHeadOnHandAnim',
    'gKokiriSittingCrossedLegsAnim', 'gKokiriSittingCrossedArmsLegsAnim',
)
ALL_CLIPS = (*CLIPS, 'gKokiriLaughingAnim')
SIDES = (('Left', 30, 8), ('Right', 29, 11))


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class FadoRig:
    def __init__(self, archive):
        self.ns = NS
        raw = archive.read(NS+'/Skel')
        count, self.dlist_count = struct.unpack_from('<II', raw, 66)
        self.limbs, self.near, offset = [], [], 79
        for index in range(count):
            path, offset = read_string(raw, offset)
            fields, near, _ = read_limb(archive.read(path))
            self.limbs.append(skeleton.Limb(index, *fields, int(bool(near))))
            self.near.append(near)
        self.skel = skeleton.Skeleton(self.limbs, count, self.dlist_count)
        self.pose(np.zeros((count+1, 3)))
        if count != 15 or self.skel.slot_to_limb != list(range(15)):
            raise ValueError('The exact drawable native Kokiri hierarchy is required')

    def pose(self, frame):
        skeleton._pose(self.skel, np.asarray(frame).ravel().tolist())

    def matrices(self):
        result = np.tile(np.eye(4), (len(self.limbs), 1, 1))
        for index, limb in enumerate(self.limbs):
            result[index, :3, :3], result[index, :3, 3] = limb.M, limb.T
        return result


def pose_frame(archive, pose):
    path = NS+'/Animations/Seated5' if pose == 5 else 'objects/object_os_anime/'+ALL_CLIPS[pose]
    frames = animation_frames(archive.read(path))
    # The shared fallback must match the actual native Fado idle bind frame.
    return frames[0].copy()


def source_ids(data, mesh, part, mesh_ids, bone_map):
    """Match read-back atlas coordinates to the recovered source cap vertices."""
    lookup = defaultdict(list)
    uv = np.rint(mesh['uv']*16384).astype(int)
    for index in np.flatnonzero(mesh_ids == part):
        lookup[(tuple(uv[index]), bone_map[int(mesh['bones'][index])])].append(int(index))
    result = []
    for uv, bone in zip(np.rint(data['uv']*16384).astype(int), data['bones']):
        ids = lookup[(tuple(uv), int(bone))]
        if not ids or np.ptp(mesh['positions'][ids], axis=0).max() > 1e-4:
            raise ValueError('A shoulder vertex cannot be uniquely matched to its source position')
        result.append(ids[0])
    return np.asarray(result)


def cap_topology(source, mesh, mesh_ids, side, part):
    sp = np.concatenate([piece['positions'] for piece in source.meshes])
    arm = np.flatnonzero(mesh_ids == part)
    garment = np.flatnonzero(mesh_ids == 11)
    distance, _ = cKDTree(sp[garment]).query(sp[arm])
    contact = arm[distance < 1e-5]
    joints = {name: source.world[index][:3, 3] for name, index in zip(source.names, source.joints)}
    axis = unit(joints[side+'ForeArm']-joints[side+'Arm'])
    along = (sp-joints[side+'Arm'])@axis
    # Stop well before the elbow. The first untouched source edge is retained
    # verbatim, making the new cap a replacement surface rather than a plug.
    cut = .075
    faces = mesh['faces'][(mesh_ids[mesh['faces']] == part).all(1)]
    faces = faces[(along[faces] < cut).any(1)]
    ids = np.unique(faces)
    _, group = np.unique(np.round(sp[ids], 5), axis=0, return_inverse=True)
    representative = np.array([ids[np.flatnonzero(group == i)[0]] for i in range(group.max()+1)])
    inverse = dict(zip(map(int, ids), map(int, group)))
    f = np.array([[inverse[int(i)] for i in face] for face in faces])
    if np.any(np.diff(np.sort(f, axis=1), axis=1) == 0):
        raise ValueError('The source cap has a degenerate welded triangle')
    edge = np.unique(np.sort(np.vstack([f[:, [0, 1]], f[:, [1, 2]], f[:, [2, 0]]]), axis=1), axis=0)
    a, b = np.r_[edge[:, 0], edge[:, 1]], np.r_[edge[:, 1], edge[:, 0]]
    adjacency = coo_matrix((np.ones(len(a)), (a, b)), shape=(len(representative),)*2).tocsr()
    laplacian = diags(np.asarray(adjacency.sum(1)).ravel())-adjacency
    seam = np.array([np.isin(ids[group == i], contact).any() for i in range(len(representative))])
    distal = along[representative] >= cut
    fixed = seam | distal
    if not seam.any() or not distal.any() or np.any(seam & distal):
        raise ValueError('The source cap needs distinct garment and upper-arm boundaries')
    t = np.zeros(len(representative)); t[distal] = 1
    free = ~fixed
    t[free] = spsolve(laplacian[free][:, free], -laplacian[free][:, fixed]@t[fixed])
    return SimpleNamespace(ids=ids, group=group, representative=representative, faces=f,
                           source=sp[representative], seam=seam, distal=distal,
                           fixed=fixed, free=free, a=a, b=b, laplacian=laplacian,
                           weight=np.clip(t, 0, 1), inverse=inverse)


def fit_cap(topology, target):
    """ARAP deformation of the source cap with both real surface edges fixed."""
    t = topology
    a, b = t.source[t.seam], target[t.seam]
    ac, bc = a.mean(0), b.mean(0)
    u, singular, vt = np.linalg.svd((a-ac).T@(b-bc))
    rotation = vt.T@u.T
    if np.linalg.det(rotation) < 0:
        vt[-1] *= -1
        rotation = vt.T@u.T
    scale = singular.sum()/np.square(a-ac).sum()
    reference = (t.source-ac)@rotation.T*scale+bc
    edges = reference[t.a]-reference[t.b]
    solve = factorized(t.laplacian[t.free][:, t.free].tocsc())
    boundary = t.laplacian[t.free][:, t.fixed]@target[t.fixed]
    points = reference.copy(); points[t.fixed] = target[t.fixed]
    rotations = np.tile(np.eye(3), (len(points), 1, 1))
    for _ in range(100):
        covariance = np.zeros((len(points), 3, 3))
        np.add.at(covariance, t.a, np.einsum('ni,nj->nij', edges, points[t.a]-points[t.b]))
        uu, _, vv = np.linalg.svd(covariance)
        rotations = vv.transpose(0, 2, 1)@uu.transpose(0, 2, 1)
        flipped = np.linalg.det(rotations) < 0
        vv[flipped, 2] *= -1
        rotations = vv.transpose(0, 2, 1)@uu.transpose(0, 2, 1)
        rhs = np.zeros_like(points)
        np.add.at(rhs, t.a, np.einsum('nij,nj->ni', .5*(rotations[t.a]+rotations[t.b]), edges))
        points[t.free] = solve(rhs[t.free]-boundary)
    return points, rotations, rotation


def preserve_orientation(points, normals, faces, fixed):
    """Project the small residual ARAP folds outward, retaining both boundaries.

    ARAP alone has no signed-area constraint. A sharp bend can leave a few
    underside triangles folded even after the large rigid-fit fold is gone.
    This correction moves surface vertices; changing normals cannot hide it.
    """
    points = points.copy()
    expected = unit(normals[faces].mean(1))
    for _ in range(1000):
        triangle = points[faces]
        first, second = triangle[:, 1]-triangle[:, 0], triangle[:, 2]-triangle[:, 0]
        cross = np.cross(first, second)
        signed = (cross*expected).sum(1)
        required = np.maximum(np.linalg.norm(cross, axis=1)*.3, 30.)
        bad = np.flatnonzero(signed < required-1e-6)
        if not len(bad):
            return points
        for index in bad:
            a, b, c = faces[index]
            first, second = points[b]-points[a], points[c]-points[a]
            gb = np.cross(second, expected[index]); gc = np.cross(expected[index], first)
            gradient = np.array([-gb-gc, gb, gc])
            gradient[fixed[[a, b, c]]] = 0
            magnitude = np.square(gradient).sum()
            if magnitude < 1e-12:
                raise ValueError('The fixed shoulder boundary forces an inverted triangle')
            signed = np.cross(first, second)@expected[index]
            amount = max(0., required[index]-signed)/magnitude
            points[[a, b, c]] += gradient*amount
    raise ValueError('The cap orientation constraints did not converge')


def stabilize_clip(points, normals, topology, palette, rig, archive, pose, host):
    """Keep the same bounded cap outward through the native clip's motion.

    A single fitted frame is insufficient for laughter. Project its small
    residual folds in canonical skin space, so no animation or rig changes.
    """
    reference_matrices = palette.matrices(rig.matrices())
    slots = np.array([7 if q == 0 else host if q == STEPS else
                      len(palette.slots)+palette.blends.index((7, host, q))
                      for q in np.rint(topology.weight*STEPS).astype(int)])
    ref = reference_matrices[slots]
    inverse = np.linalg.inv(ref)
    canonical = np.einsum('nij,nj->ni', inverse[:, :3, :3], points)+inverse[:, :3, 3]
    canonical_normal = unit(np.einsum('nij,nj->ni', inverse[:, :3, :3], normals))
    path = NS+'/Animations/Seated5' if pose == 5 else 'objects/object_os_anime/'+ALL_CLIPS[pose]
    frames = animation_frames(archive.read(path))
    transforms = []
    for index, frame in enumerate(frames):
        following = frames[(index+1) % len(frames)]
        delta = (following-frame+32768) % 65536-32768
        half = frame.astype(float)+delta*.5
        half[0] = (frame[0]+following[0])*.5
        for sample in (frame, half):
            rig.pose(sample)
            matrices = palette.matrices(rig.matrices())[slots]
            transforms.append((matrices, np.linalg.inv(matrices)))
    for _ in range(8):
        maximum = 0.
        for matrix, inverse in transforms:
            world = np.einsum('nij,nj->ni', matrix[:, :3, :3], canonical)+matrix[:, :3, 3]
            normal = unit(np.einsum('nij,nj->ni', matrix[:, :3, :3], canonical_normal))
            corrected = preserve_orientation(world, normal, topology.faces, topology.fixed)
            maximum = max(maximum, float(np.linalg.norm(corrected-world, axis=1).max()))
            canonical = np.einsum('nij,nj->ni', inverse[:, :3, :3], corrected)+inverse[:, :3, 3]
        # Vertices serialize to whole native units. Stop well below that
        # quantization rather than chasing alternating sub-millithresholds.
        if maximum < 1e-3:
            result = np.einsum('nij,nj->ni', ref[:, :3, :3], canonical)+ref[:, :3, 3]
            movement = float(np.linalg.norm(result-points, axis=1).max())
            if movement > 20:
                raise ValueError('Clip stabilization escaped the 20-unit shoulder correction bound')
            return result, movement
    raise ValueError('The native clip requires a different shoulder surface; no candidate was written')


def emit_records(output, path, parent_material, records, faces, host):
    """Copy the original material state and preserve untouched vertex records."""
    raw = parent_material
    offset = 72
    while offset < len(raw):
        first, _ = struct.unpack_from('<II', raw, offset)
        if first >> 24 == 0xDA:
            break
        offset += 16 if first >> 24 in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36) else 8
    d = asset.DLBuilder(); d.marker(path)
    # The first original command is its 16-byte debug marker.
    d.words.extend(struct.unpack('<'+'I'*((offset-88)//4), raw[88:offset]))
    groups = defaultdict(list)
    for face in faces:
        groups[tuple(sorted({records[int(i)][0] for i in face}))].append(face)
    vertices, current = [], None

    def flush(batch):
        nonlocal current
        unique = sorted({records[int(i)] for f in batch for i in f})
        lookup = {record: i for i, record in enumerate(unique)}
        for slot in sorted({record[0] for record in unique}):
            subset = [(i, record) for i, record in enumerate(unique) if record[0] == slot]
            if current != slot:
                d.raw(0xDA380003, 0x0D000001+slot*64); current = slot
            offset = len(vertices)*16
            vertices.extend([(*r[1:4], slot << 8, *r[4:]) for _, r in subset])
            d.vtx_hash(len(subset), subset[0][0], offset, path+'/V')
        triangles = [[lookup[records[int(i)]]*2 for i in f] for f in batch]
        for i in range(0, len(triangles), 2):
            a, b, c = triangles[i]; first = (a << 16) | (b << 8) | c
            if i+1 < len(triangles):
                a, b, c = triangles[i+1]; d.raw(0x06000000 | first, (a << 16) | (b << 8) | c)
            else:
                d.raw(0x05000000 | first, 0)

    for _, fs in sorted(groups.items()):
        batch, used = [], set()
        for face in fs:
            more = {records[int(i)] for i in face}
            if len(used | more) > 32:
                flush(batch); batch, used = [], set()
            batch.append(face); used |= more
        if batch:
            flush(batch)
    d.raw(0xDA380003, 0x0D000001+host*64); d.end()
    output.add(path, d.to_resource())
    output.add(path+'/V', asset.build_vertex_array(vertices))


def build(native, parent, source_path, fitted_mesh, output):
    native, parent, source_path, fitted_mesh, output = map(Path, (native, parent, source_path, fitted_mesh, output))
    require_separate_destination(output, native, parent, source_path, fitted_mesh)
    for path, expected in ((parent, PARENT_SHA256), (source_path, SOURCE_SHA256), (fitted_mesh, MESH_SHA256)):
        if sha256(path) != expected:
            raise ValueError(f'Input hash mismatch: {path.name}')
    source = SourceModel(source_path)
    mesh = dict(np.load(fitted_mesh, allow_pickle=False))
    mesh_ids = np.concatenate([np.full(len(piece['positions']), i) for i, piece in enumerate(source.meshes)])
    if len(mesh['positions']) < len(mesh_ids):
        raise ValueError('The recovered fitted mesh is shorter than its GLB')
    mesh_ids = np.pad(mesh_ids, (0, len(mesh['positions'])-len(mesh_ids)), constant_values=-1)
    archive = Archive(native, parent); rig = FadoRig(archive)
    records = {path: archive.candidate.read(path) for path in archive.candidate.namelist()}
    manifest = json.loads(records['YoungFadoNpcInfo.json'])
    bone_map = {int(a): int(b) for a, b in manifest['vertex_bone_mapping'].items()}
    rig.pose(pose_frame(archive, 0)); bind = rig.matrices()
    weights = np.zeros((STEPS*2, len(rig.limbs)))
    for side_index, (_, _, host) in enumerate(SIDES):
        for q in range(1, STEPS):
            weights[side_index*STEPS+q, 7] = 1-q/STEPS
            weights[side_index*STEPS+q, host] = q/STEPS
        weights[side_index*STEPS, 7] = 1
    palette = Palette(bind, rig.skel.slot_to_limb, weights)
    slot_for = {(7, host, q): 7 if q == 0 else host if q == STEPS else
                len(palette.slots)+palette.blends.index((7, host, q))
                for _, _, host in SIDES for q in range(STEPS+1)}
    out = asset.O2R(); out.add('alt/'+NS+'/Skin/Palette', palette.resource())
    details = []
    for side, part, host in SIDES:
        original_path = NS+f'/Body/Limb{host:02d}/Mat00'
        data = archive.geometry(original_path, rig, host)
        ids = source_ids(data, mesh, part, mesh_ids, bone_map)
        topology = cap_topology(source, mesh, mesh_ids, side, part)
        source_to_record = {int(source_id): i for i, source_id in enumerate(ids)}
        if any(int(i) not in source_to_record for i in topology.representative):
            raise ValueError('The bounded cap escaped its original upper-arm display list')
        source_normals = np.concatenate([piece['normals'] for piece in source.meshes])
        for pose, clip in enumerate(ALL_CLIPS):
            rig.pose(pose_frame(archive, pose)); matrices = rig.matrices(); skin_matrices = palette.matrices(matrices)
            world = np.zeros_like(data['local']); normal = np.zeros_like(data['normals'])
            for bone in np.unique(data['bones']):
                mask = data['bones'] == bone; matrix = matrices[bone]
                world[mask] = data['local'][mask]@matrix[:3, :3].T+matrix[:3, 3]
                normal[mask] = unit(data['normals'][mask]@matrix[:3, :3].T)
            target = world[[source_to_record[int(i)] for i in topology.representative]]
            points, rotations, source_rotation = fit_cap(topology, target)
            normals = unit(np.einsum('nij,nj->ni', rotations,
                           source_normals[topology.representative]@source_rotation.T))
            normals[topology.fixed] = normal[[source_to_record[int(i)] for i in topology.representative[topology.fixed]]]
            points = preserve_orientation(points, normals, topology.faces, topology.fixed)
            points, motion_correction = stabilize_clip(points, normals, topology, palette, rig, archive, pose, host)
            emitted = []
            for index, (local, n, uv, bone, source_id) in enumerate(zip(data['local'], data['normals'], data['uv'], data['bones'], ids)):
                slot = int(bone)
                if int(source_id) in topology.inverse:
                    group = topology.inverse[int(source_id)]
                    if topology.free[group]:
                        q = int(np.rint(topology.weight[group]*STEPS)); slot = slot_for[(7, host, q)]
                        matrix = skin_matrices[slot]
                        inverse = np.linalg.inv(matrix)
                        local = points[group]@inverse[:3, :3].T+inverse[:3, 3]
                        n = unit(normals[group]@inverse[:3, :3].T)
                xyz = np.rint(local).astype(int); nn = np.rint(n*127).astype(int); st = np.rint(uv*16384).astype(int)
                if max(abs(xyz)) > 32767:
                    raise ValueError('The fitted shoulder exceeds signed native vertex coordinates')
                emitted.append((slot, *map(int, xyz), *map(int, st), *map(int, nn), 255))
            prefix = NS+'/PoseAnimations/objects/object_os_anime/'+clip+'/Skin/Limbs'
            material = prefix+f'/Limb{host:02d}/Shoulder'
            emit_records(out, material, records[original_path], emitted, data['faces'], host)
            wrapper(out, 'alt/'+prefix+f'/Limb{host:02d}', [material])
            if pose == 0:
                wrapper(out, 'alt/'+NS+f'/Skin/Limbs/Limb{host:02d}', [material])
            details.append(dict(side=side, clip=clip, source_cap_vertices=len(topology.representative),
                                refitted_vertices=int(topology.free.sum()), seam_vertices=int(topology.seam.sum()),
                                distal_boundary_vertices=int(topology.distal.sum()), source_cap_triangles=len(topology.faces),
                                total_upper_arm_triangles=len(data['faces']), geometry='same connected source faces'))
            details[-1]['maximum_clip_stabilization_units'] = motion_correction
    report = dict(candidate=output.name, parent_sha256=PARENT_SHA256, native_sha256=sha256(native),
                  source_sha256=SOURCE_SHA256, fitted_mesh_sha256=MESH_SHA256,
                  new_resources=sorted(out.files), shoulder_caps=details,
                  runtime_tested=False, accepted=False, promoted=False,
                  limitation='The rejected parent is retained; all poses and live animations require in-game review.')
    manifest['shoulder_repair'] = report
    records.update(out.files); records['YoungFadoNpcInfo.json'] = json.dumps(manifest, indent=2).encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for path, raw in sorted(records.items()):
            info = zipfile.ZipInfo(path, (2026, 10, 20, 0, 0, 0)); info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, raw)
    archive.native.close(); archive.candidate.close()
    report['candidate_sha256'] = sha256(output)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('native', 'parent', 'source', 'fitted_mesh', 'output'):
        parser.add_argument(name, type=Path)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    if args.report:
        require_separate_destination(args.report, args.native, args.parent, args.source,
                                     args.fitted_mesh, args.output)
    report = build(args.native, args.parent, args.source, args.fitted_mesh, args.output)
    text = json.dumps(report, indent=2)+'\n'
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(text)
    print(text)


if __name__ == '__main__':
    main()
