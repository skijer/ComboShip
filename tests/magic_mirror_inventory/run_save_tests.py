#!/usr/bin/env python3
"""Round-trip both hosts' production NEI serializers, resets and Fleet option table.

Reuses the existing real-JSON SaveManager fixture, replacing only its main. No
runtime game, network transport, or whole-application save boundary is simulated.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_shared_slot"))
from run_save_tests import build_native_equipment_dependency, extract, production_for

MAIN = r'''
int main() {
#if !HOST_MM
    SaveManager manager;
    SaveManager::Instance = &manager;
    sFixtureSaveDirectory = std::getenv("NEI_SAVE_TEST_DIRECTORY");
    std::filesystem::create_directories(sFixtureSaveDirectory);
#endif
    for (uint16_t first: {uint16_t(ITEM_TIME_GATE), uint16_t(EXT_ITEM_MAGIC_MIRROR)}) {
        Reset();
        TravelTools_Grant(first);
        TravelTools_Grant(first == ITEM_TIME_GATE ? EXT_ITEM_MAGIC_MIRROR : ITEM_TIME_GATE);
        ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR - 1, 0xF123);
        ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR + 1, 0x0234);
        const auto saved = SaveBlob();
        Reset();
        LoadBlob(json::parse(saved.dump()));
        Check(Nei_Save()->timeGateOwned && Nei_Save()->magicMirrorOwned,
              "production serializer discarded one of the independent travel flags");
        TravelTools_Heal();
        Check(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == first, "selected travel tool survived native save");
        Check(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR - 1) == 0xF123 &&
              ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR + 1) == 0x0234, "neighboring u16 items survived save");
        auto legacy = saved;
        legacy.erase("timeGateOwned"); legacy.erase("magicMirrorOwned");
        LoadBlob(json::parse(legacy.dump()));
        Check(!Nei_Save()->timeGateOwned && !Nei_Save()->magicMirrorOwned, "missing flag keys default to zero");
        TravelTools_Heal();
        Check(TravelTools_IsOwned(first) && !TravelTools_IsOwned(first == ITEM_TIME_GATE ? EXT_ITEM_MAGIC_MIRROR : ITEM_TIME_GATE),
              "legacy selected travel item migrated independently");
    }
    Reset();
    Fixture_FCO_APPLY(json{{"timeGateOwned", 1}, {"magicMirrorOwned", 1}});
    Check(Nei_Save()->timeGateOwned && Nei_Save()->magicMirrorOwned, "actual option table imports independent travel flags");
    Fixture_FCO_APPLY(json{{"timeGateOwned", 0}, {"magicMirrorOwned", 0}});
    Check(Nei_Save()->timeGateOwned && Nei_Save()->magicMirrorOwned, "MAX merge retains travel flags");
    json exported = json::object();
    Fixture_FCO_EXTRACT(exported);
    Reset();
    Fixture_FCO_EXTRACT(exported);
    Fixture_FCO_APPLY(json::parse(exported.dump()));
    Check(Nei_Save()->timeGateOwned && Nei_Save()->magicMirrorOwned, "MAX export retains shared travel flags");
    TravelTools_Heal();
    LoadBlob(SaveBlob());
    Check(Nei_Save()->timeGateOwned && Nei_Save()->magicMirrorOwned, "synced flags persisted after native save");
    Reset();
    TravelTools_Heal();
    Check(!TravelTools_IsOwned(ITEM_TIME_GATE) && !MagicMirror_IsOwned(), "new save clears both flags and selected cell");
    puts(HOST_MM ? "PASS native MM travel save and reset" : "PASS native OoT travel save and reset");
}
'''
fixture = (ROOT / "tests/nei_shared_slot/save_test.cpp").read_text().split("int main() {")[0]
for host in ("mm", "soh"):
    inv = (ROOT / host / "mods/extended_inventory.c").read_text()
    production = production_for(host, None)
    production += "\n" + "\n".join(extract(inv, name) for name in (
        "TravelTools_Heal", "TravelTools_IsOwned", "TravelTools_Grant", "MagicMirror_IsOwned"))
    with tempfile.TemporaryDirectory(prefix="mirror-save-" + host + "-") as td:
        path = Path(td); cpp = path / "test.cpp"; binary = path / "test"
        (path / "nei_save_production.inc").write_text(production)
        (path / "BS_thread_pool.hpp").write_text("#pragma once\nnamespace BS { class thread_pool; }\n")
        cpp.write_text(fixture + MAIN)
        includes = ["-I" + str(p) for p in (path, ROOT, ROOT / host, ROOT / host / "include", ROOT / host / "include/PR", ROOT / host / "2s2h", ROOT / host / "assets", ROOT / "libultraship/include", ROOT / "combo")]
        flags = ["-std=c++20", "-O1", "-g", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DLOG_LEVEL_GAME_PRINTS=0", "-DHOST_MM=" + str(int(host == "mm"))]
        native_objects = build_native_equipment_dependency(host, path, includes, flags)
        subprocess.run([os.environ.get("CXX", "c++"), *flags, *includes, str(cpp), *native_objects, "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, env={**os.environ, "NEI_SAVE_TEST_DIRECTORY": str(path / "Save")}, check=True)
