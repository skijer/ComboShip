#!/usr/bin/env python3
"""Check production FleetSync selection/ownership blocks with a small JSON API adapter.

This executes the real ownedItems apply block, FCO apply/extract macros and
Grace/hourglass grant/heal helpers. It does not test JSON parsing, whole-save
serialization, network transport or native gameplay.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def block(text, marker):
    start = text.index(marker)
    opening = text.index("{", start)
    depth = 0
    for end in range(opening, len(text)):
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1
            if depth == 0:
                return text[start:end + 1]
    raise ValueError("Unclosed production block: " + marker)


def macro(text, name):
    start = text.index("#define " + name + "(")
    ending = "#undef " + name
    return text[start:text.index(ending, start) + len(ending)]


PREFIX = r'''
#include "mods/nei_save.h"
#include "gameplay/MagicMirrorItem.h"
#include "z64item.h"
#include "FleetComboIds.h"
#include "FleetComboOptions.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
static NeiSaveData state{};
extern "C" NeiSaveData* Nei_Save() { return &state; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return state.ownedItems[slot - 24]; }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { state.ownedItems[slot - 24] = item; }
uint16_t ExtInv_GetSlotItem(int slot) { return Nei_GetOwnedItem(slot); }
void ExtInv_SetSlotItem(int slot, uint16_t item) { Nei_SetOwnedItem(slot, item); }
// API-only adapter: production blocks below are copied verbatim from FleetSync.
struct Value {
    bool array = false;
    int integer = 0;
    std::vector<int> entries;
    Value() = default;
    Value(int v): integer(v) {}
    bool is_array() const { return array; }
    bool is_number_integer() const { return !array; }
    size_t size() const { return entries.size(); }
    Value operator[](size_t i) const { return entries.at(i); }
    template<class T> T get() const { return static_cast<T>(integer); }
};
struct Shared {
    std::map<std::string, Value> entries;
    bool contains(const char* key) const { return entries.contains(key); }
    const Value& operator[](const char* key) const { return entries.at(key); }
    Value& operator[](const char* key) { return entries[key]; }
    int value(const char* key, int fallback) const {
        return contains(key) ? entries.at(key).integer : fallback;
    }
};
struct NativeInventory { uint8_t items[48]{}; } native;
#define MM_INV native
void reset() {
    state = {};
    for (auto& item: state.ownedItems) item = ITEM_NONE;
    for (auto& item: native.items) item = ITEM_NONE;
}
Shared incomingGrace() {
    Shared sh;
    Value owned; owned.array = true; owned.entries.assign(48, ITEM_NONE);
    owned.entries.at(17) = 0xA1; // Canonical OoT ITEM_HYLIAS_GRACE, unchanged across hosts.
    sh.entries["ownedItems"] = owned;
    sh["hyliasGraceOwned"] = 1;
    return sh;
}
'''

CHECKS = r'''
void assertBoth(uint16_t selected) {
    assert(ExtInv_GetSlotItem(SLOT_PHANTOM_HOURGLASS) == selected);
    assert(GraceHourglass_IsOwned(ITEM_HYLIAS_GRACE));
    assert(GraceHourglass_IsOwned(EXT_ITEM_PHANTOM_HOURGLASS));
    assert(state.hyliasGraceOwned == 1 && state.phantomHourglassOwned == 1);
}
int main() {
    // Both modern and legacy hourglass selections survive incoming canonical Grace.
    for (uint8_t flag : {uint8_t(0), uint8_t(1)}) {
        reset();
        ExtInv_SetSlotItem(SLOT_PHANTOM_HOURGLASS, EXT_ITEM_PHANTOM_HOURGLASS);
        state.phantomHourglassOwned = flag;
        const auto sh = incomingGrace();
        ApplySharedCells(sh);
        assertBoth(EXT_ITEM_PHANTOM_HOURGLASS);
        ApplySharedCells(sh);
        assertBoth(EXT_ITEM_PHANTOM_HOURGLASS);
    }
    // Legacy Grace retains its identity while incoming flags grant the hourglass.
    reset();
    ExtInv_SetSlotItem(SLOT_PHANTOM_HOURGLASS, ITEM_HYLIAS_GRACE);
    Shared hourglass; hourglass["phantomHourglassOwned"] = 1;
    ApplySharedCells(hourglass);
    assertBoth(ITEM_HYLIAS_GRACE);
    // Empty new saves work for each first-acquisition order through sync.
    reset();
    ApplySharedCells(hourglass);
    assert(ExtInv_GetSlotItem(SLOT_PHANTOM_HOURGLASS) == EXT_ITEM_PHANTOM_HOURGLASS);
    ApplySharedCells(incomingGrace());
    assertBoth(EXT_ITEM_PHANTOM_HOURGLASS);
    reset();
    ApplySharedCells(incomingGrace());
    assert(ExtInv_GetSlotItem(SLOT_PHANTOM_HOURGLASS) == ITEM_HYLIAS_GRACE);
    ApplySharedCells(hourglass);
    assertBoth(ITEM_HYLIAS_GRACE);
    // Retained hourglass ownership repairs a cleared cell before the sibling grant.
    reset();
    state.phantomHourglassOwned = 1;
    ApplySharedCells(incomingGrace());
    assertBoth(EXT_ITEM_PHANTOM_HOURGLASS);
    // Actual FCO macros carry both flags, backfill legacy ownership, and retain MAX values.
    for (uint16_t legacy: {uint16_t(ITEM_HYLIAS_GRACE), uint16_t(EXT_ITEM_PHANTOM_HOURGLASS)}) {
        reset();
        ExtInv_SetSlotItem(SLOT_PHANTOM_HOURGLASS, legacy);
        Shared exported;
        ExtractSharedFlags(exported);
        assert(exported[legacy == ITEM_HYLIAS_GRACE ? "hyliasGraceOwned" : "phantomHourglassOwned"].integer == 1);
        reset();
        ApplySharedCells(exported);
        assert(ExtInv_GetSlotItem(SLOT_PHANTOM_HOURGLASS) == legacy);
    }
    reset();
    Shared previous; previous["hyliasGraceOwned"] = 1; previous["phantomHourglassOwned"] = 1;
    ExtractSharedFlags(previous);
    assert(previous["hyliasGraceOwned"].integer == 1 && previous["phantomHourglassOwned"].integer == 1);
    ApplySharedCells(previous);
    assertBoth(ITEM_HYLIAS_GRACE);
    // Ordinary page-2 cells still use the existing native translation/store path.
    reset();
    Shared ordinary;
    Value owned; owned.array = true; owned.entries.assign(48, ITEM_NONE);
    owned.entries.at(0) = FC_OOT_PAGE2_FIRST;
    ordinary.entries["ownedItems"] = owned;
    ApplySharedCells(ordinary);
    assert(state.ownedItems[0] == EXPECTED_ORDINARY);
    assert(state.ownedItems[17] == ITEM_NONE && !state.hyliasGraceOwned && !state.phantomHourglassOwned);
    puts("PASS sync: selected sibling, both orders, legacy migration, empty/cleared cells, FCO flags and ordinary cells");
}
'''


def main():
    parents = Path(__file__).resolve().parents
    inferred = parents[2] if len(parents) > 2 else Path.cwd()
    default = inferred if (inferred / "mm/mods/extended_inventory.c").is_file() else Path.cwd()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=default)
    parser.add_argument("--revision", help="Extract source from this git revision; compile against current headers")
    args = parser.parse_args()
    root = args.repo.resolve()
    def read(path):
        if args.revision:
            return subprocess.check_output(["git", "show", args.revision + ":" + path], cwd=root, text=True)
        return (root / path).read_text()
    for host, fleet in (("mm", "mm/2s2h/FleetShipCombo"), ("soh", "soh/soh/FleetShipCombo")):
        inventory = read(host + "/mods/extended_inventory.c")
        header = read(host + "/mods/extended_inventory.h") + read(host + "/include/z64item.h")
        defines = ""
        for name in ("SLOT_PHANTOM_HOURGLASS", "SLOT_SHOVEL", "SLOT_ROD_OF_SEASONS", "SLOT_SHADOW_CRYSTAL",
                     "EXT_ITEM_PHANTOM_HOURGLASS", "EXT_ITEM_SHEIKAH_SLATE", "EXT_ITEM_ROD_OF_SEASONS"):
            definition = re.search(r"^#define " + name + r"\b[^\n]*", header, re.MULTILINE)
            if not definition:
                raise ValueError("Missing production constant: " + name)
            defines += "\n#ifndef " + name + "\n" + definition.group(0) + "\n#endif\n"
        sync = read(fleet + "/FleetSync.cpp")
        helpers = "\n".join(block(inventory, marker) for marker in (
            "void GraceHourglass_Heal(", "uint8_t GraceHourglass_IsOwned(", "void GraceHourglass_Grant(",
            "void TravelTools_Heal(", "uint8_t TravelTools_IsOwned(", "void TravelTools_Grant(")
            if marker in inventory)
        repair = block(sync, "static void RepairFlagOwnedCells(")
        owned = block(sync, '    if (sh.contains("ownedItems") && sh["ownedItems"].is_array())')
        apply = "void ApplySharedCells(const Shared& sh) { NeiSaveData* nei = Nei_Save();\n" + owned + "\n" + macro(sync, "FCO_APPLY") + "\nRepairFlagOwnedCells(nei);\n}\n"
        extract = "void ExtractSharedFlags(Shared& sh) { NeiSaveData* nei = Nei_Save(); RepairFlagOwnedCells(nei);\n" + macro(sync, "FCO_EXTRACT") + "\n}\n"
        expected = "FcEquip_OotToMm(FC_OOT_PAGE2_FIRST)" if host == "mm" else "FC_OOT_PAGE2_FIRST"
        code = PREFIX + defines + "\n#define EXPECTED_ORDINARY " + expected + "\n" + helpers + "\n" + repair + "\n" + apply + extract + CHECKS
        with tempfile.TemporaryDirectory(prefix="review-shared-sync-" + host + "-") as td:
            path = Path(td); cpp = path / "test.cpp"; binary = path / "test"
            cpp.write_text(code)
            includes = ["-I" + str(root / p) for p in (host, host + "/include", host + "/include/PR", "libultraship/include", "combo", "combo/menu", fleet)]
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *includes, str(cpp), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
            print("PASS host:", host, flush=True)


if __name__ == "__main__":
    main()
