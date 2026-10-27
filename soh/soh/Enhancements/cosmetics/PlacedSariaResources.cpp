#ifdef COMBO_BUILD
#include "PlacedSariaResources.h"
#include "ComboExport.h"
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>

extern "C" COMBO_EXPORT int OOT_PlacedSariaAltEnabled(void) {
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    return owner && owner->IsAltAssetsEnabled();
}

extern "C" COMBO_EXPORT int OOT_PlacedSariaLoadV2(unsigned pose, ComboPlacedSariaResourcesV2* output,
                                                  size_t callerSize) {
    if (!output || callerSize < sizeof(*output))
        return 0;
    *output = {};
    output->drawScale = 1.f;
    const auto owner = Ship::CrossRMRegistry::Get("oot");
    if (!owner)
        return 0;
    try {
        Ship::OwnRMScope scope("oot");
        auto snapshot = PlacedSaria::Build([&](const std::string& path) { return owner->LoadResource(path, true); },
                                           owner->IsAltAssetsEnabled(), pose);
        if (!snapshot)
            return 0;
        output->resources = snapshot->result;
        output->drawScale = snapshot->drawScale;
        output->resources.owner = snapshot.release();
        return 1;
    } catch (const std::exception&) { return 0; }
}

extern "C" COMBO_EXPORT int OOT_PlacedSariaLoad(unsigned pose, ComboPlacedSariaResources* output) {
    if (!output)
        return 0;
    ComboPlacedSariaResourcesV2 current{};
    const int loaded = OOT_PlacedSariaLoadV2(pose, &current, sizeof(current));
    *output = current.resources; // Exactly the frozen V1 type, including its padding.
    return loaded;
}

extern "C" COMBO_EXPORT void OOT_PlacedSariaRelease(void* owner) {
    delete static_cast<PlacedSaria::Snapshot*>(owner);
}
#endif
