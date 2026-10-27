"""Execute OoT's native event-flag accessor and Rod inventory/gate functions."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_weather_tests import production_function


def body(path, name):
    return production_function((ROOT / path).read_text().replace('extern "C" ', ''), name)


parts = [body("soh/src/code/z_actor.c", "Flags_GetEventChkInf"),
         body("soh/soh/Enhancements/randomizer/NeiSeasons.cpp", "Seasons_UpdateGates")]
parts += [body("soh/mods/extended_inventory.c", name) for name in
          ("Seasons_HasRod", "Seasons_GrantRod", "Seasons_SeasonOwned", "Seasons_GrantSeason",
           "Seasons_SeasonCount", "Seasons_SeasonAt", "Seasons_GetSeason", "Seasons_SetSeason")]
give = (ROOT / "soh/soh/Enhancements/randomizer/randomizer.cpp").read_text()
rod = give[give.index("        case RG_ROD_OF_SEASONS:\n"):
           give.index("        // Crossover Items.", give.index("        case RG_ROD_OF_SEASONS:\n"))]
seasons = give[give.index("        case RG_SEASON_SPRING:\n"):
               give.index("        case RG_EXT_PENDANT_OF_MEMORIES:\n", give.index("        case RG_SEASON_SPRING:\n"))]
parts.append("void Pickup(RandomizerGet item) { switch (item) {\n" + rod + seasons +
             "default: assert(false); } }\n")
start = (ROOT / "soh/soh/Enhancements/randomizer/savefile.cpp").read_text()
start = start[start.index("    if (Randomizer_GetSettingValue(RSK_STARTING_ROD_OF_SEASONS))"):
              start.index("    int startingAge =", start.index("void SetStartingItems()"))]
parts.append("void StartingRod() {\n" + start + "}\n")
prefix = r'''
#include "z64.h"
#include "mods/extended_inventory.h"
#include "rando/CrossEntranceBoss.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
SaveContext gSaveContext{};
ComboRando::CrossDungeon::BossCompletionState bossCompletion;
bool crossBossEnabled = false;
extern "C" int Combo_SOH_CrossBossLogicalClear(int id, int nativeValue) {
    return bossCompletion.LogicalClear(id, nativeValue, crossBossEnabled);
}
NeiSaveData state{};
NeiSaveData* Nei_Save() { return &state; }
uint16_t Nei_GetOwnedItem(uint8_t slot) { return state.ownedItems[slot - 24]; }
void Nei_SetOwnedItem(uint8_t slot, uint16_t item) { state.ownedItems[slot - 24] = item; }
uint8_t mode = NEI_SEASONS_GATED;
uint8_t Seasons_RandoMode() { return mode; }
uint8_t startingRod = 0;
uint8_t Randomizer_GetSettingValue(RandomizerSettingKey key) {
    assert(key == RSK_STARTING_ROD_OF_SEASONS);
    return startingRod;
}
'''
checks = r'''
int main() {
    struct Gate { int flag; uint8_t season; } gates[] = {
        {EVENTCHKINF_LEARNED_SONG_OF_STORMS, SEASON_SPRING},
        {EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP, SEASON_SPRING},
        {EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP, SEASON_SPRING},
        {EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP, SEASON_SUMMER},
        {EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP, SEASON_AUTUMN},
        {EVENTCHKINF_LEARNED_SERENADE_OF_WATER, SEASON_WINTER},
    };
    for (auto gate : gates) {
        state = {}; memset(gSaveContext.eventChkInf, 0, sizeof(gSaveContext.eventChkInf));
        gSaveContext.inventory.questItems = 0xFFFFFFFFu; // every shuffled reward and song, no completed checks
        Pickup(RG_ROD_OF_SEASONS);
        assert(Seasons_SeasonCount() == 0 && Seasons_GetSeason() == SEASON_OFF);
        gSaveContext.eventChkInf[gate.flag >> 4] |= 1u << (gate.flag & 15);
        assert(Seasons_SeasonOwned(gate.season));
        assert(Seasons_SeasonCount() == 1 && state.seasonsGates == (1u << gate.season));
        memset(gSaveContext.eventChkInf, 0, sizeof(gSaveContext.eventChkInf));
        assert(Seasons_SeasonOwned(gate.season));
    }
    state = {};
    gSaveContext.eventChkInf[EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP >> 4] |=
        1u << (EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP & 15);
    Seasons_UpdateGates();
    assert(state.seasonsGates == 2 && !Seasons_HasRod());
    // Native arena flags must not unlock the wrong source dungeon's season.
    crossBossEnabled = true;
    for (int flag : {EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP,
                     EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP,
                     EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP,
                     EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP}) {
        gSaveContext.eventChkInf[flag >> 4] |= 1u << (flag & 15);
    }
    state = {};
    Seasons_UpdateGates();
    assert(state.seasonsGates == 0);
    memset(gSaveContext.eventChkInf, 0, sizeof(gSaveContext.eventChkInf));
    struct BossGate { int boss; uint8_t season; } bossGates[] = {
        {COMBO_BOSS_BARINADE, SEASON_SPRING}, {COMBO_BOSS_MORPHA, SEASON_SPRING},
        {COMBO_BOSS_VOLVAGIA, SEASON_SUMMER}, {COMBO_BOSS_PHANTOM_GANON, SEASON_AUTUMN},
    };
    for (auto gate : bossGates) {
        state = {};
        bossCompletion.SetCompletedMask(1u << gate.boss);
        Seasons_UpdateGates();
        assert(state.seasonsGates == (1u << gate.season));
        bossCompletion.SetCompletedMask(0);
        Seasons_UpdateGates();
        assert(state.seasonsGates == (1u << gate.season)); // completion remains latched
    }
    crossBossEnabled = false;
    mode = NEI_SEASONS_INDIVIDUAL;
    Pickup(RG_SEASON_WINTER);
    assert(!Seasons_SeasonOwned(SEASON_SUMMER)); // gates are inactive
    mode = NEI_SEASONS_ROD;
    state = {};
    Pickup(RG_ROD_OF_SEASONS);
    assert(Seasons_SeasonCount() == 4 && state.seasonsOwned == 15);
    for (uint8_t rule : {NEI_SEASONS_ROD, NEI_SEASONS_INDIVIDUAL, NEI_SEASONS_GATED}) {
        mode = rule;
        state = {};
        startingRod = 0;
        StartingRod();
        assert(!Seasons_HasRod());
        startingRod = 1;
        StartingRod();
        assert(Seasons_HasRod());
        assert(state.seasonsOwned == (rule == NEI_SEASONS_ROD ? 15 : 0));
        assert(rule == NEI_SEASONS_ROD || state.season == SEASON_OFF);
    }
    puts("PASS OoT season gates: native and shuffled source clears, persistent gates and inactive ungated rules");
}
'''
with tempfile.TemporaryDirectory(prefix="seasons-oot-modes-") as td:
    path = Path(td) / "test.cpp"
    path.write_text(prefix + "\n" + "\n".join(parts) + "\n" + checks)
    binary = Path(td) / "test"
    includes = ["-I" + str(ROOT / p) for p in ("soh", "soh/include", "soh/include/PR", "soh/assets",
                                                  "libultraship/include", "combo", "combo/menu")]
    result = subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-w", "-DF3DEX_GBI_2",
                             "-DCOMBO_BUILD", *includes, str(path), "-o", str(binary)], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(binary)], check=True)
