#include <cstdio>
#include <cstdlib>
#include <functional>
#include <vector>
#include "global.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/RegionalPassages.h"

// Only event dispatch, DLL resolution and disk IO are boundaries. The registered
// production trigger/fade/save/handoff bodies and all engine state types stay real.
struct GameInteractor {
    struct OnSceneInit { using Function = std::function<void(int16_t)>; };
    struct OnPlayerUpdate { using Function = std::function<void()>; };
    struct OnGameFrameUpdate { using Function = std::function<void()>; };
    struct OnPlayDestroy { using Function = std::function<void()>; };
    struct OnExitGame { using Function = std::function<void(int32_t)>; };
    static GameInteractor* Instance;
    template<class Hook> static auto& Hooks() {
        static std::vector<typename Hook::Function> hooks;
        return hooks;
    }
    template<class Hook> void RegisterGameHook(typename Hook::Function fn) { Hooks<Hook>().push_back(fn); }
    template<class Hook, class... Args> static void Fire(Args... args) {
        for (auto& hook : Hooks<Hook>()) hook(args...);
    }
};
static GameInteractor instance;
GameInteractor* GameInteractor::Instance = &instance;
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
static PlayState play{};
static Player player{};
static int saves, switches, recordedRoute, savedSlot;
static int sComboFwSwitchSlot = -1;
static int sComboWoodfallPassageSlot = -1;
namespace ComboCrossOotRuntime {
    static bool pending = false;
    static struct { bool Enabled() const { return false; } } state;
    static bool PrepareSwitch() { bool value = pending; pending = false; return value; }
    static void ObserveScene(int) {}
}
extern "C" int Combo_SOH_CrossDungeonTransitionPending(void) { return ComboCrossOotRuntime::pending; }
extern "C" void Combo_SOH_CancelCrossDungeonTransition(void) { ComboCrossOotRuntime::pending = false; }
void Combo_SOH_CapturePendingChecks() {}
void Combo_FlushDormantAccumulators() {}
extern "C" void Play_PerformSave(PlayState*) { ++saves; savedSlot = gSaveContext.fileNum; }
static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
struct SaveManager {
    static SaveManager* Instance;
    void SaveFile(int slot) { ++saves; savedSlot = slot; }
    void ThreadPoolWait() {}
};
static SaveManager saveManager;
SaveManager* SaveManager::Instance = &saveManager;
static void Switch(int) { ++switches; recordedRoute = COMBO_PORTAL_MARKET; }
static void SetMmRoute(int route) { recordedRoute = route; }
static void (*gComboSceneSwitchCallback)(int) = Switch;
static bool mirrorDestinationAvailable = true;
static void* Combo_ResolveSym(const char*, const char*) {
    return mirrorDestinationAvailable ? (void*)SetMmRoute : nullptr;
}
extern "C" s32 Player_InCsMode(PlayState*) { return 0; }

// Regional collision/actor policy is exercised separately with its production
// endpoint module; this fixture keeps DLL/endpoint dispatch as boundaries.
static int regionalDeparture;
static bool regionalReady=true, regionalFall;
extern "C" int ComboRegionalPassagesOot_AtDeparture(PlayState*,const Vec3f*) { return regionalDeparture; }
extern "C" int ComboRegionalPassagesOot_Get(PlayState*,int*,Vec3f*,s16*) { return regionalReady; }
extern "C" int ComboRegionalPassagesOot_OwnsGraveFall(PlayState*) { return regionalFall; }
extern "C" void ComboRegionalPassagesOot_CancelDeparture(void) { regionalDeparture=0; regionalFall=false; }

/* MIRROR_QUEUE */

void RegisterCandidate() {
    /* HANDOFF_HOOKS */
}

