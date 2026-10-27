#!/usr/bin/env python3
"""Build a separate TP POC4 cosmetic candidate from the immutable POC3 archive.

Only the body/head limb display-list path strings change. Original skin records,
vertices, UVs, normals, skeleton, animations, textures and POC3 display lists stay
intact. New lists add optional GPU overlays before the exact final eye preload,
and tint only the iris triangles. Fixed alpha masks never change at runtime.

Requires the supplied POC3 checkpoint directory (NumPy/Pillow tools and source
OBJ), not a game ROM. The source checksum pins the approved appearance and rig.
Offline preview/structural checks are separate from live renderer acceptance.
"""

import argparse
import hashlib
import importlib
import json
from pathlib import Path
import struct
import sys
import zipfile

import numpy as np
from PIL import Image, ImageDraw

BASE_SHA = "9fe83f6b7a11b0eaf8df36ac9a36c7dd272857cdab1efd0eb628bddc0f40f5a3"
OLD = "objects/object_horse/tp_poc2/"
NEW = "objects/object_horse/tp_poc4_cosmetics/"
BODY_LIMB = "alt/objects/object_horse/gEponaBodyLimb"
HEAD_LIMB = "alt/objects/object_horse/gEponaHeadLimb"
OPA, DECAL, GEOMETRY = 0xC8112078, 0xC8104DD8, 0x00230405
END = struct.pack("<II", 0xDF000000, 0)
EYE_PRELOAD = struct.pack("<II", 0x01014028, 0x08000001 + 5892 * 16)

# Authored semantic support in the accepted 1254px chestnut atlas. These regions
# exclude the saddle blanket, bridle, buckles, straps, horseshoe and hoof toe.
# Pixel chroma is used only INSIDE the three fur regions to follow white-hair
# strands at the brown/white boundary; it never classifies tack across the atlas.
FUR_REGIONS = (
    ((0, 0), (593, 0), (610, 81), (642, 129), (665, 137), (681, 237),
     (653, 367), (644, 427), (616, 453), (599, 494), (592, 580),
     (624, 653), (675, 703), (711, 754), (719, 791), (702, 834),
     (699, 870), (649, 913), (617, 971), (617, 1188), (346, 1188),
     (342, 660), (210, 657), (190, 632), (160, 582), (136, 548),
     (107, 505), (63, 470), (0, 439)),
    ((639, 137), (726, 141), (848, 135), (912, 133), (912, 363),
     (966, 370), (918, 407), (868, 429), (818, 442),
     (814, 378), (738, 371), (716, 365), (656, 368)),
    ((813, 449), (937, 426), (979, 373), (1159, 373), (1159, 480),
     (1253, 480), (1253, 961), (1181, 961), (1181, 989), (844, 992),
     (756, 963), (756, 867), (798, 860), (799, 798), (748, 781),
     (699, 747), (632, 710), (602, 651), (589, 601), (596, 536),
     (627, 496), (659, 464), (662, 450)),
)
HAIR_REGIONS = (
    ((620, 130), (981, 125), (981, 456), (646, 456)),
    ((814, 366), (1253, 366), (1253, 996), (931, 996), (973, 549)),
    ((341, 960), (622, 960), (622, 1190), (341, 1190)),
)
PROTECTED_TACK = (
    ((631, 903), (696, 866), (723, 930), (652, 969)),
    ((914, 214), (978, 214), (978, 366), (914, 366)),
)
TACK_PROBES = ((60, 740), (182, 913), (175, 1104), (1150, 250),
               (937, 94), (1196, 421), (753, 409), (756, 827),
               (691, 1054), (1000, 1100), (1206, 1110), (934, 300), (658, 911))


def require(value, message):
    if not value:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def helpers(checkpoint):
    path = Path(checkpoint).resolve()
    require((path / "o2r.py").is_file() and (path / "polish_mesh.py").is_file(), "missing POC3 checkpoint tools")
    sys.path.insert(0, str(path))
    return {n: importlib.import_module(n) for n in ("o2r", "native_roundtrip", "rig", "polish_mesh", "inspect_mesh")}


