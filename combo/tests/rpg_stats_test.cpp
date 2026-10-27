#include "rando/RpgStatsJson.h"
#include "mods/nei_save.h"
#include "mods/combo_rpg.h"
#include <cmath>
#include <cassert>
#include <cstring>
#include <iostream>

using nlohmann::json;
#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#include "rpg_nei_json_production.inc"
struct TestMagicData {
    int8_t magic;
    uint8_t magicLevel, isMagicAcquired, isDoubleMagicAcquired;
};
struct TestSave {
    struct {
        TestMagicData playerData;
    } saveInfo;
    struct {
        NeiSaveData nei;
    } shipSaveInfo;
};
static struct {
    TestSave save;
    int16_t magicFillTarget;
} gSaveContext;
static auto& liveSave = gSaveContext.save.shipSaveInfo.nei;
extern "C" NeiSaveData* Nei_Save(void) {
    return &liveSave;
}
#define MM_PD gSaveContext.save.saveInfo.playerData
#define MAGIC_NORMAL_METER 48
#define MAGIC_DOUBLE_METER 96
#define SET_WEEKEVENTREG(x) ((void)0)
enum { RI_PROGRESSIVE_MAGIC, RI_SINGLE_MAGIC, RI_DOUBLE_MAGIC, RI_JUNK };
#include "rpg_mm_sync_production.inc"
#include "rpg_mm_native_magic_production.inc"

static int failures;
static void check(bool ok, const char* name) {
    if (!ok) {
        std::cerr << "FAIL: " << name << '\n';
        ++failures;
    }
}
static bool near(float a, float b) {
    return std::fabs(a - b) < 0.0001f;
}

