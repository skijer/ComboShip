#include "rando/RpgStatsJson.h"
#include <array>
#include <cmath>
#include <iostream>
#include <map>

#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name, ...) name,
#define RANDO_ENUM_END(name) \
    }                        \
    ;
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerSettingKey.h"
#include "soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END

struct TestOption {
    uint8_t value = 0;
    uint8_t Get() {
        return value;
    }
    operator bool() const {
        return value != 0;
    }
};
namespace Rando {
struct Context {
    TestOption options[RSK_MAX];
    TestOption& GetOption(RandomizerSettingKey key) {
        return options[key];
    }
    static Context* GetInstance() {
        static Context context;
        return &context;
    }
};
} // namespace Rando
struct PlayState {};
struct StatSave {
    uint8_t quarterHearts, defenseUpgrades, speedUpgrades, powerUpgrades, magicStatUpgrades;
    uint8_t crawlSpeedUpgrades, climbSpeedUpgrades, pushSpeedUpgrades;
    uint8_t comboNativeMagicLevel;
};
static struct {
    struct {
        struct {
            struct {
                StatSave randomizer;
            } data;
        } quest;
    } ship;
    int16_t health = 0x30, healthCapacity = 0x30, magicFillTarget = 0, magic = 0;
    uint8_t magicLevel = 0, isMagicAcquired = 0, isDoubleMagicAcquired = 0;
} gSaveContext;
static bool isRando = true;
#define IS_RANDO isRando
#define RAND_GET_OPTION(key) Rando::Context::GetInstance()->GetOption(key)
#define FULL_HEART_HEALTH 16
#define MAGIC_NORMAL_METER 48
#define MAGIC_DOUBLE_METER 96
#define MIN(a, b) ((a) < (b) ? (a) : (b))
static int fills;
static void Magic_Fill(PlayState*) {
    ++fills;
}
static std::map<RandomizerGet, std::array<int, 4>> pool;
static void AddItemToPool(RandomizerGet item, int plentiful, int balanced, int scarce, int minimal, bool) {
    pool[item] = { plentiful, balanced, scarce, minimal };
}
#include "rpg_soh_bridge_production.inc"

