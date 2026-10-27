"""Repair young Malon's clasp using her supplied concept artwork and relief.

The malformed source clasp and the scarf tails share two source components.
Only their clasp-height faces are replaced.  The reference crop remains intact;
its full horns, forehead, muzzle and jaw define a closed, chest-bound object.
"""
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image
from scipy.spatial import Delaunay

from native_npc_models import asset, emit_mesh, wrapper, unit

_ART = Path(__file__).with_name('brooch_art')
_CLASP_BOTTOM = .662


def source_mask(source, model):
    """Remove complete old clasp triangles, including those crossing its join.

    A centroid test left the lower jaw and long pointed triangles visible.
    These dedicated source pieces contain the clasp and the tails; the latter
    remain below the join. The collar's inner tab faces are re-emitted whole
    with a small smooth outward adjustment; the fold, UVs and skin are retained.
    """
    pieces = [i for i, mesh in enumerate(source.meshes)
              if mesh.get('name') in ('tripo_part_11', 'tripo_part_15')]
    p = model['source_positions'][model['faces']]
    clasp = (np.isin(model['mesh_ids'][model['faces']], pieces).all(1) &
             (p[:, :, 1].max(1) >= _CLASP_BOTTOM))
    return clasp | _collar_join_mask(source, model)


def _join_offset(p, side):
    """Move the inner scarf tab outward without opening or cutting its fold."""
    x = p[:, 0]*side
    u = np.clip((x-.020)/.020, 0, 1)
    wx = 1-u*u*(3-2*u)
    v = (p[:, 1]-.739)/.015
    wy = np.maximum(1-v*v, 0)**2
    wy[abs(v) >= 1] = 0
    return side*.0085*wx*wy


def _collar_offsets(source, model):
    result = np.zeros(len(model['source_positions']))
    for i, mesh in enumerate(source.meshes):
        if mesh.get('name') not in ('tripo_part_12', 'tripo_part_14'):
            continue
        side = -1 if mesh['name'] == 'tripo_part_12' else 1
        selected = model['mesh_ids'] == i
        result[selected] = _join_offset(model['source_positions'][selected], side)
    return result


def _collar_join_mask(source, model):
    offset = _collar_offsets(source, model)
    return (abs(offset[model['faces']]) > 1e-12).any(1)


def collar_join_geometry(source, model):
    """Re-emit complete scarf faces with the two inner tabs gently rounded out.

    The original UVs, face topology, low join and native owners remain intact.
    Compact smooth displacement leaves the outer fold and upper neckline exact.
    This avoids exposing the inside of the thick scarf with a geometric cut.
    """
    offset = _collar_offsets(source, model)
    chosen = (abs(offset[model['faces']]) > 1e-12).any(1)
    ids = model['faces'][chosen].ravel()
    p = model['source_positions'][ids]
    delta = offset[ids]
    a = model['head_transform'][:, :3]
    result = {name: model[name][ids].copy() for name in
              ('positions', 'source_positions', 'normals', 'uv', 'bones')}
    result['positions'] += delta[:, None]*a[:, 0]
    # Apply the same smooth deformation to the source normal covectors.
    grad = np.zeros((len(ids), 3))
    side = np.array([-1 if source.meshes[int(i)].get('name') == 'tripo_part_12' else 1
                     for i in model['mesh_ids'][ids]])
    for sign in (-1, 1):
        mask = side == sign
        for axis in (0, 1):
            d = np.zeros(3);d[axis] = 1e-6
            grad[mask, axis] = (_join_offset(p[mask]+d, sign)-_join_offset(p[mask]-d, sign))/2e-6
    jacobian = np.eye(3)[None]+np.einsum('i,vj->vij', a[:, 0], grad@np.linalg.inv(a))
    result['normals'] = unit(np.einsum('vi,vij->vj', result['normals'], np.linalg.inv(jacobian)))
    result['faces'] = np.arange(len(ids)).reshape(-1, 3)
    return result


def reference_art():
    """Read the unpainted concept crop and its reproducible source provenance."""
    pixels = np.asarray(Image.open(_ART/'young-malon-reference.png').convert('RGBA'))
    report = json.loads((_ART/'young-malon-reference.json').read_text())
    if hashlib.sha256(pixels.tobytes()).hexdigest() != report['crop_pixel_sha256']:
        raise ValueError('The recorded young Malon concept crop has changed')
    return pixels, report


def _inside(points, outline):
    """Even/odd point-in-polygon test, including points on the traced edge."""
    points = np.atleast_2d(points)
    x, y = points[:, 0], points[:, 1]
    result = np.zeros(len(points), bool)
    edge = np.zeros(len(points), bool)
    for a, b in zip(outline, np.roll(outline, -1, axis=0)):
        dx, dy = b-a
        cross = (x-a[0])*dy-(y-a[1])*dx
        dot = (x-a[0])*dx+(y-a[1])*dy
        edge |= (abs(cross) < 1e-7) & (dot >= -1e-7) & (dot <= dx*dx+dy*dy+1e-7)
        if abs(dy) > 1e-10:
            result ^= ((a[1] > y) != (b[1] > y)) & (x < dx*(y-a[1])/dy+a[0])
    return result | edge


