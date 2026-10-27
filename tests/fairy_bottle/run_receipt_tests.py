#!/usr/bin/env python3
"""Execute fairy catalog entries against the native tables used for foreign draws.

The catalog assignments and Item constructor are production code. Native draw
tables, IDs, resource paths and export bodies are imported verbatim; the fixture
only supplies engine services and observes the chosen callback and recipe.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function


def native(host, namespace):
    source = (ROOT / host / "src/code/z_draw.c").read_text()
    start = source.index("DrawItemTableEntry sDrawItemTable[")
    table = source[start:source.index("\n};", start) + 3]
    export = function(source, "GetItem_GetDrawTableEntry")
    paths = {}
    for header in (ROOT / host / "assets/objects").rglob("*.h"):
        paths.update(re.findall(r'#define d(\w+) "([^"\n]+)"', header.read_text()))
    resources = sorted(set(re.findall(r"\b(g\w+)\b", table)))
    declarations = "\n".join('char ' + name + '[]="' + paths.get(name, name) + '";' for name in resources)
    callbacks = sorted(set(re.findall(r"GetItem_Draw\w+", table + export)))
    declarations += "\n" + "\n".join(
        'void ' + name + '(PlayState*,s16) { selectedCallback="' + name + '"; }' for name in callbacks)
    if host == "soh":
        # Native C accepts its OTR-name strings as Gfx*. C++ needs explicit casts.
        table = re.sub(r"\b(g\w+)\b", r"(Gfx*)\1", table)
        entry = "struct DrawItemTableEntry { void (*drawFunc)(PlayState*,s16); Gfx* dlists[8]; };"
    else:
        entry = "struct DrawItemTableEntry { void (*drawFunc)(PlayState*,s16); void* drawResources[8]; };"
        declarations += '\nchar gGiBlueFireChamberstickDL[]="__OTR__objects/object_gi_fire/gGiBlueFireChamberstickDL";'
    # The two real engine headers share guards; MM also publishes OoT aliases.
    # Keep those preprocessing names from changing the subsequent OoT catalog.
    undefs = '\n'.join('#undef ' + name for name in re.findall(
        r'^#define\s+(\w+)', (ROOT / host / 'include/z64item.h').read_text(), re.M))
    return '\n'.join([
        "namespace " + namespace + " {", '#include "' + host + '/include/z64item.h"',
        '#include "' + host + '/include/z64object.h"', declarations, entry, table,
        function(source, "GetItem_EmptyBottleShell"), function(source, "GetItem_FairyBottleShell"), export, "}",
        undefs, "#undef Z64OBJECT_H"])


def catalog():
    source = (ROOT / "soh/soh/Enhancements/randomizer/item.cpp").read_text()
    constructor = re.search(r"^Item::Item\(const RandomizerGet randomizerGet_, Text name_.*?^}", source, re.M | re.S)[0]
    source = (ROOT / "soh/soh/Enhancements/randomizer/item_list.cpp").read_text()
    rows = []
    for name in ("RG_EMPTY_BOTTLE", "RG_BOTTLE_WITH_FAIRY", "RG_BUY_FAIRYS_SPIRIT"):
        rows.append(re.search(r"^\s*itemTable\[" + name + r"\]\s*=.*?;", source, re.M)[0])
    manager = (ROOT / "soh/soh/Enhancements/item-tables/ItemTableManager.h").read_text()
    manager += '\n' + (ROOT / "soh/soh/Enhancements/item-tables/ItemTableManager.cpp").read_text()
    manager = '\n'.join(line for line in manager.splitlines() if not line.startswith(("#include", "#pragma")))
    vanilla = function((ROOT / "soh/soh/OTRGlobals.cpp").read_text(), "VanillaItemTable_Init")
    return (constructor + '\nvoid InitReceipts() {\n' + '\n'.join(rows) + '\n}\n' + manager +
            '\nItemTableManager* ItemTableManager::Instance=nullptr;\n' + vanilla)


def mm_receipts():
    source = (ROOT / "mm/2s2h/Rando/StaticData/Items.cpp").read_text()
    rows = []
    for name in ("RI_FAIRY_REFILL", "RI_OOT_BOTTLE_FAIRY"):
        row = re.search(r"RI\(" + name + r",[^\n]+,\s*(GID_\w+)\)", source)
        assert row, name
        rows.append('{"' + name + '",mm::' + row[1] + '}')
    return 'const MmReceipt mmReceipts[]={' + ','.join(rows) + '};\n'


def main():
    fixture = (ROOT / "tests/fairy_bottle/receipt_test.cpp").read_text()
    fixture = fixture.replace("/* PRODUCTION_NATIVE */", native("soh", "oot") + '\n' + native("mm", "mm"))
    fixture = fixture.replace("/* PRODUCTION_CATALOG */", catalog())
    fixture = fixture.replace("/* PRODUCTION_MM_RECEIPTS */", mm_receipts())
    with tempfile.TemporaryDirectory(prefix="fairy-receipts-") as temp:
        source, binary = Path(temp) / "test.cpp", Path(temp) / "test"
        source.write_text(fixture)
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra",
                        "-Wno-unused-parameter", "-Wno-unused-variable", "-Wno-sign-compare",
                        "-Wno-missing-field-initializers", "-DCOMBO_BUILD", "-I" + str(ROOT),
                        str(source), "-o", str(binary)], check=True)
        failed = []
        for case in ("randomizer-bottle", "randomizer-refill", "vanilla-fairy", "mm"):
            print("CASE " + case, flush=True)
            if subprocess.run([str(binary), case]).returncode:
                failed.append(case)
        if failed:
            raise RuntimeError("Failed fairy receipt cases: " + ", ".join(failed))


if __name__ == "__main__":
    main()
