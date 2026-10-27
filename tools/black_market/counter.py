#!/usr/bin/env python3
"""Raise only the audited Granny counter, preserving POC12 and its native keeper."""
import argparse
import ctypes as C
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import xml.etree.ElementTree as ET
import zipfile

from keeper_lean import ROOM, keeper_placement, room_faces

ROOT = Path(__file__).resolve().parents[2]
BASELINE_SHA256 = "761fa811149f860b6a174901bdda1ad7fdf5610a0064f0e3efe5927853bb1352"
VERTICES = "custom/skiejer_market/geometry/mahouya_room_0DL_006548_vtx_2"
DRAW = "custom/skiejer_market/geometry/mahouya_room_0DL_006548_tri_2"
COLLISION = "scenes/nonmq/skiejer_market/collision"
MANIFEST = "custom/skiejer_market/manifest.json"
TOP_Y = 41
# Actual counter component; the nearby Y12 stool is deliberately excluded.
COUNTER_VERTICES = (
    603,604,605,606,607,634,635,636,637,655,656,657,658,659,660,661,662,663,664,665,666,
    667,668,669,670,671,672,673,688,689,690,691,692,693,694,695,696,697,698,699,700,701,
    702,703,704,705,706,707,708,709,710,711,712,713,714,715,716,717,718,719,720,721,722,
    723,724,725,726,727,728,729,730,731,732,733,734,735,1180,1181,1182,1183,1184,1185,
    1186,1187,1188)
LOW_JOIN_VERTICES = {661, 726, 727, 730, 731, 734, 735}
CAP_POINTS = ((-51,30,93), (-51,41,93), (-51,41,123), (-51,30,123),
              (-51,26,120), (-51,30,120), (-51,30,123), (-51,26,123))
CAP_TRIANGLES = ((0,2,1), (0,3,2), (4,5,6), (4,6,7))


class Resources:
    def __init__(self, resources):
        self.resources = resources

    def read(self, name):
        return self.resources[name]

    def namelist(self):
        return list(self.resources)


def _normal_record(surface, indices, points, flags):
    a, b, c = points
    ab, ac = [b[i] - a[i] for i in range(3)], [c[i] - a[i] for i in range(3)]
    normal = (ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0])
    length = math.sqrt(sum(value * value for value in normal))
    assert length >= .1
    normal = tuple(value / length for value in normal)
    packed = [max(-32767, min(32767, round(value * 32767))) for value in normal]
    distance = round(-sum(normal[i] * a[i] for i in range(3)))
    return struct.pack("<HHHHhhhh", surface, *(index | flag for index, flag in zip(indices, flags)), *packed, distance)


