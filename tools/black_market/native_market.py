#!/usr/bin/env python3
"""Retain POC14 market geometry with static stock-texture material bindings.

The checked-in bundle contains no texture pixels, custom keeper, title image,
cosmetics, masks or scrolling. Public room/material paths remain overrideable.
"""
import argparse
import base64
import hashlib
import io
import json
from pathlib import Path
import stat
import xml.etree.ElementTree as ET
import zipfile
import zlib

SOURCE_SHA = "3ac1432d0e8f75e51f08892739a13e16762bb4d820a2bbc7a35469af1db872e2"
PRIVATE = "custom/skiejer_market/"
MARKER = PRIVATE + "native_geometry_v1"
STOCK_TEXTURE = "scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0"
STOCK_KEEPER = tuple("objects/object_bg/" + name for name in (
    "gTreasureChestShopGalSkel", "object_bg_Anim_009890",
    "gTreasureChestShopGalEyeOpenUpTex", "gTreasureChestShopGalEyeHalfUpTex",
    "gTreasureChestShopGalEyeClosedTex", "gTreasureChestShopGalTLUT"))
SCENES = {
    "mm": "scenes/nonmq/skiejer_market/skiejer_market",
    "oot": "scenes/shared/skiejer_market_oot/skiejer_market_oot",
}
COLLISION = "scenes/nonmq/skiejer_market/collision"
BUNDLE_FILE = "resources.zlib.b64"
MAX_BUNDLE_BYTES = 8 * 1024 * 1024


def digest(data):
    return hashlib.sha256(data).hexdigest()


def retained(path, include_title=False):
    return (path.startswith(PRIVATE + "geometry/") or
            path.startswith(PRIVATE + "entrance") or path == PRIVATE + "room" or
            (include_title and path == PRIVATE + "title") or
            path in (COLLISION, *(scene + suffix for scene in SCENES.values()
                                 for suffix in ("", "_room_0"))))


def static_material(data):
    """Change only material commands; vertex UVs and mesh commands stay exact."""
    tree = ET.fromstring(data)
    if tree.tag != "DisplayList":
        raise ValueError("POC14 material must be a native XML display list")
    required = ("SetTextureImage", "LoadBlock", "SetTileSize", "SetPrimColor")
    if any(len(tree.findall(tag)) != 1 for tag in required) or len(tree.findall("SetTile")) != 2:
        raise ValueError("unexpected POC14 material texture framing")
    for node in list(tree):
        if node.tag == "ScrollTexture":
            tree.remove(node)
        for attribute in ("CosmeticEntry", "CosmeticCategory"):
            node.attrib.pop(attribute, None)
    image = tree.find("SetTextureImage")
    image.set("Path", STOCK_TEXTURE)
    image.set("Format", "G_IM_FMT_RGBA")
    image.set("Size", "G_IM_SIZ_16b_LOAD_BLOCK")
    image.set("Width", "1")
    for tile in tree.findall("SetTile"):
        tile.set("Format", "G_IM_FMT_RGBA")
        tile.set("Size", "G_IM_SIZ_16b" if tile.get("Tile") == "0" else "G_IM_SIZ_16b_LOAD_BLOCK")
        tile.set("Line", "8" if tile.get("Tile") == "0" else "0")
        for axis in ("S", "T"):
            tile.set("Mask" + axis, "5")
            tile.set("Shift" + axis, "0")
        for key in ("Cms0", "Cmt0"):
            tile.set(key, "G_TX_WRAP")
        for key in ("Cms1", "Cmt1"):
            tile.set(key, "G_TX_NOMIRROR")
    block = tree.find("LoadBlock")
    block.set("Lrs", "1023")
    block.set("Dxt", "256")
    size = tree.find("SetTileSize")
    size.set("Uls", "0")
    size.set("Ult", "0")
    size.set("Lrs", "124")
    size.set("Lrt", "124")
    color = tree.find("SetPrimColor")
    for channel in "RGBA":
        color.set(channel, "255")
    ET.indent(tree, space="  ")
    return ET.tostring(tree, encoding="utf-8") + b"\n"


