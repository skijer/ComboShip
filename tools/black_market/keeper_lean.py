#!/usr/bin/env python3
"""Patch only the exact POC11 keeper placement; audit the complete native lean."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
BASELINE_SHA256 = "d439e88073d6f8d5230d4ccff1aa744824be6b60975ac8469e1ba9fde9c346c2"
ROOM = "scenes/nonmq/skiejer_market/skiejer_market_room_0"
MANIFEST = "custom/skiejer_market/manifest.json"


def keeper_contract():
    """Read the exact C++ contract used by both native keeper controllers."""
    source = (ROOT / "combo/BlackMarketKeeper.h").read_text()
    animation = re.search(r'alignas\(2\) inline constexpr char kAnimation\[\] = "(__OTR__[^"\n]+)";', source)
    assert animation, "Inspect the changed native keeper animation contract"
    values = {}
    for name in ("X", "Y", "Z", "Scale"):
        match = re.search(r"inline constexpr float k" + name + r" = (-?\d+(?:\.\d+)?)f;", source)
        assert match, f"Inspect the changed native keeper {name} contract"
        values[name] = float(match[1])
    yaw = re.search(r"inline constexpr int16_t kYaw = (-?\d+);", source)
    assert yaw and -32768 <= int(yaw[1]) <= 32767
    return {"animation": animation[1].removeprefix("__OTR__"),
            "position": [values[name] for name in ("X", "Y", "Z")],
            "yaw": int(yaw[1]), "scale": values["Scale"]}


def keeper_actor_offset(data):
    """Locate the real MM room carrier without rewriting any scene framing."""
    assert struct.unpack_from("<I", data, 4)[0] == 0x4F524F4D, "Expected native room resource"
    at = 68
    for _ in range(struct.unpack_from("<I", data, 64)[0]):
        command, = struct.unpack_from("<I", data, at)
        at += 4
        if command == 0x08:
            at += 6
        elif command == 0x0B:
            count, = struct.unpack_from("<I", data, at)
            at += 4 + count * 2
        elif command == 0x01:
            count, = struct.unpack_from("<I", data, at)
            at += 4
            keepers = [at + index * 16 for index in range(count)
                       if struct.unpack_from("<H", data, at + index * 16)[0] == 0x1C1]
            assert len(keepers) == 1, "Expected exactly one native MM keeper carrier"
            return keepers[0]
        else:
            raise ValueError(f"Keeper actor list not found before room command {command:02x}")
    raise ValueError("Keeper actor list is missing")


def keeper_placement(data):
    row = struct.unpack_from("<HhhhhhhH", data, keeper_actor_offset(data))
    return {"position": list(row[1:4]), "yaw": row[5], "scale": keeper_contract()["scale"]}


def room_faces(archive):
    """Read only the room's reachable XML draw graph, including native flex-free repairs."""
    cache, faces, active = {}, [], set()

    def visit(name):
        assert name not in active, "Recursive room display list"
        active.add(name)
        for command in ET.fromstring(archive.read(name)):
            if command.tag == "CallDisplayList":
                visit(command.get("Path"))
            elif command.tag == "LoadVertices":
                vertices = list(ET.fromstring(archive.read(command.get("Path"))))
                start, offset, count = (int(command.get(key)) for key in ("VertexBufferIndex", "VertexOffset", "Count"))
                for index, vertex in enumerate(vertices[offset:offset + count]):
                    cache[start + index] = tuple(float(vertex.get(key)) for key in ("X", "Y", "Z"))
            elif command.tag in ("Triangle1", "Triangles2"):
                for face in range(2 if command.tag == "Triangles2" else 1):
                    faces.append([cache[int(command.get(f"V{face}{index}"))] for index in range(3)])
            elif command.tag.startswith("Triangle"):
                raise ValueError(f"Unsupported native room triangle command {command.tag}")
        active.remove(name)

    visit("custom/skiejer_market/room")
    return faces


