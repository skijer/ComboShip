#include "NativeNpcPose.h"
#include "NativeNpcPoseResources.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Animation.h"
#include "soh/resource/type/Skeleton.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include "soh/resource/type/Array.h"
#include <fast/resource/type/DisplayList.h>

static std::string NativeNpcPose_Path(const SkelAnime* skelAnime, AnimationHeader* animation) {
    if (skelAnime == nullptr || animation == nullptr || !ResourceMgr_IsAltAssetsEnabled() ||
        !ResourceMgr_OTRSigCheck(reinterpret_cast<char*>(animation))) {
        return {};
    }
    for (auto entry = SOH::SkeletonPatcher::skeletons.rbegin(); entry != SOH::SkeletonPatcher::skeletons.rend();
         ++entry) {
        if (entry->skelAnime == skelAnime) {
            return ResourceMgr_FileAltExists(entry->vanillaSkeletonPath.c_str())
                       ? SOH::NativeNpcPose::AnimationPath(entry->vanillaSkeletonPath,
                                                           reinterpret_cast<const char*>(animation))
                       : std::string{};
        }
    }
    return {};
}

extern "C" AnimationHeader* NativeNpcPose_ResolveAnimation(const SkelAnime* skelAnime, AnimationHeader* animation) {
    const auto path = NativeNpcPose_Path(skelAnime, animation);
    if (path.empty() || !ResourceMgr_FileAltExists(path.c_str())) {
        return animation;
    }
    const auto fitted =
        std::dynamic_pointer_cast<SOH::Animation>(ResourceMgr_GetResourceByNameHandlingMQ(("alt/" + path).c_str()));
    const auto native = std::dynamic_pointer_cast<SOH::Animation>(
        ResourceMgr_GetResourceByNameHandlingMQ(reinterpret_cast<const char*>(animation)));
    if (fitted == nullptr || native == nullptr || fitted->type != SOH::AnimationType::Normal ||
        native->type != SOH::AnimationType::Normal ||
        !SOH::NativeNpcPose::Validate(*fitted, *native, skelAnime->limbCount)) {
        return animation;
    }
    return reinterpret_cast<AnimationHeader*>(&fitted->animationData.animationHeader);
}

extern "C" Gfx* NativeNpcPose_SelectLimb(const SkelAnime* skelAnime, s32 limbIndex, Gfx* original) {
    if (original == nullptr || skelAnime == nullptr || limbIndex < 1 || limbIndex >= skelAnime->limbCount) {
        return original;
    }
    auto path = NativeNpcPose_Path(skelAnime, static_cast<AnimationHeader*>(skelAnime->animation));
    if (path.empty()) {
        return original;
    }
    char suffix[32];
    snprintf(suffix, sizeof(suffix), "/Limbs/Limb%02d", limbIndex - 1);
    if (skelAnime->skeletonHeader &&
        NativeNpcPose_GetSkinPlan(skelAnime->skeleton,
                                  reinterpret_cast<const FlexSkeletonHeader*>(skelAnime->skeletonHeader)->dListCount)) {
        const auto split = path.find("/PoseAnimations/");
        bool ocarina = false;
        if (ResourceMgr_OTRSigCheck(reinterpret_cast<char*>(original))) {
            auto originalPath = SOH::NativeNpcPose::Canonical(reinterpret_cast<const char*>(original));
            ocarina = originalPath == "objects/object_sa/NativeNpcV1/Props/RightHandOcarina" ||
                      originalPath == "objects/object_sa/gSariaRightHandAndOcarinaDL";
        }
        const auto base = path.substr(0, split) + "/Skin" + (ocarina ? "/Props/RightHandOcarina" : suffix);
        for (const auto& skinPath : { path + "/Skin" + suffix, base }) {
            if (!ResourceMgr_FileAltExists(skinPath.c_str()))
                continue;
            auto resource = std::dynamic_pointer_cast<Fast::DisplayList>(
                ResourceMgr_GetResourceByNameHandlingMQ(("alt/" + skinPath).c_str()));
            if (resource && resource->GetPointerSize() > 0)
                return reinterpret_cast<Gfx*>(resource->GetRawPointer());
        }
    }
    path += suffix;
    if (!ResourceMgr_FileAltExists(path.c_str())) {
        return original;
    }
    // Return the resource's display-list data; never a temporary path string.
    auto resource =
        std::dynamic_pointer_cast<Fast::DisplayList>(ResourceMgr_GetResourceByNameHandlingMQ(("alt/" + path).c_str()));
    return resource != nullptr && resource->GetPointerSize() > 0 ? reinterpret_cast<Gfx*>(resource->GetRawPointer())
                                                                 : original;
}

