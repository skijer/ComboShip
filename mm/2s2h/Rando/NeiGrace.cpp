#include "Rando.h"
#include "../../../combo/NeiGracePolicy.h"

extern "C" int HGrace_CanActivateMM(void) {
    if (!IS_RANDO)
        return 1;
    const auto& save = gSaveContext.save;
    return NeiGrace_CanActivate(save.shipSaveInfo.rando.randoSaveOptions[RO_HYLIAS_GRACE],
                                save.shipSaveInfo.rando.randoSaveOptions[RO_HYLIAS_GRACE_REWARDS],
                                save.shipSaveInfo.nei.ootQuestItems, save.saveInfo.inventory.questItems);
}
