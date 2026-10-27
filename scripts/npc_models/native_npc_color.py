"""Hash-bounded skin calibration on Traditional Saria's hands and ears.

The hands use continuous clean-cheek chroma and conservative luminance
normalization. The ears retain the earlier green-spill correction. The source
GLB and PIL image are never mutated. Other mesh UV footprints, including filter
support, are protected even if a future source shares atlas pixels.
"""
import hashlib

import numpy as np
from PIL import Image, ImageDraw, ImageFilter


TRADITIONAL_SARIA_SHA256 = "902dfca93f4640a65a72e4266f855966fa708b412e0742789552f55f85c94aff"
TARGET_PIECES = {4: "tripo_part_12", 7: "tripo_part_15", 8: "tripo_part_16", 11: "tripo_part_19"}
HAND_PIECES = (4, 8)
_LUMA = np.array([.2126, .7152, .0722])


def _linear(rgb):
    srgb = np.asarray(rgb, float) / 255.
    return np.where(srgb <= .04045, srgb / 12.92, ((srgb + .055) / 1.055) ** 2.4)


def _srgb(linear):
    linear = np.clip(linear, 0., 1.)
    value = np.where(linear <= .0031308, 12.92 * linear, 1.055 * linear ** (1. / 2.4) - .055)
    return np.rint(value * 255.).astype(np.uint8)


def green_weight(rgb):
    """Smoothly rise from neutral green dominance to the audited 1.10 ratio."""
    rgb = np.asarray(rgb, float)
    ratio = rgb[..., 1] / np.maximum(np.maximum(rgb[..., 0], rgb[..., 2]), 1.)
    t = np.clip((ratio - 1.) / .10, 0., 1.)
    return t * t * (3. - 2. * t)


def _mesh_footprint(meshes, size, padding):
    canvas = Image.new("L", size)
    draw = ImageDraw.Draw(canvas)
    for mesh in meshes:
        points = np.asarray(mesh["uv"], float) * np.asarray(size)
        for face in mesh["faces"]:
            draw.polygon([tuple(point) for point in points[face]], fill=255)
    if padding:
        canvas = canvas.filter(ImageFilter.MaxFilter(2 * padding + 1))
    return np.asarray(canvas) != 0


def uv_footprint(uv, faces, size, padding=2):
    """Conservative triangle coverage plus adjacent texture-filter texels."""
    return _mesh_footprint([dict(uv=uv, faces=faces)], size, padding)


def correct_green_chroma(pixels, target, protected, reference):
    """Return a new RGBA atlas; change only green, exclusively owned texels.

    Masks are pixel-space ownership established by the caller. Their overlap
    always favors protected geometry. Chroma replacement is performed in
    linear RGB at constant luminance, with a smooth transition in green ratio.
    """
    result = np.array(pixels, dtype=np.uint8, copy=True)
    pixels = np.asarray(pixels)
    if pixels.ndim != 3 or pixels.shape[-1] != 4:
        raise ValueError("Expected an RGBA diffuse atlas")
    masks = [np.asarray(mask, bool) for mask in (target, protected, reference)]
    if any(mask.shape != pixels.shape[:2] for mask in masks):
        raise ValueError("Diffuse ownership masks have the wrong dimensions")
    target, protected, reference = masks
    clean = _linear(pixels[reference, :3])
    clean_y = clean @ _LUMA
    clean = clean[clean_y > .01]
    clean_y = clean_y[clean_y > .01]
    if not len(clean):
        raise ValueError("No clean source skin texels are available for chroma repair")
    chroma = np.median(clean / clean_y[:, None], axis=0)
    chroma /= chroma @ _LUMA
    if chroma[0] <= chroma[1] or chroma[1] <= chroma[2]:
        raise ValueError("The reference does not contain warm clean skin chroma")

    safe = target & ~protected
    original = pixels[safe, :3]
    weight = green_weight(original)
    selected = weight > 0.
    rgb = original[selected]
    linear = _linear(rgb)
    y = linear @ _LUMA
    goal = y[:, None] * chroma
    # If needed, move the desired chroma toward neutral at the same luminance
    # rather than clipping a channel and flattening highlights.
    gamut = np.minimum(1., (1. - y) / np.maximum(goal.max(axis=1) - y, 1e-12))
    goal = y[:, None] + gamut[:, None] * (goal - y[:, None])
    corrected = _srgb(linear + weight[selected, None] * (goal - linear))
    output = original.copy()
    output[selected] = corrected
    result[safe, :3] = output
    strong_before = green_weight(original) >= 1. - 1e-10
    strong_after = green_weight(output) >= 1. - 1e-10
    error = np.abs(_linear(corrected) @ _LUMA - y)
    return result, dict(
        changed_texels=int(np.count_nonzero(np.any(output != original, axis=1))),
        original_strong_green_texels=int(strong_before.sum()),
        remaining_strong_green_texels=int(strong_after.sum()),
        protected_overlap_texels=int(np.count_nonzero(target & protected)),
        reference_texels=int(len(clean)),
        reference_linear_chroma=[round(float(value), 8) for value in chroma],
        max_linear_luminance_error=float(error.max(initial=0.)),
    )


