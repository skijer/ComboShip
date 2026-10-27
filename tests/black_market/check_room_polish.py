#!/usr/bin/env python3
"""Check visible room closure and native dynamic-cosmetic binding coverage."""
import argparse
from collections import Counter
import math
import struct
import xml.etree.ElementTree as ET
import zipfile


def packed_faces(pack, root="custom/skiejer_market/room"):
    cache, faces = {}, []

    def visit(path):
        for cmd in ET.fromstring(pack.read(path)):
            if cmd.tag == "CallDisplayList":
                visit(cmd.get("Path"))
            elif cmd.tag == "LoadVertices":
                vertices = list(ET.fromstring(pack.read(cmd.get("Path"))))
                start, offset, count = (int(cmd.get(a)) for a in ("VertexBufferIndex", "VertexOffset", "Count"))
                for i, vertex in enumerate(vertices[offset:offset + count]):
                    cache[start + i] = tuple(float(vertex.get(a)) for a in ("X", "Y", "Z"))
            elif cmd.tag in ("Triangle1", "Triangles2"):
                for face in range(2 if cmd.tag == "Triangles2" else 1):
                    keys = [f"V{face}{i}" for i in range(3)]
                    faces.append(tuple(cache[int(cmd.get(k))] for k in keys))

    visit(root)
    return faces


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0])


def check_collision(pack):
    # Collision must follow the actual root draw graph. An unreferenced donor
    # list previously produced invisible interior obstacles despite closure.
    expected = Counter(face for face in packed_faces(pack)
                       if dot(cross(sub(face[1], face[0]), sub(face[2], face[0])),
                              cross(sub(face[1], face[0]), sub(face[2], face[0]))) >= .01)
    data = pack.read("scenes/nonmq/skiejer_market/collision")
    vertex_count, = struct.unpack_from("<I", data, 76)
    vertices = [struct.unpack_from("<3h", data, 80 + i * 6) for i in range(vertex_count)]
    offset = 80 + vertex_count * 6
    polygon_count, = struct.unpack_from("<I", data, offset)
    actual = Counter()
    for i in range(polygon_count):
        row = struct.unpack_from("<HHHHhhhh", data, offset + 4 + i * 16)
        actual[tuple(vertices[index & 0x1FFF] for index in row[1:4])] += 1
    missing, extra = sum((expected - actual).values()), sum((actual - expected).values())
    assert not missing and not extra, f"collision differs from drawn room: {missing} missing, {extra} invisible faces"
    print(f"PASS collision: all {polygon_count} drawn nondegenerate faces, no invisible donor lists")


def check_repairs(pack):
    prefix = "custom/skiejer_market/geometry/"
    donor = {frozenset(face) for face in packed_faces(pack, prefix + "mahouya_room_0DL_006548")}
    added = packed_faces(pack, prefix + "room_boundary_repairs")
    assert len(added) == 7, "only two shelf-end quads and the thin floor band need repairs"
    assert all(frozenset(face) not in donor for face in added), "repair duplicates an existing native donor face"
    print("PASS repairs: seven added faces, no duplicate donor triangles")


def normal(v):
    length = math.sqrt(dot(v, v))
    return tuple(x / length for x in v)


def hits(origin, direction, face):
    a, b, c = face
    edge1, edge2 = sub(b, a), sub(c, a)
    h = cross(direction, edge2)
    det = dot(edge1, h)
    if abs(det) < 1e-9:
        return False
    s = sub(origin, a)
    u = dot(s, h) / det
    q = cross(s, edge1)
    v = dot(direction, q) / det
    return -1e-6 <= u <= 1.000001 and v >= -1e-6 and u + v <= 1.000001 and dot(edge2, q) / det > 0


def check_closure(pack):
    # Rays through the three observed empty-background pixels in the accepted
    # camera. A camera change or recolor cannot satisfy this geometry check.
    faces = packed_faces(pack)
    views = [((-35.,86.,-20.), (0.,95.,250.), 45, ((380,165),(400,175),(430,175))),
             ((0.,70.,-30.), (0.,70.,155.), 114, ((75,100),(973,100),(757,320))),
             ((96.,87.,-30.), (96.,87.,48.), 75, ((385,696),))]
    for eye, target, fov, pixels in views:
        forward = normal(sub(target, eye))
        right = normal(cross(forward, (0., 1., 0.)))
        up = cross(right, forward)
        for x, y in pixels:
            sx = (2*(x + .5)/1050 - 1) * math.tan(math.radians(fov)/2) * 1050/760
            sy = (1 - 2*(y + .5)/760) * math.tan(math.radians(fov)/2)
            ray = normal(tuple(forward[i] + sx*right[i] + sy*up[i] for i in range(3)))
            assert any(hits(eye, ray, face) for face in faces), f"room boundary gap remains visible at {fov}-degree preview pixel {(x, y)}"
    # The roof and thin shelf-side faces already existed as native Triangle1
    # commands. Correct preview decoding must preserve them without overlays.
    assert not any("rear_ceiling_join" in path for path in pack.namelist())
    print("PASS room closure: observed empty-background rays meet packed geometry")


