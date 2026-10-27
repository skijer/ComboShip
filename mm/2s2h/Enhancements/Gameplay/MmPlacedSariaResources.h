#pragma once
#include "actors/PlacedSariaABI.h"
#include "actors/NativeNpcDrawScale.h"
#include "2s2h/resource/type/Array.h"
#include <fast/resource/type/DisplayList.h>
#include <array>
#include <cstdio>
#include <cstring>
#include <set>

#include <functional>
#include <memory>
#include <vector>
#include "2s2h/resource/type/Skeleton.h"
#include "2s2h/resource/type/Animation.h"
#include <fast/resource/type/Texture.h>

namespace MmPlacedSaria {
using Resource = std::shared_ptr<Ship::IResource>;

/* MM's skeleton factory stores borrowed limb pointers only. Resolve and retain
 * each child immediately, requiring identity with the factory pointer before
 * dereferencing it. A stale or replaced cache entry fails closed. */
inline bool ValidateSkeleton(const Resource& resource, unsigned limbs, unsigned matrices,
                             const std::function<Resource(const std::string&)>& load, bool alt,
                             std::vector<Resource>& retained) {
    auto skeleton = std::dynamic_pointer_cast<SOH::Skeleton>(resource);
    if (!skeleton || skeleton->type != SOH::SkeletonType::Flex || skeleton->limbType != SOH::LimbType::Standard ||
        skeleton->limbTableType != SOH::LimbType::Standard || skeleton->limbCount != (int)limbs ||
        skeleton->limbTableCount != (int)limbs || skeleton->dListCount != (int)matrices || limbs == 0 || limbs > 254 ||
        matrices > limbs || skeleton->limbTable.size() != limbs || skeleton->skeletonHeaderSegments.size() != limbs)
        return false;
    const auto& header = skeleton->skeletonData.flexSkeletonHeader;
    if (header.sh.limbCount != limbs || header.sh.skeletonType != (uint8_t)SOH::SkeletonType::Flex ||
        header.dListCount != matrices || header.sh.segment != skeleton->skeletonHeaderSegments.data())
        return false;
    std::vector<Resource> children, limbChildren;
    unsigned actualMatrices = 0;
    for (unsigned i = 0; i < limbs; ++i) {
        std::string path = skeleton->limbTable[i];
        if (path.starts_with("__OTR__"))
            path.erase(0, 7);
        auto matches = [&](const Resource& resource) {
            auto candidate = std::dynamic_pointer_cast<SOH::SkeletonLimb>(resource);
            return candidate && candidate->limbType == SOH::LimbType::Standard &&
                           header.sh.segment[i] == candidate->GetRawPointer()
                       ? candidate
                       : nullptr;
        };
        auto child = matches(load(path));
        // MM's stock skeleton factory resolves nested children with its Alt policy,
        // even when the parent skeleton was requested exactly. Retain only the
        // explicit candidate that owns the factory-selected pointer.
        if (!child && !path.starts_with("alt/"))
            child = matches(load("alt/" + path));
        if (!child)
            return false;
        const auto& limb = child->limbData.standardLimb;
        if ((limb.child != 255 && limb.child >= limbs) || (limb.sibling != 255 && limb.sibling >= limbs))
            return false;
        actualMatrices += limb.dList != nullptr;
        if (limb.dList) {
            // Use the owning string, rather than dereferencing an unchecked Gfx pointer.
            if (child->dListPtr.empty() || reinterpret_cast<const char*>(limb.dList) != child->dListPtr.c_str())
                return false;
            std::string path = child->dListPtr;
            if (path.starts_with("__OTR__"))
                path.erase(0, 7);
            // DL paths are routed through MM's render owner and Alt policy. The
            // private pack may supply only alt/Body entries for a base limb path.
            Resource rendered = alt && !path.starts_with("alt/") ? load("alt/" + path) : nullptr;
            if (!rendered)
                rendered = load(path);
            auto display = std::dynamic_pointer_cast<Fast::DisplayList>(rendered);
            if (!display || display->GetPointerSize() < sizeof(Gfx))
                return false;
            children.push_back(display);
        }
        children.push_back(child);
        limbChildren.push_back(child);
    }
    if (actualMatrices != matrices)
        return false;
    std::vector<bool> visited(limbs, false);
    std::function<bool(unsigned)> walk = [&](unsigned index) {
        if (index == 255)
            return true;
        if (visited[index])
            return false;
        visited[index] = true;
        auto limb = std::static_pointer_cast<SOH::SkeletonLimb>(limbChildren[index]);
        return walk(limb->limbData.standardLimb.child) && walk(limb->limbData.standardLimb.sibling);
    };
    if (!walk(0))
        return false;
    for (bool seen : visited)
        if (!seen)
            return false;
    retained.push_back(resource);
    retained.insert(retained.end(), children.begin(), children.end());
    return true;
}

inline bool ValidateAnimation(const Resource& resource, unsigned limbs, unsigned frames) {
    auto animation = std::dynamic_pointer_cast<SOH::Animation>(resource);
    if (!animation || animation->type != SOH::AnimationType::Normal)
        return false;
    const auto& header = animation->animationData.animationHeader;
    if (!frames || header.common.frameCount != (int)frames || animation->rotationIndices.size() != limbs + 1 ||
        animation->rotationValues.empty() || header.staticIndexMax > animation->rotationValues.size() ||
        (void*)header.frameData != animation->rotationValues.data() ||
        (void*)header.jointIndices != animation->rotationIndices.data())
        return false;
    for (const auto& joint : animation->rotationIndices) {
        for (unsigned index : { joint.x, joint.y, joint.z }) {
            size_t end = index + (index < header.staticIndexMax ? 1 : frames);
            if (end > animation->rotationValues.size())
                return false;
        }
    }
    return true;
}

inline bool ValidateTexture(const Resource& resource) {
    auto texture = std::dynamic_pointer_cast<Fast::Texture>(resource);
    if (!(texture && texture->Type >= Fast::TextureType::RGBA32bpp &&
          texture->Type <= Fast::TextureType::GrayscaleAlpha16bpp && texture->Width && texture->Height &&
          texture->ImageData && texture->ImageDataSize && texture->GetPointer() == texture->ImageData))
        return false;
    if (texture->mImageBuffer) {
        uintptr_t begin = reinterpret_cast<uintptr_t>(texture->mImageBuffer->data());
        uintptr_t pixels = reinterpret_cast<uintptr_t>(texture->ImageData);
        size_t size = texture->mImageBuffer->size();
        if (pixels < begin || pixels - begin > size || texture->ImageDataSize > size - (pixels - begin))
            return false;
    }
    if (texture->Flags == 0) {
        static constexpr unsigned bits[] = { 0, 32, 16, 4, 8, 4, 8, 4, 8, 16 };
        size_t required = ((size_t)texture->Width * texture->Height * bits[(unsigned)texture->Type] + 7) / 8;
        if (texture->ImageDataSize < required)
            return false;
    }
    return true;
}
} // namespace MmPlacedSaria