def raise_counter_resources(resources):
    """Return the counter vertex/DL and collision replacements only.

    Main counter sides stretch from their fixed Y0 bases. The Y26/Y30 trim
    moves to Y37/Y41 on the central/right station. Left faces retain their
    separate low seam vertices. Oppositely wound upper/lower end quads close
    only the actual exposed seam; no face spans the empty forward underside.
    Collision appends new vertices, retaining even unused original vertices;
    this avoids moving a collision point shared with unaffected room scenery.
    """
    original = resources[VERTICES]
    rows = list(ET.fromstring(original))
    assert len(COUNTER_VERTICES) == 85
    selected, moved = set(COUNTER_VERTICES), 0
    for index in selected:
        row = rows[index]
        x, y, z = (int(row.get(key)) for key in ("X", "Y", "Z"))
        assert -160 <= x <= 58 and 93 <= z <= 123 and y in (0, 26, 30), "Audited counter component changed"
        moved += y != 0 and x >= -51 and index not in LOW_JOIN_VERTICES
    assert moved == 46
    at = -1

    def vertex(match):
        nonlocal at
        at += 1
        if at not in selected or at in LOW_JOIN_VERTICES or rows[at].get("Y") == "0" or int(rows[at].get("X")) < -51:
            return match[0]
        return re.sub(rb'(\bY=")(-?\d+)(")', lambda value:
                      value[1] + str(int(value[2]) + 11).encode() + value[3], match[0], count=1)

    changed_vertices = re.sub(rb"<Vtx\b[^>]*>", vertex, original)
    assert at + 1 == len(rows)
    # Copy the native counter end-panel UV directions/material, stretching the
    # existing stone tile over the upper step and retained lower lip end.
    cap_xml = []
    for (x, y, z), s, t in zip(CAP_POINTS, (762,649,649,762,803,762,762,803),
                              (701,701,1151,1151,1110,1110,1131,1131)):
        cap_xml.append(f'  <Vtx X="{x}" Y="{y}" Z="{z}" S="{s}" T="{t}" R="160" G="160" B="160" A="255" />\n'.encode())
    assert changed_vertices.count(b"</Vertex>") == 1
    changed_vertices = changed_vertices.replace(b"</Vertex>", b"".join(cap_xml) + b"</Vertex>")
    draw = resources[DRAW]
    addition = (f'  <LoadVertices Path="{VERTICES}" VertexBufferIndex="0" VertexOffset="{len(rows)}" Count="4" />\n'
                '  <Triangles2 V00="0" V01="2" V02="1" Flag0="0" V10="0" V11="3" V12="2" Flag1="0" />\n'
                f'  <LoadVertices Path="{VERTICES}" VertexBufferIndex="0" VertexOffset="{len(rows) + 4}" Count="4" />\n'
                '  <Triangles2 V00="0" V01="1" V02="2" Flag0="0" V10="0" V11="2" V12="3" Flag1="0" />\n').encode()
    assert draw.count(b"  <EndDisplayList />") == 1
    changed_draw = draw.replace(b"  <EndDisplayList />", addition + b"  <EndDisplayList />")
    updated = dict(resources)
    updated[VERTICES], updated[DRAW] = changed_vertices, changed_draw
    old_faces, drawn_new = room_faces(Resources(resources)), room_faces(Resources(updated))
    cap_faces = [[CAP_POINTS[index] for index in triangle] for triangle in CAP_TRIANGLES]
    insertion = next(index for index, face in enumerate(drawn_new) if face == cap_faces[0])
    assert drawn_new[insertion:insertion + 4] == cap_faces
    new_faces = drawn_new[:insertion] + drawn_new[insertion + 4:]
    assert len(old_faces) == len(new_faces)
    # Match the native build's nondegenerate face filter and exact order.
    def valid(face):
        a, b, c = face
        ab, ac = [b[i]-a[i] for i in range(3)], [c[i]-a[i] for i in range(3)]
        n = (ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0])
        return sum(value*value for value in n) >= .01
    pairs = [(a, b) for a, b in zip(old_faces, new_faces) if valid(a)]
    data = resources[COLLISION]
    vertex_count, = struct.unpack_from("<I", data, 76)
    vertices = [struct.unpack_from("<3h", data, 80 + index * 6) for index in range(vertex_count)]
    old_vertex_bytes = data[80:80 + vertex_count * 6]
    polygon_at = 80 + vertex_count * 6
    polygon_count, = struct.unpack_from("<I", data, polygon_at)
    assert polygon_count == len(pairs) == 7819
    index_by_point = {point: index for index, point in enumerate(vertices)}
    polygons, changed_polygons = [], 0
    for index, (old_face, new_face) in enumerate(pairs):
        raw = data[polygon_at + 4 + index * 16:polygon_at + 4 + (index + 1) * 16]
        row = struct.unpack("<HHHHhhhh", raw)
        assert [vertices[value & 8191] for value in row[1:4]] == [tuple(point) for point in old_face], "Native collision face order changed"
        if old_face == new_face:
            polygons.append(raw)
            continue
        indices = []
        for point in new_face:
            point = tuple(int(value) for value in point)
            if point not in index_by_point:
                index_by_point[point] = len(vertices)
                vertices.append(point)
            indices.append(index_by_point[point])
        polygons.append(_normal_record(row[0], indices, new_face, [value & ~8191 for value in row[1:4]]))
        changed_polygons += 1
    assert changed_polygons == 34
    cap_polygons = []
    for face in cap_faces:
        indices = []
        for point in face:
            if point not in index_by_point:
                index_by_point[point] = len(vertices)
                vertices.append(point)
            indices.append(index_by_point[point])
        cap_polygons.append(_normal_record(0, indices, face, (0,0,0)))
    polygon_insertion = sum(valid(face) for face in old_faces[:insertion])
    polygons[polygon_insertion:polygon_insertion] = cap_polygons
    assert len(vertices) < 8192 and len(polygons) == polygon_count + 4
    bounds = [min(point[axis] for point in vertices) for axis in range(3)] + [max(point[axis] for point in vertices) for axis in range(3)]
    assert bounds == list(struct.unpack_from("<6h", data, 64)), "Counter change unexpectedly changed room bounds"
    tail = data[polygon_at + 4 + polygon_count * 16:]
    collision = data[:76] + struct.pack("<I", len(vertices)) + old_vertex_bytes
    collision += b"".join(struct.pack("<3h", *point) for point in vertices[vertex_count:])
    collision += struct.pack("<I", polygon_count + 4) + b"".join(polygons) + tail
    scenery = retained_scenery_report(old_faces, new_faces, rows, cap_faces)
    metadata = {"top_y": TOP_Y, "previous_top_y": 30, "trim_bottom_y": 37,
                "fixed_base_y": 0, "footprint_changed": False, "changed_vertices": moved,
                "changed_collision_faces": changed_polygons,
                "added_counter_vertices": 8, "added_collision_faces": 4,
                "raised_section_x": [-51, 58], "unchanged_prop_section_x": [-160, -51],
                "closed_step": {"x": -51,
                    "upper": {"y_range": [30,41], "z_range": [93,123], "outward_normal": [-1,0,0]},
                    "lower_lip_end": {"y_range": [26,30], "z_range": [120,123], "outward_normal": [1,0,0]},
                    "native_vertex_cache_loads": [[0,4],[0,4]]},
                "retained_scenery": scenery,
                "collision": {"vertices": len(vertices), "triangles": polygon_count + 4, "bounds": bounds}}
    return {VERTICES: changed_vertices, DRAW: changed_draw, COLLISION: collision}, metadata


