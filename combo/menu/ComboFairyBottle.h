#ifndef COMBO_FAIRY_BOTTLE_H
#define COMBO_FAIRY_BOTTLE_H

#include <math.h>
#include <stdint.h>
#include <string.h>
#include "ComboBottleShimmer.h"

typedef struct {
    const char* opaque;
    const char* glass;
} ComboFairyBottleShell;

static inline int ComboFairyBottle_IsBlueFireShell(const char* path) {
    return path != 0 && strstr(path, "objects/object_gi_fire/gGiBlueFireChamberstickDL") != 0;
}

static inline int ComboFairyBottle_IsBundledShell(const char* path) {
    return path != 0 && strstr(path, "objects/combo_bottle_gi/EmptyOpaque") != 0;
}

static inline ComboFairyBottleShell ComboFairyBottle_BundledShell(ComboFairyBottleShell shell, const char* opaque,
                                                                  const char* glass, int (*isSelectedMod)(const char*),
                                                                  int available) {
    if (available && isSelectedMod && !isSelectedMod(opaque) && !isSelectedMod(glass) &&
        strcmp(shell.opaque, opaque) == 0 && strcmp(shell.glass, glass) == 0) {
        shell.opaque = "__OTR__objects/combo_bottle_gi/EmptyOpaque";
        shell.glass = "__OTR__objects/combo_bottle_gi/EmptyXlu";
    }
    return shell;
}

// Strengthen only the bundled fairy's glass. Empty/general bottles and selected
// casing/material recipes remain complete, including those in older archives.
static inline ComboFairyBottleShell ComboFairyBottle_ReadableShell(ComboFairyBottleShell shell,
                                                                   int (*isSelectedMod)(const char*), int available) {
    if (available && isSelectedMod && strcmp(shell.opaque, "__OTR__objects/combo_bottle_gi/EmptyOpaque") == 0 &&
        strcmp(shell.glass, "__OTR__objects/combo_bottle_gi/EmptyXlu") == 0 && !isSelectedMod(shell.opaque) &&
        !isSelectedMod(shell.glass) && !isSelectedMod("__OTR__objects/combo_bottle_gi/BottleShell") &&
        !isSelectedMod("__OTR__objects/combo_bottle_gi/shell/mat_gGiBlueFireChamberstickDL_BottleGlassOutside")) {
        shell.glass = "__OTR__objects/combo_bottle_gi/FairyXlu";
    }
    return shell;
}

typedef struct {
    float x, y, z;
    float scaleX, scaleY, scaleZ;
} ComboFairyBottleMotion;

// Keep the native contents and their placement matrix. Only the contents move:
// <= .65 X, 1.15 Y, .35 Z GI units, with a smaller .74..82 fluttering silhouette.
// This guarantees the transform envelope, not containment in arbitrary mod art.
// A shared active-host clock avoids frozen motion when the asset owner is dormant.
static inline ComboFairyBottleMotion ComboFairyBottle_Sample(uint32_t frame) {
    const float phase = (float)(frame % 360u) * (6.2831853071795864769f / 180.0f);
    ComboFairyBottleMotion motion = { .65f * sinf(phase),
                                      1.15f * sinf(phase * 1.5f + .4f),
                                      .35f * cosf(phase),
                                      .78f + .04f * sinf(phase * 30.0f),
                                      .78f,
                                      .78f };
    return motion;
}

// The bundled casing's body spans Y=-30..6. Use its own center and a readable
// bounce; arbitrary selected mods retain the prior native placement envelope.
static inline ComboFairyBottleMotion ComboFairyBottle_SampleForShell(uint32_t frame, const char* shell) {
    ComboFairyBottleMotion motion = ComboFairyBottle_Sample(frame);
    if (ComboFairyBottle_IsBundledShell(shell)) {
        const float phase = (float)(frame % 360u) * (6.2831853071795864769f / 180.0f);
        motion.x = 4.f * sinf(phase);
        motion.y = -10.f + 7.f * sinf(phase * 1.5f + .4f);
        motion.z = 2.f * cosf(phase);
    }
    return motion;
}

#endif
