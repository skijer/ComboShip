#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#if __has_include("soh/Enhancements/randomizer/NeiElementalGiColors.h")
#include "soh/Enhancements/randomizer/NeiElementalGiColors.h"
int main() {
    const char* oot[][2]={{"Arrows.FirePrimary","Arrows.FireSecondary"},{"Arrows.IcePrimary","Arrows.IceSecondary"},
                         {"Arrows.LightPrimary","Arrows.LightSecondary"}};
    const char* mm[][2]={{"Effects.FireArrowPrim","Effects.FireArrowSec"},{"Effects.IceArrowPrim","Effects.IceArrowSec"},
                        {"Effects.LightArrowPrim","Effects.LightArrowSec"}};
    const char* spells[][2]={{"Magic.DinsPrimary","Magic.DinsSecondary"},{"Magic.FaroresPrimary","Magic.FaroresSecondary"},
                            {"Magic.NayrusPrimary","Magic.NayrusSecondary"}};
    for(bool spell:{false,true}) for(bool mmOwner:{false,true}) for(int p=1;p<=3;++p) {
        const auto& row=spell?spells[p-1]:mmOwner?mm[p-1]:oot[p-1];
        std::string primary=std::string(mmOwner?"gCosmetic.":"gCosmetics.")+row[0];
        std::string secondary=std::string(mmOwner?"gCosmetic.":"gCosmetics.")+row[1];
        auto edited=[&](const char* key,uint32_t& rgb) {
            if(key==primary) {rgb=0xA044E2;return true;}
            if(key==secondary) {rgb=0x20CC78;return true;}
            assert(false && "GI selected an unrelated cosmetic entry or archive owner");return false;
        };
        const auto color=NeiElementalGi::ReadPalette(p,spell,mmOwner,edited);
        assert(color.hot==0xA044E2 && color.edge==0x20CC78 && color.shimmer==0x20CC78);
        auto reset=[](const char*,uint32_t&){return false;};
        const auto base=NeiElementalGi::ReadPalette(p,spell,mmOwner,reset);
        if(!spell) assert(base.shimmer==(p==1?0xFA8B20u:p==2?0x357CFFu:0xFDFF7Bu));
    }
    std::cout<<"PASS all six GIs use their own primary/secondary editor entries, donor ownership and default arrow shimmer\n";
}
#else
int main() {assert(false && "live elemental GI cosmetic routing is missing");}
#endif
