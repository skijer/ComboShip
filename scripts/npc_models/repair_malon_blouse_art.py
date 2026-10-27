"""Bounded archive-only white blouse artwork POC for adult Malon.

Use the exact FaceNormals POC1 parent, its actual exported geometry/UVs, and a
registered imagegen-authored projection. Only connected, visible front-blouse
texels exclusively owned by the chest may change. All other resources and alpha
bytes are preserved. This is offline evidence, never runtime acceptance.
"""
import argparse
import hashlib
import json
from pathlib import Path
import zipfile

import numpy as np
from PIL import Image
from scipy.ndimage import binary_dilation, binary_fill_holes, binary_propagation, distance_transform_edt, map_coordinates

from native_npc_face import bake
from native_assets.path_guard import require_separate_destination
from native_assets.npc_sampling import native_uv_modes
from native_npc_models import NativeRig, VARIANTS
from verify_native_npc_models import Archive

PARENT_SHA256 = 'bfa988d27caa6a4a02fea221aab160daad3f5e204159ebe2bd9c7fcb7cf89e8f'
NATIVE_SHA256 = 'ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b'
SOURCE_SHA256 = '11d200aadce51cc51f3181c5d64ffb990392b28fed53158fc544cc92703e2fe0'
BOUNDS = [-950., 3500., 650., 5100.]
CHEST = 10


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def cloth_color(image):
    """Warm ivory and its gray folds; saturated details need a separate veto."""
    rgb = np.asarray(image)[..., :3].astype(float)
    r, g, b = np.moveaxis(rgb, -1, 0)
    return ((rgb.max(-1)-rgb.min(-1) < 60) & (abs(r-g) < 32)
            & (b >= g*.42) & (g >= r*.88) & (b <= g+20) & (rgb.max(-1) > 20))


def semantic_blouse_mask(base, owners, face_limbs, bounds):
    """Grow only connected neutral fabric from two measured chest seed patches.

    A hard surface/region gate rejects sleeves, buckle and neck before color
    connectivity. Saturated details stay holes. The feather is inside the
    connected fabric, so no surrounding rectangle receives generated colors.
    """
    base, owners, face_limbs = map(np.asarray, (base, owners, face_limbs))
    h, w = owners.shape
    yy, xx = np.indices((h, w)); x0, y0, x1, y1 = bounds
    x = x0+xx/(w-1)*(x1-x0); y = y1-yy/(h-1)*(y1-y0)
    host = np.full(owners.shape, -1, int)
    valid = owners >= 0; host[valid] = face_limbs[owners[valid]]
    gate = ((host == CHEST) & (x > -680) & (x < 260) & (y > 3825) & (y < 4510))
    rgb = base[..., :3].astype(float)
    r, g, b = np.moveaxis(rgb, -1, 0)
    yellow = (r > g*1.03) & (r < g*1.5) & (r-g > 8) & (b < g*.70) & (g > 70)
    # Connected yellow plus its enclosed ink and a measured five-pixel rim
    # protects the actual scarf design. Dark neutral folds outside that rim
    # belong to the white fabric and may receive the clean authored ivory.
    scarf = binary_dilation(binary_fill_holes(yellow), iterations=max(2, round(h/205)))
    eligible = cloth_color(base) & gate & ~scarf
    seeds = np.zeros(owners.shape, bool)
    for cx, cy in ((-470., 4150.), (70., 4140.)):
        seeds |= ((x-cx)/70.)**2+((y-cy)/110.)**2 < 1
    region = binary_propagation(seeds & eligible, mask=eligible)
    # Filling only neutral holes catches enclosed dark folds while saturated
    # scarf/skin pixels remain protected by the final eligibility intersection.
    region = binary_fill_holes(region) & eligible
    return np.clip(distance_transform_edt(region)/max(2., h/256.), 0., 1.)


