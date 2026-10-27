#include "2s2h/Rando/Logic/Logic.h"
#include "2s2h/Rando/MiscBehavior/ClockShuffle.h"
#include "2s2h/FleetShipCombo/FleetShipCombo.h"
#include "2s2h/ShipUtils.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "tests/test_require.h"
#include "NeiGracePolicy.h"
#include "ComboGameMode.h"
#include <algorithm>
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
SaveContext gSaveContext{};
int gMMComboGoalHunt = 0, gMMComboGoalRequired = 0, gMMComboGoalPieces = -1;
int unified;
extern "C" int32_t CVarGetInteger(const char*, int32_t fallback) { return fallback; }
extern "C" int FleetCombo_UnifiedPoolActive(void) { return unified; }
extern "C" s32 Ship_Random(s32 min, s32 max) { REQUIRE(min < max); return min; }
extern "C" void Ship_Random_Seed(u64) {}
namespace Rando {
std::vector<RandoItemId> GetStartingItemsFromSave(RandoSaveInfo&) { return {}; }
std::vector<RandoCheckId> GetExcludedChecksFromConfig() { return {}; }
namespace StaticData {
std::map<RandoCheckId, RandoStaticCheck> Checks;
std::map<RandoItemId, RandoStaticItem> Items;
}
namespace Logic { std::map<RandoRegionId, RandoRegion> Regions; }
}
#include "pool.inc"
int main() {
    for (unified = 0; unified < 2; ++unified) {
        for (int nei : {0, 1}) {
            for (int start : {0, 1}) {
                for (int mode : {NEI_SEASONS_ROD, NEI_SEASONS_INDIVIDUAL, NEI_SEASONS_GATED}) {
                    RandoSaveInfo info{};
                    info.randoSaveOptions[RO_STARTING_HEALTH] = 3;
                    info.randoSaveOptions[RO_SHUFFLE_NEI_ITEMS] = nei;
                    info.randoSaveOptions[RO_ROD_OF_SEASONS] = mode;
                    info.randoSaveOptions[RO_STARTING_ROD_OF_SEASONS] = start;
                    auto starting = Rando::GetComputedStartingItems(info);
                    REQUIRE(std::count(starting.begin(), starting.end(), RI_OOT_NEI_ROD_OF_SEASONS) == start);
                    std::vector<RandoCheckId> checks;
                    std::vector<RandoItemId> pool;
                    Rando::Logic::GeneratePools(info, checks, pool);
                    REQUIRE(std::count(pool.begin(), pool.end(), RI_OOT_NEI_ROD_OF_SEASONS) ==
                            (nei && !start && mode != NEI_SEASONS_INDIVIDUAL));
                    for (auto season : {RI_OOT_NEI_SEASON_SPRING, RI_OOT_NEI_SEASON_SUMMER,
                                        RI_OOT_NEI_SEASON_AUTUMN, RI_OOT_NEI_SEASON_WINTER})
                        REQUIRE(std::count(pool.begin(), pool.end(), season) ==
                                (nei && mode == NEI_SEASONS_INDIVIDUAL));
                }
            }
        }
    }
    puts("PASS complete MM generator and computed start: three modes, start flag, NEI master, solo/combo");
}
