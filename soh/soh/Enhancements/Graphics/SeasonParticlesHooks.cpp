#include "SeasonParticles.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"

extern "C" PlayState* gPlayState;

static void RegisterSeasonParticles() {
    COND_HOOK(OnPlayDrawEnd, true, []() { SeasonParticles_Draw(gPlayState); });
    COND_HOOK(OnSceneInit, true, [](int16_t) { SeasonParticles_Reset(); });
    COND_HOOK(OnPlayDestroy, true, []() { SeasonParticles_Reset(); });
    COND_HOOK(OnLoadGame, true, [](int32_t) { SeasonParticles_Reset(); });
    COND_HOOK(OnExitGame, true, [](int32_t) { SeasonParticles_Reset(); });
}

static RegisterShipInitFunc initSeasonParticles(RegisterSeasonParticles);
