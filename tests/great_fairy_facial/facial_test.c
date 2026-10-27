/* Real MM types and exact production function bodies, with services recorded
 * at the resource/animation/cutscene boundary. No fixture copy of the adapter. */
#include <stdio.h>
#include <string.h>
#include "tests/test_require.h"
#include "src/overlays/actors/ovl_Bg_Dy_Yoseizo/z_bg_dy_yoseizo.h"
#include "2s2h/BenPort.h"

static int altEnabled = 1, packPresent = 1, missing = -1, failed = -1;
static int resourceLoads, nativeActions, movementCalls, effectCalls, animationUpdates, animationChanges;
static int cueId, animationFinished;
static AnimationHeader* lastAnimation;
static Gfx heads[6][1], nativeHead[1];
static _Alignas(2) const char selectedHead[] =
    "__OTR__objects/object_dy_obj/HWGreatFairyMMPOC2_BaseHeadDL";
static _Alignas(2) const char unrelatedHead[] = "__OTR__objects/another_pack/HeadDL";

static int headIndex(const char* path) {
    if (strcmp(path, "alt/objects/object_dy_obj/HWGreatFairyMMPOC2_BaseHeadDL") == 0) return 6;
    int eye = -1, mouth = -1;
    REQUIRE(sscanf(path, "alt/objects/object_dy_obj/HWGreatFairyMMPOC2_Eye%dMouth%dDL", &eye, &mouth) == 2);
    REQUIRE(eye >= 0 && eye < 3 && mouth >= 0 && mouth < 2);
    return mouth * 3 + eye;
}
bool ResourceMgr_IsAltAssetsEnabled(void) { return altEnabled; }
uint8_t ResourceMgr_FileExists(const char* path) { return packPresent && headIndex(path) != missing; }
Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    ++resourceLoads;
    int index = headIndex(path);
    REQUIRE(packPresent && index != missing);
    return index == failed ? NULL : heads[index];
}
int ResourceMgr_OTRSigCheck(char* pointer) {
    return pointer != NULL && memcmp(pointer, "__OTR__", 7) == 0;
}
void* Lib_SegmentedToVirtual(void* pointer) { return pointer; }
f32 Rand_ZeroFloat(f32 max) { REQUIRE(max == 60.0f); return 7.0f; }
void Actor_MoveWithGravity(Actor* actor) { ++movementCalls; }
void BgDyYoseizo_UpdateEffects(BgDyYoseizo* actor, PlayState* play) { ++effectCalls; }
void BgDyYoseizo_Bob(BgDyYoseizo* actor, PlayState* play) {}
s32 SkelAnime_Update(SkelAnime* skelAnime) { ++animationUpdates; return animationFinished; }
s32 Cutscene_IsCueInChannel(PlayState* play, u16 type) {
    REQUIRE(type == CS_CMD_ACTOR_CUE_103);
    return cueId != 0;
}
s32 Cutscene_GetCueChannel(PlayState* play, u16 type) { return 0; }
s16 Animation_GetLastFrame(void* animation) { return 44; }
void Animation_Change(SkelAnime* skeleton, AnimationHeader* animation, f32 speed, f32 start, f32 end,
                      u8 mode, f32 morph) {
    ++animationChanges;
    lastAnimation = animation;
}
void Actor_PlaySfx(Actor* actor, u16 sfx) {}
static void ordinaryAction(BgDyYoseizo* actor, PlayState* play) { ++nativeActions; }
void func_80A0B500(BgDyYoseizo* actor, PlayState* play) { ++nativeActions; }
void func_80A0AFDC(BgDyYoseizo* actor) { actor->actionFunc = ordinaryAction; }

#include "great_fairy_production.inc"

static void resetServices(void) {
    altEnabled = packPresent = 1;
    missing = failed = -1;
    resourceLoads = nativeActions = movementCalls = effectCalls = 0;
    animationUpdates = animationChanges = cueId = animationFinished = 0;
}

