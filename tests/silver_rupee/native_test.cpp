#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

// Use the production enum values; only the game/registry boundary is a fixture.
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name, ...) name __VA_OPT__(=) __VA_ARGS__,
#define RANDO_ENUM_END(name) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerCheck.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerInf.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
#define DEFINE_SCENE(_0, _1, name, _3, _4, _5) name,
enum SceneID {
#include "soh/include/tables/scene_table.h"
    SCENE_ID_MAX
};
#undef DEFINE_SCENE
using s8 = int8_t;
using s16 = int16_t;
struct PlayState { s16 sceneNum = SCENE_ZORAS_DOMAIN; };
struct Vec3f { float x{}, y{}, z{}; };
struct Actor {
    struct { Vec3f pos{}; } world;
    int room = 1;
    bool killed = false;
    void (*draw)(Actor*, PlayState*) = nullptr;
};
enum { ENGSWITCH_SILVER_TRACKER, ENGSWITCH_SILVER_RUPEE };
struct EnGSwitch { Actor actor; int type = ENGSWITCH_SILVER_TRACKER; s16 switchFlag = 5; };
struct SilverFields {
    s8 silverShadowBlades{}, silverShadowPit{}, silverShadowSpikes{}, silverSpiritChild{}, silverSpiritSun{},
       silverSpiritBoulders{}, silverBotw{}, silverIceCavernBlades{}, silverIceCavernBlock{}, silverGtgSlope{},
       silverGtgLava{}, silverGtgWater{}, silverGanonLight{}, silverGanonForest{}, silverGanonFire{}, silverGanonSpirit{},
       silverMqDodongosCavern{}, silverMqShadowInvisibleBlades{}, silverMqSpiritLobby{}, silverMqSpiritBigWall{},
       silverMqGanonWater{}, silverMqGanonShadow{};
};
struct SaveContext { struct { struct { struct { SilverFields randomizer; } data; } quest; } ship; };
PlayState state;
PlayState* gPlayState = &state;
SaveContext gSaveContext{};
struct CheckIdentity { RandomizerCheck randomizerCheck; RandomizerInf randomizerInf; };
CheckIdentity identity{RC_UNKNOWN_CHECK, RAND_INF_MAX};
int savedChecks = 0, switchWrites = 0, clearedRooms = 0, hookCleanups = 0;
namespace Rando {
enum { SHADOW_TEMPLE, GERUDO_TRAINING_GROUND };
struct DungeonInfo { bool mq = false; bool IsMQ() const { return mq; } };
struct Context {
    std::array<DungeonInfo, SCENE_ID_MAX> dungeons{};
    static std::shared_ptr<Context> GetInstance() { static auto p = std::make_shared<Context>(); return p; }
    DungeonInfo* GetDungeonFromScene(SceneID scene) {
        switch (scene) {
            case SCENE_DODONGOS_CAVERN: case SCENE_SHADOW_TEMPLE: case SCENE_SPIRIT_TEMPLE:
            case SCENE_BOTTOM_OF_THE_WELL: case SCENE_ICE_CAVERN: case SCENE_GERUDO_TRAINING_GROUND:
            case SCENE_INSIDE_GANONS_CASTLE: return &dungeons[scene];
            default: return nullptr;
        }
    }
    DungeonInfo* GetDungeon(int key) {
        return &dungeons[key == SHADOW_TEMPLE ? SCENE_SHADOW_TEMPLE : SCENE_GERUDO_TRAINING_GROUND];
    }
};
}
struct Randomizer {
    static s8* SilverFieldFromSaveContext(SaveContext*, RandomizerGet);
    static s8 SilverTotal(RandomizerGet);
    CheckIdentity IdentifySilver(int, Vec3f) { return identity; }
};
struct OTRGlobals {
    static OTRGlobals* Instance;
    Randomizer* gRandomizer;
};
Randomizer randomizer;
OTRGlobals globals{&randomizer};
OTRGlobals* OTRGlobals::Instance = &globals;
void Actor_Kill(Actor* actor) { actor->killed = true; }
void Flags_SetRandomizerInf(RandomizerInf flag) { assert(flag < RAND_INF_MAX); ++savedChecks; }
bool Flags_GetRandomizerInf(RandomizerInf flag) { assert(flag < RAND_INF_MAX); return savedChecks != 0; }
void Flags_SetTempClear(PlayState*, int) { ++clearedRooms; }
void Flags_SetClear(PlayState*, int) { ++clearedRooms; }
void Flags_SetSwitch(PlayState*, int) { ++switchWrites; }
void EnGSwitch_RandomizerDraw(Actor*, PlayState*) {}
void NativeDraw(Actor*, PlayState*) {}
enum { VB_SILVER_COLLECT, VB_SILVER_COUNT_CHECK, VB_SILVER_DESPAWN };
std::array<std::function<void(bool*, va_list)>, 3> hooks;
#define IS_RANDO true
#define RAND_GET_OPTION(...) true
// Match REGISTER_VB_SHOULD's va_copy/body/va_end framing. A return in the
// production body would skip its caller's required varargs cleanup.
#define COND_VB_SHOULD(id, condition, ...) \
    do { if (condition) hooks[id] = [](bool* should, va_list original) { \
        va_list args; va_copy(args, original); __VA_ARGS__; va_end(args); ++hookCleanups; \
    }; } while (0)
