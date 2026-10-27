#pragma once

// Session mode is set before initialization, never inferred from a save or a
// persisted randomizer option. Each game DLL owns its own hidden copy.
typedef enum ComboGameMode {
    COMBO_GAME_BOTH = 0,
    COMBO_GAME_OOT = 1,
    COMBO_GAME_MM = 2,
} ComboGameMode;

#ifdef COMBO_BUILD
#ifdef __cplusplus
extern "C" {
#endif
extern int gComboGameMode;
#ifdef __cplusplus
}
#endif
static inline int Combo_IsCombinedGame(void) {
    return gComboGameMode == COMBO_GAME_BOTH;
}
#else
static inline int Combo_IsCombinedGame(void) {
    return 0;
}
#endif
