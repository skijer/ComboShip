#ifndef COMBO_FAIRY_BOTTLE_DRAW_H
#define COMBO_FAIRY_BOTTLE_DRAW_H

// GI-local actor-derived fairy effect. Include after the host's engine headers.
// No EnElf actor is spawned and no companion/Midna resources are patched.
#include "ComboFairyBottle.h"

#ifdef __cplusplus
extern "C" {
uint8_t ResourceMgr_FileExists(const char* path);
uint8_t ResourceMgr_FileAltExists(const char* path);
bool ResourceMgr_IsAltAssetsEnabled(void);
SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path, SkelAnime* skelAnime);
AnimationHeaderCommon* ResourceMgr_LoadAnimByName(const char* path);
}
#endif

static s32 ComboFairyBottle_GlowLimb(PlayState* play, s32 limb, Gfx** dList, Vec3f* pos, Vec3s* rot,
#ifdef COMBO_FAIRY_HOST_MM
                                     Actor* actor,
#else
                                     void* actor,
#endif
                                     Gfx** gfx) {
#ifdef COMBO_FAIRY_HOST_MM
    const s32 glowLimb = 6;
#else
    const s32 glowLimb = 8;
#endif
    if (limb == glowLimb) {
        const float pulse = 1.5f + .15f * sinf((float)(play->gameplayFrames % 16u) * .3926990817f);
        // The native core DL multiplies segment 01 by the camera billboard.
        // Remove the limb's rotation here, as EnElf does, so that rotation is
        // applied once. Preserve the GI position and incoming scale.
        MtxF identity = { 0 };
        identity.xx = identity.yy = identity.zz = identity.ww = 1.0f;
        Matrix_ReplaceRotation(&identity);
        Matrix_Scale(pulse, pulse, pulse, MTXMODE_APPLY);
    }
    return 0;
}

static int ComboFairyBottle_DrawVfx(PlayState* play, const char* shell) {
#ifdef COMBO_FAIRY_HOST_MM
    const char* skeletonPath = "__OTR__objects/gameplay_keep/gameplay_keep_Skel_02AF58";
    const char* animationPath = "__OTR__objects/gameplay_keep/gameplay_keep_Anim_029140";
    const s32 limbCount = 7;
#else
    const char* skeletonPath = "__OTR__objects/gameplay_keep/gFairySkel";
    const char* animationPath = "__OTR__objects/gameplay_keep/gFairyAnim";
    const s32 limbCount = 15;
#endif
    SkeletonHeader* skeleton;
    AnimationHeader* animation;
    SkelAnime skel = { 0 };
    Vec3s joints[15] = { 0 }, morph[15] = { 0 };
    Gfx* material;
    s16 lastFrame;
    if ((!ResourceMgr_FileExists(skeletonPath) &&
         !(ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(skeletonPath))) ||
        (!ResourceMgr_FileExists(animationPath) &&
         !(ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(animationPath)))) {
        return 0;
    }
    // Load raw headers without registering a temporary SkelAnime with hot-reload.
    skeleton = ResourceMgr_LoadSkeletonByName(skeletonPath, NULL);
    animation = (AnimationHeader*)ResourceMgr_LoadAnimByName(animationPath);
    if (skeleton == NULL || animation == NULL || skeleton->limbCount + 1 != limbCount) {
        return 0;
    }
    lastFrame = Animation_GetLastFrame(animation);
    if (lastFrame < 0) {
        return 0;
    }
    SkelAnime_Init(play, &skel, skeleton, animation, joints, morph, limbCount);
    Animation_Change(&skel, animation, 0.0f, (float)(play->gameplayFrames % (uint32_t)(lastFrame + 1)),
                     (float)lastFrame, ANIMMODE_LOOP, 0.0f);

#ifdef COMBO_FAIRY_HOST_MM
    material = (Gfx*)GRAPH_ALLOC(play->state.gfxCtx, sizeof(Gfx) * 5);
#else
    material = (Gfx*)Graph_Alloc(play->state.gfxCtx, sizeof(Gfx) * 5);
#endif
    if (material == NULL) {
        return 0;
    }
    gDPPipeSync(&material[0]);
    gDPSetPrimColor(&material[1], 0, 1, 255, 160, 235, 255);
    gDPSetRenderMode(&material[2], G_RM_PASS, G_RM_ZB_CLD_SURF2);
    gSPEndDisplayList(&material[3]);
    gSPEndDisplayList(&material[4]);
    Matrix_Push();
    // Enlarge only the GI-local fairy; the casing and actor assets retain their scale.
    const float scale = (ComboFairyBottle_IsBundledShell(shell) ? .008f : .004f) * 1.5f;
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    OPEN_DISPS(play->state.gfxCtx);
#ifdef COMBO_FAIRY_HOST_MM
    Gfx_SetupDL27_Xlu(play->state.gfxCtx);
#else
    Gfx_SetupDL_27Xlu(play->state.gfxCtx);
#endif
#ifdef COMBO_BUILD
    // The shell may have pinned the other game's RM on the GPU stream. These
    // limbs belong to the active host, including their nested textures/DLs.
#ifdef COMBO_FAIRY_HOST_MM
    gSPComboRMPush(POLY_XLU_DISP++, "mm");
#else
    gSPComboRMPush(POLY_XLU_DISP++, "oot");
#endif
#endif
    gSPSegment(POLY_XLU_DISP++, 0x08, (uintptr_t)material);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 160, 235, 192);
#ifdef COMBO_FAIRY_HOST_MM
    POLY_XLU_DISP =
        SkelAnime_Draw(play, skel.skeleton, skel.jointTable, ComboFairyBottle_GlowLimb, NULL, NULL, POLY_XLU_DISP);
#else
    if (skeleton->skeletonType == SKELANIME_TYPE_FLEX) {
        POLY_XLU_DISP =
            SkelAnime_DrawFlex(play, skel.skeleton, skel.jointTable, ((FlexSkeletonHeader*)skeleton)->dListCount,
                               ComboFairyBottle_GlowLimb, NULL, NULL, POLY_XLU_DISP);
    } else {
        POLY_XLU_DISP =
            SkelAnime_Draw(play, skel.skeleton, skel.jointTable, ComboFairyBottle_GlowLimb, NULL, NULL, POLY_XLU_DISP);
    }
#endif
    gSPSegment(POLY_XLU_DISP++, 0x08, (uintptr_t)&material[4]);
#ifdef COMBO_BUILD
    gSPComboRMPop(POLY_XLU_DISP++);
#endif
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
    return 1;
}

#endif
