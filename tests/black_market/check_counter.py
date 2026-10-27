#!/usr/bin/env python3
"""Asset regression for the isolated raised Granny counter candidate."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/black_market"))
from keeper_lean import room_faces

BASELINE_SHA256 = "761fa811149f860b6a174901bdda1ad7fdf5610a0064f0e3efe5927853bb1352"
VERTICES = "custom/skiejer_market/geometry/mahouya_room_0DL_006548_vtx_2"
COLLISION = "scenes/nonmq/skiejer_market/collision"
MANIFEST = "custom/skiejer_market/manifest.json"
DRAW = "custom/skiejer_market/geometry/mahouya_room_0DL_006548_tri_2"


def collision_faces(data):
    count, = struct.unpack_from("<I", data, 76)
    vertices = [struct.unpack_from("<3h", data, 80 + index * 6) for index in range(count)]
    at = 80 + count * 6
    polygons, = struct.unpack_from("<I", data, at)
    rows = [struct.unpack_from("<HHHHhhhh", data, at + 4 + index * 16) for index in range(polygons)]
    return vertices, rows, [tuple(vertices[index & 8191] for index in row[1:4]) for row in rows], data[at + 4 + polygons * 16:]


def check(source, output, report_path):
    before = hashlib.sha256(source.read_bytes()).hexdigest()
    assert before == BASELINE_SHA256
    with zipfile.ZipFile(source) as pack:
        original_vertices = list(ET.fromstring(pack.read(VERTICES)))
        assert sum(vertex.get("Y") == "30" and -160 <= int(vertex.get("X")) <= 58 and
                   93 <= int(vertex.get("Z")) <= 123 for vertex in original_vertices) == 32
        original_collision = collision_faces(pack.read(COLLISION))
    command = [sys.executable, str(ROOT / "tools/black_market/counter.py"),
               "--source", str(source), "--output", str(output), "--report", str(report_path)]
    include = ROOT.parent / "verification-deps/nlohmann_json/include"
    if include.exists():
        command += ["--json-include", str(include)]
    result = subprocess.run(command, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    report = json.loads(report_path.read_text())
    with zipfile.ZipFile(source) as baseline, zipfile.ZipFile(output) as candidate:
        assert baseline.namelist() == candidate.namelist()
        changed = {name for name in baseline.namelist() if baseline.read(name) != candidate.read(name)}
        assert changed == {VERTICES, DRAW, COLLISION, MANIFEST}, f"Unexpected resource mutation: {changed}"
        vertices = list(ET.fromstring(candidate.read(VERTICES)))
        assert len(vertices) == len(original_vertices) + 8
        changed_indices = []
        for index, (old, new) in enumerate(zip(original_vertices, vertices)):
            a, b = dict(old.attrib), dict(new.attrib)
            old_y, new_y = int(a.pop("Y")), int(b.pop("Y"))
            assert a == b, f"Counter X/Z/UV/color changed at vertex {index}"
            if old_y != new_y:
                changed_indices.append(index)
                assert old_y in (26, 30) and new_y == old_y + 11, "Only upper counter geometry may rise"
                assert int(a["X"]) >= -51, "Keep the book-side counter unchanged; interpolate existing faces to the raised main counter"
            else:
                assert new_y == old_y
        assert len(changed_indices) == 46, "Expected 46 audited main-counter vertices"
        for index in (661, 726, 727, 730, 731, 734, 735):
            assert vertices[index].attrib == original_vertices[index].attrib, "Keep low left-face seam vertices unchanged"
        old_v, old_p, old_faces, old_tail = original_collision
        new_v, new_p, new_faces, new_tail = collision_faces(candidate.read(COLLISION))
        assert new_v[:len(old_v)] == old_v, "Existing collision vertices must remain unchanged"
        assert len(old_p) == 7819 and len(new_p) == 7823
        assert old_tail == new_tail, "Collision surfaces/cameras/water metadata changed"
        cap_faces = [i for i, face in enumerate(new_faces) if all(point[0] == -51 for point in face) and
                     (({point[1] for point in face} == {30,41} and {point[2] for point in face} == {93,123}) or
                      ({point[1] for point in face} == {26,30} and {point[2] for point in face} == {120,123}))]
        assert len(cap_faces) == 4 and cap_faces == list(range(cap_faces[0], cap_faces[0] + 4))
        existing_rows = [row for i, row in enumerate(new_p) if i not in cap_faces]
        assert sum(a != b for a, b in zip(old_p, existing_rows)) == 34, "Only 34 main counter records may change"
        assert all(new_p[index][4:] == (-32767,0,0,-51) for index in cap_faces[:2]), "Upper step must face -X"
        assert all(new_p[index][4:] == (32767,0,0,51) for index in cap_faces[2:]), "Retained lower lip end must face +X"
        drawn = np.asarray(room_faces(candidate))
        nondegenerate = np.linalg.norm(np.cross(drawn[:, 1] - drawn[:, 0], drawn[:, 2] - drawn[:, 0]), axis=1) >= .1
        expected = [tuple(tuple(int(value) for value in point) for point in triangle) for triangle in drawn[nondegenerate]]
        assert new_faces == expected, "Collision must match all reachable room triangles in native order"
        points = np.asarray(new_faces, dtype=float)
        normals = np.cross(points[:, 1] - points[:, 0], points[:, 2] - points[:, 0])
        normals /= np.linalg.norm(normals, axis=1)[:, None]
        expected_normals = np.clip(np.rint(normals * 32767), -32767, 32767).astype(int)
        expected_distance = np.rint(-(normals * points[:, 0]).sum(1)).astype(int)
        assert np.array_equal(np.asarray(new_p)[:, 4:7], expected_normals), "Native collision normals differ from raised geometry"
        assert np.array_equal(np.asarray(new_p)[:, 7], expected_distance), "Native collision planes differ from raised geometry"
        assert Counter(old_faces).total() + 4 == Counter(new_faces).total()
        manifest = json.loads(candidate.read(MANIFEST))
        assert manifest["counter"]["top_y"] == 41 and manifest["keeper"] == json.loads(baseline.read(MANIFEST))["keeper"]
        assert manifest["collision"]["vertices"] == len(new_v)
    assert hashlib.sha256(source.read_bytes()).hexdigest() == before
    assert report["baseline_sha256"] == before and report["changed_resources"] == sorted(changed)
    assert report["preserved_resources"] == 188
    assert report["keeper"]["frames_checked"] == 32 and report["keeper"]["minimum_counter_clearance"] > .16
    assert not report["keeper"]["continuous_flat_contact_proven"], "Do not claim a static plane tracks both animated palms"
    assert report["counter"]["retained_scenery"]["potential_intersections"] == 0
    assert all(item["model_clearance_proven"] and item["native_x_clearance_proven"] for item in report["merchandise"]["items"])
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="market-counter-check-") as folder:
        folder = Path(folder)
        report = check(args.source, folder / "raised.o2r", folder / "report.json")
        second = check(args.source, folder / "raised-again.o2r", folder / "report-again.json")
        assert (folder / "raised.o2r").read_bytes() == (folder / "raised-again.o2r").read_bytes()
        assert report["candidate_sha256"] == second["candidate_sha256"]
    print("PASS raised counter: 46 Y-only changes + two seam quads; 34 changed/4 added collision faces; retained props clear; 188 payloads preserved; "
          "all 32 keeper frames clear; 8 actual sample sweeps and native X envelopes clear; deterministic candidate")


if __name__ == "__main__":
    main()
