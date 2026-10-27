#!/usr/bin/env python3
"""Verify actual Wolf eye words, emission, and protected bundled HD bytes."""
import argparse
import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile
import zipfile

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
FIXTURE = Path(__file__).with_name("eye_baseline_rgba5551.bin")
spec = importlib.util.spec_from_file_location("wolf_eye_builder", ROOT / "tools/wolf_blue_eyes/build.py")
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


def check_eye(rgb, luminous, original):
    yy, xx = np.mgrid[:64, :64]
    white = np.all(rgb >= 248, axis=-1)
    assert 8 <= white.sum() <= 24, "each iris requires a small visible white catchlight"
    assert np.all(luminous[white]), "white catchlight must bypass scene lighting"
    assert white[25, 21] and white[26, 21] and white[25, 22]
    assert 19 <= xx[white].min() <= xx[white].max() <= 23
    assert 23 <= yy[white].min() <= yy[white].max() <= 28
    pupil = (((xx - 26.0) / 8.0) ** 2 + ((yy - 32.5) / 8.5) ** 2) <= 1
    assert 0 < (white & pupil).sum() < 16, "only the catchlight may enter the protected pupil"
    assert np.array_equal(rgb[pupil & ~white], original[pupil & ~white])
    assert not np.any(luminous[pupil & ~white])
    blue = luminous & ~white
    assert 200 < blue.sum() < 400
    assert np.median(rgb[blue, 2]) >= 180 and np.median(rgb[blue, 1]) >= 105
    assert np.all(rgb[blue, 2] > rgb[blue, 0] + 80), "glow remains a blue iris"
    assert np.array_equal(rgb[~luminous], original[~luminous])
    return white, pupil


def run(tp_baseline=None):
    fixture = FIXTURE.read_bytes()
    assert len(fixture) == 16384
    assert hashlib.sha256(fixture).hexdigest() == "04775b0c702ecf6e750a291903612985a2ca18b0e2d92a1777caf288630b1812"
    words = np.frombuffer(fixture[:8192], "<u2").reshape(64, 64)
    original = builder.rgba5551(words)
    rgb, luminous = builder.eye_colors(original, "soft")
    # RGBA5551 is the actual native texture format, not an unquantized preview.
    packed = ((rgb[..., 0] >> 3) << 11) | ((rgb[..., 1] >> 3) << 6) | ((rgb[..., 2] >> 3) << 1) | 1
    packed[luminous] &= np.uint16(0xFFFE)
    decoded = builder.rgba5551(packed)
    white, pupil = check_eye(decoded, luminous, original)
    assert np.all(packed[white] == 0xFFFE)
    assert np.array_equal(packed[pupil & ~white], words[pupil & ~white])
    assert np.array_equal(packed[~luminous], words[~luminous])

    resources = [ROOT / game / "assets/custom/objects/forms/wolf_link/gWolfLinkHDData" for game in ("soh", "mm")]
    data = resources[0].read_bytes()
    assert resources[1].read_bytes() == data
    h = struct.unpack_from("<20I", data, 80)
    assert h[:6] == (8640, 2880, 46, 144, 1024, 512)
    atlas = np.frombuffer(data, "<u2", h[4] * h[5], 68 + h[16]).reshape(h[5], h[4])
    recovered = bytearray(data)
    for eye, (x, y) in enumerate(builder.EYE_TILES):
        original_words = np.frombuffer(fixture[eye * 8192:(eye + 1) * 8192], "<u2").reshape(64, 64)
        original = builder.rgba5551(original_words)
        rgb, luminous = builder.eye_colors(original, "soft")
        packed = ((rgb[..., 0] >> 3) << 11) | ((rgb[..., 1] >> 3) << 6) | ((rgb[..., 2] >> 3) << 1) | 1
        packed[luminous] &= np.uint16(0xFFFE)
        actual = atlas[y:y + 64, x:x + 64]
        assert np.array_equal(actual, packed), "bundled HD eyes must match the deterministic palette"
        check_eye(builder.rgba5551(actual), (actual & 1) == 0, original)
        for row in range(64):
            offset = 68 + h[16] + 2 * ((y + row) * h[4] + x)
            start = eye * 8192 + row * 128
            recovered[offset:offset + 128] = fixture[start:start + 128]
    assert hashlib.sha256(recovered).hexdigest() == builder.HD_SOURCE_SHA256, \
        "protected fur, geometry, chain, weights, rig, clips, audio or Blob envelope changed"
    assert builder.patch_hd_resource(bytes(recovered)) == data, "patch must reproduce both bundled resources"
    assert builder.patch_hd_resource(data) == data, "patch must be idempotent"
    assert builder.verify_hd_resource(data)["white_catchlight_texels_per_eye"] == [int(white.sum())] * 2
    malformed = bytearray(recovered)
    malformed[68 + h[6]] ^= 1
    try:
        builder.patch_hd_resource(bytes(malformed))
    except ValueError:
        pass
    else:
        raise AssertionError("unknown HD geometry must be rejected before writing")
    for game in ("soh", "mm"):
        standard = (ROOT / game / "assets/custom/objects/forms/wolf_link/gWolfLinkData").read_bytes()
        assert hashlib.sha256(standard).hexdigest() == "5047bd1f0a639db9ad50f87063cbd52303d8fe8c0056896d34ec1e6cfd5d99f2"

    chain_spec = importlib.util.spec_from_file_location("wolf_chain_builder", ROOT / "tools/wolf_blue_eyes/add_chain.py")
    chain = importlib.util.module_from_spec(chain_spec)
    chain_spec.loader.exec_module(chain)
    with tempfile.TemporaryDirectory(prefix="wolf-eye-input-") as tmp:
        pack = Path(tmp) / "input.o2r"
        with zipfile.ZipFile(pack, "w") as archive:
            archive.writestr(chain.RESOURCE, bytes(malformed))
        try:
            chain.read_soft_wolf(pack)
        except ValueError:
            pass
        else:
            raise AssertionError("chain builder must reject an unknown input payload")
        if tp_baseline:
            envelope, baseline = builder.read(tp_baseline)
            soft, soft_words, _, _ = builder.recolor_atlas(baseline)
            assert hashlib.sha256(soft).hexdigest() == chain.SOFT_GLOW_SHA
            old_soft = bytearray(soft)
            old_header = struct.unpack_from("<20I", soft, 12)
            for eye, (x, y) in enumerate(builder.EYE_TILES):
                assert np.array_equal(soft_words[y:y + 64, x:x + 64], atlas[y:y + 64, x:x + 64]), \
                    "original baseline and HD patch must produce canonical identical eye words"
                for row in range(64):
                    offset = old_header[16] + 2 * ((y + row) * old_header[4] + x)
                    start = eye * 8192 + row * 128
                    old_soft[offset:offset + 128] = fixture[start:start + 128]
            assert hashlib.sha256(old_soft).hexdigest() == chain.SOFT_SHA
            for payload in (soft, bytes(old_soft)):
                serialized = envelope + payload
                with zipfile.ZipFile(pack, "w") as archive:
                    archive.writestr(chain.RESOURCE, serialized)
                assert chain.read_soft_wolf(pack) == serialized
            print("PASS original guarded TP baseline reproduces corrected HD eye tiles; exact old/new chain inputs accepted")
    print("PASS deterministic white catchlights, blue full-bright irises, dark pupils and exact protected HD bytes in both hosts")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tp-baseline", type=Path)
    run(parser.parse_args().tp_baseline)