def geometry_report(archive):
    """Prove grounding and separation from the actual counter for all native frames.

    Every triangle is either above the entire counter box or behind its staff
    plane. This conservative separation also excludes triangle interiors; a
    vertex-only test would not prove that. Native integer frame stepping and
    decoded CPU floating point are used, not a claim of configured gameplay.
    """
    import numpy as np
    from native_preview import NativeKeeper

    contract = keeper_contract()
    assert keeper_placement(archive.read(ROOM)) == {key: contract[key] for key in ("position", "yaw", "scale")}, \
        "Actual native room keeper placement differs from the shared contract"
    keeper = NativeKeeper(archive)
    faces = np.asarray(room_faces(archive))
    low, high = faces.min(1), faces.max(1)
    horizontal = np.ptp(faces[:, :, 1], axis=1) < 1e-6
    top = faces[horizontal & (low[:, 1] == 30) & (low[:, 0] >= -51) &
                (high[:, 0] <= 30) & (low[:, 2] >= 93) & (high[:, 2] <= 123)]
    assert top.shape == (7, 3, 3), "The audited Granny counter top changed"
    counter_minimum = top.min((0, 1))
    counter_minimum[1] = 0
    counter_maximum = top.max((0, 1))
    assert np.array_equal(counter_minimum, [-51, 0, 93])
    assert np.array_equal(counter_maximum, [30, 30, 123])
    staff_before_counter = contract['position'][2] < counter_minimum[2]
    floor_y = 0.0 if staff_before_counter else 1.0
    floor = faces[horizontal & (low[:, 1] == floor_y) & (low[:, 0] < 30) &
                  (high[:, 0] > -30) & (low[:, 2] < contract['position'][2] + 15) &
                  (high[:, 2] > contract['position'][2] - 15)][:, :, (0, 2)]
    assert len(floor), "No staff-side floor under the keeper"
    edge_a, edge_b = floor[:, 1] - floor[:, 0], floor[:, 2] - floor[:, 0]
    determinant = edge_a[:, 0] * edge_b[:, 1] - edge_a[:, 1] * edge_b[:, 0]
    assert np.all(np.abs(determinant) > 1e-6), "Degenerate rear floor triangle"
    minimum, maximum = np.full(3, np.inf), np.full(3, -np.inf)
    counter_clearance, low_points_checked = float("inf"), 0
    triangle_count = None
    for frame in range(keeper.frame_count):
        triangles = keeper.position_triangles(frame, contract["position"], contract["yaw"], contract["scale"])
        assert np.isfinite(triangles).all()
        points = triangles.reshape(-1, 3)
        minimum, maximum = np.minimum(minimum, points.min(0)), np.maximum(maximum, points.max(0))
        assert points[:, 1].min() >= floor_y, f"Keeper penetrates the rear floor at frame {frame}"
        triangle_low, triangle_high = triangles.min(1), triangles.max(1)
        below_top = (triangle_low[:, 1] <= counter_maximum[1]) & (triangle_high[:, 1] >= counter_minimum[1])
        clearance = (float(counter_minimum[2] - triangle_high[below_top, 2].max()) if staff_before_counter else
                     float(triangle_low[below_top, 2].min() - counter_maximum[2]))
        assert clearance > 0, f"Keeper triangle overlaps the counter volume at frame {frame}"
        counter_clearance = min(counter_clearance, clearance)
        low_points = np.unique(points[points[:, 1] <= floor_y + 2][:, (0, 2)], axis=0)
        assert len(low_points), f"No sole points near the floor at frame {frame}"
        delta = low_points[:, None, :] - floor[None, :, 0, :]
        u = (delta[:, :, 0] * edge_b[:, 1] - delta[:, :, 1] * edge_b[:, 0]) / determinant
        v = (edge_a[:, 0] * delta[:, :, 1] - edge_a[:, 1] * delta[:, :, 0]) / determinant
        supported = ((u >= -1e-6) & (v >= -1e-6) & (u + v <= 1.000001)).any(1)
        assert supported.all(), f"Keeper sole projects outside the actual rear floor at frame {frame}"
        low_points_checked += len(low_points)
        triangle_count = len(triangles)
    return {"animation": keeper.animation_path, "frames_checked": keeper.frame_count,
            "triangles_per_frame": triangle_count, "world_bounds": [minimum.tolist(), maximum.tolist()],
            "floor_y": floor_y, "floor_clearance": float(minimum[1] - floor_y),
            "counter_bounds": [counter_minimum.tolist(), counter_maximum.tolist()],
            "counter_clearance": counter_clearance, "low_vertices_checked": low_points_checked,
            "all_low_vertices_supported": True,
            "method": "All native integer frames; every triangle above or behind the complete counter box; sole XZ points inside actual floor triangles.",
            "status": "Offline geometry verified; configured gameplay, Alt parity and user acceptance remain untested."}


