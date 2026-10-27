// Native boot and title-card selection execute; disk I/O and hooks are boundaries.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include "global.h"
#include "soh/Enhancements/randomizer/randomizer_entrance.h"
#include "soh/Enhancements/game-interactor/vanilla-behavior/GIVanillaBehavior.h"
#include "rando/CrossEntranceBoss.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/RegionalPassages.h"
#include "ComboGameMode.h"
#if __has_include("gameplay/CrossGameTitleCard.h")
#include "gameplay/CrossGameTitleCard.h"
#endif
/* TEXTURE_HEADERS */

extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
SaveContext gSaveContext;
static int gComboReturnFileNum = -1;
static int sComboReturnPortalRoute = COMBO_PORTAL_MARKET;
static int crossArrival = -1, bossRecovery = -1, fwArrival = -1;
static int presentations;
static bool hideCards;
static bool isRando;
static int titleCardSetting = -1;
static bool fallbackPositionsGrotto;
static bool coveredRegionalArrival;
extern "C" int ComboRegionalPassagesOot_IsArrivalPending(PlayState*) {
    return coveredRegionalArrival;
}
extern "C" int32_t Combo_SOH_ConsumeCrossDungeonArrival(void) {
    const int result = crossArrival; crossArrival = -1; return result;
}
extern "C" int32_t Combo_SOH_ConsumeCrossBossRecoveryResume(void) {
    const int result = bossRecovery; bossRecovery = -1; return result;
}
extern "C" void ComboFw_ApplyArrival(void) {
    if (fwArrival >= 0) { gSaveContext.entranceIndex = fwArrival; gSaveContext.respawnFlag = 2; }
    fwArrival = -1;
}
extern "C" int ComboRegionalPassagesOot_ApplyArrival(int) { std::abort(); }
extern "C" void SaveContext_Init(void) { gSaveContext = {}; }
extern "C" void Sram_OpenSave(void) {
    gSaveContext.language = LANGUAGE_ENG;
    gSaveContext.linkAge = 1;
    gSaveContext.health = 0x50;
    gSaveContext.respawn[RESPAWN_MODE_TOP].pos = {111, 222, 333};
}
extern "C" void GameInteractor_ExecuteOnExitGame(int32_t) {}
extern "C" void GameInteractor_ExecuteOnLoadGame(int32_t) { gSaveContext.entranceIndex = ENTR_LINKS_HOUSE_CHILD_SPAWN; }
extern "C" int16_t Entrance_OverrideNextIndex(int16_t entrance) {
    if (fallbackPositionsGrotto) gSaveContext.respawnFlag = 2;
    return entrance;
}
extern "C" void Play_Init(GameState*) {}
extern "C" void Title_Init(GameState*) {}
#define IS_RANDO isRando
#define CVAR_ENHANCEMENT(name) name
extern "C" bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t value, ...) {
    bool result = value;
    bool* should = &result;
    switch (behavior) {
/* TITLE_CARD_GUARD */
        default: break;
    }
    return result && (behavior != VB_SHOW_TITLE_CARD || !hideCards);
}
extern "C" s32 Flags_GetEventChkInf(s32) { return true; }
#ifdef COMBO_BUILD
extern "C" int Combo_SOH_CrossBossLogicalClear(int, int nativeClear) { return nativeClear; }
#endif
extern "C" int32_t CVarGetInteger(const char* name, int32_t defaultValue) {
    return !std::strcmp(name, "TimeSavers.DisableTitleCard") && titleCardSetting >= 0 ? titleCardSetting : defaultValue;
}
extern "C" Color_RGB8 CVarGetColor24(const char*, Color_RGB8 defaultValue) { return defaultValue; }
extern "C" void GameInteractor_ExecuteOnPresentTitleCard(void) { ++presentations; }
#define CVAR_COSMETIC(name) name
#define LUSLOG_INFO(...) ((void)0)
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)

/* REQUEST_STATE */
/* ARRIVAL_BODY */
/* BOOT_BODY */
/* ENTRANCE_TABLE */
/* NATIVE_TITLE_CARD */
/* CUSTOM_TRANSITIONS */

static void SelectPlayerTitle(PlayState* play) {
    SceneTableEntry* scene = play->loadedScene;
    Player player{};
    Player* self = &player;
    s32 respawnFlag = gSaveContext.respawnFlag;
    u32 titleFileSize;
/* PLAYER_SELECTION */
}

static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static bool Active(const PlayState& play) { return play.actorCtx.titleCtx.durationTimer > 0; }
static void Boot(int route, int slot = 1) {
    GameState state{};
    gComboReturnFileNum = slot;
    sComboReturnPortalRoute = route;
    TitleSetup_InitImpl(&state);
}
static PlayState Destination(int scene) {
    static SceneTableEntry sceneEntry{};
    PlayState play{};
    play.loadedScene = &sceneEntry;
    play.sceneNum = scene;
    return play;
}