def commands(data):
    require(len(data) >= 80 and data[4:8] == b"TLDO" and data[64] == 4, "expected F3DEX2 display list")
    offset = 72
    while offset < len(data):
        require(offset + 8 <= len(data), "truncated display command")
        w0, w1 = struct.unpack_from("<II", data, offset)
        size = 16 if w0 >> 24 in (0x20, 0x31, 0x32, 0x33) else 8
        require(offset + size <= len(data), "truncated hash display command")
        yield w0, w1, data[offset:offset + size]
        offset += size
    require(data[-8:] == END, "missing terminal EndDL")


def segment(number):
    return struct.pack("<II", 0xDE000000, (number << 24) | 1)


def marker(path, h):
    return struct.pack("<IIII", 0x33000000, 0xBEEFBEEF, h["o2r"].crc64(path) >> 32,
                       h["o2r"].crc64(path) & 0xFFFFFFFF)


def polygon_mask(polygons, size):
    mask = Image.new("L", size)
    draw = ImageDraw.Draw(mask)
    for points in polygons:
        draw.polygon(points, fill=255)
    return np.asarray(mask, dtype=np.float32) / 255


def atlas_masks(rgba):
    require(rgba.shape == (1254, 1254, 4), "unexpected approved coat atlas dimensions")
    support = polygon_mask(FUR_REGIONS, (1254, 1254))
    support *= 1 - polygon_mask(PROTECTED_TACK, (1254, 1254))
    hair_support = polygon_mask(HAIR_REGIONS, (1254, 1254)) * support
    rgb = rgba[:, :, :3].astype(np.float32)
    # Warm ivory and its brown shadows are less red-saturated than chestnut.
    whiteness = 1 - np.maximum(rgb[:, :, 0] - rgb[:, :, 2], 0) / np.maximum(rgb.max(2), 1)
    factor = np.clip((whiteness - .20) / .35, 0, 1)
    hair = np.rint(255 * hair_support * factor * factor * (3 - 2 * factor)).astype(np.uint8)
    coat = np.rint(support * 255).astype(np.uint8) - hair
    for x, y in TACK_PROBES:
        require(coat[y, x] == 0 and hair[y, x] == 0, f"mask reaches protected tack probe {(x, y)}")
    require(np.count_nonzero(coat) > 200000 and np.count_nonzero(hair) > 50000, "incomplete fur masks")
    return coat, hair


def masked_texture(original, alpha, h):
    typ, width, height, raw = h["o2r"].read_texture(original)
    require(typ == 1 and len(raw) == width * height * 4 and alpha.shape == (height, width), "unexpected raw texture")
    image = np.frombuffer(raw, np.uint8).reshape(height, width, 4).copy()
    image[:, :, 3] = np.minimum(image[:, :, 3], alpha)
    # Keep all original texture metadata, image dimensions/scales and RGB.
    return original[:-len(raw)] + image.tobytes()


def source_selection(checkpoint, h):
    v, uv, parts = h["rig"].read_obj(Path(checkpoint) / "inputs/tp-source/Horse.obj")
    v, parts = h["polish_mesh"].polish(v, parts, uv)
    require([len(p["faces"]) for p in parts] == [6, 6, 89, 1705, 104, 66], "unexpected polished source topology")
    selected = {}
    for material, wanted in ((3, (0,)), (2, (0,)), (4, (0, 1, 2))):
        faces = np.asarray(parts[material]["faces"])
        groups = h["inspect_mesh"].components(v, faces)
        selected[material] = set(int(i) for g in wanted for i in groups[g])
    require([len(selected[i]) for i in (3, 2, 4)] == [1164, 73, 72], "unexpected coat/mane/tail components")
    return selected


def body_sections(data):
    """Decode exact existing vertex loads/triangles, keeping their cache indices."""
    sections = []
    section = None
    pending_load = None
    for w0, w1, raw in commands(data):
        opcode = w0 >> 24
        if opcode == 0x20:
            section = {"triangles": [], "texture_hash": struct.unpack_from("<II", raw, 8)}
            sections.append(section)
        elif opcode == 1:
            pending_load = raw
        elif opcode == 5:
            require(section is not None and pending_load is not None, "triangle missing texture/vertices")
            section["triangles"].append((pending_load, raw))
    require([len(x["triangles"]) for x in sections] == [1705, 66, 89, 104], "unexpected POC3 body draw layout")
    return sections