#include "production.inc"
bool Invoke(int id, uint32_t initial, ...) {
    bool should = initial != 0;
    va_list args;
    va_start(args, initial);
    const int before = hookCleanups;
    hooks[id](&should, args);
    assert(hookCleanups == before + 1 && "hook must return through its va_end cleanup");
    va_end(args);
    return should;
}
void Unchanged(const EnGSwitch& silver) {
    assert(!silver.actor.killed && savedChecks == 0 && switchWrites == 0 && clearedRooms == 0);
    assert(silver.actor.draw == NativeDraw);
}
int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string_view scenario = argv[1];
    RegisterShuffleSilver();
    EnGSwitch silver;
    silver.actor.draw = NativeDraw;
    if (scenario == "mapping") {
        struct Case { SceneID scene; int flag; RandomizerGet vanilla, mq; };
        const Case cases[] = {
            {SCENE_DODONGOS_CAVERN, 0, RG_DODONGOS_CAVERN_MQ_SILVER, RG_DODONGOS_CAVERN_MQ_SILVER},
            {SCENE_SHADOW_TEMPLE, 1, RG_SHADOW_SILVER_BLADES, RG_SHADOW_SILVER_BLADES},
            {SCENE_SHADOW_TEMPLE, 3, RG_SHADOW_MQ_SILVER_INVISIBLE_BLADES, RG_SHADOW_MQ_SILVER_INVISIBLE_BLADES},
            {SCENE_SHADOW_TEMPLE, 8, RG_SHADOW_SILVER_SPIKES, RG_SHADOW_SILVER_SPIKES},
            {SCENE_SHADOW_TEMPLE, 9, RG_SHADOW_SILVER_PIT, RG_SHADOW_SILVER_PIT},
            {SCENE_SHADOW_TEMPLE, 17, RG_SHADOW_SILVER_PIT, RG_SHADOW_SILVER_PIT},
            {SCENE_SPIRIT_TEMPLE, 0, RG_SPIRIT_MQ_SILVER_BIG_WALL, RG_SPIRIT_MQ_SILVER_BIG_WALL},
            {SCENE_SPIRIT_TEMPLE, 2, RG_SPIRIT_SILVER_BOULDERS, RG_SPIRIT_SILVER_BOULDERS},
            {SCENE_SPIRIT_TEMPLE, 5, RG_SPIRIT_SILVER_CHILD, RG_SPIRIT_SILVER_CHILD},
            {SCENE_SPIRIT_TEMPLE, 10, RG_SPIRIT_SILVER_SUN, RG_SPIRIT_SILVER_SUN},
            {SCENE_SPIRIT_TEMPLE, 55, RG_SPIRIT_MQ_SILVER_LOBBY, RG_SPIRIT_MQ_SILVER_LOBBY},
            {SCENE_BOTTOM_OF_THE_WELL, 0, RG_BOTW_SILVER, RG_BOTW_SILVER},
            {SCENE_ICE_CAVERN, 8, RG_ICE_CAVERN_SILVER_BLADES, RG_ICE_CAVERN_SILVER_BLADES},
            {SCENE_ICE_CAVERN, 9, RG_ICE_CAVERN_SILVER_BLOCK, RG_ICE_CAVERN_SILVER_BLOCK},
            {SCENE_ICE_CAVERN, 31, RG_ICE_CAVERN_SILVER_BLADES, RG_ICE_CAVERN_SILVER_BLADES},
            {SCENE_GERUDO_TRAINING_GROUND, 12, RG_GTG_SILVER_LAVA, RG_GTG_SILVER_LAVA},
            {SCENE_GERUDO_TRAINING_GROUND, 27, RG_GTG_SILVER_WATER, RG_GTG_SILVER_WATER},
            {SCENE_GERUDO_TRAINING_GROUND, 28, RG_GTG_SILVER_SLOPE, RG_GTG_SILVER_SLOPE},
            {SCENE_INSIDE_GANONS_CASTLE, 1, RG_GANONS_CASTLE_SILVER_FIRE, RG_GANONS_CASTLE_SILVER_FIRE},
            {SCENE_INSIDE_GANONS_CASTLE, 2, RG_GANONS_CASTLE_MQ_SILVER_WATER, RG_GANONS_CASTLE_MQ_SILVER_WATER},
            {SCENE_INSIDE_GANONS_CASTLE, 9, RG_GANONS_CASTLE_SILVER_FIRE, RG_GANONS_CASTLE_SILVER_FIRE},
            {SCENE_INSIDE_GANONS_CASTLE, 11, RG_GANONS_CASTLE_SILVER_SPIRIT, RG_GANONS_CASTLE_MQ_SILVER_SHADOW},
            {SCENE_INSIDE_GANONS_CASTLE, 14, RG_GANONS_CASTLE_SILVER_FOREST, RG_GANONS_CASTLE_SILVER_FOREST},
            {SCENE_INSIDE_GANONS_CASTLE, 18, RG_GANONS_CASTLE_SILVER_LIGHT, RG_GANONS_CASTLE_SILVER_LIGHT},
        };
        for (const auto& c : cases) for (bool mq : {false, true}) {
            state.sceneNum = c.scene;
            Rando::Context::GetInstance()->dungeons[c.scene].mq = mq;
            assert(SilverFromSwitchFlag(c.flag) == (mq ? c.mq : c.vanilla));
        }
    } else if (scenario == "domain-tracker" || scenario == "unknown-flag") {
        // The crash report's 0x0185 actor is a six-rupee tracker on switch 5 in scene 0x58.
        if (scenario == "unknown-flag") { state.sceneNum = SCENE_SHADOW_TEMPLE; silver.switchFlag = 63; }
        assert(SilverFromSwitchFlag(silver.switchFlag) == RG_NONE);
        for (bool initial : {false, true}) {
            assert(Invoke(VB_SILVER_DESPAWN, initial, &silver) == initial);
            assert(Invoke(VB_SILVER_COUNT_CHECK, initial, &silver) == initial);
        }
        Unchanged(silver);
    } else if (scenario == "domain-rupee" || scenario == "unknown-check") {
        if (scenario == "unknown-check") state.sceneNum = SCENE_SHADOW_TEMPLE;
        silver.type = ENGSWITCH_SILVER_RUPEE;
        for (bool initial : {false, true}) {
            assert(Invoke(VB_SILVER_DESPAWN, initial, &silver) == initial);
            assert(Invoke(VB_SILVER_COLLECT, initial, &silver) == initial);
        }
        Unchanged(silver);
    } else if (scenario == "known-tracker") {
        state.sceneNum = SCENE_SHADOW_TEMPLE;
        silver.switchFlag = 1;
        assert(!Invoke(VB_SILVER_COUNT_CHECK, true, &silver));
        assert(!silver.actor.killed);
        gSaveContext.ship.quest.data.randomizer.silverShadowBlades = 5;
        assert(Invoke(VB_SILVER_DESPAWN, false, &silver));
        assert(switchWrites == 1);
        assert(!Invoke(VB_SILVER_COUNT_CHECK, true, &silver));
        assert(silver.actor.killed && switchWrites == 2);
    } else if (scenario == "known-rupee") {
        state.sceneNum = SCENE_SHADOW_TEMPLE;
        silver.type = ENGSWITCH_SILVER_RUPEE;
        identity = {RC_SHADOW_N_UNDER_BLADE_SILVER, RAND_INF_RC_SHADOW_N_UNDER_BLADE_SILVER};
        assert(!Invoke(VB_SILVER_DESPAWN, false, &silver));
        assert(silver.actor.draw == EnGSwitch_RandomizerDraw);
        assert(!Invoke(VB_SILVER_COLLECT, true, &silver));
        assert(silver.actor.killed && savedChecks == 1);
        assert(Invoke(VB_SILVER_DESPAWN, false, &silver));
    } else if (scenario == "missing-play") {
        gPlayState = nullptr;
        assert(SilverFromSwitchFlag(5) == RG_NONE);
    } else if (scenario == "invalid-group") {
        assert(!IsSilverCleared(RG_NONE));
    } else {
        assert(false);
    }
}
