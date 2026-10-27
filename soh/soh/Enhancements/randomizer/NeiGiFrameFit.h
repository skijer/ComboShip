#pragma once
#include "NeiGiEffectPolicy.h"
#include "NeiGiShopFit.h"
#include <algorithm>
#include <cstring>

namespace NeiGi {
struct FrameBounds {
    const char* slug;
    Point minimum, maximum; // after the resource Mtx, before the host draw scale
    float spinningWidth;
    Kind effect;
    ShopFit preferredShop;
};
inline constexpr FrameBounds kFrameBounds[] = {
#include "NeiGiFrameBounds.inc"
};

inline const FrameBounds* FindFrameBounds(const char* path) {
    if (!path || std::strncmp(path, "__OTR__", 7))
        return nullptr;
    path += 7;
    if (!std::strncmp(path, "@oot-gi-base:", 13))
        path += 13;
    else if (!std::strncmp(path, "@oot:", 5))
        path += 5;
    else if (!std::strncmp(path, "@mm:", 4))
        path += 4;
    constexpr char prefix[] = "objects/nei_gi_redesign/";
    if (std::strncmp(path, prefix, sizeof(prefix) - 1))
        return nullptr;
    path += sizeof(prefix) - 1;
    for (const auto& bounds : kFrameBounds) {
        const size_t length = std::strlen(bounds.slug);
        if (!std::strncmp(path, bounds.slug, length) && !std::strcmp(path + length, "/gi_dl"))
            return &bounds;
    }
    return nullptr;
}

inline const FrameBounds* FindSwordFrameBounds(Kind kind) {
    if (IsSword(kind))
        for (const auto& bounds : kFrameBounds)
            if (bounds.effect == kind)
                return &bounds;
    return nullptr;
}

// The caller supplies its world/overhead/actor matrix. One correction encloses
// model, local energy and crystal shell, preserving their relative positions.
// mmPickup is 0 for other draws, 1 for ordinary MM receipts, 2 for Goron.
inline ShopFit FrameFit(const FrameBounds& bounds, float drawScale, bool shop, int mmPickup = 0) {
    ShopFit fit = shop ? bounds.preferredShop : ShopFit{ 1.f, PresentationOffsetY(bounds.effect) };
    const float low = bounds.minimum.y * drawScale, high = bounds.maximum.y * drawScale;
    const float height = high - low, width = bounds.spinningWidth * drawScale;
    const bool feather = std::strcmp(bounds.slug, "rocs_feather") == 0;
    // MM receipts use a CustomItem at human Y=51.3 with a .21 caller scale.
    // Its closeup camera sees the near side of a spinning model above the
    // local-Y tip. Leave room for that perspective rise and the upper border.
    // Goron's Item0 camera pitches 55 degrees toward a target 12 units ahead
    // of the player. Its near-side vertices need a narrower, higher envelope.
    const bool goron = mmPickup == 2;
    const float bottom = shop ? -22.f : goron ? 36.f : mmPickup ? -44.f : -52.f;
    const float top = shop ? (feather ? 62.f : 52.f) : goron ? 90.f : mmPickup ? 16.f : 48.f;
    fit.scale = std::min(fit.scale, (top - bottom) / height);
    fit.scale = std::min(fit.scale, (shop ? 76.f : goron ? 36.f : mmPickup ? 64.f : 104.f) / width);
    fit.lift = std::clamp(fit.lift, bottom - low * fit.scale, top - high * fit.scale);
    return fit;
}
} // namespace NeiGi