static void EnterTunnel(int slot, bool openingForest = false) {
  gPlayState = &play;
  gGameState = &play.state;
  play.state.running = true;
  play.sceneNum = SCENE_LOST_WOODS;
  play.roomCtx.curRoom.num = openingForest ? 1 : 0;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
  player.actor.world.pos =
      openingForest ? Vec3f{800, 0, 340} : Vec3f{0, 0, -375};
  gSaveContext.fileNum = slot;
  gSaveContext.gameMode = GAMEMODE_NORMAL;
  GameInteractor::Fire<GameInteractor::OnPlayerUpdate>();
}
static void Frames() {
    for (int i = 0; i < 12; ++i) GameInteractor::Fire<GameInteractor::OnGameFrameUpdate>();
}
int main() {
    RegisterCandidate();
    EnterTunnel(0);
    GameInteractor::Fire<GameInteractor::OnGameFrameUpdate>();
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>(); // Ctrl+R during the fade
    gPlayState = nullptr;
    gGameState = nullptr;
    Frames();
    gPlayState = &play; // arena can reuse exactly the same address for an attract demo
    gGameState = &play.state;
    play.state.running = true;
    gSaveContext.fileNum = 0xFF;
    gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN;
    Frames();
    require(saves == 0 && switches == 0, "an interrupted crossing must never save or switch an attract demo");

    EnterTunnel(0);
    gSaveContext.fileNum = 2; // reject a source-slot change even before Play destruction
    Frames();
    require(saves == 0 && switches == 0, "an armed fade must never save another slot's context");
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    EnterTunnel(0);
    GameInteractor::Fire<GameInteractor::OnExitGame>(0);
    Frames();
    require(saves == 0 && switches == 0, "a soft reset must cancel before Play destruction or any further frame");
    EnterTunnel(0);
    Frames();
    require(saves == 1 && switches == 1 && savedSlot == 0 && recordedRoute == COMBO_PORTAL_LOST_WOODS,
            "the first save slot must hand off once and set the Woods route after the launch callback");
    require(!play.state.running, "a completed handoff must stop the active game loop");

    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    play.state.running = true;
    GameInteractor::Fire<GameInteractor::OnSceneInit>(SCENE_HAPPY_MASK_SHOP);
    GameInteractor::Fire<GameInteractor::OnGameFrameUpdate>();
    require(saves == 2 && switches == 2 && recordedRoute == COMBO_PORTAL_MARKET,
            "the following Mask Shop handoff must discard the old Woods route");
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    EnterTunnel(0);
    ComboCrossOotRuntime::pending = true;
    Frames();
    require(saves == 3 && switches == 3 && recordedRoute == COMBO_PORTAL_MARKET && !play.envCtx.fillScreen,
            "a shuffled handoff must supersede an armed forest fade without leaving a forest route");
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    EnterTunnel(0);
    sComboFwSwitchSlot = 0;
    Frames();
    require(saves == 4 && switches == 3 && recordedRoute == COMBO_PORTAL_MARKET && !play.envCtx.fillScreen,
            "Farore recall must cancel a forest fade and keep its own handoff ownership");
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    EnterTunnel(0, true);
    Frames();
    require(saves == 5 && switches == 4 &&
                recordedRoute == COMBO_PORTAL_OPENING_FOREST,
            "the pink Fairy tunnel must save and queue the separate "
            "opening-forest route exactly once");
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    EnterTunnel(0, true);
    GameInteractor::Fire<GameInteractor::OnExitGame>(0);
    Frames();
    require(saves == 5 && switches == 4,
            "reset during the pink tunnel's fade must never save or launch a "
            "new session");

    // The Mirror uses the existing clean-frame fade and save route without
    // placing Link in either physical passage or borrowing Farore's return.
    EnterTunnel(1);
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    play.sceneNum = SCENE_HYRULE_FIELD;
    require(SOH_QueueMagicMirrorHandoff() == 1, "a live combined slot can queue the Mirror crossing");
    Frames();
    require(saves == 6 && switches == 5 && savedSlot == 1 &&
                recordedRoute == COMBO_PORTAL_MAGIC_MIRROR && !play.state.running,
            "Mirror recall must save its source slot and select the stump arrival without the tunnel camera");
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>();
    play.state.running = true;
    gComboGameMode = COMBO_GAME_OOT;
    require(SOH_QueueMagicMirrorHandoff() == 0, "solo mode must use its native local forest route");
    gComboGameMode = COMBO_GAME_BOTH;
    mirrorDestinationAvailable = false;
    require(SOH_QueueMagicMirrorHandoff() == 0, "a missing destination adapter must reject the request");
    mirrorDestinationAvailable = true;
    sComboFwSwitchSlot = 1;
    require(SOH_QueueMagicMirrorHandoff() == 0, "Mirror must not replace a pending Farore return");
    sComboFwSwitchSlot = -1;
    require(SOH_QueueMagicMirrorHandoff() == 1, "valid recall can arm again after cancellation");
    gSaveContext.fileNum = 2;
    Frames();
    require(saves == 6 && switches == 5, "Mirror must not save or launch a replacement source slot");
    gSaveContext.fileNum = 1;
    require(SOH_QueueMagicMirrorHandoff() == 1, "the original slot can queue a fresh recall");
    GameInteractor::Fire<GameInteractor::OnExitGame>(0);
    Frames();
    require(saves == 6 && switches == 5, "reset must cancel a queued Mirror recall before it saves");
    std::puts("PASS: production OoT fade/handoff, Mirror recall, reset cancellation, source-slot safety and portal routing");
}
