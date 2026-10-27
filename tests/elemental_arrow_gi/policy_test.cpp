#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#if __has_include("soh/Enhancements/randomizer/NeiElementalArrowGi.h")
#include "soh/Enhancements/randomizer/NeiElementalArrowGi.h"
using namespace NeiArrowGi;
static uint64_t fingerprint(const NeiGi::Mesh& mesh) {
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < mesh.count; ++i) {
        const auto& v = mesh.vertices[i];
        for (float value : {v.p.x, v.p.y, v.p.z, v.u, v.v}) {
            uint32_t bits; std::memcpy(&bits, &value, 4);
            hash = (hash ^ bits) * 1099511628211ULL;
        }
        hash = (hash ^ v.rgb ^ v.alpha) * 1099511628211ULL;
    }
    return hash;
}
int main() {
    // Catches an oversized emission, a saturated RSP mesh, frozen motion,
    // non-finite vertices, and accidentally sharing one shape for all elements.
    uint64_t shapes[3]{};
    const NeiGi::Basis views[] = {{}, {{0,0,-1},{0,1,0},{1,0,0}}};
    for (int profile = 1; profile <= 3; ++profile) {
        size_t visible = 0;
        for (const auto& camera : views) for (uint32_t frame = 0; frame < 360; ++frame) {
            const auto layers = Sample(profile, frame, camera);
            for (const auto* mesh : {&layers.veil, &layers.energy, &layers.shimmer}) {
                assert(mesh->count > 0 && mesh->count <= 1536 && mesh->count % 3 == 0);
                for (size_t i = 0; i < mesh->count; ++i) {
                    const auto& v = mesh->vertices[i];
                    assert(std::isfinite(v.p.x) && std::isfinite(v.p.y) && std::isfinite(v.p.z));
                    // Measured native diagonal core plus the old tip envelope.
                    assert(v.p.x >= -28 && v.p.x <= 33 && std::fabs(v.p.z) <= 21);
                    assert(v.p.y >= -32 && v.p.y <= 38);
                    assert(std::isfinite(v.u) && std::isfinite(v.v));
                    assert(v.u >= 0 && v.u <= 1 && v.v >= 0 && v.v <= 1);
                    visible += v.alpha > 0;
                }
            }
        }
        assert(visible > 0);
        const auto first = Sample(profile, 0);
        const auto later = Sample(profile, 47);
        shapes[profile - 1] = fingerprint(first.energy);
        assert(fingerprint(first.energy) != fingerprint(later.energy));
        assert(fingerprint(first.energy) == fingerprint(Sample(profile, 360).energy));
        assert(Sample(profile, UINT32_MAX).energy.count > 0);
    }
    assert(shapes[0] != shapes[1] && shapes[1] != shapes[2] && shapes[0] != shapes[2]);
    for (int invalid : {-1, 0, 4, 100}) {
        const auto layers = Sample(invalid, 0);
        assert(!layers.veil.count && !layers.energy.count && !layers.shimmer.count);
    }
    std::cout << "PASS elemental arrows: distinct animated shapes, compact framing, valid UVs and bounded meshes\n";
}
#else
int main() { assert(false && "Elemental arrow GI layers have not been implemented"); }
#endif
