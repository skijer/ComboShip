"""Confine Saria's native-reference blue hue to existing colored iris pixels."""
import colorsys
import numpy as np
from native_assets.path_guard import require_separate_destination


def recolor_pixels(image, iris_mask, hue):
    image = np.asarray(image)
    rgb = image[..., :3].astype(float)
    changed = (np.asarray(iris_mask, bool) & (rgb[..., 1] > rgb[..., 0]*1.3) &
               (rgb[..., 1] > rgb[..., 2]*1.25) & (rgb[..., 1] > 55))
    result = image.copy()
    values = []
    for color in rgb[changed]/255:
        _, saturation, value = colorsys.rgb_to_hsv(*color)
        values.append(np.rint(np.array(colorsys.hsv_to_rgb(hue, saturation, value))*255).astype(np.uint8))
    if values:
        result[changed, :3] = np.asarray(values)
    return result, changed


def select_iris_mask(image, variant, bounds):
    """Find separate colored iris components at the reviewed eye apertures.

    The seed strip sits below each pupil. Green hair and brows are separate
    components above/outside it; selecting the whole eye mesh is unsafe because
    its UV islands also include those features. Ambiguous ownership is rejected.
    """
    from scipy.ndimage import label, binary_dilation
    from native_npc_face import LANDMARKS, eye_mask
    image = np.asarray(image)
    rgb = image[..., :3].astype(float)
    green = ((rgb[..., 1] > rgb[..., 0]*1.3) & (rgb[..., 1] > rgb[..., 2]*1.25))
    blue = ((rgb[..., 2] > rgb[..., 0]*1.3) & (rgb[..., 2] > rgb[..., 1]*1.15))
    colored = (green | blue) & (rgb.max(2) > 55)
    size = image.shape[0]
    if image.shape[:2] != (size, size):
        raise ValueError('The reviewed face projection must be square')
    x, y, _, _, _, _ = LANDMARKS[variant]
    yy, xx = np.indices((size, size))
    sx = bounds[0]+xx/(size-1)*(bounds[2]-bounds[0])
    sy = bounds[3]-yy/(size-1)*(bounds[3]-bounds[1])
    eligible = colored & eye_mask(variant, bounds, size=size)
    components, _ = label(eligible)
    result = np.zeros((size, size), bool)
    records = []
    for side, center in (('R', -x), ('L', x)):
        seed = (abs(sx-center) < .010) & (abs(sy-(y-.006)) < .006)
        ids, overlaps = np.unique(components[seed], return_counts=True)
        choices = sorted(((int(n), int(i)) for i, n in zip(ids, overlaps) if i), reverse=True)
        if not choices:
            continue
        overlap, chosen = choices[0]
        if overlap < 10:
            continue  # Closed lids can contain isolated colored edge fragments.
        if len(choices) > 1 and choices[1][0] > overlap*.25:
            raise ValueError('Ambiguous '+side+' iris ownership; refusing to repaint surrounding features')
        component = components == chosen
        area = int(component.sum())
        if not 60 <= area <= 4000:
            raise ValueError('The '+side+' iris component escaped the reviewed size bounds')
        component = binary_dilation(component, iterations=2) & eligible
        # Include only neighboring edge fragments. The pupil and white catchlight
        # stay excluded by the original chroma/value predicate.
        result |= component
        rows, columns = np.where(component)
        records.append(dict(side=side, pixels=int(component.sum()),
                            bounds=[int(columns.min()), int(rows.min()), int(columns.max())+1, int(rows.max())+1]))
    return result, records


