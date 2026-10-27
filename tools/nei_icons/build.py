#!/usr/bin/env python3
"""Render, package and verify optional HD GI-matched icons.

python tools/nei_icons/build.py --render --manifest --pack /tmp/nei-icons.o2r
python tools/nei_icons/build.py --verify

The native 32px logical slots remain unchanged. OTEX version 1 raw textures carry
512px RGBA pixels and 16x horizontal/vertical scale, the same format used by the
existing HD icon pack builder. Only the optional pack receives these pixels;
the built-in game assets remain unchanged. Base/Alt entries follow mod priority.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

from render import ROOT, glb_model, native_model, render

HERE = Path(__file__).resolve().parent
SIZE = 512
LOGICAL_SIZE = 32

# Each source is the actual GI checkpoint or native XML entry; all pose choices
# are explicit so the icon authoring step can be reproduced without hand edits.
ICONS = [
    ("SheikahSlate", "Sheikah Slate", "sheikah_slate", 16, 8, 0),
    ("SheikahSlateBomb", "Slate · Bomb", "slate_bomb", 16, 8, 0),
    ("SheikahSlateMasterCycle", "Slate · Master Cycle", "slate_master_cycle", 16, 8, 0),
    ("SheikahSlateStasis", "Slate · Stasis", "slate_stasis", 16, 8, 0),
    ("SheikahSlateCryonis", "Slate · Cryonis", "slate_cryonis", 16, 8, 0),
    ("SheikahSlateSensor", "Slate · Sensor", "slate_sensor", 16, 8, 0),
    ("StatDefense", "Defense Upgrade", "gStatDefenseDL", 25, 12, 0),
    ("StatSpeed", "Speed Upgrade", "gStatSpeedDL", 25, 12, 0),
    ("StatPower", "Power Upgrade", "gStatPowerDL", -80, 8, 0),
    ("StatMagic", "Magic Upgrade", "magic_jar_generated", 0, 0, 0),
    ("CrawlSpeed", "Crawl Speed", "gStatCrawlSpeedDL", 25, 12, 0),
    ("ClimbSpeed", "Climb Speed", "gStatClimbSpeedDL", 25, 12, 0),
    ("PushSpeed", "Push Speed", "gStatPushSpeedDL", 25, 12, 0),
    ("ShadowScepter", "Shadow Scepter", "shadow_scepter", 18, 8, -27),
    ("DemiseDestruction", "Demise Destruction", "demise_destruction", 20, 12, 0),
    ("MarioMask", "Mario Mask", "mario_mask", 14, 8, 0),
    ("ClimbBoots", "Climbing Boots", "climb_boots", 24, 12, 0),
    ("ElementalWand", "Elemental Wand", "elemental_wand", 18, 8, -27),
    ("SandRod", "Sand Rod", "sand_rod", 18, 8, -27),
    ("TornadoRod", "Tornado Rod", "tornado_rod", 18, 8, -27),
    ("WaterRod", "Water Rod", "water_rod", 18, 8, -27),
    ("MeteorRod", "Meteor Rod", "meteor_rod", 18, 8, -27),
    ("StormRod", "Storm Rod", "storm_rod", 18, 8, -27),
]
STAT_NAMES = {"StatDefense", "StatSpeed", "StatPower", "StatMagic", "CrawlSpeed", "ClimbSpeed", "PushSpeed"}


def resource(name):
    if name in STAT_NAMES:
        return "textures/icon_item_static/g" + name + "Tex"
    return "textures/icon_item_custom/gItemIcon" + name + "Tex"


def png_path(name):
    return HERE / "PNGS" / (Path(resource(name)).name + ".png")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def relative(path):
    return Path(path).relative_to(ROOT).as_posix()


def texture_resource(image):
    assert image.mode == "RGBA" and image.size == (SIZE, SIZE)
    header = bytearray(64)
    header[1] = 1  # little endian, custom resource
    struct.pack_into("<IIQ", header, 4, 0x4F544558, 1, 0xDEADBEEFDEADBEEF)
    body = struct.pack("<IIIIffI", 1, SIZE, SIZE, 1, SIZE/LOGICAL_SIZE,
                       SIZE/LOGICAL_SIZE, SIZE*SIZE*4)
    return bytes(header) + body + image.tobytes()


def render_icons(only=None):
    out = HERE / "PNGS"
    out.mkdir(parents=True, exist_ok=True)
    provenance_path = HERE / "render_sources.json"
    provenance = json.loads(provenance_path.read_text()) if provenance_path.exists() else {}
    for name, title, source, azimuth, elevation, roll in ICONS:
        if only and name not in only:
            continue
        if name == "MarioMask":
            # This candidate's exact icon was retained with its runtime archive;
            # its missing authoring camera is not replaced by an older recipe.
            sys.path.insert(0, str(ROOT / "tools/nei_gi/SOURCE"))
            from mario_sm64_poc3 import restore_icon
            image, provenance[name] = restore_icon()
            image.save(png_path(name), optimize=True)
            print("restored", name, "retained POC3 pixels", flush=True)
            continue
        if source == "magic_jar_generated":
            source_path = HERE / "SOURCE/magic_jar_generated.png"
            image = Image.open(source_path).convert("RGBA").resize((SIZE, SIZE), Image.Resampling.LANCZOS)
            sources = {source_path, HERE / "SOURCE/magic_jar_prompt.txt"}
            approx = ["Generated interpretation of the green native Magic Jar; native ROM geometry unavailable."]
            triangles = None
            method = "image_gen; preserved authored RGBA source, downsampled from 1280px"
        else:
            if source.startswith("gStat"):
                model = native_model("objects/object_stat_upgrade/" + source)
                method = "native XML display-list triangles and primitive/environment materials"
            else:
                model = glb_model(ROOT / "tools/nei_gi/CHECKPOINTS" / source / (source + ".glb"))
                method = "native GI checkpoint GLB triangles and materials"
            image = render(model, size=SIZE, azimuth=azimuth, elevation=elevation, roll=roll)
            sources, approx = model.sources, sorted(model.approximations)
            triangles = sum(len(p.triangles) for p in model.parts)
        sources = set(sources) | {HERE / "build.py", HERE / "render.py"}
        image.save(png_path(name), optimize=True)
        provenance[name] = {"title": title, "method": method, "source": source,
                            "camera": {"azimuth": azimuth, "elevation": elevation, "roll": roll},
                            "triangles": triangles, "approximations": list(approx),
                            "dependencies": {relative(p): sha(p.read_bytes()) for p in sorted(sources)}}
        print("rendered", name, f"{triangles} triangles" if triangles else method, flush=True)
    provenance_path.write_text(json.dumps(provenance, indent=2, sort_keys=True) + "\n")
    if all(png_path(x[0]).exists() for x in ICONS):
        contact_sheet()


def contact_sheet():
    cell, header = 236, 90
    cols = 6
    rows = (len(ICONS) + 1 + cols - 1) // cols
    sheet = Image.new("RGB", (cell*cols, header + cell*rows), "#111922")
    draw = ImageDraw.Draw(sheet)
    font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
    font = lambda size: ImageFont.truetype(font_path, size)
    draw.text((22, 14), "NEI · HD inventory icons", fill="#ecf3fa", font=font(27))
    draw.text((22, 52), "512px RGBA · native GI geometry · Magic jar authored separately · offline lighting", fill="#b2c3d3", font=font(15))
    for i, (name, title, *_rest) in enumerate(ICONS):
        x, y = i % cols * cell, header + i // cols * cell
        # Alternating tiles make alpha/fringes and dark silhouettes reviewable.
        tile = Image.new("RGBA", (cell-12, cell-12), "#293644" if i % 2 else "#d2d9de")
        icon = Image.open(png_path(name)).convert("RGBA").resize((192, 192), Image.Resampling.LANCZOS)
        tile.alpha_composite(icon, ((tile.width-192)//2, 4))
        sheet.paste(tile.convert("RGB"), (x+6, y+6))
        draw.text((x+15, y+203), title, fill="#f7f9fa" if i % 2 else "#14202a", font=font(14))
    x, y = len(ICONS) % cols * cell, header + len(ICONS) // cols * cell
    draw.text((x+20, y+82), f"{len(ICONS)} requested icons", fill="#d8e7f4", font=font(18))
    draw.text((x+20, y+115), "32px logical slots", fill="#aebdcc", font=font(16))
    draw.text((x+20, y+142), "16× raw texture scale", fill="#aebdcc", font=font(16))
    sheet.save(HERE / "contact_sheet.png")


def write_manifest():
    records = []
    sources = json.loads((HERE / "render_sources.json").read_text())
    for name, *_ in ICONS:
        image = Image.open(png_path(name)).convert("RGBA")
        data = texture_resource(image)
        arc = resource(name)
        records.append({"name": name, "resource": arc, "logical_size": [32, 32], "size": [SIZE, SIZE],
                        "raw_flags": 1, "scale": [16., 16.], "png": relative(png_path(name)),
                        "png_sha256": sha(png_path(name).read_bytes()), "resource_sha256": sha(data),
                        "source": sources[name]})
    manifest = {"version": 1, "baseline": "122dd5f68cb37fcf515c3726f8a77043db5dcdf4", "icons": records,
                "hosts": ["soh", "mm"], "runtime_tested": False,
                "delivery": "optional asset-only archive; not included in game asset trees",
                "priority": "Identical base/Alt resources follow normal external mod load order."}
    (HERE / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"recorded {len(ICONS)} optional icons; no game assets written")


def verify_resource(data, image):
    assert len(data) == 92 + SIZE*SIZE*4
    assert data[0:2] == bytes([0, 1])
    assert struct.unpack_from("<II", data, 4) == (0x4F544558, 1)
    assert struct.unpack_from("<IIIIffI", data, 64) == (1, SIZE, SIZE, 1, 16., 16., SIZE*SIZE*4)
    assert data[92:] == image.tobytes()


def verify(archive=None):
    manifest = json.loads((HERE / "manifest.json").read_text())
    assert len(manifest["icons"]) == len(ICONS) == 23
    assert {x["resource"] for x in manifest["icons"]} == {resource(x[0]) for x in ICONS}
    for entry in manifest["icons"]:
        path = ROOT / entry["png"]
        image = Image.open(path)
        assert image.mode == "RGBA" and image.size == (SIZE, SIZE), path
        assert sha(path.read_bytes()) == entry["png_sha256"], path
        a = np.asarray(image)[:, :, 3]
        assert a.min() == 0 and a.max() == 255, (path, "needs transparent and opaque pixels")
        assert not np.any(a[:8]) and not np.any(a[-8:]) and not np.any(a[:, :8]) and not np.any(a[:, -8:]), (path, "clipped border")
        yy, xx = np.where(a > 32)
        assert max(np.ptp(xx), np.ptp(yy)) >= SIZE*.78, (path, "loose framing")
        for dependency, checksum in entry["source"]["dependencies"].items():
            assert sha((ROOT / dependency).read_bytes()) == checksum, (path, "stale source", dependency)
        data = texture_resource(image)
        assert sha(data) == entry["resource_sha256"], path
        verify_resource(data, image)
    if archive:
        with zipfile.ZipFile(archive) as packed:
            base = {resource(x[0]) for x in ICONS}
            assert len(packed.namelist()) == len(base) * 2
            assert set(packed.namelist()) == base | {"alt/" + p for p in base}
            for entry in manifest["icons"]:
                verify_resource(packed.read(entry["resource"]), Image.open(ROOT / entry["png"]))
                assert packed.read(entry["resource"]) == packed.read("alt/" + entry["resource"])
    print(f"verified {len(ICONS)} PNGs, source hashes, transparent framing and OTEX1 raw flags/scales" + (" + optional base/Alt archive" if archive else ""))


def package(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, *_ in ICONS:
            arc = resource(name)
            data = texture_resource(Image.open(png_path(name)).convert("RGBA"))
            for entry in (arc, "alt/" + arc):
                info = zipfile.ZipInfo(entry, (2026, 10, 4, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o644 << 16
                archive.writestr(info, data, compresslevel=9)
    verify(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--render", action="store_true")
    parser.add_argument("--only", nargs="+", choices=[x[0] for x in ICONS])
    parser.add_argument("--manifest", action="store_true", help="refresh the optional pack manifest without installing game assets")
    parser.add_argument("--pack", type=Path)
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    if args.render:
        render_icons(args.only)
    if args.manifest:
        write_manifest()
    if args.verify or args.manifest or args.pack or not args.render:
        verify()
    if args.pack:
        package(args.pack)


if __name__ == "__main__":
    main()
