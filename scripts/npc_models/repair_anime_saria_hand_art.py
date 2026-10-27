#!/usr/bin/env python3
"""Bounded Anime Saria hand paint POC on an unchanged shared diffuse atlas.

The imagegen contact sheet is registered to the exact source hand projections.
Only its positive, median-normalized luminance residual is transferred. This
avoids the generator's broad palm recolor and retains source hue, anatomy and
fine texture. Actual exported display-list UV support protects every non-hand
surface, the complete wrist-contact triangles, distal fingers/nails and island
boundaries. All archive resources except the existing Diffuse payload stay exact.
This is offline evidence, never runtime or user acceptance.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import numpy as np
from PIL import Image, ImageDraw
from scipy.ndimage import distance_transform_edt, map_coordinates

from native_assets import o2r
from native_assets.path_guard import require_separate_destination
from native_assets.npc_sampling import native_uv_modes
from native_npc_face import bake
from native_npc_models import SourceModel
from rebatch_native_npc_archives import _commands, _decode, _index
from repair_anime_saria_hands import (NS, PROFILES, SOURCE_ID, geometry_digest,
                                     hand_meshes, normal_digest, point_key)

ATLAS = NS.removeprefix('alt/')+'/Textures/Diffuse'
ARTWORK_SHA256 = '125c55723b915b338596fa6daf940d256d5cec6bbc3f650f691bfebd614dd7ee'
UV_PROFILES = {
    'default_rigid_left': 'f3d981c12ac93af48d18d8564b8c7406697892b5feaa55f368e7a58783a424f1',
    'default_skin_left': 'd2008c93d3228b854f1578e1b02bf5ec2263769546afb3e8a602a103c128eb55',
    'grip_rigid_left': 'f3d981c12ac93af48d18d8564b8c7406697892b5feaa55f368e7a58783a424f1',
    'grip_skin_left': 'd2008c93d3228b854f1578e1b02bf5ec2263769546afb3e8a602a103c128eb55',
    'default_rigid_right': 'ae187826b2da43b8532cd5a1acf09036195ee9047b8ad8f19c2f6238b3c0c2d8',
    'default_skin_right': 'dcee5c8aef5d55dd3e119876580abaaac860c2208469e5bdcc89e427703b377b',
    'grip_rigid_right': 'ae187826b2da43b8532cd5a1acf09036195ee9047b8ad8f19c2f6238b3c0c2d8',
    'grip_skin_right': 'dcee5c8aef5d55dd3e119876580abaaac860c2208469e5bdcc89e427703b377b',
}
SIZE, PANEL, SHEET = 960, 1024, 2048
LUMA = np.array([.2126, .7152, .0722])
MAX_LIFT = 28.
STRENGTH = .85


def require(condition, message):
    if not condition:
        raise ValueError(message)


def smooth(x):
    x = np.clip(x, 0., 1.)
    return x*x*(3.-2.*x)


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def source_model(path):
    source = SourceModel(path)
    require(digest(source.raw) == SOURCE_ID, 'Exact reviewed Anime Saria GLB is required')
    require(source.image.size == (4096, 4096), 'Unexpected source atlas dimensions')
    require(source.meshes[4]['name'] == 'tripo_part_12' and
            source.meshes[2]['name'] == 'tripo_part_10', 'Source hand identity differs')
    return source


def uv_triangles(triangles):
    return np.array([[struct.unpack_from('<hh', vertex[1], 8)
                      for vertex in triangle] for triangle in triangles], np.int16)


def raster_triangles(uv, width, height, padding=0.):
    """Conservative texel-center footprint; padding is measured in texels.

    Native texture coordinates are normalized by 16384 and scale by the actual
    atlas size. Two texels of padding include every bilinear tap, quantized UV
    margin and subpixel triangle; no triangle is discarded for lacking a center.
    """
    result = np.zeros((height, width), bool)
    for q in np.asarray(uv, float)*[width, height]:
        lo = np.maximum(np.floor(q.min(0)-padding-1).astype(int), 0)
        hi = np.minimum(np.ceil(q.max(0)+padding+1).astype(int), [width-1, height-1])
        if np.any(hi < lo):
            continue
        matrix = np.column_stack([q[0]-q[2], q[1]-q[2]])
        determinant = abs(np.linalg.det(matrix))
        if determinant < 1e-12:
            # A collapsed UV triangle can still sample a line or a point. Its
            # entire tiny padded rectangle is protected instead of disappearing.
            if padding:
                result[lo[1]:hi[1]+1, lo[0]:hi[0]+1] = True
            continue
        yy, xx = np.mgrid[lo[1]:hi[1]+1, lo[0]:hi[0]+1]
        bary = (np.stack([xx+.5, yy+.5], -1)-q[2])@np.linalg.inv(matrix).T
        bary = np.dstack([bary, 1-bary.sum(2)])
        edges = np.linalg.norm(q[[1, 2, 0]]-q[[2, 0, 1]], axis=1)
        altitude = determinant/np.maximum(edges, 1e-12)
        covered = (bary*altitude).min(2) >= -padding-1e-6
        result[lo[1]:hi[1]+1, lo[0]:hi[0]+1] |= covered
    return result


def sampling_support(uv, width, height, padding):
    # Retain the authored footprint and cover both actual native filter modes.
    result = raster_triangles(uv, width, height, padding=padding)
    for mode in native_uv_modes(uv):
        result |= raster_triangles(mode, width, height, padding=padding)
    return result


def archive_ownership(records, width, height):
    """Fail closed on unreviewed hand layouts or unknown atlas geometry."""
    index = _index(records)
    hands = {path: (kind, fore, side) for path, kind, fore, side in hand_meshes()}
    hand = np.zeros((height, width), bool)
    nonhand, wrist = np.zeros_like(hand), np.zeros_like(hand)
    seen = set(); found = set(); meshes = []; wrist_seen = set()
    for path, raw in records.items():
        if raw[4:8] in (b'TLDO', b'ODLT'):
            require(raw[0] == 0 and raw[4:8] == b'TLDO',
                    'Unsupported display-list reader; cannot prove atlas ownership: '+path)
        if raw[4:8] != b'TLDO':
            continue
        commands = _commands(raw)
        if not any(c[1] in (5, 6) for c in commands):
            continue  # Recursive wrappers own no UV triangles.
        textures = [index.get(c[4]) for c in commands if c[1] == 0x20]
        if ATLAS not in textures and 'alt/'+ATLAS not in textures:
            continue
        require(len(textures) == 1, 'Unknown multi-material atlas geometry: '+path)
        mesh = _decode(records, path, index)
        uv_raw = uv_triangles(mesh.triangles)
        is_hand = path in hands
        if is_hand:
            kind, fore, side = hands[path]
            found.add(path)
            require(geometry_digest(mesh.triangles) == PROFILES[kind][0],
                    'Unreviewed hand geometry: '+path)
            require(normal_digest(mesh.triangles) in PROFILES[kind][1:],
                    'Unreviewed hand normal field: '+path)
            require(digest(uv_raw.tobytes()) == UV_PROFILES[kind],
                    'Unreviewed hand UV stream: '+path)
            forearm = _decode(records, fore, index)
            anchors = {point_key(v) for t in mesh.triangles for v in t} & {
                point_key(v) for t in forearm.triangles for v in t}
            require(len(anchors) == (19 if side == 'left' else 32), 'Wrist ownership differs')
            rim_triangles = [t for t in mesh.triangles if any(point_key(v) in anchors for v in t)]
            rim_uv = uv_triangles(rim_triangles)
            key = digest(rim_uv.tobytes())
            if key not in wrist_seen:
                wrist |= sampling_support(rim_uv.astype(float)/16384, width, height, padding=3)
                wrist_seen.add(key)
        key = (is_hand, digest(uv_raw.tobytes()))
        if key not in seen:
            mask = (raster_triangles(uv_raw.astype(float)/16384, width, height) if is_hand
                    else sampling_support(uv_raw.astype(float)/16384, width, height, padding=2))
            if is_hand:
                hand |= mask
            else:
                nonhand |= mask
            seen.add(key)
        meshes.append(dict(resource=path, hand=is_hand, triangles=len(mesh.triangles)))
    require(found == set(hands), 'Required hand display-list readers are missing')
    require(len(meshes) >= 73, 'The shared atlas non-hand readers are incomplete')
    safe = (distance_transform_edt(hand) > 4) & ~nonhand & ~wrist
    require(safe.sum() > 100000, 'No useful exclusive hand UV interior')
    return safe, hand, nonhand, wrist, meshes


def projections(source):
    atlas = np.asarray(source.image)
    for row, (side, mesh_id, sign) in enumerate((('Left', 4, 1), ('Right', 2, -1))):
        mesh = source.meshes[mesh_id]
        position = mesh['positions']
        points = np.c_[position[:, 2]*sign, position[:, 0]*sign, position[:, 1]]
        low, high = points[:, :2].min(0), points[:, :2].max(0)
        center = (low+high)/2; span = max(high-low)*1.12
        bounds = np.r_[center-span/2, center+span/2]
        for col, (surface, depth_sign) in enumerate((('top', 1), ('palm', -1))):
            p = points.copy(); p[:, 2] *= depth_sign
            base, _, valid = bake(p, mesh['uv'], atlas, mesh['faces'], bounds, size=SIZE)
            base[~valid] = [35, 40, 46, 255]
            yield mesh, p, base, valid, dict(side=side, mesh=mesh_id, surface=surface,
                depthsign=depth_sign, sign=sign, bounds=bounds.tolist(),
                panel=[col*PANEL+32, row*PANEL+52, SIZE, SIZE])


def prepare(source_path, output_dir):
    output_dir = Path(output_dir)
    targets = [output_dir/'hand-paint-edit-target.png', output_dir/'projection-spec.json']
    for target in targets:
        require_separate_destination(target, source_path, *[p for p in targets if p != target])
    source = source_model(source_path)
    sheet = Image.new('RGBA', (SHEET, SHEET), (35, 40, 46, 255)); draw = ImageDraw.Draw(sheet)
    specs = []
    for mesh, points, base, valid, spec in projections(source):
        x, y, w, h = spec['panel']
        sheet.paste(Image.fromarray(base), (x, y))
        draw.text((x+3, y-34), spec['side']+' hand | '+spec['surface']+' | ORIGINAL', fill='white')
        specs.append(spec)
    output_dir.mkdir(parents=True, exist_ok=True)
    sheet.convert('RGB').save(targets[0]); targets[1].write_text(json.dumps(specs, indent=2)+'\n')
    return dict(target=str(targets[0]), source_sha256=SOURCE_ID, projections=specs)


def distal_mask(source, width, height):
    """Preserve whole triangles influenced by a distal finger bone: all nails."""
    mask = np.zeros((height, width), bool)
    for mesh_id in (2, 4):
        mesh = source.meshes[mesh_id]
        distal = np.array([source.names[j].endswith(('3', '4')) and 'Hand' in source.names[j]
                           for j in mesh['joints'].ravel()]).reshape(mesh['joints'].shape)
        vertices = (distal & (mesh['weights'] > .1)).any(1)
        faces = mesh['faces'][vertices[mesh['faces']].any(1)]
        mask |= sampling_support(mesh['uv'][faces], width, height, padding=3)
    return mask


def normalized_lift(base, authored, valid):
    """Use authored shadow cleanup while discarding its global color change."""
    delta = (np.asarray(authored, float)[..., :3]-base[..., :3])@LUMA
    interior = distance_transform_edt(valid) > 20
    require(interior.sum() > 10000, 'Projection interior is too small')
    offset = float(np.median(delta[interior]))
    lift = np.clip(delta-offset, 0, MAX_LIFT)
    lift *= smooth(distance_transform_edt(valid)/18.)
    lift[~valid] = 0
    return lift, offset


def transfer_scalar(mesh, points, spec, lift, scalar, support):
    """Inverse UV rasterization of the registered interior projection only."""
    height, width = scalar.shape
    uv = mesh['uv']*[width, height]
    bounds = np.asarray(spec['bounds'])
    for face in mesh['faces']:
        q = uv[face]
        lo = np.maximum(np.floor(q.min(0)).astype(int), 0)
        hi = np.minimum(np.ceil(q.max(0)).astype(int), [width-1, height-1])
        if (hi < lo).any():
            continue
        matrix = np.column_stack([q[0]-q[2], q[1]-q[2]])
        if abs(np.linalg.det(matrix)) < 1e-12:
            continue
        yy, xx = np.mgrid[lo[1]:hi[1]+1, lo[0]:hi[0]+1]
        b = (np.stack([xx+.5, yy+.5], -1)-q[2])@np.linalg.inv(matrix).T
        bary = np.dstack([b, 1-b.sum(2)])
        covered = bary.min(2) >= -1e-6
        p = bary@points[face]
        s = (p[..., 0]-bounds[0])/(bounds[2]-bounds[0])
        t = (bounds[3]-p[..., 1])/(bounds[3]-bounds[1])
        covered &= (s >= 0) & (s <= 1) & (t >= 0) & (t <= 1)
        normal_y = (bary@mesh['normals'][face])[..., 1]*spec['depthsign']
        # Returning surfaces and the exact wrist band retain source paint.
        weight = smooth((normal_y-.2)/.5)*smooth((p[..., 1]-.394)/.012)
        weight *= covered
        delta = map_coordinates(lift, [t*(SIZE-1), s*(SIZE-1)], order=1, mode='nearest')
        scalar[yy, xx] += delta*weight
        support[yy, xx] += weight


def repair_records(records, source_path, artwork_path):
    source = source_model(source_path)
    require(digest(Path(artwork_path).read_bytes()) == ARTWORK_SHA256,
            'Use the reviewed, preserved imagegen hand artwork')
    info = json.loads(records.get('saria_anime_NativeNpcModelInfo.json', b'{}'))
    require(info.get('source_sha256') == SOURCE_ID and info.get('variant') == 'saria-anime',
            'Unsupported archive source identity')
    require(ATLAS in records and 'alt/'+ATLAS not in records, 'Unexpected shared Diffuse binding')
    kind, width, height, raw = o2r.read_texture(records[ATLAS])
    require(kind == 1 and (width, height) == source.image.size and len(raw) == width*height*4,
            'Shared Diffuse is no longer its RGBA32 atlas')
    atlas = np.frombuffer(raw, np.uint8).reshape(height, width, 4)
    safe, hand, nonhand, wrist, meshes = archive_ownership(records, width, height)
    original = np.asarray(source.image)
    require(np.array_equal(atlas[hand], original[hand]), 'Hand pixels differ from the reviewed source')
    nails = distal_mask(source, width, height)
    safe &= ~nails
    require(safe.sum() > 100000, 'Too little safe hand surface remains after nail protection')
    art = Image.open(artwork_path).convert('RGBA')
    require(art.width == art.height and art.width >= 1024, 'Contact sheet registration differs')
    artwork = np.asarray(art.resize((SHEET, SHEET), Image.Resampling.LANCZOS))
    scalar = np.zeros((height, width), np.float32); support = np.zeros_like(scalar)
    panels = []
    for mesh, points, base, valid, spec in projections(source):
        x, y, w, h = spec['panel']; authored = artwork[y:y+h, x:x+w]
        lift, offset = normalized_lift(base, authored, valid)
        transfer_scalar(mesh, points, spec, lift, scalar, support)
        panels.append(dict(**spec, removed_global_luminance_offset=offset,
                           positive_residual_p95=float(np.percentile(lift[valid], 95))))
    scalar /= np.maximum(support, 1.)
    # Final mask is exclusively native-archive owned; source transfer cannot
    # authorize a texel outside this measured and feathered native footprint.
    feather = smooth(distance_transform_edt(safe)/5.)
    scalar *= feather*STRENGTH
    scalar[~safe] = 0
    revised = atlas.copy()
    revised[..., :3] = np.rint(np.clip(atlas[..., :3].astype(float)+scalar[..., None], 0, 255)).astype(np.uint8)
    changed = (revised != atlas).any(2)
    require(1000 < changed.sum() < safe.sum(), 'No useful bounded hand paint change')
    require(not changed[~safe].any(), 'Hand edit escaped its safe footprint')
    require(not changed[nonhand | wrist | nails].any(), 'Protected surface paint changed')
    require(np.array_equal(revised[..., 3], atlas[..., 3]), 'Diffuse alpha changed')
    payload = revised.tobytes()
    require(records[ATLAS].endswith(raw), 'Texture payload boundary differs')
    result = dict(records); result[ATLAS] = records[ATLAS][:-len(raw)]+payload
    require(set(result) == set(records), 'Archive resource set changed')
    require(all(result[p] == before for p, before in records.items() if p != ATLAS),
            'A non-Diffuse resource changed')
    channel_delta = revised[..., :3].astype(int)-atlas[..., :3].astype(int)
    require(channel_delta.min() >= 0 and channel_delta.max() <= 24, 'Hand luminance bound exceeded')
    report = dict(source_glb_sha256=SOURCE_ID, artwork_sha256=ARTWORK_SHA256,
        changed_resources=[ATLAS], changed_resource_count=1,
        changed_pixels=int(changed.sum()), changed_rgb_bytes=int(np.count_nonzero(channel_delta)),
        maximum_channel_lift=int(channel_delta.max()), mean_active_luminance_lift=float(scalar[changed].mean()),
        measured_hand_pixels=int(hand.sum()), safe_pixels=int(safe.sum()),
        protected_nonhand_pixels=int(nonhand.sum()), protected_wrist_pixels=int(wrist.sum()),
        protected_distal_finger_nail_pixels=int(nails.sum()),
        direct_atlas_mesh_readers=len(meshes), hand_readers=sum(m['hand'] for m in meshes),
        reader_resources=meshes, projections=panels,
        transfer='imagegen positive luminance residual minus panel median; max 28; strength .85; five-texel interior feather; native 0xffff ST and point/linear sample protection',
        all_geometry_uvs_normals_poses_skinning_props_display_lists_byte_identical=True,
        all_nonhand_wrist_nail_and_outside_mask_pixels_byte_identical=True,
        diffuse_header_dimensions_format_scale_and_alpha_byte_identical=True,
        blue_eyes_face_normal_and_ocarina_repairs_preserved=True,
        runtime_tested=False, accepted=False, promoted=False,
        status='HAND_PAINT_OFFLINE_POC_RUNTIME_AND_USER_ACCEPTANCE_PENDING')
    return result, report, dict(safe=safe, changed=changed, hand=hand, nonhand=nonhand, wrist=wrist, nails=nails)


def build(parent, source_path, artwork_path, output, report_path=None, native=None, audit_dir=None):
    parent, source_path, artwork_path, output = map(Path, (parent, source_path, artwork_path, output))
    protected = (parent, source_path, artwork_path, native)
    require_separate_destination(output, *protected, report_path)
    if report_path is not None:
        require_separate_destination(report_path, *protected, output)
    audit_targets = [] if audit_dir is None else [Path(audit_dir)/(n+'-mask.png')
                         for n in ('safe', 'changed', 'hand', 'nonhand', 'wrist', 'nails')]
    for target in audit_targets:
        require_separate_destination(target, *protected, output, report_path,
                                     *[p for p in audit_targets if p != target])
    before = parent.read_bytes()
    with zipfile.ZipFile(parent) as archive:
        require(len(archive.namelist()) == len(set(archive.namelist())), 'Duplicate archive paths')
        records = {p: archive.read(p) for p in archive.namelist()}
    result, report, masks = repair_records(records, source_path, artwork_path)
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for name, raw in sorted(result.items()):
            entry = zipfile.ZipInfo(name, (2026, 10, 21, 0, 0, 0)); entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, raw)
    with zipfile.ZipFile(output) as archive:
        require({p: archive.read(p) for p in archive.namelist()} == result, 'Exported archive read-back differs')
    require(parent.read_bytes() == before, 'Parent archive changed')
    report.update(parent=str(parent), parent_sha256=digest(before), output=str(output),
                  output_sha256=digest(output.read_bytes()))
    if audit_dir is not None:
        Path(audit_dir).mkdir(parents=True, exist_ok=True)
        for name, mask in masks.items():
            Image.fromarray(mask.astype(np.uint8)*255).save(Path(audit_dir)/(name+'-mask.png'))
    if report_path is not None:
        Path(report_path).write_text(json.dumps(report, indent=2)+'\n')
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    prep = sub.add_parser('prepare'); prep.add_argument('source'); prep.add_argument('directory')
    apply = sub.add_parser('build')
    for name in ('parent', 'source', 'artwork', 'output'):
        apply.add_argument(name)
    apply.add_argument('--native'); apply.add_argument('--report'); apply.add_argument('--audit-dir')
    args = parser.parse_args()
    if args.command == 'prepare':
        result = prepare(args.source, args.directory)
    else:
        result = build(args.parent, args.source, args.artwork, args.output, args.report, args.native, args.audit_dir)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