def validate_resources(resources, include_title=False):
    title = PRIVATE + "title"
    if len(resources) != 125 + int(include_title) or resources.get(MARKER) != b"Native market geometry v1\n":
        raise ValueError("native market geometry resource inventory changed")
    required = {COLLISION, PRIVATE + "room", PRIVATE + "entrance"}
    required.update(scene + suffix for scene in SCENES.values() for suffix in ("", "_room_0"))
    if not required <= resources.keys():
        raise ValueError("native market scene/room/collision dependencies are missing")
    materials, external = 0, set()
    for name, data in resources.items():
        if include_title and name == title:
            from market_title import validate_texture
            validate_texture(data)
            continue
        if (name != MARKER and not retained(name, include_title)) or ".." in Path(name).parts or name.startswith("/"):
            raise ValueError("unexpected native market resource path: " + name)
        if data[4:8] in (b"XETO", b"OTEX"):
            raise ValueError("native market must not contain texture pixels")
        if not data.lstrip().startswith(b"<"):
            continue
        tree = ET.fromstring(data)
        if tree.tag not in ("DisplayList", "Vertex"):
            raise ValueError("unexpected native market XML resource: " + name)
        for node in tree.iter():
            if node.tag == "ScrollTexture" or any(key.startswith("Cosmetic") for key in node.attrib):
                raise ValueError("native market contains an optional animated/cosmetic treatment")
            path = node.get("Path")
            if path and path not in resources:
                if path != STOCK_TEXTURE:
                    raise ValueError("unresolved native market resource: " + path)
                external.add(path)
        if "/mat_" in name:
            materials += 1
            if tree.find("SetTextureImage").get("Path") != STOCK_TEXTURE:
                raise ValueError("native market material must use its stock texture binding")
    if materials != 28 or external != {STOCK_TEXTURE}:
        raise ValueError("native market material/dependency inventory changed")


def convert(source):
    source = Path(source)
    if digest(source.read_bytes()) != SOURCE_SHA:
        raise ValueError("native market donor identity differs from immutable POC14")
    resources, hashes, changed = {}, {}, []
    with zipfile.ZipFile(source) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)):
            raise ValueError("POC14 contains duplicate resource names")
        for name in sorted(names):
            if not retained(name):
                continue
            original = archive.read(name)
            data = static_material(original) if "/mat_" in name else original
            resources[name] = data
            hashes[name] = {"source_sha256": digest(original), "sha256": digest(data), "bytes": len(data)}
            if data != original:
                changed.append(name)
    resources[MARKER] = b"Native market geometry v1\n"
    validate_resources(resources)
    return resources, {
        "version": 1, "source_name": source.name, "source_sha256": SOURCE_SHA,
        "resource_count": len(resources), "material_count": len(changed),
        "external_resources": [STOCK_TEXTURE], "resources": hashes,
        "preservation": "POC14 geometry/vertices/UVs/collision/scene headers/spawns unchanged; only 28 material bindings adapted",
        "default_material": "static stock 32x32 RGBA16 bark, neutral primitive tint; no texture pixels copied",
        "runtime": "resource-verified candidate; configured game rendering/traversal unverified",
    }


def core_contract(resources, host):
    """Require this host's geometry and existing native keeper, never HD extras."""
    validate_resources(resources)
    if host not in SCENES:
        raise ValueError("native market host must be mm or oot")
    other = SCENES["oot" if host == "mm" else "mm"]
    from market_interior import resource_paths
    from market_title import resource_paths as title_paths
    return sorted((set(resources) - {other, other + "_room_0"}) | resource_paths() | title_paths() | set(STOCK_KEEPER))


