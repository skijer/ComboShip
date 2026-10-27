"""Execute Rod/season pickup arms and inventory state with real native headers."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags
from run_mm_weather_tests import production_function


def body(path, name):
    return production_function((ROOT / path).read_text().replace('extern "C" ', ''), name)


inventory = (ROOT / "mm/mods/extended_inventory.c").read_text()
parts = [body("mm/2s2h/Rando/NeiSeasons.cpp", "Seasons_RandoMode")]
for name in ("Seasons_HasRod", "Seasons_GrantRod", "Seasons_SeasonOwned", "Seasons_GrantSeason",
             "Seasons_SeasonCount", "Seasons_SeasonAt", "Seasons_GetSeason", "Seasons_SetSeason"):
    if re.search(r"\b" + name + r"\([^;]*\)\s*\{", inventory):
        parts.append(production_function(inventory, name))
gate_path = ROOT / "mm/2s2h/Rando/NeiSeasons.cpp"
parts.insert(0, body("mm/2s2h/Rando/NeiSeasons.cpp", "Seasons_UpdateGates")
             if gate_path.exists() else "void Seasons_UpdateGates() {}")
give = body("mm/2s2h/Rando/GiveItem.cpp", "Rando::GiveItem")
first = give.index("        case RI_OOT_NEI_ROD_OF_SEASONS:")
last = give.index("        // Elemental Wand:", first)
parts.append("void Pickup(RandoItemId randoItemId) { switch(randoItemId) {\n" +
             give[first:last] + "default: assert(false); } }")

prefix = r'''
#include "2s2h/Rando/Rando.h"
#include "mods/extended_inventory.h"
#include <cassert>
#include <cstdio>
SaveContext gSaveContext{};
NeiSaveData* Nei_Save() { return &gSaveContext.save.shipSaveInfo.nei; }
uint16_t Nei_GetOwnedItem(uint8_t slot) { return Nei_Save()->ownedItems[slot - 24]; }
void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { Nei_Save()->ownedItems[slot - 24] = item; }
void SetMode(int mode) {
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_RANDO;
    gSaveContext.save.shipSaveInfo.rando.randoSaveOptions[RO_ROD_OF_SEASONS] = mode;
}

'''
for game, path in (("MM", "mm/2s2h/FleetShipCombo/FleetSync.cpp"),
                   ("OOT", "soh/soh/FleetShipCombo/FleetSync.cpp")):
    fleet = (ROOT / path).read_text()
    first = fleet.index('    if (sh.contains("seasonsOwned")')
    last = fleet.index('    RepairFlagOwnedCells(nei);', first)
    parts.append("void Apply" + game + "(const nlohmann::json& sh) { auto* nei = Nei_Save();\n" +
                 fleet[first:last] + "\n}")
    first = fleet.index('    sh["seasonsOwned"]')
    last = fleet.index('    FleetRpg::Extract', first) if game == "OOT" else fleet.index('    sh["rpgStats"]', first)
    parts.append("nlohmann::json Export" + game + "() { nlohmann::json sh; auto* nei = Nei_Save();\n" +
                 fleet[first:last] + "\nreturn sh; }")

checks = r'''
int main() {
    SetMode(NEI_SEASONS_ROD);
    Pickup(RI_OOT_NEI_ROD_OF_SEASONS);
    assert(Nei_Save()->seasonsOwned == 15 && "one Rod pickup must unlock all four seasons in Rod mode");
    assert(Nei_GetOwnedItem(SLOT_ROD_OF_SEASONS) == EXT_ITEM_ROD_OF_SEASONS);
    assert(Seasons_SeasonCount() == 4);
    SetMode(NEI_SEASONS_INDIVIDUAL);
    *Nei_Save() = {};
    Pickup(RI_OOT_NEI_SEASON_WINTER);
    assert(Nei_Save()->seasonsOwned == 8 && Seasons_GetSeason() == SEASON_WINTER);
    assert(Seasons_HasRod() && Seasons_SeasonCount() == 1);
    Nei_Save()->seasonsGates = 15;
    assert(!Seasons_SeasonOwned(SEASON_SPRING)); // gates have no effect in Individual mode
    SetMode(NEI_SEASONS_GATED);
    *Nei_Save() = {};
    Pickup(RI_OOT_NEI_ROD_OF_SEASONS);
    gSaveContext.save.saveInfo.inventory.questItems = 15; // randomized remains never satisfy gates
    assert(Seasons_HasRod() && Seasons_SeasonCount() == 0);
    assert(Seasons_SeasonOwned(SEASON_OFF) && Seasons_GetSeason() == SEASON_OFF);
    WEEKEVENTREG(55) |= 0x80;
    assert(Seasons_SeasonOwned(SEASON_SPRING) && "Great Bay completion must unlock Spring in gated mode");
    assert(Seasons_SeasonCount() == 1);
    WEEKEVENTREG(55) &= ~0x80;
    assert(Seasons_SeasonOwned(SEASON_SPRING)); // persistent across Song of Time
    WEEKEVENTREG(20) |= 0x02;
    WEEKEVENTREG(33) |= 0x80;
    WEEKEVENTREG(52) |= 0x20;
    assert(Seasons_SeasonCount() == 4);
    *Nei_Save() = {};
    Seasons_UpdateGates();
    assert(!Seasons_HasRod() && Seasons_SeasonCount() == 0); // gates cannot grant the Rod
    for (auto apply : {ApplyMM, ApplyOOT}) {
        *Nei_Save() = {};
        apply(nlohmann::json{{"seasonsRodOwned", 1}, {"seasonsGates", 9}});
        assert(Seasons_HasRod() && "empty gated Rod ownership must import into both hosts");
        assert(Nei_Save()->seasonsGates == 9 && Nei_Save()->seasonsOwned == 0);
        apply(nlohmann::json{{"seasonsGates", 2}});
        assert(Nei_Save()->seasonsGates == 11); // OR, never MAX: independent game gates combine
        apply(nlohmann::json{{"seasonsRodOwned", 0}, {"seasonsGates", 0}});
        assert(Seasons_HasRod() && Nei_Save()->seasonsGates == 11);
        apply(nlohmann::json{{"seasonsGates", nullptr}});
        assert(Nei_Save()->seasonsGates == 11);
        *Nei_Save() = {};
        apply(nlohmann::json{{"seasonsGates", 1}});
        assert(!Seasons_HasRod()); // completion state alone cannot grant ownership
    }
    SetMode(NEI_SEASONS_INDIVIDUAL);
    *Nei_Save() = {};
    Pickup(RI_OOT_NEI_SEASON_WINTER);
    for (auto exportState : {ExportMM, ExportOOT}) {
        auto shared = exportState();
        assert(shared.value("seasonsRodOwned", 0) == 1);
        assert(shared.value("seasonsGates", -1) == 0);
        assert(shared.value("seasonsOwned", 0) == 8);
    }
    SetMode(99);
    assert(Seasons_RandoMode() == NEI_SEASONS_INDIVIDUAL);
    gSaveContext.save.shipSaveInfo.saveType = SAVETYPE_VANILLA;
    assert(Seasons_RandoMode() == NEI_SEASONS_INDIVIDUAL);
    puts("PASS saved policy, Rod/individual pickups, MM completion, persistent gates and both-host shared state");
}
'''
with tempfile.TemporaryDirectory(prefix="seasons-modes-") as td:
    path = Path(td) / "test.cpp"
    path.write_text(prefix + "\n" + "\n".join(parts) + "\n" + checks)
    binary = Path(td) / "test"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g", "-w", *flags(),
                    "-DCONTROLLERBUTTONS_T=uint32_t", "-include", "nlohmann/json.hpp",
                    str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
