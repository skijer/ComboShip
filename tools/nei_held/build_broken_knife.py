"""Derive a closed fractured Knife from its reviewed serialized GI resources.

The original Knife, material maps and all below-fracture vertices are retained.
Coordinates are in the source's quantized author frame (16 units per author
unit). The held attachment keeps the intact Knife's uniform scale and grip.
"""
from copy import deepcopy
from pathlib import Path
import argparse
import hashlib
import json
import math
import xml.etree.ElementTree as ET

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
ASSETS = ROOT / "soh/assets/custom"
SOURCE = "objects/nei_gi_redesign/giants_knife"
TARGET = "objects/nei_held_swords/broken_giants_knife/model"
FIELDS = ("X", "Y", "Z", "S", "T", "R", "G", "B", "A")
# An asymmetric chipped edge, with a maximum held tip just under X=2500.
FRACTURE = ((-400, 782), (-75, 838), (-18, 742), (40, 902), (90, 802), (400, 882))
GEOMETRY = {"LoadVertices", "Triangles2", "Triangle1"}


def clip(polygon, distance):
    result = []
    for a, b in zip(polygon, polygon[1:] + polygon[:1]):
        da, db = distance(a), distance(b)
        if da >= -1e-8:
            result.append(a)
        if (da >= 0) != (db >= 0):
            result.append(a + da / (da - db) * (b - a))
    return result


def pieces(triangle):
    # Avoid splitting or requantizing the protected guard/grip and lower blade.
    if max(v[1] for v in triangle) < min(y for _, y in FRACTURE):
        return [triangle]
    result = []
    for (lo, ya), (hi, yb) in zip(FRACTURE, FRACTURE[1:]):
        slope = (yb - ya) / (hi - lo)
        polygon = clip(list(triangle), lambda v: v[0] - lo)
        polygon = clip(polygon, lambda v: hi - v[0])
        polygon = clip(polygon, lambda v: ya + slope * (v[0] - lo) - v[1])
        for i in range(1, len(polygon) - 1):
            tri = [polygon[0], polygon[i], polygon[i + 1]]
            if np.linalg.norm(np.cross(tri[1][:3] - tri[0][:3], tri[2][:3] - tri[0][:3])) > .01:
                result.append(tri)
    return result


