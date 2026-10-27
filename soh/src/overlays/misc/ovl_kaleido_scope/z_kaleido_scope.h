#ifndef Z_KALEIDO_SCOPE_H
#define Z_KALEIDO_SCOPE_H

#include <libultraship/libultra.h>
#include "global.h"
#include "mods/extended_inventory.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"

extern u8 gAmmoItems[];
extern s16 D_8082AAEC[];
extern s16 D_8082AB2C[];
extern u8 gEquipAgeReqs[][4];
extern u8 gSlotAgeReqs[];
extern u8 gItemAgeReqs[];
extern u8 gAreaGsFlags[];

#define MAP_48x85_TEX_WIDTH 48
#define MAP_48x85_TEX_HEIGHT 85
#define MAP_48x85_TEX_SIZE ((MAP_48x85_TEX_WIDTH * MAP_48x85_TEX_HEIGHT) / 2) // 48x85 CI4 texture

#define AGE_REQ_ADULT LINK_AGE_ADULT
#define AGE_REQ_CHILD LINK_AGE_CHILD
#define AGE_REQ_NONE 9

// NEI resolves extended slots and transformed-form restrictions before the 9.3 hooks.
// Passing real requirements/indices retains Timeless Equipment and adult-mask subscribers.
#define CHECK_AGE_REQ_EQUIP(i, j)                                                                    \
    GameInteractor_Should(VB_PLAYER_MEETS_AGE_REQ,                                                    \
                          (ExtInv_GetEquipAgeReq(i, j) == AGE_REQ_NONE) ||                            \
                              (ExtInv_GetEquipAgeReq(i, j) == gSaveContext.linkAge),                  \
                          ExtInv_GetEquipAgeReq(i, j))

#define CHECK_AGE_REQ_SLOT(slotIndex)                                                                \
    GameInteractor_Should(VB_SLOT_MEETS_AGE_REQ,                                                      \
                          (ExtInv_GetSlotAgeReq(slotIndex) == AGE_REQ_NONE) ||                        \
                              (ExtInv_GetSlotAgeReq(slotIndex) == gSaveContext.linkAge),              \
                          slotIndex)

// ITEM_CUSTOM also aliases cor's Gyorg remains; native 9.3 Roc's Feather is
// the one item above that boundary whose requirement is initialized by Play_Init.
#define KALEIDO_ITEM_AGE_REQ(itemIndex)                                                              \
    (((itemIndex) >= ITEM_CUSTOM && (itemIndex) != ITEM_ROCS_FEATHER)                                 \
         ? ExtInv_GetItemAgeReq(itemIndex)                                                          \
         : gItemAgeReqs[itemIndex])

#define CHECK_AGE_REQ_ITEM(itemIndex)                                                                \
    GameInteractor_Should(VB_ITEM_MEETS_AGE_REQ,                                                      \
                          (KALEIDO_ITEM_AGE_REQ(itemIndex) == AGE_REQ_NONE) ||                        \
                              (KALEIDO_ITEM_AGE_REQ(itemIndex) == gSaveContext.linkAge),              \
                          itemIndex)

void KaleidoScope_DrawQuestStatus(PlayState* play, GraphicsContext* gfxCtx);
s32 KaleidoScope_UpdateQuestStatusPoint(PauseContext* pauseCtx, s32 point);
void KaleidoScope_DrawDebugEditor(PlayState* play);
void KaleidoScope_DrawPlayerWork(PlayState* play);
void KaleidoScope_DrawEquipment(PlayState* play);
void KaleidoScope_SetCursorVtx(PauseContext* pauseCtx, u16 index, Vtx* vtx);
void KaleidoScope_DrawItemSelect(PlayState* play);
void KaleidoScope_SetupItemEquip(PlayState* play, u16 item, u16 slot, s16 animX, s16 animY);
void KaleidoScope_UpdateItemEquip(PlayState* play);
void KaleidoScope_DrawDungeonMap(PlayState* play, GraphicsContext* gfxCtx);
void KaleidoScope_DrawWorldMap(PlayState* play, GraphicsContext* gfxCtx);
void KaleidoScope_UpdatePrompt(PlayState* play);
Gfx* KaleidoScope_QuadTextureIA4(Gfx* gfx, void* texture, s16 width, s16 height, u16 point);
Gfx* KaleidoScope_QuadTextureIA8(Gfx* gfx, void* texture, s16 width, s16 height, u16 point);
void KaleidoScope_MoveCursorToSpecialPos(PlayState* play, u16 specialPos);
void KaleidoScope_DrawQuadTextureRGBA32(GraphicsContext* gfxCtx, void* texture, u16 width, u16 height, u16 point);
void KaleidoScope_ProcessPlayerPreRender();
void KaleidoScope_SetupPlayerPreRender(PlayState* play);
void KaleidoScope_DrawCursor(PlayState* play, u16 pageIndex);
void KaleidoScope_UpdateDungeonMap(PlayState* play);

void PauseMapMark_Draw(PlayState* play);

void KaleidoScope_UpdateCursorSize(PauseContext* pauseCtx);

void KaleidoScope_ResetItemCycling();

#endif
