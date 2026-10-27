#pragma once
#include "../../soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"

// Enlarge custom sword pickups around the existing frame's center.
// Apply once in caller/GI units, outside the model's binary coordinate fit,
// so the blade, material layers and award-space effects grow together.
static inline void ComboSwordGi_ApplyPresentationSize(bool shop = false, int mmPickup = 0) {
    // Preserve shelves. Goron's close Item0 camera is already tight around
    // Kokiri's shimmer, so preserve that complete model/effect fit too.
    if (shop || mmPickup == 2)
        return;
    constexpr float size = 1.15f;
    const float center = mmPickup ? -14.f : -2.f;
    Matrix_Translate(0.f, (1.f - size) * center, 0.f, MTXMODE_APPLY);
    Matrix_Scale(size, size, size, MTXMODE_APPLY);
}

// Both current and older selected +X sword recipes must stand upright in
// pickups and shelves. Fit the same pose that the geometry and Din layers draw.
static inline float ComboSwordGi_SelectedTilt(float tilt, bool = false) {
    return std::abs(tilt - 1.8f) < .0002f ? 1.5707963267948966f : tilt;
}

// Fit emission paths to the award footprint, but cancel that fit for each
// leaf, spark and ribbon's own dimensions. Binary model units never enter here.
static inline float ComboSwordGi_ParticleScale(NeiGi::Kind kind, bool shop = false, int mmPickup = 0,
                                               bool presentation = true) {
    const auto* bounds = NeiGi::FindSwordFrameBounds(kind);
    const float scale = bounds ? NeiGi::FrameFit(*bounds, 1.f, shop, mmPickup).scale : 1.f;
    const float size = !presentation || shop || mmPickup == 2 ? 1.f : 1.15f;
    return 1.f / (scale * size);
}

// Procedural effects use authored GI units. A binary weapon's coordinate
// conversion belongs only to its geometry and matching Din material layers.
static inline void ComboSwordGi_ApplyEffectFit(NeiGi::Kind kind, bool shop = false, int mmPickup = 0) {
    if (const auto* bounds = NeiGi::FindSwordFrameBounds(kind)) {
        const auto fit = NeiGi::FrameFit(*bounds, 1.f, shop, mmPickup);
        Matrix_Translate(0.f, fit.lift, 0.f, MTXMODE_APPLY);
        Matrix_Scale(fit.scale, fit.scale, fit.scale, MTXMODE_APPLY);
    }
}
