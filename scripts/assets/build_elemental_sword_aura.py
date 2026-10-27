"""Build pack-independent blade auras from immutable Din art and shipped swords.

The original Din fire material, scroll, and sword_meshes recipe are retained.
The tiny I8 source maps and original Din layer records are retained beside their
provenance, so normal regeneration never needs the external equipment archive.
"""
from pathlib import Path
import argparse
import hashlib
import json
import math
import struct
import sys
import xml.etree.ElementTree as ET
import zipfile
import zlib

from PIL import Image
import numpy as np

from build_din_fire_weapons import sword_meshes, read_vertices
from build_din_fire_shield import crc64

ROOT = Path(__file__).resolve().parents[2]
INPUT = ROOT / "tools/elemental_sword_aura"
OUTPUT = ROOT / "combo/render/sword_element_aura_data.h"
PACK_SHA = "5f7972b03e494aa3c309860dc2e91d85ab3bbe18c61244c06d063f8205e27306"
MODERN_DIN_SHA = "ce1b8bfbfe9e1b2cf4a51355ea62c16826f60e8a65f07945d33a628b724cc781"
ASSETS = ROOT / "soh/assets/custom"
sys.path.insert(0, str(ROOT / "tools/nei_held"))
from build_swords import SWORDS, FRAMES, decode_matrix, source_folder

# Native authored blade surfaces only: no grip, pommel, sheath, or GI effects.
MODELS = {
    "kokiri_sword": (-96, {"edge_tex", "steel_tex"}),
    "mm_kokiri_sword": (-96, {"edge_tex", "steel_tex"}),
    "razor_sword": (-360, {"steel_tex", "blade_edge_tex"}),
    "gilded_sword": (0, {"tp_blade_tex"}),
    "master_sword": (0, {"tp_blade_tex"}),
    "true_master_sword": (0, {"tp_blade_tex"}),
    "biggoron_sword": (0, {"ordon_steel_tex"}),
    "great_fairy_sword": (-104, {"rose_edge_tex", "rose_tex", "violet_edge_tex", "violet_tex",
                                  "jade_tex", "jade_edge_tex", "gold_edge_tex"}),
}
KNIFE_MODELS = {
    "giants_knife": (0, {"ordon_steel_tex"}),
    "broken_giants_knife": (0, {"ordon_steel_tex"}),
}
NATIVE_SOURCES = {
    "oot": ("ea80d61b223ee38075fe5383b652998f3b08164e903134922f63f205d995717b", {
        "native_oot_master": ("objects/object_link_boy/gLinkAdultLeftHandHoldingMasterSwordNearDL", 852, True),
        "native_oot_kokiri": ("objects/object_link_child/gLinkChildLeftFistAndKokiriSwordNearDL", 480, True),
        "native_oot_bgs": ("objects/object_link_boy/gLinkAdultLeftHandHoldingBgsNearDL", 900, True),
        "native_oot_broken": ("objects/object_link_boy/gLinkAdultHandHoldingBrokenGiantsKnifeDL", 900, True),
        "native_oot_child_master": ("objects/object_link_child/gLinkChildLeftHandHoldingMasterSwordDL", -900, False),
    }),
    "mm": ("f10167e5682d74cc8da0137c524b1521f4e7ff47438b8888b63db6c7c6dc8d59", {
        "native_mm_kokiri": ("objects/gameplay_keep/gKokiriSwordBladeDL", 426, True),
        "native_mm_razor": ("objects/gameplay_keep/gRazorSwordBladeDL", 326, True),
        "native_mm_gilded": ("objects/object_link_child/gLinkHumanGildedSwordBladeDL", 426, True),
        "native_mm_fairy": ("objects/object_link_child/gLinkHumanGreatFairysSwordDL", 1039, True),
    }),
}

def sha(data): return hashlib.sha256(data).hexdigest()

