#include "soh/soh/Enhancements/randomizer/NeiGiBottleShimmerPolicy.h"
#include "combo/menu/ComboBottleContents.h"
#include <cassert>
#include <cmath>
#include <cstdio>

static bool sameVertex(const NeiGi::EffectVertex& a, const NeiGi::EffectVertex& b) {
    return a.p.x == b.p.x && a.p.y == b.p.y && a.p.z == b.p.z && a.rgb == b.rgb &&
           a.alpha == b.alpha && a.u == b.u && a.v == b.v;
}

int main() {
    const uint32_t hexes[] = {0xDE64F5u,0x80D846u,0xFFD45Au,0xFFA0EBu,0xFFE66Du};
    for(int profile=1;profile<=5;++profile){
        uint8_t color[4]{};
        assert(ComboBottleShimmer_ColorHex(profile)==hexes[profile-1]);
        assert(ComboBottleShimmer_Color(profile,color) && color[3]==255);
        assert((uint32_t(color[0])<<16|uint32_t(color[1])<<8|color[2])==hexes[profile-1]);
    }
    assert(ComboBottleContents_ShimmerProfile(CW_BOTTLE_SEAHORSE)==CW_SHIMMER_SEAHORSE);
    assert(CW_BOTTLE_SEAHORSE!=int(CW_SHIMMER_SEAHORSE));
    assert(!ComboBottleShimmer_Color(CW_SHIMMER_FAIRY,nullptr));
    assert(!ComboBottleContents_Profile(nullptr));
    assert(!ComboBottleContents_ShimmerProfile(999));

    const int profiles[] = {CW_SHIMMER_PRINCESS, CW_SHIMMER_FAIRY, CW_SHIMMER_SEAHORSE,
                            CW_SHIMMER_GOLD_DUST, CW_SHIMMER_MUSHROOM};
    const float x = 12 * NeiGi::Tau / 360, y = 25 * NeiGi::Tau / 360;
    const NeiGi::Basis cameras[] = { {},
        {{std::cos(y),0,std::sin(y)}, {std::sin(x)*std::sin(y),std::cos(x),-std::sin(x)*std::cos(y)},
         {-std::cos(x)*std::sin(y),std::sin(x),std::cos(x)*std::cos(y)}} };
    for (const auto& camera : cameras) {
        for (uint32_t frame = 0; frame < 720; ++frame) {
            auto base = NeiGi::SampleShimmer(frame, true, camera);
            for (int profile : profiles) {
                const auto mesh = NeiGi::SampleBottleShimmer(profile, frame, true, camera);
                assert(mesh.count == base.count);
                for (size_t i = 0; i < mesh.count; ++i) {
                    assert(mesh.vertices[i].p.x == base.vertices[i].p.x);
                    assert(mesh.vertices[i].p.y == base.vertices[i].p.y);
                    assert(mesh.vertices[i].p.z == base.vertices[i].p.z);
                    assert(mesh.vertices[i].alpha == base.vertices[i].alpha);
                    assert(mesh.vertices[i].rgb == (base.vertices[i].rgb == 0xFFFFFF ? 0xFFFFFF :
                                                   ComboBottleShimmer_ColorHex(profile)));
                }
                assert(NeiGi::SampleBottleShimmer(profile, frame, false, camera).count == 0);
                assert(NeiGi::SampleBottleMotes(profile, frame, false, camera).count == 0);
                const auto motes = NeiGi::SampleBottleMotes(profile, frame, true, camera);
                if (profile != CW_SHIMMER_FAIRY) {
                    assert(motes.count == 0);
                } else {
                    assert(motes.count == 4 * 16 * 3);
                    for (size_t mote = 0; mote < 4; ++mote) {
                        const auto& core = motes.vertices[mote * 48];
                        const auto& edge = motes.vertices[mote * 48 + 1];
                        const auto delta = edge.p - core.p;
                        assert(std::sqrt(delta.x*delta.x + delta.y*delta.y + delta.z*delta.z) >= 1.79f && "pink mote is too small to read beside the fairy");
                        assert(core.alpha >= 96 && "pink mote is too faint behind the bottle glass");
                    }
                    const auto repeated = NeiGi::SampleBottleMotes(profile, frame, true, camera);
                    assert(repeated.count == motes.count);
                    for (size_t i = 0; i < motes.count; ++i) {
                        const auto& v = motes.vertices[i];
                        assert(sameVertex(v, repeated.vertices[i]));
                        assert(std::isfinite(v.p.x) && std::isfinite(v.p.y) && std::isfinite(v.p.z));
                        assert(std::hypot(v.p.x,v.p.z) <= 24);
                        assert(v.p.y >= -26 && v.p.y <= 6);
                        assert(v.alpha <= 224 && v.rgb == 0xFFA0EB);
                    }
                }
            }
            assert(NeiGi::SampleBottleShimmer(0,frame,true,camera).count == 0);
            assert(NeiGi::SampleBottleMotes(999,frame,true,camera).count == 0);
        }
        auto a = NeiGi::SampleBottleShimmer(CW_SHIMMER_PRINCESS, 63, true, camera);
        (void)NeiGi::SampleBottleShimmer(CW_SHIMMER_FAIRY, 700, true, camera);
        auto b = NeiGi::SampleBottleShimmer(CW_SHIMMER_PRINCESS, 63, true, camera);
        assert(a.count == b.count);
        for (size_t i=0;i<a.count;++i) assert(sameVertex(a.vertices[i],b.vertices[i]));
    }
    puts("PASS: unchanged NEI geometry/alpha, identity recolor, deterministic sampling, disabled effects");
    puts("PASS: four readable, bounded fairy-only motes over 720 frames and two cameras");
}