def _relief(points):
    """Shallow rounded anatomical lobes following the reference's gold relief."""
    x, y = points[:, 0]-43., points[:, 1]-48.

    def lobe(cx, cy, rx, ry, height):
        return height*np.exp(-2*(((x-cx)/rx)**2+((y-cy)/ry)**2))

    # The mouth and eyes stay recessed behind paired muzzle/brow lobes.  The
    # artwork supplies the original details; this adds their actual depth.
    z = .116+lobe(0, 0, 43, 48, .003)
    z += lobe(0, -31, 9, 20, .013)  # Center forehead crest.
    for sign in (-1, 1):
        z += lobe(sign*25, -31, 7, 15, .008)  # Horns.
        z += lobe(sign*16, -18, 18, 8, .011)  # Brows.
        z += lobe(sign*30, 4, 13, 20, .008)  # Cheeks.
        z += lobe(sign*12, 8, 20, 13, .017)  # Rounded muzzle.
        z += lobe(sign*21, 31, 12, 15, .009)  # Raised jaw corners.
    jaw_y = 30+9*(abs(x)/30)**1.5
    z += .009*np.exp(-2*((y-jaw_y)/8)**2)*np.exp(-(x/36)**6)
    z += lobe(0, 43, 13, 7, .006)  # Chin beneath the open mouth.
    return z


def clasp_geometry():
    """Return a watertight reference-shaped relief in source chest coordinates."""
    pixels, report = reference_art()
    outline = np.asarray(report['silhouette_pixel_coordinates'], float)
    # Densify the exact traced contour before triangulating, so narrow horns
    # cannot be bridged by a broad triangle across their background valleys.
    contour = []
    for a, b in zip(outline, np.roll(outline, -1, axis=0)):
        n = max(1, int(np.ceil(np.linalg.norm(b-a)/1.5)))
        contour.extend(a+(b-a)*np.arange(n)[:, None]/n)
    contour = np.asarray(contour)
    yy, xx = np.mgrid[2:pixels.shape[0]-2:3.5, 2:pixels.shape[1]-2:3.5]
    grid = np.column_stack([xx.ravel(), yy.ravel()])
    xy = np.vstack([contour, grid[_inside(grid, outline)]])
    faces = Delaunay(xy).simplices
    samples = np.concatenate([xy[faces].mean(1)[:, None, :],
                              (xy[faces]+np.roll(xy[faces], -1, axis=1))/2], axis=1)
    faces = faces[_inside(samples.reshape(-1, 2), outline).reshape(-1, 4).all(1)]
    # Pixel Y points down; source Y points up.  Reverse winding to face +Z.
    e1, e2 = xy[faces[:, 1]]-xy[faces[:, 0]], xy[faces[:, 2]]-xy[faces[:, 0]]
    cross = e1[:, 0]*e2[:, 1]-e1[:, 1]*e2[:, 0]
    faces = faces[abs(cross) > 1e-8]
    cross = cross[abs(cross) > 1e-8]
    faces[cross > 0] = faces[cross > 0, ::-1]
    scale = report['source_units_per_pixel']
    origin = np.asarray(report['source_pixel_origin'])-np.asarray(report['reference_crop'][:2])
    # Roll the relief down to a thin rim, rather than extruding the whole
    # painted face as a deep slab. The contour remains the exact reference.
    distance = np.full(len(xy), np.inf)
    for pa, pb in zip(outline, np.roll(outline, -1, axis=0)):
        ab = pb-pa
        q = np.clip(((xy-pa)@ab)/np.dot(ab, ab), 0, 1)
        distance = np.minimum(distance, np.linalg.norm(xy-pa-q[:, None]*ab, axis=1))
    bevel = np.clip(distance/3., 0, 1)
    bevel = bevel*bevel*(3-2*bevel)
    depth = .1185+np.maximum(_relief(xy)-.1185, 0)*bevel
    front = np.column_stack([(xy[:, 0]-origin[0])*scale,
                             report['source_center'][1]-(xy[:, 1]-origin[1])*scale,
                             depth])
    back = front.copy()
    back[:, 2] = .117
    n = len(front)
    edges = np.sort(np.concatenate([faces[:, [0, 1]], faces[:, [1, 2]], faces[:, [2, 0]]]), axis=1)
    boundary, counts = np.unique(edges, axis=0, return_counts=True)
    boundary = boundary[counts == 1]
    directed = {(int(a), int(b)) for f in faces for a, b in zip(f, np.roll(f, -1))}
    p = list(np.vstack([front, back]))
    st = xy/np.array([pixels.shape[1]-1, pixels.shape[0]-1])
    uv = list(np.vstack([st, st]))
    side = []
    # The rim is metal, so it samples a gold source texel instead of stretching
    # the contour's skin/scarf/white border pixels down the object's thickness.
    gold = np.array([37., 28.])/[pixels.shape[1]-1, pixels.shape[0]-1]
    for a, b in boundary:
        if (int(a), int(b)) not in directed:
            a, b = b, a
        i = len(p)
        p.extend([front[b], front[a], back[a], back[b]])
        uv.extend([gold]*4)
        side.extend([[i, i+1, i+2], [i, i+2, i+3]])
    p = np.asarray(p)
    uv = np.asarray(uv)
    all_faces = np.vstack([faces, faces[:, ::-1]+n, np.asarray(side)])
    normals = np.zeros_like(p)
    fn = np.cross(p[all_faces[:, 1]]-p[all_faces[:, 0]], p[all_faces[:, 2]]-p[all_faces[:, 0]])
    for j in range(3):
        np.add.at(normals, all_faces[:, j], fn)
    return dict(positions=p, normals=unit(normals), uv=uv, faces=all_faces)