def retained_scenery_report(old_faces, new_faces, rows, cap_faces):
    """Exclude any newly occupied counter region intersecting retained scenery.

    Every changed counter face is checked against every non-counter scene
    triangle. AABB separation is sufficient here: no unresolved overlap pairs
    remain. The counter's newly occupied region is above Y26; below it the
    existing vertical side planes and fixed ground edges are unchanged.
    """
    import numpy as np
    old, new = np.asarray(old_faces), np.asarray(new_faces)
    positions = {tuple(int(rows[index].get(key)) for key in ("X", "Y", "Z")) for index in COUNTER_VERTICES}
    counter = np.array([all(tuple(point) in positions for point in face) and max(point[1] for point in face) > 1
                        for face in old_faces])
    assert counter.sum() == 50
    changed = (old != new).any((1, 2))
    assert changed.sum() == 34 and np.all(counter[changed])
    left_faces = counter & (old[:, :, 0].max(1) <= -51)
    assert np.array_equal(old[left_faces], new[left_faces]), "Prop-side counter faces changed"
    other = new[~counter]
    other_low, other_high = other.min(1), other.max(1)
    eligible = (other_high[:, 1] > 26) & (other_low[:, 1] < TOP_Y)
    potential = 0
    tested = np.concatenate((new[changed], np.asarray(cap_faces)))
    for face in tested:
        low, high = face.min(0), face.max(0)
        low[1] = max(low[1], 26)
        potential += int((eligible & np.all(other_high >= low, axis=1) & np.all(other_low <= high, axis=1)).sum())
    assert potential == 0, "Raised counter could intersect retained room props/scenery"
    nearby = eligible & (other_low[:, 0] < 58) & (other_high[:, 0] > -160) & \
             (other_low[:, 2] < 123) & (other_high[:, 2] > 93)
    assert nearby.sum() > 0 and np.all(other_high[nearby, 0] < -51)
    return {"room_triangles_checked": len(other), "changed_counter_faces_checked": len(tested),
            "unchanged_left_counter_faces": int(left_faces.sum()),
            "nearby_retained_prop_faces": int(nearby.sum()), "potential_intersections": potential,
            "minimum_x_gap_to_nearby_props": float(-51 - other_high[nearby, 0].max()),
            "method": "Every changed counter face, restricted to newly occupied Y26..41, has disjoint AABBs from all retained non-counter room triangles. Book station and all prop payloads remain unchanged."}