namespace MmPlacedSaria {
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
    return names.insert("__OTR__@mm:" + path).first->c_str();
}

struct Snapshot {
    std::vector<Resource> retained;
    std::array<SOH::StandardLimb, 16> limbs;
    std::array<void*, 16> pointers;
    SOH::FlexSkeletonHeader skeleton;
    ComboPlacedSariaResources result{};
    float drawScale = 1.f;
};

inline bool SameRoot(const SOH::AnimationHeader& a, const SOH::AnimationHeader& b) {
    for (int frame = 0; frame < a.common.frameCount; ++frame) {
        const u16* ai = reinterpret_cast<const u16*>(a.jointIndices);
        const u16* bi = reinterpret_cast<const u16*>(b.jointIndices);
        for (int axis = 0; axis < 3; ++axis) {
            auto sample = [frame](const SOH::AnimationHeader& h, u16 index) {
                return h.frameData[index + (index < h.staticIndexMax ? 0 : frame)];
            };
            if (sample(a, ai[axis]) != sample(b, bi[axis]))
                return false;
        }
    }
    return true;
}

inline bool SameHierarchy(const Resource& a, const Resource& b) {
    auto* first = static_cast<SOH::FlexSkeletonHeader*>(a->GetRawPointer());
    auto* second = static_cast<SOH::FlexSkeletonHeader*>(b->GetRawPointer());
    for (unsigned i = 0; i < 16; ++i) {
        auto* x = static_cast<SOH::StandardLimb*>(first->sh.segment[i]);
        auto* y = static_cast<SOH::StandardLimb*>(second->sh.segment[i]);
        if (x->child != y->child || x->sibling != y->sibling)
            return false;
    }
    return true;
}

