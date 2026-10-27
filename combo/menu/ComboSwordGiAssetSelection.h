#pragma once
#include "ComboItemDrawABI.h"
#include "../../soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"

// The active host chooses appearance; the producing donor keeps its own Alt
// mode. Re-resolve only the awarded sword's authored, archive-pinned recipe.
static inline int32_t ComboSwordGi_SelectBaseAssets(CwItemDrawInfo& info, bool altAssets) {
    if (altAssets || info.neiShimmer < 1 || info.neiShimmer > int32_t(NeiGi::Kind::Last) + 1)
        return 1;
    const auto* bounds = NeiGi::FindSwordFrameBounds(static_cast<NeiGi::Kind>(info.neiShimmer - 1));
    if (!bounds)
        return 1;
    static Fn_GetNeiGiDrawInfoForAssets describe = nullptr;
    if (!describe)
        describe =
            reinterpret_cast<Fn_GetNeiGiDrawInfoForAssets>(Combo_ResolveSym("soh", "OOT_GetNeiGiDrawInfoForAssets"));
    if (!describe)
        return CW_DRAW_NOT_READY;
    CwItemDrawInfo vanilla{};
    if (describe(bounds->slug, false, &vanilla) != 1)
        return CW_DRAW_NOT_READY; // Retry after resource readiness or an appearance toggle.
    vanilla.resolvedName = info.resolvedName;
    info = vanilla;
    return 2; // A declined legacy descriptor was replaced by shipped geometry.
}