def minimum_y_in_strip(triangles, front=93, back=123):
    """Exact triangle/strip clipping, including intersections along triangle edges."""
    import numpy as np
    z, y = triangles[..., 2], triangles[..., 1]
    low = np.where((z >= front) & (z <= back), y, np.inf).min(-1)
    for i, j in ((0, 1), (1, 2), (2, 0)):
        dz = z[..., j] - z[..., i]
        for plane in (front, back):
            crosses = (z[..., i] - plane) * (z[..., j] - plane) <= 0
            fraction = np.divide(plane - z[..., i], dz, out=np.zeros_like(dz), where=abs(dz) > 1e-12)
            value = y[..., i] + (y[..., j] - y[..., i]) * fraction
            low = np.minimum(low, np.where(crosses & (abs(dz) > 1e-12), value, np.inf))
    return low


def keeper_report(archive):
    import numpy as np
    from native_preview import NativeKeeper
    keeper = NativeKeeper(archive)
    placement = keeper_placement(archive.read(ROOM))
    triangles = np.array([keeper.position_triangles(frame, placement['position'], placement['yaw'], placement['scale'])
                          for frame in range(keeper.frame_count)])
    assert triangles.shape == (32, 16304, 3, 3)
    # All keeper X coordinates are inside the actual central countertop polygon
    # [-51,30] x [93,123]; therefore clipping Z is sufficient for separation.
    assert triangles[..., 0].min() > -51 and triangles[..., 0].max() < 30
    minimum = minimum_y_in_strip(triangles).min(1)
    assert minimum.min() > TOP_Y
    ranges, at = {}, 0
    for index in keeper.draw_order:
        limb = keeper.limbs[index]
        count = sum(2 if opcode == 6 else 1 if opcode == 5 else 0 for opcode, _, _, _ in keeper.commands(limb.display))
        if any(word in limb.name for word in ("HandLimb", "ForearmLimb")):
            ranges[limb.name.rsplit("/", 1)[-1]] = (at, at + count)
        at += count
    hands, hand_gaps = {}, []
    for side in ("Left", "Right"):
        a, b = ranges[f"gTreasureChestShopGal{side}HandLimb"]
        hand = triangles[:, a:b]
        gaps = hand[..., 1].min((1, 2)) - TOP_Y
        normal = np.cross(hand[:, :, 1] - hand[:, :, 0], hand[:, :, 2] - hand[:, :, 0])
        area = np.linalg.norm(normal, axis=2)
        downward = np.divide(normal[:, :, 1], area, out=np.zeros_like(area), where=area > 1e-12) < -.85
        weighted = (hand.mean(2)[:, :, 1] * area * downward).sum(1) / (area * downward).sum(1) - TOP_Y
        a, b = ranges[f"gTreasureChestShopGal{side}ForearmLimb"]
        forearm = minimum_y_in_strip(triangles[:, a:b]).min(1) - TOP_Y
        hands[side.lower()] = {"triangles": hand.shape[1], "minimum_surface_gap_each_frame": gaps.tolist(),
            "minimum_surface_gap_range": [float(gaps.min()), float(gaps.max())],
            "downward_surface_area_weighted_gap_range": [float(weighted.min()), float(weighted.max())],
            "forearm_minimum_gap_range": [float(forearm.min()), float(forearm.max())]}
        hand_gaps.append(gaps)
    least_contact = int(np.argmax(np.min(hand_gaps, axis=0)))
    return {"animation": keeper.animation_path, "frames_checked": keeper.frame_count,
            "minimum_counter_clearance": float(minimum.min() - TOP_Y),
            "highest_nonintersecting_integer_top": math.floor(float(minimum.min())),
            "hands": hands, "least_contact_frame": least_contact,
            "continuous_flat_contact_proven": False,
            "contact_note": "Static Y41 is the highest nonintersecting integer top; fingers, palms and forearms still move above it. Exact continuous flat palm contact is not claimed.",
            "method": "All actual mesh triangles clipped to the central countertop polygon in every native integer frame; includes triangle interiors, fingers and both forearms."}