static float NativeNpcPose_SkinFloat(const SOH::Array* plan, size_t offset) {
    uint32_t bits = plan->Scalars[offset].u16 | (uint32_t(plan->Scalars[offset + 1].u16) << 16);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

extern "C" float NativeNpcPose_GetDrawScale(void** skeleton) {
    if (!skeleton || !ResourceMgr_IsAltAssetsEnabled())
        return 1.f;
    for (auto entry = SOH::SkeletonPatcher::skeletons.rbegin(); entry != SOH::SkeletonPatcher::skeletons.rend();
         ++entry) {
        if (!entry->skelAnime || entry->skelAnime->skeleton != skeleton)
            continue;
        const auto path = SOH::NativeNpcPose::DrawScalePath(entry->vanillaSkeletonPath);
        if (path.empty() || !ResourceMgr_FileAltExists(entry->vanillaSkeletonPath.c_str()) ||
            !ResourceMgr_FileAltExists(path.c_str()))
            return 1.f;
        const auto resource =
            std::dynamic_pointer_cast<SOH::Array>(ResourceMgr_GetResourceByNameHandlingMQ(("alt/" + path).c_str()));
        return NativeNpcDrawScale::Read(resource.get());
    }
    return 1.f;
}

extern "C" const void* NativeNpcPose_GetSkinPlan(void** skeleton, s32 matrixCount) {
    if (!skeleton || matrixCount < 1 || matrixCount > 32 || !ResourceMgr_IsAltAssetsEnabled())
        return nullptr;
    for (auto entry = SOH::SkeletonPatcher::skeletons.rbegin(); entry != SOH::SkeletonPatcher::skeletons.rend();
         ++entry) {
        if (!entry->skelAnime || entry->skelAnime->skeleton != skeleton)
            continue;
        auto path = SOH::NativeNpcPose::AnimationPath(entry->vanillaSkeletonPath, "objects/Skin");
        if (path.empty() || !ResourceMgr_FileAltExists(entry->vanillaSkeletonPath.c_str()))
            return nullptr;
        path = path.substr(0, path.find("/PoseAnimations/")) + "/Skin/Palette";
        if (!ResourceMgr_FileAltExists(path.c_str()))
            return nullptr;
        auto plan =
            std::dynamic_pointer_cast<SOH::Array>(ResourceMgr_GetResourceByNameHandlingMQ(("alt/" + path).c_str()));
        if (!plan || plan->ArrayType != SOH::ArrayResourceType::Scalar ||
            plan->ArrayScalarType != SOH::ScalarType::ZSCALAR_U16 || plan->Scalars.size() < 4 ||
            plan->ArrayCount != plan->Scalars.size())
            return nullptr;
        const auto& words = plan->Scalars;
        const unsigned count = words[3].u16;
        if (words[0].u16 != 0x4e53 || words[1].u16 != 1 || words[2].u16 != matrixCount || count == 0 ||
            count + matrixCount > 256 || words.size() != 4 + matrixCount * 24 + count * 3)
            return nullptr;
        for (int i = 0; i < matrixCount; ++i) {
            float m[12];
            for (int j = 0; j < 12; ++j) {
                m[j] = NativeNpcPose_SkinFloat(plan.get(), 4 + i * 24 + j * 2);
                if (!std::isfinite(m[j]) || std::abs(m[j]) > 1e7f)
                    return nullptr;
            }
            const float determinant = m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) +
                                      m[2] * (m[4] * m[9] - m[5] * m[8]);
            if (std::abs(determinant) < 1e-9f)
                return nullptr;
        }
        for (size_t i = 4 + matrixCount * 24; i < words.size(); i += 3) {
            if (words[i].u16 >= words[i + 1].u16 || words[i + 1].u16 >= matrixCount || words[i + 2].u16 == 0 ||
                words[i + 2].u16 >= 16)
                return nullptr;
        }
        return plan.get(); // Owned by the resource manager for the duration of this draw.
    }
    return nullptr;
}

extern "C" s32 NativeNpcPose_SkinMatrixCount(const void* opaque) {
    return opaque ? static_cast<const SOH::Array*>(opaque)->Scalars[3].u16 : 0;
}

extern "C" void NativeNpcPose_BlendSkinMatrix(const void* opaque, s32 index, const MtxF* native, MtxF* result) {
    const auto* plan = static_cast<const SOH::Array*>(opaque);
    std::memset(result, 0, sizeof(*result));
    result->xx = result->yy = result->zz = result->ww = 1.f;
    if (!plan || index < 0 || index >= NativeNpcPose_SkinMatrixCount(plan) || !native)
        return;
    const auto& words = plan->Scalars;
    const size_t offset = 4 + words[2].u16 * 24 + index * 3;
    float* out = &result->xx;
    std::fill(out, out + 16, 0.f);
    for (int influence = 0; influence < 2; ++influence) {
        const unsigned bone = words[offset + influence].u16;
        const float weight = (influence ? words[offset + 2].u16 : 16 - words[offset + 2].u16) / 16.f;
        const float* current = &native[bone].xx;
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row) {
                for (int k = 0; k < 4; ++k) {
                    float inverse = k == 3 ? (col == 3 ? 1.f : 0.f)
                                           : NativeNpcPose_SkinFloat(plan, 4 + bone * 24 + (k * 4 + col) * 2);
                    out[col * 4 + row] += current[k * 4 + row] * inverse * weight;
                }
            }
    }
}

extern "C" Gfx* NativeNpcPose_SelectProp(const SkelAnime* skelAnime, const char* prop, Gfx* original) {
    if (skelAnime == nullptr || prop == nullptr || *prop == '\0' ||
        std::string_view(prop).find('/') != std::string_view::npos) {
        return original;
    }
    auto path = NativeNpcPose_Path(skelAnime, static_cast<AnimationHeader*>(skelAnime->animation));
    if (path.empty()) {
        return original;
    }
    path += "/Props/";
    path += prop;
    if (!ResourceMgr_FileAltExists(path.c_str())) {
        return original;
    }
    auto resource =
        std::dynamic_pointer_cast<Fast::DisplayList>(ResourceMgr_GetResourceByNameHandlingMQ(("alt/" + path).c_str()));
    return resource != nullptr && resource->GetPointerSize() > 0 ? reinterpret_cast<Gfx*>(resource->GetRawPointer())
                                                                 : original;
}
