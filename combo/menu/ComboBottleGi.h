#ifndef COMBO_BOTTLE_GI_H
#define COMBO_BOTTLE_GI_H

#include <stdint.h>
#include <string.h>

// Include after the host GBI types. XML CallDisplayList is compiled by the
// shared loader into G_DL_OTR_FILEPATH with a resource-owned stable string.
// Inspect the winning wrappers, so another pack (including a partial override)
// keeps its existing draw recipe instead of being forced into our bottle pass.
static inline int ComboBottleGi_Calls(const Gfx* commands, const char* target) {
    if (commands == NULL || ((commands[0].words.w0 >> 24) & 0xFF) != G_DL_OTR_FILEPATH || commands[0].words.w1 == 0) {
        return 0;
    }
    return strcmp((const char*)(uintptr_t)commands[0].words.w1, target) == 0;
}

static inline int ComboBottleGi_HasPotionRecipe(const char* pot, const char* liquid, const char* glass,
                                                Gfx* (*load)(const char*)) {
    return pot != NULL && liquid != NULL && glass != NULL && load != NULL &&
           ComboBottleGi_Calls(load(pot), "objects/combo_bottle_gi/PotionMarker") &&
           ComboBottleGi_Calls(load(liquid), "objects/combo_bottle_gi/Liquid") &&
           ComboBottleGi_Calls(load(glass), "objects/combo_bottle_gi/BottleShell");
}

#endif
