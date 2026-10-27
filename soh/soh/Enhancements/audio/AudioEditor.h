#pragma once

#ifdef __cplusplus

#include <libultraship/libultra/types.h>
#include <ship/window/gui/GuiWindow.h>

#include "AudioCollection.h"

class AudioEditor final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void DrawElement() override;
    void InitElement() override;
    void UpdateElement() override{};
    ~AudioEditor(){};
};

void AudioEditor_RandomizeAll();
void AudioEditor_AutoRandomizeAll();
void AudioEditor_RandomizeGroup(SeqType group);
void AudioEditor_ResetAll();
void AudioEditor_ResetGroup(SeqType group);
void AudioEditor_LockAll();
void AudioEditor_UnlockAll();

extern "C" {
#endif

u16 AudioEditor_GetReplacementSeq(u16 seqId);
#ifdef COMBO_BUILD
struct PlayState;
u16 AudioEditor_GetSceneMusicRow(u16 seqId);
u16 AudioEditor_ResolveSceneSequence(u16 original, u16 resolved);
void AudioEditor_ConfigureSceneMusic(struct PlayState* play);
#endif

#ifdef __cplusplus
}
#endif
