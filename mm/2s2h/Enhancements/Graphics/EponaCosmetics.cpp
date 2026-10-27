#include "EponaCosmetics.h"
#include "EponaCosmeticMasks.h"
#include "EponaCosmeticsDL.h"
#include "EponaCosmeticsNativeTemplates.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <fast/resource/type/DisplayList.h>
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/utils/StrHash64.h>
#ifdef COMBO_BUILD
#include <ship/resource/CrossRMRegistry.h>
#endif
#include <array>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include "global.h"
}

namespace {
constexpr const char* kTexturePrefix = "objects/object_horse_link_child/";
constexpr const char* kChanged[] = { "gCosmetic.Epona.Coat.Changed", "gCosmetic.Epona.WhiteHair.Changed",
                                     "gCosmetic.Epona.Eyes.Changed" };
constexpr const char* kColors[] = { "gCosmetic.Epona.Coat.Color", "gCosmetic.Epona.WhiteHair.Color",
                                    "gCosmetic.Epona.Eyes.Color" };
constexpr Color_RGBA8 kDefaults[] = { { 173, 57, 0, 255 }, { 255, 255, 255, 255 }, { 24, 24, 24, 255 } };
constexpr const char* kNativeLists[] = {
    "object_horse_link_child_DL_000C70",
    "object_horse_link_child_DL_001028",
    "object_horse_link_child_DL_0010D8",
    "object_horse_link_child_DL_0011E8",
    "object_horse_link_child_DL_001298",
    "object_horse_link_child_DL_0013A8",
    "object_horse_link_child_DL_0014B8",
    "object_horse_link_child_DL_001568",
    "object_horse_link_child_DL_001678",
    "object_horse_link_child_DL_00D500",
    "object_horse_link_child_Skinlimb_00A138SkinLimbDL_00D500",
};

struct NativeMask {
    std::string path;
    std::array<std::vector<uint8_t>, 3> parts;
};
std::map<std::string, NativeMask> sMasks;

EponaCosmetics::MaskTarget GetMaskTarget(const std::string& path, bool blinking) {
    auto [it, inserted] = sMasks.try_emplace(path);
    auto& mask = it->second;
    if (inserted) {
        mask.path = "__OTR__" + path;
        for (unsigned part = 0; part < 3; ++part) {
            mask.parts[part] = EponaMasks::BuildInverseMask(path, static_cast<EponaMasks::Part>(part));
        }
        // The closed blink has no iris. A fully removing mask keeps an Eyes overlay
        // invisible when segment 08 selects it, instead of tinting the whole eyelid.
        if (blinking && mask.parts[2].empty()) {
            mask.parts[2].resize(32 * 16, 1);
        }
    }
    EponaCosmetics::MaskTarget target{ mask.path.c_str() };
    for (unsigned part = 0; part < 3; ++part) {
        target.inverseMasks[part] = mask.parts[part].empty() ? nullptr : mask.parts[part].data();
    }
    return target;
}

EponaCosmetics::TextureMaterial ResolveMaterial(uint64_t hash, bool blinking) {
    EponaCosmetics::TextureMaterial material;
    if (blinking) {
        for (const auto* eye : { "gEponaEyeOpenTex", "gEponaEyeHalfTex", "gEponaEyeClosedTex" }) {
            material.targets.push_back(GetMaskTarget(std::string(kTexturePrefix) + eye, true));
        }
        return material;
    }
    if (hash == CRC64("objects/object_horse_link_child/gEponaTLUT")) {
        material.palette = true;
        return material;
    }
    // Hashes and masks are resolved only when constructing a cached DL variant.
    for (const auto& entry : EponaMasks::kEntries) {
        if (entry.name.find("object_horse_link_child_Tex_") != 0) {
            continue;
        }
        const std::string path = std::string(kTexturePrefix) + std::string(entry.name);
        if (hash == CRC64(path.c_str())) {
            material.targets.push_back(GetMaskTarget(path, false));
            break;
        }
    }
    return material;
}

struct CachedList {
    // Retain the source as long as a submitted raw copy can reference its data.
    std::shared_ptr<Fast::DisplayList> source;
    std::vector<Gfx> commands;
};
std::map<std::pair<const Fast::DisplayList*, unsigned>, CachedList> sDisplayListCopies;

Gfx* GetNativeListCopy(Gfx* original, unsigned changed) {
    // Skin mods may use N64 segmented pointers instead of host resource names.
    // The signature service expects readable host memory, so reject those first.
    if (reinterpret_cast<uintptr_t>(original) <= 0x0FFFFFFF) {
        return nullptr;
    }
    auto rm = Ship::Context::GetRawInstance()->GetResourceManager();
    if (!rm->OtrSignatureCheck(reinterpret_cast<const char*>(original))) {
        return nullptr;
    }
    const std::string path = reinterpret_cast<const char*>(original) + 7;
    bool known = false;
    for (const auto* name : kNativeLists) {
        known |= path == std::string(kTexturePrefix) + name;
    }
    if (!known) {
        return nullptr;
    }
    // A cache lookup can return native data while an active Alt replacement is
    // still unloaded. Resolve the selected archive path before consulting it.
    std::string resolved = path;
    if (rm->IsAltAssetsEnabled()) {
        const std::string altPath = "alt/" + path;
        if (rm->GetArchiveManager()->HasFile(altPath) || rm->GetArchiveManager()->HasFile(altPath + ".meta")) {
            resolved = altPath;
        }
    }
    auto loaded = rm->GetCachedResource(resolved, true);
    if (loaded == nullptr) {
        loaded = rm->LoadResource(resolved, true);
    }
    auto resource = std::dynamic_pointer_cast<Fast::DisplayList>(loaded);
    // Texture overrides are supported; custom geometry remains owned by its mod.
    if (resource == nullptr || resource->GetInitData()->IsCustom) {
        return nullptr;
    }
    const auto key = std::make_pair(resource.get(), changed);
    auto [it, inserted] = sDisplayListCopies.try_emplace(key);
    if (inserted) {
        it->second.source = resource;
        if (EponaCosmetics::NativeTemplateMatches(path, resource->Instructions, true)) {
            it->second.commands =
                EponaCosmetics::BuildNativeDisplayList(resource->Instructions, changed, ResolveMaterial);
        }
    }
    return it->second.commands.empty() ? nullptr : it->second.commands.data();
}

struct DrawSwap {
    SkinLimb* limb;
    SkinAnimatedLimbData* animated;
    Gfx* original;
};
std::vector<DrawSwap> sDrawSwaps;
Skin* sDrawingSkin = nullptr;
} // namespace

