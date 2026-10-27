#pragma once

#ifdef COMBO_BUILD
#include "2s2h/Enhancements/Gameplay/ComboForestSkullKid.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Gameplay/ComboForestActors.h"
#include "objects/object_stk2/object_stk2.h"

// Fixed joint arrays belong to DmStk; only unregister the native skeleton.
extern void ResourceMgr_UnregisterSkeleton(SkelAnime* skelAnime);

static s32 DmStk_ComboOverride(PlayState* play, s32 limb, Gfx** display, Vec3f* pos, Vec3s* rot, Actor* actor) {
    if (limb == SKULL_KID_LIMB_HEAD || limb == SKULL_KID_LIMB_LEFT_HAND)
        *display = NULL;
    return false;
}

static void DmStk_ComboPost(PlayState* play, s32 limb, Gfx** display, Vec3s* rot, Actor* actor) {
    if (limb != SKULL_KID_LIMB_HEAD && limb != SKULL_KID_LIMB_LEFT_HAND)
        return;
    OPEN_DISPS(play->state.gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    if (limb == SKULL_KID_LIMB_HEAD) {
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gSkullKidNormalHeadDL);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gSkullKidNormalEyesDL);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gSkullKidMajorasMask1DL);
    } else {
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)gSkullKidOpenLeftHandDL);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

static void DmStk_ComboDraw(Actor* actor, PlayState* play) {
    DmStk* kid = (DmStk*)actor;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Scene_SetRenderModeXlu(play, 0, 1);
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    SkelAnime_DrawFlexOpa(play, kid->skelAnime.skeleton, kid->skelAnime.jointTable, kid->skelAnime.dListCount,
                          DmStk_ComboOverride, DmStk_ComboPost, actor);
    CLOSE_DISPS(play->state.gfxCtx);
}

static void DmStk_ComboUpdate(Actor* actor, PlayState* play) {
    DmStk* kid = (DmStk*)actor;
    SkelAnime_Update(&kid->skelAnime);
    kid->bobPhase += 0x400;
    actor->shape.yOffset = Math_SinS(kid->bobPhase) * 1000.0f;
    Collider_UpdateCylinder(actor, &kid->collider);
    CollisionCheck_SetOC(play, &play->colChkCtx, &kid->collider.base);
    Actor_SetFocus(actor, 55.0f);
}

static void DmStk_ComboDestroy(Actor* actor, PlayState* play) {
    DmStk* kid = (DmStk*)actor;
    ResourceMgr_UnregisterSkeleton(&kid->skelAnime);
    Collider_DestroyCylinder(play, &kid->collider);
}

static bool DmStk_ComboTryInit(Actor* actor, PlayState* play) {
    if (actor->params != COMBO_FOREST_SKULL_KID_PARAMS)
        return false;
    actor->destroy = NULL;
    if (!ComboForestActors_IsExploration(play) || play->roomCtx.curRoom.num != 1) {
        Actor_Kill(actor);
        return true;
    }
    DmStk* kid = (DmStk*)actor;
    SkelAnime_InitFlex(play, &kid->skelAnime, (FlexSkeletonHeader*)gSkullKidSkel,
                       (AnimationHeader*)gSkullKidRecliningFloatAnim, kid->jointTable, kid->morphTable,
                       SKULL_KID_LIMB_MAX);
    actor->flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    actor->speed = actor->gravity = 0.0f;
    actor->world.rot.x = actor->world.rot.z = 0;
    actor->shape.rot = actor->world.rot;
    Actor_SetScale(actor, 0.01f);
    ActorShape_Init(&actor->shape, 0.0f, ActorShadow_DrawCircle, 24.0f);
    actor->colChkInfo.mass = MASS_IMMOVABLE;
    ColliderCylinderInit cylinder = {
        {COL_MATERIAL_NONE, AT_NONE, AC_NONE, OC1_ON | OC1_TYPE_ALL, OC2_TYPE_1, COLSHAPE_CYLINDER},
        {ELEM_MATERIAL_UNK0, {0, 0, 0}, {0, 0, 0}, ATELEM_NONE, ACELEM_NONE, OCELEM_ON},
        {20, 50, 0, {0, 0, 0}},
    };
    Collider_InitCylinder(play, &kid->collider);
    Collider_SetCylinder(play, &kid->collider, actor, &cylinder);
    Vec3f probe = actor->world.pos;
    probe.y += 10.0f;
    s32 bgId = BGCHECK_SCENE;
    actor->floorHeight = BgCheck_EntityRaycastFloor5(&play->colCtx, &actor->floorPoly, &bgId, actor, &probe);
    actor->floorBgId = bgId;
    Actor_SetFocus(actor, 55.0f);
    actor->update = DmStk_ComboUpdate;
    actor->draw = DmStk_ComboDraw;
    actor->destroy = DmStk_ComboDestroy;
    return true;
}
#endif