def overlay_material(dl, texture, cutout):
    dl.raw(0xE7000000, 0)
    dl.raw(0xE3000A01, 0x00100000)
    dl.raw(0xE200001C, DECAL)  # fog + equal-depth translucent decal, no Z_UPD
    dl.raw(0xE2001E01, 0)
    dl.raw(0xE3001001, 0)
    dl.raw(0xD9000000, GEOMETRY & ~0x400 if cutout else GEOMETRY)
    dl.raw(0xD7000002, 0xFFFFFFFF)
    dl.raw(0xFA000000, 0xFFFFFFFF)
    dl.raw(0xFC127FFF, 0xFFFFF238)  # textured shaded RGB, texture alpha / PASS2
    dl.settimg_hash(0, 2, 1, texture)
    for w0, w1 in ((0xF5100000, 0x07014050), (0xE6000000, 0), (0xF3000000, 0x073FF100),
                    (0xE7000000, 0), (0xF5101000, 0x00014050), (0xF2000000, 0x0007C07C)):
        dl.raw(w0, w1)


def overlay(path, jobs, h):
    dl = h["o2r"].DLBuilder()
    dl.marker(path)
    for section, indices, texture, cutout in jobs:
        overlay_material(dl, texture, cutout)
        last_load = None
        for index, (load, tri) in enumerate(section["triangles"]):
            if index not in indices:
                continue
            if load != last_load:
                dl.raw(*struct.unpack("<II", load))
                last_load = load
            dl.raw(*struct.unpack("<II", tri))
    dl.raw(0xE7000000, 0)
    dl.raw(0xE200001C, OPA)
    dl.raw(0xD9000000, GEOMETRY)
    dl.end()
    return dl.to_resource()


def build_files(original, checkpoint, h):
    files = original.copy()
    old_body, old_eyes = original[OLD + "BodyDL"], original[OLD + "EyesDL"]
    require(old_body[-16:-8] == EYE_PRELOAD, "POC3 final 20-eye preload differs")
    selected = source_selection(checkpoint, h)
    sections = body_sections(old_body)
    typ, width, height, raw = h["o2r"].read_texture(original[OLD + "HorsSS00Tex"])
    coat_mask, hair_mask = atlas_masks(np.frombuffer(raw, np.uint8).reshape(height, width, 4))
    for name, mask in (("CoatMaskTex", coat_mask), ("WhiteHairMaskTex", hair_mask)):
        files[NEW + name] = masked_texture(original[OLD + "HorsSS00Tex"], mask, h)
    files[NEW + "CoatOverlayDL"] = overlay(NEW + "CoatOverlayDL", [
        (sections[0], selected[3], NEW + "CoatMaskTex", False)], h)
    files[NEW + "HairOverlayDL"] = overlay(NEW + "HairOverlayDL", [
        (sections[0], selected[3], NEW + "WhiteHairMaskTex", False),
        (sections[2], selected[2], OLD + "HorsMS05Tex", True),
        (sections[3], selected[4], OLD + "HorsSS08Tex", True)], h)
    # POC3's original finish is before the preload. The overlays restore its
    # opaque state and the dispatch lists clear grayscale before this load.
    new_body = old_body[:-16] + segment(0xD) + segment(0xE) + old_body[-16:]
    files[NEW + "BodyDL"] = new_body[:72] + marker(NEW + "BodyDL", h) + new_body[88:]
    # The first six triangles are the isolated iris. Do not tint the coat-colored
    # blink eyelids which use the original segment08 texture in material two.
    eye_commands = list(commands(old_eyes))
    triangle_count = 0
    out = bytearray(old_eyes[:72] + marker(NEW + "EyesDL", h) + segment(0xB))
    reset = False
    for w0, w1, command in eye_commands[1:]:
        if triangle_count == 6 and not reset:
            out.extend(segment(0xC))
            reset = True
        out.extend(command)
        triangle_count += w0 >> 24 == 5
    require(triangle_count == 12 and reset, "unexpected iris/blink layout")
    files[NEW + "EyesDL"] = bytes(out)
    for path, field, destination in ((BODY_LIMB, "skin_display_list2", NEW + "BodyDL"),
                                     (HEAD_LIMB, "skin_display_list", NEW + "EyesDL")):
        limb = h["native_roundtrip"].decode_resource(original[path])
        limb[field] = destination
        files[path] = h["native_roundtrip"].encode_resource(limb)
    return files, selected, {"coat": coat_mask, "hair": hair_mask}


