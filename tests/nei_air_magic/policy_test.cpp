#include "soh/Enhancements/randomizer/NeiAirMagicPolicy.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

using namespace NeiAirMagic;
static void check(const Layers& layers, float bound, unsigned maxVertices) {
    size_t total = 0;
    bool soft = false;
    for (const auto* mesh : {&layers.surface, &layers.detail}) {
        assert(mesh->count % 3 == 0 && mesh->count < mesh->vertices.size());
        total += mesh->count;
        for (size_t i = 0; i < mesh->count; ++i) {
            const auto& v = mesh->vertices[i];
            assert(std::isfinite(v.p.x) && std::isfinite(v.p.y) && std::isfinite(v.p.z));
            assert(std::abs(v.p.x) <= bound && std::abs(v.p.y) <= bound && std::abs(v.p.z) <= bound);
            assert(std::isfinite(v.u) && std::isfinite(v.v));
            assert(std::abs(v.u * 1024) < 32767 && std::abs(v.v * 1024) < 32767);
            soft |= v.alpha > 0 && v.alpha < 100;
        }
    }
    assert(total > 0 && total <= maxVertices && soft);
}
int main() {
    const Basis cameras[] = {{}, {{0,0,1},{0,1,0},{-1,0,0}}, {{1,0,0},{0,0,1},{0,1,0}}};
    for (const auto& camera : cameras) for (unsigned frame = 0; frame < 180; ++frame) {
        for (Point direction : {Point{1,0,0}, Point{0,1,0}, Point{0,0,1}}) {
            check(SampleLightning(frame, direction, camera), 100, 1800);
            for (bool blow : {false,true})
                check(SampleGust(frame, direction, 220, 70, blow, 0xDFE6EF, camera), 235, 1900);
        }
        for (float height : {44.f,68.f,100.f})
            check(SampleEnvelope(frame, height, {8,0,0}, camera), 140, 1900);
    }
    auto off = SampleEnvelope(0, 0, {});
    assert(off.surface.count == 0 && off.detail.count == 0);
    off = SampleGust(0, {0,0,1}, 0, 70, false, 0xFFFFFF);
    assert(off.surface.count == 0 && off.detail.count == 0);
    off = SampleLightning(0, {std::numeric_limits<float>::quiet_NaN(),0,0});
    assert(off.surface.count == 0 && off.detail.count == 0);
    off = SampleEnvelope(0, std::numeric_limits<float>::quiet_NaN(), {});
    assert(off.surface.count == 0 && off.detail.count == 0);
    const auto envelope = SampleEnvelope(30, 68, {});
    float low=1000, high=-1000, left=1000, right=-1000;
    for(size_t i=0;i<envelope.surface.count;++i) {
        const auto& v=envelope.surface.vertices[i];
        low=std::min(low,v.p.y);high=std::max(high,v.p.y);
        left=std::min(left,v.p.x);right=std::max(right,v.p.x);
        assert(v.rgb == 0xDDE5EF || v.rgb == 0xF1F5F9);
    }
    assert(low<12 && high>50 && left<-12 && right>12);
    std::cout << "PASS air magic: finite bounded geometry, vertical/horizontal aim, UV limits, soft alpha, body envelope and invalid-input rejection\n";
}