static int failures;
static void check(bool ok, const char* message) {
    if (!ok) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

int main() {
    auto ctx = Rando::Context::GetInstance();
    const RandomizerGet items[] = { RG_DEFENSE_UPGRADE,    RG_SPEED_UPGRADE,       RG_POWER_UPGRADE,
                                    RG_MAGIC_STAT_UPGRADE, RG_CRAWL_SPEED_UPGRADE, RG_CLIMB_SPEED_UPGRADE,
                                    RG_PUSH_SPEED_UPGRADE };
#define SET_RULES(key)                                \
    ctx->GetOption(RSK_##key).value = 1;              \
    ctx->GetOption(RSK_##key##_ADJUSTABLE).value = 1; \
    ctx->GetOption(RSK_##key##_TOTAL).value = 19;     \
    ctx->GetOption(RSK_##key##_REQUIRED).value = 24;
    SET_RULES(DEFENSE_UPGRADE)
    SET_RULES(SPEED_UPGRADE)
    SET_RULES(POWER_UPGRADE)
    SET_RULES(MAGIC_STAT_UPGRADE)
    SET_RULES(CRAWL_SPEED_UPGRADE)
    SET_RULES(CLIMB_SPEED_UPGRADE)
    SET_RULES(PUSH_SPEED_UPGRADE)
#undef SET_RULES
    ctx->GetOption(RSK_QUARTER_HEART).value = 1;
    GenerateRpgPool();
    for (auto item : items)
        check(pool[item] == std::array<int, 4>{ 20, 20, 20, 20 }, "saved totals generate all seven RPG pools");
    for (auto item : items)
        for (int i = 0; i < 3; ++i)
            GiveRpg(nullptr, item);
    GiveRpg(nullptr, RG_QUARTER_HEART);
    nlohmann::json shared;
    FleetRpg::Extract(shared);
    ComboRpgState mm{};
    ComboRpg::MergeJson(mm, shared.at("rpgStats"));
    for (int i = 0; i < COMBO_RPG_COUNT; ++i)
        check(mm.level[i] == 3 && mm.required[i] == 5 && ComboRpgState_Enabled(&mm, i),
              "production OoT pickups export saved caps and levels to MM");
    check(mm.magicTotal == 100 && mm.quarterHeartsEnabled && gSaveContext.healthCapacity == 0x34 && fills == 3,
          "adjustable magic and quarter hearts cross from the donor settings");
    FleetRpg::Apply({ { "rpgStats", ComboRpg::ToJson(mm) } });
    check(gSaveContext.ship.quest.data.randomizer.speedUpgrades == 3,
          "returning an MM snapshot does not grant a second stat item");
    for (auto item : items)
        for (int i = 0; i < 20; ++i)
            GiveRpg(nullptr, item);
    FleetRpg::Extract(shared);
    ComboRpg::MergeJson(mm, shared.at("rpgStats"));
    for (int i = 0; i < COMBO_RPG_COUNT; ++i)
        check(mm.level[i] == 5, "extra pool copies stop at the saved required cap");
    FleetRpg::Apply({ { "rpgStats", { { "speed", { { "enabled", false }, { "required", 1 }, { "level", 80 } } } } } });
    check(FleetRpg::Read().required[COMBO_RPG_SPEED] == 5 && gSaveContext.ship.quest.data.randomizer.speedUpgrades == 5,
          "MM metadata cannot override the OoT seed or exceed its cap");
    gSaveContext.ship.quest.data.randomizer.magicStatUpgrades = 1;
    FleetRpg::Apply({ { "rpgStats", { { "nativeMagicLevel", 1 } } } });
    check(gSaveContext.magic == 48 && gSaveContext.ship.quest.data.randomizer.comboNativeMagicLevel == 1,
          "MM native first magic tier raises OoT fractional capacity to 48");
    FleetRpg::Apply({ { "rpgStats", { { "nativeMagicLevel", 2 } } } });
    check(gSaveContext.magic == 96, "MM native double magic raises OoT capacity to 96");
    gSaveContext.magic = 7;
    FleetRpg::Apply({ { "rpgStats", { { "nativeMagicLevel", 2 } } } });
    check(gSaveContext.magic == 7, "returning native floor snapshot cannot refill OoT twice");
    GiveRpg(nullptr, RG_MAGIC_STAT_UPGRADE);
    check(gSaveContext.magicFillTarget == 96 && gSaveContext.ship.quest.data.randomizer.magicStatUpgrades == 2,
          "later OoT stat pickup preserves native MM capacity and counts only once");
    FleetRpg::Extract(shared);
    ComboRpg::MergeJson(mm, shared.at("rpgStats"));
    check(mm.nativeMagicLevel == 2, "native magic floor returns from OoT without an additive grant");
    ctx->GetOption(RSK_SPEED_UPGRADE).value = 0;
    gSaveContext.ship.quest.data.randomizer.speedUpgrades = 0;
    FleetRpg::Apply({ { "rpgStats", { { "speed", { { "level", 5 } } } } } });
    check(gSaveContext.ship.quest.data.randomizer.speedUpgrades == 0, "disabled seed stat ignores stale peer counters");
    pool.clear();
    GenerateRpgPool();
    check(pool.count(RG_SPEED_UPGRADE) == 0, "disabled RPG setting removes items from generation");
    // Foreign/shared native grants must retain their full capacity even when
    // OoT's own pool substitutes fractional RPG magic for native magic items.
    for (int nativeFirst = 0; nativeFirst < 2; ++nativeFirst) {
        for (int tier = 1; tier <= 2; ++tier) {
            gSaveContext = {};
            if (!nativeFirst)
                GiveRpg(nullptr, RG_MAGIC_STAT_UPGRADE);
            GiveNativeOotMagic(nullptr, tier == 1 ? RG_MAGIC_SINGLE : RG_MAGIC_DOUBLE);
            check(gSaveContext.ship.quest.data.randomizer.comboNativeMagicLevel == tier,
                  "native OoT grant records its capacity floor independently of RPG ownership");
            GiveRpg(nullptr, RG_MAGIC_STAT_UPGRADE);
            check(gSaveContext.magicFillTarget == tier * 48,
                  "native OoT capacity survives later fractional RPG pickups in either order");
            FleetRpg::Extract(shared);
            mm = {};
            ComboRpg::MergeJson(mm, shared.at("rpgStats"));
            check(mm.nativeMagicLevel == tier, "native OoT grant survives the cross-game snapshot");
            if (tier == 2) {
                GiveNativeOotMagic(nullptr, RG_MAGIC_SINGLE);
                check(gSaveContext.ship.quest.data.randomizer.comboNativeMagicLevel == 2,
                      "duplicate first-tier magic cannot downgrade a native double-magic floor");
            }
        }
    }
    isRando = false;
    FleetRpg::Extract(shared);
    ComboRpg::MergeJson(mm, shared.at("rpgStats"));
    check(mm.enabledMask == 0 && ComboRpgState_SpeedMultiplier(&mm) == 1.0f,
          "nonrandomized state explicitly disables mirrored RPG effects");
    if (!failures)
        std::cout << "PASS: real OoT RPG pool, pickup, saved-rule export and idempotent MM return\n";
    return failures != 0;
}
