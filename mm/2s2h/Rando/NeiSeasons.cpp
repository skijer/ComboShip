#include "Rando.h"
#include "mods/extended_inventory.h"

extern "C" uint8_t Seasons_RandoMode(void) {
    return IS_RANDO ? NeiSeasons_SeedMode(RANDO_SAVE_OPTIONS[RO_ROD_OF_SEASONS]) : NEI_SEASONS_INDIVIDUAL;
}

extern "C" void Seasons_UpdateGates(void) {
    if (Seasons_RandoMode() != NEI_SEASONS_GATED)
        return;
    // Completion events are separate from shuffled boss remains. Latch them so
    // Song of Time cannot take back an unlocked season.
    Nei_Save()->seasonsGates |= NeiSeasons_MmGates(CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE),
                                                   CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE),
                                                   CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE),
                                                   CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE));
}
