/**
 * ext_buttons.cpp — Extended-button infrastructure accessor API (see ext_buttons.h).
 *
 * Globbed and compiled as its own translation unit by soh/CMakeLists.txt (mods/*.cpp, recursive).
 * Exports C-linkage helpers so the u8 buttonItems marker (ITEM_EXT_BUTTON) and the parallel u16
 * extButtons array stay in lockstep.
 */
#include "ext_buttons.h"
#include "macros.h"
#ifdef COMBO_BUILD
#include "functions.h"
#include "gameplay/MagicMirrorItem.h"
#include <libultraship/bridge/consolevariablebridge.h>
#endif

// gSaveContext is declared in variables.h, which z64.h does not pull in. Declaring the one symbol
// this file needs keeps the TU free of that header's global-variable flood (same as nei_save.cpp).
extern "C" SaveContext gSaveContext;

extern "C" {

u16 ExtButton_GetItem(s32 btn) {
    u8 raw = gSaveContext.equips.buttonItems[btn];
    if (raw == ITEM_EXT_BUTTON) {
        const u16 item = EXT_BUTTON_ITEM(btn);
        return item >= 0x0200 ? item : (u16)ITEM_NONE;
    }
    // Native C/D equips always carry their inventory slot. Preserve B's
    // separate contract and valid zero-valued sticks with SLOT_STICK.
    if (raw == ITEM_STICK && btn > 0 && gSaveContext.equips.cButtonSlots[btn - 1] == SLOT_NONE) {
        return ITEM_NONE;
    }
    return (u16)raw;
}

void ExtButton_SetItem(s32 btn, u16 extId) {
    gSaveContext.equips.buttonItems[btn] = ITEM_EXT_BUTTON;
    EXT_BUTTON_ITEM(btn) = extId;
}

void ExtButton_ClearItem(s32 btn) {
    gSaveContext.equips.buttonItems[btn] = ITEM_NONE;
    EXT_BUTTON_ITEM(btn) = 0;
}

#ifdef COMBO_BUILD
static void ExtButton_WriteMirrorEquip(s32 btn, u16 item, u8 slot) {
    if (item >= 0x0200) {
        ExtButton_SetItem(btn, item);
        gSaveContext.equips.cButtonSlots[btn - 1] = SLOT_NONE;
    } else {
        ExtButton_ClearItem(btn);
        gSaveContext.equips.buttonItems[btn] = (u8)item;
        gSaveContext.equips.cButtonSlots[btn - 1] = item == ITEM_NONE ? SLOT_NONE : slot;
    }
}

s32 ExtButton_EquipMagicMirror(PlayState* play, s32 btn, u16 item, u8 slot) {
    if (btn < 1 || btn >= (s32)ARRAY_COUNT(gSaveContext.equips.buttonItems)) {
        return false;
    }
    const u16 displaced = ExtButton_GetItem(btn);
    if (item != EXT_ITEM_MAGIC_MIRROR && displaced != EXT_ITEM_MAGIC_MIRROR) {
        return false;
    }
    const u8 displacedSlot = gSaveContext.equips.cButtonSlots[btn - 1];
    bool moved = displaced == item;
    for (s32 other = 1; other < (s32)ARRAY_COUNT(gSaveContext.equips.buttonItems); ++other) {
        if (other == btn) {
            continue;
        }
        const u16 otherItem = ExtButton_GetItem(other);
        const bool same = item >= 0x0200 ? otherItem == item
                                         : slot != SLOT_NONE && gSaveContext.equips.cButtonSlots[other - 1] == slot;
        const bool bow = item == ITEM_BOW && otherItem >= ITEM_BOW_ARROW_FIRE && otherItem <= ITEM_BOW_ARROW_LIGHT &&
                         !CVarGetInteger("gEnhancements.SeparateArrows", 0);
        if (same || bow) {
            // Match the Mirror's full ID: its native slot is intentionally NONE.
            // Old duplicates get cleared; the displaced item moves only once.
            ExtButton_WriteMirrorEquip(other, moved ? ITEM_NONE : displaced, displacedSlot);
            Interface_LoadItemIcon2(play, other);
            moved = true;
        }
    }
    ExtButton_WriteMirrorEquip(btn, item, slot);
    return true;
}
#endif

void ExtButton_SaveAgeItems(s32 age) {
    auto& saved = age == LINK_AGE_CHILD ? gSaveContext.ship.childExtButtons : gSaveContext.ship.adultExtButtons;
    for (size_t button = 0; button < ARRAY_COUNT(saved.items); ++button) {
        saved.items[button] = gSaveContext.equips.buttonItems[button] == ITEM_EXT_BUTTON ? EXT_BUTTON_ITEM(button) : 0;
    }
}

void ExtButton_LoadAgeItems(s32 age) {
    const auto& saved = age == LINK_AGE_CHILD ? gSaveContext.ship.childExtButtons : gSaveContext.ship.adultExtButtons;
    for (size_t button = 0; button < ARRAY_COUNT(saved.items); ++button) {
        EXT_BUTTON_ITEM(button) = gSaveContext.equips.buttonItems[button] == ITEM_EXT_BUTTON ? saved.items[button] : 0;
    }
}

void ExtButton_ResetSaveData(void) {
    gSaveContext.ship.extButtons = {};
    gSaveContext.ship.childExtButtons = {};
    gSaveContext.ship.adultExtButtons = {};
}

} // extern "C"