def build(output=ASSETS):
    original = ET.parse(ASSETS / SOURCE / "gi_dl").getroot()
    records = [np.array([int(v.get(k)) for k in FIELDS], float)
               for v in ET.parse(ASSETS / SOURCE / "mesh_opa_vtx").getroot()]
    display = ET.Element("DisplayList", Version="0")
    vertex_root = ET.Element("Vertex", Version="0")
    cache, pending, steel, state = {}, [], [], []
    texture, reading_state, lighting = "", False, False

    def emit(triangles):
        vertices = [v for tri in triangles for v in tri]
        for start in range(0, len(vertices), 30):
            batch = vertices[start:start + 30]
            offset = len(vertex_root)
            ET.SubElement(display, "LoadVertices", Path=TARGET + "/mesh_opa_vtx",
                          VertexBufferIndex="0", VertexOffset=str(offset), Count=str(len(batch)))
            for v in batch:
                values = np.rint(v).astype(int)
                # Lit normals are signed int8 in the resource's byte fields;
                # unlit vertex colors remain ordinary unsigned channels.
                values[5:8] &= 255
                ET.SubElement(vertex_root, "Vtx", **dict(zip(FIELDS, map(str, values))))
            for i in range(0, len(batch), 3):
                ET.SubElement(display, "Triangle1", V00=str(i), V01=str(i + 1), V02=str(i + 2), Flag0="0")

    def cap():
        triangles = []
        for (lo, ya), (hi, yb) in zip(FRACTURE, FRACTURE[1:]):
            slope = (yb - ya) / (hi - lo)
            points = {}
            for tri in steel:
                for v in tri:
                    if lo - 1e-6 <= v[0] <= hi + 1e-6 and abs(v[1] - ya - slope * (v[0] - lo)) < 1e-5:
                        points[tuple(np.round(v[:3], 6))] = v[:3]
            if len(points) < 3:
                continue
            center = np.mean(list(points.values()), axis=0)
            ring = sorted(points.values(), key=lambda p: math.atan2(p[2] - center[2], p[0] - center[0]))
            normal = np.array([-slope, 1, 0], float)
            normal *= 127 / np.linalg.norm(normal)

            def vertex(p):
                return np.array([*p, p[0] * 4, p[2] * 12, *normal, 255])

            for a, b in zip(ring, ring[1:] + ring[:1]):
                # Reverse x/z winding so the exposed fracture faces outward.
                if np.linalg.norm(np.cross(b - center, a - center)) > .01:
                    triangles.append([vertex(center), vertex(b), vertex(a)])
        return triangles

    for command in original:
        if command.tag not in GEOMETRY and pending:
            emit(pending)
            pending = []
        if command.tag == "LoadTextureBlock":
            texture = Path(command.get("Path")).name
        if command.tag == "LoadGeometryMode":
            lighting = bool(int(command.get("G_LIGHTING", "0")))
        elif command.tag == "SetGeometryMode" and command.get("G_LIGHTING") == "1":
            lighting = True
        elif command.tag == "ClearGeometryMode" and command.get("G_LIGHTING") == "1":
            lighting = False
        if command.tag == "LoadVertices":
            reading_state = False
            offset, count = int(command.get("VertexOffset")), int(command.get("Count"))
            first = int(command.get("VertexBufferIndex", "0"))
            cache.update({first + i: v for i, v in enumerate(records[offset:offset + count])})
        elif command.tag in ("Triangles2", "Triangle1"):
            groups = [("V00", "V01", "V02")]
            if command.tag == "Triangles2":
                groups.append(("V10", "V11", "V12"))
            for group in groups:
                triangle = [cache[int(command.get(k))].copy() for k in group]
                if lighting:
                    for vertex in triangle:
                        vertex[5:8] = (vertex[5:8] + 128) % 256 - 128
                clipped = pieces(triangle)
                pending.extend(clipped)
                if texture == "ordon_steel_tex":
                    steel.extend(clipped)
        else:
            if command.tag == "PopMatrix":
                for material in state:
                    display.append(deepcopy(material))
                fracture = cap()
                assert fracture, "Missing exposed fracture surface"
                emit(fracture)
                ET.SubElement(display, "Texture", S="0", T="0", Level="0", Tile="0", On="0")
            copy = deepcopy(command)
            if copy.tag == "Matrix":
                copy.set("Path", TARGET + "/scale_mtx")
                reading_state = True
            elif reading_state:
                state.append(deepcopy(copy))
            display.append(copy)
    output = Path(output) / TARGET
    output.mkdir(parents=True, exist_ok=True)
    for name, root in (("gi_dl", display), ("mesh_opa_vtx", vertex_root)):
        text = ET.tostring(root, encoding="unicode", short_empty_elements=True)
        (output / name).write_text(text.replace("><", ">\n<") + "\n")
    (output / "scale_mtx").write_bytes((ASSETS / SOURCE / "scale_mtx").read_bytes())
    return {"source": SOURCE, "source_sha256": {
        name: hashlib.sha256((ASSETS / SOURCE / name).read_bytes()).hexdigest()
        for name in ("gi_dl", "mesh_opa_vtx", "scale_mtx")},
        "fracture_quantized_xy": FRACTURE, "vertex_count": len(vertex_root),
        "bounds_author": [[min(int(v.get(k)) for v in vertex_root) / 16,
                            max(int(v.get(k)) for v in vertex_root) / 16] for k in ("X", "Y", "Z")],
        "preserved": "source textures, hilt, uniform grip scale; no gameplay changes",
        "runtime_tested": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ASSETS)
    parser.add_argument("--report", type=Path, default=ROOT / "tools/nei_held/BROKEN_KNIFE.json")
    args = parser.parse_args()
    report = build(args.output)
    args.report.write_text(json.dumps(report, indent=2) + "\n")
    print("Wrote closed authored broken Giant's Knife; source resources unchanged")
