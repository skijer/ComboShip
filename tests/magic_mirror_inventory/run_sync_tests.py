#!/usr/bin/env python3
"""Execute the real FleetSync travel cell merge and native option table for both hosts."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_shared_slot"))
from run_sync_tests import block, macro, PREFIX

CHECKS = r'''
Shared incomingTimeGate() {
    Shared sh;
    Value owned; owned.array = true; owned.entries.assign(48, ITEM_NONE);
    owned.entries.at(7) = CANONICAL_TIME_GATE; // host-native enum translated to canonical OoT ID
    sh.entries["ownedItems"] = owned;
    sh["timeGateOwned"] = 1;
    return sh;
}
void both(uint16_t selected) {
    assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == selected);
    assert(TravelTools_IsOwned(ITEM_TIME_GATE) && MagicMirror_IsOwned());
    assert(state.timeGateOwned && state.magicMirrorOwned);
}
int main() {
    for (uint16_t selected: {uint16_t(ITEM_TIME_GATE), uint16_t(EXT_ITEM_MAGIC_MIRROR)}) {
        reset();
        TravelTools_Grant(selected);
        TravelTools_Grant(selected == ITEM_TIME_GATE ? EXT_ITEM_MAGIC_MIRROR : ITEM_TIME_GATE);
        assert(FindInvSlot(ITEM_TIME_GATE) == EXPECTED_TRAVEL_SLOT);
        assert(FindInvSlot(EXT_ITEM_MAGIC_MIRROR) == EXPECTED_TRAVEL_SLOT &&
               "equip restoration dropped an owned travel tool hidden behind its selected sibling");
        assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == selected);
    }
    reset();
    assert(FindInvSlot(EXT_ITEM_MAGIC_MIRROR) == -1);
    assert(FindInvSlot(ITEM_TIME_GATE) == -1);
    Shared mirror; mirror["magicMirrorOwned"] = 1;
    reset();
    ApplySharedCells(mirror);
    assert(MagicMirror_IsOwned() && "Fleet merge discarded independent Mirror ownership");
    assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == EXT_ITEM_MAGIC_MIRROR);
    ApplySharedCells(incomingTimeGate());
    both(EXT_ITEM_MAGIC_MIRROR);
    ApplySharedCells(incomingTimeGate());
    both(EXT_ITEM_MAGIC_MIRROR);
    for (uint8_t flag: {uint8_t(0), uint8_t(1)}) {
        reset();
        ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR, EXT_ITEM_MAGIC_MIRROR);
        state.magicMirrorOwned = flag;
        ApplySharedCells(incomingTimeGate());
        both(EXT_ITEM_MAGIC_MIRROR);
    }
    reset();
    ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR, ITEM_TIME_GATE);
    ApplySharedCells(mirror);
    both(ITEM_TIME_GATE);
    for (uint16_t legacy: {uint16_t(ITEM_TIME_GATE), uint16_t(EXT_ITEM_MAGIC_MIRROR)}) {
        reset();
        ExtInv_SetSlotItem(SLOT_MAGIC_MIRROR, legacy);
        Shared exported;
        ExtractSharedFlags(exported);
        assert(exported[legacy == ITEM_TIME_GATE ? "timeGateOwned" : "magicMirrorOwned"].integer == 1);
        reset();
        ApplySharedCells(exported);
        assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == legacy);
    }
    reset();
    Shared oldPeer; oldPeer["timeGateOwned"] = 1; oldPeer["magicMirrorOwned"] = 1;
    ExtractSharedFlags(oldPeer);
    assert(oldPeer["timeGateOwned"].integer == 1 && oldPeer["magicMirrorOwned"].integer == 1);
    ApplySharedCells(oldPeer);
    both(ITEM_TIME_GATE);
    Shared downgrade; downgrade["timeGateOwned"] = 0; downgrade["magicMirrorOwned"] = 0;
    ApplySharedCells(downgrade);
    both(ITEM_TIME_GATE);
    reset();
    Shared unrelated;
    Value owned; owned.array = true; owned.entries.assign(48, ITEM_NONE);
    owned.entries.at(0) = FC_OOT_PAGE2_FIRST;
    unrelated.entries["ownedItems"] = owned;
    ApplySharedCells(unrelated);
    assert(!TravelTools_IsOwned(ITEM_TIME_GATE) && !MagicMirror_IsOwned());
    assert(ExtInv_GetSlotItem(SLOT_MAGIC_MIRROR) == ITEM_NONE);
    puts("PASS travel sync: independent MAX flags, both orders, old saves, selected Mirror retained, unrelated cells unchanged");
}
'''

for host, fleet in (("mm", "mm/2s2h/FleetShipCombo"), ("soh", "soh/soh/FleetShipCombo")):
    inv = (ROOT / host / "mods/extended_inventory.c").read_text()
    sync = (ROOT / fleet / "FleetSync.cpp").read_text()
    helpers = "\n".join(block(inv, marker) for marker in (
        "void GraceHourglass_Heal(", "uint8_t GraceHourglass_IsOwned(", "void GraceHourglass_Grant(",
        "void TravelTools_Heal(", "uint8_t TravelTools_IsOwned(", "void TravelTools_Grant(", "uint8_t MagicMirror_IsOwned("))
    repair = block(sync, "static void RepairFlagOwnedCells(")
    owned = block(sync, '    if (sh.contains("ownedItems") && sh["ownedItems"].is_array())')
    apply = "void ApplySharedCells(const Shared& sh) { NeiSaveData* nei = Nei_Save();\n" + owned + "\n" + macro(sync, "FCO_APPLY") + "\nRepairFlagOwnedCells(nei);\n}\n"
    extract = "void ExtractSharedFlags(Shared& sh) { NeiSaveData* nei = Nei_Save(); RepairFlagOwnedCells(nei);\n" + macro(sync, "FCO_EXTRACT") + "\n}\n"
    slot_lookup = block(sync, "int FindInvSlot(")
    defines = '''
#ifndef ARRAY_COUNT
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#endif
constexpr int kSlotOcarina = 0;
struct { NativeInventory inventory; } gSaveContext;
#include "combo/gameplay/MagicMirrorItem.h"
#define SLOT_PHANTOM_HOURGLASS 41
#define EXT_ITEM_PHANTOM_HOURGLASS 0x0221
'''
    with tempfile.TemporaryDirectory(prefix="mirror-sync-" + host + "-") as td:
        path = Path(td); cpp = path / "test.cpp"; binary = path / "test"
        canonical = "FcEquip_MmToOot(ITEM_TIME_GATE)" if host == "mm" else "ITEM_TIME_GATE"
        expected = "SLOT_MAGIC_MIRROR + 48" if host == "mm" else "SLOT_MAGIC_MIRROR"
        cpp.write_text(PREFIX + defines + "\n#define CANONICAL_TIME_GATE " + canonical +
                       "\n#define EXPECTED_TRAVEL_SLOT " + expected + "\n" + helpers + repair + slot_lookup + apply + extract + CHECKS)
        includes = ["-I" + str(ROOT / p) for p in (".", host, host + "/include", host + "/include/PR", "libultraship/include", "combo", "combo/menu", fleet)]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *includes, str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print("PASS sync host:", host, flush=True)
