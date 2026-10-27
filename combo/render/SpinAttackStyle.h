#ifndef COMBO_SPIN_ATTACK_STYLE_H
#define COMBO_SPIN_ATTACK_STYLE_H

#include <stdint.h>

#define CW_SPIN_STYLE_CVAR "gEnhancements.Graphics.SpinStyle"
#define CW_SPIN_ELEMENT_EFFECTS_CVAR "gEnhancements.Graphics.ElementalSpinEffects"

typedef enum {
    CW_SPIN_STYLE_NATIVE,
    CW_SPIN_STYLE_FIRE,
    CW_SPIN_STYLE_ICE,
    CW_SPIN_STYLE_LIGHT,
    CW_SPIN_STYLE_LIGHTNING,
    CW_SPIN_STYLE_ARCANE,
    CW_SPIN_STYLE_ASTRAL,
    CW_SPIN_STYLE_DARK_ENERGY,
    CW_SPIN_STYLE_COUNT
} CwSpinStyle;

typedef struct {
    CwSpinStyle style;
    uint8_t nativeMaterialElement;
    uint8_t accentElement;
} CwSpinVisualSelection;

// Element IDs are the existing sword selector's None/Fire/Ice/Light (0..3).
// Presentation never grants an element or changes combat eligibility.
static inline CwSpinVisualSelection CwSpinStyle_Resolve(int requestedStyle, int effectsEnabled, uint8_t swordElement,
                                                        int eligiblePoweredSpin) {
    CwSpinVisualSelection result = { CW_SPIN_STYLE_NATIVE, 0, 0 };
    if (!eligiblePoweredSpin)
        return result;
    if (requestedStyle >= 0 && requestedStyle < CW_SPIN_STYLE_COUNT)
        result.style = (CwSpinStyle)requestedStyle;
    if (effectsEnabled && swordElement > 0 && swordElement <= 3) {
        result.accentElement = swordElement;
        if (result.style == CW_SPIN_STYLE_NATIVE)
            result.nativeMaterialElement = swordElement;
    }
    return result;
}

#endif
