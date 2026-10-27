#pragma once

#include "z64.h"
#include "soh/cvar_prefixes.h"
#include <libultraship/bridge/consolevariablebridge.h>

// Presentation and matching water physics only. Never substitute a child scene
// setup or write progression flags to make the adult Domain appear thawed.
static inline int ZorasDomain_IsAlwaysThawed(const PlayState* play) {
    return play != NULL && play->sceneNum == SCENE_ZORAS_DOMAIN &&
           CVarGetInteger(CVAR_ENHANCEMENT("AlwaysThawedZorasDomain"), 0) != 0;
}
