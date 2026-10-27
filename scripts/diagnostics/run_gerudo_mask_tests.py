#!/usr/bin/env python3
"""Validate native assets, isolated cuff taper, preserved rig/UVs and MM GI routing."""
from pathlib import Path
import json
import os
import numpy as np
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.gerudo_mask.build_assets import BASE, CUSTOM, FORM, GI, original, texture_info


def function(source, name):
    import re
    match = re.search(r'^.*\b' + name + r'\([^;{}]*\)\s*\{', source, re.M)
    assert match, name
    start = match.start()
    pos = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[pos] == "{") - (source[pos] == "}")
        pos += 1
    return source[start:pos]


def assets():
    provenance = json.loads((ROOT / "tools/gerudo_mask/provenance.json").read_text())
    cache, seen = {}, set()
    triangles = 0
    def walk(path):
        nonlocal triangles
        assert (CUSTOM / path).is_file(), path
        seen.add(path)
        for c in ET.parse(CUSTOM / path).getroot():
            a, tag = c.attrib, c.tag
            if tag == "CallDisplayList": walk(a["Path"])
            elif tag == "LoadVertices":
                v = list(ET.parse(CUSTOM / a["Path"]).getroot())
                offset, count, slot = (int(a[k]) for k in ("VertexOffset", "Count", "VertexBufferIndex"))
                assert 0 <= offset < len(v) and offset + count <= len(v)
                assert 0 < count <= 32 and slot + count <= 32
                cache.update({slot+i: x for i,x in enumerate(v[offset:offset+count])})
                # Generated GI copies retain the source UVs exactly.
                src = list(ET.fromstring(original(FORM + "/" + Path(a["Path"]).name)))
                for x,y in zip(v,src):
                    assert all(x.attrib[k] == y.attrib[k] for k in ("S", "T", "A"))
                    assert all(abs(int(x.attrib[k])) <= 40 for k in ("X", "Y", "Z"))
            elif tag in ("Triangle1", "Triangle2", "Triangles2"):
                for prefix in (["V0"] if tag == "Triangle1" else ["V0", "V1"]):
                    assert all(int(a[prefix+str(i)]) in cache for i in range(3))
                    triangles += 1
            elif tag == "SetTextureImage":
                assert not a["Path"].startswith(">"), "GI must not depend on player eye segments"
                raw = (CUSTOM / a["Path"]).read_bytes()
                assert texture_info(raw)[0] == 2, "GI must have direct RGBA textures, no missing palette"
            elif tag == "SetGeometryMode": assert "G_FOG" not in a
    walk(GI + "/gGiGerudoWarriorMaskDL")
    assert triangles == 328
    for rig, age in (("object_link_boy", "Adult"), ("object_link_child", "Child")):
        base = "objects/forms/gerudo/" + rig
        states = {}
        for state in ("Open", "Half", "Closedf"):
            raw = (CUSTOM / base / ("gLink"+age+"Eyes"+state+"Tex")).read_bytes()
            typ,w,h,flags,hs,vs,off = texture_info(raw)
            assert (typ,w,h,flags,hs,vs) == (2,256,256,1,16.,8.)
            assert len(raw[off:]) == w*h*4
            states[state] = raw[off:]
        assert len(set(states.values())) == 3
        for suffix in ("RollLeft", "RollRight", "Shock", "Unk1", "Unk2"):
            raw = (CUSTOM / base / ("gLink"+age+"Eyes"+suffix+"Tex")).read_bytes()
            assert raw[92:] == states["Open"], "unused expressions must not leak Link's eyes"
        material = ET.parse(CUSTOM / base / ("mat_gLink"+age+"Skel_Eyes_f3d_layerOpaque")).getroot()
        assert material.find("SetTextureImage").attrib["Path"] == ">0x08000000"
        cloth_paths = set((CUSTOM / base).glob("mat_*Tunic_Color*")) | set((CUSTOM / base).glob("mat_*Facemask*"))
        for path in cloth_paths:
            material = ET.parse(path).getroot()
            assert material.find("SetTextureImage").attrib["Path"] == base + "/geld_00_600", "full Henriko cloth atlas missing"
            assert material.find("SetCombineLERP").attrib["C1"] == "G_CCMUX_PRIMITIVE", "source purple multiplied by a tunic tint"
            assert all(material.find("SetPrimColor").attrib[k] == "255" for k in ("R","G","B","A"))
        for name in ("geld_00_600", "geld_01_4600"):
            raw = (CUSTOM / base / name).read_bytes()
            typ,w,h,flags,_,_,off = texture_info(raw)
            assert [w,h] == provenance["sources"][name]["size"]
            assert typ == 2 and flags == 1 and len(raw[off:]) == w*h*4
        blade = ("mat_gLinkAdultLeftHandHoldingMasterSwordNearDL_Steelblade_f3d" if age == "Adult"
                 else "mat_gLinkChildLeftFistAndKokiriSwordNearDL_Blade_001_f3d")
        mat = ET.parse(CUSTOM / base / blade).getroot()
        assert mat.find("SetGeometryMode").attrib["G_TEXTURE_GEN"] == "1"
        assert "G_TEXTURE_GEN" not in mat.find("ClearGeometryMode").attrib
        texture = mat.find("LoadTextureBlock")
        assert texture.attrib["Size"] == "3" and texture.attrib["Width"] == "32"
        assert texture_info((CUSTOM / texture.attrib["Path"]).read_bytes())[0] == 1
    cuff_files = {
        "bone004_gLinkAdultRightLegLimb_mesh_layer_Opaque_vtx_1",
        "bone007_gLinkAdultLeftLegLimb_mesh_layer_Opaque_vtx_1",
        "bone004_gLinkChildRightShinLimb_mesh_layer_Opaque_vtx_1",
        "bone007_gLinkChildLeftShinLimb_mesh_layer_Opaque_vtx_1",
    }
    for p in (CUSTOM / "objects/forms/gerudo").rglob("*"):
        if p.is_file() and ("_vtx_" in p.name or "SkelLimb" in p.name or p.name.endswith("Skel")):
            before = original(p.relative_to(CUSTOM).as_posix())
            if p.name not in cuff_files:
                assert p.read_bytes() == before, "form rig/geometry changed: " + str(p)
                continue
            old, new = list(ET.fromstring(before)), list(ET.parse(p).getroot())
            assert len(old) == len(new) == 37
            cuff_top = max(int(v.attrib["X"]) for v in old if v.attrib["T"] == "928")
            old_tip = max(int(v.attrib["X"]) for v in old)
            new_tip = max(int(v.attrib["X"]) for v in new)
            assert new_tip - cuff_top <= (old_tip - cuff_top) * .5, "cuff flare still protrudes far below the ankle"
            seam_positions = {}
            for a,b in zip(old,new):
                assert all(a.attrib[k] == b.attrib[k] for k in ("S","T","A")), "cuff UV/alpha changed"
                permitted = a.attrib["T"] == "1000" and int(a.attrib["X"]) > cuff_top
                if not permitted: assert a.attrib == b.attrib, "cuff change spread into the shin/weighted knee"
                if permitted:
                    source_position = tuple(a.attrib[k] for k in ("X","Y","Z"))
                    position = tuple(b.attrib[k] for k in ("X","Y","Z"))
                    assert seam_positions.setdefault(source_position,position) == position, "cuff seam split"
                    normal = [((int(b.attrib[k])+128)%256-128) for k in ("R","G","B")]
                    assert 120 <= np.linalg.norm(normal) <= 130, "invalid cuff lighting normal"
            selected = {i for i,v in enumerate(old) if v.attrib["T"] == "1000" and int(v.attrib["X"]) > cuff_top}
            positions = [np.array([[int(v.attrib[k]) for k in ("X","Y","Z")] for v in vertices]) for vertices in (old,new)]
            cache = {}
            for command in ET.parse(p.with_name(p.name.replace("_vtx_1","_tri_1"))).getroot():
                a = command.attrib
                if command.tag == "LoadVertices":
                    offset,count,slot = (int(a[k]) for k in ("VertexOffset","Count","VertexBufferIndex"))
                    cache.update({slot+i:offset+i for i in range(count)})
                elif command.tag == "Triangle1":
                    indices = [cache[int(a["V0"+str(i)])] for i in range(3)]
                    if not selected.intersection(indices): continue
                    before,after = [np.cross(v[indices[1]]-v[indices[0]],v[indices[2]]-v[indices[0]]) for v in positions]
                    assert np.dot(before,after) > 0, "folded cuff triangle"
                    assert np.linalg.norm(after) > np.linalg.norm(before)*.05, "degenerate cuff triangle"
    oot = (ROOT / "soh/src/code/z_draw.c").read_text()
    assert "{ GetItem_DrawMaskOrBombchu, { gGiGerudoWarriorMaskDL } }" in oot
    print("PASS Gerudo assets: 328 real triangles, valid cache/dependencies, full Henriko cloth, isolated cuff taper, preserved UV/rig, HD metadata, three eye states and reflection format")


