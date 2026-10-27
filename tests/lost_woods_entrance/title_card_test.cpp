// Execute the complete production enhancement and native MM title-card
// init/update/draw bodies. Hooks, resource availability and GPU setup are boundaries.
#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/GameInteractor/GameInteractor_VanillaBehavior.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <vector>

#include "ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }

SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
static bool playerInCutscene, hideTitleCards;
extern "C" bool Player_InCsMode(PlayState*) { return playerInCutscene; }
extern "C" bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t value, ...) {
    assert(behavior == VB_SHOW_TITLE_CARD);
    return value && !hideTitleCards;
}

struct GameInteractor {
    struct OnSceneInit { static inline std::vector<std::function<void(s8, s8)>> callbacks; };
    struct OnGameStateUpdate { static inline std::vector<std::function<void()>> callbacks; };
    struct OnPlayDestroy { static inline std::vector<std::function<void()>> callbacks; };
    static GameInteractor* Instance;
    template <typename Hook, typename Callback> void RegisterGameHook(Callback callback) {
        Hook::callbacks.push_back(callback);
    }
};
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
struct RegisterShipInitFunc {
    RegisterShipInitFunc(std::function<void()> fn, const std::set<std::string>& = {}) { fn(); }
};

namespace Ship {
struct ArchiveManager {
    std::set<std::string> files;
    bool HasFile(const std::string& path) { return files.contains(path); }
};
struct ResourceManager {
    std::shared_ptr<ArchiveManager> archive = std::make_shared<ArchiveManager>();
    bool alt = false;
    auto GetArchiveManager() { return archive; }
    bool IsAltAssetsEnabled() { return alt; }
};
struct CrossRMRegistry {
    static inline std::map<std::string, std::shared_ptr<ResourceManager>> managers;
    static std::shared_ptr<ResourceManager> Get(const std::string& game) {
        const auto it = managers.find(game);
        return it == managers.end() ? nullptr : it->second;
    }
};
} // namespace Ship

// Native render entry/exit markers and DL setup are GPU boundaries. The title
// draw itself still writes the real GBI texture load and rectangle commands.
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int) {}
extern "C" void FrameInterpolation_RecordCloseChild() {}
extern "C" Gfx* Gfx_SetupDL52_NoCD(Gfx* gfx) { return gfx; }

/* NATIVE_TITLE_CARD */
/* ENHANCEMENT */

static PlayState play;
static GraphicsContext graphics;
static Gfx overlay[128], opaque[128], translucent[128];
static const char* kTexture = "custom/oot_location_titles/v1/lost_woods";

static void Reset() {
    for (const auto& fn : GameInteractor::OnPlayDestroy::callbacks) fn();
    play = {};
    graphics = {};
    gSaveContext = {};
    gPlayState = &play;
    gGameState = &play.state;
    play.state.running = true;
    play.state.gfxCtx = &graphics;
    graphics.overlay.p = overlay;
    graphics.polyOpa.p = opaque;
    graphics.polyXlu.p = translucent;
    play.sceneId = SCENE_LOST_WOODS;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.fileNum = 1;
    gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
    gSaveContext.showTitleCard = true;
    std::memset(&gSaveContext.save.saveInfo, 0x5A, sizeof(gSaveContext.save.saveInfo));
    playerInCutscene = hideTitleCards = false;
    auto owner = std::make_shared<Ship::ResourceManager>();
    owner->archive->files.insert(kTexture);
    Ship::CrossRMRegistry::managers = {{"mm", owner}};
}