inline std::unique_ptr<Snapshot> Build(const Load& load, bool alt, unsigned pose) {
    pose = pose < std::size(kAnimations) && kAnimations[pose] ? pose : 0;
    auto snapshot = std::make_unique<Snapshot>();
    auto nativeAnimation = load(kAnimations[pose]);
    if (!MmPlacedSaria::ValidateAnimation(nativeAnimation, 16, kAnimationFrames[pose]))
        return nullptr;
    auto animation = nativeAnimation;
    auto skeleton = load("objects/object_sa/gSariaSkel");
    std::vector<Resource> nativeRig;
    if (!MmPlacedSaria::ValidateSkeleton(skeleton, 16, 15, load, alt, nativeRig))
        return nullptr;

    std::vector<Resource> selectedRig, baseRig;
    auto privateRig = alt ? load(std::string("alt/") + kNamespace + "/Skel") : nullptr;
    auto privateBase = privateRig ? load(std::string(kNamespace) + "/Skel") : nullptr;
    const std::string head = std::string("alt/") + kNamespace + "/Body/Limb15";
    auto privateHead = privateBase ? load(head) : nullptr;
    const bool custom = std::dynamic_pointer_cast<Fast::DisplayList>(privateHead) &&
                        privateHead->GetPointerSize() >= sizeof(Gfx) &&
                        MmPlacedSaria::ValidateSkeleton(privateBase, 16, 15, load, alt, baseRig) &&
                        MmPlacedSaria::ValidateSkeleton(privateRig, 16, 15, load, alt, selectedRig) &&
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
    if (MmPlacedSaria::ValidateAnimation(fitted, 16, kAnimationFrames[pose]) &&
        SameRoot(*static_cast<SOH::AnimationHeader*>(fitted->GetRawPointer()),
                 *static_cast<SOH::AnimationHeader*>(nativeAnimation->GetRawPointer())))
        animation = fitted;
    snapshot->retained.push_back(nativeAnimation);
    snapshot->retained.push_back(animation);

    auto* source = static_cast<SOH::FlexSkeletonHeader*>(skeleton->GetRawPointer());
    for (unsigned i = 0; i < 16; ++i) {
        snapshot->limbs[i] = *static_cast<SOH::StandardLimb*>(source->sh.segment[i]);
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
            if (std::dynamic_pointer_cast<Fast::DisplayList>(part) && part->GetPointerSize() >= sizeof(Gfx)) {
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
        if (!MmPlacedSaria::ValidateTexture(texture))
            return nullptr;
        snapshot->retained.push_back(texture);
        snapshot->result.eyes[eye] = Route(eyes[eye]);
        if (custom) {
            const std::string path =
                std::string("alt/") + kNamespace + "/Faces/L" + std::to_string(eye) + "R" + std::to_string(eye) + "M0";
            auto face = load(path);
            if (std::dynamic_pointer_cast<Fast::DisplayList>(face) && face->GetPointerSize() >= sizeof(Gfx)) {
                snapshot->retained.push_back(face);
                snapshot->result.heads[eye] = Route(path);
            } else {
                snapshot->result.heads[eye] = Route(head);
            }
        }
    }
    auto mouth = load("objects/object_sa/gSariaMouthClosed2Tex");
    if (!MmPlacedSaria::ValidateTexture(mouth))
        return nullptr;
    snapshot->retained.push_back(mouth);
    snapshot->result.mouth = Route("objects/object_sa/gSariaMouthClosed2Tex");
    snapshot->result.skeleton = &snapshot->skeleton;
    snapshot->result.animation = animation->GetRawPointer();
    snapshot->result.custom = custom;
    snapshot->result.altEnabled = alt;
    return snapshot;
}
} // namespace MmPlacedSaria

// Local MM resource ownership; callable without initializing any OoT subsystem.
int MM_PlacedSariaLoad(unsigned pose, ComboPlacedSariaResources* output);
int MM_PlacedSariaLoadV2(unsigned pose, ComboPlacedSariaResourcesV2* output, size_t callerSize);
void MM_PlacedSariaRelease(void* owner);
int MM_PlacedSariaAlt();
