#ifdef COMBO_BUILD
#include "ComboExport.h"
#include "ComboGameMode.h"
#include "ComboBlackMarketNative.h"
#include "soh/cvar_prefixes.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include <libultraship/bridge/consolevariablebridge.h>
#include <nlohmann/json.hpp>
extern "C" {
#include "z64.h"
extern SaveContext gSaveContext;
extern PlayState* gPlayState;
}

namespace {
void (*sReportClear)(int, int) = nullptr;
void RegisterMarketClears() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnBossDefeat>([](void* pointer) {
        if (gComboGameMode == COMBO_GAME_MM || !pointer || !gPlayState || gSaveContext.fileNum > 2 ||
            gSaveContext.gameMode != GAMEMODE_NORMAL)
            return;
        const auto* actor = static_cast<Actor*>(pointer);
        int boss = -1;
        switch (actor->id) {
            case ACTOR_BOSS_GOMA:
                boss = 0;
                break;
            case ACTOR_BOSS_DODONGO:
                boss = 1;
                break;
            case ACTOR_BOSS_VA:
                boss = 2;
                break;
            case ACTOR_BOSS_GANONDROF:
                boss = 3;
                break;
            case ACTOR_BOSS_FD2:
                boss = 4;
                break;
            case ACTOR_BOSS_MO:
                boss = 5;
                break;
            case ACTOR_BOSS_TW:
                boss = 6;
                break;
            case ACTOR_BOSS_SST:
                boss = 7;
                break;
        }
        static constexpr int scenes[] = { SCENE_DEKU_TREE_BOSS,     SCENE_DODONGOS_CAVERN_BOSS, SCENE_JABU_JABU_BOSS,
                                          SCENE_FOREST_TEMPLE_BOSS, SCENE_FIRE_TEMPLE_BOSS,     SCENE_WATER_TEMPLE_BOSS,
                                          SCENE_SPIRIT_TEMPLE_BOSS, SCENE_SHADOW_TEMPLE_BOSS };
        if (boss >= 0 && gPlayState->sceneNum != scenes[boss])
            return; // captured/spawned bosses outside their dungeon arena do not count
        if (boss >= 0) {
            if (gComboGameMode == COMBO_GAME_OOT)
                ComboBlackMarketOot_ReportClear(boss, gSaveContext.fileNum);
            else if (Combo_IsCombinedGame() && sReportClear)
                sReportClear(boss, gSaveContext.fileNum);
        }
    });
}
RegisterShipInitFunc init(RegisterMarketClears);
} // namespace

extern "C" COMBO_EXPORT void SOH_SetBlackMarketClearCallback(void (*callback)(int, int)) {
    sReportClear = callback;
}

// Freeze these options once per generation. The launcher has no CVar access.
extern "C" COMBO_EXPORT const char* SOH_ReadBlackMarketOptions() {
    auto value = [](const char* key, int fallback) { return CVarGetInteger(key, fallback); };
    static std::string result;
    result = nlohmann::json{
        { "pricing", value("gCombo.Rando.BlackMarketPricing", 0) },
        { "dungeonUnlocks", value("gCombo.Rando.BlackMarketDungeonUnlocks", 1) != 0 },
        { "sohMode", value(CVAR_RANDOMIZER_SETTING("ShopsanityPrices"), 0) },
        { "fixedPrice", value(CVAR_RANDOMIZER_SETTING("ShopsanityFixedPrice"), 10) * 5 },
        { "rangeMin", value(CVAR_RANDOMIZER_SETTING("ShopsanityPriceRange1"), 10) * 5 },
        { "rangeMax", value(CVAR_RANDOMIZER_SETTING("ShopsanityPriceRange2"), 100) * 5 },
        { "walletWeights",
          { value(CVAR_RANDOMIZER_SETTING("ShopsanityNoWalletWeight"), 10),
            value(CVAR_RANDOMIZER_SETTING("ShopsanityChildWalletWeight"), 10),
            value(CVAR_RANDOMIZER_SETTING("ShopsanityAdultWalletWeight"), 10),
            value(CVAR_RANDOMIZER_SETTING("ShopsanityGiantWalletWeight"), 10),
            value(CVAR_RANDOMIZER_SETTING("ShopsanityTycoonWalletWeight"), 10) } },
        { "tycoon", value(CVAR_RANDOMIZER_SETTING("IncludeTycoonWallet"), 0) != 0 },
        { "affordable", value(CVAR_RANDOMIZER_SETTING("ShopsanityPricesAffordable"), 0) != 0 }
    }.dump();
    return result.c_str();
}
#endif
