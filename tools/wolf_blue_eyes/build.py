#!/usr/bin/env python3
"""Palette-only Wolf eye POC. Geometry, rig, clips and audio remain exact.

Texture alpha is an inverse emission mask for the matching native material
patch. Existing opaque Wolf materials still draw these texels, with lighting.
No image generation, reskinning, rasterized mesh or animation export is used.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import numpy as np
from PIL import Image

RESOURCE = "objects/forms/wolf_link/gWolfLinkData"
BASE_SHA256 = "d6447975f22bcb1e62a6fac6009f5aa7ab7f78f65f499f19c9b4b7ce3216416e"
HD_SOURCE_SHA256 = "132cbb2b593a13c557772acd007785dd7c5924ac22e2c1ba00de22c689930c65"
HD_RESULT_SHA256 = "7705dc56447b571a0d13b019fb4461ac5147dcaaf441db16d24dfe9a990f451c"
HD_PROTECTED_SHA256 = "52aff16711c94ba30830a7790fc4476ae7e8088287463f9df5f02aa71a42f88d"
EYE_TILES = ((532, 4), (612, 4))


def read(path):
    with zipfile.ZipFile(path) as archive:
        resource = archive.read(RESOURCE)
        if archive.read("alt/" + RESOURCE) != resource:
            raise ValueError("baseline normal/Alt payloads differ")
    payload = resource[68:]
    if resource[4:8] != b"BLBO" or struct.unpack_from("<I", resource, 64)[0] != len(payload):
        raise ValueError("invalid native Blob envelope")
    if hashlib.sha256(payload).hexdigest() != BASE_SHA256:
        raise ValueError("expected the recovered TP POC1 payload; do not recolor an unknown atlas")
    return resource[:68], payload


def rgba5551(words):
    return np.stack([(words >> 11) & 31, (words >> 6) & 31, (words >> 1) & 31], axis=-1).astype(float) * 255 / 31


def eye_colors(old, style="soft"):
    yy, xx = np.mgrid[:64, :64]
    # Select the original blue iris/pupil by color inside its existing eye tile.
    # The baked fur, white sclera and dark eyelid pixels are protected.
    iris = (old[:, :, 2] - old[:, :, 0] >= 8) & (old[:, :, 2] >= old[:, :, 1])
    iris &= (xx >= 9) & (xx <= 44) & (yy >= 19) & (yy <= 48)
    radius = np.sqrt(((xx - 26.5) / 15.5) ** 2 + ((yy - 33.5) / 14.0) ** 2)
    # A visible reflection needs actual white texels. The previous soft blue
    # glint peaked inside the excluded pupil and never reached white. Permit
    # this small reflection alone to overlap the upper-left pupil edge.
    catchlight = iris & ((((xx - 21.0) / 2.4) ** 2 + ((yy - 25.5) / 1.9) ** 2) <= 1)
    if style == "soft":
        # Keep the wide pupil and dark outer iris, with a readable blue crescent
        # that the matching native alpha-mask material draws full bright.
        pupil = (((xx - 26.0) / 8.0) ** 2 + ((yy - 32.5) / 8.5) ** 2) <= 1
        luminous = iris & ~pupil & (radius < 0.87)
        inner = np.exp(-((radius - 0.65) / 0.23) ** 2)
        blue = np.stack([32 + 16 * inner, 112 + 48 * inner, 208 + 40 * inner], axis=-1)
        result = old.copy()
        result[luminous] = blue[luminous]
        result[catchlight] = 255
        luminous |= catchlight
        return np.rint(result).astype(np.uint16), luminous
    pupil = (((xx - 26.0) / 5.4) ** 2 + ((yy - 32.5) / 6.0) ** 2) <= 1
    luminous = iris & ~pupil
    inner = np.exp(-((radius - 0.43) / 0.3) ** 2)
    rim = np.exp(-((radius - 0.8) / 0.23) ** 2)
    glint = np.exp(-(((xx - 20) / 2.8) ** 2 + ((yy - 27) / 2.7) ** 2))
    blue = np.empty_like(old)
    blue[:, :, 0] = 24 + 40 * inner + 12 * rim + 125 * glint
    blue[:, :, 1] = 128 + 56 * inner + 14 * rim + 64 * glint
    blue[:, :, 2] = 248 + 7 * np.maximum(inner, glint)
    result = old.copy()
    result[luminous] = np.minimum(blue[luminous], 255)
    result[catchlight] = 255
    luminous |= catchlight
    # Keep dark, recognizable pupils except for the small white reflection.
    return np.rint(result).astype(np.uint16), luminous


def recolor_atlas(baseline, style="soft"):
    header = struct.unpack_from("<20I", baseline, 12)
    if header[4:6] != (1024, 512):
        raise ValueError("unexpected atlas layout")
    words = np.frombuffer(baseline, "<u2", header[4] * header[5], header[16]).reshape(header[5], header[4]).copy()
    original = words.copy()
    masks = np.zeros(words.shape, bool)
    counts = []
    for x, y in EYE_TILES:
        old = rgba5551(words[y:y + 64, x:x + 64])
        rgb, luminous = eye_colors(old, style)
        changed = ((rgb[:, :, 0] >> 3) << 11) | ((rgb[:, :, 1] >> 3) << 6) | ((rgb[:, :, 2] >> 3) << 1) | 1
        changed[luminous] &= np.uint16(0xFFFE)
        tile = words[y:y + 64, x:x + 64]
        tile[luminous] = changed[luminous]
        masks[y:y + 64, x:x + 64] = luminous
        counts.append(int(luminous.sum()))
    assert all(100 < count < 900 for count in counts)
    assert np.array_equal(words[~masks], original[~masks])
    assert np.all((words[~masks] & 1) == 1)
    candidate = bytearray(baseline)
    candidate[header[16]:header[16] + header[17]] = words.astype("<u2").tobytes()
    # All nontexture bytes are identical, including the 144 clips and PCM sounds.
    assert candidate[:header[16]] == baseline[:header[16]]
    assert candidate[header[16] + header[17]:] == baseline[header[16] + header[17]:]
    return bytes(candidate), words, masks, counts


def protected_hd_sha256(resource):
    header = struct.unpack_from("<20I", resource, 80)
    protected = bytearray(resource)
    for x, y in EYE_TILES:
        for row in range(y, y + 64):
            offset = 68 + header[16] + 2 * (row * header[4] + x)
            protected[offset:offset + 128] = bytes(128)
    return hashlib.sha256(protected).hexdigest()


def patch_hd_resource(resource):
    digest = hashlib.sha256(resource).hexdigest()
    if digest == HD_RESULT_SHA256:
        return resource
    if digest != HD_SOURCE_SHA256:
        raise ValueError("expected the bundled chain-equipped HD Wolf; unknown resources are protected")
    candidate, _, _, _ = recolor_atlas(resource[68:])
    result = resource[:68] + candidate
    assert protected_hd_sha256(result) == HD_PROTECTED_SHA256
    assert hashlib.sha256(result).hexdigest() == HD_RESULT_SHA256
    return result


def verify_hd_resource(resource):
    if hashlib.sha256(resource).hexdigest() != HD_RESULT_SHA256:
        raise ValueError("HD eye resource does not match the deterministic corrected palette")
    assert protected_hd_sha256(resource) == HD_PROTECTED_SHA256
    payload = resource[68:]
    header = struct.unpack_from("<20I", payload, 12)
    assert header[:6] == (8640, 2880, 46, 144, 1024, 512)
    words = np.frombuffer(payload, "<u2", header[4] * header[5], header[16]).reshape(header[5], header[4])
    white, luminous = [], []
    for x, y in EYE_TILES:
        tile = words[y:y + 64, x:x + 64]
        white.append(int((tile == 0xFFFE).sum()))
        luminous.append(int(((tile & 1) == 0).sum()))
    return {"canonical_eye_source_payload_sha256": BASE_SHA256,
            "source_resource_sha256": HD_SOURCE_SHA256,
            "candidate_resource_sha256": HD_RESULT_SHA256,
            "protected_bytes_sha256": HD_PROTECTED_SHA256,
            "white_catchlight_texels_per_eye": white,
            "full_bright_texels_per_eye": luminous,
            "vertices": header[0], "bones": header[2], "animation_clips": header[3],
            "non_eye_bytes": "identical", "runtime_status": "untested"}


def build(source, output, style="soft"):
    envelope, baseline = read(source)
    candidate, words, masks, counts = recolor_atlas(baseline, style)
    header = struct.unpack_from("<20I", baseline, 12)
    output.mkdir(parents=True, exist_ok=True)
    (output / "wolf_link.bin").write_bytes(candidate)
    serialized = envelope + candidate
    pack = output / ("zz_Wolf_Link_Blue_Eyes_POC2.o2r" if style == "soft" else "zz_Wolf_Link_Blue_Eyes_POC1.o2r")
    with zipfile.ZipFile(pack, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name in (RESOURCE, "alt/" + RESOURCE):
            info = zipfile.ZipInfo(name, (2026, 10, 9, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, serialized, compresslevel=9)
    with zipfile.ZipFile(pack) as archive:
        assert archive.testzip() is None
        assert archive.namelist() == [RESOURCE, "alt/" + RESOURCE]
        assert archive.read(RESOURCE) == serialized == archive.read("alt/" + RESOURCE)
    Image.fromarray(np.rint(rgba5551(words)).astype(np.uint8)).save(output / "atlas_rgb.png")
    Image.fromarray((masks * 255).astype(np.uint8)).save(output / "emission_mask.png")
    report = {
        "baseline_payload_sha256": hashlib.sha256(baseline).hexdigest(),
        "candidate_payload_sha256": hashlib.sha256(candidate).hexdigest(),
        "candidate_archive_sha256": hashlib.sha256(pack.read_bytes()).hexdigest(),
        "changed_texels_per_eye": counts,
        "eye_style": style,
        "atlas_dimensions": [1024, 512],
        "geometry_weights_uvs_normals_rig_animation_audio_bytes": "identical",
        "unmasked_texture_words": "identical",
        "vertices": header[0], "triangles": header[1], "bones": header[2], "animation_clips": header[3],
        "normal_alt_payloads": "identical",
        "glow_requires_matching_native_material_patch": True,
        "current_binary_result": "blue irises and white catchlights in the inverse full-bright mask",
        "runtime_status": "untested"
    }
    (output / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    sources = parser.add_mutually_exclusive_group(required=True)
    sources.add_argument("--source", type=Path)
    sources.add_argument("--hd-resource", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--verify", action="store_true", help="verify a corrected HD resource without writing")
    parser.add_argument("--style", choices=("soft", "bright"), default="soft")
    args = parser.parse_args()
    if args.hd_resource:
        if args.style != "soft":
            parser.error("the bundled HD resource uses the soft eye profile")
        resource = args.hd_resource.read_bytes()
        if not args.verify:
            if not args.output:
                parser.error("--output is required when writing")
            resource = patch_hd_resource(resource)
            report = verify_hd_resource(resource)
            args.output.mkdir(parents=True, exist_ok=True)
            (args.output / "gWolfLinkHDData").write_bytes(resource)
            (args.output / "verification.json").write_text(json.dumps(report, indent=2) + "\n")
        print(json.dumps(verify_hd_resource(resource)))
    else:
        if args.verify or not args.output:
            parser.error("--source requires --output; --verify is for --hd-resource")
        build(args.source, args.output, args.style)
