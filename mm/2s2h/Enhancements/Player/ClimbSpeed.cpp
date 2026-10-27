#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "mods/combo_rpg.h"

#define CVAR_NAME "gEnhancements.Player.ClimbSpeed"
#define CVAR CVarGetInteger(CVAR_NAME, 1)

void RegisterClimbSpeed() {
    COND_VB_SHOULD(VB_SET_CLIMB_SPEED, CVAR > 1, {
        // The saved seed owns this multiplier when RPG climb is enabled.
        if (ComboRpg_IsEnabled(COMBO_RPG_CLIMB))
            return;
        f32* speed = va_arg(args, f32*);
        *speed *= CVAR;
    });
}

static RegisterShipInitFunc initFunc(RegisterClimbSpeed, { CVAR_NAME });
