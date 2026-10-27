#!/usr/bin/env python3
"""Format the real custom pickup catalogs with the production CustomMessage class."""
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def initializer(source, name):
    start = source.index("{", source.index(name))
    tokens = re.finditer(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/|.', source[start:], re.S)
    depth = 0
    for token in tokens:
        value = token.group()
        if value == "{":
            depth += 1
        elif value == "}":
            depth -= 1
            if not depth:
                return source[start + 1:start + token.start()]
    raise AssertionError("Unterminated catalog: " + name)


def fields(source):
    result, current, depth = [], [], 0
    for token in re.findall(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*.*?\*/|.', source, re.S):
        if token.startswith(("//", "/*")):
            continue
        if token in ("{", "[", "("):
            depth += 1
        elif token in ("}", "]", ")"):
            depth -= 1
        if token == "," and not depth:
            result.append("".join(current).strip())
            current = []
        else:
            current.append(token)
    if "".join(current).strip():
        result.append("".join(current).strip())
    return result


def main():
    rows = []
    registry = (ROOT / "soh/soh/Enhancements/randomizer/randomizer.cpp").read_text()
    builder = (ROOT / "soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp").read_text()
    registry_type = re.search(r'struct CustomItemMessageEntry\s*\{\s*s16 rgId;\s*(\w+) itemId;', registry)[1]
    builder_type = re.search(r'struct CustomItemMessageEntry\s*\{\s*s16 rgId;\s*(\w+) itemId;', builder)[1]
    assert registry_type == builder_type, "Receipt registry declarations must agree"
    format_call = re.search(r'msg\.AutoFormat\([^;]*customMsg->itemId[^;]*;', builder)[0]
    custom = initializer(registry, "customItemMessages[] =")
    for row in fields(custom):
        rg, icon, english, german, french = fields(row[1:-1])
        rows.append((rg, icon, english, german, french))
    nei = initializer((ROOT / "soh/mods/extended_player.c").read_text(), "sNeiItems[] =")
    for row in fields(nei):
        values = fields(row[1:-1])
        assert len(values) == 13
        icon, rg, english, french, german = values[0], *values[9:]
        if rg == "NEI_NO_RG" or english == "NULL":
            continue
        rows.append((rg, icon, english, english if german == "NULL" else german,
                     english if french == "NULL" else french))
    assert len(rows) > 50, "Do not silently lose the real pickup catalogs"
    definitions = "\n".join(re.findall(r'^#define (?:ITEM_EXT_|EXT_ITEM_)\w+ 0x[0-9A-Fa-f]+',
        (ROOT / "soh/mods/extended_equipment.h").read_text() + "\n" +
        (ROOT / "soh/mods/extended_inventory.h").read_text(), re.M))
    definitions += "\n" + re.search(r'^#define NEI_NO_ITEM[^\n]+',
                                    (ROOT / "soh/mods/extended_inventory.h").read_text(), re.M)[0]
    catalog = definitions + "\nstatic const CatalogEntry catalog[] = {\n" + "\n".join(
        "{" + json.dumps(rg) + ", static_cast<" + registry_type + ">(" + icon + "), " +
        ", ".join((en, de, fr)) + "}," for rg, icon, en, de, fr in rows) + "\n};\n"
    magic = (ROOT / "combo/menu/ComboMagicItemReceiptText.h").read_text()
    magic_names = ['"Elemental Wand"'] + [fields(row[1:-1])[0]
        for row in fields(initializer(magic, "static const Entry entries[] ="))]
    catalog += "static const char* magicNames[] = {" + ",".join(magic_names) + "};\n"
    magic_call = re.search(r'msg\.AutoFormat\([^;]*magic->slate[^;]*;', builder)[0]
    flags = ["-std=c++20", "-O1", "-g", "-ffunction-sections", "-fdata-sections",
             "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DSOH_BUILD_DLL", "-DFMT_HEADER_ONLY",
             "-DIMGUI_DEFINE_MATH_OPERATORS", "-DLOG_LEVEL_GAME_PRINTS=0", "-Wl,--gc-sections"]
    prefixes = (ROOT / "CMake/soh-cvars.cmake").read_text()
    flags += ['-D' + key + '="' + value + '"' for key, value in
              re.findall(r'set\((CVAR_PREFIX_\w+)\s+"([^"]+)"\)', prefixes)]
    if "--sanitize" in sys.argv:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                  "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    flags += ["-I" + str(ROOT / p) for p in (".", "soh", "soh/include", "soh/include/PR",
              "soh/assets", "soh/soh", "libultraship/include", "libultraship/src", "combo", "combo/menu")]
    with tempfile.TemporaryDirectory(prefix="pickup-catalog-") as directory:
        build = Path(directory)
        (build / "pickup_catalog.inc").write_text(catalog)
        fixture = (ROOT / "tests/item_receipts/pickup_catalog_test.cpp").read_text()
        alias = "using CatalogIconType = int32_t; // CATALOG_ICON_TYPE"
        assert fixture.count(alias) == 1
        fixture = fixture.replace(alias, "using CatalogIconType = " + registry_type + ";")
        fixture = fixture.replace("/* CATALOG_AUTO_FORMAT */",
                                  format_call.replace("msg.", "message.").replace("customMsg->itemId", "icon"))
        fixture = fixture.replace("/* MAGIC_AUTO_FORMAT */", magic_call.replace("msg.", "message."))
        (build / "catalog.cpp").write_text(fixture)
        binary = build / "catalog"
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags, "-I" + str(build),
                        str(build / "catalog.cpp"),
                        str(ROOT / "soh/soh/Enhancements/custom-message/CustomMessageManager.cpp"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