def patch(source, output, report_path):
    source, output, report_path = Path(source), Path(output), Path(report_path)
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    if source_hash != BASELINE_SHA256:
        raise ValueError(f"Expected exact POC11 baseline SHA256 {BASELINE_SHA256}, got {source_hash}")
    if len({source.resolve(), output.resolve(), report_path.resolve()}) != 3:
        raise ValueError("Source, candidate output and report paths must be distinct")
    if output.exists() or report_path.exists():
        raise FileExistsError("Refusing to overwrite an existing keeper candidate/report")
    contract = keeper_contract()
    assert all(value == round(value) for value in contract["position"]), "Native room actors require integer positions"
    with zipfile.ZipFile(source) as baseline:
        names = baseline.namelist()
        assert len(names) == len(set(names)), "Duplicate baseline ZIP entries"
        resources = {name: baseline.read(name) for name in names}
        assert keeper_placement(resources[ROOM]) == {"position": [0, 0, 155], "yaw": -32768, "scale": .01}
        room = bytearray(resources[ROOM])
        at = keeper_actor_offset(room)
        row = list(struct.unpack_from("<HhhhhhhH", room, at))
        row[1:4], row[5] = [round(value) for value in contract["position"]], contract["yaw"]
        struct.pack_into("<HhhhhhhH", room, at, *row)
        resources[ROOM] = bytes(room)
        manifest = json.loads(resources[MANIFEST])
        manifest["keeper"] = contract
        resources[MANIFEST] = (json.dumps(manifest, indent=2) + "\n").encode()
        changed = [name for name in names if baseline.read(name) != resources[name]]
        assert set(changed) == {ROOM, MANIFEST}, "Keeper patch changed an unexpected resource"
        output.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(output, "x") as candidate:
            for info in baseline.infolist():
                candidate.writestr(info, resources[info.filename])
    with zipfile.ZipFile(source) as baseline, zipfile.ZipFile(output) as candidate:
        assert baseline.namelist() == candidate.namelist()
        for name in names:
            assert (baseline.read(name) != candidate.read(name)) == (name in changed), f"Unexpected payload mutation: {name}"
        assert keeper_placement(candidate.read(ROOM)) == {key: contract[key] for key in ("position", "yaw", "scale")}
        geometry = geometry_report(candidate)
    assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash, "Retained baseline changed"
    report = {"baseline": str(source.resolve()), "baseline_sha256": source_hash,
              "candidate": str(output.resolve()), "candidate_sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
              "contract": contract, "changed_resources": changed, "preserved_resources": len(names) - len(changed),
              "geometry": geometry,
              "preservation": "All other ZIP entry payloads byte-identical, including skeleton, both animations, every model/room vertex and texture, collision, and OoT room/scene headers."}
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    result = patch(args.source, args.output, args.report)
    print(json.dumps({key: result[key] for key in ("candidate", "candidate_sha256", "changed_resources", "preserved_resources", "geometry")}, indent=2))


if __name__ == "__main__":
    main()