def uv_fragments(uv, faces, shape, padding=0):
    """Yield covered texels, with optional bounded bilinear-sampling gutters."""
    height, width = shape
    pixel = np.asarray(uv)*[width-1, height-1]
    for index, face in enumerate(faces):
        q = pixel[face]
        low = np.maximum(np.floor(q.min(0)).astype(int)-padding, 0)
        high = np.minimum(np.ceil(q.max(0)).astype(int)+padding, [width-1, height-1])
        if np.any(high < low):
            continue
        matrix = np.column_stack([q[0]-q[2], q[1]-q[2]])
        if abs(np.linalg.det(matrix)) < 1e-10:
            continue
        xx, yy = np.meshgrid(np.arange(low[0], high[0]+1), np.arange(low[1], high[1]+1))
        b = (np.stack([xx, yy], -1)-q[2])@np.linalg.inv(matrix).T
        weights = np.dstack([b, 1-b.sum(-1)])
        edges = np.linalg.norm(q[[1, 2, 0]]-q[[2, 0, 1]], axis=1)
        altitude = abs(np.linalg.det(matrix))/np.maximum(edges, 1e-12)
        keep = (weights*altitude).min(-1) >= -padding-1e-6
        if keep.any():
            selected = weights[keep].clip(0., 1.)
            selected /= selected.sum(1, keepdims=True)
            yield index, xx[keep], yy[keep], selected


def native_protection_mask(uv, faces, shape):
    mask = np.zeros(shape, bool)
    height, width = shape
    # uv_fragments uses array-index coordinates UV*(size-1). Native shader
    # coordinates address texel centers at (index+.5)/size; convert modes to
    # that index frame before conservatively covering their filter kernels.
    modes = [uv]
    for mode in native_uv_modes(uv):
        modes.append((mode*[width,height]-.5)/[width-1,height-1])
    for mode in modes:
        for _, xx, yy, _ in uv_fragments(mode, faces, shape, padding=2):
            mask[yy, xx] = True
    return mask


