#pragma once

#include "actors/PlacedSariaABI.h"
#include "actors/NativeNpcDrawScale.h"
#include "soh/resource/type/Array.h"
#include "mods/transformation_masks/assets/mm_normal_actor_resource.h"
#include <fast/resource/type/DisplayList.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <set>

namespace PlacedSaria {
using Resource = MmNormalActor::Resource;
using Load = std::function<Resource(const std::string&)>;
inline constexpr char kNamespace[] = "objects/object_sa/NativeNpcV1";
inline constexpr const char* kAnimations[] = {
    "objects/object_sa/gSariaWaitArmsToSideAnim",
    "objects/object_sa/gSariaHandsBehindBackWaitAnim",
    "objects/object_sa/gSariaPlayingOcarinaAnim",
    "objects/object_sa/gSariaSitting3Anim",
};
inline constexpr unsigned kAnimationFrames[] = { 24, 24, 65, 24 };

/* Render commands may outlive an actor destroyed during a scene handoff.
 * Keep routed names stable; actual foreign resources are held by each snapshot. */
inline const char* Route(const std::string& path) {
    static std::set<std::string> names;
    return names.insert("__OTR__@oot:" + path).first->c_str();
}

struct Snapshot {
    std::vector<Resource> retained;
    std::array<StandardLimb, 16> limbs;
    std::array<void*, 16> pointers;
    FlexSkeletonHeader skeleton;
    ComboPlacedSariaResources result{};
    float drawScale = 1.f;
};

inline bool SameRoot(const AnimationHeader& a, const AnimationHeader& b) {
    for (int frame = 0; frame < a.common.frameCount; ++frame) {
        const u16* ai = reinterpret_cast<const u16*>(a.jointIndices);
        const u16* bi = reinterpret_cast<const u16*>(b.jointIndices);
        for (int axis = 0; axis < 3; ++axis) {
            auto sample = [frame](const AnimationHeader& h, u16 index) {
                return h.frameData[index + (index < h.staticIndexMax ? 0 : frame)];
            };
            if (sample(a, ai[axis]) != sample(b, bi[axis]))
                return false;
        }
    }
    return true;
}

inline bool SameHierarchy(const Resource& a, const Resource& b) {
    auto* first = static_cast<FlexSkeletonHeader*>(a->GetRawPointer());
    auto* second = static_cast<FlexSkeletonHeader*>(b->GetRawPointer());
    for (unsigned i = 0; i < 16; ++i) {
        auto* x = static_cast<StandardLimb*>(first->sh.segment[i]);
        auto* y = static_cast<StandardLimb*>(second->sh.segment[i]);
        if (x->child != y->child || x->sibling != y->sibling)
            return false;
    }
    return true;
}

inline std::unique_ptr<Snapshot> Build(const Load& load, bool alt, unsigned pose) {
    pose = pose < std::size(kAnimations) && kAnimations[pose] ? pose : 0;
    auto snapshot = std::make_unique<Snapshot>();
    auto nativeAnimation = load(kAnimations[pose]);
    if (!MmNormalActor::ValidateAnimation(nativeAnimation, 16, kAnimationFrames[pose]))
        return nullptr;
    auto animation = nativeAnimation;
    auto skeleton = load("objects/object_sa/gSariaSkel");
    std::vector<Resource> nativeRig;
    if (!MmNormalActor::ValidateSkeleton(skeleton, 16, 15, nativeRig))
        return nullptr;

    std::vector<Resource> selectedRig, baseRig;
    auto privateRig = alt ? load(std::string("alt/") + kNamespace + "/Skel") : nullptr;
    auto privateBase = privateRig ? load(std::string(kNamespace) + "/Skel") : nullptr;
    const std::string head = std::string("alt/") + kNamespace + "/Body/Limb15";
    auto privateHead = privateBase ? load(head) : nullptr;
    const bool custom = std::dynamic_pointer_cast<Fast::DisplayList>(privateHead) &&
                        privateHead->GetPointerSize() >= sizeof(Gfx) &&
                        MmNormalActor::ValidateSkeleton(privateBase, 16, 15, baseRig) &&
                        MmNormalActor::ValidateSkeleton(privateRig, 16, 15, selectedRig) &&
                        SameHierarchy(skeleton, privateBase) && SameHierarchy(skeleton, privateRig);
    if (custom) {
        skeleton = privateRig;
        snapshot->retained = std::move(selectedRig);
        snapshot->retained.push_back(privateHead);
    } else {
        snapshot->retained = std::move(nativeRig);
    }
    const auto drawScale =
        custom ? std::dynamic_pointer_cast<SOH::Array>(load(std::string("alt/") + kNamespace + "/DrawScale")) : nullptr;
    snapshot->drawScale = NativeNpcDrawScale::Read(drawScale.get());
    const std::string poseRoot = std::string("alt/") + kNamespace + "/PoseAnimations/" + kAnimations[pose];
    auto fitted = custom ? load(poseRoot) : nullptr;
    if (MmNormalActor::ValidateAnimation(fitted, 16, kAnimationFrames[pose]) &&
        SameRoot(*static_cast<AnimationHeader*>(fitted->GetRawPointer()),
                 *static_cast<AnimationHeader*>(nativeAnimation->GetRawPointer())))
        animation = fitted;
    snapshot->retained.push_back(nativeAnimation);
    snapshot->retained.push_back(animation);

    auto* source = static_cast<FlexSkeletonHeader*>(skeleton->GetRawPointer());
    for (unsigned i = 0; i < 16; ++i) {
        snapshot->limbs[i] = *static_cast<StandardLimb*>(source->sh.segment[i]);
        auto& limb = snapshot->limbs[i];
        if (limb.dList) {
            const char* path = reinterpret_cast<const char*>(limb.dList);
            if (std::strncmp(path, "__OTR__", 7) != 0)
                return nullptr;
            limb.dList = reinterpret_cast<Gfx*>(const_cast<char*>(Route(path + 7)));
        }
        snapshot->pointers[i] = &limb;
        if (custom) {
            char suffix[32];
            std::snprintf(suffix, sizeof(suffix), "/Limbs/Limb%02u", i);
            const std::string path = poseRoot + suffix;
            auto part = load(path);
            if (std::dynamic_pointer_cast<Fast::DisplayList>(part)) {
                snapshot->retained.push_back(part);
                snapshot->result.poseLimbs[i + 1] = Route(path);
            }
        }
    }
    if (pose == 2) {
        std::string path = "objects/object_sa/gSariaRightHandAndOcarinaDL";
        auto prop = load(path);
        if (custom) {
            const std::string privatePath = std::string("alt/") + kNamespace + "/Props/RightHandOcarina";
            auto candidate = load(privatePath);
            if (std::dynamic_pointer_cast<Fast::DisplayList>(candidate) && candidate->GetPointerSize() >= sizeof(Gfx)) {
                path = privatePath;
                prop = candidate;
            }
        }
        if (!std::dynamic_pointer_cast<Fast::DisplayList>(prop) || prop->GetPointerSize() < sizeof(Gfx))
            return nullptr;
        snapshot->retained.push_back(prop);
        // A fitted right-hand limb keeps precedence, matching the native OoT adapter.
        snapshot->limbs[14].dList = reinterpret_cast<Gfx*>(const_cast<char*>(Route(path)));
    }
    snapshot->skeleton = *source;
    snapshot->skeleton.sh.segment = snapshot->pointers.data();
    static const char* eyes[] = { "objects/object_sa/gSariaEyeOpenTex", "objects/object_sa/gSariaEyeHalfTex",
                                  "objects/object_sa/gSariaEyeClosedTex" };
    for (int eye = 0; eye < 3; ++eye) {
        auto texture = load(eyes[eye]);
        if (!MmNormalActor::ValidateTexture(texture))
            return nullptr;
        snapshot->retained.push_back(texture);
        snapshot->result.eyes[eye] = Route(eyes[eye]);
        if (custom) {
            const std::string path =
                std::string("alt/") + kNamespace + "/Faces/L" + std::to_string(eye) + "R" + std::to_string(eye) + "M0";
            auto face = load(path);
            if (std::dynamic_pointer_cast<Fast::DisplayList>(face)) {
                snapshot->retained.push_back(face);
                snapshot->result.heads[eye] = Route(path);
            } else {
                snapshot->result.heads[eye] = Route(head);
            }
        }
    }
    auto mouth = load("objects/object_sa/gSariaMouthClosed2Tex");
    if (!MmNormalActor::ValidateTexture(mouth))
        return nullptr;
    snapshot->retained.push_back(mouth);
    snapshot->result.mouth = Route("objects/object_sa/gSariaMouthClosed2Tex");
    snapshot->result.skeleton = &snapshot->skeleton;
    snapshot->result.animation = animation->GetRawPointer();
    snapshot->result.custom = custom;
    snapshot->result.altEnabled = alt;
    return snapshot;
}
} // namespace PlacedSaria
