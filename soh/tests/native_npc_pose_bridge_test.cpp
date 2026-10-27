#include <cassert>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <cmath>
#include <fast/lus_gbi.h>
#undef GIMMCMD
#include "soh/Enhancements/cosmetics/NativeNpcPose.h"
#include "soh/Enhancements/cosmetics/NativeNpcPoseResources.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/resource/type/Animation.h"
#include "soh/resource/type/Skeleton.h"
#include "soh/resource/type/Array.h"
#include <fast/resource/type/DisplayList.h>

// In-memory resource-manager fixture; the production resolver and real SOH
// animation resources below are compiled and linked without the game runtime.
namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData> init) {
}
IResource::~IResource() = default;
} // namespace Ship
namespace Fast {
DisplayList::DisplayList() : Resource(nullptr) {
}
DisplayList::~DisplayList() = default;
Gfx* DisplayList::GetPointer() {
    return Instructions.data();
}
size_t DisplayList::GetPointerSize() {
    return Instructions.size() * sizeof(Gfx);
}
} // namespace Fast
std::vector<SOH::SkeletonPatchInfo> SOH::SkeletonPatcher::skeletons;
static std::unordered_map<std::string, std::shared_ptr<Ship::IResource>> resources;
static bool alt = true;
bool ResourceMgr_IsAltAssetsEnabled() {
    return alt;
}
int ResourceMgr_OTRSigCheck(char* data) {
    return data && std::memcmp(data, "__OTR__", 7) == 0;
}
uint8_t ResourceMgr_FileAltExists(const char* path) {
    return resources.contains("alt/" + std::string(SOH::NativeNpcPose::Canonical(path)));
}
std::shared_ptr<Ship::IResource> ResourceMgr_GetResourceByNameHandlingMQ(const char* path) {
    auto key = std::string(SOH::NativeNpcPose::Canonical(path));
    if (alt && resources.contains("alt/" + key))
        return resources["alt/" + key];
    return resources.contains(key) ? resources[key] : nullptr;
}
static std::shared_ptr<SOH::Animation> Clip() {
    auto clip = std::make_shared<SOH::Animation>();
    clip->type = SOH::AnimationType::Normal;
    clip->animationData.animationHeader.common.frameCount = 2;
    clip->animationData.animationHeader.staticIndexMax = 3;
    clip->rotationValues = { 10, 20, 30, 100, 200 };
    clip->rotationIndices = { { 0, 1, 2 }, { 3, 1, 2 } };
    return clip;
}
int main() {
    SkelAnime skel = {};
    skel.limbCount = 2;
    const char source[] = "__OTR__objects/object_os_anime/gKokiriStandingArmsBehindBackAnim";
    auto original = reinterpret_cast<AnimationHeader*>(const_cast<char*>(source));
    skel.animation = original;
    auto native = Clip(), fit = Clip();
    fit->rotationValues[3] = 1000;
    const std::string skeleton = "objects/object_kw1/NativeNpcV1/Skel";
    const auto key = "alt/" + SOH::NativeNpcPose::AnimationPath(skeleton, source);
    resources["objects/object_os_anime/gKokiriStandingArmsBehindBackAnim"] = native;
    resources["alt/" + skeleton] = Clip(); // Presence witness only.
    resources[key] = fit;
    SOH::SkeletonPatcher::skeletons.push_back({ &skel, skeleton });
    SOH::SkeletonPatcher::skeletons.push_back({ &skel, "objects/object_fa/gFaSkel" });
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    SOH::SkeletonPatcher::skeletons.pop_back();
    auto result = NativeNpcPose_ResolveAnimation(&skel, original);
    assert(result == reinterpret_cast<AnimationHeader*>(&fit->animationData.animationHeader));
    assert(skel.animation == original);
    alt = false;
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    alt = true;
    fit->rotationValues[0]++;
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    fit->rotationValues[0]--;
    fit->type = SOH::AnimationType::Link;
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    fit->type = SOH::AnimationType::Normal;
    resources.erase(key);
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);
    Gfx fallback[1] = {};
    const auto limbKey = key + "/Limbs/Limb00";
    auto display = std::make_shared<Fast::DisplayList>();
    display->Instructions.resize(1);
    resources[limbKey] = display;
    assert(NativeNpcPose_SelectLimb(&skel, 1, fallback) == reinterpret_cast<Gfx*>(display->GetRawPointer()));
    assert(NativeNpcPose_SelectLimb(&skel, 0, fallback) == fallback);
    resources[key + "/Props/Basket"] = display;
    assert(NativeNpcPose_SelectProp(&skel, "Basket", fallback) == reinterpret_cast<Gfx*>(display->GetRawPointer()));
    assert(NativeNpcPose_SelectProp(&skel, "MissingProp", fallback) == fallback);
    resources[limbKey] = native;
    assert(NativeNpcPose_SelectLimb(&skel, 1, fallback) == fallback);
    SOH::SkeletonPatcher::skeletons[0].vanillaSkeletonPath = "objects/object_fa/gFaSkel";
    assert(NativeNpcPose_ResolveAnimation(&skel, original) == original);

    // Exercise Fado's seated correction through both the native key and the
    // older paired Seated5 alias, with the real 16-joint Kokiri layout.
    const std::string fadoRig = "objects/object_fa/YoungFadoNpcPOC1/Skel";
    const char seated[] = "__OTR__objects/object_os_anime/gKokiriSittingCrossedArmsLegsAnim";
    const char legacy[] = "__OTR__objects/object_fa/YoungFadoNpcPOC1/Animations/Seated5";
    skel.limbCount = 16;
    auto nativeSeated = Clip(), fittedSeated = Clip();
    nativeSeated->rotationIndices.resize(16, { 3, 1, 2 });
    fittedSeated->rotationIndices.resize(16, { 3, 1, 2 });
    fittedSeated->rotationValues[3] = 2222;
    resources[std::string(SOH::NativeNpcPose::Canonical(seated))] = nativeSeated;
    resources[std::string(SOH::NativeNpcPose::Canonical(legacy))] = nativeSeated;
    resources["alt/" + fadoRig] = Clip();
    const auto fittedKey = "alt/" + SOH::NativeNpcPose::AnimationPath(fadoRig, seated);
    resources[fittedKey] = fittedSeated;
    resources[fittedKey + "/Limbs/Limb11"] = display;
    SOH::SkeletonPatcher::skeletons[0].vanillaSkeletonPath = fadoRig;
    for (const char* animation : { seated, legacy }) {
        skel.animation = reinterpret_cast<AnimationHeader*>(const_cast<char*>(animation));
        assert(NativeNpcPose_ResolveAnimation(&skel, static_cast<AnimationHeader*>(skel.animation)) ==
               reinterpret_cast<AnimationHeader*>(&fittedSeated->animationData.animationHeader));
        assert(NativeNpcPose_SelectLimb(&skel, 12, fallback) == reinterpret_cast<Gfx*>(display->GetRawPointer()));
        assert(skel.animation == reinterpret_cast<const AnimationHeader*>(animation));
        alt = false;
        assert(NativeNpcPose_ResolveAnimation(&skel, static_cast<AnimationHeader*>(skel.animation)) == skel.animation);
        assert(NativeNpcPose_SelectLimb(&skel, 12, fallback) == fallback);
        alt = true;
    }
    puts("PASS Fado seated native/legacy keys: fitted arm rotations, hand mesh, native identity and Alt fallback.");
    puts("PASS production pose resource bridge: exact private clip/mesh/prop, live Alt, absent/invalid resources, "
         "original "
         "identity and unrelated actor fallback.");
    void* limbs[2] = {};
    skel.skeleton = limbs;
    SOH::SkeletonPatcher::skeletons[0].vanillaSkeletonPath = skeleton;
    auto skin = std::make_shared<SOH::Array>();
    skin->ArrayType = SOH::ArrayResourceType::Scalar;
    skin->ArrayScalarType = SOH::ScalarType::ZSCALAR_U16;
    const auto word = [&](uint16_t value) {
        SOH::ScalarData s{};
        s.u16 = value;
        skin->Scalars.push_back(s);
    };
    for (auto value : { 0x4e53, 1, 2, 1 })
        word(value);
    for (int bone = 0; bone < 2; ++bone) {
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 4; ++col) {
                float value = row == col ? 1.f : (bone == 1 && row == 0 && col == 3 ? -1.f : 0.f);
                uint32_t bits;
                std::memcpy(&bits, &value, sizeof(bits));
                word(bits);
                word(bits >> 16);
            }
        }
    }
    for (auto value : { 0, 1, 8 })
        word(value);
    skin->ArrayCount = skin->Scalars.size();
    resources["alt/objects/object_kw1/NativeNpcV1/Skin/Palette"] = skin;
    const void* plan = NativeNpcPose_GetSkinPlan(limbs, 2);
    assert(plan && NativeNpcPose_SkinMatrixCount(plan) == 1);
    FlexSkeletonHeader flex{};
    flex.dListCount = 2;
    skel.skeletonHeader = &flex.sh;
    skel.animation = original;
    resources["alt/objects/object_kw1/NativeNpcV1/Skin/Limbs/Limb00"] = display;
    assert(NativeNpcPose_SelectLimb(&skel, 1, fallback) == reinterpret_cast<Gfx*>(display->GetRawPointer()));
    // Shared joint surfaces must work for every static/native/credits pose,
    // including a new animation that has no fitted clip or pose-specific mesh.
    for (const char* animation :
         { "__OTR__objects/object_os_anime/gKokiriIdleAnim",
           "__OTR__objects/object_os_anime/gKokiriStandingArmsBehindBackAnim",
           "__OTR__objects/object_os_anime/gKokiriStandingHandsOnHipsAnim",
           "__OTR__objects/object_os_anime/gKokiriSittingHeadOnHandAnim",
           "__OTR__objects/object_os_anime/gKokiriSittingCrossedLegsAnim",
           "__OTR__objects/object_os_anime/gKokiriSittingCrossedArmsLegsAnim",
           "__OTR__objects/object_os_anime/gKokiriStandUpAnim",
           "__OTR__objects/object_os_anime/gKokiriStandingHandOnChestAnim",
           "__OTR__objects/object_os_anime/gKokiriStandingRightArmUpAnim", "__OTR__objects/object_ec/gDemoEcKokiriAnim",
           "__OTR__objects/object_ec/gDemoEcDancingKokiriAnim",
           "__OTR__objects/object_os_anime/gFutureKokiriPoseAnim" }) {
        skel.animation = reinterpret_cast<AnimationHeader*>(const_cast<char*>(animation));
        assert(NativeNpcPose_SelectLimb(&skel, 1, fallback) == reinterpret_cast<Gfx*>(display->GetRawPointer()));
        alt = false;
        assert(NativeNpcPose_SelectLimb(&skel, 1, fallback) == fallback);
        alt = true;
    }
    skel.animation = original;
    puts("PASS all six static poses, native girl poses, credits and future clips inherit shared skin without a clip "
         "whitelist.");
    MtxF matrices[2] = {}, blended = {};
    for (auto& m : matrices)
        m.xx = m.yy = m.zz = m.ww = 1.f;
    matrices[1].xx = matrices[1].yy = 0.f;
    matrices[1].xy = -1.f;
    matrices[1].yx = 1.f;
    matrices[1].xw = 1.f;
    NativeNpcPose_BlendSkinMatrix(plan, 0, matrices, &blended);
    assert(std::abs(blended.xx + blended.xy * .2f + blended.xw - .9f) < 1e-6f);
    assert(std::abs(blended.yx + blended.yy * .2f + blended.yw - .1f) < 1e-6f);
    matrices[0].xw += 500.f;
    matrices[1].xw += 500.f;
    NativeNpcPose_BlendSkinMatrix(plan, 0, matrices, &blended);
    assert(std::abs(blended.xx + blended.xy * .2f + blended.xw - 500.9f) < 1e-4f);
    assert(NativeNpcPose_GetSkinPlan(limbs, 3) == nullptr);
    alt = false;
    assert(NativeNpcPose_GetSkinPlan(limbs, 2) == nullptr);
    alt = true;
    skin->Scalars.back().u16 = 17;
    assert(NativeNpcPose_GetSkinPlan(limbs, 2) == nullptr);
    assert(NativeNpcPose_SelectLimb(&skel, 1, fallback) == fallback);
    skin->Scalars.back().u16 = 8;
    skel.limbCount = 16;
    const std::string saria = "objects/object_sa/NativeNpcV1";
    SOH::SkeletonPatcher::skeletons[0].vanillaSkeletonPath = saria + "/Skel";
    resources["alt/" + saria + "/Skel"] = Clip();
    resources["alt/" + saria + "/Skin/Palette"] = skin;
    resources["alt/" + saria + "/Skin/Limbs/Limb14"] = display;
    auto instrument = std::make_shared<Fast::DisplayList>();
    instrument->Instructions.resize(1);
    resources["alt/" + saria + "/Skin/Props/RightHandOcarina"] = instrument;
    const char held[] = "__OTR__objects/object_sa/NativeNpcV1/Props/RightHandOcarina";
    assert(NativeNpcPose_SelectLimb(&skel, 15, reinterpret_cast<Gfx*>(const_cast<char*>(held))) ==
           reinterpret_cast<Gfx*>(instrument->GetRawPointer()));
    puts("PASS production joint blending: live two-bone bend, bind pivots, Alt off and invalid-palette fallback.");
    const auto drawScale = [](float value) {
        auto data = std::make_shared<SOH::Array>();
        data->ArrayType = SOH::ArrayResourceType::Scalar;
        data->ArrayScalarType = SOH::ScalarType::ZSCALAR_U16;
        uint32_t bits;
        std::memcpy(&bits, &value, sizeof(bits));
        for (uint16_t word : { uint16_t(0x4e44), uint16_t(1), uint16_t(bits), uint16_t(bits >> 16) }) {
            SOH::ScalarData scalar{};
            scalar.u16 = word;
            data->Scalars.push_back(scalar);
        }
        data->ArrayCount = data->Scalars.size();
        return data;
    };
    for (const char* model :
         { "objects/object_sa/NativeNpcV1", "objects/object_ma1/NativeNpcV1", "objects/object_fa/YoungFadoNpcPOC1" }) {
        const std::string ns = model, scalePath = "alt/" + ns + "/DrawScale", rig = ns + "/Skel";
        SOH::SkeletonPatcher::skeletons[0].vanillaSkeletonPath = rig;
        resources["alt/" + rig] = Clip();
        assert(NativeNpcPose_GetDrawScale(limbs) == 1.f);
        const float factor = ns == "objects/object_fa/YoungFadoNpcPOC1" ? 1.25f : 1.10f;
        auto scale = drawScale(factor);
        resources[scalePath] = scale;
        assert(std::abs(NativeNpcPose_GetDrawScale(limbs) - factor) < 1e-6f);
        alt = false;
        assert(NativeNpcPose_GetDrawScale(limbs) == 1.f);
        alt = true;
        resources.erase("alt/" + rig);
        assert(NativeNpcPose_GetDrawScale(limbs) == 1.f);
        resources["alt/" + rig] = Clip();
        for (float invalid : { 0.f, -1.f, .49f, 2.01f, INFINITY, -INFINITY, NAN }) {
            resources[scalePath] = drawScale(invalid);
            assert(NativeNpcPose_GetDrawScale(limbs) == 1.f);
        }
        resources[scalePath] = scale;
        scale->ArrayCount = 3;
        assert(NativeNpcPose_GetDrawScale(limbs) == 1.f);
        scale->ArrayCount = 4;
        scale->Scalars[1].u16 = 2;
        assert(NativeNpcPose_GetDrawScale(limbs) == 1.f);
        scale->Scalars[1].u16 = 1;
        scale->Scalars[0].u16 = 0;
        assert(NativeNpcPose_GetDrawScale(limbs) == 1.f);
        scale->Scalars[0].u16 = 0x4e44;
        scale->ArrayScalarType = SOH::ScalarType::ZSCALAR_S16;
        assert(NativeNpcPose_GetDrawScale(limbs) == 1.f);
        resources[scalePath] = display;
        assert(NativeNpcPose_GetDrawScale(limbs) == 1.f);
        resources.erase(scalePath);
    }
    for (const char* unrelated :
         { "objects/object_ma1/gMalonChildSkel", "objects/object_sa/gSariaSkel", "objects/object_ma2/NativeNpcV1/Skel",
           "objects/object_kw1/NativeNpcV1/Skel", "objects/object_fa/gFaSkel" }) {
        SOH::SkeletonPatcher::skeletons[0].vanillaSkeletonPath = unrelated;
        assert(NativeNpcPose_GetDrawScale(limbs) == 1.f);
    }
    assert(NativeNpcPose_GetDrawScale(nullptr) == 1.f);
    puts("PASS production draw scale: opt-in Malon/Saria/Fado, missing/invalid/Alt-off and unrelated fallback.");
}
