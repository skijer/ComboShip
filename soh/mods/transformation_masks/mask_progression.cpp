#include "mods/transformation_masks/mask_progression.h"
#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
#include "mods/extended_inventory.h"
#include "soh/Notification/Notification.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" uint8_t MaskProgression_IsCompatible(void) {
    // Those seeds can require an early form to clear its own reward dungeon.
    // Read the loaded seed's options rather than live seed-generator controls.
    return !IS_RANDO || !(RAND_GET_OPTION(RSK_MM_MASKS_ALL) || RAND_GET_OPTION(RSK_MM_MASKS_TRANSFORM));
}

static bool ChildDungeonGatesActive(void) {
    return CVarGetInteger("gMods.TransformMasks.ChildDungeonGates", 0) && MaskProgression_IsCompatible();
}

extern "C" uint8_t MaskProgression_CanTransform(int32_t item) {
    if (!ChildDungeonGatesActive())
        return 1;
    // Completion flags identify the dungeon, independent of shuffled stone rewards.
    switch (item) {
        case ITEM_MM_MASK_DEKU:
        case ITEM_MASK_KEATON:
        case ITEM_MM_MASK_KEATON:
            return Flags_GetEventChkInf(EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP) != 0;
        case ITEM_MASK_GORON:
        case ITEM_MM_MASK_GORON:
            return Flags_GetEventChkInf(EVENTCHKINF_USED_DODONGOS_CAVERN_BLUE_WARP) != 0;
        case ITEM_MASK_ZORA:
        case ITEM_MM_MASK_ZORA:
            return Flags_GetEventChkInf(EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP) != 0;
        default:
            return 1;
    }
}

static bool GiveDungeonMask(uint16_t item) {
    if (ExtInv_HasMmMask(item))
        return false;
    ExtInv_SetItemById(item);
    return true;
}

static void GiveDungeonTradeMask(uint8_t item, RandomizerInf flag) {
    if (!Flags_GetRandomizerInf(flag))
        Flags_SetRandomizerInf(flag);
    // Keep Zelda's Letter and other active trade items in their current slot.
    if (INV_CONTENT(ITEM_TRADE_CHILD) == ITEM_NONE)
        INV_CONTENT(ITEM_TRADE_CHILD) = item;
}

extern "C" void MaskProgression_Update(PlayState* play) {
    if (play == nullptr || gSaveContext.fileNum == 0xFF || !ChildDungeonGatesActive())
        return;

    // Native mask ownership persists in the existing NEI save and participates
    // in the existing sharing bridge. Rechecking also catches up older saves.
    if (Flags_GetEventChkInf(EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP)) {
        const bool deku = GiveDungeonMask(ITEM_MM_MASK_DEKU);
        const bool keaton = GiveDungeonMask(ITEM_MM_MASK_KEATON);
        GiveDungeonTradeMask(ITEM_MASK_KEATON, RAND_INF_CHILD_TRADES_HAS_MASK_KEATON);
        if (deku || keaton)
            Notification::Emit(
                { .message = "Deku Tree cleared: Deku and Keaton Masks unlocked!", .remainingTime = 8.0f });
    }
    if (Flags_GetEventChkInf(EVENTCHKINF_USED_DODONGOS_CAVERN_BLUE_WARP)) {
        const bool goron = GiveDungeonMask(ITEM_MM_MASK_GORON);
        GiveDungeonTradeMask(ITEM_MASK_GORON, RAND_INF_CHILD_TRADES_HAS_MASK_GORON);
        if (goron)
            Notification::Emit({ .message = "Dodongo's Cavern cleared: Goron Mask unlocked!", .remainingTime = 8.0f });
    }
    if (Flags_GetEventChkInf(EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP)) {
        const bool zora = GiveDungeonMask(ITEM_MM_MASK_ZORA);
        GiveDungeonTradeMask(ITEM_MASK_ZORA, RAND_INF_CHILD_TRADES_HAS_MASK_ZORA);
        if (zora)
            Notification::Emit({ .message = "Jabu-Jabu's Belly cleared: Zora Mask unlocked!", .remainingTime = 8.0f });
    }
}
