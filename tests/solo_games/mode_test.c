#include <assert.h>
#include <stdio.h>
#if __has_include("combo/ComboGameMode.h")
#include "combo/ComboGameMode.h"
#else
#define COMBO_GAME_BOTH 0
#define COMBO_GAME_OOT 1
#define COMBO_GAME_MM 2
static int gComboGameMode;
static int Combo_IsCombinedGame(void) { return 1; }
#endif

#ifdef COMBO_BUILD
#ifndef COMBO_GAME_MODE_MISSING
int gComboGameMode = COMBO_GAME_BOTH;
#endif
#endif

int main(void) {
#ifdef COMBO_BUILD
    gComboGameMode = COMBO_GAME_BOTH;
    assert(Combo_IsCombinedGame());
    gComboGameMode = COMBO_GAME_OOT;
    assert(!Combo_IsCombinedGame() && "OoT-only must use native progression");
    gComboGameMode = COMBO_GAME_MM;
    assert(!Combo_IsCombinedGame() && "MM-only must use native progression");
#else
    assert(!Combo_IsCombinedGame() && "a native standalone build is not combined");
#endif
    puts("game-mode C ABI: PASS");
}
