/**
 * object_elemental_wand.c — the Elemental Wand in Link's hand (Skijer's NEI).
 *
 * Six rods, six models: the export gives each its own opaque + translucent pair, so the staff in
 * the fist IS the active element. Unity-included by item_elemental_wand.c, which owns Wand_IsDrawn.
 */

#include "z64.h"
#include "../custom_items.h"
#include "../helpers/equip_helper.h"
#include "extended_inventory.h" // Wand_GetMode / WAND_MODE_*
#include "macros.h"
#include "functions.h"
#include "soh/Enhancements/randomizer/NeiHeldPresentation.h"
#include "soh/ResourceManagerHelpers.h"
#define QUEST_HELD_IS_MOD ResourceMgr_IsModAsset
#include "quest_held_resources.inc"

// Dialled in game and baked. The staff measures 350 units tall against the slate's 96, which is the
// whole reason its scale is not the slate's. Order: offset XYZ, rotation XYZ, scale.
static const ItemHandPose sWandPose = {
    0.0f, 7.356f, -3.218f, 91.034f, 180.0f, 111.724f, 0.12f,
};

// The original paths remain the resource-pack override points. Built-in lists
// delegate to the matching held meshes; incomplete bundles use retained geometry.
static const QuestHeldModel* const sWandModel[WAND_MODE_COUNT] = {
    &sQuest_sand_rod,   &sQuest_tornado_rod, &sQuest_water_rod,
    &sQuest_meteor_rod, &sQuest_storm_rod,   &sQuest_shadow_scepter,
};

void CustomItems_DrawElementalWand(Player* player, PlayState* play) {
    u8 mode = Wand_GetMode();

    // Before the gate below: both of these are world effects that outlive the staff being out. The
    // bolt is already in flight and the wind burns whether or not the wand is in Link's hand.
    WandShadow_Draw(play);
    WandWind_Draw(player, play);

    if (!Wand_IsDrawn() || (mode >= WAND_MODE_COUNT)) {
        return;
    }
    const QuestHeldModel* model = sWandModel[mode];
    const char* opa;
    const char* xlu;
    QuestHeld_Select(model, &opa, &xlu);
    if (opa != NULL) {
        ItemEquip_DrawHeldModel(player, play, opa, xlu, &sWandPose);
    } else if (xlu != NULL && ItemEquip_ApplyHandPose(player, &sWandPose)) {
        // An XLU-only resource-pack pass stays visible even when its pristine
        // opaque counterpart is unavailable. Keep the same calibrated wrist pose.
        OPEN_DISPS(play->state.gfxCtx);
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_XLU_DISP++, ResourceMgr_LoadGfxByName(xlu));
        CLOSE_DISPS(play->state.gfxCtx);
    }
}