def draw():
    source = (ROOT / "tests/gerudo_mask/draw_test.cpp").read_text()
    source = source.replace("/* PRODUCTION_GERUDO_DRAW */", function((ROOT / "mm/2s2h/Rando/DrawItem.cpp").read_text(), "DrawOotGerudoMask"))
    includes = ("mm", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm/2s2h",
                "libultraship/include", "libultraship/src", "combo", "combo/menu")
    flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DLOG_LEVEL_GAME_PRINTS=0", "-DCONTROLLERBUTTONS_T=uint32_t"]
    with tempfile.TemporaryDirectory(prefix="gerudo-mask-") as folder:
        p = Path(folder)
        (p / "draw.cpp").write_text(source)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=gnu++20", "-rdynamic", *flags,
                        *["-I" + str(ROOT / i) for i in includes], str(p / "draw.cpp"),
                        str(ROOT / "mm/2s2h/Rando/NeiResourceRouting.cpp"), "-ldl", "-o", str(p / "draw")], check=True)
        subprocess.run([str(p / "draw")], check=True)
        form = (ROOT / "soh/mods/transformation_masks/custom_forms.cpp").read_text()
        source = (ROOT / "tests/gerudo_mask/face_test.cpp").read_text().replace("/* PRODUCTION_FACE_RESOLVER */",
            "\n".join(function(form, name) for name in ("VanillaPlayerLeaf", "CustomForms_ResolveVanillaTexture",
                                                       "CustomForms_PreferFaceTextures")))
        (p / "face.cpp").write_text(source)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=gnu++20", str(p / "face.cpp"), "-o", str(p / "face")], check=True)
        subprocess.run([str(p / "face")], check=True)


if __name__ == "__main__":
    assets()
    draw()
