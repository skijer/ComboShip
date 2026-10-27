/* Matching quest wand in MM's captured right wrist. World effects stay in Wand_Draw. */
#include "z64.h"
#include "../helpers/equip_helper.h"
#include "../logic/adult_link_render.h"
#include "mods/extended_inventory.h"
#include "2s2h/Rando/NeiHeldPresentation.h"
#include "2s2h/Rando/NeiResourceRouting.h"
#define QUEST_HELD_IS_MOD NeiResource_IsMod
#include "quest_held_resources.inc"
#include "functions.h"

extern u8 Wand_IsDrawn(void);

static const QuestHeldModel* const sMmWandModel[WAND_MODE_COUNT] = {
    &sQuest_sand_rod,   &sQuest_tornado_rod, &sQuest_water_rod,
    &sQuest_meteor_rod, &sQuest_storm_rod,   &sQuest_shadow_scepter,
};

void CustomItems_DrawElementalWand(Player* player, PlayState* play) {
    if (!Wand_IsDrawn())
        return;
    u8 mode = Wand_GetMode();
    if (mode >= WAND_MODE_COUNT)
        return;
    const QuestHeldModel* model = sMmWandModel[mode];
    const char* opa;
    const char* xlu;
    QuestHeld_Select(model, &opa, &xlu);
    if (opa == NULL && xlu == NULL)
        return;

    // The measured native child/adult fist socket is the same canonical grip
    // frame as MM Somaria. +Y staff becomes wrist +X; restore the exact legacy
    // grip center -45.5 after the .12 native-to-world scale. OoT rotations are
    // deliberately absent: these hosts have different wrist frames.
    u8 adult = AdultLink_UsesAdultPresentation(player);
    ItemHandPose pose = {
        -(adult ? 328.0f : 216.22f) * player->actor.scale.x,
        45.5f * .12f,
        (adult ? 77.0f : -4.5f) * player->actor.scale.x,
        0.0f,
        0.0f,
        -90.0f,
        .12f,
    };
    Matrix_Push();
    if (ItemEquip_ApplyHandPose(player, &pose)) {
        // NeiHeld_DrawModel routes these original override points through OoT
        // ownership in ComboShip, including active Alt resource packs.
        if (opa != NULL) {
            NeiHeld_DrawModel(play, opa, xlu);
        } else {
            // Retain a custom XLU pass independently of unavailable native OPA.
            OPEN_DISPS(play->state.gfxCtx);
            Gfx_SetupDL25_Xlu(play->state.gfxCtx);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
            gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, NeiResource_Route(xlu), 0, G_DL_PUSH);
            CLOSE_DISPS(play->state.gfxCtx);
        }
    }
    Matrix_Pop();
}