def validate(original, files, h):
    require(set(files) - set(original) == {NEW + n for n in (
        "BodyDL", "EyesDL", "CoatOverlayDL", "HairOverlayDL", "CoatMaskTex", "WhiteHairMaskTex")}, "unexpected added resources")
    require({p for p in original if files.get(p) != original[p]} == {BODY_LIMB, HEAD_LIMB}, "unexpected baseline resource change")
    for path, field, destination in ((BODY_LIMB, "skin_display_list2", NEW + "BodyDL"),
                                     (HEAD_LIMB, "skin_display_list", NEW + "EyesDL")):
        source = h["native_roundtrip"].decode_resource(original[path])
        candidate = h["native_roundtrip"].decode_resource(files[path])
        require(candidate[field] == destination, "incorrect POC4 limb redirect")
        candidate[field] = source[field]
        require(candidate == source, "non-path skin/limb data changed")
    def without_inert_calls(data):
        return [raw for w0, w1, raw in commands(data) if w0 >> 24 not in (0x33, 0xDE)]
    for name in ("BodyDL", "EyesDL"):
        require(without_inert_calls(files[NEW + name]) == without_inert_calls(original[OLD + name]),
                "default draw command stream changed")
    body = list(commands(files[NEW + "BodyDL"]))
    eyes = list(commands(files[NEW + "EyesDL"]))
    require(body[-2][2] == EYE_PRELOAD, "final eye preload changed")
    require([r for w0, w1, r in body if w0 >> 24 == 0xDE] == [segment(0xD), segment(0xE)], "wrong overlay call order")
    require(not any(w0 >> 24 in (1, 2, 0x31, 0x32, 0xDB) for w0, w1, _ in eyes), "eye cache can be overwritten")
    require([r for w0, w1, r in eyes if w0 >> 24 == 0xDE] == [segment(0xB), segment(0xC)],
            "eye display list calls must be state-only iris and reset")
    require(any((w0, w1) == (0xFD100000, 0x08000001) for w0, w1, _ in eyes), "native blink texture changed")
    tinted, iris_count, blink_count = False, 0, 0
    for w0, w1, raw in eyes:
        if raw == segment(0xB): tinted = True
        if raw == segment(0xC): tinted = False
        if w0 >> 24 == 5:
            if tinted: iris_count += 1
            else: blink_count += 1
    require((iris_count, blink_count) == (6, 6), "iris color affects blink eyelids")
    hashes = {h["o2r"].crc64(p.removeprefix("alt/")): p for p in files}
    for name in ("BodyDL", "EyesDL", "CoatOverlayDL", "HairOverlayDL"):
        cache = {}
        for w0, w1, raw in commands(files[NEW + name]):
            opcode = w0 >> 24
            if opcode in (0x20, 0x31, 0x32, 0x33):
                hi, lo = struct.unpack_from("<II", raw, 8)
                require((hi << 32 | lo) in hashes, "unresolved resource hash")
            if opcode == 1:
                n = (w0 >> 12) & 255
                dst = ((w0 >> 1) & 127) - n
                start = (w1 & 0x00FFFFFE) // 16
                require(w1 >> 24 == 8 and w1 & 1 and (w1 & 0x00FFFFFE) % 16 == 0,
                        "invalid body buffer load")
                require(0 < n <= 32 and 0 <= dst <= 32 - n and start + n <= 5912, "vertex cache/buffer overrun")
                if "Overlay" in name:
                    require(start + n <= 5892, "overlay loads eye vertices")
                cache.update({dst + i: start + i for i in range(n)})
            if opcode == 5 and name != "EyesDL":
                require(all(((w0 >> shift) & 255) // 2 in cache for shift in (16, 8, 0)), "unloaded triangle cache index")
        if "Overlay" in name:
            modes = [w1 for w0, w1, _ in commands(files[NEW + name]) if w0 == 0xE200001C]
            require(all(m == DECAL for m in modes[:-1]) and modes[-1] == OPA and not DECAL & 0x20, "overlay depth/write state differs")
    return {"baseline_resources_changed": [BODY_LIMB, HEAD_LIMB], "limb_changes": "display-list path strings only",
            "original_draw_triangles": 1976, "eye_cache_vertices": 20, "body_buffer_vertices": 5912,
            "default_draw_commands_identical": True, "iris_triangles": 6, "unchanged_blink_triangles": 6,
            "original_textures_skin_skeleton_animations_preserved": True, "hash_dependencies_and_vertex_bounds_valid": True}


def write_archive(path, files):
    path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name in sorted(files):
            info = zipfile.ZipInfo(name, (2026, 9, 23, 0, 0, 0))
            info.create_system = 3
            info.external_attr = 0o100644 << 16
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, files[name], compresslevel=9)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", required=True, type=Path)
    parser.add_argument("--checkpoint", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    require(args.source.resolve() != args.output.resolve(), "cannot overwrite POC3 source")
    require(not (args.output.exists() and args.output.samefile(args.source)), "cannot overwrite POC3 hardlink")
    source_bytes = args.source.read_bytes()
    require(sha(source_bytes) == BASE_SHA, "source is not the pinned POC3 archive")
    h = helpers(args.checkpoint)
    with zipfile.ZipFile(args.source) as archive:
        require(archive.testzip() is None and len(archive.namelist()) == 118, "invalid POC3 archive")
        original = {p: archive.read(p) for p in archive.namelist()}
    files, selected, masks = build_files(original, args.checkpoint, h)
    report = validate(original, files, h)
    write_archive(args.output, files)
    with zipfile.ZipFile(args.output) as archive:
        require(archive.testzip() is None, "output ZIP CRC failure")
        validate(original, {p: archive.read(p) for p in archive.namelist()}, h)
    require(args.source.read_bytes() == source_bytes, "POC3 source changed")
    for name, mask in masks.items():
        Image.fromarray(mask).save(args.output.with_name("TP_Epona_POC4_" + name + "_mask.png"))
    report.update({"source": args.source.name, "source_sha256": BASE_SHA,
                   "candidate": args.output.name, "candidate_sha256": sha(args.output.read_bytes()),
                   "candidate_bytes": args.output.stat().st_size, "resources": len(files), "zip_crc_valid": True,
                   "selected_triangles": {"horse_atlas": len(selected[3]), "mane_forelock": len(selected[2]), "tail": len(selected[4])},
                   "segments": {"0B": "iris grayscale/color state", "0C": "reset grayscale",
                                "0D": "conditional coat state + CoatOverlayDL + reset; EndDL when unchanged",
                                "0E": "conditional hair state + HairOverlayDL + reset; EndDL when unchanged"},
                   "resources_added": {p: {"bytes": len(files[p]), "sha256": sha(files[p])} for p in files if p not in original},
                   "mask_provenance": "fixed authored atlas fur support; source chroma separates ivory strands only inside hair regions",
                   "limits": ["Candidate requires POC4-compatible per-draw segment binding in the executable.",
                              "Install this standalone replacement instead of POC3, not alongside it.",
                              "Offline semantic/geometry proof is not live renderer or user acceptance."]})
    manifest = args.output.with_suffix(".manifest.json")
    manifest.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"archive": str(args.output), "manifest": str(manifest), "sha256": report["candidate_sha256"]}, indent=2))


if __name__ == "__main__":
    main()
