#pragma once
#ifdef COMBO_BUILD
#include "rando/BlackMarket.h"
#include "2s2h/Rando/Types.h"
namespace ComboBlackMarketItems {
RandoItemId Id(const ComboRando::BlackMarket::Item& item);
bool Owned(const ComboRando::BlackMarket::Item& item);
bool Available(const ComboRando::BlackMarket::Item& item);
bool Grant(const ComboRando::BlackMarket::Item& item);
} // namespace ComboBlackMarketItems
#endif
