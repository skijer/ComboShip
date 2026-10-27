"""Restore Mario SM64 POC3 from its exact retained serialized source.

The missing procedural recipe is not substituted with the earlier Mario mask.
Runtime resources are copied verbatim; the inspection GLB is reconstructed from
the display-list cache, preserving positions, winding, normals, UVs and colors.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET
import zipfile

from PIL import Image

ROOT = Path(__file__).resolve().parents[3]
SOURCE = Path(__file__).with_suffix("")
ARCHIVE = SOURCE / "serialized_source.o2r"
PREFIX = "objects/nei_gi_redesign/mario_mask/"
ICON = "textures/icon_item_custom/gItemIconMarioMaskTex"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def retained_resources():
    meta = json.loads((SOURCE / "source.json").read_text())
    assert sha(ARCHIVE.read_bytes()) == meta["archive_sha256"], "changed retained Mario archive"
    with zipfile.ZipFile(ARCHIVE) as archive:
        names = archive.namelist()
        base = set(meta["resources"])
        assert len(names) == len(set(names)) == 8
        assert set(names) == base | {"alt/" + name for name in base}
        resources = {name: archive.read(name) for name in base}
        for name, data in resources.items():
            assert sha(data) == meta["resources"][name], name
            assert data == archive.read("alt/" + name), (name, "base/Alt mismatch")
    return meta, resources


def geometry(resources):
    vertices = ET.fromstring(resources[PREFIX + "mesh_opa_vtx"])
    records = [{k: int(v.get(k)) for k in ("X", "Y", "Z", "S", "T", "R", "G", "B", "A")}
               for v in vertices]
    matrix = resources[PREFIX + "scale_mtx"]
    words = struct.unpack_from("<16I", matrix, 64)
    fixed = []
    for i in range(8):
        for shift in (16, 0):
            bits = (((words[i] >> shift) & 65535) << 16) | ((words[i + 8] >> shift) & 65535)
            fixed.append((bits - (1 << 32) if bits >= 1 << 31 else bits) / 65536)
    scale = fixed[0]
    assert fixed == [scale if i in (0, 5, 10) else (1 if i == 15 else 0) for i in range(16)]
    runs, cache, loads = [], {}, 0
    for command in ET.fromstring(resources[PREFIX + "gi_dl"]):
        if command.tag == "SetPrimColor":
            runs.append({"color": [int(command.get(k)) for k in ("R", "G", "B", "A")], "indices": []})
        elif command.tag == "LoadVertices":
            offset, count, start = (int(command.get(k)) for k in ("VertexOffset", "Count", "VertexBufferIndex"))
            assert 0 < count <= 32 and 0 <= start and start + count <= 32
            assert 0 <= offset and offset + count <= len(records)
            cache = {start + i: offset + i for i in range(count)}
            loads += 1
        elif command.tag in ("Triangle1", "Triangles2"):
            fields = [("V00", "V01", "V02")]
            if command.tag == "Triangles2":
                fields.append(("V10", "V11", "V12"))
            for face in fields:
                runs[-1]["indices"].extend(cache[int(command.get(k))] for k in face)
    return records, [run for run in runs if run["indices"]], scale, loads


def glb_bytes(records, runs, scale):
    blob, views, accessors = bytearray(), [], []

    def accessor(values, kind, fmt, component):
        blob.extend(b"\0" * (-len(blob) % 4))
        width = {"SCALAR": 1, "VEC2": 2, "VEC3": 3}[kind]
        flat = [v for row in values for v in row]
        data = struct.pack("<" + fmt * len(flat), *flat)
        views.append({"buffer": 0, "byteOffset": len(blob), "byteLength": len(data),
                      "target": 34963 if kind == "SCALAR" else 34962})
        blob.extend(data)
        acc = {"bufferView": len(views) - 1, "componentType": component, "count": len(values), "type": kind}
        if kind == "VEC3":
            acc.update(min=[min(row[i] for row in values) for i in range(width)],
                       max=[max(row[i] for row in values) for i in range(width)])
        accessors.append(acc)
        return len(accessors) - 1

    signed = lambda value: value - 256 if value >= 128 else value
    attributes = {
        "POSITION": accessor([[r[k] for k in ("X", "Y", "Z")] for r in records], "VEC3", "f", 5126),
        "NORMAL": accessor([[signed(r[k]) / 127 for k in ("R", "G", "B")] for r in records], "VEC3", "f", 5126),
        "TEXCOORD_0": accessor([[r[k] / 1024 for k in ("S", "T")] for r in records], "VEC2", "f", 5126),
    }
    materials, primitives = [], []
    for index, run in enumerate(runs):
        # Primitive colors are recovered; roughness is only an offline preview
        # approximation because the runtime display list has no PBR roughness.
        materials.append({"name": "Retained primitive color " + str(index), "doubleSided": False,
                          "pbrMetallicRoughness": {"baseColorFactor": [c / 255 for c in run["color"]],
                                                  "metallicFactor": 0, "roughnessFactor": .6}})
        primitives.append({"attributes": attributes, "indices": accessor([[i] for i in run["indices"]],
                           "SCALAR", "I", 5125), "material": index, "mode": 4})
    doc = {"asset": {"version": "2.0", "generator": "Mario SM64 POC3 retained display-list reconstruction"},
           "buffers": [{"byteLength": len(blob)}], "bufferViews": views, "accessors": accessors,
           "materials": materials, "meshes": [{"name": "Mario SM64 POC3", "primitives": primitives}],
           "nodes": [{"name": "Mario Mask SM64 POC3", "mesh": 0, "scale": [scale] * 3}],
           "scenes": [{"nodes": [0]}], "scene": 0,
           "extras": {"source": "tools/nei_gi/SOURCE/mario_sm64_poc3/serialized_source.o2r",
                      "authoring_recipe_recovered": False, "runtime_tested_in_recovery": False}}
    encoded = json.dumps(doc, separators=(",", ":")).encode()
    encoded += b" " * (-len(encoded) % 4)
    blob.extend(b"\0" * (-len(blob) % 4))
    body = struct.pack("<I4s", len(encoded), b"JSON") + encoded + struct.pack("<I4s", len(blob), b"BIN\0") + blob
    return struct.pack("<4sII", b"glTF", 2, len(body) + 12) + body


def restore_icon():
    """Return the exact retained icon pixels and truthful optional-pack provenance."""
    meta, resources = retained_resources()
    raw = resources[ICON]
    assert struct.unpack_from("<IIIIffI", raw, 64) == (1, 512, 512, 1, 16., 16., 512 * 512 * 4)
    assert len(raw) == 92 + 512 * 512 * 4
    image = Image.frombytes("RGBA", (512, 512), raw[92:])
    provenance = {"title": "Mario Mask", "method": "lossless pixels from retained Mario SM64 POC3 OTEX1 source",
                  "source": "ComboShip_Mario_SM64_POC3", "camera": None, "triangles": meta["triangles"],
                  "approximations": ["Original icon camera/lighting and procedural authoring recipe unavailable; pixels preserved exactly."],
                  "dependencies": {p.relative_to(ROOT).as_posix(): sha(p.read_bytes())
                                   for p in (ARCHIVE, SOURCE / "source.json", Path(__file__))}}
    # Match the icon pipeline's sorted provenance serialization so a rebuild
    # after --render --manifest remains byte-idempotent as well as pixel-exact.
    return image, json.loads(json.dumps(provenance, sort_keys=True))


def rebuild(install=False):
    meta, resources = retained_resources()
    records, runs, scale, loads = geometry(resources)
    triangles = sum(len(run["indices"]) // 3 for run in runs)
    assert (len(records), loads, triangles) == (meta["vertex_records"], meta["vertex_loads"], meta["triangles"])
    for name, data in resources.items():
        if name.startswith(PREFIX):
            destinations = [ROOT / "tools/nei_gi/RESOURCES" / name]
            if install:
                destinations.append(ROOT / "soh/assets/custom" / name)
            for path in destinations:
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
    checkpoint = ROOT / "tools/nei_gi/CHECKPOINTS/mario_mask"
    checkpoint.mkdir(parents=True, exist_ok=True)
    (checkpoint / "mario_mask.glb").write_bytes(glb_bytes(records, runs, scale))
    info = {"resource_count": 3, "vertex_records": len(records), "vertex_loads": loads,
            "slug": "mario_mask", "name": "Mario Mask SM64 POC3", "entry": PREFIX + "gi_dl",
            "draw_scale": 1., "matrix_scale": scale, "triangles": triangles,
            "bounds_author": [[op(r[k] for r in records) / 16 for k in ("X", "Y", "Z")] for op in (min, max)],
            "markers": {}, "notes": ["Exact Mario SM64 POC3 runtime geometry/material colors, recovered from retained serialized source.",
                                        "Original procedural authoring recipe unavailable; inspection GLB reconstructs display-list triangles.",
                                        "Runtime GI resources and optional icon are restored verbatim; no new mesh authored."],
            "source_archive_sha256": meta["archive_sha256"], "runtime_tested": False}
    (checkpoint / "checkpoint.json").write_text(json.dumps(info, indent=2) + "\n")
    sys.path.insert(0, str(ROOT / "tools/nei_icons"))
    from render import glb_model, render
    model = glb_model(checkpoint / "mario_mask.glb")
    for name, angle in (("front", 25), ("back", 150)):
        render(model, size=480, azimuth=angle, elevation=12).convert("RGB").save(checkpoint / (name + ".png"))
    icon, provenance = restore_icon()
    icon.save(ROOT / "tools/nei_icons/PNGS/gItemIconMarioMaskTex.png", optimize=True)
    if install:
        native = icon.resize((32, 32), Image.Resampling.LANCZOS)
        for host in ("soh", "mm"):
            target = ROOT / host / "assets/custom" / ICON
            assert not target.exists(), (target, "duplicate raw/.png icon source")
            native.save(target.with_name(target.name + ".rgba32.png"), optimize=True)
    path = ROOT / "tools/nei_icons/render_sources.json"
    sources = json.loads(path.read_text())
    sources["MarioMask"] = provenance
    path.write_text(json.dumps(sources, indent=2, sort_keys=True) + "\n")
    # Update only Mario's manifest entry; unrelated icon records are preserved.
    path = ROOT / "tools/nei_icons/manifest.json"
    manifest = json.loads(path.read_text())
    for entry in manifest["icons"]:
        if entry["name"] == "MarioMask":
            entry.update(png_sha256=sha((ROOT / entry["png"]).read_bytes()), resource_sha256=sha(resources[ICON]), source=provenance)
    path.write_text(json.dumps(manifest, indent=2) + "\n")
    verify(install)
    print(f"restored Mario SM64 POC3: {triangles} triangles, {loads} vertex loads; exact GI/icon source bytes")


def verify(installed=True):
    meta, resources = retained_resources()
    target = ROOT / ("soh/assets/custom" if installed else "tools/nei_gi/RESOURCES")
    for name, data in resources.items():
        if name.startswith(PREFIX):
            assert (target / name).read_bytes() == data, (name, "changed Mario runtime resource")
    sys.path.insert(0, str(ROOT / "tools/nei_gi"))
    from verify_assets import verify as verify_geometry
    verify_geometry(("mario_mask",), assets=target)
    records, runs, scale, loads = geometry(resources)
    assert (ROOT / "tools/nei_gi/CHECKPOINTS/mario_mask/mario_mask.glb").read_bytes() == glb_bytes(records, runs, scale), "stale reconstructed Mario GLB/materials"
    sys.path.insert(0, str(ROOT / "tools/nei_icons"))
    from build import texture_resource
    assert texture_resource(Image.open(ROOT / "tools/nei_icons/PNGS/gItemIconMarioMaskTex.png")) == resources[ICON], "changed retained Mario icon"
    if installed:
        icon, _ = restore_icon()
        native = icon.resize((32, 32), Image.Resampling.LANCZOS)
        files = []
        for host in ("soh", "mm"):
            target = ROOT / host / "assets/custom" / ICON
            assert not target.exists(), (target, "duplicate raw/.png icon source")
            path = target.with_name(target.name + ".rgba32.png")
            image = Image.open(path)
            assert image.mode == "RGBA" and image.size == (32, 32)
            assert image.tobytes() == native.tobytes(), (path, "changed native icon downsample")
            files.append(path.read_bytes())
        assert files[0] == files[1], "native Mario icon host mismatch"
    print("verified retained Mario source/archive parity, geometry/winding/cache/matrix, GLB normals/UVs/colors, exact OTEX1 icon")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--install", action="store_true")
    parser.add_argument("--verify", action="store_true", help="verify installed assets without writing outputs")
    args = parser.parse_args()
    if args.verify:
        verify()
    else:
        rebuild(args.install)
