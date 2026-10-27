// Export the exact GI sampler at the same position/UV precision as native GBI.
#include "soh/Enhancements/randomizer/NeiElementalSpellGi.h"
#include <fstream>
#include <iostream>
#include "soh/Enhancements/randomizer/NeiGiEnergyTexture.h"

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ofstream out(argv[1], std::ios::binary);
    const uint32_t frames = 240;
    out.write(reinterpret_cast<const char*>(&frames), 4);
    const uint32_t items = 9;
    out.write(reinterpret_cast<const char*>(&items), 4);
    const float x = 12 * NeiGi::Tau / 360, y = 25 * NeiGi::Tau / 360;
    const NeiGi::Basis camera{
        {std::cos(y), 0, std::sin(y)},
        {std::sin(x) * std::sin(y), std::cos(x), -std::sin(x) * std::cos(y)},
        {-std::cos(x) * std::sin(y), std::sin(x), std::cos(x) * std::cos(y)}};
    auto mesh = [&](const NeiGi::Mesh& m, int tile) {
        const uint32_t count = m.count;
        out.write(reinterpret_cast<const char*>(&count), 4);
        for (size_t i = 0; i < m.count; ++i) {
            const auto& v = m.vertices[i];
            const float p[] = {std::round(v.p.x * 16) / 16, std::round(v.p.y * 16) / 16,
                               std::round(v.p.z * 16) / 16};
            const uint8_t rgba[] = {uint8_t(v.rgb >> 16), uint8_t(v.rgb >> 8), uint8_t(v.rgb), v.alpha};
            auto uv = [tile](float value) {
                const int packed = std::lround(value * tile * 32);
                return (((packed * 65535) >> 16) / 32.f + .5f) / (tile == 63 ? 64 : 32);
            };
            const float coords[] = {tile ? uv(v.u) : 0, tile ? uv(v.v) : 0};
            out.write(reinterpret_cast<const char*>(p), sizeof(p));
            out.write(reinterpret_cast<const char*>(rgba), sizeof(rgba));
            out.write(reinterpret_cast<const char*>(coords), sizeof(coords));
        }
    };
    for (uint32_t tick = 0; tick < frames; ++tick) {
        const uint32_t frame=tick*3;
        for (int mode=0;mode<3;++mode) for (int item=0;item<int(items);++item) {
            const bool spell=item>=3;
            const int profile=item%3+1;
            if(item>=6) {
                const NeiGi::Kind kinds[]={NeiGi::Kind::Hylia,NeiGi::Kind::Zonai,NeiGi::Kind::Demise};
                const auto kind=kinds[item-6];
                const auto colors=NeiGi::OrbPalette(kind);
                out.write(reinterpret_cast<const char*>(&colors.hot),4);
                out.write(reinterpret_cast<const char*>(&colors.edge),4);
                mesh(NeiGi::SampleOrb(kind,camera),63);
                mesh(NeiGi::SampleEnergy(kind,frame,camera),0);
                mesh(NeiGi::SampleShimmer(frame,true,camera,kind),0);
                mesh(NeiGi::SampleCrystalSheen(frame,camera),0);
                continue;
            }
            auto palette=NeiElementalGi::DefaultPalette(profile,spell);
            if(mode==2) palette={0xA044E2,0x20CC78,0x20CC78};
            out.write(reinterpret_cast<const char*>(&palette.hot),4);
            out.write(reinterpret_cast<const char*>(&palette.edge),4);
            if(!spell) {
                const auto layers=NeiArrowGi::Sample(profile,frame,camera,&palette,mode==1);
                mesh(layers.veil,32);mesh(layers.energy,0);mesh(layers.shimmer,0);
            } else {
                const auto layers=NeiSpellGi::Sample(profile,frame,camera,&palette,mode==1);
                mesh(layers.back,0);mesh(layers.veil,63);mesh(layers.energy,0);
                mesh(layers.front,0);mesh(layers.shimmer,0);
            }
        }
        for(int profile=1;profile<=3;++profile) {
            const auto kind=profile==1?NeiGi::Kind::Fire:profile==2?NeiGi::Kind::Hylia:NeiGi::Kind::Ice;
            const auto& texture=NeiGi::OrbTexture(kind,frame);
            out.write(reinterpret_cast<const char*>(texture.data()),texture.size());
        }
        for(const auto kind:{NeiGi::Kind::Hylia,NeiGi::Kind::Zonai,NeiGi::Kind::Demise}) {
            const auto& texture=NeiGi::OrbTexture(kind,frame);
            out.write(reinterpret_cast<const char*>(texture.data()),texture.size());
        }
    }
    if (!out) return 1;
    std::cout << "Exported six elemental GIs and three existing NEI crystals, including production facet sheen\n";
}
