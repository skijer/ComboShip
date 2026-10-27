#include "MiscBehavior.h"
#include "2s2h/BenGui/HudEditor.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/Rando/RupeeCounterDigits.h"

extern "C" {
#include "interface/parameter_static/parameter_static.h"
extern const char* sCounterTextures[];
}

static void DrawTycoonRupeeIcon() {
    PlayState* play = gPlayState;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    OPEN_DISPS(play->state.gfxCtx);

    gDPSetPrimColorOverride(OVERLAY_DISP++, 0, 0, 220, 120, 255, interfaceCtx->magicAlpha,
                            COSMETIC_ID("HUD.RupeeIcon"));
    gDPSetEnvColor(OVERLAY_DISP++, 50, 0, 80, 255);
    HudEditor_SetActiveElement(HUD_EDITOR_ELEMENT_RUPEE_COUNTER);
    OVERLAY_DISP =
        Gfx_DrawTexRectIA8(OVERLAY_DISP, (TexturePtr)gRupeeCounterIconTex, 16, 16, 26, 206, 16, 16, 1 << 10, 1 << 10);

    CLOSE_DISPS(play->state.gfxCtx);
}

static void DrawTycoonRupeeCounter() {
    PlayState* play = gPlayState;
    InterfaceContext* interfaceCtx = &play->interfaceCtx;

    OPEN_DISPS(play->state.gfxCtx);

    s16 counterDigits[RUPEE_COUNTER_MAX_DIGITS];
    s16 minimumDigits = CUR_CAPACITY(UPG_WALLET) >= 1000 ? 4 : 3;
    s16 digitCount =
        RupeeCounter_BuildDigits(gSaveContext.save.saveInfo.playerData.rupees, minimumDigits, counterDigits);

    s16 magicAlpha = interfaceCtx->magicAlpha;
    if (magicAlpha > 180) {
        magicAlpha = 180;
    }

    // Match the live wallet capacity (ComboShip uses 999); restored balances still expand.
    s16 digitIndex = 0;
    s16 xPos = 42;

    for (s16 i = 0; i < digitCount; i++, digitIndex++, xPos += 8) {
        gDPPipeSync(OVERLAY_DISP++);
        gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 0, 0, magicAlpha);

        HudEditor_SetActiveElement(HUD_EDITOR_ELEMENT_RUPEE_COUNTER);
        OVERLAY_DISP = Gfx_DrawTexRectI8(OVERLAY_DISP, (TexturePtr)sCounterTextures[counterDigits[digitIndex]], 8, 16,
                                         xPos + 1, 207, 8, 16, 1 << 10, 1 << 10);

        gDPPipeSync(OVERLAY_DISP++);

        if (gSaveContext.save.saveInfo.playerData.rupees == CUR_CAPACITY(UPG_WALLET)) {
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 120, 255, 0, interfaceCtx->magicAlpha);
        } else if (gSaveContext.save.saveInfo.playerData.rupees != 0) {
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 255, 255, 255, interfaceCtx->magicAlpha);
        } else {
            gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 100, 100, 100, interfaceCtx->magicAlpha);
        }

        HudEditor_SetActiveElement(HUD_EDITOR_ELEMENT_RUPEE_COUNTER);
        if (HudEditor_ShouldOverrideDraw()) {
            if (CVarGetInteger(hudEditorElements[hudEditorActiveElement].modeCvar, HUD_EDITOR_ELEMENT_MODE_VANILLA) ==
                HUD_EDITOR_ELEMENT_MODE_HIDDEN) {
                hudEditorActiveElement = HUD_EDITOR_ELEMENT_NONE;
            } else {
                s16 rectLeft = xPos;
                s16 rectTop = 824 / 4;
                s16 rectWidth = 0x20 / 4;
                s16 rectHeight = (888 / 4) - rectTop;
                s16 dsdx = 512;
                s16 dtdy = 512;

                HudEditor_ModifyDrawValues(&rectLeft, &rectTop, &rectWidth, &rectHeight, &dsdx, &dtdy);

                hudEditorActiveElement = HUD_EDITOR_ELEMENT_NONE;

                gSPWideTextureRectangle(OVERLAY_DISP++, rectLeft << 2, rectTop << 2, (rectLeft + rectWidth) << 2,
                                        (rectTop + rectHeight) << 2, G_TX_RENDERTILE, 0, 0, dsdx << 1, dtdy << 1);
            }
        } else {
            gSPTextureRectangle(OVERLAY_DISP++, xPos * 4, 206 << 2, (xPos * 4) + 0x20, 222 << 2, G_TX_RENDERTILE, 0, 0,
                                1 << 10, 1 << 10);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

void Rando::MiscBehavior::InitTycoonWallet() {
    bool shouldRegister = IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_TYCOON_WALLET] == RO_GENERIC_YES;

    if (shouldRegister) {
        gUpgradeCapacities[UPG_WALLET][3] = 5000;
    }

    COND_VB_SHOULD(VB_DRAW_RUPEE_ICON, shouldRegister, {
        if (CUR_UPG_VALUE(UPG_WALLET) >= 3) {
            *should = false;
            DrawTycoonRupeeIcon();
        }
    });

    COND_VB_SHOULD(VB_DRAW_RUPEE_COUNTER, shouldRegister, {
        if (CUR_UPG_VALUE(UPG_WALLET) >= 3) {
            *should = false;
            DrawTycoonRupeeCounter();
        }
    });
}