int main() {
    ComboRpgState state{};
    check(near(ComboRpgState_SpeedMultiplier(&state), 1.0f), "old non-RPG saves keep native speed");
    for (int i = 0; i < COMBO_RPG_COUNT; ++i) {
        state.knownMask |= 1 << i;
        state.enabledMask |= 1 << i;
        state.required[i] = 10;
        state.level[i] = 5;
    }
    state.magicTotal = 100;
    state.quarterHeartsEnabled = 1;
    check(near(ComboRpgState_SpeedMultiplier(&state) * 1.5f, 1.8f), "speed stat stacks with Kokiri Emerald");
    check(ComboRpgState_Defense(&state, -16) == -12, "partial defense reduces incoming damage");
    check(ComboRpgState_Defense(&state, 16) == 16, "defense never reduces healing");
    check(ComboRpgState_Power(&state, 140, 0.25f) == 255, "power critical saturates byte damage");
    check(ComboRpgState_Power(&state, 7, 0.75f) == 7, "failed critical preserves damage");
    check(ComboRpgState_MagicCapacity(&state, 96) == 50, "adjustable magic honors saved cap");
    ComboRpgState nativeFloor = state;
    nativeFloor.level[COMBO_RPG_MAGIC] = 1;
    ComboRpg::MergeJson(nativeFloor, { { "nativeMagicLevel", 1 } });
    check(ComboRpgState_MagicCapacity(&nativeFloor, 96) == 48,
          "first native MM magic tier is a floor below RPG's maximum");
    ComboRpg::MergeJson(nativeFloor, { { "nativeMagicLevel", 2 } });
    check(ComboRpgState_MagicCapacity(&nativeFloor, 96) == 96,
          "RPG fractional magic cannot invalidate MM's double-magic solver state");
    ComboRpg::MergeJson(nativeFloor, { { "nativeMagicLevel", 1 }, { "magic", { { "level", 2 } } } });
    check(ComboRpgState_MagicCapacity(&nativeFloor, 96) == 96,
          "older snapshots or new fractional upgrades cannot demote native magic");
    check(near(ComboRpgState_CrawlMultiplier(&state), 3.0f), "crawl upgrade interpolates to donor maximum");
    check(near(ComboRpgState_MovementBonus(&state, COMBO_RPG_CLIMB) + 2, 4.5f), "climb stat stacks with Goron Ruby");
    check(near(ComboRpgState_MovementBonus(&state, COMBO_RPG_PUSH), 2.5f), "push upgrade interpolates");
    check(ComboRpgState_HeartCount(&state, 0x34) == 4, "quarter-heart capacity displays its partial heart");

    auto wire = ComboRpg::ToJson(state);
    ComboRpgState mm{};
    ComboRpg::MergeJson(mm, wire);
    check(ComboRpg::ToJson(mm) == wire, "all seven counts and saved rules arrive in MM");
    ComboRpg::MergeJson(mm, wire);
    check(ComboRpg::ToJson(mm) == wire, "duplicate handoff never grants twice");
    ApplyMmRpg({ { "rpgStats", wire } });
    check(near(ComboRpg_SpeedMultiplier(), 1.2f) && MM_PD.magic == 50 && MM_PD.isDoubleMagicAcquired,
          "real MM runtime receives speed and adjustable magic capacity");
    MM_PD.magic = 7;
    MM_PD.magicLevel = 2;
    ApplyMmRpg({ { "rpgStats", wire } });
    check(MM_PD.magic == 7 && MM_PD.magicLevel == 2, "duplicate RPG snapshot never refills spent magic");
    ApplyMmRpg({ { "rpgStats", { { "magic", { { "level", 6 } } } } } });
    check(MM_PD.magic == 60 && MM_PD.magicLevel == 0, "next magic stat grants one fill at the new cap");
    ApplyMmRpg({ { "rpgStats", { { "speed", { { "level", 6 } } } } } });
    check(MM_PD.magic == 60, "unrelated partial stat delta never resets magic");
    check(ComboRpg_ApplyDefense(-16) == -12 && ComboRpg_ApplyPower(4, 0.25f) == 8 &&
              near(ComboRpg_CrawlMultiplier(), 3.0f) && near(ComboRpg_ClimbBonus(), 2.5f) &&
              near(ComboRpg_PushBonus(), 2.5f) && ComboRpg_MagicCapacity(96) == 60 && ComboRpg_HeartCount(0x34) == 4,
          "C-facing MM adapters use the saved state for every stat family");
    ComboRpg::MergeJson(mm, { { "speed", { { "level", 1 } } } });
    check(mm.level[COMBO_RPG_SPEED] == 5, "older peer state never loses an upgrade");
    ComboRpg::MergeJson(mm, { { "speed", { { "level", 200 } } } });
    check(mm.level[COMBO_RPG_SPEED] == 10, "incoming level cannot exceed saved seed cap");
    check(mm.level[COMBO_RPG_MAGIC] == 5, "partial delta does not reset other stat families");
    ComboRpg::MergeJson(mm, { { "speed", { { "enabled", false } } } });
    check(near(ComboRpgState_SpeedMultiplier(&mm), 1.0f), "disabled saved setting wins over a retained count");
    ComboRpg::MergeJson(mm, { { "speed", { { "enabled", true }, { "required", 0 } } },
                              { "magic", { { "level", "broken" }, { "required", nullptr } } } });
    check(mm.required[COMBO_RPG_SPEED] == 1 && mm.level[COMBO_RPG_SPEED] == 1,
          "malformed zero cap cannot divide by zero");
    check(mm.level[COMBO_RPG_MAGIC] == 5 && mm.required[COMBO_RPG_MAGIC] == 10,
          "malformed stat values leave valid state intact");

    ComboRpgState partial{};
    ComboRpg::MergeJson(partial, { { "speed", { { "level", 20 } } } });
    ComboRpg::MergeJson(partial, { { "speed", { { "enabled", true }, { "required", 40 } } } });
    check(partial.level[COMBO_RPG_SPEED] == 20 && near(ComboRpgState_SpeedMultiplier(&partial), 1.2f),
          "count-before-settings deltas do not truncate adjustable progress");

    NeiSaveData save{};
    save.comboRpg = state;
    save.comboSpeedUpgrades = state.level[COMBO_RPG_SPEED];
    save.comboSpeedRequired = state.required[COMBO_RPG_SPEED];
    json disk;
    to_json(disk, save);
    NeiSaveData restored{};
    from_json(json::parse(disk.dump()), restored);
    check(ComboRpg::ToJson(restored.comboRpg) == wire, "real MM JSON save preserves RPG settings and counts");
    from_json({ { "comboSpeedUpgrades", 3 }, { "comboSpeedRequired", 10 } }, restored);
    check(near(ComboRpgState_SpeedMultiplier(&restored.comboRpg), 1.12f), "old speed-only saves migrate once");
    from_json(json::object(), restored);
    check(near(ComboRpgState_SpeedMultiplier(&restored.comboRpg), 1.0f) && restored.comboRpg.enabledMask == 0,
          "loading a legacy or new file clears the previous file's RPG state");

    liveSave = {};
    ApplyMmRpg({ { "rpgStats", wire } }); // RPG cap 50 already sets both native ownership flags.
    check(NativeMagicNext() == RI_SINGLE_MAGIC && NativeMagicObtainable(RI_PROGRESSIVE_MAGIC),
          "RPG-derived flags cannot skip the first native magic pickup");
    GiveNativeMagic(NativeMagicNext());
    check(ComboRpg_NativeMagicTier(2) == 1 && MM_PD.magic == 50 && NativeMagicNext() == RI_DOUBLE_MAGIC,
          "native first pickup retains larger RPG capacity and advances exactly one tier");
    GiveNativeMagic(NativeMagicNext());
    check(MM_PD.magic == 96 && ComboRpg_MagicCapacity(96) == 96 && !NativeMagicObtainable(RI_PROGRESSIVE_MAGIC),
          "native second pickup guarantees the solver's 96-unit capacity and stops the chain");
    GiveNativeMagic(RI_SINGLE_MAGIC);
    check(MM_PD.magic == 96 && ComboRpg_NativeMagicTier(2) == 2,
          "replayed lower concrete native tier cannot demote capacity");
    to_json(disk, liveSave);
    from_json(disk, restored);
    check(restored.comboRpg.nativeMagicLevel == 2, "native capacity floor survives the real MM JSON round trip");
    TestSave migration{};
    migration.saveInfo.playerData.isMagicAcquired = migration.saveInfo.playerData.isDoubleMagicAcquired = 1;
    MigrateMmMagic({ { "shipSaveInfo", { { "nei", { { "comboRpg", { { "nativeMagicLevel", 0 } } } } } } } }, migration);
    check(migration.shipSaveInfo.nei.comboRpg.nativeMagicLevel == 0,
          "new RPG saves never infer native magic from RPG-derived flags");
    MigrateMmMagic({ { "shipSaveInfo", { { "nei", json::object() } } } }, migration);
    check(migration.shipSaveInfo.nei.comboRpg.nativeMagicLevel == 2,
          "legacy MM save retains the native capacity it had before the RPG bridge");

    if (!failures)
        std::cout << "PASS: RPG caps, all stat formulas, partial sync, idempotency and MM save migration\n";
    return failures != 0;
}
