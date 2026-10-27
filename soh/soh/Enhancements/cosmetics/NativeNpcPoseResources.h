#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "actors/NativeNpcDrawScale.h"

namespace SOH::NativeNpcPose {
inline std::string_view Canonical(std::string_view path) {
    if (path.starts_with("__OTR__")) {
        path.remove_prefix(7);
    }
    return path;
}

inline std::string DrawScalePath(std::string_view skeleton) {
    skeleton = Canonical(skeleton);
    if (skeleton != "objects/object_sa/NativeNpcV1/Skel" && skeleton != "objects/object_ma1/NativeNpcV1/Skel" &&
        skeleton != "objects/object_fa/YoungFadoNpcPOC1/Skel")
        return {};
    return std::string(skeleton.substr(0, skeleton.size() - 5)) + "/DrawScale";
}

inline std::string AnimationPath(std::string_view skeleton, std::string_view animation) {
    skeleton = Canonical(skeleton);
    animation = Canonical(animation);
    if (skeleton == "objects/object_fa/YoungFadoNpcPOC1/Skel" &&
        animation == "objects/object_fa/YoungFadoNpcPOC1/Animations/Seated5") {
        animation = "objects/object_os_anime/gKokiriSittingCrossedArmsLegsAnim";
    }
    constexpr std::string_view skeletons[] = {
        "objects/object_sa/NativeNpcV1/Skel",      "objects/object_kw1/NativeNpcV1/Skel",
        "objects/object_ma1/NativeNpcV1/Skel",     "objects/object_ma2/NativeNpcV1/Skel",
        "objects/object_fa/YoungFadoNpcPOC1/Skel",
    };
    if (!animation.starts_with("objects/") || animation.find("..") != std::string_view::npos) {
        return {};
    }
    for (auto candidate : skeletons) {
        if (skeleton == candidate) {
            return std::string(skeleton.substr(0, skeleton.size() - 5)) + "/PoseAnimations/" + std::string(animation);
        }
    }
    return {};
}

/* Normal-animation resources retain native timing, joint order and root motion.
 * Full fitted rotations then use the engine's ordinary interpolation/morphing. */
template <typename Clip> bool Validate(const Clip& fitted, const Clip& native, unsigned joints) {
    const auto& header = fitted.animationData.animationHeader;
    const auto frames = header.common.frameCount;
    if (frames <= 0 || frames != native.animationData.animationHeader.common.frameCount || joints == 0 ||
        fitted.rotationIndices.size() != joints || native.rotationIndices.size() != joints ||
        header.staticIndexMax > fitted.rotationValues.size()) {
        return false;
    }
    const auto valid = [](const Clip& clip) {
        const auto& h = clip.animationData.animationHeader;
        if (h.staticIndexMax > clip.rotationValues.size()) {
            return false;
        }
        for (const auto& row : clip.rotationIndices) {
            for (auto index : { row.x, row.y, row.z }) {
                size_t last = index + (index < h.staticIndexMax ? 0 : h.common.frameCount - 1);
                if (last >= clip.rotationValues.size()) {
                    return false;
                }
            }
        }
        return true;
    };
    if (!valid(fitted) || !valid(native)) {
        return false;
    }
    const auto root = [](const Clip& clip, int frame, int axis) {
        const auto& row = clip.rotationIndices[0];
        uint16_t index = axis == 0 ? row.x : axis == 1 ? row.y : row.z;
        return clip.rotationValues[index + (index < clip.animationData.animationHeader.staticIndexMax ? 0 : frame)];
    };
    for (int frame = 0; frame < frames; ++frame) {
        for (int axis = 0; axis < 3; ++axis) {
            if (root(fitted, frame, axis) != root(native, frame, axis)) {
                return false;
            }
        }
    }
    return true;
}
} // namespace SOH::NativeNpcPose
