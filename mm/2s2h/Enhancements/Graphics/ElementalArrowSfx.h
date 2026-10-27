#ifndef MM_ELEMENTAL_ARROW_SFX_H
#define MM_ELEMENTAL_ARROW_SFX_H

#include "sfx.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include <libultraship/bridge/consolevariablebridge.h>

// Opt in at the arrow's existing impact call, without remapping shared sounds.
static inline u16 ElementalArrow_GetImpactSfx(u16 nativeSfx) {
    if (!CVarGetInteger(CVAR_COSMETIC("Arrows.ElementalImpactSounds"), 0)) {
        return nativeSfx;
    }
    switch (nativeSfx) {
        case NA_SE_IT_EXPLOSION_FRAME:
            return NA_SE_EV_FLAME_IGNITION;
        case NA_SE_IT_EXPLOSION_ICE:
            return NA_SE_EV_ICE_BROKEN;
        case NA_SE_IT_EXPLOSION_LIGHT:
            return NA_SE_EN_LIGHT_ARROW_HIT;
        default:
            return nativeSfx;
    }
}

#endif
