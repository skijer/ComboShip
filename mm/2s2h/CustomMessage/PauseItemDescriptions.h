#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "z64.h"

/**
 * Custom C-Up description for `itemId` on kaleido page `pageIndex`, or NULL when the item has none
 * (vanilla MM items keep their own 0x17xx message).
 */
const char* PauseItemDesc_Get(u16 itemId, s32 pageIndex);
// Current dungeon's owned map/compass information, including Start With items.
const char* PauseItemDesc_GetMapInfo(s32 dungeon, u16 itemId);

// C-Up on the equipment page's upgrade column: row 0 = Magic Cape, row 1 = Pendant of Memories.
const char* PauseItemDesc_GetEquipUpgrade(s16 row);

/**
 * True when `textId` actually exists in the NES message table.
 *
 * Message_FindMessageNES leaves `font->messageStart` UNTOUCHED when the id isn't found, and
 * func_801514B0 then dereferences it as a MessageTableEntry — so asking for a description that
 * doesn't exist (every id past the vanilla item range) reads a stale pointer, or NULL on the first
 * textbox of the session. Always gate func_801514B0 on this for computed ids.
 */
u8 PauseItemDesc_VanillaTextExists(u16 textId);

/**
 * Open `desc` as a pause-menu description textbox, styled exactly like the vanilla 0x17xx boxes.
 * `textBoxPos` is the same argument func_801514B0 takes (3 = upper rows, 1 = lower rows).
 */
void PauseItemDesc_Show(PlayState* play, const char* desc, u8 textBoxPos);
// Route the selected identity/mode and preserve encoded message byte lengths.
u8 PauseItemDesc_ShowItem(PlayState* play, u16 itemId, s32 pageIndex, u8 textBoxPos);
u8 PauseItemDesc_ShowEquipment(PlayState* play, s16 subPage, s16 row, s16 col, u8 textBoxPos);
// A on the owned Magic Cape opens its Show/Hide appearance choice.
u8 PauseItemDesc_ShowCapeVisibility(PlayState* play, u8 textBoxPos);
u8 PauseItemDesc_ShowForm(PlayState* play, s32 form, u8 textBoxPos);

#ifdef __cplusplus
}
#endif
