/**
 * ext_buttons.cpp — Extended-button infrastructure accessor API (see ext_buttons.h).
 *
 * Globbed and compiled as its own translation unit by mm/CMakeLists.txt (mods/*.cpp). Exports
 * C-linkage helpers so the u8 buttonItems marker (ITEM_EXT_BUTTON) and the parallel u16
 * extButtons array stay in lockstep.
 */
#include "ext_buttons.h"
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope.h"
#ifdef COMBO_BUILD
#include "gameplay/MagicMirrorItem.h"
#endif

extern "C" {

u16 ExtButton_GetItem(s32 form, s32 btn) {
    u8 raw = BUTTON_ITEM_EQUIP(form, btn);
    if (raw == ITEM_EXT_BUTTON) {
        return EXT_BUTTON_ITEM(form, btn);
    }
    return (u16)raw;
}

void ExtButton_SetItem(s32 form, s32 btn, u16 extId) {
    BUTTON_ITEM_EQUIP(form, btn) = ITEM_EXT_BUTTON;
    EXT_BUTTON_ITEM(form, btn) = extId;
}

void ExtButton_ClearItem(s32 form, s32 btn) {
    BUTTON_ITEM_EQUIP(form, btn) = ITEM_NONE;
    EXT_BUTTON_ITEM(form, btn) = 0;
}

u16 ExtButton_GetDpadItem(s32 form, s32 btn) {
    u8 raw = DPAD_BUTTON_ITEM_EQUIP(form, btn);
    auto& equips = gSaveContext.save.shipSaveInfo.dpadEquips;
    if (raw == ITEM_EXT_BUTTON && equips.extItems[form][btn] >= 0x0200) {
        return equips.extItems[form][btn];
    }
    // Old saves truncated EXT ids but retained the unique page-2 slot band (72..95).
    // Repair only a matching owned slot: a real mushroom's bottle slot never qualifies.
    const u8 slot = DPAD_SLOT_EQUIP(form, btn);
    if (slot >= 72 && slot < 96) {
        const u16 owned = gSaveContext.save.shipSaveInfo.nei.ownedItems[slot - 72];
        if (owned >= 0x0200 && (raw == ITEM_EXT_BUTTON || raw == (u8)owned)) {
            DPAD_BUTTON_ITEM_EQUIP(form, btn) = ITEM_EXT_BUTTON;
            equips.extItems[form][btn] = owned;
            return owned;
        }
    }
    return raw == ITEM_EXT_BUTTON ? ITEM_NONE : raw;
}

void ExtButton_SetDpadItem(s32 form, s32 btn, u16 item) {
    DPAD_BUTTON_ITEM_EQUIP(form, btn) = item >= 0x0200 ? ITEM_EXT_BUTTON : (u8)item;
    gSaveContext.save.shipSaveInfo.dpadEquips.extItems[form][btn] = item >= 0x0200 ? item : 0;
}

static u16 ExtButton_GetEquipItem(s32 button) {
    return button < PAUSE_EQUIP_D_RIGHT ? ExtButton_GetItem(0, button + EQUIP_SLOT_C_LEFT)
                                        : ExtButton_GetDpadItem(0, button - PAUSE_EQUIP_D_RIGHT);
}

static u8 ExtButton_GetEquipSlot(s32 button) {
    return button < PAUSE_EQUIP_D_RIGHT ? C_SLOT_EQUIP(0, button + EQUIP_SLOT_C_LEFT)
                                        : DPAD_SLOT_EQUIP(0, button - PAUSE_EQUIP_D_RIGHT);
}

static void ExtButton_WriteEquip(PlayState* play, s32 button, u16 item, u8 slot) {
    if (button < PAUSE_EQUIP_D_RIGHT) {
        s32 btn = button + EQUIP_SLOT_C_LEFT;
        if (item >= 0x0200) {
            ExtButton_SetItem(0, btn, item);
        } else {
            ExtButton_ClearItem(0, btn);
            BUTTON_ITEM_EQUIP(0, btn) = (u8)item;
        }
        // Keep the existing C-button contract: EXT cells are not native inventory indices.
        C_SLOT_EQUIP(0, btn) = item >= 0x0200 ? SLOT_NONE : slot;
        Interface_LoadItemIconImpl(play, btn);
    } else {
        s32 btn = button - PAUSE_EQUIP_D_RIGHT;
        ExtButton_SetDpadItem(0, btn, item);
        DPAD_SLOT_EQUIP(0, btn) = slot;
        Interface_Dpad_LoadItemIconImpl(play, btn);
    }
}

s32 ExtButton_EquipItem(PlayState* play, s32 button, u16 item, u8 slot) {
    if (button < PAUSE_EQUIP_C_LEFT || button > PAUSE_EQUIP_D_UP) {
        return false;
    }
    u16 displaced = ExtButton_GetEquipItem(button);
    if (item < 0x0200 && displaced < 0x0200) {
        return false;
    }
    // Kaleido's temporary magic-arrow animation ids resolve to ordinary bow items here.
    if (item >= 0xB5 && item < 0xB8) {
        item -= 0xB5 - ITEM_BOW_FIRE;
        slot = SLOT_BOW;
    }
    u8 displacedSlot = ExtButton_GetEquipSlot(button);
#ifdef COMBO_BUILD
    bool mirrorMoved = displaced == EXT_ITEM_MAGIC_MIRROR;
#endif
    for (s32 other = PAUSE_EQUIP_C_LEFT; other <= PAUSE_EQUIP_D_UP; ++other) {
        if (other == button) {
            continue;
        }
        u16 otherItem = ExtButton_GetEquipItem(other);
        // EXT items in older C-button saves have SLOT_NONE: match their full id, never that slot.
        if ((item >= 0x0200 && otherItem == item) ||
            (item < 0x0200 && slot != SLOT_NONE && ExtButton_GetEquipSlot(other) == slot)) {
#ifdef COMBO_BUILD
            if (item == EXT_ITEM_MAGIC_MIRROR) {
                // Heal old duplicate mappings without duplicating the displaced item.
                ExtButton_WriteEquip(play, other, mirrorMoved ? ITEM_NONE : displaced,
                                     mirrorMoved ? SLOT_NONE : displacedSlot);
                mirrorMoved = true;
                continue;
            }
#endif
            ExtButton_WriteEquip(play, other, displaced, displacedSlot);
            break;
        }
    }
    ExtButton_WriteEquip(play, button, item, slot);
    return true;
}

} // extern "C"