def transfer_blouse_art(points, uv, atlas, faces, face_limbs, artwork, bounds=BOUNDS):
    """Transfer registered art only onto exclusively owned visible fabric UVs.

    Every non-chest triangle is an unconditional protection mask, including UV
    overlaps. Hidden chest surfaces and non-fabric chest pixels also veto a
    shared texel. The final UV inset blends within the legal footprint.
    """
    points, uv, atlas, faces, face_limbs, artwork = map(np.asarray,
        (points, uv, atlas, faces, face_limbs, artwork))
    if artwork.ndim != 3 or artwork.shape[0] != artwork.shape[1] or artwork.shape[2] != 4:
        raise ValueError('A square, registered RGBA blouse projection is required')
    size = artwork.shape[0]
    base, _, valid, owners = bake(points, uv, atlas, faces, bounds, size=size,
                                  face_labels=np.arange(len(faces)))
    mask = semantic_blouse_mask(base, owners, face_limbs, bounds)
    # A z plane from each visible triangle rejects returning or hidden surfaces.
    triangle = points[faces]
    plane = np.zeros((len(faces), 3), float)
    design = np.concatenate([triangle[:, :, :2], np.ones((len(faces), 3, 1))], axis=2)
    nonsingular = abs(np.linalg.det(design)) > 1e-8
    plane[nonsingular] = np.linalg.solve(design[nonsingular], triangle[nonsingular, :, 2, None])[..., 0]
    protection = native_protection_mask(uv, faces[face_limbs != CHEST], atlas.shape[:2])
    weight_uv = np.zeros(atlas.shape[:2], np.float32)
    color = np.zeros(atlas.shape[:2]+(3,), np.float32)
    totals = np.zeros(atlas.shape[:2], np.float32)
    x0, y0, x1, y1 = bounds
    for index, xx, yy, bary in uv_fragments(uv, faces, atlas.shape[:2], padding=2):
        if face_limbs[index] != CHEST:
            protection[yy, xx] = True
            continue
        source = bary@triangle[index]
        s = (source[:, 0]-x0)/(x1-x0)*(size-1)
        t = (y1-source[:, 1])/(y1-y0)*(size-1)
        inside = (s >= 0) & (s <= size-1) & (t >= 0) & (t <= size-1)
        owner = owners[np.clip(np.rint(t).astype(int), 0, size-1),
                       np.clip(np.rint(s).astype(int), 0, size-1)]
        safe_owner = np.maximum(owner, 0)
        front_z = (np.c_[source[:, :2], np.ones(len(source))]*plane[safe_owner]).sum(1)
        visible = ((owner >= 0) & (face_limbs[safe_owner] == CHEST)
                   & nonsingular[safe_owner] & (abs(source[:, 2]-front_z) <= 8.))
        weights = map_coordinates(mask, [t, s], order=1, mode='nearest')
        paint = np.stack([map_coordinates(artwork[..., c].astype(float), [t, s], order=1,
                                         mode='nearest') for c in range(3)], axis=1)
        permitted = (inside & visible & (weights > 0) & cloth_color(atlas[yy, xx]) & cloth_color(paint))
        protection[yy[~permitted], xx[~permitted]] = True
        if not permitted.any():
            continue
        qx, qy = xx[permitted], yy[permitted]
        amount = weights[permitted].astype(np.float32)
        weight_uv[qy, qx] = np.maximum(weight_uv[qy, qx], amount)
        color[qy, qx] += paint[permitted].astype(np.float32)*amount[:, None]
        totals[qy, qx] += amount
    allowed = (weight_uv > 0) & ~protection
    weight_uv[~allowed] = 0
    # The two-texel ownership gutter keeps original colors from leaking
    # through bilinear filtering at disconnected UV chart seams. Feather only
    # its outer rim; the true chart edge receives the full semantic weight.
    weight_uv *= np.clip(distance_transform_edt(allowed)/2., 0., 1.)
    out = atlas.copy()
    art_rgb = color[allowed]/totals[allowed, None]
    amount = weight_uv[allowed, None]
    out[allowed, :3] = np.rint(atlas[allowed, :3]*(1-amount)+art_rgb*amount).clip(0, 255).astype(np.uint8)
    # Alpha and every non-owned texel are explicit invariants, not inferred from
    # a visually similar renderer result.
    out[..., 3] = atlas[..., 3]
    if np.any(out[~allowed] != atlas[~allowed]):
        raise AssertionError('Paint escaped its exclusive connected blouse UV footprint')
    return out, allowed, dict(base=base, projection_mask=mask, uv_weight=weight_uv,
                              protected_uv=protection, owners=owners, valid=valid)


def exported_geometry(archive, rig):
    """Read the actual exported skin-cache geometry in the recorded idle frame."""
    matrices = archive.matrices(rig)
    points, uv, faces, hosts = [], [], [], []
    for host, limb in enumerate(rig.limbs):
        if not limb.dlist:
            continue
        path = rig.ns+f'/Body/Limb{host:02d}'
        if archive.skin(rig) and host != rig.profile['head']:
            path = rig.ns+f'/Skin/Limbs/Limb{host:02d}'
        data = archive.geometry(path, rig, host)
        if not len(data['faces']):
            continue
        world = np.zeros_like(data['local'])
        for bone in np.unique(data['bones']):
            keep = data['bones'] == bone
            matrix = matrices[bone]
            world[keep] = data['local'][keep]@matrix[:3, :3].T+matrix[:3, 3]
        if set(data['textures']) != {rig.ns+'/Textures/Diffuse'}:
            raise ValueError('Unexpected material in the exact parent projection')
        faces.extend(data['faces']+len(points)); points.extend(world); uv.extend(data['uv'])
        hosts.extend([host]*len(data['faces']))
    return tuple(map(np.asarray, (points, uv, faces, hosts)))