int main() {
#ifdef COMBO_BUILD
    // Regional boot first resolves the endpoint under black cover, then
    // reloads with the native positioned/grotto start. Only that reveal owns
    // a card; the cover hides world geometry, not the overlay title renderer.
    for (int scene : {SCENE_GRAVEYARD, SCENE_LAKE_HYLIA, SCENE_ZORAS_FOUNTAIN}) {
        Boot(COMBO_PORTAL_MARKET);
        gSaveContext.entranceIndex = scene == SCENE_GRAVEYARD ? ENTR_GRAVEYARD_ENTRANCE
                                     : scene == SCENE_LAKE_HYLIA ? ENTR_LAKE_HYLIA_OUTSIDE_LAB
                                                               : ENTR_ZORAS_FOUNTAIN_OUTSIDE_ICE_CAVERN;
        coveredRegionalArrival = true;
        auto firstLoad = Destination(scene);
        SelectPlayerTitle(&firstLoad);
        require(!Active(firstLoad), "covered regional boot must not show the first of two destination cards");
        coveredRegionalArrival = false;
        gSaveContext.respawnFlag = 2;
        Combo_SOH_RequestCrossGameTitleCard();
        auto reveal = Destination(scene);
        SelectPlayerTitle(&reveal);
        require(Active(reveal), "the final positioned regional reveal must retain its native destination card");
        const int previous = presentations;
        for (int frame = 0; frame < 120; ++frame) TitleCard_Update(&reveal, &reveal.actorCtx.titleCtx);
        require(presentations == previous + 1, "regional return must present exactly one native place card");
        reveal = Destination(scene);
        SelectPlayerTitle(&reveal);
        require(!Active(reveal), "the regional title request must not replay on a subsequent positioned spawn");
    }
    // Both forest bridges and the newer Woodfall return preserve positioned spawns.
    for (int route : {COMBO_PORTAL_LOST_WOODS, COMBO_PORTAL_OPENING_FOREST, COMBO_PORTAL_WOODFALL}) {
        for (int slot = 0; slot < 3; ++slot) {
            Boot(route, slot);
            require(gSaveContext.respawnFlag == 2, "forest return must keep its positioned spawn");
            const SaveContext before = gSaveContext;
            auto play = Destination(SCENE_LOST_WOODS);
            SelectPlayerTitle(&play);
            require(Active(play), "cross-game positioned return must display the Lost Woods card");
            require(!std::strcmp(static_cast<const char*>(play.actorCtx.titleCtx.texture), gLostWoodsTitleCardENGTex),
                    "forest arrival must select the native Lost Woods texture");
            require(play.actorCtx.titleCtx.x == 160 && play.actorCtx.titleCtx.y == 120 &&
                    play.actorCtx.titleCtx.width == 144 && play.actorCtx.titleCtx.height == 24 &&
                    play.actorCtx.titleCtx.delayTimer == 20, "native place-card layout and delay must survive");
            require(!std::memcmp(&before, &gSaveContext, sizeof(before)), "title request must preserve save and respawn data");
            require(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222 && gSaveContext.health == 0x50,
                    "paired-save Farore point and health must survive");
            const int previous = presentations;
            for (int frame = 0; frame < 120; ++frame) TitleCard_Update(&play, &play.actorCtx.titleCtx);
            require(presentations == previous + 1 && play.actorCtx.titleCtx.alpha == 0 && !Active(play),
                    "native card must present once and fade away");
            play = Destination(SCENE_LOST_WOODS);
            SelectPlayerTitle(&play);
            require(!Active(play), "a later ordinary positioned respawn must not replay the crossing card");
        }
    }
    // A composed shuffle/boss arrival must retain its destination, not the stale forest route.
    for (bool boss : {false, true}) for (int respawn : {0, 2}) {
        isRando = true;
        (boss ? bossRecovery : crossArrival) = ENTR_DEKU_TREE_ENTRANCE;
        Boot(COMBO_PORTAL_LOST_WOODS);
        gSaveContext.respawnFlag = respawn;
        auto play = Destination(SCENE_DEKU_TREE);
        const auto field = gEntranceTable[ENTR_DEKU_TREE_ENTRANCE].field;
        gEntranceTable[ENTR_DEKU_TREE_ENTRANCE].field &= ~ENTRANCE_INFO_DISPLAY_TITLE_CARD_FLAG;
        SelectPlayerTitle(&play);
        gEntranceTable[ENTR_DEKU_TREE_ENTRANCE].field = field;
        require(Active(play) && !std::strcmp(static_cast<const char*>(play.actorCtx.titleCtx.texture), gDekuTreeTitleCardENGTex),
                "shuffled cross-game destination must show its own card even without an entrance card flag");
    }
    for (int setting : {0, 1}) {
        titleCardSetting = setting;
        Boot(COMBO_PORTAL_LOST_WOODS);
        auto play = Destination(SCENE_LOST_WOODS);
        SelectPlayerTitle(&play);
        require(Active(play) == (setting == 0), "crossing must respect an explicit title-card setting in rando");
    }
    titleCardSetting = -1;
    // Same-game custom returns, including the fountain, use the same request
    // without booting TitleSetup or modifying their positioned respawn.
    for (bool rando : {false, true}) {
        isRando = rando;
        gSaveContext.respawnFlag = 2;
        gSaveContext.showTitleCard = false;
        gSaveContext.entranceIndex = ENTR_LOST_WOODS_SOUTH_EXIT;
        Combo_SOH_RequestCrossGameTitleCard();
        const SaveContext before = gSaveContext;
        auto localReturn = Destination(SCENE_LOST_WOODS);
        SelectPlayerTitle(&localReturn);
        require(Active(localReturn) && !std::strcmp(static_cast<const char*>(localReturn.actorCtx.titleCtx.texture),
                                                  gLostWoodsTitleCardENGTex),
                "a custom positioned return must select the native destination card, including shuffled seeds");
        require(!std::memcmp(&before, &gSaveContext, sizeof(before)),
                "a custom title request must not alter its positioned return or player progress");
        localReturn = Destination(SCENE_LOST_WOODS);
        SelectPlayerTitle(&localReturn);
        require(!Active(localReturn), "a custom title request must be consumed after one destination");
    }
    isRando = true;
    for (bool market : {false, true}) {
        gSaveContext.respawnFlag = 2;
        gSaveContext.showTitleCard = false;
        auto localReturn = Destination(SCENE_LOST_WOODS);
        if (market) Transition(&localReturn, ENTR_LOST_WOODS_SOUTH_EXIT);
        else ForestTransition(&localReturn, ENTR_LOST_WOODS_SOUTH_EXIT);
        SelectPlayerTitle(&localReturn);
        require(Active(localReturn), "explicit same-game shop and forest returns must request the native destination card");
        localReturn = Destination(SCENE_LOST_WOODS);
        SelectPlayerTitle(&localReturn);
        require(!Active(localReturn), "same-game shop and forest return requests must be consumed once");
    }
    gSaveContext.respawnFlag = 0;
    auto shuffledLocal = Destination(SCENE_LOST_WOODS);
    SelectPlayerTitle(&shuffledLocal);
    require(!Active(shuffledLocal), "randomizer default must still suppress ordinary same-game cards");
    isRando = false;
    fallbackPositionsGrotto = true;
    Boot(COMBO_PORTAL_MARKET);
    auto play = Destination(SCENE_MARKET_DAY);
    SelectPlayerTitle(&play);
    require(Active(play), "non-forest cross-game returns must include positioned/grotto compositions");
    fallbackPositionsGrotto = false;
    fwArrival = ENTR_LOST_WOODS_SOUTH_EXIT;
    Boot(COMBO_PORTAL_MARKET);
    play = Destination(SCENE_LOST_WOODS);
    gSaveContext.language = LANGUAGE_FRA;
    SelectPlayerTitle(&play);
    require(Active(play) && std::strstr(static_cast<const char*>(play.actorCtx.titleCtx.texture), "FRATex"),
            "cross-game recall must retain the native destination and language selection");

    // Native user/cutscene/no-art guards still consume the request without leaking to a later spawn.
    for (int guard = 0; guard < 3; ++guard) {
        Boot(COMBO_PORTAL_LOST_WOODS);
        play = Destination(guard == 2 ? SCENE_GROTTOS : SCENE_LOST_WOODS);
        hideCards = guard == 0;
        gSaveContext.sceneLayer = guard == 1 ? 4 : 0;
        SelectPlayerTitle(&play);
        require(!Active(play), "hidden, cutscene-layer and untitled destinations must not display a card");
        hideCards = false;
        gSaveContext.sceneLayer = 0;
        play = Destination(SCENE_LOST_WOODS);
        SelectPlayerTitle(&play);
        require(!Active(play), "a suppressed/untitled request must not leak to a later positioned respawn");
    }
    Boot(COMBO_PORTAL_LOST_WOODS);
    gComboReturnFileNum = -1;
    GameState state{};
    TitleSetup_InitImpl(&state);
    gSaveContext.respawnFlag = 2;
    play = Destination(SCENE_LOST_WOODS);
    SelectPlayerTitle(&play);
    require(!Active(play), "ordinary title boot must clear an abandoned crossing request");
#endif
    // Vanilla card behavior remains intact in both solo SoH and Combo.
    gSaveContext = {};
    gSaveContext.language = LANGUAGE_ENG;
    gSaveContext.showTitleCard = true;
    gSaveContext.entranceIndex = ENTR_LOST_WOODS_SOUTH_EXIT;
    auto normal = Destination(SCENE_LOST_WOODS);
    SelectPlayerTitle(&normal);
    require(Active(normal), "ordinary native entrance must still display its card");
    gSaveContext.respawnFlag = 2;
    normal = Destination(SCENE_LOST_WOODS);
    SelectPlayerTitle(&normal);
    require(!Active(normal), "ordinary same-game positioned respawn must retain native suppression");
    std::puts("PASS: OoT cross-game title cards, native destination/language/lifecycle, one-use guards and solo parity");
}
