/**
 * ext_buttons.h — Extended-button infrastructure accessor API.
 *
 * `equips.buttonItems[]` is u8 and the u8 ItemID space is essentially exhausted. To equip custom
 * items whose real id is u16 (>= 0x0200), one reserved u8 (ITEM_EXT_BUTTON, z64item.h) acts as a
 * MARKER: when a slot holds ITEM_EXT_BUTTON, the REAL (u16) id lives in the parallel array
 * gSaveContext.ship.extButtons.items[button] (EXT_BUTTON_ITEM macro, z64save.h).
 *
 * OoT's equips arrays are FLAT — there is no per-form dimension like MM's [form][slot] — so `btn`
 * here is the same index used for buttonItems: 0 = B, 1-3 = C-left/down/right, 4-7 = D-pad.
 *
 * These accessors are the single place that keeps buttonItems + extButtons in sync. Callable from
 * C (z_parameter.c, the kaleido overlays) and C++ (mods) — the .cpp exports C linkage.
 */
#ifndef EXT_BUTTONS_H
#define EXT_BUTTONS_H

#include "z64.h"

#ifdef __cplusplus
extern "C" {
#endif

// Returns the effective item id for a button slot: the real u16 id from extButtons when the slot is
// marked ITEM_EXT_BUTTON, otherwise the plain u8 buttonItems value (widened to u16).
// A marker without a valid full custom ID is empty; zero must not become a stick.
// Native C/D zero with SLOT_NONE is also empty; B and valid SLOT_STICK are preserved.
u16 ExtButton_GetItem(s32 btn);

// Equip an extended (u16) item to a button slot: marks buttonItems with ITEM_EXT_BUTTON and stores
// the real id in extButtons. Does NOT touch cButtonSlots — callers set that as needed.
void ExtButton_SetItem(s32 btn, u16 extId);

// Clear a button slot: buttonItems -> ITEM_NONE, extButtons -> 0.
void ExtButton_ClearItem(s32 btn);

#ifdef COMBO_BUILD
// Narrow pause-equip path for moves involving Magic Mirror. Preserves full IDs
// while swapping/clearing the previous C/D mapping; caller reloads the target icon.
// Returns false without changing anything when neither item is the Mirror.
s32 ExtButton_EquipMagicMirror(PlayState* play, s32 btn, u16 item, u8 slot);
#endif

// Pair the native child/adult ItemEquips snapshots with their full custom IDs.
// Store before changing the raw buttons; load only when restoring that age's buttons.
void ExtButton_SaveAgeItems(s32 age);
void ExtButton_LoadAgeItems(s32 age);
void ExtButton_ResetSaveData(void);

#ifdef __cplusplus
}
#endif

#endif // EXT_BUTTONS_H
