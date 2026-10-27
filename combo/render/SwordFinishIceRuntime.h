#pragma once
#include "SwordFinishIceMaterial.h"
#include "SwordFinishIceMaterial.hpp"
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>

namespace {
SwordFinishIce::Cache sSwordFinishIceCache;
void ClearSwordFinishIce() {
    sSwordFinishIceCache.Clear();
}
} // namespace

extern "C" Gfx* SwordFinishIce_GetDisplayList(uint32_t frame) {
    static const bool registered = [] {
        Ship::CrossRMRegistry::RegisterTeardownListener(ClearSwordFinishIce);
        return true;
    }();
    (void)registered;
    const auto owner = Ship::CrossRMRegistry::GetOrActive(SWORD_FINISH_ICE_HOST);
    if (!owner)
        return nullptr;
    const char* path = SWORD_FINISH_ICE_COMBINED ? "objects/gameplay_keep/gEffIceFragment2DL"
                                                 : "objects/gameplay_keep/gEffIceFragment2MaterialDL";
    try {
        Ship::OwnRMScope scope(SWORD_FINISH_ICE_HOST);
        const auto source = std::dynamic_pointer_cast<Fast::DisplayList>(owner->LoadResource(path));
        return sSwordFinishIceCache.Get(source, frame, SWORD_FINISH_ICE_COMBINED);
    } catch (const std::exception&) { return nullptr; }
}
