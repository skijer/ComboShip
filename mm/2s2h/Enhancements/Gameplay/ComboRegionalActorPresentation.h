#pragma once
#ifdef COMBO_BUILD
#include "ComboRegionalActors.h"
#include "actors/RegionalActorFocus.h"
#include "overlays/actors/ovl_En_Rail_Skb/z_en_rail_skb.h"
#include "overlays/actors/ovl_En_Zog/z_en_zog.h"
#include "overlays/actors/ovl_En_Gg/z_en_gg.h"

extern uint8_t ResourceMgr_FileExists(const char* path);
extern void ResourceMgr_UnregisterSkeleton(SkelAnime* skelAnime);

static SkelAnime* ComboRegionalActor_Skeleton(Actor* actor) {
    if (actor->id == ACTOR_EN_RAIL_SKB)
        return &((EnRailSkb*)actor)->skelAnime;
    if (actor->id == ACTOR_EN_ZOG)
        return &((EnZog*)actor)->skelAnime;
    return &((EnGg*)actor)->skelAnime;
}
static void ComboRegionalActor_Focus(Actor* actor, s32 limb) {
    const unsigned family = actor->id == ACTOR_EN_RAIL_SKB ? COMBO_FOCUS_STALCHILD
                            : actor->id == ACTOR_EN_ZOG    ? COMBO_FOCUS_MIKAU
                                                           : COMBO_FOCUS_DARMANI;
    if (limb == ComboRegionalFocus_Limb(family)) {
        Vec3f offset = { 0, ComboRegionalFocus_OffsetY(family), 0 };
        Matrix_MultVec3f(&offset, &actor->focus.pos);
    }
}
static void ComboRegionalActor_Post(PlayState* play, s32 limb, Gfx** list, Vec3s* rot, Actor* actor) {
    ComboRegionalActor_Focus(actor, limb);
    if (actor->id != ACTOR_EN_ZOG || ((actor->params >> 4) & 15) != 1 || limb != OBJECT_ZOG_LIMB_11)
        return;
    OPEN_DISPS(play->state.gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)object_zog_DL_0280A8);
    CLOSE_DISPS(play->state.gfxCtx);
}
static void ComboRegionalActor_PostXlu(PlayState* play, s32 limb, Gfx** list, Vec3s* rot, Actor* actor, Gfx** gfx) {
    ComboRegionalActor_Focus(actor, limb);
}
static void ComboRegionalActor_Draw(Actor* actor, PlayState* play) {
    SkelAnime* skel = ComboRegionalActor_Skeleton(actor);
    if (actor->id == ACTOR_EN_GG) {
        OPEN_DISPS(play->state.gfxCtx);
        Gfx_SetupDL25_Xlu(play->state.gfxCtx);
        gSPSegment(POLY_XLU_DISP++, 8, (uintptr_t)object_gg_Tex_009260);
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 176);
        POLY_XLU_DISP = SkelAnime_DrawFlex(play, skel->skeleton, skel->jointTable, skel->dListCount, NULL,
                                           ComboRegionalActor_PostXlu, actor, POLY_XLU_DISP);
        CLOSE_DISPS(play->state.gfxCtx);
        return;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    if (actor->id == ACTOR_EN_RAIL_SKB) {
        SkelAnime_DrawOpa(play, skel->skeleton, skel->jointTable, NULL, ComboRegionalActor_Post, actor);
    } else {
        Scene_SetRenderModeXlu(play, 0, 1);
        gSPSegment(POLY_OPA_DISP++, 8, (uintptr_t)object_zog_Tex_024750);
        gSPSegment(POLY_OPA_DISP++, 9, (uintptr_t)object_zog_Tex_025F50);
        SkelAnime_DrawFlexOpa(play, skel->skeleton, skel->jointTable, skel->dListCount, NULL, ComboRegionalActor_Post,
                              actor);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}
static void ComboRegionalActor_Update(Actor* actor, PlayState* play) {
    SkelAnime_Update(ComboRegionalActor_Skeleton(actor));
}
static void ComboRegionalActor_Destroy(Actor* actor, PlayState* play) {
    /* Fixed joint arrays are embedded in the original actor; never free them. */
    ResourceMgr_UnregisterSkeleton(ComboRegionalActor_Skeleton(actor));
}
static bool ComboRegionalActor_TryInit(Actor* actor, PlayState* play) {
    if (!ComboRegionalActors_IsPlaced(actor->id, actor->params))
        return false;
    const unsigned pose = ((u16)actor->params >> 4) & 15;
    const char* skeleton;
    const char* animation;
    Vec3s *joints, *morph;
    unsigned count;
    SkelAnime* skel = ComboRegionalActor_Skeleton(actor);
    actor->destroy = NULL;
    if (actor->id == ACTOR_EN_RAIL_SKB) {
        static const char* clips[] = { gStalchildIdleAnim, gStalchildSitLaughAnim, gStalchildSitTapToesAnim,
                                       gStalchildSwingOnBranchAnim, gStalchildSaluteAnim };
        EnRailSkb* child = (EnRailSkb*)actor;
        skeleton = gStalchildSkel;
        animation = clips[pose];
        count = STALCHILD_LIMB_MAX;
        joints = child->jointTable;
        morph = child->morphTable;
    } else if (actor->id == ACTOR_EN_ZOG) {
        EnZog* zora = (EnZog*)actor;
        skeleton = object_zog_Skel_029170;
        animation = pose ? object_zog_Anim_00931C : object_zog_Anim_0041D0;
        count = OBJECT_ZOG_LIMB_MAX;
        joints = zora->jointTable;
        morph = zora->morphTable;
        if (!ResourceMgr_FileExists(dobject_zog_Tex_024750 + 7) ||
            !ResourceMgr_FileExists(dobject_zog_Tex_025F50 + 7) ||
            (pose && !ResourceMgr_FileExists(dobject_zog_DL_0280A8 + 7))) {
            Actor_Kill(actor);
            return true;
        }
    } else {
        EnGg* ghost = (EnGg*)actor;
        skeleton = object_gg_Skel_00F6C0;
        animation = pose ? object_gg_Anim_00AF40 : object_gg_Anim_00F578;
        count = OBJECT_GG_LIMB_MAX;
        joints = ghost->jointTable;
        morph = ghost->morphTable;
        if (!ResourceMgr_FileExists(dobject_gg_Tex_009260 + 7)) {
            Actor_Kill(actor);
            return true;
        }
    }
    if (!ResourceMgr_FileExists(skeleton + 7) || !ResourceMgr_FileExists(animation + 7)) {
        Actor_Kill(actor);
        return true;
    }
    if (actor->id == ACTOR_EN_RAIL_SKB)
        SkelAnime_Init(play, skel, (SkeletonHeader*)skeleton, (AnimationHeader*)animation, joints, morph, count);
    else
        SkelAnime_InitFlex(play, skel, (FlexSkeletonHeader*)skeleton, (AnimationHeader*)animation, joints, morph,
                           count);
    Actor_ChangeCategory(play, &play->actorCtx, actor, ACTORCAT_NPC);
    actor->flags = ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_FRIENDLY;
    actor->speed = actor->gravity = 0;
    actor->world.rot.x = actor->world.rot.z = 0;
    actor->shape.rot = actor->world.rot;
    Actor_SetScale(actor, 0.01f);
    Actor_SetFocus(actor, 55.0f);
    ActorShape_Init(&actor->shape, 0, NULL, 0);
    actor->colChkInfo.mass = MASS_IMMOVABLE;
    actor->update = ComboRegionalActor_Update;
    actor->draw = ComboRegionalActor_Draw;
    actor->destroy = ComboRegionalActor_Destroy;
    return true;
}
#endif
