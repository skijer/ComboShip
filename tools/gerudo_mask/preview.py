#!/usr/bin/env python3
"""Offline previews of the actual GI/head/blade display lists (not game captures)."""
from __future__ import annotations

import argparse
import copy
from functools import lru_cache
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.nei_icons.render import Material, Model, Part, render, rotation, unit
from tools.gerudo_mask.build_assets import CUSTOM, FORM, GI, original, texture_info


@lru_cache(maxsize=128)
def pixels(path):
    raw = (CUSTOM / path).read_bytes()
    typ, w, h, flags, _, _, offset = texture_info(raw)
    if flags & 1 or typ == 1:
        a = np.frombuffer(raw, np.uint8, offset=offset).reshape(h, w, 4)
    elif typ == 2:
        v = np.frombuffer(raw, ">u2", offset=offset).reshape(h, w).astype(np.uint32)
        a = np.stack([((v >> b) & 31) * 255 / 31 for b in (11, 6, 1)] + [(v & 1) * 255], axis=-1)
    else:
        raise ValueError((path, typ))
    return a.astype(float) / 255


def model(entry, eye="Open", transform=None, matrices=None, tint=(165, 85, 210, 255), vertex_overrides=None):
    result = Model()
    cache = {}
    state = Material()
    env = np.array(tint) / 255
    env_mult = False
    tile_size = (32, 32)
    current_matrix = np.eye(4) if transform is None else transform.copy()

    def visit(path):
        nonlocal env, env_mult, tile_size, current_matrix
        result.sources.add(CUSTOM / path)
        for c in ET.parse(CUSTOM / path).getroot():
            a, tag = c.attrib, c.tag
            if tag == "CallDisplayList": visit(a["Path"])
            elif tag == "LoadVertices":
                raw = (vertex_overrides or {}).get(a["Path"])
                vs = list(ET.fromstring(raw) if raw is not None else ET.parse(CUSTOM / a["Path"]).getroot())
                start, count, slot = (int(a[k]) for k in ("VertexOffset", "Count", "VertexBufferIndex"))
                for i, v in enumerate(vs[start:start + count]):
                    b = v.attrib
                    position = np.array([int(b[k]) for k in ("X", "Y", "Z")])
                    normal = unit([((int(b[k])+128)%256-128) for k in ("R", "G", "B")])
                    cache[slot+i] = (position @ current_matrix[:3,:3].T + current_matrix[:3,3],
                                     unit(normal @ current_matrix[:3,:3].T),
                                     [int(b[k]) / 32 for k in ("S", "T")])
            elif tag == "Matrix":
                address = int(a["Path"][1:], 16)
                assert address >> 24 == 13 and matrices is not None, (entry, a)
                current_matrix = matrices[(address & 0xFFFFFF) // 64].copy()
            elif tag in ("Triangle1", "Triangle2", "Triangles2"):
                for prefix in (["V0"] if tag == "Triangle1" else ["V0", "V1"]):
                    vs = [cache[int(a[prefix+str(i)])] for i in range(3)]
                    mat = copy.copy(state)
                    if env_mult: mat.color = (np.array(state.color) * env).tolist()
                    result.parts.append(Part(np.array([v[0] for v in vs]), np.array([v[1] for v in vs]),
                                             np.array([v[2] for v in vs]) / tile_size,
                                             np.array([[0, 1, 2]]), mat))
            elif tag == "SetCombineLERP": env_mult = a.get("C1") == "G_CCMUX_ENVIRONMENT"
            elif tag == "SetEnvColor": env = [int(a[k])/255 for k in ("R", "G", "B", "A")]
            elif tag == "SetPrimColor": state.color = [int(a[k])/255 for k in ("R", "G", "B", "A")]
            elif tag in ("SetGeometryMode", "ClearGeometryMode"):
                on = tag == "SetGeometryMode"
                if "G_TEXTURE_GEN" in a: state.texgen = on
                if "G_TEXTURE_GEN_LINEAR" in a: state.texgen_linear = on
                if "G_CULL_BACK" in a: state.cull = on
            elif tag in ("SetTextureImage", "LoadTextureBlock"):
                path = a["Path"]
                if path.startswith(">"):
                    path = FORM + "/gLinkAdultEyes" + eye + "Tex"
                state.texture = pixels(path)
                if tag == "LoadTextureBlock": tile_size = (int(a["Width"]), int(a["Height"]))
            elif tag == "SetTileSize": tile_size = ((int(a["Lrs"])-int(a["Uls"]))/4+1,
                                                    (int(a["Lrt"])-int(a["Ult"]))/4+1)
            elif tag == "Texture":
                state.tex_scale = (int(a["S"])/2048, int(a["T"])/2048)
                if a.get("On") == "0": state.texture = None
    visit(entry)
    return result


def full_form(frame=0, animation="db_idle01_loop", eye="Open", swords=False, vertex_overrides=None):
    """Pose real limbs with the packed 67-s16 player frame and flex matrix slots.

    The root uses jointTable[0] translation and jointTable[1] rotation. Other
    limbs use their skeleton translation plus jointTable[limb+1] rotation, as
    SkelAnime_DrawFlexOpa does. Vertices are transformed when loaded into the
    cache, including matrix changes inside the torso's weighted display lists.
    """
    anim_path = CUSTOM / "misc/link_animetion" / ("gPlayerAnim_mhr_" + animation)
    raw = anim_path.read_bytes()
    count, = struct.unpack_from("<I", raw, 64)
    assert len(raw) == 68 + count * 2 and count % 67 == 0
    pose = np.frombuffer(raw, "<i2", offset=68).reshape(-1, 67)[frame]
    joints = pose[:66].reshape(22, 3)
    skeleton_path = CUSTOM / FORM / "gLinkAdultSkel"
    limbs = [ET.parse(CUSTOM / x.attrib["Path"]).getroot().attrib for x in ET.parse(skeleton_path).getroot()]
    world, draw_order = {}, []

    def visit(i, parent):
        limb = limbs[i]
        local = np.eye(4)
        degrees = joints[i+1].astype(float) * (360 / 65536)
        local[:3,:3] = rotation("z", degrees[2]) @ rotation("y", degrees[1]) @ rotation("x", degrees[0])
        local[:3,3] = joints[0] if i == 0 else [int(limb["LegTrans"+k]) for k in "XYZ"]
        world[i] = parent @ local
        draw_order.append(i)
        if int(limb["ChildIndex"]) != 255: visit(int(limb["ChildIndex"]), world[i])
        if int(limb["SiblingIndex"]) != 255: visit(int(limb["SiblingIndex"]), parent)
    visit(0, np.eye(4))
    # Empty limbs consume no matrix. Hidden, originally nonempty limbs still
    # consume their slot, so matrix addresses in weighted meshes stay unchanged.
    matrices = [world[i] for i in draw_order if limbs[i]["DisplayList1"] != "gEmptyDL"]
    result = Model(sources={anim_path, skeleton_path})
    for i in draw_order:
        path = limbs[i]["DisplayList1"]
        if path == "gEmptyDL" or (swords and i == 19): continue
        if swords and i in (15, 18): path = FORM + "/gLinkAdultLeftHandHoldingMasterSwordNearDL"
        piece = model(path, eye=eye, transform=world[i], matrices=matrices, vertex_overrides=vertex_overrides)
        result.parts.extend(piece.parts)
        result.sources.update(piece.sources)
    assert len(world) == 21 and len(matrices) == 18
    return result


def full_body_previews(output):
    standing = full_form(swords=False)
    ready = full_form(swords=True)
    sheet = Image.new("RGB", (1800, 940), (25, 28, 39))
    d = ImageDraw.Draw(sheet)
    headings = [(standing, 0, "Front | packed dual-blade idle"),
                (standing, 90, "Side | packed dual-blade idle"),
                (standing, 180, "Back | packed dual-blade idle"),
                (ready, 25, "Twin swords | packed dual-blade idle")]
    for i, (m, angle, label) in enumerate(headings):
        img = render(m, size=880, azimuth=angle, elevation=0, supersample=2, fill=.93)
        bbox = img.getbbox()
        img = img.crop(bbox)
        scale = min(430/img.width, 850/img.height, 1.)
        img = img.resize((round(img.width*scale), round(img.height*scale)), Image.Resampling.LANCZOS)
        sheet.paste(img, (i*450+(450-img.width)//2, 885-img.height), img)
        d.text((i*450+16, 911), label, fill="white")
    label = "HENRIKO FULL ATLAS CANDIDATE | Fixed source purple cloth"
    d.text((18, 9), label + " | Real rig + packed dual-blade idle | Offline lighting", fill=(175,185,200))
    prefix = "gerudo-full-body"
    sheet.save(output / (prefix + "-preview.png"))
    front = render(standing, size=1200, azimuth=0, elevation=0, supersample=2, fill=.92)
    front.save(output / (prefix + "-front.png"))
    print("Full form triangles:", len(standing.parts), "combat triangles:", len(ready.parts))


def ankle_previews(output):
    """Compare the previous cuff geometry with the candidate's native geometry."""
    overrides = {FORM + "/" + name: original(FORM + "/" + name) for name in (
        "bone004_gLinkAdultRightLegLimb_mesh_layer_Opaque_vtx_1",
        "bone007_gLinkAdultLeftLegLimb_mesh_layer_Opaque_vtx_1",
    )}
    after = full_form()
    points = np.concatenate([part.positions for part in after.parts])
    cutoff = points[:,1].min() + np.ptp(points[:,1]) * .32
    models = [full_form(vertex_overrides=overrides), after]
    sheet = Image.new("RGB", (1560, 1040), (25, 28, 39))
    d = ImageDraw.Draw(sheet)
    for row,m in enumerate(models):
        lower = Model(parts=[part for part in m.parts if part.positions[:,1].max() < cutoff])
        for col,angle in enumerate((0, 90, 180)):
            img = render(lower, size=520, azimuth=angle, elevation=0, supersample=2, fill=.88)
            sheet.paste(img, (col*520, row*520), img)
            d.text((col*520+16, row*520+491),
                   ("Previous flare" if row == 0 else "Tucked cuff") + f" | yaw {angle}", fill="white")
    d.text((16, 8), "ACTUAL CUFF GEOMETRY | Same Henriko atlas and offline lighting", fill=(175,185,200))
    sheet.save(output / "gerudo-ankle-comparison.png")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("output", type=Path)
    p.add_argument("--full-body-only", action="store_true")
    p.add_argument("--ankles-only", action="store_true", help="Compare the previous cuff geometry with the candidate")
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    ankle_previews(args.output)
    if args.ankles_only: return
    full_body_previews(args.output)
    if args.full_body_only: return
    gi = model(GI + "/gGiGerudoWarriorMaskDL")
    angles = [0, 35, 180]
    sheet = Image.new("RGB", (1320, 500), (25, 28, 39))
    d = ImageDraw.Draw(sheet)
    for i, a in enumerate(angles):
        frame = render(gi, size=440, azimuth=a, elevation=5, supersample=2)
        sheet.paste(frame, (440*i, 22), frame)
        d.text((440*i+18, 468), f"Veiled Gerudo GI | yaw {a}", fill="white")
    d.text((18, 6), "ACTUAL CANDIDATE MESH | Offline lighting approximation", fill=(170,180,195))
    sheet.save(args.output / "gerudo-gi-preview.png")
    frames = []
    # The GI deliberately keeps an open expression. Show the FORM's three
    # segmented eye textures by substituting its own eyes into this same mesh.
    for state in ("Open", "Half", "Closedf", "Half", "Open"):
        m = copy.deepcopy(gi)
        original_eye = pixels(FORM + "/gelb_eye01_CI00_5600")
        for part in m.parts:
            if part.material.texture is not None and part.material.texture.shape == original_eye.shape \
                    and np.array_equal(part.material.texture, original_eye):
                part.material.texture = pixels(FORM + "/gLinkAdultEyes" + state + "Tex")
        img = Image.new("RGB", (400, 440), (25, 28, 39))
        face = render(m, size=400, azimuth=0, elevation=0, supersample=2)
        img.paste(face, (0, 15), face)
        ImageDraw.Draw(img).text((14, 420), "Form eye binding | " + state, fill="white")
        frames.append(img)
    frames[0].save(args.output / "gerudo-blink-preview.gif", save_all=True, append_images=frames[1:],
                   duration=[1500, 80, 110, 80, 1500], loop=0, disposal=2)
    sword = model(FORM + "/gLinkAdultLeftHandHoldingMasterSwordNearDL")
    # Player hand coordinates use the rig's cyclic axes too.
    basis = np.array([[0,0,1], [1,0,0], [0,1,0]])
    for part in sword.parts:
        part.positions = part.positions @ basis.T
        part.normals = part.normals @ basis.T
    panel = Image.new("RGB", (960, 520), (25, 28, 39))
    for i, a in enumerate((0, 65)):
        frame = render(sword, size=480, azimuth=a, elevation=12, supersample=2)
        panel.paste(frame, (480*i, 20), frame)
    ImageDraw.Draw(panel).text((16, 6), "ACTUAL SCIMITAR | Sphere-mapped silver finish | Two view angles", fill="white")
    panel.save(args.output / "gerudo-sword-preview.png")
    print("GI triangles:", len(gi.parts), "sword triangles:", len(sword.parts))


if __name__ == "__main__": main()
