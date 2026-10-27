#!/usr/bin/env python3
"""Check the installed market resource contract against the built O2R."""
import argparse
from pathlib import Path
import json
import re
import struct
import xml.etree.ElementTree as ET
import zipfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/black_market"))
from build import crc64

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("pack", type=Path)
args = parser.parse_args()
with zipfile.ZipFile(args.pack) as archive:
    names = set(archive.namelist())
    hashes = {crc64(n) for n in names}
    manifest = json.loads(archive.read("custom/skiejer_market/manifest.json"))
    assert set(manifest["resources"]) == names
    assert manifest["scene"] == 2 and manifest["entrance"] == 0x1200
    assert manifest["donor"] == "Djipi Granny Potion Shop"
    assert len(manifest["materials"]) == 28
    assert manifest["collision"]["triangles"] > 100
    assert manifest["collision"]["vertices"] < 8192
    # SoH resolves non-dungeon scenes through scenes/shared. Permit only the
    # two privately named native shop headers, never donor/global overrides.
    oot_headers = {"scenes/shared/skiejer_market_oot/skiejer_market_oot",
                   "scenes/shared/skiejer_market_oot/skiejer_market_oot_room_0"}
    assert not any(n.startswith(("alt/", "objects/", "scenes/shared/", "scenes/nonmq/Z2_"))
                   and n not in oot_headers for n in names)
    for material in manifest["materials"]:
        tree = ET.fromstring(archive.read(material["path"]))
        assert tree.find("ScrollTexture") is not None
        assert int(tree.find("ScrollTexture").get("SpeedX")) or int(tree.find("ScrollTexture").get("SpeedY"))
        assert tree.find("SetTextureImage").get("Path").startswith("custom/skiejer_market/")
    for name in names:
        value = archive.read(name)
        if value.lstrip().startswith(b"<"):
            for element in ET.fromstring(value).iter():
                if ET.fromstring(value).tag == "DisplayList":
                    factory = (Path(__file__).resolve().parents[2] / "libultraship/src/fast/resource/factory/DisplayListFactory.cpp").read_text()
                    assert element.tag == "DisplayList" or f'childName == "{element.tag}"' in factory, (name, element.tag)
                path = element.get("Path")
                if path and not path.startswith(">"):
                    assert path in names, (name, path)
        if "/keeper/" in name:
            assert b"objects/object_bg/" not in value, name
            if value[4:8] == b"TLDO":
                offset = 72
                while offset < len(value):
                    opcode = struct.unpack_from("<I", value, offset)[0] >> 24
                    offset += 8
                    if opcode in {0x20, 0x31, 0x32, 0x33, 0x35, 0x36, 0x42}:
                        hi, lo = struct.unpack_from("<II", value, offset)
                        assert (hi << 32 | lo) in hashes, (name, hex(hi << 32 | lo))
                        offset += 8
                    if opcode in (0xDF, 0xB8): break
                assert offset == len(value), name
    # Native spawn must enter the main room, facing forward with PLAYER_START_MODE_E.
    from build import scene_resources
    for name, expected in scene_resources().items():
        assert archive.read(name) == expected, (name, "native scene/spawn contract mismatch")
    title = archive.read("custom/skiejer_market/title")
    assert struct.unpack_from("<IIII", title, 64)[:3] == (8, 192, 24)
    assert any(title[80:])
    assert "custom/skiejer_market/keeper/ShopGalMMDBlinkHead2DL" in names
    assert "custom/skiejer_market/keeper/ShopGalMMDLevelIdleAnim" in names
    print(f"PASS: {len(names)} scoped resources; all 28 interior materials scroll; native scene/collision, title and keeper")