def write_records(output, records):
    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as result:
        for name, raw in sorted(records.items()):
            info = zipfile.ZipInfo(name, (2026, 10, 21, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            result.writestr(info, raw)


def verify_readback(parent, output, texture, expected, allowed, manifest):
    """Independently reopen the candidate and assert the complete byte contract."""
    from native_npc_models import asset
    with zipfile.ZipFile(parent) as before, zipfile.ZipFile(output) as after:
        if before.namelist() and set(before.namelist()) != set(after.namelist()):
            raise AssertionError('Archive resource identities changed')
        changed = [name for name in before.namelist() if before.read(name) != after.read(name)]
        if set(changed) != {texture, manifest}:
            raise AssertionError(f'Unexpected changed resources: {changed}')
        original = asset.read_texture(before.read(texture))
        revised = asset.read_texture(after.read(texture))
        if original[:3] != revised[:3] or revised[:3] != (1, expected.shape[1], expected.shape[0]):
            raise AssertionError('Diffuse dimensions/type changed')
        old = np.frombuffer(original[3], np.uint8).reshape(expected.shape)
        new = np.frombuffer(revised[3], np.uint8).reshape(expected.shape)
        np.testing.assert_array_equal(new, expected)
        np.testing.assert_array_equal(old[~allowed], new[~allowed])
        np.testing.assert_array_equal(old[:, :, 3], new[:, :, 3])
        # The entire texture header, including 8x logical scaling, is protected.
        if before.read(texture)[:-old.nbytes] != after.read(texture)[:-new.nbytes]:
            raise AssertionError('Diffuse header/scaling changed')
        return dict(changed_resources=sorted(changed), changed_resource_count=len(changed),
                    protected_resource_count=len(before.namelist())-len(changed),
                    changed_uv_pixel_count=int(np.any(old != new, axis=2).sum()),
                    allowed_uv_pixel_count=int(allowed.sum()),
                    protected_uv_pixel_count=int((~allowed).sum()),
                    all_other_resources_byte_identical=True, alpha_bytes_identical=True,
                    texture_header_and_scaling_byte_identical=True)


def build(native, parent, artwork, output, evidence_dir=None, render_previews=False):
    native, parent, artwork, output = map(Path, (native, parent, artwork, output))
    require_separate_destination(output, native, parent, artwork)
    # Check every write destination before opening input archives or artwork.
    # Also reject destinations that alias one another through symlinks/hardlinks.
    destinations = [output]
    if evidence_dir is not None:
        root = Path(evidence_dir)
        names = ['blouse-source-projection.png', 'blouse-connected-projection-mask.png',
                 'blouse-allowed-uv-mask.png', 'blouse-changed-uv-mask.png',
                 'blouse-uv-blend.png', 'blouse-candidate-projection.png', 'blouse-report.json']
        if render_previews:
            names += [f'previews/blouse-{label}-{light}.png'
                      for label in ('parent', 'candidate') for light in ('unlit', 'lit')]
        for name in names:
            target = root/name
            require_separate_destination(target, native, parent, artwork, *destinations)
            destinations.append(target)
    if digest(parent) != PARENT_SHA256 or digest(native) != NATIVE_SHA256:
        raise ValueError('Use the exact frozen native and FaceNormals POC1 parent')
    archive = Archive(native, parent)
    try:
        records = {name: archive.candidate.read(name) for name in archive.candidate.namelist()}
        manifest = 'malon_NativeNpcModelInfo.json'; meta = json.loads(records[manifest])
        if meta['variant'] != 'malon' or meta['source_sha256'] != SOURCE_SHA256:
            raise ValueError('Wrong Malon source identity')
        rig = NativeRig(archive, VARIANTS['malon']['profile'], private=True)
        points, uv, faces, hosts = exported_geometry(archive, rig)
        texture = rig.ns+'/Textures/Diffuse'
        if 'alt/'+texture in records:
            texture = 'alt/'+texture
        kind, width, height, raw = archive.texture(rig.ns+'/Textures/Diffuse')
        if (kind, width, height) != (1, 4096, 4096):
            raise ValueError('The exact RGBA32 source atlas must be retained')
        atlas = np.frombuffer(raw, np.uint8).reshape(height, width, 4)
        art = np.asarray(Image.open(artwork).convert('RGBA'))
        if art.shape != (1024, 1024, 4):
            raise ValueError('Register the generated edit to the 1024 square projection first')
        revised, allowed, masks = transfer_blouse_art(points, uv, atlas, faces, hosts, art)
        changed = np.any(revised != atlas, axis=2)
        if changed.sum() < 1000 or changed.sum() > 450000:
            raise ValueError('The white-front paint footprint is empty or unexpectedly broad')
        if not records[texture].endswith(raw):
            raise ValueError('Unexpected texture payload layout')
        records[texture] = records[texture][:-len(raw)]+revised.tobytes()
        report = dict(parent_sha256=PARENT_SHA256, native_sha256=NATIVE_SHA256,
                      source_sha256=SOURCE_SHA256, artwork_sha256=digest(artwork),
                      projection_bounds=BOUNDS, projection_size=1024,
                      mask='connected source neutral front fabric; protected yellow scarf+enclosed ink+5px rim; chest-only; visible-depth ownership; 2texel chart gutters; native 0xffff ST point/linear sampling veto for other UV owners; inward semantic feather',
                      geometry_uvs_normals_rig_poses_all_other_textures_byte_identical=True,
                      scarf_knot_blue_embroidery_sleeves_belt_body_and_alpha_protected=True,
                      runtime_tested=False, accepted=False, promoted=False)
        meta['white_blouse_art_poc'] = report
        records[manifest] = json.dumps(meta, indent=2).encode()
        write_records(output, records)
    finally:
        archive.native.close(); archive.candidate.close()
    report['readback'] = verify_readback(parent, output, texture, revised, allowed, manifest)
    report.update(candidate=str(output), candidate_sha256=digest(output))
    if evidence_dir is not None:
        evidence_dir = Path(evidence_dir); evidence_dir.mkdir(parents=True, exist_ok=True)
        base = masks['base'].copy(); base[~masks['valid']] = [28, 33, 40, 255]
        Image.fromarray(base).save(evidence_dir/'blouse-source-projection.png')
        Image.fromarray(np.rint(masks['projection_mask']*255).astype(np.uint8)).save(evidence_dir/'blouse-connected-projection-mask.png')
        Image.fromarray(allowed.astype(np.uint8)*255).save(evidence_dir/'blouse-allowed-uv-mask.png')
        Image.fromarray(changed.astype(np.uint8)*255).save(evidence_dir/'blouse-changed-uv-mask.png')
        Image.fromarray(np.rint(masks['uv_weight']*255).astype(np.uint8)).save(evidence_dir/'blouse-uv-blend.png')
        projection, _, valid = bake(points, uv, revised, faces, BOUNDS, size=1024)
        projection[~valid] = [28, 33, 40, 255]
        Image.fromarray(projection).save(evidence_dir/'blouse-candidate-projection.png')
        (evidence_dir/'blouse-report.json').write_text(json.dumps(report, indent=2)+'\n')
        if render_previews:
            from preview_native_npc_models import preview
            for label, path in (('parent', parent), ('candidate', output)):
                for unlit in (True, False):
                    preview((str(native), str(evidence_dir), 'malon', 'pose0', dict(
                        archive=str(path), bounds=BOUNDS, size=1024, supersample=1,
                        unlit=unlit, name=f'blouse-{label}-'+('unlit' if unlit else 'lit'))))
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('native', 'parent', 'artwork', 'output'):
        parser.add_argument(name, type=Path)
    parser.add_argument('--evidence-dir', type=Path)
    parser.add_argument('--render-previews', action='store_true')
    args = parser.parse_args()
    print(json.dumps(build(args.native, args.parent, args.artwork, args.output,
                           args.evidence_dir, args.render_previews), indent=2))


if __name__ == '__main__':
    main()
