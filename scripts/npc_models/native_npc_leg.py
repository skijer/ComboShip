"""Omit the audited Traditional Saria shorts fin hanging over her rear thigh.

This source artifact is garment geometry, not a leg-normal seam. Only the
52 diagnosed fin and neck triangles are omitted. Four triangles restore the
original hem using existing garment vertices; every vertex field and the
separately enclosed, invisible shorts debris remain unchanged. Call on a
fresh fitted model before skin palette and display-list export.
"""
import hashlib

import numpy as np


TRADITIONAL_SOURCE_SHA256 = '902dfca93f4640a65a72e4266f855966fa708b412e0742789552f55f85c94aff'
VISIBLE_FIN_SOURCE_MESH_FACES = tuple(range(2068, 2120))
# These are the garment-chart UV copies at the fin root, ordered downward.
HEM_RING_SOURCE_MESH_VERTICES = (1158, 1175, 1162, 1120, 1105, 1116)
HEM_CAP_SOURCE_MESH_VERTEX_TRIANGLES = (
    (1158, 1175, 1162), (1158, 1162, 1120),
    (1158, 1120, 1105), (1158, 1105, 1116),
)


def remove_traditional_saria_leg_fin(source, model, rig):
    """Remove only the source-gated visible fin; return its exact face report.

    Existing removals in other source pieces are supported. The selected
    shorts triangles must still have their original source ordering and
    vertex correspondence, otherwise the operation fails before mutation.
    """
    if getattr(rig, 'name', None) != 'saria':
        return None
    digest = hashlib.sha256(getattr(source, 'raw', b'')).hexdigest()
    if digest != TRADITIONAL_SOURCE_SHA256:
        return None

    mesh_id = 21
    if len(source.meshes) <= mesh_id or source.meshes[mesh_id].get('name') != 'tripo_part_6':
        raise ValueError('The audited Traditional Saria shorts piece changed')
    mesh = source.meshes[mesh_id]
    local_faces = np.asarray(mesh['faces'])
    if len(local_faces) != 2304:
        raise ValueError('The audited Traditional Saria shorts face count changed')
    # Faces 2120–2303 are a disconnected internal component; no visibility
    # evidence calls for removing it. This box includes the complete narrow
    # fin up to its six-edge hem root, excluding every regular hem neighbor.
    lower = np.array([-.066, .410, -.0611])
    upper = np.array([-.059, .4492, -.047])
    points = mesh['positions'][local_faces]
    visible_fin = ((np.arange(len(local_faces)) < 2120) &
                   ((points > lower) & (points < upper)).all((1, 2)))
    local_ids = np.flatnonzero(visible_fin)
    if local_ids.tolist() != list(VISIBLE_FIN_SOURCE_MESH_FACES):
        raise ValueError('The audited visible rear-shorts fin mask changed')
    local_cap = np.asarray(HEM_CAP_SOURCE_MESH_VERTEX_TRIANGLES, int)
    cap_points = mesh['positions'][local_cap]
    cap_cross = np.cross(cap_points[:, 1]-cap_points[:, 0],
                         cap_points[:, 2]-cap_points[:, 0])
    if not (cap_cross[:, 1] < 0).all():
        raise ValueError('The audited rear-shorts hem cap winding changed')

    vertex_offset = sum(len(piece['positions']) for piece in source.meshes[:mesh_id])
    source_face_offset = sum(len(piece['faces']) for piece in source.meshes[:mesh_id])
    faces = np.asarray(model['faces'])
    shorts_rows = np.flatnonzero((model['mesh_ids'][faces] == mesh_id).all(1))
    if not np.array_equal(faces[shorts_rows], local_faces+vertex_offset):
        raise ValueError('Fitted shorts no longer match their audited source face order')
    hosts = np.asarray(model['hosts'])
    if len(hosts) != len(faces):
        raise ValueError('Fitted shorts face ownership is misaligned')

    removed_rows = shorts_rows[local_ids]
    keep = np.ones(len(faces), bool)
    keep[removed_rows] = False
    cap = local_cap+vertex_offset
    cap_hosts = np.array([np.bincount(model['bones'][face]).argmax() for face in cap],
                         dtype=hosts.dtype)
    bounds = mesh['positions'][np.unique(local_faces[local_ids])]
    report = dict(
        source_sha256=digest, source_mesh_id=mesh_id,
        source_mesh_name=mesh['name'], removed_triangles=len(local_ids),
        added_triangles=len(local_cap),
        removed_source_faces=(local_ids+source_face_offset).tolist(),
        removed_source_mesh_faces=local_ids.tolist(),
        added_source_mesh_vertex_triangles=local_cap.tolist(),
        added_source_vertex_triangles=cap.tolist(),
        hem_ring_source_mesh_vertices=list(HEM_RING_SOURCE_MESH_VERTICES),
        cap_normals_downward=True,
        removed_source_bounds=dict(min=bounds.min(0).tolist(), max=bounds.max(0).tolist()),
        mask=dict(source_mesh_id=mesh_id, local_face_index_lt=2120,
                  all_source_vertices_inside=dict(min=lower.tolist(), max=upper.tolist())),
        vertices_unchanged=True, uvs_unchanged=True, weights_unchanged=True,
        normals_unchanged=True, internal_debris_preserved=True,
        reason='complete narrow rear-shorts fin and neck; original hem closed with existing garment vertices')
    model['faces'] = np.concatenate([faces[keep], cap])
    model['hosts'] = np.concatenate([hosts[keep], cap_hosts])
    return report