def seat_clasp(geometry):
    """Nest the clasp against the scarf's sloping front, keeping its outline.

    The upper fold recedes toward the neckline. A constant forward depth
    left a large gap there. Seat that edge farther back while keeping the
    lower jaw close to the existing scarf-tail join. This affine seat keeps
    the complete frontal silhouette, UVs, topology and relief detail intact.
    """
    result=dict(geometry)
    p=geometry['positions'].copy()
    p[:,2]-=.005+.5*(p[:,1]-.662)
    jacobian=np.eye(3);jacobian[2,1]=-.5
    result['positions']=p
    result['normals']=unit(geometry['normals']@np.linalg.inv(jacobian))
    return result


def export_brooch(archive, donor, source, model, rig, texpath):
    selected = source_mask(source, model)
    removed = np.flatnonzero(selected)
    chest = rig.profile['chest']
    keep = model['faces'][(model['hosts'] == chest) & ~selected]
    base = rig.ns+'/Body/ChestWithoutBrooch'
    emit_mesh(archive, 'alt/'+base, model, rig, keep, texpath, chest)
    pixels, provenance = reference_art()
    # Keep the established 512×512 texture interface. This resamples the same
    # complete reference image; it does not project cropped donor triangles.
    pixels = np.asarray(Image.fromarray(pixels).resize((512, 512), Image.Resampling.LANCZOS))
    texture = rig.ns+'/Textures/BowserBrooch'
    archive.add('alt/'+texture, asset._header(asset.T_OTEX, 1)+
                struct.pack('<IIIIffI', 1, 512, 512, 1, 1., 1., pixels.nbytes)+pixels.tobytes())
    geometry = seat_clasp(clasp_geometry())
    a = model['head_transform'][:, :3]
    hip = source.world[source.joints[source.names.index('Hips')]][:3, 3]
    t = rig.limbs[rig.profile['hips']].T-a@hip
    geometry['positions'] = geometry['positions']@a.T+t
    geometry['normals'] = unit(geometry['normals']@np.linalg.inv(a))
    geometry['bones'] = np.full(len(geometry['positions']), chest)
    badge = rig.ns+'/Body/BowserBrooch'
    emit_mesh(archive, 'alt/'+badge+'/Relief', geometry, rig, geometry['faces'], texture, chest)
    collar = collar_join_geometry(source, model)
    emit_mesh(archive, 'alt/'+badge+'/ScarfJoin', collar, rig, collar['faces'], texpath, chest)
    wrapper(archive, 'alt/'+badge, [badge+'/ScarfJoin', badge+'/Relief'])
    wrapper(archive, 'alt/'+rig.ns+f'/Body/Limb{chest:02d}', [base, badge])
    added = len(geometry['faces'])+len(collar['faces'])
    return dict(removed_source_faces=removed.tolist(), added_triangles=added,
                relief_triangles=len(geometry['faces']), retained_collar_triangles=len(collar['faces']),
                collar_join_method='complete original scarf faces/UVs; compact smooth outward tab adjustment, max .0085 source units',
                donor_variant='malon', donor_source_sha256=hashlib.sha256(donor.raw).hexdigest(),
                artwork_method='unaltered supplied young Malon concept crop on a complete closed Bowser relief',
                scarf_seat=dict(source_depth_pull='.005 + .5 * (source_y - .662)',
                                frontal_outline_and_uvs_preserved=True,normal_covectors_adjusted=True),
                artwork_provenance=provenance,
                artwork_pixel_sha256=hashlib.sha256(pixels.tobytes()).hexdigest(),
                body_triangles=int(np.count_nonzero(model['hosts'] == chest)-len(removed)+added))
