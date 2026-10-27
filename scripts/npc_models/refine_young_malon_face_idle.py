"""Bounded child Malon eye-surface, skin-shading and vanilla idle repair.

Operate on the exact POC8 pack. No body fitting, hand remeshing, shoulder
rebuilding or replacement of the other two pose packs is performed here.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import numpy as np
from PIL import Image
from scipy.ndimage import binary_fill_holes, distance_transform_edt, map_coordinates
from scipy.spatial import cKDTree

from native_npc_models import NativeRig, SourceModel, animation_frames, asset, emit_mesh, fit_model, unit
from native_npc_face import expression_subdivision, reproject_face_paint
from verify_native_npc_models import Archive

PARENT = 'da1f2ae86efdaef03897f73b06621a4aa9bf451c80c2f71e19372e2fba74f21f'
SOURCE = '3160899f85770bcd86f664ed73fd7d12277eea2905f42933e79be150e0d2cd3b'
IDLE = 'objects/object_ma1/gMalonChildIdleAnim'
BOUNDS = [-.095, .768, .095, .920]
EYES = (-.032, .030)


def smooth(value):
    value = np.clip(value, 0., 1.)
    return value*value*(3.-2.*value)


def aperture_coordinates(points, aperture):
    """Sample the visible painted aperture, independently of an eye ellipse."""
    if np.asarray(aperture).shape != (512, 512):
        raise ValueError('Expected the source-registered 512-square aperture')
    p = np.asarray(points)
    return [(.920-p[:, 1])/.152*511, (p[:, 0]+.095)/.190*511]


def eye_curves(points, aperture):
    """Fit the eyeball's own depth, keeping the surrounding orbital seat."""
    p = np.asarray(points)
    inside = map_coordinates(np.asarray(aperture, float), aperture_coordinates(p, aperture),
                             order=1, mode='constant') > .5
    curves = []
    for center in EYES:
        x, y = (p[:, 0]-center)/.024, (p[:, 1]-.855)/.018
        chosen = inside&(abs(p[:, 0]-center) < .025)&(p[:, 2] > .05)
        if chosen.sum() < 8:
            raise ValueError('Eye has insufficient visible aperture samples')
        design = np.column_stack([np.ones(len(p)), x, y])
        coef = np.zeros(6)
        coef[:3] = np.linalg.lstsq(design[chosen], p[chosen, 2], rcond=None)[0]
        # A shallow lens at the original iris seat removes the interior
        # facets. Fitting a forehead/cheek plane instead filled the socket
        # and moved skin outside the eye, producing the POC9 mask effect.
        coef[0] += .003
        coef[3] = coef[4] = -.003
        curves.append(coef)
    return np.asarray(curves)


def repair_eye_depth(points, curves=None, aperture=None):
    """Retain every skin/rim point; repair only the actual eye interior."""
    p = np.asarray(points, float)
    if aperture is None:
        raise ValueError('Eye repair requires the visible aperture mask')
    coordinates = aperture_coordinates(p, aperture)
    visible = map_coordinates(np.asarray(aperture, float), coordinates, order=1, mode='constant') > .5
    distance = map_coordinates(distance_transform_edt(aperture), coordinates, order=1, mode='constant')
    support = smooth((distance-.5)/3)*visible*(p[:, 2] > .05)
    result = p.copy()
    if curves is None:
        curves = eye_curves(p, aperture)
    for center, coef in zip(EYES, curves):
        x, y = (p[:, 0]-center)/.024, (p[:, 1]-.855)/.018
        weight = support*(p[:, 0]*center > 0)
        target = np.column_stack([np.ones(len(p)), x, y, x*x, y*y, x*y])@coef
        result[:, 2] += (target-p[:, 2])*weight
    return result


def skin_normal_field(points, faces):
    """Weld UV duplicates, then reconstruct and filter geometric normals.

    Authored normals on this source contradict adjacent skin and even other
    copies of the same point. Filtering only similarly oriented normals
    retained those sharp islands. Geometry supplies the shared field here.
    """
    p, inverse = np.unique(np.round(points, 6), axis=0, return_inverse=True)
    triangles = inverse[np.asarray(faces)]
    cross = np.cross(p[triangles[:, 1]]-p[triangles[:, 0]], p[triangles[:, 2]]-p[triangles[:, 0]])
    total = np.zeros_like(p)
    for k in range(3):
        np.add.at(total, triangles[:, k], cross)
    normal = unit(total)
    front = (p[:, 2] > .04)&(abs(p[:, 0]) < .07)&(p[:, 1] > .78)
    if np.median(normal[front, 2]) < 0:
        normal = -normal
    tree = cKDTree(p)
    field = normal.copy()
    for i, point in enumerate(p):
        # Keep the modeled nose and lip relief; cheeks/forehead need the
        # broader field that removes the visible source polygon planes.
        detail = (smooth((.024-abs(point[0]))/.012)*
                  smooth((point[1]-.782)/.014)*smooth((.851-point[1])/.014))
        radius = .014-.004*detail
        near = np.asarray(tree.query_ball_point(point, radius*2), int)
        distance = np.linalg.norm(p[near]-point, axis=1)
        weight = np.exp(-(distance/radius)**2)
        field[i] = unit((normal[near]*weight[:, None]).sum(0))
    return field[inverse]


