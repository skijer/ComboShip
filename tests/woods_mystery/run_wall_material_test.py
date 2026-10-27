#!/usr/bin/env python3
"""Check the baked wall's native texture extent, then execute its renderer load/decode."""
import argparse
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
WALL = "custom/cor_woods_bridge/wall"
TEXTURE = "scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00A1C0"


def function(source, name):
    pattern = r"^(?:static )?(?:uint32_t|bool|void)\s+" + re.escape(name) + r"\([^;{}]*\)\s*\{"
    match = re.search(pattern, source, re.M)
    assert match, name
    masked = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',
                    lambda m: " " * len(m.group()), source, flags=re.S)
    end, depth = match.end(), 1
    while depth:
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    return source[match.start():end] + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, help="Read the actual shipped wall from a 2ship.o2r")
    parser.add_argument("--native", type=Path, help="Decode original pixels from a native MM resource archive")
    args = parser.parse_args()
    if args.archive:
        with zipfile.ZipFile(args.archive) as archive:
            wall = archive.read(WALL)
    else:
        wall = (ROOT / "mm/assets/custom" / WALL).read_bytes()
    # The default fixture records only the native format/dimensions, without game pixels.
    width, height, size = 32, 32, 2048
    pixels = bytes((i * 37 + 11) & 255 for i in range(size))
    if args.native:
        with zipfile.ZipFile(args.native) as archive:
            data = archive.read(TEXTURE)
        assert struct.unpack_from("<I", data, 8)[0] == 0
        fmt, width, height, size = struct.unpack_from("<IIII", data, 64)
        assert (fmt, width, height, size) == (2, 32, 32, 2048)
        pixels = data[80:]
        assert len(pixels) == size
    material = []
    wall_xml = ET.fromstring(wall)
    assert len(wall_xml.findall("SetTextureImage")) == len(wall_xml.findall("LoadBlock")) == 1
    for element in wall_xml:
        if element.tag == "LoadVertices":
            break
        material.append(element)
    image = next(e for e in material if e.tag == "SetTextureImage")
    assert image.get("Path") == TEXTURE
    assert image.get("Format") == "G_IM_FMT_RGBA"
    assert image.get("Size") == "G_IM_SIZ_16b_LOAD_BLOCK"
    load = next(e for e in material if e.tag == "LoadBlock")
    tile = next(e for e in material if e.tag == "SetTile" and e.get("Tile") == "0")
    region = next(e for e in material if e.tag == "SetTileSize")
    requested = (int(load.get("Lrs")) + 1) * 2
    # Fail before executing an invalid memory read; the old shipped wall requests8192 from2048.
    assert requested == size, f"wall requests {requested} bytes from a {size}-byte native texture"
    assert int(tile.get("Line")) * 8 == width * 2
    assert (int(region.get("Lrs")) // 4 + 1, int(region.get("Lrt")) // 4 + 1) == (width, height)
    assert (int(tile.get("MaskS")), int(tile.get("MaskT"))) == (5, 5)
    renderer = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    header = (ROOT / "libultraship/include/fast/interpreter.h").read_text()
    metadata = re.search(r"struct RawTexMetadata \{.*?\n\};", header, re.S).group(0)
    rdp = header[header.index("struct RDP {"):header.index("    uint8_t first_tile_index;")] + "};\n"
    names = ["GetEffectiveLineSize", "GetTileSizeFromCoordinates", "IsPyramidLike",
             "Interpreter::GfxDpSetTile", "Interpreter::GfxDpLoadBlock", "Interpreter::ImportTextureRgba16"]
    with tempfile.TemporaryDirectory(prefix="woods-wall-material-") as directory:
        build = Path(directory)
        (build / "wall_layout.inc").write_text(metadata + "\n" + rdp)
        (build / "wall_renderer.inc").write_text("\n".join(function(renderer, name) for name in names))
        (build / "spdlog").mkdir()
        (build / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n")
        (build / "pixels.bin").write_bytes(pixels)
        calls = []
        for element in material:
            a = element.attrib
            if element.tag == "SetTile":
                fields = [a["Format"], a["Size"], a["Line"], a["TMem"], a["Tile"], a["Palette"],
                          a["Cmt0"] + "|" + a["Cmt1"], a["MaskT"], a["ShiftT"],
                          a["Cms0"] + "|" + a["Cms1"], a["MaskS"], a["ShiftS"]]
                calls.append("renderer.GfxDpSetTile(" + ",".join(fields) + ");")
            elif element.tag == "LoadBlock":
                calls.append("renderer.GfxDpLoadBlock(" + ",".join(a[k] for k in ("Tile", "Uls", "Ult", "Lrs", "Dxt")) + ");")
            elif element.tag == "SetTileSize":
                for key, member in (("Uls", "uls"), ("Ult", "ult"), ("Lrs", "lrs"), ("Lrt", "lrt")):
                    calls.append(f'renderer.state.texture_tile[{a["T"]}].{member}={a[key]};')
        (build / "wall_commands.inc").write_text("\n".join(calls))
        binary = build / "test"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g",
                        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-DF3DEX_GBI_2",
                        "-I" + str(build), "-I" + str(ROOT / "libultraship/include"),
                        "-I" + str(ROOT / "mm/include"), "-I" + str(ROOT / "mm/include/PR"),
                        str(Path(__file__).with_name("wall_material_test.cpp")),
                        str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
                        str(ROOT / "libultraship/src/fast/resource/type/Texture.cpp"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary), str(build / "pixels.bin")], check=True,
                       env=dict(os.environ, ASAN_OPTIONS="detect_leaks=0", UBSAN_OPTIONS="halt_on_error=1"))


if __name__ == "__main__":
    main()
