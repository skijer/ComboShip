"""Exercise OoT shelf spawn/consumption with real actor structs and production bodies."""
import re
import subprocess
import sys
import tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_weather_tests import production_function
shop = (ROOT / 'soh/src/overlays/actors/ovl_En_Ossan/z_en_ossan.c').read_text()
item = (ROOT / 'soh/src/overlays/actors/ovl_En_GirlA/z_en_girla.c').read_text()
source = r'''

#include "global.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#include "overlays/actors/ovl_En_GirlA/z_en_girla.h"
#include "overlays/actors/ovl_En_Tana/z_en_tana.h"
#include <cassert>
#include <cstdio>
#undef IS_RANDO
#define IS_RANDO 1
#undef CVAR_ENHANCEMENT
#define CVAR_ENHANCEMENT(x) x
bool obtained=false;
int spawned=0;
EnGirlA spawnedItems[8]{};
s32 Flags_GetItemGetInf(s32){return 0;}
s32 Flags_GetRandomizerInf(RandomizerInf){return obtained;}
s32 CVarGetInteger(const char*,s32 fallback){return fallback;}
u8 Randomizer_GetSettingValue(RandomizerSettingKey){return 1;}
ShopItemIdentity Randomizer_IdentifyShopItem(s32,u8){ShopItemIdentity identity{};
identity.identity.randomizerCheck=(RandomizerCheck)1;identity.enGirlAShopItem=SI_RANDOMIZED_ITEM;
__OLD_VANILLA_FIELD__
return identity;}
GetItemEntry Randomizer_GetItemFromKnownCheckWithoutObtainabilityCheck(RandomizerCheck,GetItemID){return {};}
Actor* Actor_Spawn(ActorContext*,PlayState*,s16,f32,f32,f32,s16,s16,s16,s16 params){
EnGirlA* item=&spawnedItems[spawned++];item->actor.params=params;return &item->actor;}
s16 ShopItemDisp_Default(s16 item){return item;}
EnOssanGetGirlAParamsFunc sShopItemReplaceFunc[SI_MAX];
s16 sItemShelfRot[8]{};
'''
# Before the exact-parameter fix, the generic vanilla map collapsed the Bombchu variants.
old = 'identity.vanillaEnGirlAShopItem=SI_BOMBCHU_10_1;' if 'vanillaEnGirlAShopItem' in item else ''
source = source.replace('__OLD_VANILLA_FIELD__', old)
source += re.search(r'typedef struct \{\s*/\* 0x00 \*/ s16 shopItemIndex;.*?\} ShopItem;', shop, re.S)[0]
source += re.sub(r'\bthis\b','self',production_function(shop,'EnOssan_SpawnItemsOnShelves'))
source += re.sub(r'\bthis\b','self',production_function(item,'EnGirlA_TryChangeShopItemShip'))
source += r'''
int main(){
PlayState play{};EnOssan seller{};EnTana shelf{};seller.shelves=&shelf;
for(auto& fn:sShopItemReplaceFunc)fn=ShopItemDisp_Default;
ShopItem stock[8]{};for(auto& cell:stock)cell.shopItemIndex=-1;
stock[0].shopItemIndex=SI_BOMBCHU_10_3;
EnOssan_SpawnItemsOnShelves(&seller,&play,stock);
assert(spawned==1&&seller.shelfSlots[0]->actor.params==SI_RANDOMIZED_ITEM);
assert(!EnGirlA_TryChangeShopItemShip(seller.shelfSlots[0],&play));
obtained=true;
assert(EnGirlA_TryChangeShopItemShip(seller.shelfSlots[0],&play));
assert(seller.shelfSlots[0]->actor.params==SI_BOMBCHU_10_3);
assert(!EnGirlA_TryChangeShopItemShip(seller.shelfSlots[0],&play));
// Revisiting the scene selects its native entry immediately, with no randomized shelf again.
spawned=0;EnOssan_SpawnItemsOnShelves(&seller,&play,stock);
assert(seller.shelfSlots[0]->actor.params==SI_BOMBCHU_10_3);
puts("PASS OoT exact original stock, permanent consumed check and revisit");
}
'''
with tempfile.TemporaryDirectory(prefix='oot-shop-') as td:
    p=Path(td)/'test.cpp';p.write_text(source)
    binary=Path(td)/'test'
    includes=['soh','soh/include','soh/include/PR','soh/src','soh/assets','libultraship/include','combo']
    subprocess.run(['c++','-std=c++20','-w','-fmax-errors=5','-fpermissive','-DF3DEX_GBI_2','-DCONTROLLERBUTTONS_T=uint32_t',*[f'-I{ROOT/x}' for x in includes],str(p),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
