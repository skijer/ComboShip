#include "2s2h/Rando/Logic/Logic.h"
#include "2s2h/Rando/MiscBehavior/ClockShuffle.h"
#include "2s2h/FleetShipCombo/FleetShipCombo.h"
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
namespace Rando {
std::vector<RandoItemId> GetStartingItemsFromSave(RandoSaveInfo&) { return {}; }
std::vector<RandoItemId> GetComputedStartingItems(RandoSaveInfo&) { return {}; }
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
        for (int nei : {RO_GENERIC_NO, RO_GENERIC_YES}) {
            for (int mode : {RO_GRACE_ON, RO_GRACE_OFF, RO_GRACE_GATED}) {
                RandoSaveInfo info{};
                info.randoSaveOptions[RO_STARTING_HEALTH] = 3;
                info.randoSaveOptions[RO_SHUFFLE_NEI_ITEMS] = nei;
                info.randoSaveOptions[RO_HYLIAS_GRACE] = mode;
                std::vector<RandoCheckId> checks;
                std::vector<RandoItemId> pool;
                Rando::Logic::GeneratePools(info, checks, pool);
                const int want = nei == RO_GENERIC_YES && mode != RO_GRACE_OFF;
                REQUIRE(std::count(pool.begin(), pool.end(), RI_OOT_NEI_HYLIAS_GRACE) == want);
                REQUIRE(std::count(pool.begin(), pool.end(), RI_OOT_NEI_PHANTOM_HOURGLASS) == nei);
            }
        }
    }
    puts("PASS real MM pool: Grace On/Off/Gated, master NEI switch, solo/combo and independent Hourglass");
}
