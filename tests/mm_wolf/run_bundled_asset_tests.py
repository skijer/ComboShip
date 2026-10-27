#!/usr/bin/env python3
"""Verify bundled Wolf resources, preserved animation/audio and chain UV bounds."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

ROOT = Path(__file__).resolve().parents[2]
PREFIX = "objects/forms/wolf_link/"
STANDARD, HD = "gWolfLinkData", "gWolfLinkHDData"


def read_resource(data):
    assert data[4:8] == b"BLBO" and struct.unpack_from("<I", data, 8)[0] == 0
    assert struct.unpack_from("<I", data, 64)[0] == len(data) - 68
    blob = data[68:]
    assert blob[:8] == b"NEIWOLF1" and struct.unpack_from("<I", blob, 8)[0] == 2
    h = struct.unpack_from("<20I", blob, 12)
    assert h[18] == len(blob)
    return blob, h


def verify(soh_archive=None, mm_archive=None, tp_baseline=None):
    resources = {}
    for name in (STANDARD, HD):
        files = [ROOT / game / "assets/custom" / PREFIX / name for game in ("soh", "mm")]
        data = files[0].read_bytes()
        assert files[1].read_bytes() == data, "both native owners must supply identical resources"
        resources[name] = data
    standard, sh = read_resource(resources[STANDARD])
    hd, h = read_resource(resources[HD])
    assert sh[:6] == (8112, 2704, 40, 144, 256, 256)
    assert h[:6] == (8640, 2880, 46, 144, 1024, 512)
    assert struct.unpack_from("<6h", hd, h[8] + 40 * 2) == (17,) * 6
    for index, stride in ((8, 2), (9, 64), (10, 12)):
        assert hd[h[index]:h[index] + 40 * stride] == standard[sh[index]:sh[index] + 40 * stride]
    assert hd[h[12]:h[12] + h[13]] == standard[sh[12]:sh[12] + sh[13]]
    assert hd[h[19]:] == standard[sh[19]:], "Wolf PCM audio changed"
    frames = 0
    cuff_head = None
    for clip in range(144):
        old = struct.unpack_from("<IHHfI", standard, sh[11] + clip * 16)
        new = struct.unpack_from("<IHHfI", hd, h[11] + clip * 16)
        assert old[1] == new[1] and old[3] == new[3]
        assert old[0] - sh[12] == new[0] - h[12] and (old[2], new[2]) == (40, 46)
        for frame in range(old[1]):
            start = new[4] + frame * 46 * 36
            original = old[4] + frame * 40 * 36
            assert hd[start:start + 40 * 36] == standard[original:original + 40 * 36]
            head = hd[start + 40 * 36:start + 40 * 36 + 12]
            if cuff_head is None:
                cuff_head = head
            assert head == cuff_head, "chain head can drift during native interpolation"
        frames += old[1]
    assert frames == 5047
    for vertex in range(7776, 8640):
        weight = hd[h[7] + vertex * 8:h[7] + (vertex + 1) * 8]
        assert weight == bytes([40 + (vertex - 7776) // 144, 0, 0, 0, 255, 0, 0, 0])
        values = struct.unpack_from("<3f3bhhB", hd, h[6] + vertex * 20)
        assert values[-1] == 255
        assert 820 * 32 <= values[-3] <= 852 * 32 and 4 * 32 <= values[-2] <= 36 * 32, \
            "authored chain UV repetition escaped its atlas tile"
    if tp_baseline:
        original = Path(tp_baseline).read_bytes()
        th = struct.unpack_from("<20I", original, 12)
        assert hd[h[6]:h[6] + 7776 * 20] == original[th[6]:th[6] + 7776 * 20]
        assert hd[h[7]:h[7] + 7776 * 8] == original[th[7]:th[7] + 7776 * 8]
        protected_changes, emission = 0, 0
        for y in range(512):
            for x in range(1024):
                word = struct.unpack_from("<H", hd, h[16] + 2 * (y * 1024 + x))[0]
                old = struct.unpack_from("<H", original, th[16] + 2 * (y * 1024 + x))[0]
                eye = 4 <= y < 68 and (532 <= x < 596 or 612 <= x < 676)
                chain = 0 <= y < 40 and 816 <= x < 856
                assert eye or chain or word == old, "protected fur texel changed"
                if not word & 1:
                    assert eye, "emission escaped the irises"
                    emission += 1
                protected_changes += word != old and not chain
        assert emission == protected_changes == 536
    for game, path in (("soh", soh_archive), ("mm", mm_archive)):
        if path:
            with zipfile.ZipFile(path) as archive:
                assert archive.testzip() is None
                for name, data in resources.items():
                    assert archive.read(PREFIX + name) == data, f"{game} archive dropped or changed the Wolf resource"
                for key in ("objects/object_nei_shadow_crystal/gNeiShadowCrystalDL",
                            "textures/icon_item_custom/gItemIconShadowCrystalTex",
                            "textures/item_name_custom/gShadowCrystalNameTex"):
                    assert key in archive.namelist(), f"{game} requires a separate Shadow Crystal pack"
    report = {"standard_vertices": sh[0], "hd_vertices": h[0], "original_wolf_bones": 40,
              "chain_bones": 6, "clips": 144, "original_animation_frames_byte_preserved": frames,
              "original_rig_pcm": "byte-preserved", "chain_uvs": "bounded in their authored atlas tile",
              "generated_archives": bool(soh_archive and mm_archive),
              "resources": {name: {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
                            for name, data in resources.items()}}
    print(json.dumps(report))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--soh-archive")
    parser.add_argument("--mm-archive")
    parser.add_argument("--tp-baseline")
    args = parser.parse_args()
    verify(args.soh_archive, args.mm_archive, args.tp_baseline)
