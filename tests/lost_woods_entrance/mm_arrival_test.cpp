#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "global.h"
#include "z_title_setup.h"
#include "overlays/gamestates/ovl_title/z_title.h"
#include "2s2h/BenPort.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/RegionalPassages.h"
#include "gameplay/CrossGameTitleCard.h"

SaveContext gSaveContext;
int gComboStartFileNum = 0;
int gComboEntryIsResume = 0;
int gComboOwlBlobSlot = -1;
extern "C" void ComboBlackMarket_ApplyResume(int) {} // separately tested market resume boundary
static int sComboPortalRoute = COMBO_PORTAL_MARKET;
static bool sComboPortalArrivalPending = false;
static s32 sComboCrossGameTitleCardPending;
static bool sComboReturnPending, sComboFwReturnPending, sComboResetReturnPending, sComboOwlSaveQuitPending;
static int sComboReturnPortalRoute, sComboPassageFade, sComboPassageFileNum;
static bool sComboOpeningReturnArmed;
extern "C" void ComboWoodfallPassage_Reset(void) {} // separately covered by Woodfall geometry tests
PlayState* gPlayState;
GameState* gGameState;
static bool sRememberLocation;
static int hookCount = 0;
static int queuedCameraRoute = -1, queuedCameraCount = 0;
static int mirrorArrivalCount = 0;
extern "C" void MagicMirror_QueueArrival(void) { ++mirrorArrivalCount; }
extern "C" void ComboForestArrivalCamera_Queue(int route) { queuedCameraRoute = route; ++queuedCameraCount; }
static int crossArrival = -1, bossRecovery = -1, savedRecovery = -1, fwArrival = -1;
extern "C" int32_t Combo_MM_ConsumeCrossDungeonArrival(void) {
    if (crossArrival >= 0) gSaveContext.respawnFlag = 0;
    int value = crossArrival; crossArrival = -1; return value;
}
extern "C" int32_t Combo_MM_ConsumeCrossBossRecoveryResume(void) {
    if (bossRecovery >= 0) gSaveContext.respawnFlag = 0;
    int value = bossRecovery; bossRecovery = -1; return value;
}
extern "C" int32_t Combo_MM_ConsumeCrossDungeonRecovery(void) {
    int value = savedRecovery; savedRecovery = -1; return value;
}
extern "C" int ComboFw_ApplyArrival(void) {
    const bool applied = fwArrival >= 0;
    if (fwArrival >= 0) { gSaveContext.save.entrance = fwArrival; gSaveContext.respawnFlag = 0; }
    fwArrival = -1;
    return applied;
}
static void require(bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

extern "C" s32 SysFlashrom_InitFlash(void) { return 0; }
extern "C" void SaveContext_Init(void) { std::memset(&gSaveContext, 0, sizeof(gSaveContext)); }
extern "C" void Sram_LoadGlobalOptions(void) {}
void Setup_InitRegs(void) {}
extern "C" void Combo_AdoptOOTGlobalOptions(void) {}
extern "C" int Combo_LoadMMSaveFile(int slot) {
    gSaveContext.fileNum = slot - 1; // SaveManager takes filename slots 1..3; native slots are 0..2.
    gSaveContext.save.day = 2;
    gSaveContext.save.time = 0x8000;
    gSaveContext.save.saveInfo.playerData.health = 0x50;
    gSaveContext.respawn[RESPAWN_MODE_TOP].pos = { 111, 222, 333 };
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance = ENTRANCE(SOUTH_CLOCK_TOWN, 3);
    return 0;
}
extern "C" void Combo_RepairMMSaveForSlot(int) {}
extern "C" void Combo_ApplyOwlSaveOpen(int) {}
extern "C" void Combo_MMDropOwlSaveBlob(void) {}
extern "C" void GameInteractor_ExecuteOnSaveLoad(s16) { ++hookCount; }
extern "C" s32 CVarGetInteger(const char*, s32) { return sRememberLocation; }
extern "C" void Play_Init(GameState*) {}
void ConsoleLogo_Init(GameState*) {}
#define GameInteractor_Should(...) false

static int regionalArrival;
extern "C" int ComboRegionalPassagesMm_ApplyArrival(int route) { regionalArrival=route; return 1; }
extern "C" void ComboRegionalPassagesMm_Reset(void) {}

/* ARRIVAL_BODY */
/* ROUTE_BODY */
/* BOOT_BODY */

int main() {
    SetupState state{};
    gComboStartFileNum = 1;
    MM_SetComboEntryIsResume(0);
    MM_SetComboPortalRoute(COMBO_PORTAL_LOST_WOODS);
    Setup_InitImpl(&state);
    require(gSaveContext.save.entrance == ENTRANCE(WOODS_OF_MYSTERY, 0),
            "Lost Woods crossing must arrive in Woods of Mystery, not Clock Town");
    require(gSaveContext.respawnFlag == 2 && gSaveContext.respawn[RESPAWN_MODE_RETURN].roomIndex == 1,
            "arrival must use the first clearing and the native return respawn");
    require(queuedCameraRoute == COMBO_PORTAL_LOST_WOODS && queuedCameraCount == 1,
            "the real portal arrival must queue its camera exactly once after loading the destination");
    require(gSaveContext.respawn[RESPAWN_MODE_RETURN].pos.x == 0 &&
            gSaveContext.respawn[RESPAWN_MODE_RETURN].pos.z == 0,
            "arrival must face out of the side passage, outside its return trigger");
    require(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222,
            "portal arrival must preserve Farore's Wind's TOP respawn slot");
    require(gSaveContext.save.day == 2 && gSaveContext.save.time == 0x8000 &&
            gSaveContext.save.saveInfo.playerData.health == 0x50,
            "crossing must preserve the loaded cycle, time and health");
    require(!sComboPortalArrivalPending && hookCount == 1 && gComboStartFileNum == -1,
            "arrival must consume its request and run the save-load hook once");

    const int oldCameraCount = queuedCameraCount;
    gComboStartFileNum = 1;
    MM_SetComboEntryIsResume(0);
    MM_SetComboPortalRoute(COMBO_PORTAL_MAGIC_MIRROR);
    Setup_InitImpl(&state);
    require(gSaveContext.save.entrance == ENTRANCE(LOST_WOODS, 0) && gSaveContext.respawnFlag == 0 &&
            mirrorArrivalCount == 1 && queuedCameraCount == oldCameraCount,
            "Mirror must queue its stump spawn after save loading and leave the tunnel camera unqueued");
    MM_ApplyComboPortalArrival();
    require(mirrorArrivalCount == 1, "a cross-game Mirror request must be consumed once");

    gComboStartFileNum = 0;
    MM_SetComboEntryIsResume(0);
    Setup_InitImpl(&state);
    require(gSaveContext.save.entrance == ENTRANCE(SOUTH_CLOCK_TOWN, 0) && gSaveContext.respawnFlag == 0,
            "the existing Mask Shop portal must retain its native Clock Town arrival");
    require(gSaveContext.fileNum == 0 && ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum),
            "the first combo slot must be accepted after native MM file-number conversion");

    MM_SetComboPortalRoute(COMBO_PORTAL_LOST_WOODS);
    MM_SetComboEntryIsResume(1);
    sRememberLocation = true;
    gComboStartFileNum = 2;
    Setup_InitImpl(&state);
    require(gSaveContext.save.entrance == ENTRANCE(SOUTH_CLOCK_TOWN, 3) && gSaveContext.respawnFlag == 0 &&
            !sComboPortalArrivalPending,
            "a later real save resume must discard the old crossing and honor Remember Save Location");
    for (bool recoverBoss : {false, true}) {
        MM_SetComboEntryIsResume(0);
        MM_SetComboPortalRoute(COMBO_PORTAL_LOST_WOODS);
        (recoverBoss ? bossRecovery : crossArrival) = ENTRANCE(WOODFALL_TEMPLE, 0);
        gComboStartFileNum = 0;
        Setup_InitImpl(&state);
        require(gSaveContext.save.entrance == ENTRANCE(WOODFALL_TEMPLE, 0) &&
                gSaveContext.respawnFlag == 0 && !sComboPortalArrivalPending,
                "a composed dungeon/boss arrival must consume and supersede a stale forest route");
    }
    MM_SetComboEntryIsResume(0);
    MM_SetComboPortalRoute(COMBO_PORTAL_LOST_WOODS);
    fwArrival = ENTRANCE(SOUTH_CLOCK_TOWN, 3);
    gComboStartFileNum = 0;
    Setup_InitImpl(&state);
    require(gSaveContext.save.entrance == ENTRANCE(SOUTH_CLOCK_TOWN, 3) && !sComboPortalArrivalPending,
            "an explicit Farore recall must take precedence and consume the forest route");
    std::puts("PASS: native MM boot routes Woods and Market crossings; cycle, save hooks and Farore state preserved");
}
