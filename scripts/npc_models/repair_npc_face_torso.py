"""Bounded, archive-authoritative Adult Malon/Anime Saria skin-normal POC.

All positions, UVs, display lists, rigs, hands and props are kept. An optional
Anime Saria eye-art pass changes only its measured eye UV footprint; source
brows/hair are protected. Rough torso shading is present in unlit source art
and remains unchanged. Inputs are exact rejected parents, never masters.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import numpy as np
from scipy.spatial import cKDTree

from native_npc_models import NativeRig, SourceModel, VARIANTS, unit
from verify_native_npc_models import Archive
from native_assets.path_guard import require_separate_destination


PARENTS = {
    'malon': '8ffe4694a9eb8a444490da4cdcc3cb10d7c47ca5024ca6b9ae2b5df1136b77fa',
    'saria-anime': 'bbcf3551ab4a9eb5184f28d7403795dbab2240d7a62f9d6f1ff459ac63e92562',
}
SOURCES = {
    'malon': '11d200aadce51cc51f3181c5d64ffb990392b28fed53158fc544cc92703e2fe0',
    'saria-anime': 'dddeaed512e7b4aaedf960e1ddab13e74c480897e0f44404fb980c0c9ce5d63a',
}
NATIVE_SHA256 = 'ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b'


def smooth(value):
    value = np.clip(value, 0., 1.)
    return value*value*(3.-2.*value)


def smooth_skin_field(support_points, support_normals, points, normals, strength, sigma=.018):
    """Evaluate one local, continuous normal field on unchanged skin samples.

    UV/batch copies contribute one geometric sample. Gaussian spatial weights
    span the diagnosed polygon-scale fluctuations without flattening the face's
    broad curvature. Depth weighting separates frontal and returning surfaces.
    The caller owns the protected-surface envelope.
    """
    support_points = np.asarray(support_points, float)
    support_normals = np.asarray(support_normals, float)
    points, normals, strength = map(np.asarray, (points, normals, strength))
    result = normals.astype(float).copy()
    unique, inverse = np.unique(np.round(support_points, 6), axis=0, return_inverse=True)
    total = np.zeros_like(unique); np.add.at(total, inverse, support_normals)
    field = unit(total)
    tree = cKDTree(unique[:, :2])
    for i in np.flatnonzero(strength > 0):
        ids = np.asarray(tree.query_ball_point(points[i, :2], 3*sigma), int)
        if not len(ids):
            continue
        delta = unique[ids]-points[i]
        weight = np.exp(-np.square(delta[:, :2]/sigma).sum(1)
                        -np.square(delta[:, 2]/.025))
        target = unit((field[ids]*weight[:, None]).sum(0))
        amount = float(strength[i])
        result[i] = unit(normals[i]*(1-amount)+target*amount)
    return result


def projection_frame(archive, rig, variant):
    """Recover the existing source XY frame from the authored blink UVs.

    This is a measured affine coordinate conversion, not an inferred model fit.
    Blink UVs encode the projection bounds recorded beside the authoring tool.
    Native integer rounding is retained; a trimmed fit rejects altered margins.
    """
    spec = json.loads(Path(__file__).with_name('face_art').joinpath(variant, 'projection.json').read_text())
    if spec['source_sha256'] != SOURCES[variant]:
        raise ValueError('Blink projection belongs to a different source')
    bounds = np.asarray(spec['bounds']); local, xy = [], []
    head = rig.profile['head']
    for part in ('L1', 'R1'):
        data = archive.geometry(rig.ns+'/FaceParts/'+part, rig, head)
        keep = data['bones'] == head
        local.extend(data['local'][keep]); uv = data['uv'][keep]
        xy.extend(np.c_[bounds[0]+uv[:, 0]*(bounds[2]-bounds[0]),
                        bounds[3]-uv[:, 1]*(bounds[3]-bounds[1])])
    local, xy = np.asarray(local), np.asarray(xy)
    unique = np.unique(np.c_[local, xy], axis=0)
    local, xy = unique[:, :3], unique[:, 3:]
    design = np.c_[local, np.ones(len(local))]
    keep = np.ones(len(local), bool)
    for _ in range(4):
        coefficient = np.linalg.lstsq(design[keep], xy[keep], rcond=None)[0]
        error = np.linalg.norm(design@coefficient-xy, axis=1)
        keep = error < max(.00020, float(np.quantile(error, .90)))
    if error[keep].max() > .0005 or keep.sum() < 30:
        raise ValueError('The archive no longer has its measured blink projection')
    lateral = unit(coefficient[:3, 0]); vertical = unit(coefficient[:3, 1])
    front = unit(np.cross(lateral, vertical))
    frame = np.array([lateral, vertical, front])
    scale = float(np.mean(np.linalg.norm(coefficient[:3], axis=0)))
    return coefficient, frame, scale, dict(
        projection_vertices=int(keep.sum()), max_xy_fit_error=float(error[keep].max()),
        native_units_to_source=scale, xy_coefficients=coefficient.tolist())


def canonical(local, coefficient, frame, scale):
    local = np.asarray(local)
    xy = np.c_[local, np.ones(len(local))]@coefficient
    return np.c_[xy, local@frame[2]*scale]


def skin_color(rgb):
    rgb = np.asarray(rgb, float)
    return ((rgb[:, 0] > rgb[:, 1]*1.015) & (rgb[:, 1] > rgb[:, 2]*1.08)
            & (rgb[:, 1] > rgb[:, 0]*.62) & (rgb.mean(1) > 85))


def sample_colors(archive, texture, uv):
    kind, width, height, raw = archive.texture(texture)
    if kind != 1:
        raise ValueError('The skin path must retain its RGBA32 source texture')
    image = np.frombuffer(raw, np.uint8).reshape(height, width, 4)
    ij = np.floor(uv*[width, height]).astype(int)
    return image[np.clip(ij[:, 1], 0, height-1), np.clip(ij[:, 0], 0, width-1), :3]


def envelope(points, rgb):
    x, y, z = np.asarray(points).T
    outer = (smooth((.081-abs(x))/.018)*smooth((y-.785)/.014)
             *smooth((.933-y)/.017)*smooth((z-.021)/.012))
    # Keep the narrow nose/lip relief while softening its surrounding field.
    nose = np.exp(-np.square(x/.014)-np.square((y-.835)/.025))
    lips = np.exp(-np.square(x/.028)-np.square((y-.808)/.009))
    return outer*(1-.75*nose)*(1-.8*lips)*skin_color(rgb)


def eye_art_mask(bounds, size):
    """Only the two source eye apertures and their thin lash margins."""
    from scipy.ndimage import distance_transform_edt
    x0, y0, x1, y1 = bounds
    yy, xx = np.indices((size, size))
    x = x0+xx/(size-1)*(x1-x0); y = y1-yy/(size-1)*(y1-y0)
    region = np.zeros((size, size), bool)
    for center in (-.031, .031):
        region |= ((x-center)/.0285)**4+((y-.8525)/.0205)**4 < 1
    region &= (abs(x) > .009) & (y < .870)
    # The brows sit above this envelope; the central nose and hair silhouettes
    # remain outside it. A four-pixel interior feather preserves the seam.
    return np.clip(distance_transform_edt(region)/4., 0, 1)


def transfer_eye_art(points, uv, atlas, faces, artwork, bounds):
    """Reproject authored eyes to existing UV islands with a measured footprint."""
    from native_npc_face import bake, reproject_face_paint
    from scipy.ndimage import binary_dilation, binary_fill_holes, binary_propagation, distance_transform_edt
    artwork = np.asarray(artwork, np.uint8)
    if artwork.shape[0] != artwork.shape[1]:
        raise ValueError('The registered eye projection must remain square')
    outer = eye_art_mask(bounds, len(artwork))
    base, _, _ = bake(np.asarray(points), np.asarray(uv), np.asarray(atlas), np.asarray(faces),
                      bounds, size=len(artwork))
    yy, xx = np.indices(artwork.shape[:2]); x0, y0, x1, y1 = bounds
    x = x0+xx/(len(artwork)-1)*(x1-x0); y = y1-yy/(len(artwork)-1)*(y1-y0)
    seeds = np.zeros(artwork.shape[:2], bool)
    for center in (-.031, .031):
        seeds |= ((x-center)/.012)**2+((y-.851)/.012)**2 < 1
    aperture = np.zeros(artwork.shape[:2], bool)
    for image in (base, artwork):
        rgb = image[:, :, :3].astype(float)
        white = ((abs(rgb[:, :, 0]-rgb[:, :, 1]) < 25)
                 & (abs(rgb[:, :, 1]-rgb[:, :, 2]) < 25) & (rgb.min(2) > 100))
        green = (rgb[:, :, 1] > rgb[:, :, 0]*1.07) & (rgb[:, :, 1] > rgb[:, :, 2]*1.05)
        dark = rgb.max(2) < 100
        eligible = (white | green | dark) & (outer > 0)
        aperture |= binary_fill_holes(binary_propagation(seeds & eligible, mask=eligible))
    # Protect the original cheek/eyelid skin even if the generator retouched
    # nearby colors: only actual connected eye pixels plus a two-pixel lash
    # margin receive the edit. This prevents a rectangular projection seam.
    aperture = binary_dilation(aperture, iterations=2) & (outer > 0)
    mask = np.rint(np.minimum(outer, np.clip(distance_transform_edt(aperture)/3., 0, 1))*255).astype(np.uint8)
    revised = reproject_face_paint(points, uv, atlas, np.asarray(faces), artwork, mask, bounds)
    white = np.full_like(artwork, 255)
    footprint = reproject_face_paint(points, uv, np.zeros_like(atlas), np.asarray(faces), white, mask, bounds)
    allowed = footprint[:, :, 0] > 0
    # Green brows/hair can overlap an eye's projection. Their source texels are
    # protected; the only green allowed to change is inside the iris footprint.
    yy, xx = np.indices(artwork.shape[:2]); x0, y0, x1, y1 = bounds
    x = x0+xx/(len(artwork)-1)*(x1-x0); y = y1-yy/(len(artwork)-1)*(y1-y0)
    iris = np.zeros(artwork.shape[:2], bool)
    for center in (-.031, .031):
        iris |= ((x-center)/.019)**4+((y-.851)/.019)**4 < 1
    iris &= y < .866
    iris_footprint = reproject_face_paint(points, uv, np.zeros_like(atlas), np.asarray(faces), white,
                                        iris.astype(np.uint8)*255, bounds)[:, :, 0] > 0
    rgb = atlas[:, :, :3].astype(float)
    green = (rgb[:, :, 1] > rgb[:, :, 0]*1.08) & (rgb[:, :, 1] > rgb[:, :, 2]*1.08)
    allowed &= ~(green & ~iris_footprint)
    revised[~allowed] = atlas[~allowed]
    revised[:, :, 3] = atlas[:, :, 3]
    if np.any(revised[~allowed] != atlas[~allowed]):
        raise AssertionError('The eye transfer escaped its UV pixel footprint')
    return revised, allowed


def author_eye_atlas(records, archive, rig, source_path, artwork_path):
    """The exact GLB supplies coordinate/UV provenance; only its eye art changes."""
    from PIL import Image
    source_path, artwork_path = map(Path, (source_path, artwork_path))
    source = SourceModel(source_path)
    if hashlib.sha256(source.raw).hexdigest() != SOURCES['saria-anime']:
        raise ValueError('Eye authoring requires the exact Anime Saria source GLB')
    texture = rig.ns+'/Textures/Diffuse'
    kind, width, height, raw = archive.texture(texture)
    atlas = np.frombuffer(raw, np.uint8).reshape(height, width, 4)
    if kind != 1 or (width, height) != source.image.size or raw != np.asarray(source.image).tobytes():
        raise ValueError('The authoritative source atlas differs from the parent')
    head = source.meshes[0]
    if head['name'] != 'tripo_part_0':
        raise ValueError('Unexpected source head identity')
    faces = head['faces'][(head['positions'][head['faces'], 2] > .015).all(1)]
    artwork = np.asarray(Image.open(artwork_path).convert('RGBA'))
    bounds = [-.075, .795, .075, .90]
    revised, allowed = transfer_eye_art(head['positions'], head['uv'], atlas, faces, artwork, bounds)
    changed = np.any(revised != atlas, axis=2)
    if changed.sum() < 100:
        raise ValueError('The authored eye pass contains no material eye cleanup')
    # The source atlas can be shared by every body part. Prove that no body
    # triangle actually covers any changed texel before allowing the write.
    for piece in source.meshes[1:]:
        pixel = piece['uv']*[width-1, height-1]
        for face in piece['faces']:
            q = pixel[face]
            lo = np.maximum(np.floor(q.min(0)).astype(int), 0)
            hi = np.minimum(np.ceil(q.max(0)).astype(int), [width-1, height-1])
            if np.any(hi < lo) or not changed[lo[1]:hi[1]+1, lo[0]:hi[0]+1].any():
                continue
            yy, xx = np.nonzero(changed[lo[1]:hi[1]+1, lo[0]:hi[0]+1])
            xy = np.c_[xx+lo[0], yy+lo[1]]
            matrix = np.column_stack([q[0]-q[2], q[1]-q[2]])
            if abs(np.linalg.det(matrix)) < 1e-10:
                continue
            bary = (xy-q[2])@np.linalg.inv(matrix).T
            if ((bary.min(1) >= -1e-6) & (bary.sum(1) <= 1+1e-6)).any():
                raise ValueError('A source body UV triangle shares an authored eye texel')
    name = 'alt/'+texture if 'alt/'+texture in records else texture
    original = records[name]
    if not original.endswith(raw):
        raise ValueError('The source texture payload is not at its recorded boundary')
    records[name] = original[:-len(raw)]+revised.tobytes()
    return dict(texture=name, artwork_sha256=hashlib.sha256(artwork_path.read_bytes()).hexdigest(),
                source_glb_sha256=SOURCES['saria-anime'], projection_bounds=bounds,
                authored_pixel_count=int(changed.sum()), allowed_uv_pixel_count=int(allowed.sum()),
                mask='connected original/authored iris+sclera+lashes, two-pixel rim, protected green brows/hair',
                outside_eye_uv_footprint_byte_identical=True, body_uv_pixels_byte_identical=True,
                geometry_positions_uvs_and_all_other_textures_byte_identical=True,
                original_half_and_closed_blink_art_preserved=True, iris_green_for_separate_blue_palette_pass=True,
                purpose='Balance open eye apertures and remove inherited sharp gray iris/sclera triangles',
                runtime_tested=False)


def build(native, parent, variant, output, eye_source=None, eye_art=None):
    native, parent, output = map(Path, (native, parent, output))
    require_separate_destination(output, native, parent, eye_source, eye_art)
    digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    if variant not in PARENTS or digest(parent) != PARENTS[variant]:
        raise ValueError('Use the exact rejected source parent for this isolated POC')
    if digest(native) != NATIVE_SHA256:
        raise ValueError('Native archive differs from the recorded parent')
    archive = Archive(native, parent)
    records = {name: archive.candidate.read(name) for name in archive.candidate.namelist()}
    manifest = variant.replace('-', '_')+'_NativeNpcModelInfo.json'
    meta = json.loads(records[manifest])
    if meta['variant'] != variant or meta['source_sha256'] != SOURCES[variant]:
        raise ValueError('The model manifest differs from the bounded source')
    rig = NativeRig(archive, VARIANTS[variant]['profile'], private=True)
    head = rig.profile['head']; head_slot = rig.skel.slot_to_limb.index(head)
    coefficient, frame, scale, frame_report = projection_frame(archive, rig, variant)
    data = archive.geometry(rig.ns+'/Faces/L0R0M0', rig, head)
    colors = np.empty((len(data['local']), 3))
    for face, texture in zip(data['faces'], data['textures']):
        colors[face] = sample_colors(archive, texture, data['uv'][face])
    points = canonical(data['local'], coefficient, frame, scale)
    normals = unit(data['normals']@frame.T)
    strength = envelope(points, colors)
    support = ((data['bones'] == head) & (strength > 0) & (normals[:, 2] > .1))
    if support.sum() < 100:
        raise ValueError('The exact parent lacks the measured frontal skin support')
    support_points, support_normals = points[support], normals[support]
    changed, count, turns = [], 0, []
    for name, raw in list(records.items()):
        if not name.startswith(('alt/'+rig.ns+'/FaceParts/', 'alt/'+rig.ns+f'/Body/Limb{head:02d}')):
            continue
        if raw[4:8] != b'RRAO' or struct.unpack_from('<I', raw, 64)[0] != 25:
            continue
        path = name.removeprefix('alt/').removesuffix('/V')
        geometry = archive.geometry(path, rig, head)
        textures = set(geometry['textures'])
        if len(textures) != 1:
            raise ValueError('A bounded skin vertex buffer unexpectedly has multiple materials')
        buffer = archive.vertex_buffer(name)
        p = canonical(buffer['xyz'], coefficient, frame, scale)
        n = buffer['normal'].astype(float)/127
        rgb = sample_colors(archive, next(iter(textures)), buffer['uv'].astype(float)/16384)
        normal = unit(n@frame.T)
        amount = envelope(p, rgb)*smooth((normal[:, 2]-.10)/.30)
        amount[(buffer['flag'] >> 8) != head_slot] = 0
        revised = smooth_skin_field(support_points, support_normals, p, normal, amount)
        encoded = np.rint(np.clip(unit(revised@np.linalg.inv(frame.T)), -1, 1)*127).astype(np.int8)
        # No changes at all outside the caller's envelope, including normal
        # length rounding; every byte outside normal XYZ remains untouched.
        active = (amount > 0) & (encoded != buffer['normal']).any(1)
        if not active.any():
            continue
        patched = bytearray(raw)
        for index in np.flatnonzero(active):
            offset = 72+int(index)*16+12
            patched[offset:offset+3] = encoded[index].tobytes()
        turn = np.degrees(np.arccos(np.clip((unit(n[active])*unit(encoded[active])).sum(1), -1, 1)))
        turns.extend(turn.tolist()); count += int(active.sum())
        records[name] = bytes(patched); changed.append(dict(path=name, vertex_records=int(active.sum())))
    if not changed:
        raise ValueError('No bounded skin normals changed')
    report = dict(parent_sha256=PARENTS[variant], native_sha256=NATIVE_SHA256,
                  source_sha256=SOURCES[variant], frame=frame_report,
                  normal_field='unique source skin samples; Gaussian XY=.018m, depth=.025m; smooth face envelope',
                  changed_vertex_buffers=changed, vertex_records_changed=count,
                  max_normal_turn_degrees=max(turns), mean_normal_turn_degrees=float(np.mean(turns)),
                  positions_uvs_artwork_display_lists_rigs_hands_props_unchanged=True,
                  torso_diagnosis='Rough fold shading persists unlit in source diffuse; 4096 atlas scaling is correctly 8x. No torso artwork changed.',
                  runtime_tested=False, accepted=False, promoted=False)
    if (eye_source is None) != (eye_art is None):
        raise ValueError('Both the exact eye source GLB and registered authored art are required')
    if eye_art is not None:
        if variant != 'saria-anime':
            raise ValueError('The bounded eye authoring pass is Anime Saria only')
        report['eye_art_cleanup'] = author_eye_atlas(records, archive, rig, eye_source, eye_art)
        report['positions_uvs_artwork_display_lists_rigs_hands_props_unchanged'] = False
        report['positions_uvs_display_lists_rigs_hands_props_unchanged'] = True
        report['artwork_outside_eye_uv_footprint_unchanged'] = True
    meta['face_normal_poc'] = report
    records[manifest] = json.dumps(meta, indent=2).encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for name, raw in sorted(records.items()):
            info = zipfile.ZipInfo(name, (2026, 10, 20, 0, 0, 0)); info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, raw)
    archive.native.close(); archive.candidate.close()
    return dict(report, candidate=str(output), candidate_sha256=digest(output))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('native', 'parent', 'variant', 'output'):
        parser.add_argument(name)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--eye-source', type=Path)
    parser.add_argument('--eye-art', type=Path)
    args = parser.parse_args()
    if args.report:
        require_separate_destination(args.report, args.native, args.parent, args.output,
                                     args.eye_source, args.eye_art)
    report = build(args.native, args.parent, args.variant, args.output, args.eye_source, args.eye_art)
    result = json.dumps(report, indent=2)+'\n'
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(result)
    print(result)


if __name__ == '__main__':
    main()
