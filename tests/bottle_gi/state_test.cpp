#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include "combo/menu/ComboItemDrawABI.h"
#define COMBO_EXPORT
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name) name,
#define RANDO_ENUM_END(name) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
/* MM_ITEM_ENUM */
static RandoItemId requested;
static bool alternate;
namespace ItemGrantAudit { struct Scope { Scope(const char*,int,int,bool) {} }; }
namespace Rando::StaticData {
RandoItemId GetItemIdFromDisplayName(const char*) { return requested; }
RandoItemId GetItemIdFromName(const char*) { return requested; }
}
int DungeonItem_GetOwner(RandoItemId) { return -1; }
bool MM_HasAnimDraw(RandoItemId) { return false; }
// Native stream/export execution is covered by render_test.cpp. Here the
// descriptor is the engine boundary and the actual owner dependency policy runs.
int MM_FillItemDrawInfo(RandoItemId, CwItemDrawInfo* out) {
    out->dlistCount=alternate?3:6;
    out->xluStartIndex=alternate?0:4;
    return 1;
}
/* PRODUCTION_DEPENDENCIES */
int main() {
    for (bool alt : {false,true,false}) {
        alternate=alt;
        for (auto rg : {RG_BOTTLE_WITH_RED_POTION,RG_BOTTLE_WITH_GREEN_POTION,RG_BOTTLE_WITH_BLUE_POTION,
                        RG_RED_POTION_REFILL,RG_GREEN_POTION_REFILL,RG_BLUE_POTION_REFILL,
                        RG_BUY_RED_POTION_30,RG_BUY_RED_POTION_40,RG_BUY_RED_POTION_50,
                        RG_BUY_GREEN_POTION,RG_BUY_BLUE_POTION}) {
            CwItemDrawInfo out{};
            assert(OOT_DrawDependency(rg,out)==2 && "foreign OoT potion must refresh after owner Alt changes");
        }
        for (auto id : {RI_RED_POTION_REFILL,RI_GREEN_POTION_REFILL,RI_BLUE_POTION_REFILL,
                        RI_OOT_BOTTLE_GREEN_POTION,RI_OOT_BOTTLE_BLUE_POTION}) {
            requested=id;
            CwItemDrawInfo out{};
            assert(MM_GetItemDrawInfo("preview",&out)==1);
            assert(out.stateDependent==2 && "foreign MM potion must refresh after owner Alt changes");
            assert(out.dlistCount==(alt?3:6) && out.xluStartIndex==(alt?0:4));
        }
    }
    CwItemDrawInfo out{};
    assert(OOT_DrawDependency(RG_PROGRESSIVE_KOKIRI_SWORD,out)==1);
    assert(OOT_DrawDependency(RG_ARROWS_10,out)==0);
    requested=RI_PROGRESSIVE_SWORD;
    assert(MM_GetItemDrawInfo("preview",&out)==1 && out.stateDependent==1);
    requested=RI_ARROWS_10;
    assert(MM_GetItemDrawInfo("preview",&out)==1 && out.stateDependent==0);
    requested=RI_UNKNOWN;
    assert(MM_GetItemDrawInfo("unknown",&out)==0);
    puts("PASS foreign potion appearance refresh; progressive acquisition remains latched");
}
