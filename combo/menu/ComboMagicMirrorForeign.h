#pragma once
#include "ComboItemDrawABI.h"
#include "gameplay/MagicMirrorItem.h"
// Matrix belongs to the receiving native GI caller. Never replay a donor scale.
static inline bool ComboDrawMagicMirrorForeign(PlayState* play, int32_t drawKind) {
    if (drawKind != CW_DRAW_KIND_MAGIC_MIRROR)
        return false;
    MagicMirror_DrawGI(play);
    return true;
}
