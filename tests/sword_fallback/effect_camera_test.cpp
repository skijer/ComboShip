// Project production sword particle/shimmer vertices through all five Item0 cameras.
// Includes signed 1/16 packing, billboard axes, arbitrary caller yaw and receipt lift.
// Existing Kokiri shimmer clipping is reported separately from new regressions.
#include <cassert>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>

constexpr int MTXMODE_APPLY = 1;
struct Pose { float scale = 1.f, lift = 0.f; } pose;
void Matrix_Translate(float x, float y, float z, int) {
    assert(x == 0.f && z == 0.f);
    pose.lift += pose.scale * y;
}
void Matrix_Scale(float x, float y, float z, int) {
    assert(x == y && y == z);
    pose.scale *= x;
}
#include "combo/menu/ComboSwordGiEffectFit.h"

using namespace NeiGi;
struct Camera {
    const char* label;
    int context;
    float baseReceiptHeight, pitch, at, distance, fov, forward;
    float Origin() const { return baseReceiptHeight + 2.f + .007f * 900.f; }
};
constexpr Camera cameras[] = {
    { "Human", 1, 45.f, 25.f, 46.8f, 39.2f, 45.f, 0.f },
    { "Goron", 2, 90.f, 55.f, 94.8f, 33.3f, 55.f, 12.f },
    { "Zora", 1, 75.f, 30.f, 74.8f, 47.6f, 42.f, 4.f },
    { "Deku", 1, 35.f, -8.f, 28.2f, 46.8f, 60.f, 0.f },
    { "Fierce", 1, 100.f, 40.f, 95.2f, 33.6f, 80.f, 6.f },
};
struct Sword { const char* label; Kind kind; };
constexpr Sword swords[] = {
    { "Four", Kind::FourSword }, { "Razor", Kind::RazorSword },
    { "Gilded", Kind::GildedSword }, { "Kokiri", Kind::KokiriSword },
    { "MmKokiri", Kind::MmKokiriSword }, { "Master", Kind::MasterSword },
    { "TrueMaster", Kind::SwordAura }, { "Biggoron", Kind::BiggoronSword },
    { "GiantsKnife", Kind::GiantsKnife },
    { "GreatFairy", Kind::GreatFairySword },
};
struct Witness {
    float ndc = 0.f;
    unsigned frame = 0;
    int yaw = 0;
    size_t vertex = 0;
    Point local{};
    uint8_t alpha = 0;
};
struct Result {
    Witness minY, maxY, minX, maxX;
    float scale = 0.f, lift = 0.f;
    bool Clips() const {
        return minY.ndc <= -1.f || maxY.ndc >= 1.f || minX.ndc <= -1.f || maxX.ndc >= 1.f;
    }
};
Result Check(Kind kind, const Camera& view, bool enlarge, bool sweep) {
    pose = { .007f * 30.f, view.Origin() };
    if (enlarge)
        ComboSwordGi_ApplyPresentationSize(false, view.context);
    ComboSwordGi_ApplyEffectFit(kind, false, view.context);
    const float scale = pose.scale, lift = pose.lift;
    const float pitch = view.pitch * Tau / 360.f;
    const float sp = std::sin(pitch), cp = std::cos(pitch);
    const float tanFov = std::tan(view.fov * Tau / 720.f);
    Result result{};
    result.scale = scale;
    result.lift = lift;
    // Check every particle age and shimmer phase over the shared 360-frame cycle.
    // The caller can have any relative yaw: CustomItem keeps spinning while
    // camera yaw follows player orientation. Shared external/legacy FX do not
    // require the geometry drawer's additional local spin for this witness.
    for (unsigned frame = 0; frame < 360; ++frame)
        for (int yaw = 0; yaw < (sweep ? 360 : 1); yaw += 5) {
            const float angle = yaw * Tau / 360.f;
            const float sy = std::sin(angle), cy = std::cos(angle);
            // The production CameraBasis is the billboard axes transformed
            // into the uniform caller matrix's local coordinates.
            const Basis camera{ { cy, 0, sy }, { sp * sy, cp, -sp * cy }, { -cp * sy, sp, cp * cy } };
            for (const auto mesh : {SampleShimmer(frame, true, camera, kind),
                                   SampleSword(kind, frame, camera,
                                               ComboSwordGi_ParticleScale(kind, false, view.context, enlarge))})
            for (size_t i = 0; i < mesh.count; ++i) {
                const auto& v = mesh.vertices[i];
                if (!v.alpha)
                    continue;
                const Point packed{ std::lround(v.p.x * 16.f) / 16.f,
                                    std::lround(v.p.y * 16.f) / 16.f,
                                    std::lround(v.p.z * 16.f) / 16.f };
                const float z = view.forward + scale * (-packed.x * sy + packed.z * cy);
                const float dy = lift + scale * packed.y - view.at;
                const float depth = view.distance - dy * sp - z * cp;
                assert(depth > 0.f);
                const float screenY = (dy * cp - z * sp) / (depth * tanFov);
                const float screenX = scale * (packed.x * cy + packed.z * sy) / (depth * tanFov * (4.f / 3.f));
                const Witness y{ screenY, frame, yaw, i, packed, v.alpha };
                const Witness x{ screenX, frame, yaw, i, packed, v.alpha };
                if (screenY < result.minY.ndc) result.minY = y;
                if (screenY > result.maxY.ndc) result.maxY = y;
                if (screenX < result.minX.ndc) result.minX = x;
                if (screenX > result.maxX.ndc) result.maxX = x;
            }
        }
    return result;
}
void Write(const char* label, const Result& result) {
    std::cout << "  " << label << " Y=[" << result.minY.ndc << "," << result.maxY.ndc
              << "] X=[" << result.minX.ndc << "," << result.maxX.ndc
              << "] scale=" << result.scale << " lift=" << result.lift << '\n';
    for (const auto& witness : { result.minY, result.maxY })
        std::cout << "    Y=" << witness.ndc << " frame=" << witness.frame << " yaw=" << witness.yaw
                  << " vertex=" << witness.vertex << " alpha=" << int(witness.alpha)
                  << " local=(" << witness.local.x << "," << witness.local.y << "," << witness.local.z << ")\n";
}
int main(int argc, char** argv) {
    const bool all = true;
    (void)argc; (void)argv;
    bool newClip = false;
    std::cout << std::fixed << std::setprecision(7);
    for (const auto& view : cameras) {
        if (!all && view.context != 2) continue;
        for (size_t i = 0; i < (all ? std::size(swords) : 3); ++i)
            for (bool sweep : { false, true }) {
                const auto before = Check(swords[i].kind, view, false, sweep);
                const auto enlarged = Check(swords[i].kind, view, true, sweep);
                const bool worsensExisting=before.Clips() &&
                    (enlarged.minY.ndc<before.minY.ndc-.000001f || enlarged.maxY.ndc>before.maxY.ndc+.000001f ||
                     enlarged.minX.ndc<before.minX.ndc-.000001f || enlarged.maxX.ndc>before.maxX.ndc+.000001f);
                if ((!before.Clips() && enlarged.Clips()) || worsensExisting) {
                    std::cout << view.label << ' ' << swords[i].label << " NEW OR WORSENED CLIPPING sweep=" << sweep << '\n';
                    newClip = true;
                    Write("before", before);
                    Write("enlarged", enlarged);
                } else if (before.Clips()) {
                    std::cout << view.label << ' ' << swords[i].label << " existing shimmer clipping sweep=" << sweep << '\n';
                }
            }
    }
    if (!newClip) std::cout << "PASS all five MM receipt cameras: sword particles/shimmer add no new or worsened clipping\n";
    return newClip ? 1 : 0;
}