def map_projection_mask(positions, uv, faces, projection_mask, bounds, dimensions):
    """Rasterize source XYZ ownership into the unchanged atlas UV islands."""
    width, height = dimensions
    pixel = np.asarray(uv)*[width-1, height-1]
    result = np.zeros((height, width), bool)
    ph, pw = projection_mask.shape
    x0, y0, x1, y1 = bounds
    for face in faces:
        points, q = np.asarray(positions)[face], pixel[face]
        if (points[:, 0].max() < x0 or points[:, 0].min() > x1 or
                points[:, 1].max() < y0 or points[:, 1].min() > y1):
            continue
        low = np.maximum(np.floor(q.min(0)).astype(int), 0)
        high = np.minimum(np.ceil(q.max(0)).astype(int), [width-1, height-1])
        if np.any(high < low):
            continue
        matrix = np.column_stack([q[0]-q[2], q[1]-q[2]])
        if abs(np.linalg.det(matrix)) < 1e-10:
            continue
        xx, yy = np.meshgrid(np.arange(low[0], high[0]+1), np.arange(low[1], high[1]+1))
        b = (np.stack([xx, yy], -1)-q[2])@np.linalg.inv(matrix).T
        bary = np.dstack([b, 1-b.sum(-1)])
        xyz = bary@points
        px = np.rint((xyz[..., 0]-x0)/(x1-x0)*(pw-1)).astype(int)
        py = np.rint((y1-xyz[..., 1])/(y1-y0)*(ph-1)).astype(int)
        inside = (bary.min(-1) >= -1e-6) & (px >= 0) & (px < pw) & (py >= 0) & (py < ph)
        ids = np.where(inside)
        selected = projection_mask[py[ids], px[ids]]
        result[yy[ids][selected], xx[ids][selected]] = True
    return result


def native_blue_reference(native):
    """Measure blue hue from the exact native eye and palette, without copying art."""
    import hashlib
    import zipfile
    from native_assets import o2r as asset, n64texture
    with zipfile.ZipFile(native) as archive:
        eye = archive.read('objects/object_sa/gSariaEyeOpenTex')
        palette = archive.read('objects/object_sa/gSariaEyeTLUT')
    _, w, h, raw = asset.read_texture(eye)
    _, _, _, tlut = asset.read_texture(palette)
    image = n64texture.decode(raw, 2, 1, w, h, tlut)
    rgb = image[..., :3].astype(float)
    selected = ((rgb[..., 2] > rgb[..., 0]*1.5) &
                (rgb[..., 2] > rgb[..., 1]*1.25) & (rgb[..., 2] > 60))
    hues = [colorsys.rgb_to_hsv(*(color/255))[0] for color in rgb[selected]]
    if len(hues) < 30:
        raise ValueError('Native Saria blue iris reference was not recovered')
    hue = float(np.median(hues))
    if not .58 < hue < .72:
        raise ValueError('Native Saria iris reference is not blue')
    return hue, dict(eye_resource='objects/object_sa/gSariaEyeOpenTex',
                     palette_resource='objects/object_sa/gSariaEyeTLUT',
                     eye_sha256=hashlib.sha256(eye).hexdigest(),
                     palette_sha256=hashlib.sha256(palette).hexdigest(),
                     selected_native_blue_pixels=len(hues), blue_hue_degrees=hue*360)


