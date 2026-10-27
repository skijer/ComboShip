#!/usr/bin/env python3
"""Verify native counter displays against room, keeper, each other and camera."""
import argparse
import ast
import hashlib
import itertools
import json
from pathlib import Path
import re
import sys
import zipfile

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/black_market"))
from counter import merchandise_report
from keeper_lean import room_faces
from native_preview import NativeKeeper
from presentation_preview import shelf_positions, triangle_box_intersects

HOSTS = ("mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp",
         "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp")


def overlaps(a, b):
    return bool(np.all(a[1] >= b[0]) and np.all(b[1] >= a[0]))


def intersections(triangles, box):
    low, high = box
    candidates = np.all(triangles.max(1) >= low, axis=1) & np.all(triangles.min(1) <= high, axis=1)
    return sum(triangle_box_intersects(face, low, high) for face in triangles[candidates])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pack", type=Path)
    parser.add_argument("--json-include", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    pack_hash = hashlib.sha256(args.pack.read_bytes()).hexdigest()
    shelves = np.array(shelf_positions())
    # A counter display must sit near the keeper rather than on the front side walls.
    assert abs(shelves[:, 0]).max() <= 90, "Stock is spread along the side walls rather than grouped at the counter"
    assert shelves[:, 2].min() >= 80 and shelves[:, 2].max() <= 125
    # Native shop banks use an angled plane: rows remain aligned, while the
    # outer column follows the bank depth. A frontal-only assertion missed the
    # mismatch between native shelf turning and the old wall-like fixture.
    sys.path.insert(0,str(ROOT/'tests/black_market'))
    from check_browsing_frame import camera_views
    cameras=camera_views()
    for bank,view in ((shelves[:4],cameras['right']),(shelves[4:],cameras['left'])):
        assert np.array_equal(bank[0,[0,2]],bank[1,[0,2]])
        assert np.array_equal(bank[2,[0,2]],bank[3,[0,2]])
        column=bank[2]-bank[0];column[1]=0;column/=np.linalg.norm(column)
        direction=np.array(view['at'])-view['eye'];direction[1]=0;direction/=np.linalg.norm(direction)
        assert abs(column@direction)<.001,'Shelf columns must face the native bank camera without an oblique wall view'
    for path in HOSTS:
        source = (ROOT / path).read_text()
        block = re.search(r"constexpr Vec3f kShelves\[8\] = \{(.*?)\};", source, re.S)
        native = [[float(n.strip().removesuffix("f")) for n in row.split(",")] for row in re.findall(r"\{([^{}]+)\}", block[1])]
        assert np.array_equal(native, shelves), "Both native games and preview must use the same physical display layout"

    with zipfile.ZipFile(args.pack) as archive:
        room = np.asarray(room_faces(archive), dtype=float)
        keeper = NativeKeeper(archive)
        posed = np.concatenate([keeper.position_triangles(frame) for frame in range(keeper.frame_count)])

    measured = merchandise_report(args.json_include)
    sample_boxes = []
    for item in measured["items"]:
        for kind in ("model", "native_x"):
            box = np.array(item[kind + "_swept_bounds"])
            assert intersections(room, box) == 0, (item["slug"], kind, "room intersection")
            assert intersections(posed, box) == 0, (item["slug"], kind, "keeper intersection")
        sample_boxes.append(np.array(item["native_x_swept_bounds"]))
    assert not [(i, j) for i, j in itertools.combinations(range(8), 2)
                if overlaps(sample_boxes[i], sample_boxes[j])], "Displayed models/seals must remain separated"

    # Also reserve the complete authored-shop fit envelope, including feather's
    # taller top, both scales/bob extrema and conservative native X inflation.
    # This is conditional on FrameFit; arbitrary native/Alt replacements are not covered.
    fit = (ROOT / "soh/soh/Enhancements/randomizer/NeiGiFrameFit.h").read_text()
    for contract in ("bottom = shop ? -22.f", "feather ? 62.f : 52.f", "shop ? 76.f"):
        assert contract in fit, "Inspect changed shop-fit limits before reusing the reserved envelope"
    radius, half_height = .32 * 38, .32 * (62 + 22) / 2
    def extension(half, floor, extra):
        return max((floor + 1.4) / .875, (max(half, floor) + 1.4) / .875 - half) + extra + 1 / 64
    ex, ey, ez = extension(radius, 2.4, .85), extension(half_height, 3.2, .85), extension(radius, 2.4, .45)
    relative = np.array([[-radius-ex, -2-.32*22-ey, -radius-ez],
                         [radius+ex, 2+.32*62+ey, radius+ez]])
    reserved = [relative + position for position in shelves]
    for slot, box in enumerate(reserved):
        assert intersections(room, box) == 0, (slot, "reserved envelope intersects room")
        assert intersections(posed, box) == 0, (slot, "reserved envelope intersects keeper")
    assert not [(i, j) for i, j in itertools.combinations(range(8), 2)
                if overlaps(reserved[i], reserved[j])], "Full fit envelopes must not overlap, even at selected scale"

    constants = {}
    for node in ast.parse((ROOT / "tools/black_market/preview.py").read_text()).body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id in ("OVERVIEW_VIEW", "OVERVIEW_FOV"):
                    constants[target.id] = ast.literal_eval(node.value)
    eye, target = np.array(constants["OVERVIEW_VIEW"], float)
    forward = target-eye; forward /= np.linalg.norm(forward)
    right = np.cross(forward, [0, 1, 0]); right /= np.linalg.norm(right)
    up = np.cross(right, forward)
    half = np.tan(np.deg2rad(constants["OVERVIEW_FOV"]) / 2)
    for slot, box in enumerate(reserved):
        for point in itertools.product(*zip(*box)):
            offset = np.array(point)-eye; depth = offset @ forward
            assert depth > 0
            x = 525 + (offset @ right) / depth / half * 380
            y = 380 - (offset @ up) / depth / half * 380
            assert 16 < x < 1034 and 83 < y < 628, (slot, "offer clipped by preview frame/labels", x, y)

    result = {"pack_sha256": pack_hash, "shelves": shelves.tolist(), "sample_merchandise": measured,
              "fit_policy_relative_envelope": relative.tolist(), "reserved_slot_envelopes": [b.tolist() for b in reserved],
              "keeper_frames_checked": keeper.frame_count, "room_triangles": len(room),
              "model_and_native_x_room_keeper_clear": True, "offer_envelopes_separated": True,
              "all_offers_inside_single_camera": True, "runtime_tested": False,
              "scope": "Eight actual retained samples plus the conditional authored FrameFit envelope; native/Alt replacements, GI effects, camera/talk behavior and gameplay untested."}
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(result, indent=2) + "\n")
    assert hashlib.sha256(args.pack.read_bytes()).hexdigest() == pack_hash
    print("PASS counter display: matching native positions; eight actual model/X sweeps and complete authored-fit envelopes clear room, all32 keeper poses and each other; one-camera framing; pack unchanged")


if __name__ == "__main__":
    main()