def _calibrate_hand_skin(pixels, target, protected, reference, chroma):
    """Calibrate every owned hand texel without a hue threshold.

    Yellow-green source charts can meet at an identical surface point with
    their G/R ratio on opposite sides of one. Selecting by that ratio made
    those previously matching UV seams acquire different skin colors. A
    common chroma and a single affine luminance map avoid that discontinuity.
    Mixing halfway toward clean cheek luminance lifts baked underlighting
    while retaining the source's luminance ordering and half its contrast.
    """
    safe = target & ~protected
    rgb = pixels[safe, :3]
    original_y = _linear(rgb) @ _LUMA
    clean_y = _linear(pixels[reference, :3]) @ _LUMA
    reference_y = float(np.median(clean_y[clean_y > .01]))
    y = .5 * original_y + .5 * reference_y
    goal = y[:, None] * chroma
    gamut = np.minimum(1., (1. - y) / np.maximum(goal.max(axis=1) - y, 1e-12))
    goal = y[:, None] + gamut[:, None] * (goal - y[:, None])
    corrected = _srgb(goal)
    return safe, corrected, dict(
        target_mesh_ids=list(HAND_PIECES),
        changed_texels=int(np.count_nonzero(np.any(corrected != rgb, axis=1))),
        reference_linear_luminance=reference_y,
        original_median_linear_luminance=float(np.median(original_y)),
        corrected_median_linear_luminance=float(np.median(_linear(corrected) @ _LUMA)),
        max_luminance_mapping_error=float(np.max(np.abs(_linear(corrected) @ _LUMA - y), initial=0.)),
        original_luminance_contrast_retained=.5,
        method="all hand texels use clean cheek chroma; half source and half clean cheek linear luminance",
    )


def repair_traditional_saria_legs(source, pixels, reference_chroma):
    """Remove baked green from the two skin tubes, retaining their luminance.

    Boots and shorts are separate source pieces. Their complete padded UV
    footprints, and every other piece, take precedence over this correction.
    Continuous chroma avoids new boundaries between adjacent UV charts.
    """
    result = np.array(pixels, copy=True)
    report = dict(applied=False, changed_texels=0)
    if hashlib.sha256(source.raw).hexdigest() != TRADITIONAL_SARIA_SHA256:
        return result, report
    pieces = {18: 'tripo_part_3', 23: 'tripo_part_8'}
    for index, name in pieces.items():
        if source.meshes[index]['name'] != name:
            raise ValueError('Traditional Saria leg-piece identity changed')
    target = _mesh_footprint([source.meshes[i] for i in pieces], source.image.size, 2)
    protected = _mesh_footprint([m for i, m in enumerate(source.meshes) if i not in pieces], source.image.size, 2)
    safe = target & ~protected
    y = _linear(np.asarray(pixels)[safe, :3]) @ _LUMA
    chroma = np.asarray(reference_chroma, float)
    goal = y[:, None]*chroma
    gamut = np.minimum(1., (1.-y)/np.maximum(goal.max(axis=1)-y, 1e-12))
    goal = y[:, None]+gamut[:, None]*(goal-y[:, None])
    result[safe, :3] = _srgb(goal)
    changed = np.any(result != pixels, axis=-1)
    report.update(applied=True, target_mesh_ids=list(pieces),
                  changed_texels=int(changed.sum()), protected_overlap_texels=int((target & protected).sum()),
                  max_linear_luminance_error=float(np.abs(_linear(result[safe, :3])@_LUMA-y).max(initial=0)),
                  method='clean cheek chroma on leg skin only; source linear luminance retained')
    return result, report


