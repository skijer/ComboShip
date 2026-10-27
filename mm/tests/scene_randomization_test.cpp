#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
            std::exit(1);                                                     \
        }                                                                     \
    } while (0)

using s8 = int8_t;
using f32 = float;
constexpr int GAMEMODE_NORMAL = 0;
constexpr int GAMEMODE_TITLE_SCREEN = 1;
constexpr int GAMEMODE_END_CREDITS = 2;
struct {
    int gameMode = GAMEMODE_NORMAL;
} gSaveContext;

static std::map<std::string, int> cvars;
int CVarGetInteger(const char* key, int fallback) {
    auto found = cvars.find(key);
    return found == cvars.end() ? fallback : found->second;
}
#define CVAR_AUDIO(name) "gAudioEditor." name

constexpr int ACTOR_PLAYER = 0;
constexpr int PLAYER_FORM_HUMAN = 4;
constexpr int PLAYER_FORM_FIERCE_DEITY = 0;
struct Actor {
    int id;
    float scale;
};
struct Player {
    Actor actor;
    int transformation;
};
static float linkSizeMultiplier = 1.0f;
float CVarGetFloat(const char* key, float fallback) {
    return std::string(key) == "gCosmetic.Silly.LinkSize" ? linkSizeMultiplier : fallback;
}
void Actor_SetScale(Actor* actor, float scale) {
    actor->scale = scale;
}
static const char* kSillyLinkSizeCvar = "gCosmetic.Silly.LinkSize";

// Headless event boundary: retain the real callback signatures and execute the
// production callbacks. Rendering and platform initialization are not needed.
struct GameInteractor {
    struct OnSceneInit {
        using Callback = std::function<void(s8, s8)>;
    };
    struct OnRandoSeedGeneration {
        using Callback = std::function<void()>;
    };
    struct OnGameStateUpdate {
        using Callback = std::function<void()>;
    };
    struct OnActorUpdate {
        using Callback = std::function<void(Actor*)>;
    };
    template <class Hook> static auto& Callbacks() {
        static std::vector<typename Hook::Callback> callbacks;
        return callbacks;
    }
    template <class Hook> void RegisterGameHook(typename Hook::Callback callback) {
        Callbacks<Hook>().push_back(callback);
    }
    template <class Hook> void RegisterGameHookForID(int id, typename Hook::Callback callback) {
        Callbacks<Hook>().push_back([id, callback](Actor* actor) {
            if (actor->id == id) {
                callback(actor);
            }
        });
    }
    static GameInteractor* Instance;
};
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;

struct Color_RGBA8 {
    uint8_t r, g, b, a;
};
struct ImVec4 {
    float x, y, z, w;
};
struct CosmeticOption {
    const char* valuesCvar;
    Color_RGBA8 defaultColor;
    ImVec4 currentColor;
};
static std::map<std::string, CosmeticOption> cosmeticOptions;
Color_RGBA8 CVarGetColor(const char*, Color_RGBA8 color) {
    return color;
}
static bool modelOwnershipReady = false;
static int cosmeticRefreshes = 0;
void CosmeticEditorRefreshElement(const CosmeticOption&) {
    // Native fixed-index callbacks must see custom-model ownership on their
    // first invocation, before any replacement display list can be patched.
    CHECK(modelOwnershipReady);
    ++cosmeticRefreshes;
}
void CosmeticEditorSave() {
}
void RefreshDynamicCosmeticsStateIfNeeded() {
    modelOwnershipReady = true;
}
void ApplyDynamicCosmetics() {
}
void CosmeticEditorUpdateTick() {
}
static int cosmeticRandomizations = 0;
void CosmeticEditorRandomizeAllElements() {
    ++cosmeticRandomizations;
}
struct CosmeticEditorWindow {
    void InitElement();
};

static int audioRandomizations = 0;
static int destinationReplacement = 10;
static int queuedDestination = -1;
void AudioEditor_RandomizeAll() {
    ++audioRandomizations;
    ++destinationReplacement;
}
struct PlayState {};
void Interface_SetSceneRestrictions(PlayState*) {
}
void Environment_PlaySceneSequence(PlayState*) {
    queuedDestination = destinationReplacement;
}

