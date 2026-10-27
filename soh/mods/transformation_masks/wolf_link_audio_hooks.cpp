#include "wolf_link_form.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"

extern "C" PlayState* gPlayState;

// Keep a Human transform tail alive and responsive to pause/volume after the
// Wolf actor hands control back. Session changes discard all outstanding cues.
static void RegisterWolfLinkAudioHooks() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(
        []() { WolfLinkForm_UpdateSfx(gPlayState); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(
        [](int32_t) { WolfLinkForm_UpdateSfx(nullptr); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>(
        [](int32_t) { WolfLinkForm_UpdateSfx(nullptr); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnZTitleInit>(
        [](void*) { WolfLinkForm_UpdateSfx(nullptr); });
}

static RegisterShipInitFunc initWolfLinkAudio(RegisterWolfLinkAudioHooks, {});
