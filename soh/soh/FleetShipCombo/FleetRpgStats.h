#pragma once

#include "../../../combo/rando/RpgStatsJson.h"
#include "soh/Enhancements/randomizer/randostatupgrade.h"

// Included after the native save declarations. RPG pickups remain OoT items in
// the combined pool, including foreign MM placements; the existing cross-item
// delivery grants them once into this save. MM consumes the resulting snapshot.
namespace FleetRpg {
#define FLEET_RPG_FIELDS(X)                                        \
    X(COMBO_RPG_DEFENSE, defenseUpgrades, DEFENSE_UPGRADE, 5)      \
    X(COMBO_RPG_SPEED, speedUpgrades, SPEED_UPGRADE, 5)            \
    X(COMBO_RPG_POWER, powerUpgrades, POWER_UPGRADE, 5)            \
    X(COMBO_RPG_MAGIC, magicStatUpgrades, MAGIC_STAT_UPGRADE, 8)   \
    X(COMBO_RPG_CRAWL, crawlSpeedUpgrades, CRAWL_SPEED_UPGRADE, 5) \
    X(COMBO_RPG_CLIMB, climbSpeedUpgrades, CLIMB_SPEED_UPGRADE, 5) \
    X(COMBO_RPG_PUSH, pushSpeedUpgrades, PUSH_SPEED_UPGRADE, 5)

inline ComboRpgState Read() {
    ComboRpgState state{};
    state.knownMask = (1 << COMBO_RPG_COUNT) - 1;
    if (!IS_RANDO)
        return state;
    auto ctx = Rando::Context::GetInstance();
#define READ(index, field, key, fallback)                                                               \
    state.level[index] = gSaveContext.ship.quest.data.randomizer.field;                                 \
    state.required[index] =                                                                             \
        StatUpgradeRequired(fallback, RSK_##key##_ADJUSTABLE, RSK_##key##_TOTAL, RSK_##key##_REQUIRED); \
    if (ctx->GetOption(RSK_##key))                                                                      \
        state.enabledMask |= 1 << index;
    FLEET_RPG_FIELDS(READ)
#undef READ
    state.magicTotal = ctx->GetOption(RSK_MAGIC_STAT_UPGRADE_ADJUSTABLE) ? 100 : 96;
    state.quarterHeartsEnabled = static_cast<bool>(ctx->GetOption(RSK_QUARTER_HEART));
    state.nativeMagicLevel = gSaveContext.ship.quest.data.randomizer.comboNativeMagicLevel;
    return state;
}

inline void Extract(nlohmann::json& shared) {
    const auto state = Read();
    shared["rpgStats"] = ComboRpg::ToJson(state);
    // Compatibility with saves created by the earlier speed-only POC.
    shared["speedUpgrades"] = state.level[COMBO_RPG_SPEED];
    shared["speedUpgradeRequired"] = ComboRpgState_Required(&state, COMBO_RPG_SPEED);
}

inline void Apply(const nlohmann::json& shared) {
    if (!IS_RANDO)
        return;
    auto state = Read();
    const int16_t previousMagicCapacity = ComboRpgState_MagicCapacity(&state, 0);
    if (shared.contains("rpgStats"))
        ComboRpg::MergeJson(state, shared["rpgStats"], false);
    else if (shared.contains("speedUpgrades"))
        ComboRpg::MergeJson(state, { { "speed", { { "level", shared["speedUpgrades"] } } } }, false);
#define WRITE(index, field, key, fallback)    \
    if (ComboRpgState_Enabled(&state, index)) \
        gSaveContext.ship.quest.data.randomizer.field = state.level[index];
    FLEET_RPG_FIELDS(WRITE)
#undef WRITE
    gSaveContext.ship.quest.data.randomizer.comboNativeMagicLevel = state.nativeMagicLevel;
    const int16_t magicCapacity = ComboRpgState_MagicCapacity(&state, 0);
    if (magicCapacity > 0) {
        gSaveContext.isMagicAcquired = true;
        if (magicCapacity > 48)
            gSaveContext.isDoubleMagicAcquired = true;
        if (magicCapacity != previousMagicCapacity) {
            gSaveContext.magic = gSaveContext.magicFillTarget = magicCapacity;
            gSaveContext.magicLevel = 0;
        } else if (gSaveContext.magic > magicCapacity) {
            gSaveContext.magic = magicCapacity;
        }
    }
}
#undef FLEET_RPG_FIELDS
} // namespace FleetRpg
