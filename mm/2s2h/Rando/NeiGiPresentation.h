#pragma once
#include "ComboItemDrawABI.h"
#include "Rando/Types.h"
#include "../../../soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
void MM_DrawNeiGi(const CwItemDrawInfo& info, bool shop = false, int mmPickup = 0);
bool MM_DescribeNeiGi(RandoItemId item, CwItemDrawInfo* out);
bool MM_TryDrawNeiGi(RandoItemId item, bool shop = false, int mmPickup = 0);

// Wrap only the legacy fallback after the authored presentation was declined.
// Its model may change the matrix; identity effects retain authored GI units.
class MM_NeiGiFallbackShimmer {
  public:
    explicit MM_NeiGiFallbackShimmer(RandoItemId item, bool shop = false, int mmPickup = 0,
                                     NeiGi::Kind fitKind = NeiGi::Kind::Neutral);
    ~MM_NeiGiFallbackShimmer();
    MM_NeiGiFallbackShimmer(const MM_NeiGiFallbackShimmer&) = delete;
    MM_NeiGiFallbackShimmer& operator=(const MM_NeiGiFallbackShimmer&) = delete;

  private:
    NeiGi::Kind mKind;
    bool mEnabled;
    bool mShop;
    int mMmPickup;
};
