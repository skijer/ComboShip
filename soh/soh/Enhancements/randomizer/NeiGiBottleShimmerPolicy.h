#pragma once

#include "ComboBottleShimmer.h"
#include "NeiGiEffectPolicy.h"

namespace NeiGi {
inline Mesh SampleBottleShimmer(int profile, uint32_t frame, bool enabled, const Basis& camera = {}) {
    const uint32_t color = ComboBottleShimmer_ColorHex(profile);
    if (!enabled || !color)
        return {};
    auto mesh = SampleShimmer(frame, true, camera);
    // Same identity recolor used by the existing NeiGi_DrawShimmerOverlay.
    // White centers remain white; all positions, timing and alpha are retained.
    for (size_t i = 0; i < mesh.count; ++i)
        if (mesh.vertices[i].rgb != 0xFFFFFFu)
            mesh.vertices[i].rgb = color;
    return mesh;
}

inline Mesh SampleBottleMotes(int profile, uint32_t frame, bool enabled, const Basis& camera = {}) {
    Mesh mesh;
    if (!enabled || profile != CW_SHIMMER_FAIRY)
        return mesh;
    const float phase = (frame % 360u) * (Tau / 180.f);
    // Follow the accepted native fairy bounce without mutating its skeleton.
    const Point fairy{ 4 * std::sin(phase), -10 + 7 * std::sin(phase * 1.5f + .4f), 2 * std::cos(phase) };
    for (int i = 0; i < 4; ++i) {
        const float a = (frame % 360u) * (Tau / 360.f) + i * Tau / 4;
        const float radius = 12.0f + i;
        const Point center = fairy + Point{ radius * std::cos(a), 5 * std::sin(a * 2 + i), radius * std::sin(a) };
        const float pulse = .5f - .5f * std::cos(a * 2 + i);
        Glow(mesh, center, 1.8f + 1.2f * pulse, 0xFFA0EBu, uint8_t(96 + 128 * pulse), camera);
    }
    return mesh;
}
} // namespace NeiGi