static void Arrive() {
    for (const auto& fn : GameInteractor::OnSceneInit::callbacks) fn(play.sceneId, 0);
}
static void Tick() {
    TitleCard_Update(&play.state, &play.actorCtx.titleCtx);
    for (const auto& fn : GameInteractor::OnGameStateUpdate::callbacks) fn();
}
static bool CardActive() {
    const auto& card = play.actorCtx.titleCtx;
    return card.durationTimer || card.delayTimer || card.alpha;
}
static void Require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main() {
    Reset();
    // Play_Init arms every ordinary entry fade with TRANS_TRIGGER_END. This
    // is still an arrival, even before Play_UpdateTransition starts its mode.
    play.transitionTrigger = TRANS_TRIGGER_END;
    Arrive();
    const SaveContext before = gSaveContext;
    // Entrance fades, pause, death, native cutscenes and player entry control
    // must settle before the title is armed.
    play.transitionMode = TRANS_MODE_INSTANCE_WAIT;
    Tick();
    Require(!CardActive(), "the card must wait until the entrance fade finishes");
    play.transitionMode = TRANS_MODE_OFF;
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    Tick();
    Require(!CardActive(), "pause must defer the arrival card");
    play.pauseCtx.state = PAUSE_STATE_OFF;
    play.gameOverCtx.state = GAMEOVER_DEATH_START;
    Tick();
    Require(!CardActive(), "game over must defer the arrival card");
    play.gameOverCtx.state = GAMEOVER_INACTIVE;
    play.csCtx.state = CS_STATE_RUN;
    Tick();
    Require(!CardActive(), "native cutscenes must defer the arrival card");
    play.csCtx.state = CS_STATE_IDLE;
    playerInCutscene = true;
    Tick();
    Require(!CardActive(), "player entry control must finish before the card");
    playerInCutscene = false;
    Tick();
    Require(CardActive(), "a safe playable MM Lost Woods arrival must present its title card");
    auto& card = play.actorCtx.titleCtx;
    Require(card.x == 160 && card.y == 120 && card.width == 144 && card.height == 24,
            "the reused OoT place card must retain its native dimensions and centered placement");
    Require(card.durationTimer == 80 && card.delayTimer == 20 && card.alpha == 0 && card.intensity == 0,
            "the place card must retain native OoT delay and MM fade timing");
    Require(!std::strcmp((const char*)card.texture,
                        "__OTR__@mm:custom/oot_location_titles/v1/lost_woods"),
            "the title must use the exact OoT Lost Woods texture through its owning resource manager");
    /* ROUTED_ALIGNMENT */
    for (int i = 0; i < 45; ++i) Tick();
    Require(card.alpha == 255, "the production MM title update must fade the card fully in");
    TitleCard_Draw(&play.state, &card);
    int matchingTextures = 0;
    for (auto* command = overlay; command != graphics.overlay.p; ++command)
        matchingTextures += command->words.w1 == (uintptr_t)card.texture;
    Require(matchingTextures == 1, "the production MM draw must submit the routed texture exactly once");
    for (int i = 0; i < 240; ++i) Tick();
    Require(!CardActive(), "a visit must finish its title without restarting each frame");
    Require(!std::memcmp(&before, &gSaveContext, sizeof(before)),
            "showing the title must preserve every story, cycle, history and save byte");
    Arrive();
    Tick();
    Require(CardActive(), "a later arrival must get a fresh single title presentation");

    // Combo loads Save directly, bypassing MM file select's transient flag.
    // The forest prologue bypass also skips the native reset of this flag.
    Reset();
    gSaveContext.showTitleCard = false;
    const SaveContext directLoad = gSaveContext;
    Arrive(); Tick();
    Require(CardActive(), "a direct Combo load with a stale native title flag must still show the forest card");
    Require(!std::memcmp(&directLoad, &gSaveContext, sizeof(directLoad)),
            "a direct-load title must not rewrite native cutscene/title state");

    // The MM owner retains its own Alt choice. Both modes submit the logical
    // card path, allowing the real ResourceManager's native Alt fallback.
    for (bool alt : {false, true}) {
        Reset();
        Ship::CrossRMRegistry::managers["mm"]->alt = alt;
        Arrive(); Tick();
        Require(CardActive(), "native and Djipi-enabled MM owners must both supply the card");
        Require(!std::strcmp((const char*)play.actorCtx.titleCtx.texture,
                            "__OTR__@mm:custom/oot_location_titles/v1/lost_woods"),
                "the card path must allow the owning resource manager to select native or Djipi");
    }
    Reset();
    auto owner = Ship::CrossRMRegistry::managers["mm"];
    owner->alt = true;
    owner->archive->files.clear();
    owner->archive->files.insert(std::string("alt/") + kTexture);
    Arrive(); Tick();
    Require(CardActive(), "an installed Alt-only Lost Woods card is valid when MM Alt is enabled");
    for (bool alt : {false, true}) {
        Reset();
        owner = Ship::CrossRMRegistry::managers["mm"];
        owner->alt = alt;
        owner->archive->files.clear();
        owner->archive->files.insert((alt ? std::string("alt/") : std::string()) + kTexture + ".meta");
        Arrive(); Tick();
        Require(CardActive(), "native and Alt texture metadata aliases must be recognized as available cards");
    }
    Reset();
    Ship::CrossRMRegistry::managers.erase("mm");
    // An identically named foreign asset must never stand in for the MM owner.
    Ship::CrossRMRegistry::managers["oot"] = owner;
    Arrive(); Tick();
    Require(!CardActive(), "missing MM owner must safely skip rather than borrow foreign textures");
    Reset();
    Ship::CrossRMRegistry::managers["mm"]->archive->files.clear();
    Arrive(); Tick();
    Require(!CardActive(), "a missing native and Alt card must safely skip drawing");
    Reset();
    owner = Ship::CrossRMRegistry::managers["mm"];
    owner->archive->files.clear();
    owner->archive->files.insert(std::string("alt/") + kTexture);
    Arrive(); Tick();
    Require(!CardActive(), "an Alt-only card must not be submitted while MM Alt is disabled");

    for (int variant = 0; variant < 10; ++variant) {
        Reset();
        switch (variant) {
            case 0: gSaveContext.gameMode = GAMEMODE_TITLE_SCREEN; break;
            case 1: gSaveContext.fileNum = 0xFF; break;
            case 2: gSaveContext.sceneLayer = 1; break;
            case 3: gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 1); break;
            case 4: gSaveContext.save.cutsceneIndex = 0xFFF0; break;
            case 5: play.sceneId = SCENE_26SARUNOMORI; break;
            case 6: hideTitleCards = true; break;
            case 7: gComboGameMode = COMBO_GAME_MM; break;
            case 8: gSaveContext.cutsceneTrigger = 1; break;
            case 9: play.soaringCsOrSoTCsPlaying = true; break;
        }
        const SaveContext protectedSave = gSaveContext;
        Arrive();
        for (int i = 0; i < 160; ++i) Tick();
        Require(!CardActive(), "flashbacks, title demos, suppressed cards and native story control must stay protected");
        Require(!std::memcmp(&protectedSave, &gSaveContext, sizeof(protectedSave)),
                "excluded visits must preserve every save and story byte");
        gComboGameMode = COMBO_GAME_BOTH;
    }
    Reset();
    Arrive();
    play.transitionTrigger = TRANS_TRIGGER_START;
    Tick();
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    Tick();
    Require(!CardActive(), "leaving before arrival completes must cancel the pending card");
    Reset();
    Arrive();
    for (const auto& fn : GameInteractor::OnPlayDestroy::callbacks) fn();
    Tick();
    Require(!CardActive(), "destroying play must cancel pending title state");
    Reset();
    Arrive();
    play.actorCtx.titleCtx.durationTimer = 17;
    play.actorCtx.titleCtx.texture = (TexturePtr)"native-title";
    Tick();
    Require(play.actorCtx.titleCtx.durationTimer == 16 &&
            !std::strcmp((const char*)play.actorCtx.titleCtx.texture, "native-title"),
            "arrival must not replace an existing native title card");
    std::puts("PASS Lost Woods title: safe arrival, native fade/draw, once per visit, protected story state and donor fallback");
}