def fit_idle(archive):
    """Use vanilla's idle arm rotations, including its animated hand roll."""
    native = animation_frames(archive.native.read(IDLE))
    ns = 'objects/object_ma1/NativeNpcV1'
    result = animation_frames(archive.candidate.read('alt/'+ns+'/PoseAnimations/'+IDLE)).copy()
    rig = NativeRig(archive, 'young-malon', True)
    joints = [bone+1 for side in ('left', 'right') for bone in rig.profile[side][3:]]
    result[:, joints] = native[:, joints]
    return result


def eye_artwork(archive, namespace, artwork):
    """Apply the authored iris cleanup inside the existing eye aperture only."""
    kind, w, h, pixels = archive.texture(namespace+'/Textures/EyesNeutral')
    if (kind, w, h) != (1, 512, 512):
        raise ValueError('Unexpected child eye atlas')
    old = np.frombuffer(pixels, np.uint8).reshape(h, w, 4).copy()
    edit = np.asarray(Image.open(artwork).convert('RGBA').resize((w, h), Image.Resampling.LANCZOS))
    yy, xx = np.indices((h, w))
    rgb = old[..., :3].astype(float)
    aperture = np.zeros((h, w), bool)
    for cx in (165, 328):
        region = ((xx-cx)/49)**2+((yy-223)/49)**2 < 1
        visible = (rgb[..., 2] > rgb[..., 0]*.83)&(rgb[..., 2] > rgb[..., 1]*.82)&region
        aperture |= binary_fill_holes(visible)
    weight = np.clip((distance_transform_edt(aperture)-1)/2, 0, 1)[..., None]
    revised = np.uint8(np.rint(old*(1-weight)+edit*weight))
    # Keep the previously repaired cheek skin exactly. The original diffuse
    # still paints those specks on Base triangles outside the tiny C region.
    atlas_mask = np.uint8(aperture)*255
    for cx, cy, radius in ((286, 313, 9), (334, 319, 9), (238, 160, 5)):
        spot = (xx-cx)**2+(yy-cy)**2 < radius*radius
        fade = smooth((radius-np.hypot(xx-cx, yy-cy))/2)
        if cy == 160:
            revised = np.uint8(np.rint(revised*(1-fade[..., None])+edit*fade[..., None]))
        atlas_mask[spot] = np.maximum(atlas_mask[spot], np.rint(fade[spot]*255).astype(np.uint8))
    return revised, atlas_mask, aperture


def texture_resource(pixels):
    h, w = pixels.shape[:2]
    return asset._header(asset.T_OTEX, 1)+struct.pack('<IIIIffI', 1, w, h, 1, w/512, h/512, pixels.nbytes)+pixels.tobytes()


def bind_face_diffuse(raw, original, replacement):
    if raw[4:8] != b'TLDO':
        return raw
    result = bytearray(raw)
    old, new = asset.crc64(original), asset.crc64(replacement)
    offset = 72
    while offset+8 <= len(raw):
        first = struct.unpack_from('<I', raw, offset)[0]
        size = 16 if first >> 24 in (0x20, 0x31, 0x32, 0x33, 0x35, 0x36) else 8
        if first >> 24 == 0x20 and struct.unpack_from('<II', raw, offset+8) == (old >> 32, old & 0xffffffff):
            struct.pack_into('<II', result, offset+8, new >> 32, new & 0xffffffff)
        offset += size
    return bytes(result)


