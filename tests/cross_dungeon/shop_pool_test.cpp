#include "combo/rando/CrossDungeonSeed.h"
#include <cassert>
#include <iostream>
using namespace ComboRando::CrossDungeon;
int main() {
    PoolOptions options; options.dungeons=false; options.interiors=true;
    assert(!InteriorPool(InteriorKind::ShopDoor,options) &&
           "retail shops must never enter the broad Interior pool when shop shuffle is off");
    options.shops=true;
    assert(InteriorPool(InteriorKind::ShopDoor,options)==Pool::Shop &&
           "shop shuffle must retain its own pool alongside broad Interiors");
    Dungeon shop{"shop",{0,100},{0,101},{0,101}};shop.pool=Pool::Shop;
    assert(Selected(shop,options));
    auto peer=shop;peer.id="peer";peer.entrance={1,200};peer.exit=peer.returnTo={1,201};
    Dungeon house{"house",{0,300},{0,301},{0,301}};house.pool=Pool::Interior;
    auto seed=Encode({shop,peer,house},{1,0,2});
    assert(seed.at("version")==4 && "new shop membership requires an explicit replay version");
    assert(Decode(seed)==FromPermutation({shop,peer,house},{1,0,2}));
    // A recorded v3 shop map remains valid without changing its route or pool.
    auto legacy=Encode({shop,peer},{1,0});legacy["version"]=3;
    assert(Decode(legacy)==FromPermutation({shop,peer},{1,0}));
    auto oldOptions=OptionsFromSeed(legacy);
    assert(oldOptions.legacyInteriors);
    assert(ParseOptions(OptionsJson(oldOptions)).legacyInteriors &&
           "host catalogue transport must retain the recorded legacy membership policy");
    oldOptions.interiors=true;
    assert(InteriorPool(InteriorKind::ShopDoor,oldOptions)==Pool::Interior);
    assert(InteriorPool(InteriorKind::ShopPassage,oldOptions)==Pool::Interior);
    oldOptions.interiors=false;
    assert(!InteriorPool(InteriorKind::ShopPassage,oldOptions));
    seed["options"]["legacyInteriors"]=true;
    assert(!OptionsFromSeed(seed).legacyInteriors &&
           "schema v4 membership must be owned by its version");
    std::cout<<"shop selection: exclusive pool, broad coexistence, v4 and legacy replay PASS\n";
}