def repair_traditional_saria_diffuse(source):
    """Return ``(rgba_pixels, report)`` for export, leaving source state intact.

    A different source is a byte-identical no-op. Matching source identity but
    unexpected piece names is an error: silently widening the correction to
    another skin/clothing region would break the bounded repair contract.
    """
    pixels = np.asarray(source.image)
    digest = hashlib.sha256(source.raw).hexdigest()
    report = dict(applied=False, source_sha256=digest, target_mesh_ids=[], changed_texels=0)
    if digest != TRADITIONAL_SARIA_SHA256:
        return pixels.copy(), report
    for index, name in TARGET_PIECES.items():
        if index >= len(source.meshes) or source.meshes[index]["name"] != name:
            raise ValueError("Traditional Saria skin-piece identity changed")
    target = _mesh_footprint([source.meshes[index] for index in TARGET_PIECES], source.image.size, 2)
    protected = _mesh_footprint([mesh for index, mesh in enumerate(source.meshes)
                                if index not in TARGET_PIECES], source.image.size, 2)

    face = source.meshes[6]
    if face["name"] != "tripo_part_14":
        raise ValueError("Traditional Saria clean-face reference identity changed")
    center = face["positions"][face["faces"]].mean(axis=1)
    cheek = ((abs(center[:, 0]) < .045) & (center[:, 1] > .81) &
             (center[:, 1] < .85) & (center[:, 2] > .035))
    reference = uv_footprint(face["uv"], face["faces"][cheek], source.image.size, padding=0)
    yy, xx = np.nonzero(reference)
    rgb = pixels[yy, xx, :3].astype(float)
    # Reject eyes, linework, green spill, and very dark areas. All colors used
    # for the repair therefore come from warm skin on this model's cheeks.
    warm = ((rgb[:, 0] > 1.06 * rgb[:, 1]) & (rgb[:, 1] > 1.12 * rgb[:, 2]) &
            (rgb[:, 0] > 80) & (rgb[:, 2] > 15))
    reference[yy[~warm], xx[~warm]] = False
    result, details = correct_green_chroma(pixels, target, protected, reference)
    hands = _mesh_footprint([source.meshes[index] for index in HAND_PIECES], source.image.size, 2)
    ears = _mesh_footprint([source.meshes[index] for index in TARGET_PIECES if index not in HAND_PIECES],
                           source.image.size, 2)
    # Ear ownership wins any overlap, so the earlier accepted ear repair is
    # preserved exactly. Other geometry always retains its original texels.
    safe, calibrated, hand_report = _calibrate_hand_skin(
        pixels, hands, protected | ears, reference, np.asarray(details["reference_linear_chroma"]))
    result[safe, :3] = calibrated
    details["changed_texels"] = int(np.count_nonzero(np.any(result != pixels, axis=-1)))
    details["remaining_strong_green_texels"] = int(np.count_nonzero(
        green_weight(result[target & ~protected, :3]) >= 1. - 1e-10))
    # Hand luminance is intentionally normalized and is reported separately.
    ear_safe = ears & ~protected
    details.pop("max_linear_luminance_error")
    details["ears_max_linear_luminance_error"] = float(np.max(np.abs(
        (_linear(result[ear_safe, :3]) - _linear(pixels[ear_safe, :3])) @ _LUMA), initial=0.))
    details["hand_calibration"] = hand_report
    report.update(details)
    report.update(applied=True, target_mesh_ids=sorted(TARGET_PIECES),
                  target_mesh_names=[TARGET_PIECES[index] for index in sorted(TARGET_PIECES)],
                  method="continuous hand skin calibration; original green-only ear correction retained",
                  filter_padding_texels=2, source_image_unchanged=True)
    return result, report
