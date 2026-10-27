"""Catch replacement or distortion of the preserved Shadow Crystal mesh."""
from collections import Counter, defaultdict
from pathlib import Path
import sys
import xml.etree.ElementTree as ET
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/nei_gi/SOURCE"))
from quest_revamp import shadow_crystal


def face_key(vertices):
    face = tuple(vertices)
    return min(face[i:] + face[:i] for i in range(3))


def original_faces():
    assets = ROOT / "soh/assets/custom"
    prefix = "objects/object_nei_shadow_crystal/"
    result = defaultdict(Counter)
    for group in range(3):
        material = ET.parse(assets / (prefix + f"mat_shadow_crystal_{group}")).getroot()
        color = next(command for command in material if command.tag == "SetPrimColor")
        color = tuple(int(color.get(channel)) for channel in ("R", "G", "B"))
        cache = {}
        for command in ET.parse(assets / (prefix + f"shadow_crystal_tri_{group}")).getroot():
            if command.tag == "LoadVertices":
                vertices = list(ET.parse(assets / command.get("Path")).getroot())
                offset, count, start = (int(command.get(key)) for key in
                                        ("VertexOffset", "Count", "VertexBufferIndex"))
                for index in range(count):
                    vertex = vertices[offset + index]
                    cache[start + index] = tuple(int(vertex.get(key)) for key in
                                                 ("X", "Y", "Z", "S", "T", "R", "G", "B"))
            elif command.tag in ("Triangle1", "Triangles2"):
                fields = [("V00", "V01", "V02")]
                if command.tag == "Triangles2":
                    fields.append(("V10", "V11", "V12"))
                for triangle in fields:
                    result[color][face_key([cache[int(command.get(key))] for key in triangle])] += 1
    return dict(result)


def test_preserved_geometry():
    expected = original_faces()
    assert sum(sum(faces.values()) for faces in expected.values()) == 616
    # Two triangles were already collapsed by the original integer export.
    # They draw no pixels; retain every renderable face without reauthoring it.
    expected = {color: Counter({face: count for face, count in faces.items()
                if np.any(np.cross(np.array(face[1][:3]) - face[0][:3],
                                   np.array(face[2][:3]) - face[0][:3]))})
                for color, faces in expected.items()}
    assert sum(sum(faces.values()) for faces in expected.values()) == 614
    model = shadow_crystal()
    actual = defaultdict(Counter)
    for part in model.parts:
        material = model.materials[part["mat"]]
        assert material["alpha"] == 1, "The original opaque mesh must not acquire a translucent crystal shell"
        color = tuple(round(value * 255) for value in material["color"])
        for triangle in part["tri"]:
            vertices = []
            for index in triangle:
                position = tuple(int(value) for value in part["p"][index])
                uv = tuple(round(value * 1024) for value in part["uv"][index])
                normal = tuple(round(value * 127) & 255 for value in part["n"][index])
                vertices.append(position + uv + normal)
            actual[color][face_key(vertices)] += 1
    assert dict(actual) == expected, "GI must preserve original triangles, winding, materials, UVs and normals"
    extent = max(abs(float(point[1])) for part in model.parts for point in part["p"])
    assert extent * 16 * model.native_scale * model.draw_scale <= 27, "Overhead GI must stay within its established height"
    print("PASS Shadow Crystal: all 614 renderable original triangles, three materials, winding, UVs, normals and GI height")


if __name__ == "__main__":
    test_preserved_geometry()
