#pragma once
#include "ComboGameMode.h"

// The native fairy and player-talk entry points pass their own scene enum.
// No actor flags, companion option, save value or normal-scene behavior changes.
static inline int ComboBlackMarket_FairyQuiet(int host, int scene, int marketScene) {
#ifdef COMBO_BUILD
    return scene == marketScene && (gComboGameMode == COMBO_GAME_BOTH || gComboGameMode == host);
#else
    return 0;
#endif
}
