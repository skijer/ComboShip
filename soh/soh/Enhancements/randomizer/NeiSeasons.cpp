#include "rando/CrossEntranceBoss.h"
#include "randomizer.h"
#include "SeedContext.h"
#include "variables.h"
#include "functions.h"
extern "C" {
#include "mods/extended_inventory.h"
}

extern "C" uint8_t Seasons_RandoMode(void) {
    return IS_RANDO ? NeiSeasons_SeedMode(Rando::Context::GetInstance()->GetOption(RSK_ROD_OF_SEASONS).Get())
                    : NEI_SEASONS_INDIVIDUAL;
}

extern "C" void Seasons_UpdateGates(void) {
    if (Seasons_RandoMode() != NEI_SEASONS_GATED)
        return;
    // Event flags describe completed checks, never possession of their randomized rewards.
    // Sheik's Ice Cavern check is the cavern's final completion event in both layouts.
    Nei_Save()->seasonsGates |= NeiSeasons_OotGates(
        Flags_GetEventChkInf(EVENTCHKINF_LEARNED_SONG_OF_STORMS),
        Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_BARINADE,
                                        Flags_GetEventChkInf(EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP)),
        Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_MORPHA,
                                        Flags_GetEventChkInf(EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP)),
        Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_VOLVAGIA,
                                        Flags_GetEventChkInf(EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP)),
        Combo_SOH_CrossBossLogicalClear(COMBO_BOSS_PHANTOM_GANON,
                                        Flags_GetEventChkInf(EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP)),
        Flags_GetEventChkInf(EVENTCHKINF_LEARNED_SERENADE_OF_WATER));
}
