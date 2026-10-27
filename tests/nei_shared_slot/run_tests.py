#!/usr/bin/env python3
"""Exercise both hosts' shared slot, legacy healing and Slate name selection."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function

for host in ("mm", "soh"):
    inventory = (ROOT / host / "mods/extended_inventory.c").read_text()
    header = (ROOT / host / "mods/extended_inventory.h").read_text()
    kaleido = ROOT / host / ("src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_item.c" if host == "mm"
                             else "src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c")
    # Project the unchanged item-name dispatch before unrelated item families.
    # This executes the real Slate branch; the later wand/trade routing is covered elsewhere.
    name = function(inventory, "ExtInv_GetCustomItemNameTex")
    name = name[:name.index("    // ", name.index("    switch (itemId)"))] + "    return nullptr;\n}"
    parts = []
    for symbol in ("TravelTools_Heal", "TravelTools_IsOwned", "TravelTools_Grant",
                   "GraceHourglass_Heal", "GraceHourglass_IsOwned", "GraceHourglass_Grant",
                   "Slate_RuneOwned", "Slate_RuneCount", "Slate_RuneAt", "Slate_GetRune", "Slate_SetRune",
                   "Slate_RuneNeighbor", "Slate_RuneNameTex"):
        if symbol + "(" in inventory:
            parts.append(function(inventory, symbol))
    parts += [function(header, "ExtInv_GiveItem"), name,
              function(kaleido.read_text(), "Page2Relayout_Heal")]
    prefix = r'''
#include "mods/nei_save.h"
#include "gameplay/MagicMirrorItem.h"
#include "z64item.h"
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <iterator>
using s32 = int32_t;
using u16 = uint16_t;
#define SLOT_PHANTOM_HOURGLASS 41
#define SLOT_HYLIAS_GRACE 41
#define SLOT_SHOVEL 46
#define SLOT_ROD_OF_SEASONS 47
#define SLOT_SHADOW_CRYSTAL 44
#ifndef EXT_ITEM_SHEIKAH_SLATE
#define EXT_ITEM_SHEIKAH_SLATE 0x0220
#define EXT_ITEM_PHANTOM_HOURGLASS 0x0221
#define EXT_ITEM_SHADOW_CRYSTAL 0x0222
#define EXT_ITEM_ROD_OF_SEASONS 0x0223
#endif
NeiSaveData state{};
extern "C" NeiSaveData* Nei_Save() { return &state; }
uint16_t ExtInv_GetSlotItem(int slot) { return state.ownedItems[slot - 24]; }
void ExtInv_SetSlotItem(int slot, uint16_t item) { state.ownedItems[slot - 24] = item; }
extern "C" uint16_t Nei_GetOwnedItem(uint8_t slot) { return ExtInv_GetSlotItem(slot); }
extern "C" void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { ExtInv_SetSlotItem(slot, item); }
static void reset() {
    state = {};
    for (auto& item : state.ownedItems) item = 0xFF;
}
'''
    checks = r'''
int main() {
    reset();
    state.slateRunesOwned = 0x1F;
    const char* names[] = {
        "gSlateRuneBombNameTex", "gSlateRuneStasisNameTex", "gSlateRuneCryonisNameTex",
        "gSlateRuneMasterCycleNameTex", "gSlateRuneSensorNameTex"
    };
    for (uint8_t rune = 0; rune < 5; ++rune) {
        Slate_SetRune(rune);
        const char* path = static_cast<const char*>(ExtInv_GetCustomItemNameTex(0x0220, 0));
        assert(path && std::strstr(path, names[rune]));
    }
    state.slateRunesOwned = 1u << 2;
    state.slateMode = 255;
    assert(std::strstr(static_cast<const char*>(ExtInv_GetCustomItemNameTex(0x0220, 0)), names[2]));
    state.slateRunesOwned = 0;
    assert(std::strstr(static_cast<const char*>(ExtInv_GetCustomItemNameTex(0x0220, 0)), "gSheikahSlateNameTex"));
    reset();
    ExtInv_SetSlotItem(41, ITEM_HYLIAS_GRACE);
    Page2Relayout_Heal();
    assert(ExtInv_GetSlotItem(41) == ITEM_HYLIAS_GRACE);
    for (uint16_t first : {uint16_t(ITEM_HYLIAS_GRACE), uint16_t(0x0221)}) {
        reset();
        ExtInv_GiveItem(41, first);
        const uint16_t second = first == ITEM_HYLIAS_GRACE ? 0x0221 : ITEM_HYLIAS_GRACE;
        ExtInv_GiveItem(41, second);
        assert(ExtInv_GetSlotItem(41) == first);
        /* OWNERSHIP_CHECKS */
    }
    puts("PASS shared slot: both orders, legacy healing, retained ownership, all Slate titles and invalid selection");
}
'''
    if "GraceHourglass_IsOwned(" in inventory:
        checks = checks.replace("/* OWNERSHIP_CHECKS */", r'''
        assert(GraceHourglass_IsOwned(first) && GraceHourglass_IsOwned(second));
        ExtInv_SetSlotItem(41, second);
        Page2Relayout_Heal();
        assert(ExtInv_GetSlotItem(41) == second);
        assert(GraceHourglass_IsOwned(first) && GraceHourglass_IsOwned(second));
        ExtInv_SetSlotItem(41, 0xFF);
        GraceHourglass_Heal();
        assert(ExtInv_GetSlotItem(41) == ITEM_HYLIAS_GRACE);
        assert(GraceHourglass_IsOwned(first) && GraceHourglass_IsOwned(second));
        assert(!GraceHourglass_IsOwned(0xFFFF));
        GraceHourglass_Grant(0xFFFF);
        assert(ExtInv_GetSlotItem(41) == ITEM_HYLIAS_GRACE);
''')
    with tempfile.TemporaryDirectory(prefix="nei-shared-slot-") as td:
        source = Path(td) / "test.cpp"
        source.write_text(prefix + "\n".join(parts) + checks)
        binary = Path(td) / "test"
        includes = ["-I" + str(ROOT / p) for p in (host, host + "/include", host + "/include/PR",
                                                    "libultraship/include", "combo", "combo/menu")]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", *includes,
                        str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print("PASS host:", host, flush=True)
