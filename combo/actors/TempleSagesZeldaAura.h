#pragma once

#include "ForestGiParticles.h"
#include <cstring>

namespace TempleSagesZeldaAura {
inline bool Finite(float value) {
    // Preserve the guard under the project's optional fast-math build profile.
    uint32_t bits;
    static_assert(sizeof(bits) == sizeof(value));
    std::memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7F800000u) != 0x7F800000u;
}
inline bool Finite(NeiGi::Point point) {
    return Finite(point.x) && Finite(point.y) && Finite(point.z);
}
inline bool Enabled(int actorId, int params) {
    // Dedicated catalogue variants, never the native Zelda story actors.
    return actorId == 0x002A && (params == 0x7D23 || params == 0x7F34);
}

inline NeiGi::Mesh Sample(int params, uint32_t frame, const NeiGi::Basis& camera = {}) {
    if (!Enabled(0x002A, params))
        return {};
    const bool child = params == 0x7D23;
    const float height = child ? 48.f : 72.f;
    const float time = NeiGi::Time(frame);
    auto mesh = ForestGiParticles::Emit(
        frame, 8, .32f, camera,
        [&](int i, NeiGi::Point) {
            const float phase = ((frame % 180u + i * 23u) % 180u) / 180.f;
            const float angle = i * 2.39996323f + time * .12f;
            const float radius = (child ? 12.f : 18.f) + (i % 3) * 2.f;
            return NeiGi::Point{ std::cos(angle) * radius, 5.f + phase * height, std::sin(angle) * radius };
        },
        NeiGi::Kind::Neutral);
    auto halo = camera;
    halo.up = halo.up * 1.35f;
    const NeiGi::Point center{ 0, height * .5f, 0 };
    const float pulse = .5f + .5f * std::sin(time);
    NeiGi::Glow(mesh, center, child ? 22.f : 29.f, 0x75B9FF, uint8_t(26.f + 12.f * pulse), halo);
    NeiGi::Glow(mesh, center, child ? 12.f : 17.f, 0xD5EEFF, uint8_t(12.f + 8.f * pulse), halo);
    return mesh;
}
} // namespace TempleSagesZeldaAura
