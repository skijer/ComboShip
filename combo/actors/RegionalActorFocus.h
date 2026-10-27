#pragma once

/* Native one-based draw callback limbs. Positions come from the current posed
 * matrix, never a standing-height estimate. Shared by OoT and manual MM actors. */
enum { COMBO_FOCUS_STALCHILD = 1, COMBO_FOCUS_MIKAU, COMBO_FOCUS_DARMANI };
static inline int ComboRegionalFocus_Limb(unsigned family) {
    return family == COMBO_FOCUS_STALCHILD ? 11
           : family == COMBO_FOCUS_MIKAU   ? 9
           : family == COMBO_FOCUS_DARMANI ? 4
                                           : 0;
}
static inline float ComboRegionalFocus_OffsetY(unsigned family) {
    /* Matches native EnZog_PostLimbDrawOpa's head focus offset. */
    return family == COMBO_FOCUS_MIKAU ? 1000.0f : 0.0f;
}