int main(void) {
    BgDyYoseizo fairy = { 0 };
    PlayState play = { 0 };
    StandardLimb head = { 0 };
    void* skeleton[27] = { 0 };
    CsCmdActorCue cue = { 0 };
    play.csCtx.actorCues[0] = &cue;
    skeleton[GREAT_FAIRY_LIMB_HEAD - 1] = &head;
    head.dList = (Gfx*)selectedHead;
    fairy.skelAnime.skeleton = skeleton;
    fairy.skelAnime.limbCount = GREAT_FAIRY_LIMB_MAX;
    fairy.skelAnime.dListCount = GREAT_FAIRY_LIMB_MAX - 1;
    fairy.actor.draw = (ActorFunc)ordinaryAction;
    fairy.headRot.y = 90;
    fairy.headRot.z = -30;
    fairy.torsoRot.y = 45;

    REQUIRE(BgDyYoseizo_UsesMMFacialHeads(&fairy));
    for (int mouth = 0; mouth < 2; ++mouth) {
        for (int eye = 0; eye < 3; ++eye) {
            fairy.eyeIndex = eye;
            fairy.mouthIndex = mouth;
            Gfx* list = nativeHead;
            Vec3f position = { 1, 2, 3 };
            Vec3s rotation = { 100, 200, 300 };
            REQUIRE(!BgDyYoseizo_OverrideLimbDraw(&play, GREAT_FAIRY_LIMB_HEAD, &list, &position, &rotation,
                                                &fairy.actor));
            REQUIRE(list == heads[mouth * 3 + eye]);
            REQUIRE(rotation.x == 190 && rotation.y == 200 && rotation.z == 270);
            REQUIRE(position.x == 1 && position.y == 2 && position.z == 3);
        }
    }
    for (int limb = 1; limb < GREAT_FAIRY_LIMB_MAX; ++limb) {
        if (limb == GREAT_FAIRY_LIMB_HEAD) continue;
        Gfx* list = nativeHead;
        Vec3f position = { 1, 2, 3 };
        Vec3s rotation = { 100, 200, 300 };
        int loads = resourceLoads;
        REQUIRE(!BgDyYoseizo_OverrideLimbDraw(&play, limb, &list, &position, &rotation, &fairy.actor));
        REQUIRE(list == nativeHead && resourceLoads == loads);
        REQUIRE(rotation.x == (limb == GREAT_FAIRY_LIMB_TORSO ? 145 : 100));
        REQUIRE(rotation.y == 200 && rotation.z == 300);
    }
    fairy.eyeIndex = -1;
    fairy.mouthIndex = 7;
    REQUIRE(BgDyYoseizo_LoadMMFacialHead(&fairy) == heads[0]);
    fairy.eyeIndex = 100;
    fairy.mouthIndex = -1;
    REQUIRE(BgDyYoseizo_LoadMMFacialHead(&fairy) == heads[0]);
    puts("PASS six mouth/blink combinations, bounded indices, head-only selection and native tracking");

    fairy.eyeIndex = 2;
    fairy.mouthIndex = 1;
    for (int reason = 0; reason < 9; ++reason) {
        resetServices();
        if (reason == 0) altEnabled = 0;
        if (reason == 1) packPresent = 0;
        if (reason == 2) missing = 5;
        if (reason == 3) failed = 5;
        if (reason == 4) head.dList = (Gfx*)unrelatedHead;
        if (reason == 5) head.dList = nativeHead;
        if (reason == 6) fairy.skelAnime.skeleton = NULL;
        if (reason == 7) fairy.skelAnime.limbCount = 10;
        if (reason == 8) fairy.skelAnime.dListCount = 9;
        Gfx* list = nativeHead;
        Vec3f position = { 0 };
        Vec3s rotation = { 0 };
        REQUIRE(!BgDyYoseizo_OverrideLimbDraw(&play, GREAT_FAIRY_LIMB_HEAD, &list, &position, &rotation,
                                            &fairy.actor));
        REQUIRE(list == nativeHead);
        REQUIRE(resourceLoads == (reason == 3 ? 1 : 0));
        head.dList = (Gfx*)selectedHead;
        fairy.skelAnime.skeleton = skeleton;
        fairy.skelAnime.limbCount = GREAT_FAIRY_LIMB_MAX;
        fairy.skelAnime.dListCount = GREAT_FAIRY_LIMB_MAX - 1;
    }
    puts("PASS Alt off, missing/unloaded heads, another model and invalid matrix capacity preserve original draw");

    resetServices();
    fairy.eyeIndex = 0;
    fairy.mouthIndex = 0;
    fairy.blinkTimer = 2;
    fairy.timer = 10;
    fairy.actionFunc = ordinaryAction;
    BgDyYoseizo_Update(&fairy.actor, &play);
    REQUIRE(fairy.blinkTimer == 1 && fairy.eyeIndex == 0);
    REQUIRE(nativeActions == 1 && movementCalls == 1 && effectCalls == 1 && fairy.timer == 9);
    BgDyYoseizo_Update(&fairy.actor, &play);
    REQUIRE(fairy.blinkTimer == 0 && fairy.eyeIndex == 1);
    BgDyYoseizo_Update(&fairy.actor, &play);
    REQUIRE(fairy.eyeIndex == 2);
    BgDyYoseizo_Update(&fairy.actor, &play);
    REQUIRE(fairy.eyeIndex == 0 && fairy.blinkTimer == 27);
    REQUIRE(fairy.mouthIndex == 0);

    // The native reclining action already updates eyes, including the frame
    // on which it switches to the upgrade animation. Neither frame may tick twice.
    fairy.actionFunc = func_80A0B5F0;
    fairy.blinkTimer = 2;
    cue.id = cueId = 0;
    BgDyYoseizo_Update(&fairy.actor, &play);
    REQUIRE(fairy.blinkTimer == 1 && animationUpdates == 1);
    fairy.blinkTimer = 2;
    cue.id = cueId = 5;
    BgDyYoseizo_Update(&fairy.actor, &play);
    REQUIRE(fairy.blinkTimer == 1 && animationUpdates == 2);
    REQUIRE(fairy.actionFunc == func_80A0B500 && fairy.mouthIndex == 1);
    REQUIRE(animationChanges == 1 && lastAnimation == sAnimations[GREATFAIRY_ANIM_START_GIVING_UPGRADE]);
    BgDyYoseizo_Update(&fairy.actor, &play);
    REQUIRE(fairy.blinkTimer == 0 && fairy.eyeIndex == 1 && fairy.mouthIndex == 1);

    // Original native eyes continue in their native action with no custom pack.
    altEnabled = 0;
    fairy.actionFunc = func_80A0B5F0;
    fairy.blinkTimer = 2;
    cue.id = cueId = 0;
    BgDyYoseizo_Update(&fairy.actor, &play);
    REQUIRE(fairy.blinkTimer == 1);
    fairy.actionFunc = ordinaryAction;
    fairy.blinkTimer = 2;
    BgDyYoseizo_Update(&fairy.actor, &play);
    REQUIRE(fairy.blinkTimer == 2);
    altEnabled = 1;
    fairy.actor.draw = NULL;
    BgDyYoseizo_Update(&fairy.actor, &play);
    REQUIRE(fairy.blinkTimer == 2);
    puts("PASS blink progression, no duplicate tick on native action/transition, native mouth cue and hidden/vanilla behavior");
    return 0;
}