def refine(native, baseline, source_path, artwork, output):
    native, baseline, source_path, artwork, output = map(Path, (native, baseline, source_path, artwork, output))
    if output.resolve() in (native.resolve(), baseline.resolve(), source_path.resolve(), artwork.resolve()):
        raise ValueError('Preserve all inputs')
    if hashlib.sha256(baseline.read_bytes()).hexdigest() != PARENT:
        raise ValueError('Use the reviewed POC8 child Malon pack')
    source = SourceModel(source_path)
    if hashlib.sha256(source.raw).hexdigest() != SOURCE:
        raise ValueError('Unexpected child Malon source')
    archive = Archive(native, baseline)
    records = {name: archive.candidate.read(name) for name in archive.candidate.namelist()}
    info = 'young_malon_NativeNpcModelInfo.json'
    meta = json.loads(records[info])
    if meta['native_sha256'] != hashlib.sha256(native.read_bytes()).hexdigest():
        raise ValueError('Native reference differs')
    namespace = meta['namespace']
    rig = NativeRig(archive, 'young-malon', True)
    fitted_rig = NativeRig(archive.native, 'young-malon')
    model = fit_model(source, fitted_rig)
    head = fitted_rig.limbs[14]
    transform = model['head_transform'][:, :3]
    mesh = source.meshes[25]
    if mesh['name'] != 'tripo_part_7':
        raise ValueError('The audited continuous face piece is missing')
    p, uv, faces = mesh['positions'], mesh['uv'], mesh['faces']
    neutral, mask, aperture = eye_artwork(archive, namespace, artwork)
    curves = eye_curves(p, aperture)
    revised_p = repair_eye_depth(p, curves, aperture)
    field = skin_normal_field(revised_p, faces)
    field_local = unit(field@np.linalg.inv(transform)@head.M)
    delta_local = (revised_p-p)@transform.T@head.M
    body = archive.geometry(namespace+'/Body/Limb14', rig, 14)
    lookup = {}
    for local, st in zip(body['local'], np.rint(body['uv']*512*32).astype(int)):
        lookup.setdefault(tuple(st), set()).add(tuple(local))
    expected = (p@transform.T+model['head_transform'][:, 3]-head.T)@head.M
    old_local = []
    for i, (point, st) in enumerate(zip(expected, np.rint(uv*512*32).astype(int))):
        # Lower neck skin shares the chest transform and is outside this
        # repair. It must not be compared to a rigid head-space prediction.
        if p[i, 1] <= .765:
            old_local.append(None)
            continue
        choices = lookup.get(tuple(st))
        if not choices:
            old_local.append(None)
            continue
        nearest = np.array(min(choices, key=lambda q: np.linalg.norm(point-q)))
        if np.linalg.norm(nearest-point) > 3:
            raise ValueError('Face does not match the selected source surface')
        old_local.append(nearest)
    mapping = {}
    for i, local in enumerate(old_local):
        if local is not None and p[i, 1] > .765:
            mapping[tuple(local)] = i
    if len(mapping) < 250:
        raise ValueError('Too few exact head vertices matched')
    old_normal = {tuple(local): normal for local, normal in zip(body['local'], body['normals'])}
    strength = smooth((p[:, 2]-.015)/.015)*smooth((p[:, 1]-.765)/.020)
    for local, i in mapping.items():
        field_local[i] = unit(old_normal[local]*(1-strength[i])+field_local[i]*strength[i])
    diffuse = namespace+'/Textures/Diffuse'
    face_diffuse = namespace+'/Textures/FaceDiffusePOC9'
    frontal = faces[(p[faces, 2] > .015).all(1)]
    painted = reproject_face_paint(p, uv, np.asarray(source.image), frontal, neutral, mask, BOUNDS, padding=2)
    changes = {'alt/'+face_diffuse: texture_resource(painted),
               'alt/'+namespace+'/Textures/EyesNeutral': texture_resource(neutral)}
    # The half blink keeps its eyelid artwork; replace only its visible iris.
    _, w, h, raw = archive.texture(namespace+'/Textures/EyesHalf')
    half = np.frombuffer(raw, np.uint8).reshape(h, w, 4).copy()
    rgb = half[..., :3].astype(float)
    visible = binary_fill_holes((rgb[..., 2] > rgb[..., 0]*.83)&(rgb[..., 2] > rgb[..., 1]*.82)&aperture)
    weight = np.clip((distance_transform_edt(visible)-1)/2, 0, 1)[..., None]
    half = np.uint8(np.rint(half*(1-weight)+neutral*weight))
    changes['alt/'+namespace+'/Textures/EyesHalf'] = texture_resource(half)
    patched_vertices = 0
    for name, raw in records.items():
        if not name.startswith(('alt/'+namespace+'/FaceParts/', 'alt/'+namespace+'/Body/Limb14')):
            continue
        if raw[4:8] == b'RRAO' and struct.unpack_from('<I', raw, 64)[0] == 25:
            revised = bytearray(raw)
            for k in range(struct.unpack_from('<I', raw, 68)[0]):
                offset = 72+k*16
                local = struct.unpack_from('<hhh', raw, offset)
                if local not in mapping:
                    continue
                i = mapping[local]
                if strength[i] == 0:
                    continue
                struct.pack_into('<hhh', revised, offset, *np.rint(np.asarray(local)+delta_local[i]).astype(int))
                revised[offset+12:offset+15] = np.rint(field_local[i]*127).astype(np.int8).tobytes()
                patched_vertices += 1
            if revised != raw:
                changes[name] = bytes(revised)
        elif raw[4:8] == b'TLDO':
            rebound = bind_face_diffuse(raw, diffuse, face_diffuse)
            if rebound != raw:
                changes[name] = rebound
    # More interior samples keep the rounded eyes curved in oblique views.
    # Centroid fans preserve every existing shared boundary edge exactly.
    writer = asset.O2R()
    for side in ('L', 'R'):
        for state in range(5):
            path = namespace+f'/FaceParts/{side}{state}'
            data = archive.geometry(path, rig, 14)
            indices = np.array([mapping[tuple(q)] for q in data['local']])
            bary, source_faces = expression_subdivision(data['faces'], levels=2)
            interpolate = lambda values: np.einsum('fij,fjk->fik', bary, values[source_faces]).reshape(-1, values.shape[-1])
            sample = interpolate(p[indices])
            delta = (repair_eye_depth(sample, curves, aperture)-sample)@transform.T@head.M
            local = interpolate(data['local'])+delta
            normal = unit(interpolate(field_local[indices]))
            output_model = dict(positions=local@rig.limbs[14].M.T+rig.limbs[14].T,
                                normals=normal@rig.limbs[14].M.T,
                                uv=interpolate(data['uv']), bones=np.full(len(local), 14),
                                faces=np.arange(len(local)).reshape(-1, 3))
            texture = namespace+'/Textures/Eyes'+('Half' if state == 1 else 'Closed' if state == 2 else 'Neutral')
            emit_mesh(writer, 'alt/'+path, output_model, rig, output_model['faces'], texture, 14)
    changes.update(writer.files)
    from edit_native_npc_pose import encode_animation
    changes['alt/'+namespace+'/PoseAnimations/'+IDLE] = encode_animation(fit_idle(archive))
    report = dict(parent_sha256=PARENT, source_sha256=SOURCE,
                  artwork_sha256=hashlib.sha256(artwork.read_bytes()).hexdigest(),
                  eye_aperture_coefficients=curves.tolist(),
                  max_eye_depth_change=float(abs(revised_p[:, 2]-p[:, 2]).max()),
                  source_face_xy_and_outer_silhouette_preserved=True,
                  original_skin_and_eyelid_depth_outside_visible_aperture_preserved=True,
                  shared_skin_field=True, exact_head_vertex_matches=len(mapping),
                  patched_head_vertex_records=patched_vertices,
                  native_idle_arm_channels=True,
                  other_pose_resources_byte_identical=True,
                  original_diffuse_unchanged=True, runtime_tested=False)
    meta.setdefault('geometry_repairs', {})['young_malon_face_idle_poc10'] = report
    idle_info = meta['fitted_poses']['gMalonChildIdleAnim']
    idle_info['native_arm_channels'] = True
    idle_info['native_pose_frames_identical'] = True
    idle_info.pop('max_wrist_error', None)  # This candidate no longer uses the previous wrist IK fit.
    changes[info] = json.dumps(meta, indent=2).encode()
    allowed = lambda name: (name == info or name == 'alt/'+namespace+'/PoseAnimations/'+IDLE or
                            name.startswith(('alt/'+namespace+'/FaceParts/', 'alt/'+namespace+'/Body/Limb14',
                                             'alt/'+namespace+'/Textures/Eyes', 'alt/'+face_diffuse)))
    if any(not allowed(name) for name in changes):
        raise ValueError('Repair escaped the private head/idle resources')
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for name in sorted(set(records)|set(changes)):
            raw = changes[name] if name in changes else records[name]
            entry = zipfile.ZipInfo(name, (2026, 10, 19, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(entry, raw)
    return dict(report, changed_resources=sorted(changes),
                output_sha256=hashlib.sha256(output.read_bytes()).hexdigest())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('native', 'baseline', 'source', 'artwork', 'output'):
        parser.add_argument(name)
    args = parser.parse_args()
    print(json.dumps(refine(args.native, args.baseline, args.source, args.artwork, args.output), indent=2))
