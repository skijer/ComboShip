#pragma once

#ifdef COMBO_BUILD
#include "2s2h/Enhancements/Gameplay/ComboForestEpona.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Gameplay/ComboForestActors.h"
#include "2s2h/Enhancements/Graphics/EponaCosmetics.h"
#include "objects/object_horse_link_child/object_horse_link_child.h"

static s32 EnHorse_ComboOverride(Actor* actor, PlayState* play, s32 limb, Skin* skin) {
    EnHorse* horse = (EnHorse*)actor;
    static const char* eyes[] = { gEponaEyeOpenTex, gEponaEyeHalfTex, gEponaEyeClosedTex };
    static const u8 sequence[] = { 0, 1, 2, 1 };
    if (limb == 13) {
        OPEN_DISPS(play->state.gfxCtx);
        gSPSegment(POLY_OPA_DISP++, 8, (uintptr_t)eyes[sequence[horse->blinkTimer]]);
        CLOSE_DISPS(play->state.gfxCtx);
    }
    return true;
}

static void EnHorse_ComboDraw(Actor* actor, PlayState* play) {
    EnHorse* horse = (EnHorse*)actor;
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    s32 cosmetics = MMEponaCosmetics_BeginDraw(play, &horse->skin);
    // No native post-draw controller: it registers gameplay colliders from draw.
    func_80138258(actor, play, &horse->skin, NULL, EnHorse_ComboOverride, true);
    if (cosmetics)
        MMEponaCosmetics_EndDraw(play, &horse->skin);
}

static void EnHorse_ComboUpdate(Actor* actor, PlayState* play) {
    EnHorse* horse = (EnHorse*)actor;
    SkelAnime_Update(&horse->skin.skelAnime);
    if (horse->blinkTimer == 0) {
        if (++horse->unk_58C >= 50) {
            horse->unk_58C = 0;
            horse->blinkTimer = 1;
        }
    } else if (++horse->blinkTimer > 3) {
        horse->blinkTimer = 0;
    }
    Collider_UpdateCylinder(actor, &horse->colliderCylinder1);
    Collider_UpdateCylinder(actor, &horse->colliderCylinder2);
    horse->colliderCylinder1.dim.pos.x += (s16)(Math_SinS(actor->shape.rot.y) * 11.0f);
    horse->colliderCylinder1.dim.pos.z += (s16)(Math_CosS(actor->shape.rot.y) * 11.0f);
    horse->colliderCylinder2.dim.pos.x -= (s16)(Math_SinS(actor->shape.rot.y) * 18.0f);
    horse->colliderCylinder2.dim.pos.z -= (s16)(Math_CosS(actor->shape.rot.y) * 18.0f);
    CollisionCheck_SetOC(play, &play->colChkCtx, &horse->colliderCylinder1.base);
    CollisionCheck_SetOC(play, &play->colChkCtx, &horse->colliderCylinder2.base);
    Actor_SetFocus(actor, 55.0f);
}

static void EnHorse_ComboDestroy(Actor* actor, PlayState* play) {
    EnHorse* horse = (EnHorse*)actor;
    Skin_Free(&play->state, &horse->skin);
    Collider_DestroyCylinder(play, &horse->colliderCylinder1);
    Collider_DestroyCylinder(play, &horse->colliderCylinder2);
}

// Return false without touching an ordinary horse; reserved placements never
// reach the native riding, summon, cutscene, race or persistence controllers.
static bool EnHorse_ComboTryInit(Actor* actor, PlayState* play) {
    if (actor->params != COMBO_FOREST_EPONA_PARAMS)
        return false;
    actor->destroy = NULL;
    if (!ComboForestActors_IsExploration(play) || play->roomCtx.curRoom.num != 0) {
        Actor_Kill(actor);
        return true;
    }
    EnHorse* horse = (EnHorse*)actor;
    horse->type = HORSE_TYPE_2;
    horse->stateFlags = ENHORSE_UNRIDEABLE | ENHORSE_CANT_JUMP;
    horse->action = ENHORSE_ACTION_IDLE;
    Skin_Setup(&horse->skin);
    Skin_Init(&play->state, &horse->skin, (SkeletonHeader*)gEponaSkel, (AnimationHeader*)gEponaIdleAnim);
    horse->skin.skelAnime.playSpeed = 2.0f / 3.0f;
    actor->flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED;
    Actor_SetScale(actor, 0.00648f);
    actor->speed = actor->gravity = 0.0f;
    actor->world.rot.x = actor->world.rot.z = 0;
    actor->shape.rot = actor->world.rot;
    actor->colChkInfo.mass = MASS_IMMOVABLE;
    ActorShape_Init(&actor->shape, 0.0f, ActorShadow_DrawCircle, 40.0f);
    ColliderCylinderInit cylinder = {
        { COL_MATERIAL_NONE, AT_NONE, AC_NONE, OC1_ON | OC1_TYPE_ALL, OC2_TYPE_1, COLSHAPE_CYLINDER },
        { ELEM_MATERIAL_UNK0, { 0, 0, 0 }, { 0, 0, 0 }, ATELEM_NONE, ACELEM_NONE, OCELEM_ON },
        { 22, 60, 0, { 0, 0, 0 } },
    };
    Collider_InitCylinder(play, &horse->colliderCylinder1);
    Collider_SetCylinder(play, &horse->colliderCylinder1, actor, &cylinder);
    Collider_InitCylinder(play, &horse->colliderCylinder2);
    Collider_SetCylinder(play, &horse->colliderCylinder2, actor, &cylinder);
    Vec3f probe = actor->world.pos;
    probe.y += 10.0f;
    s32 bgId = BGCHECK_SCENE;
    actor->floorHeight = BgCheck_EntityRaycastFloor5(&play->colCtx, &actor->floorPoly, &bgId, actor, &probe);
    actor->floorBgId = bgId;
    Actor_SetFocus(actor, 55.0f);
    actor->update = EnHorse_ComboUpdate;
    actor->draw = EnHorse_ComboDraw;
    actor->destroy = EnHorse_ComboDestroy;
    return true;
}
#endif
