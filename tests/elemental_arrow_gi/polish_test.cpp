#include "soh/Enhancements/randomizer/NeiElementalArrowGi.h"
#include <cassert>
#include <cmath>
#include <iostream>
#if __has_include("soh/Enhancements/randomizer/NeiElementalSpellGi.h")
#include "soh/Enhancements/randomizer/NeiElementalSpellGi.h"
#endif

int main() {
    // The real native arrow in both supplied archives ends at (-15,-18,0).
    // An emitter around the shaft's center/top fails this attachment check.
    for (int profile=1; profile<=3; ++profile) for (unsigned f=0; f<360; ++f) {
        const auto layers=NeiArrowGi::Sample(profile,f);
        for (const auto* mesh:{&layers.veil,&layers.energy}) {
            assert(mesh->count>0);
            for (size_t i=0;i<mesh->count;++i) {
                const auto& p=mesh->vertices[i].p;
                assert(p.x>=-28 && p.x<=13 && p.y>=-32 && p.y<=16 && std::abs(p.z)<=20 &&
                       "elemental emission must stay at the lower native arrow tip, inside the vanilla envelope");
            }
        }
    }
#if __has_include("soh/Enhancements/randomizer/NeiElementalSpellGi.h")
    for (int profile=1;profile<=3;++profile) for (unsigned f=0;f<360;++f) {
        const auto layers=NeiSpellGi::Sample(profile,f);
        assert(layers.back.count && layers.front.count && layers.veil.count && layers.energy.count && layers.shimmer.count);
        for (const auto* mesh:{&layers.back,&layers.front,&layers.veil,&layers.energy})
            for(size_t i=0;i<mesh->count;++i) {
                const auto& p=mesh->vertices[i].p;
                assert(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z));
                assert(std::abs(p.x)<=22 && std::abs(p.z)<=22 && std::abs(p.y)<=37);
                if(mesh==&layers.veil || mesh==&layers.energy)
                    assert((std::abs(p.x)+std::abs(p.z))/22+std::abs(p.y)/37<=.96f &&
                           "spell energy must remain inside the faceted crystal, including animated wisps");
            }
    }
#else
    assert(false && "the three volatile crystal spell GIs are missing");
#endif
    std::cout<<"PASS tip-local arrow emissions and native-size crystal containment over all animation frames\n";
}