def core_contract_header(resources):
    result = "#pragma once\n// Generated by tools/black_market/native_market.py; retained geometry, original surfaces and native keeper.\n"
    for host, label in (("mm", "Mm"), ("oot", "Oot")):
        result += f"inline constexpr const char* kNativeBlackMarket{label}Resources[] = {{\n"
        result += "".join(f'    "{name}",\n' for name in core_contract(resources, host))
        result += "};\n"
    return result


def write_bundle(resources, receipt, directory):
    """Store deterministic geometry sources, without an installable mod archive."""
    validate_resources(resources)
    if receipt["source_sha256"] != SOURCE_SHA:
        raise ValueError("native market bundle source identity changed")
    stream = io.BytesIO()
    with zipfile.ZipFile(stream, "w", compression=zipfile.ZIP_STORED) as archive:
        for name, data in sorted(resources.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.create_system = 3
            info.external_attr = (stat.S_IFREG | 0o644) << 16
            archive.writestr(info, data)
    decoded = stream.getvalue()
    packed = zlib.compress(decoded, 9)
    metadata = {
        "version": 1, "encoding": "base64+zlib+zip-store", "file": BUNDLE_FILE,
        "packed_bytes": len(packed), "packed_sha256": digest(packed),
        "decoded_bytes": len(decoded), "decoded_sha256": digest(decoded),
        "resources": {name: {"bytes": len(data), "sha256": digest(data)}
                      for name, data in sorted(resources.items())},
        "receipt": receipt,
    }
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    (directory / BUNDLE_FILE).write_bytes(base64.b64encode(packed))
    (directory / "manifest.json").write_text(json.dumps(metadata, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return metadata


def read_bundle(directory):
    """Validate the complete source bundle before the packer's asset tree changes."""
    directory = Path(directory)
    metadata = json.loads((directory / "manifest.json").read_text(encoding="utf-8"))
    if (metadata["version"] != 1 or metadata["encoding"] != "base64+zlib+zip-store" or
            metadata["file"] != BUNDLE_FILE or metadata["receipt"]["source_sha256"] != SOURCE_SHA or
            not 0 < metadata["decoded_bytes"] <= MAX_BUNDLE_BYTES):
        raise ValueError("unexpected native market source bundle identity/size")
    packed = base64.b64decode((directory / BUNDLE_FILE).read_bytes(), validate=True)
    if len(packed) != metadata["packed_bytes"] or digest(packed) != metadata["packed_sha256"]:
        raise ValueError("native market compressed source failed its size/hash check")
    decoder = zlib.decompressobj()
    decoded = decoder.decompress(packed, metadata["decoded_bytes"] + 1)
    if (len(decoded) != metadata["decoded_bytes"] or not decoder.eof or decoder.unused_data or
            decoder.unconsumed_tail or digest(decoded) != metadata["decoded_sha256"]):
        raise ValueError("native market decoded source failed its size/hash check")
    with zipfile.ZipFile(io.BytesIO(decoded)) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)) or set(names) != set(metadata["resources"]):
            raise ValueError("native market source resource inventory changed")
        resources = {name: archive.read(name) for name in names}
    for name, data in resources.items():
        expected = metadata["resources"][name]
        if len(data) != expected["bytes"] or digest(data) != expected["sha256"]:
            raise ValueError("native market resource failed its size/hash check: " + name)
    validate_resources(resources)
    return resources


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("--bundle-dir", type=Path, required=True)
    parser.add_argument("--contract", type=Path)
    args = parser.parse_args()
    resources, receipt = convert(args.source)
    metadata = write_bundle(resources, receipt, args.bundle_dir)
    if args.contract:
        args.contract.parent.mkdir(parents=True, exist_ok=True)
        args.contract.write_text(core_contract_header(resources), encoding="utf-8")
    print(json.dumps({"resources": len(resources), "materials": receipt["material_count"],
                      "packed_bytes": metadata["packed_bytes"], "packed_sha256": metadata["packed_sha256"]}, indent=2))


if __name__ == "__main__":
    main()
