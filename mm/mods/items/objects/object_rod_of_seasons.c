#include "2s2h/Rando/NeiHeldPresentation.h"
#include "2s2h/Rando/NeiResourceRouting.h"

void CustomItems_DrawRodOfSeasons(Player* player, PlayState* play) {
    static const ItemHandPose pose = { 0.0f, 5.977f, -3.218f, 109.655f, 72.414f, 0.0f, 0.4f };
    const char* original = "__OTR__objects/object_nei_rod_of_seasons/gNeiRodOfSeasonsDL";
    const char* redesigned = NEI_HELD_PATH("rod_of_seasons");
    if (!Seasons_IsDrawn()) {
        return;
    }
    // Matching user packs remain authoritative; retain the native object as the fallback.
    const char* model = NeiResource_IsMod(original) || !NeiHeld_HasResources(redesigned, NULL) ? original : redesigned;
    Matrix_Push();
    if (ItemEquip_ApplyHandPose(player, &pose)) {
        NeiHeld_DrawModel(play, model, NULL);
    }
    Matrix_Pop();
}
