#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    using namespace NeiGi;
    // Append the new palette without renumbering any existing cross-game identity.
    const Kind chateau = static_cast<Kind>(static_cast<int>(Kind::IronKnuckleAxe) + 1);
    assert(static_cast<int>(Kind::Last) == static_cast<int>(chateau));
    assert(ColorHex(chateau) == 0x4A0C41); // Dominant background pixel in the supplied 3DS label.
    assert(!IsEquipment(chateau) && !IsSword(chateau) && !IsSpell(chateau));
    assert(SampleShimmer(0, false, {}, chateau).count == 0);
    for (unsigned frame = 0; frame < 360; ++frame) {
        const auto a = SampleShimmer(frame, true, {}, chateau);
        const auto b = SampleShimmer(frame, true, {}, chateau);
        assert(a.count == 360 && a.count == b.count);
        unsigned gold = 0, burgundy = 0;
        for (size_t i = 0; i < a.count; ++i) {
            const auto& v = a.vertices[i];
            gold += v.rgb == 0xFFD45A;
            burgundy += v.rgb == 0x4A0C41;
            assert(v.rgb == 0xFFD45A || v.rgb == 0x4A0C41 || v.rgb == 0xFFFFFF);
            assert(std::isfinite(v.p.x) && std::isfinite(v.p.y) && std::isfinite(v.p.z));
            assert(std::abs(v.p.y) < 38 && std::hypot(v.p.x, v.p.z) < 38);
            assert(v.rgb == b.vertices[i].rgb && v.alpha == b.vertices[i].alpha);
            assert(v.p.x == b.vertices[i].p.x && v.p.y == b.vertices[i].p.y && v.p.z == b.vertices[i].p.z);
        }
        assert(gold && burgundy); // Both hues are present in every frame.
    }
    puts("PASS Chateau gold/#4A0C41 shimmer: full cycle, finite bounds, deterministic, disabled control");
}
