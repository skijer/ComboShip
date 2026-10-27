#include "2s2h/Rando/Logic/Logic.h"
#include "2s2h/Rando/MiscBehavior/ClockShuffle.h"
#include "2s2h/FleetShipCombo/FleetShipCombo.h"
#include "2s2h/Rando/NeiGiPresentation.h"
#include "ComboResolve.h"
#include "ComboGameMode.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "tests/test_require.h"
#include "NeiGracePolicy.h"
#include <algorithm>
#include <cstring>

using NeiGi::Kind;
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
extern "C" int32_t CVarGetInteger(const char *, int32_t defaultValue) { return defaultValue; }

SaveContext gSaveContext{};
int gMMComboGoalHunt = 0;
int gMMComboGoalRequired = 0;
int gMMComboGoalPieces = -1;
int unified = 0;
namespace Rando {
std::vector<RandoItemId> GetStartingItemsFromSave(RandoSaveInfo &) { return {}; }
std::vector<RandoItemId> GetComputedStartingItems(RandoSaveInfo &) { return {}; }
std::vector<RandoCheckId> GetExcludedChecksFromConfig() { return {}; }
namespace StaticData {
std::map<RandoCheckId, RandoStaticCheck> Checks;
}
namespace Logic {
std::map<RandoRegionId, RandoRegion> Regions;
}
} // namespace Rando
extern "C" int FleetCombo_UnifiedPoolActive(void) { return unified; }
extern "C" s32 Ship_Random(s32 min, s32 max) {
    REQUIRE(min < max);
    return min;
}

static std::string described;
static std::string modPath;
extern "C" int32_t OOT_GetNeiGiDrawInfo(const char *slug, CwItemDrawInfo *out) {
    described = slug;
    *out = CwItemDrawInfo{};
    return 1;
}
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return false; }
extern "C" int ResourceMgr_IsModAssetForGame(const char *game, const char *path) {
    REQUIRE(strcmp(game, "mm") == 0);
    return modPath == path;
}
const char *NeiResource_Route(const char *path) { return path; }
extern "C" int ResourceMgr_IsIkanaShieldGiAvailable() {
    REQUIRE(false && "pool/editor bindings must not select the native shield pair");
    return 0;
}

#include "pool_binding_production.inc"

int main() {
    // Run the complete production pool generator. Region/check input is empty so
    // this isolates its option-driven item additions, before solver/placement.
    const auto existingSave = gSaveContext.save;
    for (int combo = 0; combo < 2; ++combo) {
        unified = combo;
        for (int enabled = 0; enabled < 2; ++enabled) {
            for (int wand = RO_WAND_MEDALLIONS; wand <= RO_WAND_ELEMENTAL_SHUFFLE; ++wand) {
                RandoSaveInfo info{};
                info.randoSaveOptions[RO_STARTING_HEALTH] = 3;
                info.randoSaveOptions[RO_SHUFFLE_NEI_ITEMS] = enabled ? RO_GENERIC_YES : RO_GENERIC_NO;
                info.randoSaveOptions[RO_ELEMENTAL_WAND_SHUFFLE] = wand;
                std::vector<RandoCheckId> checks;
                std::vector<RandoItemId> pool;
                Rando::Logic::GeneratePools(info, checks, pool);
                REQUIRE(std::count(pool.begin(), pool.end(), RI_OOT_NEI_PHANTOM_HOURGLASS) == enabled);
                REQUIRE(std::count(pool.begin(), pool.end(), RI_OOT_NEI_HYLIAS_GRACE) == enabled);
                if (enabled) {
                    const RandoItemId powers[] = {
                        RI_OOT_NEI_DESIRE_SENSOR,     RI_OOT_NEI_SLATE_RUNE_BOMB,    RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE,
                        RI_OOT_NEI_SLATE_RUNE_STASIS, RI_OOT_NEI_SLATE_RUNE_CRYONIS, RI_OOT_NEI_SEASON_SPRING,
                        RI_OOT_NEI_SEASON_SUMMER,     RI_OOT_NEI_SEASON_AUTUMN,      RI_OOT_NEI_SEASON_WINTER};
                    for (const auto id : powers)
                        REQUIRE(std::count(pool.begin(), pool.end(), id) == 1);
                    REQUIRE(std::count(pool.begin(), pool.end(), RI_OOT_NEI_SHEIKAH_SLATE) == 0);
                    REQUIRE(std::count(pool.begin(), pool.end(), RI_OOT_NEI_ROD_OF_SEASONS) == 0);
                }
                REQUIRE(memcmp(&existingSave, &gSaveContext.save, sizeof(existingSave)) == 0);
            }
        }
    }
    puts("PASS production new pools: Hourglass once iff NEI enabled, solo/combo and all wand rules; five runes/four "
         "seasons; existing save untouched");

    for (const auto mode : {RO_GRACE_ON, RO_GRACE_OFF, RO_GRACE_GATED}) {
        RandoSaveInfo info{};
        info.randoSaveOptions[RO_STARTING_HEALTH] = 3;
        info.randoSaveOptions[RO_SHUFFLE_NEI_ITEMS] = RO_GENERIC_YES;
        info.randoSaveOptions[RO_HYLIAS_GRACE] = mode;
        std::vector<RandoCheckId> checks;
        std::vector<RandoItemId> pool;
        Rando::Logic::GeneratePools(info, checks, pool);
        REQUIRE(std::count(pool.begin(), pool.end(), RI_OOT_NEI_HYLIAS_GRACE) == (mode != RO_GRACE_OFF));
        REQUIRE(std::count(pool.begin(), pool.end(), RI_OOT_NEI_PHANTOM_HOURGLASS) == 1);
    }
    puts("PASS Grace seed pool: On/Gated include it, Off excludes it and keeps Hourglass");

    CwItemDrawInfo draw{};
    REQUIRE(MM_DescribeNeiGi((RandoItemId)194, &draw));
    REQUIRE(described == "hylia_grace");
    REQUIRE(MM_DescribeNeiGi((RandoItemId)192, &draw));
    REQUIRE(described == "fire_rod");
    modPath = "objects/object_nei_magic_spell/gHyliaGraceGiveDL";
    REQUIRE(!MM_DescribeNeiGi((RandoItemId)194, &draw));
    REQUIRE(MM_DescribeNeiGi((RandoItemId)192, &draw));
    REQUIRE(described == "fire_rod");
    puts("PASS production GI bindings: RI194 Grace and RI192 Fire Rod stay distinct, including Grace legacy override");
    return 0;
}
