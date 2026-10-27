#pragma once
// TU glue: include in each AudioEditor after native globals/collection APIs.
// Every entry point here runs on the graph thread, never the audio worker.
#include "SceneMusic.h"
#include "../ComboGameMode.h"
#include "../entrances/SoloForest.h"
#include "../entrances/BlackMarketEntrance.h"

#ifdef COMBO_SCENE_MUSIC_MM
extern "C" void Audio_InvalidateSceneSequence(void);
#else
extern "C" void Audio_CancelPreviewRestore(void);
extern "C" void Audio_PreviewSceneSequence(uint16_t sequence);
#endif
static uint16_t sPreviousSceneMusicRow = 0;

static uint16_t SceneMusicRowForPlay(const PlayState* play) {
    if (!play || gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.fileNum < 0 || gSaveContext.fileNum >= 3)
        return 0;
#ifdef COMBO_SCENE_MUSIC_MM
    if (ComboOpeningForest_IsExploration(play) ||
        ComboSoloForest_IsMmVisit(play->sceneId, 0, gSaveContext.gameMode, gSaveContext.fileNum,
                                  gSaveContext.sceneLayer, gSaveContext.save.cutsceneIndex))
        return ComboSceneMusic::kForest;
    if (gComboGameMode != COMBO_GAME_OOT && play->sceneId == COMBO_BLACK_MARKET_SCENE && gSaveContext.sceneLayer == 0 &&
        gSaveContext.save.cutsceneIndex == 0)
        return ComboSceneMusic::kMarket;
#else
    if (ComboSoloForest_IsOotVisit(play->sceneNum, 0, gSaveContext.gameMode, gSaveContext.fileNum,
                                   gSaveContext.sceneLayer, gSaveContext.cutsceneIndex))
        return ComboSceneMusic::kForest;
    if (gComboGameMode == COMBO_GAME_OOT && play->sceneNum == SCENE_SKIEJER_MARKET && gSaveContext.sceneLayer == 0 &&
        gSaveContext.cutsceneIndex == 0)
        return ComboSceneMusic::kMarket;
#endif
    return 0;
}

extern "C" uint16_t AudioEditor_GetSceneMusicRow(uint16_t request) {
    const auto row = SceneMusicRowForPlay(gPlayState);
    if (!row)
        return 0;
    const auto base = AudioCollection::Instance->GetAllSequences().at(row).sequenceId;
    return ComboSceneMusic::Matches(request, base) ? row : 0;
}

extern "C" uint16_t AudioEditor_ResolveSceneSequence(uint16_t request, uint16_t resolved) {
#ifdef COMBO_SCENE_MUSIC_MM
    // Our preview retains a native bookkeeping ID while playing the exact
    // slot selection. It must also work outside the target scene.
    if (sComboSceneMusicPreviewRow) {
        const auto base = AudioCollection::Instance->GetAllSequences().at(sComboSceneMusicPreviewRow).sequenceId;
        if (ComboSceneMusic::Matches(request, base))
            return AudioCollection::Instance->GetReplacementSequence(sComboSceneMusicPreviewRow);
    }
#endif
    if (CVarGetInteger("gAudioEditor.Playing", 0) != 0)
        return resolved;
    const auto row = AudioEditor_GetSceneMusicRow(request);
    return row ? AudioCollection::Instance->GetReplacementSequence(row) : resolved;
}

extern "C" void AudioEditor_ConfigureSceneMusic(PlayState* play) {
#ifdef COMBO_SCENE_MUSIC_MM
    const bool hadPreview = sComboSceneMusicPreviewRow != 0;
    if (sComboSceneMusicPreviewRow)
        AudioPreview::CancelForScene();
#else
    const bool hadPreview = CVarGetInteger("gAudioEditor.Playing", 0) != 0;
    if (hadPreview) {
        Audio_CancelPreviewRestore();
        CVarSetInteger("gAudioEditor.Playing", 0);
    }
#endif
    const auto row = SceneMusicRowForPlay(play);
    const auto previous = sPreviousSceneMusicRow;
    sPreviousSceneMusicRow = row;
    if (!play || gSaveContext.gameMode != GAMEMODE_NORMAL || gSaveContext.fileNum < 0 || gSaveContext.fileNum >= 3 ||
        (!row && !previous && !hadPreview))
        return;
#ifdef COMBO_SCENE_MUSIC_MM
    auto& sceneSequence = play->sceneSequences.seqId;
    if (row == ComboSceneMusic::kMarket)
        play->sceneSequences.ambienceId = AMBIENCE_ID_13;
#else
    auto& sceneSequence = play->sequenceCtx.seqId;
    if (row == ComboSceneMusic::kMarket)
        play->sequenceCtx.natureAmbienceId = NATURE_ID_NONE;
#endif
    if (row)
        sceneSequence = AudioCollection::Instance->GetAllSequences().at(row).sequenceId;
    else if (!hadPreview && sceneSequence != AudioCollection::Instance->GetAllSequences().at(previous).sequenceId)
        return;
    // Let the native environment enqueue after all scene commands and shuffle.
    // Invalidate both caches at row boundaries, including a return to a native
    // scene sharing the same tune. Forced music, Final Hours and time rules stay native.
    gSaveContext.seqId = NA_BGM_DISABLED;
#ifdef COMBO_SCENE_MUSIC_MM
    Audio_InvalidateSceneSequence();
#endif
}
