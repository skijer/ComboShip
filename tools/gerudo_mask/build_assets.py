#!/usr/bin/env python3
"""Build the veiled GI from the existing rig and import Henriko's matching atlases.

The ankle cuff tips are tucked toward their existing inner ring; the remaining
form vertices, skeletons, UVs and animations are preserved. The GI bakes the
head/hat rest transforms into a separate, centered get-item object. Texture v1
metadata keeps the original N64 tile coordinates while loading HD RGBA pixels.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
CUSTOM = ROOT / "soh/assets/custom"
FORM = "objects/forms/gerudo/object_link_boy"
GI = "objects/object_gi_gerudo_warrior"
BASE = "c0181856968dd6f3869be0d1aa240743091fd6a8"
SOURCES = {
    "geld_00_600": "tex1_128x128_C146422CF70C04F1_13_mip0.png",
    "geld_01_4600": "tex1_128x64_9E707002742B4A45_12_mip0.png",
    "open": "tex1_32x32_8174C47B15759209_12_mip0.png",
    "half": "tex1_32x32_8405073A0CD4B017_12_mip0.png",
    "closed": "tex1_32x32_C02A490608A19B53_12_mip0.png",
}


def original(path):
    return subprocess.check_output(["git", "show", BASE + ":soh/assets/custom/" + path], cwd=ROOT)


def xml(path):
    return ET.fromstring(original(path))


def save_xml(path, root):
    dest = CUSTOM / path
    dest.parent.mkdir(parents=True, exist_ok=True)
    ET.indent(root, space="\t")
    dest.write_text(ET.tostring(root, encoding="unicode") + "\n")


def texture_info(raw):
    version = struct.unpack_from("<I", raw, 8)[0]
    typ, w, h = struct.unpack_from("<III", raw, 64)
    if version == 1:
        flags, hs, vs, size = struct.unpack_from("<IffI", raw, 76)
        offset = 92
    else:
        flags, hs, vs = 0, 1., 1.
        size = struct.unpack_from("<I", raw, 76)[0]
        offset = 80
    assert len(raw) == offset + size
    return typ, w, h, flags, hs, vs, offset


def import_rgba(original_raw, image):
    typ, w, h, flags, hs, vs, _ = texture_info(original_raw)
    assert typ == 2
    # HByteScale describes bytes relative to the original RGBA16 tile. Old
    # native resources need the extra x2 for RGBA32; existing raw ones don't.
    header = bytearray(original_raw[:64])
    struct.pack_into("<I", header, 8, 1)
    image = image.convert("RGBA")
    data = image.tobytes()
    return bytes(header) + struct.pack("<IIIIffI", typ, *image.size, 1,
                                     hs * image.width / w * (1 if flags & 1 else 2),
                                     vs * image.height / h, len(data)) + data


def full_color_cloth(tree, base):
    """Bind the supplied colored atlas without multiplying it by a tunic tint."""
    tree.find("SetTextureImage").attrib["Path"] = base + "/geld_00_600"
    tree.find("SetCombineLERP").attrib["C1"] = "G_CCMUX_PRIMITIVE"
    tree.insert(1, ET.Element("SetPrimColor", M="0", L="0", R="255", G="255", B="255", A="255"))


def refine_cuffs():
    """Shorten only the five flared cuff tips, preserving seam duplicates."""
    changes = {}
    for rig, age, right, left in (
        ("object_link_boy", "Adult", "RightLeg", "LeftLeg"),
        ("object_link_child", "Child", "RightShin", "LeftShin"),
    ):
        for bone, side in (("bone004", right), ("bone007", left)):
            base = "objects/forms/gerudo/" + rig
            mesh = bone + "_gLink" + age + side + "Limb_mesh_layer_Opaque"
            path = base + "/" + mesh + "_vtx_1"
            tree = xml(path)
            verts = list(tree)
            old = np.array([[int(v.attrib[k]) for k in ("X", "Y", "Z")] for v in verts])
            anchors = np.unique([old[i] for i,v in enumerate(verts) if v.attrib["T"] == "928"], axis=0)
            selected = {i for i,v in enumerate(verts) if v.attrib["T"] == "1000" and old[i,0] > anchors[:,0].max()}
            assert len(np.unique(old[sorted(selected)], axis=0)) == 5
            new = old.copy()
            for i in selected:
                anchor = anchors[np.argmin(np.linalg.norm(anchors - old[i], axis=1))]
                new[i] = np.rint(anchor + .35 * (old[i] - anchor)).astype(int)
            # Rebuild the affected lighting normals from the actual cuff faces.
            # Group by the old normal as well as position to retain hard creases.
            def group(i):
                return tuple(old[i]) + tuple(int(verts[i].attrib[k]) for k in ("R", "G", "B"))
            normals = {group(i): np.zeros(3) for i in selected}
            cache = {}
            for c in xml(base + "/" + mesh + "_tri_1"):
                a = c.attrib
                if c.tag == "LoadVertices":
                    assert a["Path"] == path
                    start, count, slot = (int(a[k]) for k in ("VertexOffset", "Count", "VertexBufferIndex"))
                    cache.update({slot+j: start+j for j in range(count)})
                elif c.tag == "Triangle1":
                    indices = [cache[int(a["V0" + str(j)])] for j in range(3)]
                    if not selected.intersection(indices): continue
                    before = np.cross(old[indices[1]] - old[indices[0]], old[indices[2]] - old[indices[0]])
                    after = np.cross(new[indices[1]] - new[indices[0]], new[indices[2]] - new[indices[0]])
                    assert np.linalg.norm(after) > 0 and np.dot(before, after) > 0, (path, indices)
                    for i in selected.intersection(indices):
                        normal = np.array([((int(verts[i].attrib[k]) + 128) % 256 - 128) for k in ("R", "G", "B")])
                        normals[group(i)] += after * (1 if np.dot(before, normal) >= 0 else -1)
            for i in selected:
                n = normals[group(i)]
                assert np.linalg.norm(n) > 0
                n = np.rint(n / np.linalg.norm(n) * 127).astype(int)
                for k,val in zip(("X", "Y", "Z"), new[i]): verts[i].attrib[k] = str(val)
                for k,val in zip(("R", "G", "B"), n): verts[i].attrib[k] = str(int(val) % 256)
            save_xml(path, tree)
            changes[path] = {"tips": 5, "seamVertices": len(selected), "remainingFlare": .35}
    return changes


def build_gi():
    head = "bone010_gLinkAdultHeadLimb_mesh_layer_Opaque"
    hat = "bone011_gLinkAdultHatLimb_mesh_layer_Opaque"
    translations = {head: np.zeros(3), hat: np.array([-298, -700, 0])}
    # The eye UVs establish the head axes: -Y is up, +X faces forward and
    # Z spans the eyes. This rotation preserves handedness and triangle winding.
    basis = np.array([[0, 0, 1], [0, -1, 0], [1, 0, 0]])
    vertices = {}
    for mesh, shift in translations.items():
        for path in sorted({c.attrib["Path"] for c in xml(FORM + "/" + mesh) if c.tag == "CallDisplayList"
                            and "_tri_" in c.attrib["Path"]}):
            for c in xml(path):
                if c.tag == "LoadVertices":
                    p = c.attrib["Path"]
                    vertices[p] = (xml(p), shift)
    points = np.concatenate([np.array([[int(v.attrib[k]) for k in ("X", "Y", "Z")] for v in tree]) + shift
                             for tree, shift in vertices.values()]) @ basis.T
    low, high = points.min(axis=0), points.max(axis=0)
    center = (low + high) / 2
    scale = 58 / (high[1] - low[1])
    for path, (tree, shift) in vertices.items():
        for v in tree:
            a = v.attrib
            p = (np.array([int(a[k]) for k in ("X", "Y", "Z")]) + shift) @ basis.T
            p = np.rint((p - center) * scale).astype(int)
            n = np.array([((int(a[k]) + 128) % 256 - 128) for k in ("R", "G", "B")]) @ basis.T
            for k, val in zip(("X", "Y", "Z"), p): a[k] = str(val)
            for k, val in zip(("R", "G", "B"), n): a[k] = str(int(val) % 256)
        save_xml(GI + "/" + Path(path).name, tree)

    seen = set()
    def clone(path):
        if path in seen: return
        seen.add(path)
        tree = xml(path)
        for c in tree:
            if c.tag in ("CallDisplayList", "LoadVertices"):
                src = c.attrib["Path"]
                c.attrib["Path"] = GI + "/" + Path(src).name
                if c.tag == "CallDisplayList": clone(src)
            elif c.tag == "SetGeometryMode":
                c.attrib.pop("G_FOG", None)
            elif c.tag == "ClearGeometryMode":
                c.attrib["G_FOG"] = "1"
            elif c.tag == "SetOtherMode" and c.attrib.get("Cmd") == "G_SETOTHERMODE_L":
                c.attrib.pop("G_RM_FOG_SHADE_A", None)
                c.attrib["G_RM_PASS"] = "1"
        if "Facemask" in path:
            full_color_cloth(tree, FORM)
        save_xml(GI + "/" + Path(path).name, tree)

    for mesh in translations: clone(FORM + "/" + mesh)
    top = ET.Element("DisplayList", Version="0")
    ET.SubElement(top, "PipeSync")
    ET.SubElement(top, "SetPrimColor", M="0", L="0", R="255", G="255", B="255", A="255")
    for mesh in translations: ET.SubElement(top, "CallDisplayList", Path=GI + "/" + mesh)
    ET.SubElement(top, "PipeSync")
    ET.SubElement(top, "ClearGeometryMode", G_TEXTURE_GEN="1", G_TEXTURE_GEN_LINEAR="1")
    ET.SubElement(top, "Texture", S="65535", T="65535", Level="0", Tile="0", On="0")
    ET.SubElement(top, "SetEnvColor", R="255", G="255", B="255", A="255")
    ET.SubElement(top, "EndDisplayList")
    save_xml(GI + "/gGiGerudoWarriorMaskDL", top)
    return {"sourceBounds": [low.tolist(), high.tolist()], "scale": scale,
            "center": center.tolist(), "vertices": sum(len(t) for t, _ in vertices.values())}


def reflection_material(path, reflection_path):
    tree = xml(path)
    # Keep the existing lit, opaque material and replace only the blade map.
    for c in tree:
        if c.tag == "SetGeometryMode": c.attrib["G_TEXTURE_GEN"] = "1"
        elif c.tag == "ClearGeometryMode": c.attrib.pop("G_TEXTURE_GEN", None)
        elif c.tag == "Texture": c.attrib.update(S="1984", T="1984")
    # Use the same factory macro as the existing metallic GI. It calculates
    # the RGBA32 load-block/TMEM strides instead of retaining RGBA16 strides.
    insertion = list(tree).index(tree.find("SetTextureImage"))
    for c in list(tree):
        if c.tag in ("SetTextureImage", "SetTile", "LoadBlock", "SetTileSize", "LoadSync", "TileSync"):
            tree.remove(c)
    insertion = min(insertion, len(tree) - 1)
    tree.insert(insertion, ET.Element("LoadTextureBlock", Path=reflection_path, Format="0", Size="3",
                                     Width="32", Height="32", MaskS="0", MaskT="0", ShiftS="0", ShiftT="0",
                                     CMS_TXNoMirror="1", CMS_TXClamp="1", CMT_TXNoMirror="1", CMT_TXClamp="1"))
    save_xml(path, tree)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("henriko", type=Path, help="extracted CHARACTERS directory")
    args = parser.parse_args()
    provenance = {"baseline": BASE, "sources": {}, "gi": build_gi(),
                  "notes": ["Henriko pixels imported without repainting or resampling",
                            "Full Henriko clothing and veil use fixed source purple instead of a tunic tint",
                            "Only five outer ankle cuff tips per leg are tucked in; rig and UVs preserved",
                            "Sphere-mapped metallic highlights, not world-space ray-traced reflections"]}
    images = {}
    for target, name in SOURCES.items():
        source = args.henriko / name
        images[target] = Image.open(source).convert("RGBA")
        provenance["sources"][target] = {"name": name, "sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
                                          "size": list(images[target].size)}
    for rig, age in (("object_link_boy", "Adult"), ("object_link_child", "Child")):
        base = "objects/forms/gerudo/" + rig
        for leaf in ("geld_00_600", "geld_01_4600"):
            (CUSTOM / base / leaf).write_bytes(import_rgba(original(base + "/" + leaf), images[leaf]))
        eye_raw = original(base + "/gelb_eye01_CI00_5600")
        (CUSTOM / base / "gelb_eye01_CI00_5600").write_bytes(import_rgba(eye_raw, images["open"]))
        for suffix in ("Open", "Half", "Closedf", "RollLeft", "RollRight", "Shock", "Unk1", "Unk2"):
            state = "half" if suffix == "Half" else "closed" if suffix == "Closedf" else "open"
            dest = CUSTOM / base / ("gLink" + age + "Eyes" + suffix + "Tex")
            dest.write_bytes(import_rgba(eye_raw, images[state]))
        material = xml(base + "/mat_gLink" + age + "Skel_Eyes_f3d_layerOpaque")
        material.find("SetTextureImage").attrib["Path"] = ">0x08000000"
        save_xml(base + "/mat_gLink" + age + "Skel_Eyes_f3d_layerOpaque", material)
        cloth_paths = set((CUSTOM / base).glob("mat_*Tunic_Color*")) | set((CUSTOM / base).glob("mat_*Facemask*"))
        for cloth in sorted(cloth_paths):
            path = cloth.relative_to(CUSTOM).as_posix()
            material = xml(path)
            full_color_cloth(material, base)
            save_xml(path, material)
        # Reflection uses the repository's existing silver environment map.
        reflected = base + "/gGerudoSwordReflectionTex"
        (CUSTOM / reflected).write_bytes((CUSTOM / "objects/nei_gi_redesign/master_sword/steel_tex").read_bytes())
        blade = ("mat_gLinkAdultLeftHandHoldingMasterSwordNearDL_Steelblade_f3d" if age == "Adult"
                 else "mat_gLinkChildLeftFistAndKokiriSwordNearDL_Blade_001_f3d")
        reflection_material(base + "/" + blade, reflected)
    provenance["cuffs"] = refine_cuffs()
    dest = ROOT / "tools/gerudo_mask/provenance.json"
    dest.write_text(json.dumps(provenance, indent=2) + "\n")
    print(json.dumps(provenance, indent=2))


if __name__ == "__main__":
    main()
