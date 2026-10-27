#!/usr/bin/env python3
"""Check retained Prelude source and optional complete scene without editing them."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
SCENE = ROOT / "tools/magic_mirror/scene"
MANIFEST = json.loads((SCENE / "manifest.json").read_text())
COLLISION = "alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODSCollisionHeader_005DF8"
VISUALS = ["scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00Vtx_0019D8",
           "scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00Vtx_002820"]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def collision(data):
    offset = 76  # resource header and six s16 bounds
    count, = struct.unpack_from("<I", data, offset)
    vertices = [struct.unpack_from("<3h", data, offset + 4 + i * 6) for i in range(count)]
    offset += 4 + count * 6
    count, = struct.unpack_from("<I", data, offset)
    polys = [struct.unpack_from("<8h", data, offset + 4 + i * 16) for i in range(count)]
    return [(tuple(vertices[index & 0x1FFF] for index in p[1:4]), p[4:7])
            for p in polys if not ((p[1] >> 13) & 2)]


def floor_at(polys, x, z, top):
    hits = []
    for (a, b, c), normal in polys:
        if normal[1] < 30000:
            continue
        d = (b[0] - a[0]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[0] - a[0])
        if abs(d) < 1e-6:
            continue
        u = ((x - a[0]) * (c[2] - a[2]) - (z - a[2]) * (c[0] - a[0])) / d
        v = ((b[0] - a[0]) * (z - a[2]) - (b[2] - a[2]) * (x - a[0])) / d
        y = a[1] + u * (b[1] - a[1]) + v * (c[1] - a[1])
        if min(u, v) >= -1e-6 and u + v <= 1.000001 and y <= top:
            hits.append(y)
    return max(hits, default=-32000)


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def blocked(polys, start, end):
    direction = sub(end, start)
    for (a, b, c), _ in polys:
        e1, e2 = sub(b, a), sub(c, a)
        h = cross(direction, e2)
        det = dot(e1, h)
        if abs(det) < 1e-6:
            continue
        s = sub(start, a)
        u = dot(s, h) / det
        q = cross(s, e1)
        v = dot(direction, q) / det
        t = dot(e2, q) / det
        if min(u, v) >= -1e-6 and u + v <= 1.000001 and 1e-6 < t <= 1.000001:
            return True
    return False


def verify(path, compiled=False):
    archive = path.read_bytes()
    identity = MANIFEST["compiled_scene" if compiled else "source_edit"]
    assert digest(archive) == identity["sha256"], "scene input identity changed"
    with zipfile.ZipFile(path) as pack:
        assert pack.testzip() is None
        data = pack.read(COLLISION)
        assert digest(data) == (identity["collision_sha256"] if compiled else MANIFEST["source_collision_sha256"])
        polys = collision(data)
        if compiled:
            # The full forest converts native vertex edits to its MM3D mesh.
            resource = "custom/cor_mm3d/lost_woods_poc1/room_0/Geometry4_vtx"
            if resource not in pack.namelist():
                resource = "alt/" + resource
            visual = {tuple(int(v.get(axis)) for axis in "XYZ") for v in ET.fromstring(pack.read(resource))
                      if v.tag == "Vtx"}
            # The final archive supplies a native transition door; no entrance
            # collision is needed by its Open-action hook. Check its actual data
            # and the return footprint against retained static collision.
            scene = pack.read("alt/scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS")
            expected = MANIFEST["authored_door"]
            entry = struct.pack("<4bHhhhhH", 0, -1, 0, -1, expected["actor_id"],
                                *expected["position"], expected["packed_yaw"], expected["params"])
            assert scene.count(entry) == 1, "authored native transition door changed"
            yaw = int(((expected["packed_yaw"] >> 7) & 0x1FF) * 65536 / 360) * (2 * math.pi / 65536)
            door_x, door_y, door_z = expected["position"]
            return_x = door_x + math.sin(yaw) * expected["return_distance"]
            return_z = door_z + math.cos(yaw) * expected["return_distance"]
            for dx, dz in [(0, 0), (40, 0), (-40, 0), (0, 40), (0, -40),
                           (28, 28), (28, -28), (-28, 28), (-28, -28)]:
                x, z = return_x + dx, return_z + dz
                assert abs(floor_at(polys, x, z, door_y + 5) - door_y) <= 5, "unsupported door return"
                assert not blocked(polys, (x, door_y + 5, z), (x, door_y + 100, z)), "low door-return ceiling"
                for height in (10, 55, 100):
                    assert not blocked(polys, (return_x, door_y + height, return_z),
                                       (x, door_y + height, z)), "wall at door return"
        else:
            project = json.loads(pack.read("prelude/project/edits.json"))
            edits = next(e["data"] for e in project["edits"]["Z2_LOST_WOODS"] if e["kind"] == "geometry")
            visual = {tuple(v["pos"]) for v in edits["verts"] if v["path"] in VISUALS}
        cap = [p for p, normal in polys if normal[1] >= 30000 and
               all(v[1] == 80 and -433 <= v[0] <= -321 and 359 <= v[2] <= 473 for v in p)]
        cap_vertices = {v for triangle in cap for v in triangle}
        assert len(cap) == 12 and len(cap_vertices) == 13 and cap_vertices <= visual, "visual/collision stump mismatch"
        # Execute the actual production locator on every horizontal scene face,
        # then translate the entire scene beyond the old position/height bounds.
        triangles = [p for p, normal in polys if normal[1] >= 30000]
        rows = ["MagicMirrorAnchor::Triangle{{" + ",".join("MagicMirrorAnchor::Point{" +
                 ",".join(str(c) for c in v) + "}" for v in p) + "}}" for p in triangles]
        code = '#include "combo/gameplay/MagicMirrorAnchor.h"\n#include <cassert>\nint main(){\n'
        code += "std::vector<MagicMirrorAnchor::Triangle> t{" + ",".join(rows) + "};\nMagicMirrorAnchor::Point p;\n"
        code += "assert(MagicMirrorAnchor::Find(t,p));assert(p.x==-377&&p.y==80&&p.z==416);\n"
        code += "for(auto& a:t)for(auto& v:a.p){v.x+=1200;v.y+=260;v.z-=2000;}\n"
        code += "assert(MagicMirrorAnchor::Find(t,p));assert(p.x==823&&p.y==340&&p.z==-1584);}\n"
        with tempfile.TemporaryDirectory(prefix="mirror-scene-") as directory:
            source, binary = Path(directory) / "scene.cpp", Path(directory) / "scene"
            source.write_text(code)
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-I" + str(ROOT), str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
        x, y, z = MANIFEST["current_clear_landing"]
        assert floor_at(polys, x, z, 81) == y
        for dx, dz in [(0, -120), (85, -85), (120, 0), (85, 85), (0, 120), (-85, 85), (-120, 0), (-85, -85)]:
            edge = (x + dx / 3, z + dz / 3)
            assert abs(floor_at(polys, edge[0], edge[1], y + 5) - y) <= 5, "unsupported landing footprint"
            for height in (10, 55, 100):
                assert not blocked(polys, (x, y + height, z), (edge[0], y + height, edge[1])), "wall at landing"
            assert not blocked(polys, (edge[0], y + 5, edge[1]), (edge[0], y + 100, edge[1])), "low ceiling"
    print("PASS", path.name, "identity, visual/collision stump, production locator after scene translation, base floor/clearance")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiled", type=Path)
    args = parser.parse_args()
    verify(SCENE / MANIFEST["source_edit"]["path"])
    if args.compiled:
        verify(args.compiled, compiled=True)
