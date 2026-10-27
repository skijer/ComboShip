#include "z_kaleido_scope.h"
#include "textures/icon_item_static/icon_item_static.h"
#include "textures/parameter_static/parameter_static.h"
#define SWORD_KALEIDO_SETUP_OPA Gfx_SetupDL_42Opa
#define SWORD_KALEIDO_SETUP_OVERLAY Gfx_SetupDL_39Overlay
#define SWORD_KALEIDO_ALLOC Graph_Alloc
#include "sword_element_kaleido.h"
#include "sword_element.h"
#include "../../../../../combo/render/sword_element_ui.h"
#include <string.h>

/* The native elemental-arrow fusion cadence, with its destination changed to the sword.
 * This state is deliberately independent of pauseCtx->equipTarget* and all C-button slots. */
void SwordElementKaleido_Reset(SwordElementMenu* menu) {
    memset(menu, 0, sizeof(*menu));
}

uint8_t SwordElementKaleido_CanOpen(uint16_t item, uint8_t ownedAndUsable) {
    uint8_t element;
    if (!ownedAndUsable || SwordElement_SlotForItem(item) < 0) {
        return 0;
    }
    for (element = SWORD_ELEMENT_FIRE; element < SWORD_ELEMENT_COUNT; ++element) {
        if (SwordElement_IsAvailable(element)) {
            return 1;
        }
    }
    return 0;
}

uint8_t SwordElementKaleido_Open(SwordElementMenu* menu, uint16_t item, uint8_t ownedAndUsable, int16_t swordX,
                                 int16_t swordY) {
    if (!SwordElementKaleido_CanOpen(item, ownedAndUsable)) {
        return 0;
    }
    SwordElementKaleido_Reset(menu);
    menu->item = item;
    menu->selection = SwordElement_Get(item);
    if (menu->selection >= SWORD_ELEMENT_COUNT) {
        menu->selection = SWORD_ELEMENT_NONE;
    }
    menu->targetX = swordX;
    menu->targetY = swordY;
    menu->phase = SWORD_ELEMENT_MENU_CHOOSER;
    return 1;
}

uint8_t SwordElementKaleido_Update(SwordElementMenu* menu, uint8_t contextValid, int8_t direction, uint8_t confirm,
                                   uint8_t cancel) {
    uint8_t result = SWORD_ELEMENT_MENU_EVENT_NONE;
    if (menu->phase == SWORD_ELEMENT_MENU_CLOSED) {
        return result;
    }
    if (!contextValid || cancel) {
        SwordElementKaleido_Reset(menu);
        return SWORD_ELEMENT_MENU_EVENT_CANCEL;
    }
    if (menu->moveCooldown) {
        menu->moveCooldown--;
    }
    switch (menu->phase) {
        case SWORD_ELEMENT_MENU_CHOOSER:
            if (direction && !menu->moveCooldown) {
                int16_t next = menu->selection + (direction < 0 ? -1 : 1);
                if (next >= SWORD_ELEMENT_NONE && next < SWORD_ELEMENT_COUNT) {
                    menu->selection = next;
                    menu->moveCooldown = 8;
                    result |= SWORD_ELEMENT_MENU_EVENT_CURSOR;
                }
            }
            if (!confirm) {
                return result;
            }
            if (!SwordElement_IsAvailable(menu->selection)) {
                return result | SWORD_ELEMENT_MENU_EVENT_ERROR;
            }
            if (menu->selection == SWORD_ELEMENT_NONE) {
                if (!SwordElement_Set(menu->item, menu->selection)) {
                    return result | SWORD_ELEMENT_MENU_EVENT_ERROR;
                }
                SwordElementKaleido_Reset(menu);
                return result | SWORD_ELEMENT_MENU_EVENT_APPLY;
            }
            menu->x = menu->targetX - 440;
            menu->y = menu->targetY - 440;
            menu->alpha = 0;
            menu->phase = SWORD_ELEMENT_MENU_GROW_ORB;
            return result | SWORD_ELEMENT_MENU_EVENT_ARROW;
        case SWORD_ELEMENT_MENU_GROW_ORB:
            if (menu->alpha > 241) {
                menu->alpha = 254;
                menu->timer = 5;
                menu->phase = SWORD_ELEMENT_MENU_HOLD_SOURCE;
            } else {
                menu->alpha += 14;
            }
            break;
        case SWORD_ELEMENT_MENU_HOLD_SOURCE:
            if (--menu->timer == 0) {
                menu->timer = 6;
                menu->phase = SWORD_ELEMENT_MENU_MOVE_TO_SWORD;
            }
            break;
        case SWORD_ELEMENT_MENU_MOVE_TO_SWORD:
            menu->x += (menu->targetX - menu->x) / menu->timer;
            menu->y += (menu->targetY - menu->y) / menu->timer;
            if (--menu->timer == 0) {
                menu->timer = 4;
                menu->phase = SWORD_ELEMENT_MENU_HOLD_SWORD;
            }
            break;
        case SWORD_ELEMENT_MENU_HOLD_SWORD:
            if (--menu->timer == 0) {
                /* Save and native equip happen together only after the cancellable animation.
                 * Revalidate here: an unlocked arrow or its checkbox may have changed meanwhile. */
                if (!SwordElement_Set(menu->item, menu->selection)) {
                    menu->phase = SWORD_ELEMENT_MENU_CHOOSER;
                    menu->alpha = 0;
                    return SWORD_ELEMENT_MENU_EVENT_ERROR;
                }
                SwordElementKaleido_Reset(menu);
                return SWORD_ELEMENT_MENU_EVENT_FUSE | SWORD_ELEMENT_MENU_EVENT_APPLY;
            }
            break;
        default:
            SwordElementKaleido_Reset(menu);
            break;
    }
    return result;
}