def import_modern_din(path):
    """Retain only the original pack's two actual longsword blade surfaces.

    These leaf DLs already use the held-hand basis, with no matrices. Do not
    infer their fit from the outer native/CustomEquipment alias or its hilt.
    """
    assert sha(path.read_bytes()) == MODERN_DIN_SHA, "Use the preserved original adult Din equipment"
    result = {}
    metadata = {"archive_sha256": MODERN_DIN_SHA, "profiles": {}}
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None
        hashes = {crc64(name): name for name in archive.namelist()}
        for profile, leaf in (("modern_din_bgs", "BLADE"), ("modern_din_broken", "BROKEN")):
            root = "alt/objects/object_custom_equip/ModernDinPOC2_LONGSWORD_" + leaf
            data = archive.read(root)
            dependencies = {root: sha(data)}
            cache, triangles = {}, []
            offset = 72
            while offset < len(data):
                w0, w1 = struct.unpack_from("<II", data, offset)
                offset += 8
                op = w0 >> 24
                if op in (0x20, 0x31, 0x32, 0x33, 0x36, 0x37, 0x39):
                    hi, lo = struct.unpack_from("<II", data, offset)
                    offset += 8
                    resource = hashes[(hi << 32) | lo]
                if op == 0x32:
                    raw = archive.read(resource)
                    dependencies[resource] = sha(raw)
                    kind, total = struct.unpack_from("<II", raw, 64)
                    assert kind == 25 and len(raw) == 72 + total * 16
                    count = (w0 >> 12) & 255
                    first = ((w0 >> 1) & 127) - count
                    assert w1 % 16 == 0 and 0 <= first < first + count <= 32
                    assert 0 <= w1 // 16 < w1 // 16 + count <= total
                    for i in range(count):
                        cache[first + i] = struct.unpack_from("<hhhHhhBBBB", raw, 72 + w1 + i * 16)
                elif op in (0x05, 0x06):
                    for packed in ([w0, w1] if op == 0x06 else [w0]):
                        triangles.extend(cache[((packed >> shift) & 255) // 2] for shift in (16, 8, 0))
                elif op in (0x31, 0x36, 0xDA, 0xD8):
                    raise AssertionError(("Unexpected nested ModernDin blade/transform", root, hex(op)))
                elif op == 0xDF:
                    break
            assert len(triangles) == 172 * 3
            result[profile] = triangles
            metadata["profiles"][profile] = {"root": root, "root_crc64": "%016X" % crc64(root),
                "source_sha256": dependencies, "embedded_matrices": 0,
                "blade_bounds": [[min(v[k] for v in triangles), max(v[k] for v in triangles)] for k in range(3)]}
        metadata["equipment_aliases"] = {}
        for name in ("gCustomLongswordDL", "gCustomBreakableLongswordDL", "gCustomBrokenLongswordDL",
                     "gLinkAdultLeftHandHoldingBgsNearDL", "gLinkAdultLeftHandHoldingBgsFarDL",
                     "gLinkAdultHandHoldingBrokenGiantsKnifeDL", "gLinkAdultHandHoldingBrokenGiantsKnifeFarDL"):
            folder = "object_custom_equip" if name.startswith("gCustom") else "object_link_boy"
            resource = "alt/objects/" + folder + "/" + name
            data = archive.read(resource)
            metadata["equipment_aliases"][name] = {"resource": resource, "sha256": sha(data),
                "commands": [struct.unpack_from("<II", data, i) for i in range(72, len(data), 8)]}
    encoded = zlib.compress(json.dumps(result, separators=(",", ":")).encode(), 9)
    (INPUT / "modern_din_blades.json.zlib").write_bytes(encoded)
    metadata["records_sha256"] = sha(encoded)
    (INPUT / "modern_din_source.json").write_text(json.dumps(metadata, indent=2) + "\n")

def import_fire(path):
    assert sha(path.read_bytes()) == PACK_SHA, "Use the preserved accepted Din progressive candidate"
    INPUT.mkdir(parents=True, exist_ok=True)
    meta = {"archive_sha256": PACK_SHA, "texture_size": [64, 32], "resample": "Pillow LANCZOS, existing I8 red channel",
            "protected_artwork_regenerated": False, "resources": {}}
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None
        old = json.loads(archive.read("DinFireWeaponsPOC1.json"))
        meta["original_artwork"] = old["texture_sources"]
        for name in ("Core", "Flame"):
            resource = "objects/din_fire_sword/poc1/" + name + "Tex"
            data = archive.read(resource)
            fmt, width, height, flags, sx, sy, size = struct.unpack_from("<IIIIffI", data, 64)
            assert (fmt, width, height, flags, sx, sy, size) == (6, 1024, 1024, 3, 1, 1, 1024 * 1024 * 4)
            image = Image.frombytes("RGBA", (width, height), data[92:])
            channels = image.split()
            assert all(channel.tobytes() == channels[0].tobytes() for channel in channels)
            pixels = channels[0].resize((64, 32), Image.Resampling.LANCZOS).tobytes()
            (INPUT / (name.lower() + ".i8")).write_bytes(pixels)
            meta["resources"][resource] = {"sha256": sha(data), "fallback_sha256": sha(pixels)}
        layers = {}
        for profile in ("adult", "child", "bgs", "broken"):
            layers[profile] = {}
            for name in ("Core", "Flame"):
                resource = "objects/din_fire_sword/poc1/" + profile + "/" + name + "Vertices"
                data = archive.read(resource)
                layers[profile][name.lower()] = read_vertices(data)
                meta["resources"][resource] = {"sha256": sha(data)}
        encoded = zlib.compress(json.dumps(layers, separators=(",", ":")).encode(), 9)
        (INPUT / "din_layers.json.zlib").write_bytes(encoded)
        meta["layer_records_sha256"] = sha(encoded)
        meta["equipment_aliases"] = {}
        for profile, family in (("adult", "MasterSword"), ("child", "KokiriSword"),
                                ("bgs", "Longsword"), ("broken", "BrokenLongsword")):
            name = "alt/objects/object_custom_equip/gCustom" + family + "DL"
            data = archive.read(name)
            meta["equipment_aliases"][profile] = {"resource": name, "sha256": sha(data),
                "commands": [struct.unpack_from("<II", data, i) for i in range(72, len(data), 8)]}
    (INPUT / "fire_source.json").write_text(json.dumps(meta, indent=2) + "\n")

def import_native(game, path):
    expected, sources = NATIVE_SOURCES[game]
    assert sha(path.read_bytes()) == expected
    result, metadata = {}, {"archive_sha256": expected, "profiles": {}}
    with zipfile.ZipFile(path) as archive:
        assert archive.testzip() is None
        hashes = {crc64(name): name for name in archive.namelist()}
        for model, (root, cutoff, above) in sources.items():
            cache, triangles, dependencies = {}, [], {}
            def read(name):
                data = archive.read(name)
                dependencies[name] = sha(data)
                return data
            def walk(name, depth=0):
                assert depth < 20
                data = read(name)
                offset = 72  # OTRv0 DL ucode byte and padding, after the 64-byte resource header.
                while offset < len(data):
                    w0, w1 = struct.unpack_from("<II", data, offset)
                    offset += 8
                    op = w0 >> 24
                    if op in (0x20, 0x31, 0x32, 0x33, 0x36, 0x37, 0x39):
                        hi, lo = struct.unpack_from("<II", data, offset)
                        offset += 8
                        resource = (hi << 32) | lo
                    if op == 0x31:
                        walk(hashes[resource], depth + 1)
                    elif op == 0x32:
                        vertices = read(hashes[resource])
                        kind, total = struct.unpack_from("<II", vertices, 64)
                        assert kind == 25 and len(vertices) == 72 + total * 16
                        count = (w0 >> 12) & 255
                        first = ((w0 >> 1) & 127) - count
                        assert 0 <= first < first + count <= 32
                        for i in range(count):
                            cache[first + i] = np.array(struct.unpack_from("<hhh", vertices, 72 + w1 + i * 16), float)
                    elif op in (0x05, 0x06):
                        for packed in ([w0, w1] if op == 0x06 else [w0]):
                            polygon = [cache[((packed >> shift) & 255) // 2] for shift in (16, 8, 0)]
                            polygon = clip_polygon(polygon, 0, cutoff, above)
                            for i in range(1, len(polygon) - 1):
                                tri = [polygon[0], polygon[i], polygon[i + 1]]
                                normal = np.cross(tri[1] - tri[0], tri[2] - tri[0])
                                length = np.linalg.norm(normal)
                                if length < .01: continue
                                normal = np.rint(normal * (127 / length)).astype(int)
                                triangles.extend((*[round(float(v)) for v in point], 0, 0, 0,
                                                  *[int(v) & 255 for v in normal], 255) for point in tri)
                    elif op in (0x36, 0xDA):
                        raise AssertionError(("Unexpected native matrix; do not guess its fit", name))
                    elif op == 0xDF:
                        return
                raise AssertionError(("Missing native EndDL", name))
            walk(root)
            assert len(triangles) >= 12 and len(triangles) % 3 == 0
            result[model] = triangles
            metadata["profiles"][model] = {"root": root, "blade_x_clip": [cutoff, above],
                "source_sha256": dependencies, "blade_bounds": [[min(v[k] for v in triangles), max(v[k] for v in triangles)] for k in range(3)]}
    encoded = zlib.compress(json.dumps(result, separators=(",", ":")).encode(), 9)
    (INPUT / (game + "_native.json.zlib")).write_bytes(encoded)
    metadata["records_sha256"] = sha(encoded)
    (INPUT / (game + "_native_source.json")).write_text(json.dumps(metadata, indent=2) + "\n")

def clip_polygon(points, axis, cut, above):
    result = []
    for before, after in zip(points, points[1:] + points[:1]):
        inside_a = before[axis] >= cut if above else before[axis] <= cut
        inside_b = after[axis] >= cut if above else after[axis] <= cut
        if inside_a: result.append(before)
        if inside_a != inside_b:
            fraction = (cut - before[axis]) / (after[axis] - before[axis])
            result.append(before + fraction * (after - before))
    return result

def blade(slug, frame, broken=False):
    folder = ASSETS / source_folder(slug)
    records = [v.attrib for v in ET.parse(folder / "mesh_opa_vtx").getroot()]
    matrix = decode_matrix((folder / "scale_mtx").read_bytes()) @ decode_matrix(
        (ASSETS / "objects/nei_held_swords" / slug / frame / "grip_mtx").read_bytes())
    base, materials = MODELS[slug] if slug in MODELS else KNIFE_MODELS[slug]
    cutoff = float((np.array([0, base, 0, 1]) @ matrix)[0])
    current, cache, triangles = "", {}, []
    for command in ET.parse(folder / "gi_dl").getroot():
        if command.tag == "LoadTextureBlock": current = Path(command.get("Path")).name
        if command.tag == "LoadVertices":
            start, count = int(command.get("VertexOffset")), int(command.get("Count"))
            v0 = int(command.get("VertexBufferIndex", 0))
            for i, vertex in enumerate(records[start:start + count]):
                cache[v0 + i] = (np.array([int(vertex[k]) for k in ("X", "Y", "Z")] + [1]) @ matrix)[:3]
        if current not in materials: continue
        groups = []
        if command.tag == "Triangles2": groups = [("V00", "V01", "V02"), ("V10", "V11", "V12")]
        if command.tag == "Triangle1": groups = [("V00", "V01", "V02")]
        for group in groups:
            polygon = [cache[int(command.get(k, command.get(k.replace("V0", "V"))))] for k in group]
            polygon = clip_polygon(polygon, 0, cutoff, True)
            if broken and polygon: polygon = clip_polygon(polygon, 0, 2500, False)
            for index in range(1, len(polygon) - 1):
                tri = [polygon[0], polygon[index], polygon[index + 1]]
                normal = np.cross(tri[1] - tri[0], tri[2] - tri[0])
                length = np.linalg.norm(normal)
                if length < .01: continue
                normal = np.rint(normal * (127 / length)).astype(int)
                for point in tri:
                    triangles.append((*[round(float(v)) for v in point], 0, 0, 0,
                                      *[int(v) & 255 for v in normal], 255))
    assert len(triangles) >= 30 and len(triangles) % 3 == 0, (slug, frame)
    return triangles

def tight_shell(records):
    # The Din cross-section construction, with a narrow expansion for frost and
    # shimmer. The authored blade's center/width/thickness are fitted per ring.
    low, high = min(v[0] for v in records), max(v[0] for v in records)
    length = high - low
    rings = []
    for i in range(17):
        t = i / 16
        x = low + length * t
        points = []
        for start in range(0, len(records), 3):
            a, b, c = records[start:start + 3]
            for a, b in ((a, b), (b, c), (c, a)):
                if a[0] == b[0]:
                    if abs(x - a[0]) < 1e-6: points.extend((a[1:3], b[1:3]))
                elif min(a[0], b[0]) <= x <= max(a[0], b[0]):
                    f = (x - a[0]) / (b[0] - a[0])
                    points.append((a[1] + f * (b[1] - a[1]), a[2] + f * (b[2] - a[2])))
        assert points
        ys, zs = zip(*points)
        cy, cz = (min(ys) + max(ys)) / 2, (min(zs) + max(zs)) / 2
        envelope = max(0, math.sin(math.pi * t)) ** .35
        ry = (max(ys) - min(ys)) * .54 + length * .004 * envelope + 4
        rz = (max(zs) - min(zs)) * .55 + length * .002 * envelope + 4
        ring = []
        for side in range(13):
            angle = side * math.tau / 12
            ring.append((round(x + length * .006 * t**3), round(cy + ry * math.cos(angle)),
                         round(cz + rz * math.sin(angle)), 0, round(side / 12 * 2048), round(t * 1024),
                         255, 255, 255, round(150 * min(1, t / .1) * min(1, (1 - t) / .1))))
        rings.append(ring)
    result = []
    for i in range(16):
        for side in range(12):
            a, b, c, d = rings[i][side], rings[i][side + 1], rings[i + 1][side], rings[i + 1][side + 1]
            result.extend((a, b, c, b, d, c))
    return result

def emit_mesh(name, records):
    batches, vertices, indices = [], [], []
    for start in range(0, len(records), 3):
        triangle = [tuple(v) for v in records[start:start + 3]]
        extra = [v for v in triangle if v not in vertices]
        if len(set(vertices + extra)) > 30:
            batches.append((vertices, indices)); vertices, indices = [], []
        local = []
        for vertex in triangle:
            if vertex not in vertices: vertices.append(vertex)
            local.append(vertices.index(vertex))
        indices.append(local)
    if vertices: batches.append((vertices, indices))
    text = []
    for index, (vertices, _) in enumerate(batches):
        text.append(f"static const Vtx {name}V{index}[] = {{")
        for v in vertices:
            x, y, z, flag, s, t, r, g, b, a = v
            text.append(f"    {{{{{{ {x}, {y}, {z} }}, {flag}, {{ {s}, {t} }}, {{ {r}, {g}, {b}, {a} }}}}}},")
        text.append("};")
    text.append(f"static const Gfx {name}[] = {{")
    for index, (vertices, indices) in enumerate(batches):
        text.append(f"    gsSPVertex({name}V{index}, {len(vertices)}, 0),")
        for tri in indices: text.append(f"    gsSP1Triangle({tri[0]}, {tri[1]}, {tri[2]}, 0),")
    text += ["    gsSPEndDisplayList(),", "};"]
    return "\n".join(text)

def texture_array(name, pixels):
    assert len(pixels) == 2048
    rows = [f"static const ALIGN_ASSET(8) u8 {name}[2048] = {{"]
    rows += ["    " + ", ".join(str(v) for v in pixels[i:i + 32]) + "," for i in range(0, len(pixels), 32)]
    return "\n".join(rows + ["};"])

def build():
    meta = json.loads((INPUT / "fire_source.json").read_text())
    assert meta["archive_sha256"] == PACK_SHA
    text = ["// Generated by scripts/assets/build_elemental_sword_aura.py; do not edit.", "// clang-format off",
            "#ifndef SWORD_ELEMENT_AURA_DATA_H", "#define SWORD_ELEMENT_AURA_DATA_H", "#include \"align_asset_macro.h\"", "#include \"sword_element_aura_profiles.h\"",
            "typedef struct { const Gfx* core; const Gfx* flame; const Gfx* tight; } SwordAuraGeometry;"]
    for name in ("core", "flame"):
        pixels = (INPUT / (name + ".i8")).read_bytes()
        resource = "objects/din_fire_sword/poc1/" + name.capitalize() + "Tex"
        assert sha(pixels) == meta["resources"][resource]["fallback_sha256"]
        text.append(texture_array("sSwordAuraFire" + name.capitalize() + "Tex", pixels))
    frost, light = [], []
    for y in range(32):
        for x in range(64):
            # Broader veins/rays and a lifted floor survive distance filtering.
            # Preserve the existing motifs, UVs, blade fit and 64x32 I8 budget.
            frost.append(round(48 + 120 * max(0, math.cos(x * .40 + math.sin(y * .61)))**8 +
                               87 * max(0, math.cos(x * .17 - y * .73))**12))
            light.append(round(48 + 125 * max(0, math.cos(x * .21 + y * .31))**6 +
                               82 * max(0, math.cos(x * .69 - y * .34))**12))
    text += [texture_array("sSwordAuraIceTex", frost), texture_array("sSwordAuraLightTex", light)]
    models = [*MODELS, "broken_knife"]
    table = []
    fits = {}
    for model in models:
        frames = []
        for frame in FRAMES:
            source = "biggoron_sword" if model == "broken_knife" else model
            records = blade(source, frame, model == "broken_knife")
            core, flame = sword_meshes(records)
            tight = tight_shell(records)
            prefix = "sSwordAura_" + model + "_" + frame
            for suffix, mesh in (("Core", core), ("Flame", flame), ("Tight", tight)):
                text.append(emit_mesh(prefix + suffix, mesh))
            frames.append("{ " + ", ".join(prefix + suffix for suffix in ("Core", "Flame", "Tight")) + " }")
            fits[model + "/" + frame] = {"source": source, "source_vertices_sha256": sha((ASSETS / "objects/nei_gi_redesign" / source / "mesh_opa_vtx").read_bytes()),
                "held_matrix_sha256": sha((ASSETS / "objects/nei_held_swords" / source / frame / "grip_mtx").read_bytes()),
                "blade_bounds": [[min(v[k] for v in records), max(v[k] for v in records)] for k in range(3)]}
        table.append("    { " + ", ".join(frames) + " },")
    native = {}
    for game in NATIVE_SOURCES:
        metadata = json.loads((INPUT / (game + "_native_source.json")).read_text())
        encoded = (INPUT / (game + "_native.json.zlib")).read_bytes()
        assert sha(encoded) == metadata["records_sha256"]
        native[game] = metadata
        for model, records in json.loads(zlib.decompress(encoded)).items():
            core, flame = sword_meshes(records)
            tight = tight_shell(records)
            prefix = "sSwordAura_" + model
            for suffix, mesh in (("Core", core), ("Flame", flame), ("Tight", tight)):
                text.append(emit_mesh(prefix + suffix, mesh))
            entry = "{ " + ", ".join(prefix + suffix for suffix in ("Core", "Flame", "Tight")) + " }"
            table.append("    { " + ", ".join([entry] * 3) + " },")
    encoded = (INPUT / "din_layers.json.zlib").read_bytes()
    assert sha(encoded) == meta["layer_records_sha256"]
    layers = json.loads(zlib.decompress(encoded))
    assert len(layers) == 4
    for name, layer in layers.items():
        prefix = "sSwordAura_din_" + name
        for kind in ("core", "flame"):
            text.append(emit_mesh(prefix + kind, layer[kind]))
        text.append(emit_mesh(prefix + "tight", tight_shell(layer["core"])))
        entry = "{ " + ", ".join(prefix + kind for kind in ("core", "flame", "tight")) + " }"
        table.append("    { " + ", ".join([entry] * 3) + " },")
    modern = json.loads((INPUT / "modern_din_source.json").read_text())
    assert modern["archive_sha256"] == MODERN_DIN_SHA
    encoded = (INPUT / "modern_din_blades.json.zlib").read_bytes()
    assert sha(encoded) == modern["records_sha256"]
    modern_records = json.loads(zlib.decompress(encoded))
    assert set(modern_records) == {"modern_din_bgs", "modern_din_broken"}
    for model in ("modern_din_bgs", "modern_din_broken"):
        records = modern_records[model]
        core, flame = sword_meshes(records)
        prefix = "sSwordAura_" + model
        for suffix, mesh in (("Core", core), ("Flame", flame), ("Tight", tight_shell(records))):
            text.append(emit_mesh(prefix + suffix, mesh))
        entry = "{ " + ", ".join(prefix + suffix for suffix in ("Core", "Flame", "Tight")) + " }"
        table.append("    { " + ", ".join([entry] * 3) + " },")
    # Appended identities keep every existing authored/native/Din profile and
    # its generated bytes stable. These fits consume the actual Knife meshes,
    # including the closed fracture, rather than clipping a Biggoron surrogate.
    for model in KNIFE_MODELS:
        frames = []
        for frame in FRAMES:
            records = blade(model, frame)
            core, flame = sword_meshes(records)
            prefix = "sSwordAura_" + model + "_" + frame
            for suffix, mesh in (("Core", core), ("Flame", flame), ("Tight", tight_shell(records))):
                text.append(emit_mesh(prefix + suffix, mesh))
            frames.append("{ " + ", ".join(prefix + suffix for suffix in ("Core", "Flame", "Tight")) + " }")
            fits[model + "/" + frame] = {
                "source": model,
                "source_vertices_sha256": sha((ASSETS / source_folder(model) / "mesh_opa_vtx").read_bytes()),
                "held_matrix_sha256": sha((ASSETS / "objects/nei_held_swords" / model / frame / "grip_mtx").read_bytes()),
                "blade_bounds": [[min(v[k] for v in records), max(v[k] for v in records)] for k in range(3)]}
        table.append("    { " + ", ".join(frames) + " },")
    text += ["static const SwordAuraGeometry sSwordAuraGeometry[SWORD_AURA_MODEL_COUNT][3] = {", *table, "};", "#endif", "// clang-format on", ""]
    return "\n".join(text), json.dumps({"fire_source": meta, "fits": fits, "native": native, "modern_din": modern,
        "fire_geometry_recipe": "build_din_fire_weapons.sword_meshes unchanged", "ice_light": "translucent blade-only cross-section envelope; procedural 64x32 I8 frost/shimmer",
        "runtime_tested": False}, indent=2) + "\n"

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--import-fire", type=Path)
    parser.add_argument("--import-oot", type=Path)
    parser.add_argument("--import-mm", type=Path)
    parser.add_argument("--import-modern-din", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.import_fire: import_fire(args.import_fire)
    if args.import_oot: import_native("oot", args.import_oot)
    if args.import_mm: import_native("mm", args.import_mm)
    if args.import_modern_din: import_modern_din(args.import_modern_din)
    header, manifest = build()
    if args.check:
        assert OUTPUT.read_text() == header
        assert (INPUT / "manifest.json").read_text() == manifest
        print("PASS deterministic aura geometry, protected fire maps and source-fit identities")
    else:
        OUTPUT.parent.mkdir(parents=True, exist_ok=True)
        OUTPUT.write_text(header)
        (INPUT / "manifest.json").write_text(manifest)
        print("Generated", OUTPUT, len(header), "bytes")

if __name__ == "__main__": main()