def recolor(native, baseline, output, source=None, audit_directory=None):
    import hashlib
    import json
    from pathlib import Path
    import zipfile
    from PIL import Image
    from scipy.ndimage import binary_dilation
    from native_assets import o2r as asset
    from native_npc_face import bake, face_bounds
    from native_npc_models import SourceModel

    baseline, output, native = map(Path, (baseline, output, native))
    audit = Path(audit_directory) if audit_directory else None
    destinations = [output]
    if audit:
        for variant in ('saria-anime', 'saria-render'):
            destinations += [audit/(variant+'-'+label+'-'+suffix+'.png')
                             for label in ('EyesNeutral', 'EyesHalf', 'EyesClosed', 'Diffuse')
                             for suffix in ('mask', 'before', 'after')]
            destinations += [audit/(variant+suffix) for suffix in
                             ('-open-before.png', '-open-after.png',
                              '-exact-changed-pixel-masks.npz', '-report.json')]
    # Reports and images can alias an input or the candidate just as an archive
    # output can. Check every possible variant before parsing or writing.
    for index, destination in enumerate(destinations):
        require_separate_destination(destination, baseline, native, source, *destinations[:index])
    with zipfile.ZipFile(baseline) as archive:
        records = {name: archive.read(name) for name in archive.namelist()}
    manifests = [name for name in records if name.endswith('_NativeNpcModelInfo.json')]
    if len(manifests) != 1:
        raise ValueError('The candidate needs one exact model manifest')
    metadata = json.loads(records[manifests[0]])
    variant = metadata['variant']
    expected = {'saria-anime': 'dddeaed512e7b4aaedf960e1ddab13e74c480897e0f44404fb980c0c9ce5d63a',
                'saria-render': '902dfca93f4640a65a72e4266f855966fa708b412e0742789552f55f85c94aff'}
    if variant not in expected or metadata['source_sha256'] != expected[variant]:
        raise ValueError('This palette pass is bounded to the two reviewed Saria models')
    native_hash = hashlib.sha256(native.read_bytes()).hexdigest()
    if native_hash != metadata['native_sha256']:
        raise ValueError('Native input differs from the model manifest')
    hue, reference = native_blue_reference(native)
    namespace = metadata['namespace']
    bounds = face_bounds(variant)
    report = dict(variant=variant, parent_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),
                  native_sha256=native_hash, source_sha256=metadata['source_sha256'], reference=reference,
                  method='Hue-only native Saria blue applied to connected colored iris pixels; existing saturation/value and all drawn shapes retained',
                  runtime_tested=False, user_accepted=False, textures=[])
    changes, masks = {}, {}
    if audit:
        audit.mkdir(parents=True, exist_ok=True)

    def active(path):
        return 'alt/'+path if 'alt/'+path in records else path

    def process(name, image, mask, owners):
        changed_image, changed = recolor_pixels(image, mask, hue)
        if not np.array_equal(image[~changed], changed_image[~changed]):
            raise ValueError('A protected non-iris pixel changed')
        if (not np.array_equal(image[changed, :3].max(1), changed_image[changed, :3].max(1)) or
                not np.array_equal(image[changed, :3].min(1), changed_image[changed, :3].min(1))):
            raise ValueError('The hue-only transform changed saturation or value')
        if not np.array_equal(image[..., 3], changed_image[..., 3]):
            raise ValueError('Iris recolor changed alpha')
        protected = hashlib.sha256(image[~changed].tobytes()).hexdigest()
        detail = dict(resource=name, dimensions=[int(image.shape[1]), int(image.shape[0])],
                      iris_ownership=owners, iris_mask_pixels=int(mask.sum()),
                      changed_pixels=int(changed.sum()), mask_sha256=hashlib.sha256(np.packbits(changed).tobytes()).hexdigest(),
                      non_iris_pixel_sha256=protected,
                      non_iris_pixels_alpha_and_header_byte_identical=True,
                      iris_saturation_and_value_identical=True)
        if changed.any():
            raw = records[name]
            offset = len(raw)-image.nbytes
            changes[name] = raw[:offset]+changed_image.tobytes()
            detail['previous_sha256'] = hashlib.sha256(raw).hexdigest()
            detail['sha256'] = hashlib.sha256(changes[name]).hexdigest()
        report['textures'].append(detail)
        masks[name] = changed
        if audit:
            label = variant+'-'+name.rsplit('/', 1)[-1]
            Image.fromarray((changed*255).astype(np.uint8)).save(audit/(label+'-mask.png'))
            if image.shape[0] == 512:
                Image.fromarray(image).save(audit/(label+'-before.png'))
                Image.fromarray(changed_image).save(audit/(label+'-after.png'))
        return changed_image

    # Projected expression art is already in source XY, including its eyelids.
    for label in ('EyesNeutral', 'EyesHalf', 'EyesClosed'):
        name = active(namespace+'/Textures/'+label)
        if name not in records:
            continue
        kind, w, h, raw = asset.read_texture(records[name])
        if (kind, w, h) != (1, 512, 512):
            raise ValueError('The projected eye texture layout differs: '+name)
        image = np.frombuffer(raw, np.uint8).reshape(h, w, 4)
        mask, owners = select_iris_mask(image, variant, bounds)
        if label != 'EyesClosed' and len(owners) != 2:
            raise ValueError('Both visible irises must be identified before changing '+name)
        if label == 'EyesClosed' and owners:
            raise ValueError('Closed-eye art has ambiguous colored ownership; refusing to repaint it')
        process(name, image, mask, owners)

    if variant == 'saria-anime':
        if source is None:
            raise ValueError('Anime iris ownership requires its exact source GLB')
        model = SourceModel(source)
        if hashlib.sha256(model.raw).hexdigest() != expected[variant]:
            raise ValueError('Source GLB differs from the reviewed anime model')
        head = model.meshes[0]
        if head['name'] != 'tripo_part_0':
            raise ValueError('The reviewed source head ownership differs')
        name = active(namespace+'/Textures/Diffuse')
        kind, w, h, raw = asset.read_texture(records[name])
        if (kind, w, h) != (1, 4096, 4096):
            raise ValueError('The reviewed source atlas layout differs')
        image = np.frombuffer(raw, np.uint8).reshape(h, w, 4)
        positions, uv = head['positions'], head['uv']
        front = head['faces'][(positions[head['faces'], 2] > .015).all(1)]
        projection, *_ = bake(positions, uv, image, front, bounds)
        projected_mask, owners = select_iris_mask(projection, variant, bounds)
        if len(owners) != 2:
            raise ValueError('Both source-atlas irises must be identified')
        mask = map_projection_mask(positions, uv, front,
                                   binary_dilation(projected_mask, iterations=1), bounds, (w, h))
        changed_image = process(name, image, mask, owners)
        after, *_ = bake(positions, uv, changed_image, front, bounds)
        after_mask, _ = select_iris_mask(after, variant, bounds)
        old_mask, _ = select_iris_mask(projection, variant, bounds)
        old_green = recolor_pixels(projection, old_mask, hue)[1].sum()
        remaining_green = recolor_pixels(after, after_mask, hue)[1].sum()
        report['open_projection_green_pixels_before'] = int(old_green)
        report['open_projection_green_pixels_after'] = int(remaining_green)
        # A few bilinear edge pixels mix protected dark pupil/white texels.
        # Keep this absolute floor so rerunning the palette pass stays safe.
        if remaining_green > max(8, old_green*.01):
            raise ValueError('The native blue pass left a visible green iris remnant')
        if audit:
            Image.fromarray(projection).save(audit/(variant+'-open-before.png'))
            Image.fromarray(after).save(audit/(variant+'-open-after.png'))
    else:
        # Both reviewed traditional active eye variants are already blue. Its
        # source/corrected diffuse atlases are preserved verbatim as well.
        report['existing_traditional_blue_art_preserved'] = not bool(changes)

    report['changed_texture_resources'] = sorted(changes)
    report['all_geometry_normals_uvs_rig_face_shapes_pupils_catchlights_lashes_hair_skin_clips_and_other_resources_byte_identical'] = True
    report['changed_pixels'] = sum(item['changed_pixels'] for item in report['textures'])
    records.update(changes)
    info = namespace+'/IrisBlueColorInfo.json'
    records[info] = json.dumps(report, indent=2).encode()
    output.parent.mkdir(parents=True, exist_ok=True)
    asset.O2R(files=records).write(output, compression=zipfile.ZIP_DEFLATED,
                                 timestamp=(2026, 10, 20, 0, 0, 0))
    with zipfile.ZipFile(baseline) as before, zipfile.ZipFile(output) as after:
        actual = {name for name in before.namelist() if before.read(name) != after.read(name)}
        expected_changes = set(changes) | ({info} if info in before.namelist() else set())
        if actual != expected_changes:
            raise ValueError('The palette pass changed a protected resource')
        if after.testzip() is not None:
            raise ValueError('Candidate ZIP integrity failure')
        report['protected_resources_verified_byte_identical'] = len(before.namelist())-len(actual)
    report['output_sha256'] = hashlib.sha256(output.read_bytes()).hexdigest()
    if audit:
        np.savez_compressed(audit/(variant+'-exact-changed-pixel-masks.npz'), **masks)
        (audit/(variant+'-report.json')).write_text(json.dumps(report, indent=2)+'\n')
    return report


if __name__ == '__main__':
    import argparse
    import json
    from pathlib import Path
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('native', 'baseline', 'output'):
        parser.add_argument(name, type=Path)
    parser.add_argument('--source', type=Path)
    parser.add_argument('--audit-directory', type=Path)
    args = parser.parse_args()
    print(json.dumps(recolor(args.native, args.baseline, args.output, args.source, args.audit_directory), indent=2))