void SwordElementKaleido_Draw(PlayState* play, const SwordElementMenu* menu, void* swordIcon) {
    static const u8 color[][3] = { { 255, 255, 255 }, { 255, 0, 0 }, { 100, 100, 255 }, { 255, 255, 100 } };
    u8 element;
    if (!menu->phase || play->pauseCtx.alpha == 0) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);
    if (menu->phase >= SWORD_ELEMENT_MENU_GROW_ORB) {
        /* The picker closes while the native orb grows beside the sword, then merges into it.
         * Use this page's existing model/projection, so the destination stays on the icon. */
        Vtx* v = SWORD_KALEIDO_ALLOC(play->state.gfxCtx, sizeof(Vtx) * 4);
        s16 grow = (menu->alpha / 8) / 2;
        s32 i;
        if (v != NULL) {
            memset(v, 0, sizeof(Vtx) * 4);
            for (i = 0; i < 4; ++i) {
                v[i].v.ob[0] = menu->x / 10 - grow + ((i & 1) ? 32 + grow * 2 : 0);
                v[i].v.ob[1] = menu->y / 10 + grow - ((i & 2) ? 32 + grow * 2 : 0);
                v[i].v.tc[0] = (i & 1) ? 32 << 5 : 0;
                v[i].v.tc[1] = (i & 2) ? 32 << 5 : 0;
                memset(v[i].v.cn, 255, sizeof(v[i].v.cn));
            }
            SWORD_KALEIDO_SETUP_OPA(play->state.gfxCtx);
            gDPPipeSync(POLY_OPA_DISP++);
            gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, color[menu->selection][0], color[menu->selection][1],
                            color[menu->selection][2], menu->alpha * play->pauseCtx.alpha / 255);
            gSPVertex(POLY_OPA_DISP++, v, 4, 0);
            gDPLoadTextureBlock(POLY_OPA_DISP++, gMagicArrowEquipEffectTex, G_IM_FMT_IA, G_IM_SIZ_8b, 32, 32, 0,
                                G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMIRROR | G_TX_WRAP, G_TX_NOMASK, G_TX_NOMASK,
                                G_TX_NOLOD, G_TX_NOLOD);
            gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);
        }
    } else {
        uint8_t available = 0;
        for (element = SWORD_ELEMENT_NONE; element < SWORD_ELEMENT_COUNT; ++element) {
            available |= (SwordElement_IsAvailable(element) != 0) << element;
        }
        /* The controller-driven chooser uses a fixed 320x240 canvas. Its fonts,
         * rounded chrome and 64x64 tiled artwork are bundled into the executable. */
        SWORD_KALEIDO_SETUP_OVERLAY(play->state.gfxCtx);
        SwordElementUi_DrawChooser(&OVERLAY_DISP, swordIcon, menu->selection, SwordElement_ForItem(menu->item),
                                   available, play->pauseCtx.alpha);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}
