#pragma once
#include <cstdint>

namespace ComboRando::CrossDungeon {
// Native entrance identities keep retail ownership independent of an opening door.
// The Happy Mask Shop belongs to the existing cross-game portal instead.
inline bool IsOotRetailShopEntrance(int32_t entrance) {
    switch (entrance) {
        case ENTR_KOKIRI_SHOP_0:
        case ENTR_GORON_SHOP_0:
        case ENTR_ZORA_SHOP_0:
        case ENTR_BAZAAR_0:
        case ENTR_BAZAAR_1:
        case ENTR_POTION_SHOP_MARKET_0:
        case ENTR_BOMBCHU_SHOP_1:
        case ENTR_POTION_SHOP_GRANNY_0:
        case ENTR_POTION_SHOP_KAKARIKO_FRONT:
        case ENTR_POTION_SHOP_KAKARIKO_BACK:
            return true;
        default:
            return false;
    }
}
} // namespace ComboRando::CrossDungeon