def check_cosmetics(pack):
    assert "CosmeticEntries" in pack.namelist(), "room has no native cosmetic-editor manifest"
    entries = list(ET.fromstring(pack.read("CosmeticEntries")))
    materials = [n for n in pack.namelist() if n.startswith("custom/skiejer_market/geometry/mat_")]
    bindings = {entry.get("MaterialPath"): entry for entry in entries}
    assert set(bindings) == set(materials), "every room material must have a cosmetic-editor binding"
    defaults = {}
    for path in materials:
        entry = bindings[path]
        assert entry.get("CosmeticType") == "Prim"
        assert entry.get("CosmeticCategory") == "Skiejer's Black Market"
        color = ET.fromstring(pack.read(path)).find("SetPrimColor")
        assert color is not None
        label = entry.get("CosmeticEntry")
        assert label and color.get("CosmeticEntry") == label
        assert color.get("CosmeticCategory") == entry.get("CosmeticCategory")
        rgba = tuple(int(color.get(channel)) for channel in ("R", "G", "B", "A"))
        assert defaults.setdefault(label, rgba) == rgba, "shared control must reset all bound materials to their original default"
    assert len(defaults) == 4, "preserve the room's four-color palette with four independent controls"
    print(f"PASS cosmetics: {len(materials)} native material bindings, four palette-preserving controls")


def check_oot_scene(pack):
    # Read exactly the fields consumed by SoH's scene-command factories. In
    # particular MM room-behavior bytes and actor-cutscene commands are not a
    # valid substitute for the native OoT room schema.
    prefix = "scenes/shared/skiejer_market_oot/skiejer_market_oot"
    assert prefix in pack.namelist(), "native OoT shop scene is missing"
    for path in (prefix, prefix + "_room_0"):
        data = pack.read(path)
        offset = 68
        count = struct.unpack_from("<I", data, 64)[0]
        actor_count = None

        def take(fmt):
            nonlocal offset
            value = struct.unpack_from(fmt, data, offset)
            offset += struct.calcsize(fmt)
            return value

        def text():
            nonlocal offset
            length, = take("<I")
            value = data[offset:offset+length].decode()
            offset += length
            return value

        for _ in range(count):
            cmd, = take("<I")
            if cmd == 0x04:
                rooms, = take("<I")
                assert rooms == 1 and text() == prefix + "_room_0"
                take("<II")
            elif cmd == 0x03:
                assert text() == "scenes/nonmq/skiejer_market/collision"
            elif cmd == 0x06:
                assert take("<Ibb") == (1, 0, 0)
            elif cmd == 0x07:
                assert take("<bh") == (0, 2)
            elif cmd == 0x00:
                assert take("<I") == (1,)
                spawn = take("<HhhhhhhH")
                assert spawn[3] >= 30 and spawn[-1] == 0x0DFF, "OoT spawn must be idle and outside the exit trigger"
            elif cmd in (0x15, 0x10):
                take("<BBB")
            elif cmd == 0x11:
                take("<BBBB")
            elif cmd == 0x13:
                assert take("<I") == (0,)
            elif cmd == 0x0F:
                assert take("<I") == (1,)
                take("<18BhH")
            elif cmd == 0x08:
                take("<bi")
            elif cmd in (0x0B, 0x01):
                assert take("<I") == (0,), "dedicated controller must own the keeper; no native shop actor list"
                if cmd == 0x01:
                    actor_count = 0
            elif cmd == 0x0A:
                assert take("<BBBB") == (0, 0, 1, 0)
                assert text() == "custom/skiejer_market/room" and text() == ""
            elif cmd == 0x12:
                take("<BB")
            elif cmd == 0x16:
                take("<B")
            else:
                assert cmd == 0x14, f"unsupported OoT scene command {cmd:#x}"
        assert offset == len(data), "native OoT scene has extra bytes or a mismatched command layout"
        if path.endswith("_room_0"):
            assert actor_count == 0
    print("PASS OoT scene: native factory framing, safe idle spawn, shared room and no vanilla shop actor")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pack")
    args = parser.parse_args()
    errors = []
    with zipfile.ZipFile(args.pack) as pack:
        for check in (check_closure, check_collision, check_repairs, check_cosmetics, check_oot_scene):
            try:
                check(pack)
            except AssertionError as error:
                errors.append(str(error))
    assert not errors, "\n".join(errors)
