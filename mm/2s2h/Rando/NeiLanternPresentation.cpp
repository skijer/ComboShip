#include "NeiResourceRouting.h"
#include "NeiLanternPresentation.h"
#include "../../../soh/soh/Enhancements/randomizer/NeiLanternEffectPolicy.h"
#include "../../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "NeiHeldPresentation.h"

extern "C" {
#include "functions.h"
#include "mods/nei_oot_compat.h"
#include "macros.h"
#include "mods/items/custom_items.h"
#include "mods/extended_player.h"
#include "mods/items/helpers/equip_helper.h"

static const char kLanternOpaque[] = "__OTR__objects/nei_gi_redesign/lantern/gi_dl";
static const char kLanternGlass[] = "__OTR__objects/nei_gi_redesign/lantern/gi_xlu_dl";

static void NeiLantern_DrawPass(PlayState* play, bool glass) {
    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Push();
    // The shared GI display list multiplies serialized vertices by its native
    // 1.25 matrix. Convert that frame back to author units before held placement.
    Matrix_Scale(NeiLantern::ModelScale, NeiLantern::ModelScale, NeiLantern::ModelScale, MTXMODE_APPLY);
    if (glass) {
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, NeiResource_Route(kLanternGlass), 0, G_DL_PUSH);
    } else {
        Gfx_SetupDL_25Opa(play->state.gfxCtx);
        gSPMatrix(POLY_OPA_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, NeiResource_Route(kLanternOpaque), 0, G_DL_PUSH);
    }
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}

bool NeiLantern_UsesGrip(const Player* player) {
    if (!player || player->transformation != PLAYER_FORM_HUMAN ||
        !(gCustomItemState.lanternEquipped || gCustomItemState.lanternSwinging) ||
        (player->heldItemAction != PLAYER_IA_NONE && player->heldItemAction != PLAYER_IA_LANTERN)) {
        return false;
    }
    for (u8 slot = 0; slot < 8; ++slot) {
        if (ItemEquip_GetItemOnSlot(slot) == ITEM_LANTERN) {
            return true;
        }
    }
    return false;
}

bool NeiLantern_DrawHeld(Player* player, PlayState* play, uint8_t fireType) {
    if (player == nullptr || play == nullptr || !NeiHeld_HasResources(kLanternOpaque, kLanternGlass))
        return false;

    const Vec3f hand = player->bodyPartsPos[PLAYER_BODYPART_L_HAND];
    Matrix_Push();
    Matrix_Translate(hand.x, hand.y, hand.z, MTXMODE_NEW);
    Matrix_RotateY(player->actor.shape.rot.y * (M_PI / 32768.f), MTXMODE_APPLY);
    Matrix_Scale(NeiLantern::HeldScale, NeiLantern::HeldScale, NeiLantern::HeldScale, MTXMODE_APPLY);
    Matrix_Translate(0, -NeiLantern::GripY, 0, MTXMODE_APPLY);

    NeiLantern_DrawPass(play, false);
    const auto camera = NeiGi_CameraBasis(play);
    NeiGi_DrawMesh(play, NeiLantern::SampleCore(fireType, play->gameplayFrames, camera),
                   NeiLantern::CoreTexture(fireType));
    NeiGi_DrawMesh(play, NeiLantern::SampleAccents(fireType, play->gameplayFrames, camera));
    NeiLantern_DrawPass(play, true);
    Matrix_Pop();
    return true;
}
}
