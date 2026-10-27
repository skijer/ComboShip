#pragma once

#include <array>
#include <string_view>

// Include after the host's native engine types.
namespace BlackMarketKeeperAssets {
alignas(2) inline constexpr char kNativeSkeleton[] = "__OTR__objects/object_bg/gTreasureChestShopGalSkel";
alignas(2) inline constexpr char kNativeAnimation[] = "__OTR__objects/object_bg/object_bg_Anim_009890";
inline constexpr const char* kNativeEyes[] = {
    "__OTR__objects/object_bg/gTreasureChestShopGalEyeOpenUpTex",
    "__OTR__objects/object_bg/gTreasureChestShopGalEyeHalfUpTex",
    "__OTR__objects/object_bg/gTreasureChestShopGalEyeClosedTex",
    "__OTR__objects/object_bg/gTreasureChestShopGalEyeHalfUpTex",
};

template <size_t N, typename Exists> bool CustomAvailable(const char* const (&resources)[N], Exists exists) {
    bool found = false;
    for (const char* path : resources) {
        if (!std::string_view(path).starts_with("custom/skiejer_market/keeper/"))
            continue;
        found = true;
        if (!exists(path))
            return false;
    }
    return found;
}

inline constexpr const char* kNativeLimbs[] = {
    "objects/object_bg/gTreasureChestShopGalRootLimb",
    "objects/object_bg/gTreasureChestShopGalHipsLimb",
    "objects/object_bg/gTreasureChestShopGalTorsoLimb",
    "objects/object_bg/gTreasureChestShopGalNeckLimb",
    "objects/object_bg/gTreasureChestShopGalHeadLimb",
    "objects/object_bg/gTreasureChestShopGalLeftArmRootLimb",
    "objects/object_bg/gTreasureChestShopGalLeftUpperArmLimb",
    "objects/object_bg/gTreasureChestShopGalLeftForearmLimb",
    "objects/object_bg/gTreasureChestShopGalLeftHandLimb",
    "objects/object_bg/gTreasureChestShopGalRightArmRootLimb",
    "objects/object_bg/gTreasureChestShopGalRightUpperArmLimb",
    "objects/object_bg/gTreasureChestShopGalRightForearmLimb",
    "objects/object_bg/gTreasureChestShopGalRightHandLimb",
    "objects/object_bg/gTreasureChestShopGalRightLegRootLimb",
    "objects/object_bg/gTreasureChestShopGalRightThighLimb",
    "objects/object_bg/gTreasureChestShopGalRightShinLimb",
    "objects/object_bg/gTreasureChestShopGalRightFootRootLimb",
    "objects/object_bg/gTreasureChestShopGalRightFootLimb",
    "objects/object_bg/gTreasureChestShopGalLeftLegRootLimb",
    "objects/object_bg/gTreasureChestShopGalLeftThighLimb",
    "objects/object_bg/gTreasureChestShopGalLeftShinLimb",
    "objects/object_bg/gTreasureChestShopGalLeftFootRootLimb",
    "objects/object_bg/gTreasureChestShopGalLeftFootLimb",
};

// OoT owns this private hierarchy. The existing archive-scoped importers pin
// its animation, graph, images and palettes through queued renderer draws.
// Never replace the cached MM skeleton's limb pointers or resolve MM DLs by
// a global name that another game's archive can shadow.
struct NativeModel {
    FlexSkeletonHeader skeleton{};
    std::array<StandardLimb, 23> limbs{};
    std::array<void*, 23> table{};
    AnimationHeader* animation = nullptr;
    std::array<void*, 3> eyes{};

    template <typename Skeleton, typename Animation, typename Resource, typename Graph>
    bool Load(Skeleton loadSkeleton, Animation loadAnimation, Resource loadResource, Graph loadGraph) {
        *this = {};
        const auto* source = static_cast<FlexSkeletonHeader*>(loadSkeleton(kNativeSkeleton));
        animation = static_cast<AnimationHeader*>(loadAnimation(kNativeAnimation));
        if (!source || source->sh.limbCount != limbs.size() || source->dListCount != 16 || !animation ||
            animation->common.frameCount <= 0)
            return false;
        skeleton = *source;
        unsigned matrices = 0;
        for (size_t i = 0; i < limbs.size(); ++i) {
            const auto* limb = static_cast<StandardLimb*>(loadResource(kNativeLimbs[i]));
            if (!limb || (limb->child != 255 && limb->child >= limbs.size()) ||
                (limb->sibling != 255 && limb->sibling >= limbs.size()))
                return false;
            limbs[i] = *limb;
            table[i] = &limbs[i];
            if (limb->dList) {
                const auto* path = reinterpret_cast<const char*>(limb->dList);
                if (!std::string_view(path).starts_with("__OTR__objects/object_bg/"))
                    return false;
                limbs[i].dList = loadGraph(path);
                if (!limbs[i].dList)
                    return false;
                ++matrices;
            }
        }
        if (matrices != source->dListCount)
            return false;
        for (size_t i = 0; i < eyes.size(); ++i) {
            eyes[i] = loadResource(kNativeEyes[i]);
            if (!eyes[i])
                return false;
        }
        skeleton.sh.segment = table.data();
        return true;
    }
};
} // namespace BlackMarketKeeperAssets