class TextureSink:
    """Receive irrelevant texture uploads while retaining checkpoint geometry verbatim."""
    uint = C.c_uint
    def GenTextures(self, count, pointer):
        C.cast(pointer, C.POINTER(C.c_uint))[0] = 1
    def BindTexture(self, *args): pass
    def TexParameteri(self, *args): pass
    def TexImage(self, *args): pass


def merchandise_report(json_include=None):
    import numpy as np
    import presentation_preview as pp
    stock = pp.production_stock(json_include)["stock"]
    shelves = pp.shelf_positions()
    lock_source = (ROOT / "combo/BlackMarketLock.h").read_text()
    for expression in ("(std::max(half[0],2.4f)+1.4f)/.875f", "(std::max(half[1],3.2f)+1.4f)/.875f",
                       "(std::max(half[2],2.4f)+1.4f)/.875f", "std::clamp(std::min(half[0],half[1])*.07f,.5f,.85f)", "rz+loop*.45f"):
        assert expression in lock_source, "Inspect changed native X formula before using its continuous envelope"
    records = []
    for slot, (item, position) in enumerate(zip(stock, shelves)):
        models, meta, model_hash = pp.checkpoint(TextureSink(), item["slug"])
        points = np.concatenate([array[:, :3] for _, array, _ in models]).astype(float)
        low, high, lock_low, lock_high = [], [], [], []
        native_samples = 0
        for caller in (.25, .32):
            factor, lift = caller * item["fit_scale"], caller * item["fit_lift"]
            radius = float(np.linalg.norm(points[:, (0, 2)], axis=1).max()) * factor
            minimum = np.array(position) + [-radius, -2 + lift + points[:, 1].min() * factor, -radius]
            maximum = np.array(position) + [radius, 2 + lift + points[:, 1].max() * factor, radius]
            low.append(minimum); high.append(maximum)
            half = [radius, float(np.ptp(points[:, 1])) * factor / 2, radius]
            extra = []
            for axis, floor in enumerate((2.4, 3.2, 2.4)):
                # An actual AABB may have any center inside the swept bounds.
                # Bound rx-half over h in [0,max_half], plus ribbon width/loop
                # offset and half of one native 1/64 quantization unit.
                extension = max((floor + 1.4) / .875,
                                (max(half[axis], floor) + 1.4) / .875 - half[axis])
                extra.append(extension + (.45 if axis == 2 else .85) + 1/128)
            lock_low.append(minimum - extra); lock_high.append(maximum + extra)
            # Production C++ adapter validates the derivation on real item AABBs.
            for angle in np.linspace(0, 2 * math.pi, 16, endpoint=False):
                rotated = points * factor
                x, z = rotated[:, 0].copy(), rotated[:, 2].copy()
                rotated[:, 0] = x * math.cos(angle) + z * math.sin(angle)
                rotated[:, 2] = -x * math.sin(angle) + z * math.cos(angle)
                for bob in (-2, 2):
                    world = rotated + np.array(position) + [0, lift + bob, 0]
                    ribbon = pp.native_lock().vertices(world.min(0), world.max(0), 0, caller == .32)[:, :3]
                    assert len(ribbon) == 96
                    assert np.all(ribbon.min(0) >= lock_low[-1] - 1e-5) and np.all(ribbon.max(0) <= lock_high[-1] + 1e-5)
                    native_samples += 1
        minimum, maximum = np.min(low, axis=0), np.max(high, axis=0)
        ribbon_min, ribbon_max = np.min(lock_low, axis=0), np.max(lock_high, axis=0)
        records.append({"slot": slot + 1, "slug": item["slug"], "checkpoint_sha256": model_hash,
            "model_swept_bounds": [minimum.tolist(), maximum.tolist()],
            "native_x_swept_bounds": [ribbon_min.tolist(), ribbon_max.tolist()],
            "model_vertical_clearance": float(minimum[1] - TOP_Y),
            "model_front_clearance": float(93 - maximum[2]),
            "native_x_vertical_clearance": float(ribbon_min[1] - TOP_Y),
            "native_x_front_clearance": float(93 - ribbon_max[2]),
            # A counter display may sit above the top, in front of it, or both.
            "model_clearance_proven": bool(minimum[1] > TOP_Y or maximum[2] < 93),
            "native_x_clearance_proven": bool(ribbon_min[1] > TOP_Y or ribbon_max[2] < 93),
            "native_x_adapter_cases": native_samples})
    assert len(records) == 8 and all(row["model_clearance_proven"] and row["native_x_clearance_proven"] for row in records)
    return {"scope": "Eight actual production-generated sample offers and retained checkpoint geometry; no claim for every native/Alt/replacement merchandise path.",
            "method": "Conservative continuous 360-degree model radius and complete +/-2 bob at callers .25/.32; analytic production native-X envelope including minimum radii, ribbon width, second-loop offset and quantization, checked against real C++ BuildLock on 64 cases/item.",
            "lock_source_sha256": hashlib.sha256(lock_source.encode()).hexdigest(), "items": records,
            "shelf_positions": shelves}


