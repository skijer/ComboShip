// Resolve availability with the same owner and Alt policy as MM's renderer.
// ExtensionCache only indexes files with extensions; native packed resources
// can have none, so probing that cache would incorrectly skip native cards.
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#ifdef COMBO_BUILD
#include <ship/resource/CrossRMRegistry.h>
#endif
#include <string>

extern "C" int OotLocationTitle_HasTexture(const char* otrPath) {
#ifdef COMBO_BUILD
    const auto owner = Ship::CrossRMRegistry::Get("mm");
#else
    const auto owner = Ship::Context::GetRawInstance()->GetResourceManager();
#endif
    const auto archives = owner ? owner->GetArchiveManager() : nullptr;
    if (!archives)
        return false;
    const std::string path = std::string(otrPath).substr(7); // owned __OTR__ path
    const auto available = [&](const std::string& name) {
        return archives->HasFile(name) || archives->HasFile(name + ".meta");
    };
    return available(path) || (owner->IsAltAssetsEnabled() && available("alt/" + path));
}
