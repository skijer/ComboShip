#ifndef RANDO_SPIN_ATTACK_GI_H
#define RANDO_SPIN_ATTACK_GI_H

#include "2s2h/BenGui/CosmeticEditor.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "ComboItemDrawABI.h"

// Use the same burst palette as MM's real spin actor. Query on each draw so edits,
// rainbow colors and cosmetic suppression stay live, including foreign GIs in OoT.
static inline int32_t MM_FillSpinAttackGi(CwItemDrawInfo* out) {
    const Color_RGBA8 color = CosmeticEditor_GetChangedColor(255, 255, 170, 255, "Effects.GreatSpinBurst");
    out->drawKind = CW_DRAW_KIND_MM_SPIN_ATTACK;
    out->dlistCount = 2;
    out->dlists[0] = gGreatSpinAttackDiskDL;
    out->dlists[1] = gGreatSpinAttackCylinderDL;
    out->xluStartIndex = 0;
    out->scale = 0.012f;
    out->stateDependent = 2;
    out->primColorXlu[0] = color.r;
    out->primColorXlu[1] = color.g;
    out->primColorXlu[2] = color.b;
    out->primColorXlu[3] = 255;
    return 1;
}

#endif
