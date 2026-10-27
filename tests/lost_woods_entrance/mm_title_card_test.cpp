#include "global.h"
#include "regs.h"
#include "z_title_setup.h"
#include "overlays/gamestates/ovl_title/z_title.h"
#include "2s2h/GameInteractor/GameInteractor_VanillaBehavior.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Graphics/OotLocationTitleCard.h"
#include "ComboGameMode.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/RegionalPassages.h"
#include "entrances/MmRegionalBridgePlacements.h"
#include "gameplay/CrossGameTitleCard.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

SaveContext gSaveContext;
PlayState* gPlayState;
GameState* gGameState;
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
static RegEditor editor;
RegEditor* gRegEditor = &editor;
static bool hideCards;
static int presentations, entranceFlags = -1, entranceCs = CS_ID_NONE, scriptIndex = CS_SCRIPT_ID_NONE, cutscenes;
static u16 lastTitle, hud;
static int nativeTitleBegins;
static PlayState* decodedTitlePlay;
static void Require(bool condition, const char* message);
extern "C" bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t value, ...) {
    return value && (behavior != VB_SHOW_TITLE_CARD || !hideCards);
}
extern "C" s16 CutsceneManager_FindEntranceCsId(void) { return entranceCs; }
extern "C" s16 CutsceneManager_GetCutsceneScriptIndex(s16) { return scriptIndex; }
extern "C" s16 CutsceneManager_Start(s16 csId, Actor*) { ++cutscenes; return csId; }
extern "C" s16 CutsceneManager_StartWithPlayerCs(s16 csId, Actor*) { ++cutscenes; return csId; }
extern "C" void Message_OpenText(PlayState* play, u16 textId) {
    decodedTitlePlay = nullptr;
    ++presentations; lastTitle = textId; play->msgCtx.currentTextId = textId;
}
extern "C" void func_80150A84(PlayState*) {}
extern "C" void Message_DecodeNES(PlayState* play) { decodedTitlePlay = play; }
extern "C" void Interface_SetHudVisibility(u16 visibility) { hud = visibility; }

// Rendering is exercised by tests/location_titles. This fixture observes the
// handoff after native decoding and scene-title setup, in both MM build modes.
void OotLocationTitle_Begin(PlayState* play) {
    Require(play && decodedTitlePlay == play && play->msgCtx.currentTextId == lastTitle &&
            play->msgCtx.msgMode == MSGMODE_SCENE_TITLE_CARD_FADE_IN_BACKGROUND &&
            play->msgCtx.stateTimer == 30 && play->msgCtx.textDelayTimer == 0 &&
            play->msgCtx.textColorAlpha == 0 && play->msgCtx.textboxColorAlphaCurrent == 0 &&
            play->msgCtx.textboxColorAlphaTarget == 0,
            "optional presenter must receive the fully decoded native title and fade setup");
    decodedTitlePlay = nullptr;
    ++nativeTitleBegins;
}

/* SCENE_TABLE */
/* ENTRANCE_TABLE */
extern "C" s32 Entrance_GetTransitionFlags(u16 entrance) {
    if (entranceFlags >= 0) return entranceFlags;
    switch (entrance >> 9) {
        case ENTR_SCENE_WOODFALL: return sWoodfallEntrance0[0].flags;
        case ENTR_SCENE_WOODS_OF_MYSTERY: return sWoodsOfMysteryEntrance0[0].flags;
        default: return sSouthClockTownEntrance0[0].flags;
    }
}
/* NATIVE_MESSAGE_TITLE */

