#pragma once

#include "../../soh/soh/Enhancements/randomizer/NeiUsedMagicPolicy.h"

namespace ComboMagicMirrorCast {
// A single transparent botanical seal below the retained green ritual. Private
// texture failure simply leaves the existing geometry-only ritual visible.
inline NeiGi::Mesh SampleSeal(float scale, float alpha) {
    NeiGi::Mesh mesh;
    scale = NeiUsedMagic::Clamp(scale, 0, 1.25f);
    alpha = NeiUsedMagic::Clamp(alpha, 0, 255);
    if (scale <= 0 || alpha <= 0)
        return mesh;
    const float radius = 82 * scale, y = 1.5f * scale;
    const auto opacity = NeiUsedMagic::Alpha(alpha * .72f);
    constexpr uint32_t green = 0x66FF88;
    const NeiGi::EffectVertex a{ { -radius, y, -radius }, green, opacity, 0, 0 };
    const NeiGi::EffectVertex b{ { radius, y, -radius }, green, opacity, 1, 0 };
    const NeiGi::EffectVertex c{ { radius, y, radius }, green, opacity, 1, 1 };
    const NeiGi::EffectVertex d{ { -radius, y, radius }, green, opacity, 0, 1 };
    mesh.Tri(a, b, c);
    mesh.Tri(a, c, d);
    return mesh;
}
} // namespace ComboMagicMirrorCast