#include "scene_randomization.inc"

static void ReloadScene() {
    PlayState play;
    InitializeSceneAudio(&play);
    for (const auto& callback : GameInteractor::Callbacks<GameInteractor::OnSceneInit>()) {
        callback(8, 0);
    }
}

int main() {
    cosmeticOptions.emplace("Player.GoronTunic", CosmeticOption{ "gCosmetic.Player.GoronTunic.Color", {}, {} });
    CosmeticEditorWindow window;
    window.InitElement();
    window.InitElement(); // GUI initialization must not duplicate the reload hook.
    CHECK(cosmeticRefreshes == 1);
    CHECK(GameInteractor::Callbacks<GameInteractor::OnActorUpdate>().size() == 1);
    Player player{ { ACTOR_PLAYER, 0.01f }, PLAYER_FORM_HUMAN };
    Actor other{ 1, 0.75f };
    linkSizeMultiplier = 2.0f;
    for (const auto& callback : GameInteractor::Callbacks<GameInteractor::OnActorUpdate>()) {
        callback(&player.actor);
        callback(&other);
    }
    CHECK(player.actor.scale == 0.02f && other.scale == 0.75f);
    player.transformation = PLAYER_FORM_FIERCE_DEITY;
    GameInteractor::Callbacks<GameInteractor::OnActorUpdate>()[0](&player.actor);
    CHECK(player.actor.scale == 0.03f);
    linkSizeMultiplier = 1.0f;
    GameInteractor::Callbacks<GameInteractor::OnActorUpdate>()[0](&player.actor);
    CHECK(player.actor.scale == 0.015f);

    ReloadScene();
    CHECK(cosmeticRandomizations == 0);
    CHECK(audioRandomizations == 0);
    CHECK(queuedDestination == 10);

    cvars["gCosmetics.RandomizeOnSceneLoad"] = 1;
    ReloadScene();
    CHECK(cosmeticRandomizations == 1);
    CHECK(audioRandomizations == 0);
    ReloadScene(); // The same scene/spawn must shuffle again.
    CHECK(cosmeticRandomizations == 2);

    cvars["gCosmetics.RandomizeOnSceneLoad"] = 0;
    cvars["gAudioEditor.RandomizeAllOnNewScene"] = 1;
    ReloadScene();
    CHECK(cosmeticRandomizations == 2);
    CHECK(audioRandomizations == 1);
    CHECK(queuedDestination == 11); // New selection applies to this load.
    ReloadScene();
    CHECK(audioRandomizations == 2);
    CHECK(queuedDestination == 12);

    cvars["gCosmetics.RandomizeOnSceneLoad"] = 1;
    ReloadScene();
    CHECK(cosmeticRandomizations == 3);
    CHECK(audioRandomizations == 3);
    CHECK(queuedDestination == 13);

    for (int mode : { GAMEMODE_TITLE_SCREEN, GAMEMODE_END_CREDITS }) {
        gSaveContext.gameMode = mode;
        ReloadScene();
        CHECK(cosmeticRandomizations == 3);
        CHECK(audioRandomizations == 3);
    }
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    cvars["gCosmetics.RandomizeOnSceneLoad"] = 0;
    cvars["gAudioEditor.RandomizeAllOnNewScene"] = 0;
    ReloadScene();
    CHECK(cosmeticRandomizations == 3);
    CHECK(audioRandomizations == 3);

    // Preserve independent seed-generation randomization.
    cvars["gCosmetics.RandomizeOnSeedGen"] = 1;
    for (const auto& callback : GameInteractor::Callbacks<GameInteractor::OnRandoSeedGeneration>()) {
        callback();
    }
    CHECK(cosmeticRandomizations == 4);
    std::puts("PASS MM scene randomization: opt-in, independent toggles, repeat loads, destination audio ordering");
}
