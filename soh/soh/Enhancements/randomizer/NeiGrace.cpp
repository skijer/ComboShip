#include "randomizer.h"
#include "SeedContext.h"
#include "variables.h"
#include "../../../../combo/NeiGracePolicy.h"
extern "C" {
#include "mods/nei_save.h"
}

extern "C" int HGrace_CanActivateOot(void) {
    if (!IS_RANDO)
        return 1;
    const auto ctx = Rando::Context::GetInstance();
    return NeiGrace_CanActivate(ctx->GetOption(RSK_HYLIAS_GRACE).Get(), ctx->GetOption(RSK_HYLIAS_GRACE_REWARDS).Get(),
                                gSaveContext.inventory.questItems, Nei_Save()->mmQuestItems);
}
