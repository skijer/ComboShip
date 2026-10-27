#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/RegionalPassages.h"
#include "gameplay/MagicMirrorArrival.h"
#include "global.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <vector>
#define COMBO_EXPORT

struct GameInteractor {
    struct OnSceneInit { using Function = std::function<void(s8, s8)>; };
    struct OnActorUpdate { using Function = std::function<void(Actor*)>; };
    struct OnGameStateMainStart { using Function = std::function<void()>; };
    struct OnGameStateMainFinish { using Function = std::function<void()>; };
    struct OnPlayDestroy { using Function = std::function<void()>; };
    static GameInteractor* Instance;
    template<class Hook> static auto& Hooks() {
        static std::vector<typename Hook::Function> hooks;
        return hooks;
    }
    template<class Hook> void RegisterGameHook(typename Hook::Function fn) { Hooks<Hook>().push_back(fn); }
    template<class Hook> void RegisterGameHookForID(int, typename Hook::Function fn) { Hooks<Hook>().push_back(fn); }
    template<class Hook, class... Args> static void Fire(Args... args) {
        for (auto& hook : Hooks<Hook>()) hook(args...);
    }
};
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
extern "C" void ComboForestArrivalCamera_Queue(int) {}
static PlayState play{};
static Player player{};
static bool woodfallPassage;
extern "C" void ComboWoodfallPassage_Reset(void) {}
extern "C" int ComboWoodfallPassage_AtReturn(PlayState* current, const Vec3f*) {
    return woodfallPassage && current && current->sceneId == SCENE_21MITURINMAE &&
           current->roomCtx.curRoom.num == 0;
} // floor/wall services are exercised with the production endpoint in woodfall_geometry_test.cpp
static int saves, switches, recordedRoute, kind, fwCaptures;
static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
namespace Ship { struct Window { virtual ~Window() = default; }; }
namespace Fast { struct Fast3dWindow : Ship::Window { void SetIsRunning(bool) {} }; }
namespace Ship {
    struct Context {
        static Context* GetRawInstance() { static Context context; return &context; }
        std::shared_ptr<Window> GetWindow() { return std::make_shared<Fast::Fast3dWindow>(); }
    };
}
namespace ComboCrossMmRuntime {
    static bool pending = false;
    static bool PrepareSwitch() { bool value = pending; pending = false; return value; }
    static void ObserveScene(int) {}
}
extern "C" int Combo_MM_CrossDungeonTransitionPending(void) { return ComboCrossMmRuntime::pending; }
extern "C" void Combo_MM_CancelCrossDungeonTransition(void) { ComboCrossMmRuntime::pending = false; }
static void Return(int value) { ++switches; kind = value; }
static void (*gComboReturnCallback)(int) = Return;
static void SetOotRoute(int route) { recordedRoute = route; }
static void* Combo_ResolveSym(const char*, const char*) { return (void*)SetOotRoute; }
extern "C" s32 CVarGetInteger(const char*, s32) { return 0; }
extern "C" void MM_CaptureFwDeparture(void) { ++fwCaptures; }
void Combo_MM_FlushAccumulators() {}
void SaveManager_SaveCurrentForCombo(bool = false) { ++saves; }

static int regionalReturn;
extern "C" int ComboRegionalPassagesMm_ApplyArrival(int) { return 1; }
static bool regionalReady=true;
extern "C" void ComboRegionalPassagesMm_Reset(void) { regionalReturn=0; }
extern "C" int ComboRegionalPassagesMm_AtReturn(PlayState*,const Vec3f*) { return regionalReturn; }
extern "C" int ComboRegionalPassagesMm_Get(PlayState*,int*,Vec3f*,s16*) { return regionalReady; }

/* RETURN_STATE */
void RegisterCandidate() {
    /* HANDOFF_HOOKS */
}
static void EnterPassage(int slot) {
    Combo_ClearReturnRequests();
    gPlayState = &play;
    gGameState = &play.state;
    play = {};
    play.state.running = true;
    play.sceneId = SCENE_26SARUNOMORI;
    play.roomCtx.curRoom.num = 1;
    player.actor.world.pos = { 160, 0, -160 };
    gSaveContext.fileNum = slot;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    GameInteractor::Fire<GameInteractor::OnActorUpdate>(&player.actor);
}
static void Frames() {
    for (int i = 0; i < 12; ++i) {
        GameInteractor::Fire<GameInteractor::OnGameStateMainStart>();
        GameInteractor::Fire<GameInteractor::OnGameStateMainFinish>();
    }
}
int main() {
    RegisterCandidate();
    EnterPassage(0);
    GameInteractor::Fire<GameInteractor::OnGameStateMainStart>();
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    gPlayState = nullptr; gGameState = nullptr;
    Frames();
    gPlayState = &play; gGameState = &play.state; play.state.running = true;
    gSaveContext.fileNum = 2;
    Frames();
    require(saves == 0 && switches == 0, "destroyed MM Play must never resume a forest fade in another slot");
    EnterPassage(0);
    gSaveContext.fileNum = 2;
    Frames();
    require(saves == 0 && switches == 0, "MM forest fade must validate its captured source slot");
    EnterPassage(0);
    play.state.running = false;
    Frames();
    require(saves == 0 && switches == 0, "MM forest fade must reject a stopped Play state");
    EnterPassage(0);
    Frames();
    require(saves == 1 && switches == 1 && kind == 0 && recordedRoute == COMBO_PORTAL_LOST_WOODS,
            "a valid MM forest passage must save and hand off exactly once");
    EnterPassage(0);
    ComboCrossMmRuntime::pending = true;
    Frames();
    require(saves == 2 && switches == 2 && recordedRoute == COMBO_PORTAL_MARKET,
            "a shuffled MM arrival must supersede a forest fade");
    EnterPassage(0);
    MM_QueueFwHandoff();
    Frames();
    require(saves == 3 && switches == 3 && kind == 3 && fwCaptures == 1 && recordedRoute == COMBO_PORTAL_MARKET,
            "Farore must retain its capture/handoff and discard the forest route");
    EnterPassage(0);
    MM_RequestComboReturn();
    Frames();
    require(saves == 3 && switches == 4 && kind == 1 && recordedRoute == COMBO_PORTAL_MARKET,
            "reset must cancel the forest handoff and preserve the autosave policy");
    Combo_ClearReturnRequests();
    play = {};
    play.state.running = true;
    gPlayState = &play;
    gGameState = &play.state;
    play.sceneId = SCENE_LOST_WOODS;
    play.roomCtx.curRoom.num = 0;
    ActorEntry openingSpawn{};
    openingSpawn.pos = {-417, 0, -122};
    openingSpawn.rot.y = 11520;
    play.linkActorEntry = &openingSpawn;
    gSaveContext.fileNum = 0;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.sceneLayer = 0;
    gSaveContext.save.cutsceneIndex = 0;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
    player.actor.world.pos = {-417, 0, -122};
    GameInteractor::Fire<GameInteractor::OnActorUpdate>(&player.actor);
    Frames();
    require(saves == 3 && switches == 4,
            "opening arrival must not immediately return through its arch");
    player.actor.world.pos = ComboOpeningForest_ReturnAnchor(&play);
    GameInteractor::Fire<GameInteractor::OnActorUpdate>(&player.actor);
    Frames();
    require(saves == 4 && switches == 5 &&
                recordedRoute == COMBO_PORTAL_OPENING_FOREST,
            "walking from the native arrival to the stump must return to the pink "
            "Fairy tunnel once");
    std::puts("PASS: production MM forest handoff, Play/slot lifetime, reset, shuffled arrivals and Farore ownership");
}
