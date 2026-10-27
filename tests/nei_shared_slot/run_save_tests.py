#!/usr/bin/env python3
"""Exercise both hosts' NEI persistence with their native save data and real JSON.

The complete MM NeiSaveData JSON overloads and OoT NEI save/load functions are
extracted verbatim. OoT uses the production SaveManager declaration, templates,
and array traversal; a fixture binds its current section synchronously. Trade
sidecar I/O is real, with only the application Save directory supplied by the
fixture. Thread startup and unrelated photo sidecars remain outside the boundary.

--serializer-ref compiles older serializers against the current native data
headers to demonstrate that omitting the appended flags fails these checks.
Requires the normal nlohmann/json.hpp build dependency (or CPATH).
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def source_at(path, ref=None):
    if ref:
        return subprocess.check_output(["git", "show", f"{ref}:{path}"], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def extract(source, name):
    match = re.search(r'^(?:extern "C" )?(?:static )?[\w: *]+\b' + re.escape(name)
                      + r"\s*\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError(f"Production function not found: {name}")
    return block_from(source, match.start())


def option_block(source, name):
    start = source.index(f"#define {name}(")
    end = source.index(f"#undef {name}", start) + len(f"#undef {name}")
    return source[start:end]


def native_equipment_declarations():
    variables = source_at("mm/include/variables.h")
    functions = source_at("mm/include/functions.h")
    declarations = [re.search(r"^extern [^;\n]+\b" + name + r"\[\];$", variables, re.M).group()
                    for name in ("gEquipMasks", "gEquipNegMasks", "gEquipShifts")]
    declarations.append(re.search(r"^void Inventory_ChangeEquipment\([^;\n]+;$", functions, re.M).group())
    return ('#ifdef __cplusplus\nextern "C" {\n#endif\n' + "\n".join(declarations) +
            "\n#ifdef __cplusplus\n}\n#endif\n")


def build_native_equipment_dependency(host, build, includes, flags):
    """Link MM's exact native nibble writer/data, retaining their C semantics.

    The native negative-mask initializers intentionally convert signed constants
    to u16. Compile them as C, not modified C++ initializers or a fake setter.
    The real save context remains supplied by the headless persistence fixture.
    """
    if host != "mm":
        return []
    inventory = source_at("mm/src/code/z_inventory.c")
    parts = ['#include "z64save.h"\n#include "z64item.h"\n#include "macros.h"\n',
             native_equipment_declarations()]
    for marker in ("u16 gEquipMasks[]", "u16 gEquipNegMasks[]", "u8 gEquipShifts[]"):
        start = inventory.index(marker)
        end = inventory.index("\n};", start) + len("\n};")
        parts.append(inventory[start:end])
    parts.append(extract(inventory, "Inventory_ChangeEquipment"))
    source = build / "native_equipment.c"
    source.write_text("\n\n".join(parts) + "\n")
    obj = build / "native_equipment.o"
    cflags = [flag for flag in flags if not flag.startswith("-std=")]
    subprocess.run([os.environ.get("CC", "cc"), "-std=gnu11", *cflags, *includes,
                    "-c", str(source), "-o", str(obj)], cwd=ROOT, check=True)
    return [str(obj)]


def production_for(host, serializer_ref, init_ref=None):
    save = source_at(f"{host}/mods/nei_save.cpp")
    initializers = source_at(f"{host}/mods/nei_save.cpp", init_ref)
    inventory = source_at(f"{host}/mods/extended_inventory.c")
    names = ("Nei_Save", "Nei_GetOwnedItem", "Nei_SetOwnedItem")
    parts = [extract(save, name) for name in names]
    if host == "mm":
        # ExtEquip_RemoveItem also releases the equivalent native MM shield.
        # Its actual declarations precede the verbatim ownership functions;
        # build_native_equipment_dependency supplies their native C definitions.
        parts.insert(0, native_equipment_declarations())
        parts.append(extract(initializers, "Nei_InitNewSave"))
        conversions = source_at("mm/2s2h/BenJsonConversions.hpp", serializer_ref)
        for signature in ("inline void to_json(json& j, const NeiSaveData& n)",
                          "inline void from_json(const json& j, NeiSaveData& n)"):
            parts.append(block_from(conversions, conversions.index(signature)))
        fleet = source_at("mm/2s2h/FleetShipCombo/FleetSync.cpp")
    else:
        parts.insert(0, "static NeiSaveData gNeiSave;")
        parts.append(extract(initializers, "NeiSave_Init"))
        serializers = source_at("soh/mods/nei_save.cpp", serializer_ref)
        parts.extend(extract(serializers, name) for name in
                     ("Nei_SidecarPath", "TradeItems_SyncWrite", "TradeItems_SyncRead"))
        parts.extend(extract(serializers, name) for name in ("NeiSave_Save", "NeiSave_Load"))
        manager = source_at("soh/soh/SaveManager.cpp")
        parts.extend(extract(manager, name) for name in ("SaveManager::SaveArray", "SaveManager::LoadArray"))
        fleet = source_at("soh/soh/FleetShipCombo/FleetSync.cpp")
    equipment = source_at(f"{host}/mods/extended_equipment.c")
    trade = source_at(f"{host}/mods/items/logic/trade_items.c")
    for definition, source in (("TRADE_ADULT_PENDANT", equipment), ("TRADE_ADULT_COUNT", trade)):
        parts.append(re.search(r"^#define " + definition + r"\s+.*$", source, re.M).group())
    parts.append(extract(trade, "TradeAdult_GiveIndex"))
    parts.append(extract(equipment, "ExtEquip_SetCurrentByType"))
    # Keep the real persisted extended-slot writer. Engine behavior cleanup,
    # vanilla-base equipment updates and player/render refresh remain outside
    # this fixture's trade and extended-ownership boundary.
    parts.append("void ExtEquip_SetSlot(s16 type, u8 index) {\n"
                 "    assert(index == 0); ExtEquip_SetCurrentByType(type, index);\n}")
    parts.extend(extract(equipment, name) for name in
                 ("ExtEquip_GetBit", "ExtEquip_HasItem", "ExtEquip_GiveItem", "ExtEquip_GetCurrent",
                  "ExtEquip_RemoveItem", "ExtEquip_Init"))
    parts.extend(extract(inventory, name) for name in
                 ("GraceHourglass_Heal", "GraceHourglass_IsOwned", "GraceHourglass_Grant",
                  "Slate_RuneOwned", "Slate_RuneCount", "Slate_RuneAt", "Slate_GetRune", "Slate_SetRune"))
    for name in ("FCO_EXTRACT", "FCO_APPLY"):
        qualifier = "" if name == "FCO_EXTRACT" else "const "
        parts.append(f"static void Fixture_{name}({qualifier}json& sh) {{\n"
                     "    auto* nei = Nei_Save();\n" + option_block(fleet, name) + "\n}")
    return "\n\n".join(parts) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--serializer-ref", help="git revision supplying only the NEI serializers")
    parser.add_argument("--init-ref", help="git revision supplying only the new-save initializers")
    parser.add_argument("--combo-save", type=Path, help="verify a supplied repaired combined save has no phantom trade items")
    parser.add_argument("--host", choices=("mm", "soh"), help="test only one host")
    parser.add_argument("--sanitizers", action="store_true", help="enable AddressSanitizer and UBSan")
    parser.add_argument("--standalone", action="store_true", help="test the non-ComboShip build")
    args = parser.parse_args()
    hosts = (args.host,) if args.host else ("mm", "soh")
    failed = False
    for host in hosts:
        print(f"Testing {host}; serializers: {args.serializer_ref or 'working tree'}; "
              f"initializers: {args.init_ref or 'working tree'}", flush=True)
        with tempfile.TemporaryDirectory(prefix=f"nei-save-{host}-") as directory:
            build = Path(directory)
            (build / "nei_save_production.inc").write_text(production_for(host, args.serializer_ref, args.init_ref))
            # SaveManager holds only a shared_ptr to this type here. The fixture
            # constructor does not create threads or register unrelated sections.
            (build / "BS_thread_pool.hpp").write_text("#pragma once\nnamespace BS { class thread_pool; }\n")
            includes = ["-I" + str(p) for p in (build, ROOT, ROOT / host, ROOT / host / "include",
                        ROOT / host / "include/PR", ROOT / host / "2s2h", ROOT / host / "assets",
                        ROOT / "libultraship/include", ROOT / "combo")]
            flags = ["-std=c++20", "-O1", "-g", "-DF3DEX_GBI_2",
                     "-DLOG_LEVEL_GAME_PRINTS=0", "-DHOST_MM=" + str(int(host == "mm"))]
            if not args.standalone:
                flags.append("-DCOMBO_BUILD")
            if args.sanitizers:
                flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
            native_objects = build_native_equipment_dependency(host, build, includes, flags)
            binary = build / "nei_save_test"
            subprocess.run([os.environ.get("CXX", "c++"), *flags, *includes,
                            str(ROOT / "tests/nei_shared_slot/save_test.cpp"), *native_objects, "-o", str(binary)],
                           cwd=ROOT, check=True)
            result = subprocess.run([str(binary)], cwd=ROOT,
                                    env={**os.environ, "NEI_SAVE_TEST_DIRECTORY": str(build / "Save"),
                                         **({"NEI_SAVE_TEST_COMBO_SAVE": str(args.combo_save.resolve())}
                                            if args.combo_save else {})})
            failed |= result.returncode != 0
    return int(failed)


if __name__ == "__main__":
    sys.exit(main())