extern "C" int MMEponaCosmetics_BeginDraw(PlayState* play, Skin* skin) {
    unsigned changed = 0;
    for (unsigned part = 0; part < 3; ++part) {
        changed |= (CVarGetInteger(kChanged[part], 0) != 0) << part;
    }
    if (changed == 0 || skin == nullptr || skin->skeletonHeader == nullptr ||
        skin->skeletonHeader->segment == nullptr || sDrawingSkin != nullptr) {
        return false;
    }
#ifdef COMBO_BUILD
    Ship::OwnRMScope rmScope("mm");
#endif
    auto** limbs = static_cast<SkinLimb**>(Lib_SegmentedToVirtual(skin->skeletonHeader->segment));
    for (unsigned i = 0; i < skin->skeletonHeader->limbCount; ++i) {
        auto* limb = static_cast<SkinLimb*>(Lib_SegmentedToVirtual(limbs[i]));
        if (limb == nullptr || limb->segment == nullptr) {
            continue;
        }
        SkinAnimatedLimbData* animated = nullptr;
        Gfx* original = nullptr;
        if (limb->segmentType == SKIN_LIMB_TYPE_ANIMATED) {
            animated = static_cast<SkinAnimatedLimbData*>(Lib_SegmentedToVirtual(limb->segment));
            original = animated->dlist;
        } else if (limb->segmentType == SKIN_LIMB_TYPE_NORMAL) {
            original = static_cast<Gfx*>(limb->segment);
        }
        Gfx* copy = GetNativeListCopy(original, changed);
        if (copy == nullptr) {
            continue;
        }
        sDrawSwaps.push_back({ limb, animated, original });
        if (animated != nullptr) {
            animated->dlist = copy;
        } else {
            limb->segment = copy;
        }
    }
    if (sDrawSwaps.empty()) {
        return false;
    }
    sDrawingSkin = skin;
    auto* colors = static_cast<Gfx*>(GRAPH_ALLOC(play->state.gfxCtx, 11 * sizeof(Gfx)));
    for (unsigned part = 0; part < 3; ++part) {
        const auto color = CVarGetColor(kColors[part], kDefaults[part]);
        colors[part * 3] = gsSPGrayscale(true);
        colors[part * 3 + 1] = gsDPSetGrayscaleColor(color.r, color.g, color.b, 255);
        colors[part * 3 + 2] = gsSPEndDisplayList();
    }
    colors[9] = gsSPGrayscale(false);
    colors[10] = gsSPEndDisplayList();
    OPEN_DISPS(play->state.gfxCtx);
#ifdef COMBO_BUILD
    gSPComboRMPush(POLY_OPA_DISP++, "mm");
#endif
    for (unsigned part = 0; part < 4; ++part) {
        gSPSegment(POLY_OPA_DISP++, 9 + part, reinterpret_cast<uintptr_t>(&colors[part * 3]));
    }
    CLOSE_DISPS(play->state.gfxCtx);
    return true;
}

extern "C" void MMEponaCosmetics_EndDraw(PlayState* play, Skin* skin) {
    if (sDrawingSkin != skin) {
        return;
    }
    for (const auto& swap : sDrawSwaps) {
        if (swap.animated != nullptr) {
            swap.animated->dlist = swap.original;
        } else {
            swap.limb->segment = swap.original;
        }
    }
    sDrawSwaps.clear();
    sDrawingSkin = nullptr;
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
#ifdef COMBO_BUILD
    gSPComboRMPop(POLY_OPA_DISP++);
#endif
    CLOSE_DISPS(play->state.gfxCtx);
}
