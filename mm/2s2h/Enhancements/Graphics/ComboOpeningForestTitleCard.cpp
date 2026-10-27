// Reuse the built-in native Lost Woods place card for ordinary ComboShip
// visits to MM's opening forest. MM selects optional HD/Alt or native art.
#ifdef COMBO_BUILD
#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <string>

extern "C" int ComboPlaceTitleCard_Show(PlayState* play, TexturePtr texture, u8 width, u8 height) {
    if (!play || gGameState != &play->state || !play->state.running || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || play->pauseCtx.state != PAUSE_STATE_OFF ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE || play->csCtx.state != CS_STATE_IDLE ||
        gSaveContext.cutsceneTrigger || play->soaringCsOrSoTCsPlaying || Player_InCsMode(play))
        return 0;
    if (!GameInteractor_Should(VB_SHOW_TITLE_CARD, true))
        return -1;
    TitleCardContext* card = &play->actorCtx.titleCtx;
    if (card->durationTimer || card->delayTimer || card->alpha)
        return 0;
    TitleCard_InitBossName(&play->state, card, texture, 160, 120, width, height);
    card->delayTimer = 20;
    card->alpha = card->intensity = 0;
    return 1;
}

namespace {
constexpr char kLostWoodsCard[] = "custom/oot_location_titles/v1/lost_woods";
// Raw texture commands reserve odd addresses for segmented-pointer tags.
alignas(2) constexpr char kRoutedLostWoodsCard[] = "__OTR__@mm:custom/oot_location_titles/v1/lost_woods";
bool sPendingOpeningTitle = false;

bool HasLostWoodsCard() {
    // Check the same owner that resolves the routed draw path.
    const auto owner = Ship::CrossRMRegistry::Get("mm");
    const auto archives = owner ? owner->GetArchiveManager() : nullptr;
    if (!archives)
        return false;
    const auto available = [&](const std::string& path) {
        return archives->HasFile(path) || archives->HasFile(path + ".meta");
    };
    return available(kLostWoodsCard) ||
           (owner->IsAltAssetsEnabled() && available(std::string("alt/") + kLostWoodsCard));
}

void QueueOpeningTitle(s8 sceneId, s8 spawnNum) {
    sPendingOpeningTitle = sceneId == SCENE_LOST_WOODS && spawnNum == 0 && ComboOpeningForest_IsExploration(gPlayState);
}

void PresentOpeningTitle() {
    if (!sPendingOpeningTitle)
        return;
    PlayState* play = gPlayState;
    if (!play || gGameState != &play->state || !play->state.running || !ComboOpeningForest_IsExploration(play) ||
        play->transitionTrigger == TRANS_TRIGGER_START) {
        sPendingOpeningTitle = false;
        return;
    }
    // Wait for the entry fade and player start mode. Flashback/layer/story
    // eligibility is checked again above; no history flag is read or written.
    if (play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        play->pauseCtx.state != PAUSE_STATE_OFF || play->gameOverCtx.state != GAMEOVER_INACTIVE ||
        play->csCtx.state != CS_STATE_IDLE || gSaveContext.cutsceneTrigger || play->soaringCsOrSoTCsPlaying ||
        Player_InCsMode(play))
        return;

    TitleCardContext* card = &play->actorCtx.titleCtx;
    // This custom arrival owns its eligibility. Combo bypasses file select,
    // and the forest's prologue bypass skips the native transient flag reset.
    // Consult the user's hide-card hook with an eligible default, without
    // rewriting showTitleCard or any story/cycle state.
    if (card->durationTimer || card->delayTimer || card->alpha || !HasLostWoodsCard()) {
        sPendingOpeningTitle = false;
        return;
    }

    // MM retains the native IA8 title-card renderer and fade. These are OoT's
    // place-card dimensions, position and delay, rather than boss-card layout.
    if (ComboPlaceTitleCard_Show(play, const_cast<char*>(kRoutedLostWoodsCard), 144, 24) != 0)
        sPendingOpeningTitle = false;
}

void RegisterOpeningTitleCard() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>(QueueOpeningTitle);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>(PresentOpeningTitle);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] { sPendingOpeningTitle = false; });
}
static RegisterShipInitFunc initFunc(RegisterOpeningTitleCard, {});
} // namespace
#endif
