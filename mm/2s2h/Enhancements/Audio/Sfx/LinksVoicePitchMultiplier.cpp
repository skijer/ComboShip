#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "variables.h"
#include "mods/forms/custom_forms.h"
s32 AdultLink_IsActive(void); // mods/items/logic/adult_link_render.cpp — adult Link gets a deeper voice
}

// For Link's voice pitch SFX modifier
static f32 freqMultiplier = 1;

#define CVAR_NAME "gAudioEditor.LinkVoiceFreqMultiplier.Enable"
#define CVAR CVarGetInteger(CVAR_NAME, 0)

void RegisterLinksVoicePitchMultiplier() {
    // Always register — the adult-Link gate is a runtime save flag, not this CVar, so we decide per call
    // (the hook is a cheap no-op unless the audio-editor slider or adult mode is active).
    COND_VB_SHOULD(VB_LINK_VOICE_PITCH_MULTIPLIER, true, {
        Player* player = GET_PLAYER(gPlayState);
        u16 sfxId = *va_arg(args, u16*);

        u8 isVoice = (sfxId >= NA_SE_VO_LI_SWORD_N && sfxId <= NA_SE_VO_DEMO_394) || sfxId == NA_SE_PL_TRANSFORM_VOICE;
        u8 adult = AdultLink_IsActive() != 0;
        u8 editor = CVAR != 0;
        u16 formVoice = 0;

        if (isVoice && CustomForms_VoiceOverride(sfxId, &formVoice)) {
            // A custom form speaks with its own bank (Keaton = Deku, Garo = Igos) or stays silent.
            *should = false;
            if (formVoice != 0) {
                AudioSfx_PlaySfx(formVoice, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            }
        } else if (isVoice && (adult || editor)) {
            // Adult Link plays a deeper voice, baked at 0.85. The SFX-id range already excludes footsteps
            // and the Deku/Goron/Zora voice banks, so this only pitches base Link.
            freqMultiplier = adult ? 0.85f : CVarGetFloat("gAudioEditor.LinkVoiceFreqMultiplier.Scale", 1.0f);
            if (freqMultiplier <= 0) {
                freqMultiplier = 1;
            }

            *should = false;
            AudioSfx_PlaySfx(sfxId, &player->actor.projectedPos, 4, &freqMultiplier, &gSfxDefaultFreqAndVolScale,
                             &gSfxDefaultReverb);
        }
    });
}

static RegisterShipInitFunc initFunc(RegisterLinksVoicePitchMultiplier, { CVAR_NAME });
