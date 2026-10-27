#ifndef AUDIO_STREAMED_POSITIONAL_H
#define AUDIO_STREAMED_POSITIONAL_H

#include "z64audio.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern u8 sSequenceFilter[8 * 4];

// Only the native point-source music buffer identifies these effects. Authored
// sequence filters and native instrument samples must retain their own processing.
static inline void Audio_ApplyStreamedPositionalPolicy(const NoteSampleState* source, u8* gain, s16** filter) {
    const s16* positionalFilter = (s16*)(((uintptr_t)sSequenceFilter & ~(uintptr_t)0xF) + 0x10);
    if (*filter != positionalFilter || source->bitField1.isSyntheticWave || source->tunedSample == NULL ||
        source->tunedSample->sample == NULL || !CVarGetInteger("gEnhancements.Fixes.MMStreamedPositionalAudio", 0)) {
        return;
    }
    const u8 codec = source->tunedSample->sample->codec;
    if (codec == CODEC_S16 || codec == CODEC_OPUS) {
        *filter = NULL;
        if (*gain == 0x7F) {
            *gain = 0;
        }
    }
}

#endif
