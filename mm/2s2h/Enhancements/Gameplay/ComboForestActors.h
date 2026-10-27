#pragma once

#ifdef COMBO_BUILD
#include "ComboOpeningForest.h"
#include "entrances/SoloForest.h"

static inline int ComboForestActors_IsMmGame(void) {
    return gComboGameMode == COMBO_GAME_BOTH || gComboGameMode == COMBO_GAME_MM;
}

// Keep native prologue/flashback guards separate from the solo exploration copy.
static inline int ComboForestActors_IsExploration(const PlayState* play) {
    return ComboOpeningForest_IsExploration(play) ||
           (play &&
            ComboSoloForest_IsMmVisit(play->sceneId, play->roomCtx.curRoom.num, gSaveContext.gameMode,
                                      gSaveContext.fileNum, gSaveContext.sceneLayer, gSaveContext.save.cutsceneIndex) &&
            gSaveContext.save.entrance == COMBO_SOLO_FOREST_MM_ENTRANCE);
}
#endif
