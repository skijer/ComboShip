#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "global.h"
#include "gameplay/CrossGameTitleCard.h"
#include "soh/Enhancements/randomizer/randomizer_entrance.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/RegionalPassages.h"

SaveContext gSaveContext;
static int gComboReturnFileNum = 1;
static int sComboReturnPortalRoute = COMBO_PORTAL_LOST_WOODS;
static s32 sComboCrossGameTitleCardPending;
static s32 sComboCustomTitleCardPresentation;
static int loadHooks, exitHooks;
static int crossArrival = -1, bossRecovery = -1, fwArrival = -1;
static int fallbackLookups;
static bool fallbackMutatesRespawn;
extern "C" int32_t Combo_SOH_ConsumeCrossDungeonArrival(void) {
    int value = crossArrival; crossArrival = -1; return value;
}
extern "C" int32_t Combo_SOH_ConsumeCrossBossRecoveryResume(void) {
    int value = bossRecovery; bossRecovery = -1; return value;
}
extern "C" void ComboFw_ApplyArrival(void) {
    if (fwArrival >= 0) { gSaveContext.entranceIndex = fwArrival; gSaveContext.respawnFlag = 0; }
    fwArrival = -1;
}
#define LUSLOG_INFO(...) ((void)0)
static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
extern "C" void SaveContext_Init(void) { std::memset(&gSaveContext, 0, sizeof(gSaveContext)); }
extern "C" void Sram_OpenSave(void) {
    gSaveContext.linkAge = 1;
    gSaveContext.health = 0x50;
    gSaveContext.respawn[RESPAWN_MODE_TOP].pos = { 111, 222, 333 };
}
extern "C" void GameInteractor_ExecuteOnExitGame(int32_t) { ++exitHooks; }
extern "C" void GameInteractor_ExecuteOnLoadGame(int32_t) {
    ++loadHooks;
    gSaveContext.entranceIndex = ENTR_LINKS_HOUSE_CHILD_SPAWN;
}
extern "C" int16_t Entrance_OverrideNextIndex(int16_t entrance) {
    ++fallbackLookups;
    if (fallbackMutatesRespawn) {
        // Native entrance composition can materialize a grotto's positioned return.
        gSaveContext.respawnFlag = 2;
        gSaveContext.respawn[RESPAWN_MODE_RETURN].pos = { 765, 432, 123 };
    }
    return entrance;
}
extern "C" void Play_Init(GameState*) {}
extern "C" void Title_Init(GameState*) {}
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)

static int regionalArrival;
extern "C" int ComboRegionalPassagesOot_ApplyArrival(int route) { regionalArrival=route; return 1; }

/* ARRIVAL_BODY */
/* BOOT_BODY */

int main() {
    GameState state{};
    sComboCustomTitleCardPresentation = true;
    TitleSetup_InitImpl(&state);
    require(!sComboCustomTitleCardPresentation,
            "native boot must clear a previous transient custom-title presentation scope");
    require(gSaveContext.entranceIndex == ENTR_LOST_WOODS_SOUTH_EXIT,
            "the return crossing must reach OoT Lost Woods, not the Market or Link's House");
    require(gSaveContext.respawnFlag == 2 && gSaveContext.respawn[RESPAWN_MODE_RETURN].roomIndex == 0 &&
            gSaveContext.respawn[RESPAWN_MODE_RETURN].pos.z == -280 &&
            gSaveContext.respawn[RESPAWN_MODE_RETURN].yaw == 0,
            "return must face out of the same straight-ahead tunnel, outside the trigger");
    require(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222 &&
            gSaveContext.linkAge == 1 && gSaveContext.health == 0x50,
            "return must preserve Farore state, age and health from the paired OoT save");
    require(gComboReturnFileNum == -1 && sComboReturnPortalRoute == COMBO_PORTAL_MARKET &&
            loadHooks == 1 && exitHooks == 1,
            "return must consume its route after both native save hooks");
    gComboReturnFileNum = 1;
    TitleSetup_InitImpl(&state);
    require(gSaveContext.entranceIndex == ENTR_MARKET_DAY_OUTSIDE_HAPPY_MASK_SHOP && gSaveContext.respawnFlag == 0,
            "a following ordinary portal return must still reach the Mask Shop");
    require(fallbackLookups == 2, "ordinary forest and Market returns must retain native fallback composition");
    for (bool forestRoute : {false, true}) for (bool recoverBoss : {false, true}) {
        sComboReturnPortalRoute = forestRoute ? COMBO_PORTAL_LOST_WOODS : COMBO_PORTAL_MARKET;
        (recoverBoss ? bossRecovery : crossArrival) = ENTR_DEKU_TREE_ENTRANCE;
        gComboReturnFileNum = 1;
        const int before = fallbackLookups;
        fallbackMutatesRespawn = true;
        TitleSetup_InitImpl(&state);
        require(gSaveContext.entranceIndex == ENTR_DEKU_TREE_ENTRANCE &&
                gSaveContext.respawnFlag == 0 && sComboReturnPortalRoute == COMBO_PORTAL_MARKET,
                "a composed dungeon/boss arrival must consume and supersede a stale forest route");
        require(fallbackLookups == before,
                "queued dungeon/boss arrivals must not compose the ordinary Market fallback");
    }
    fallbackMutatesRespawn = false;
    sComboReturnPortalRoute = COMBO_PORTAL_LOST_WOODS;
    fwArrival = ENTR_LINKS_HOUSE_CHILD_SPAWN;
    gComboReturnFileNum = 1;
    TitleSetup_InitImpl(&state);
    require(gSaveContext.entranceIndex == ENTR_LINKS_HOUSE_CHILD_SPAWN &&
            sComboReturnPortalRoute == COMBO_PORTAL_MARKET,
            "an explicit Farore recall must take precedence and consume the forest route");
    std::puts("PASS: native OoT boot returns to the same tunnel; original portal and paired save state preserved");
}
