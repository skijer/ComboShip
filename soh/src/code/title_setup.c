#include "global.h"
#include "gameplay/ComboFaroresWind.h"
#include "gameplay/CrossGameTitleCard.h"

#ifdef COMBO_BUILD
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/randomizer/randomizer_entrance.h" // Entrance_OverrideNextIndex
// Set by SOH_ResumeGame (OTRGlobals.cpp) on a combo MM->OOT return. >= 0 means: skip the title /
// file-select screens, load this save slot, and spawn straight into Play in the Market outside the
// Happy Mask Shop (the fixed arrival point for the cross-game portal return).
extern s32 gComboReturnFileNum;
extern int32_t Combo_SOH_ConsumeCrossDungeonArrival(void);
extern int32_t Combo_SOH_ConsumeCrossBossRecoveryResume(void);
void SOH_ApplyComboPortalArrival(int allowPlacement);
static s32 sComboCrossGameTitleCardPending = false;
static s32 sComboCustomTitleCardPresentation = false;

int Combo_SOH_SwapCustomTitleCardPresentation(int active) {
    const s32 previous = sComboCustomTitleCardPresentation;
    sComboCustomTitleCardPresentation = active != 0;
    return previous;
}

int Combo_SOH_IsPresentingCustomTitleCard(void) {
    return sComboCustomTitleCardPresentation;
}

int Combo_SOH_HasCrossGameTitleCard(void) {
    return sComboCrossGameTitleCardPending;
}

int Combo_SOH_ConsumeCrossGameTitleCard(void) {
    const s32 pending = sComboCrossGameTitleCardPending;
    sComboCrossGameTitleCardPending = false;
    return pending;
}

void Combo_SOH_RequestCrossGameTitleCard(void) {
    gSaveContext.showTitleCard = true;
    sComboCrossGameTitleCardPending = true;
}
#endif

void TitleSetup_InitImpl(GameState* gameState) {
    osSyncPrintf("ゼルダ共通データ初期化\n"); // "Zelda common data initalization"
    SaveContext_Init();
#ifdef COMBO_BUILD
    sComboCrossGameTitleCardPending = false;
    sComboCustomTitleCardPresentation = false;
    if (gComboReturnFileNum >= 0) {
        // Combo MM->OOT return: mirror FileChoose_LoadGame (z_file_choose.c) to load the OOT save and
        // jump directly to Play, then override the entrance so Link spawns in the Market outside the
        // Happy Mask Shop (fixed arrival for the cross-game portal return — symmetric with the OOT->MM
        // trigger of entering the shop).
        LUSLOG_INFO("[ComboShip] TitleSetup combo jump -> Play (fileNum=%d)", gComboReturnFileNum);
        gSaveContext.fileNum = gComboReturnFileNum;
        gSaveContext.gameMode = GAMEMODE_NORMAL;
        Sram_OpenSave();
        // Mirror FileChoose_LoadGame's magic staging so Play_Init refills the meter normally.
        gSaveContext.magicFillTarget = gSaveContext.magic;
        gSaveContext.magic = 0;
        gSaveContext.magicLevel = 0;
        // Fire the exit/load hook pair a real quit+load runs: Save_LoadFile (inside Sram_OpenSave)
        // recreated gRandoContext, and consumers like the check tracker must tear down + re-init.
        GameInteractor_ExecuteOnExitGame(gSaveContext.fileNum);
        GameInteractor_ExecuteOnLoadGame(gSaveContext.fileNum);
        // Must follow OnLoadGame: the rando handler's Entrance_SetSavewarpEntrance() recomputes from
        // savedSceneNum (never set by the portal handoff) and would clobber this with Link's House.
        s32 crossDungeonArrival = Combo_SOH_ConsumeCrossDungeonArrival();
        s32 crossBossRecovery = Combo_SOH_ConsumeCrossBossRecoveryResume();
        // Native fallback composition can mutate grotto respawn state. Only run it when
        // no already-composed arrival owns the destination and its spawn data.
        gSaveContext.entranceIndex = crossDungeonArrival >= 0 ? crossDungeonArrival
                                     : crossBossRecovery >= 0
                                         ? crossBossRecovery
                                         : Entrance_OverrideNextIndex(ENTR_MARKET_DAY_OUTSIDE_HAPPY_MASK_SHOP);
        SOH_ApplyComboPortalArrival(crossDungeonArrival < 0 && crossBossRecovery < 0);
        // A portal return is never a cutscene arrival: a never-played MM-start save still holds the
        // pending intro (0xFFF1), which would shift Play_Init's entrance lookup onto a garbage layer.
        gSaveContext.cutsceneIndex = 0;
        ComboFw_ApplyArrival(); // Queued Farore recall overrides the fixed portal arrival.
        // Cross-game arrivals bypass file select and may use positioned/grotto
        // respawns. Request the final destination's native place card once.
        gSaveContext.showTitleCard = true;
        sComboCrossGameTitleCardPending = true;
        gComboReturnFileNum = -1;
        gameState->running = false;
        SET_NEXT_GAMESTATE(gameState, Play_Init, PlayState);
        return;
    }
#endif
    gameState->running = false;
    SET_NEXT_GAMESTATE(gameState, Title_Init, TitleContext);
}

void TitleSetup_Destroy(GameState* gameState) {
}

void TitleSetup_Init(GameState* gameState) {
    gameState->destroy = TitleSetup_Destroy;
    TitleSetup_InitImpl(gameState);
}