#ifdef COMBO_BUILD
/* REGIONAL_ARRIVAL */
int gComboStartFileNum = -1, gComboEntryIsResume, gComboOwlBlobSlot = -1;
static int sComboPortalRoute;
static bool sComboPortalArrivalPending;
static int crossArrival = -1, bossRecovery = -1, savedRecovery = -1, fwArrival = -1;
extern "C" int32_t Combo_MM_ConsumeCrossDungeonArrival(void) {
    int value = crossArrival; crossArrival = -1; return value;
}
extern "C" int32_t Combo_MM_ConsumeCrossDungeonRecovery(void) {
    int value = savedRecovery; savedRecovery = -1; return value;
}
extern "C" int32_t Combo_MM_ConsumeCrossBossRecoveryResume(void) {
    int value = bossRecovery; bossRecovery = -1; return value;
}
extern "C" int ComboFw_ApplyArrival(void) {
    int value = fwArrival; fwArrival = -1;
    if (value < 0) return 0;
    gSaveContext.save.entrance = value; gSaveContext.respawnFlag = 3; return 1;
}
extern "C" s32 SysFlashrom_InitFlash(void) { return 0; }
extern "C" void SaveContext_Init(void) { gSaveContext = {}; }
extern "C" void Sram_LoadGlobalOptions(void) {}
void Setup_InitRegs(void) {}
extern "C" void Combo_AdoptOOTGlobalOptions(void) {}
extern "C" int Combo_LoadMMSaveFile(int slot) {
    gSaveContext.fileNum = slot - 1;
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    gSaveContext.save.day = 2;
    gSaveContext.save.time = 0x8000;
    gSaveContext.save.saveInfo.playerData.health = 0x50;
    gSaveContext.respawn[RESPAWN_MODE_TOP].pos = {111, 222, 333};
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = ENTRANCE(SOUTH_CLOCK_TOWN, 3);
    return 0;
}
extern "C" void Combo_RepairMMSaveForSlot(int) {}
extern "C" void Combo_ApplyOwlSaveOpen(int) {}
extern "C" void Combo_MMDropOwlSaveBlob(void) {}
extern "C" void GameInteractor_ExecuteOnSaveLoad(s16) {}
extern "C" s32 CVarGetInteger(const char*, s32) { return 0; }
extern "C" void Play_Init(GameState*) {}
void ConsoleLogo_Init(GameState*) {}
extern "C" void ComboBlackMarket_ApplyResume(int) {}
extern "C" void ComboForestArrivalCamera_Queue(int) {}
extern "C" void MagicMirror_QueueArrival(void) {}
/* REQUEST_STATE */
/* ARRIVAL_BODY */
/* BOOT_BODY */
/* CUSTOM_TRANSITIONS */
#endif
/* ENTRANCE_POLICY */

static void Require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static PlayState Destination(int scene) {
    PlayState play{};
    play.loadedScene = &gSceneTable[scene]; play.sceneId = scene;
    return play;
}
static void ClearGuards() {
    hideCards = false; entranceFlags = -1; entranceCs = CS_ID_NONE; scriptIndex = CS_SCRIPT_ID_NONE;
}
#ifdef COMBO_BUILD
static void Boot(int route, int slot = 1, bool resume = false) {
    SetupState state{};
    ClearGuards(); gComboStartFileNum = slot; gComboEntryIsResume = resume;
    sComboPortalRoute = route; sComboPortalArrivalPending = ComboPassages_IsRoute(route);
    Setup_InitImpl(&state);
}
#endif