def patch(source, output, report_path, json_include=None):
    source, output, report_path = Path(source), Path(output), Path(report_path)
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    if source_hash != BASELINE_SHA256:
        raise ValueError(f"Expected exact POC12 baseline SHA256 {BASELINE_SHA256}, got {source_hash}")
    if len({source.resolve(), output.resolve(), report_path.resolve()}) != 3:
        raise ValueError("Source, candidate output and report paths must be distinct")
    if output.exists() or report_path.exists():
        raise FileExistsError("Refusing to overwrite a retained candidate/report")
    with zipfile.ZipFile(source) as baseline:
        names = baseline.namelist()
        assert len(names) == len(set(names))
        resources = {name: baseline.read(name) for name in names}
        changes, counter = raise_counter_resources(resources)
        resources.update(changes)
        keeper = keeper_report(Resources(resources))
        merchandise = merchandise_report(json_include)
        manifest = json.loads(resources[MANIFEST])
        manifest["counter"], manifest["collision"] = counter, counter["collision"]
        resources[MANIFEST] = (json.dumps(manifest, indent=2) + "\n").encode()
        changed = sorted(name for name in names if baseline.read(name) != resources[name])
        assert changed == sorted((VERTICES, DRAW, COLLISION, MANIFEST))
        output.parent.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(output, "x") as candidate:
            for info in baseline.infolist():
                candidate.writestr(info, resources[info.filename])
    with zipfile.ZipFile(source) as baseline, zipfile.ZipFile(output) as candidate:
        assert baseline.namelist() == candidate.namelist()
        for name in names:
            assert (baseline.read(name) != candidate.read(name)) == (name in changed)
    assert hashlib.sha256(source.read_bytes()).hexdigest() == source_hash
    report = {"baseline": str(source.resolve()), "baseline_sha256": source_hash,
              "candidate": str(output.resolve()), "candidate_sha256": hashlib.sha256(output.read_bytes()).hexdigest(),
              "changed_resources": changed, "preserved_resources": len(names) - 4,
              "counter": counter, "keeper": keeper, "merchandise": merchandise,
              "status": "Offline geometry/clearance candidate only; exact continuous palm contact, configured gameplay and Alt parity unproven."}
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + "\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("source", "output", "report"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--json-include", type=Path)
    args = parser.parse_args()
    report = patch(args.source, args.output, args.report, args.json_include)
    print(json.dumps({key: report[key] for key in ("candidate", "candidate_sha256", "changed_resources", "preserved_resources", "counter", "keeper")}, indent=2))


if __name__ == "__main__":
    main()
