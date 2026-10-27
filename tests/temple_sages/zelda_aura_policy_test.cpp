#include <cstdio>
#if !__has_include("actors/TempleSagesZeldaAura.h")
int main() { std::fputs("FAIL actor-scoped Zelda aura is missing\n", stderr); return 1; }
#else
#include "actors/TempleSagesZeldaAura.h"
#include <cassert>
#include <cmath>
#include <limits>

bool Equal(const NeiGi::Mesh& a, const NeiGi::Mesh& b) {
    if (a.count != b.count) return false;
    for (size_t i = 0; i < a.count; ++i) {
        const auto& x = a.vertices[i]; const auto& y = b.vertices[i];
        if (x.p.x != y.p.x || x.p.y != y.p.y || x.p.z != y.p.z || x.rgb != y.rgb || x.alpha != y.alpha) return false;
    }
    return true;
}
int main() {
    using namespace TempleSagesZeldaAura;
    assert(Enabled(0x2A, 0x7D23) && Enabled(0x2A, 0x7F34));
    for (int id = 0; id < 512; ++id)
        if (id != 0x2A) assert(!Enabled(id, 0x7D23) && !Enabled(id, 0x7F34));
    for (int params : {0, 2, 0x7D03, 0x7D13, 0x7F04, 0x7F14, 0x7F24, 0x7F44, 0x7D33, 0x7F13}) {
        assert(!Enabled(0x2A, params));
        assert(Sample(params, 0).count == 0);
    }
    unsigned enabled = 0;
    for (int params = 0; params <= 0xFFFF; ++params) enabled += Enabled(0x2A, params);
    assert(enabled == 2);
    for (int params : {0x7D23, 0x7F34}) {
        for (uint32_t frame : {0u, 1u, 30u, 60u, 179u, 180u, 359u, 1000000u}) {
            const auto mesh = Sample(params, frame);
            assert(mesh.count > 0 && mesh.count <= 720 && mesh.count % 3 == 0);
            bool blue = false, white = false, feather = false;
            for (size_t i = 0; i < mesh.count; ++i) {
                const auto& v = mesh.vertices[i];
                assert(std::isfinite(v.p.x) && std::isfinite(v.p.y) && std::isfinite(v.p.z));
                assert(std::fabs(v.p.x) < 45 && std::fabs(v.p.z) < 45 && v.p.y > -10 && v.p.y < 100);
                const unsigned r = v.rgb >> 16, g = (v.rgb >> 8) & 255, b = v.rgb & 255;
                assert(b >= g && g >= r);
                blue |= b > r + 20; white |= v.rgb == 0xFFFFFF; feather |= v.alpha == 0;
            }
            assert(blue && white && feather);
            Sample(params == 0x7D23 ? 0x7F34 : 0x7D23, frame + 5);
            assert(Equal(mesh, Sample(params, frame)));
        }
        assert(!Equal(Sample(params, 0), Sample(params, 30)));
    }
    puts("PASS Zelda aura policy: only two opted-in cutscene variants, bounded blue NEI halo/twinkles, frame-derived independent state");
}
#endif