int main() {
#ifdef COMBO_BUILD
    for (int route : {COMBO_PORTAL_IKANA, COMBO_PORTAL_GREAT_BAY, COMBO_PORTAL_SNOWHEAD}) {
        Boot(route);
        const int scene = route == COMBO_PORTAL_IKANA ? SCENE_IKANAMAE
                          : route == COMBO_PORTAL_GREAT_BAY ? SCENE_30GYOSON : SCENE_10YUKIYAMANOMURA;
        auto play = Destination(scene);
        const int old = presentations;
        Cutscene_HandleEntranceTriggers(&play);
        const bool titled = scene != SCENE_IKANAMAE;
        Require((gSceneTable[scene].titleTextId != 0) == titled,
                "Road to Ikana has no native title; Bay and Mountain Village do");
        Require(presentations == old + int(titled),
                "regional crossings must retain the destination's actual native title availability");
        gSaveContext.respawnFlag = 2;
        play = Destination(SCENE_30GYOSON);
        Cutscene_HandleEntranceTriggers(&play);
        Require(presentations == old + int(titled), "untitled Ikana must also consume its title request");
    }
    int allSceneCards = 0, allSceneUntitled = 0;
    for (int source = 0; source < 3; ++source) {
        for (int scene = 0; scene < SCENE_MAX; ++scene) {
            Boot(COMBO_PORTAL_WOODFALL, source);
            auto play = Destination(scene);
            // Entrance randomization may choose any final loaded scene after
            // an incoming portal/custom route. Its native table row owns the
            // title, independently of the incoming entrance's card bit.
            if (source == 1) Transition(&play, ENTRANCE(WOODFALL, 0));
            if (source == 2) ForestTransition(&play, ENTRANCE(WOODS_OF_MYSTERY, 0));
            entranceFlags = 0;
            gSaveContext.respawnFlag = 3;
            const auto before = gSaveContext;
            const int old = presentations;
            Cutscene_HandleEntranceTriggers(&play);
            const bool hasNativeCard = gSceneTable[scene].titleTextId != 0;
            Require(presentations == old + int(hasNativeCard),
                    "every final MM scene must follow its own native title-table availability");
            if (hasNativeCard) {
                ++allSceneCards;
                Require(lastTitle == gSceneTable[scene].titleTextId,
                        "shuffled/custom arrival must never reuse the incoming route's title");
            } else {
                ++allSceneUntitled;
            }
            Require(!std::memcmp(&before, &gSaveContext, sizeof(before)),
                    "all-scene title coverage must preserve every save/respawn/progression byte");
            play = Destination(SCENE_21MITURINMAE);
            Cutscene_HandleEntranceTriggers(&play);
            Require(presentations == old + int(hasNativeCard),
                    "every titled or untitled final arrival must consume its custom request once");
        }
    }
    std::printf("PASS all MM native scene rows across portal/shop/forest requests: %d titled, %d untitled\n",
                allSceneCards, allSceneUntitled);
    for (int route : {COMBO_PORTAL_WOODFALL, COMBO_PORTAL_LOST_WOODS, COMBO_PORTAL_MARKET}) {
        const int scene = route == COMBO_PORTAL_WOODFALL ? SCENE_21MITURINMAE
                          : route == COMBO_PORTAL_LOST_WOODS ? SCENE_26SARUNOMORI : SCENE_CLOCKTOWER;
        for (int slot = 0; slot < 3; ++slot) {
            Boot(route, slot);
            auto play = Destination(scene);
            const SaveContext before = gSaveContext;
            const int old = presentations;
            Cutscene_HandleEntranceTriggers(&play);
            Require(presentations == old + 1 && lastTitle == gSceneTable[scene].titleTextId,
                    "every direct crossing must display the native MM destination title");
            Require(play.msgCtx.msgMode == MSGMODE_SCENE_TITLE_CARD_FADE_IN_BACKGROUND &&
                    play.msgCtx.stateTimer == 30 && play.msgCtx.textColorAlpha == 0 &&
                    XREG(74) == 0x42 && XREG(75) == 0x1E && hud == HUD_VISIBILITY_NONE,
                    "crossing must retain the native MM scene-title layout and lifecycle setup");
            Require(!std::memcmp(&before, &gSaveContext, sizeof(before)),
                    "title handling must preserve the cycle, progression, entrance and positioned spawn");
            gSaveContext.respawnFlag = 2;
            Cutscene_HandleEntranceTriggers(&play);
            Require(presentations == old + 1, "the crossing request must be consumed before a later local respawn");
        }
    }
    for (bool market : {false, true}) {
        gSaveContext = {}; gSaveContext.respawnFlag = 2;
        gSaveContext.save.entrance = ENTRANCE(WOODFALL, 0);
        gSaveContext.save.day = 2; gSaveContext.save.time = 0x8000;
        gSaveContext.save.saveInfo.playerData.health = 0x50;
        entranceFlags = 0;
        auto play = Destination(SCENE_21MITURINMAE);
        if (market) Transition(&play, gSaveContext.save.entrance);
        else ForestTransition(&play, gSaveContext.save.entrance);
        const SaveContext before = gSaveContext;
        const int old = presentations;
        Cutscene_HandleEntranceTriggers(&play);
        Require(presentations == old + 1 && lastTitle == gSceneTable[SCENE_21MITURINMAE].titleTextId,
                "explicit same-game shop and forest returns must request the native destination title");
        Require(!std::memcmp(&before, &gSaveContext, sizeof(before)),
                "same-game title handling must preserve positioned returns, cycle and progress");
        Cutscene_HandleEntranceTriggers(&play);
        Require(presentations == old + 1, "custom same-game title requests must be consumed once");
    }
    for (int source = 0; source < 3; ++source) {
        if (source == 0) crossArrival = ENTRANCE(SNOWHEAD_TEMPLE, 0);
        if (source == 1) bossRecovery = ENTRANCE(SNOWHEAD_TEMPLE, 0);
        if (source == 2) fwArrival = ENTRANCE(SNOWHEAD_TEMPLE, 0);
        Boot(COMBO_PORTAL_WOODFALL, 0, source == 2);
        Require(gSaveContext.save.entrance == ENTRANCE(SNOWHEAD_TEMPLE, 0),
                "title requests must retain the final shuffled, boss or Farore destination");
        gSaveContext.respawnFlag = 3; entranceFlags = 0;
        auto play = Destination(SCENE_HAKUGIN);
        const SaveContext before = gSaveContext;
        const int old = presentations;
        Cutscene_HandleEntranceTriggers(&play);
        Require(presentations == old + 1 && lastTitle == gSceneTable[SCENE_HAKUGIN].titleTextId,
                "composed arrivals must show the final destination despite positioned spawns or unset card flags");
        Require(!std::memcmp(&before, &gSaveContext, sizeof(before)),
                "composed arrival cards must preserve every save and respawn byte");
    }
    for (int guard = 0; guard < 7; ++guard) {
        Boot(COMBO_PORTAL_WOODFALL);
        auto play = Destination(guard == 1 ? SCENE_KAKUSIANA : guard == 6 ? SCENE_LOST_WOODS : SCENE_21MITURINMAE);
        CutsceneScriptEntry script{};
        if (guard == 0) hideCards = true;
        if (guard == 2) { gSaveContext.sceneLayer = 1; entranceFlags = 0; }
        if (guard == 3) { gSaveContext.save.cutsceneIndex = 0xFFF0; entranceFlags = 0; }
        if (guard == 4 || guard == 5) {
            entranceCs = 7; scriptIndex = 0; play.csCtx.scriptList = &script;
            script.spawnFlags = guard == 4 ? CS_SPAWN_FLAG_ALWAYS : CS_SPAWN_FLAG_ONCE(WEEKEVENTREG_08_80);
        }
        if (guard == 6) gSaveContext.save.entrance = ENTRANCE(LOST_WOODS, 0);
        const int old = presentations, oldCs = cutscenes;
        Cutscene_HandleEntranceTriggers(&play);
        Require(presentations == old, "hidden, untitled, story-layer and cutscene-owned arrivals must stay native");
        if (guard == 4 || guard == 5) Require(cutscenes == oldCs + 1, "native entrance cutscene must keep control");
        ClearGuards(); gSaveContext.sceneLayer = 0; gSaveContext.save.cutsceneIndex = 0;
        gSaveContext.respawnFlag = 2;
        play = Destination(SCENE_21MITURINMAE);
        Cutscene_HandleEntranceTriggers(&play);
        Require(presentations == old, "suppressed and opening-forest requests must not leak to a later arrival");
    }
    Boot(COMBO_PORTAL_WOODFALL);
    Boot(COMBO_PORTAL_MARKET, 0, true);
    gSaveContext.respawnFlag = 2;
    auto ordinary = Destination(SCENE_CLOCKTOWER);
    const int old = presentations;
    Cutscene_HandleEntranceTriggers(&ordinary);
    Require(presentations == old, "ordinary save resume must clear any abandoned crossing request");
#endif
    ClearGuards();
    gSaveContext = {}; gSaveContext.showTitleCard = true;
    gSaveContext.save.entrance = ENTRANCE(WOODFALL, 0);
    auto native = Destination(SCENE_21MITURINMAE);
    const int beforeNative = presentations;
    entranceFlags = 0x4000;
    Cutscene_HandleEntranceTriggers(&native);
    Require(presentations == beforeNative + 1, "native same-game title behavior must remain intact");
    gSaveContext.respawnFlag = 2;
    Cutscene_HandleEntranceTriggers(&native);
    Require(presentations == beforeNative + 1, "ordinary positioned respawns must retain native suppression");
    gSaveContext.respawnFlag = 0; entranceFlags = 0;
    Cutscene_HandleEntranceTriggers(&native);
    Require(presentations == beforeNative + 1, "ordinary room entrances with no card bit must retain native suppression");
    Require(nativeTitleBegins == presentations,
            "every native title must notify the optional presenter exactly once");
    std::puts("PASS MM native destination titles: composed boot, message layout, cutscene/hide guards, consumption and solo parity");
}
