"""A bounded geometry-only lift of Anime Saria's neutral mouth corners."""
import hashlib
import numpy as np

ANIME_SHA256 = 'dddeaed512e7b4aaedf960e1ddab13e74c480897e0f44404fb980c0c9ce5d63a'


def _smooth(x):
    x = np.clip(x, 0., 1.)
    return x*x*(3.-2.*x)


def _lift(points):
    x, y, z = np.asarray(points, float).T
    # The closed mouth and cheek immediately around each corner move together.
    # The central lip, nose, eye region, silhouette and all back surfaces stay.
    return (.0045*_smooth((abs(x)-.005)/.012)*_smooth((.037-abs(x))/.014)*
            _smooth((.017-abs(y-.815))/.011)*_smooth((z-.025)/.015))


def smile_warp(points, normals):
    points, normals = np.asarray(points, float), np.asarray(normals, float)
    revised = points.copy()
    revised[:, 1] += _lift(points)
    jacobian = np.tile(np.eye(3), (len(points), 1, 1))
    for axis in range(3):
        epsilon = np.eye(3)[axis]*1e-6
        jacobian[:, 1, axis] += (_lift(points+epsilon)-_lift(points-epsilon))/(2e-6)
    normal = np.einsum('ni,nij->nj', normals, np.linalg.inv(jacobian))
    normal /= np.maximum(np.linalg.norm(normal, axis=1, keepdims=True), 1e-12)
    return revised, normal


def warm_anime_smile(source, model, rig):
    if getattr(rig, 'name', None) != 'saria' or hashlib.sha256(getattr(source, 'raw', b'')).hexdigest() != ANIME_SHA256:
        return None
    p = model['source_positions']
    ids = np.flatnonzero((model['mesh_ids'] == 0) & (_lift(p) > 0))
    if not len(ids):
        raise ValueError('The audited Anime Saria mouth-corner surface is missing')
    transform = model['head_transform'][:, :3]
    # Work in source coordinates, then map both positions and normal covectors
    # through the unchanged head transform. Source UVs and diffuse are retained.
    normal = model['normals'][ids]@transform
    normal /= np.linalg.norm(normal, axis=1, keepdims=True)
    revised, normal = smile_warp(p[ids], normal)
    displacement = revised-p[ids]
    model['positions'][ids] += displacement@transform.T
    model['posed_source_positions'][ids] += displacement
    model['posed_source_normals'][ids] = normal
    fitted_normal = normal@np.linalg.inv(transform)
    model['normals'][ids] = fitted_normal/np.linalg.norm(fitted_normal, axis=1, keepdims=True)
    return dict(source_sha256=ANIME_SHA256, mouth_corner_vertices=len(ids),
                max_source_lift=float(displacement[:, 1].max()),
                source_uvs_preserved=True, source_diffuse_preserved=True,
                head_transform_preserved=True, mouth_center_preserved=True)
