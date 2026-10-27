#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "global.h"
#include "align_asset_macro.h"
void SkelAnime_AnimateFrame(SkelAnime* skelAnime);
#include "soh/Enhancements/cosmetics/NativeNpcPose.h"

static const ALIGN_ASSET(2) char nativePath[] = "__OTR__objects/object_os_anime/gKokiriStandingArmsBehindBackAnim";
static s16 nativeValues[] = { 10, 20, 30, 100, 200, 300, 400 };
static s16 fittedValues[] = { 10, 20, 30, 1100, 1200, 1300, 1400 };
static JointIndex indices[] = { { 0, 1, 2 }, { 3, 1, 2 }, { 5, 1, 2 } };
static AnimationHeader nativeClip = { { 2 }, nativeValues, indices, 3 };
static AnimationHeader fittedClip = { { 2 }, fittedValues, indices, 3 };
static bool fittedEnabled;
static GameInfo gameInfo;
GameInfo* gGameInfo = &gameInfo;
uintptr_t gSegments[16];

AnimationHeader* NativeNpcPose_ResolveAnimation(const SkelAnime* skel, AnimationHeader* animation) {
    return animation == (AnimationHeader*)nativePath && fittedEnabled ? &fittedClip : animation;
}
int ResourceMgr_OTRSigCheck(char* data) {
    return data == nativePath;
}
AnimationHeaderCommon* ResourceMgr_LoadAnimByName(const char* path) {
    assert(path == nativePath);
    return &nativeClip.common;
}
f32 Math_SinS(s16 x) {
    return sinf(x * (3.14159265358979323846f / 32768.0f));
}
f32 Math_CosS(s16 x) {
    return cosf(x * (3.14159265358979323846f / 32768.0f));
}
void lusprintf(const char* file, int32_t line, int32_t level, const char* format, ...) {
}
void osSyncPrintfUnused(const char* format, ...) {
}
void LogUtils_LogThreadId(const char* name, s32 line) {
}

int main(void) {
    Vec3s joints[3] = { 0 }, morph[3] = { 0 };
    SkelAnime skel = { 0 };
    skel.limbCount = 3;
    skel.animation = (AnimationHeader*)nativePath;
    skel.jointTable = joints;
    skel.morphTable = morph;
    skel.animLength = 2;
    skel.mode = ANIMMODE_LOOP_INTERP;
    skel.curFrame = 0.5f;
    R_UPDATE_RATE = 3;
    SkelAnime_AnimateFrame(&skel);
    assert(joints[0].x == 10 && joints[1].x == 150);
    fittedEnabled = true;
    SkelAnime_AnimateFrame(&skel);
    assert(joints[0].x == 10 && joints[1].x == 1150);
    assert(skel.animation == (AnimationHeader*)nativePath && skel.curFrame == 0.5f);
    /* Both positive and negative morphs sample the fitted target, while callers
     * still compare the same native path and retain timing/frame cues. */
    Animation_Change(&skel, (AnimationHeader*)nativePath, 1, 0, 1, ANIMMODE_LOOP, 4);
    assert(morph[1].x == 1100 && skel.animation == (AnimationHeader*)nativePath);
    assert(skel.animLength == 2 && skel.playSpeed == 1);
    fittedEnabled = false;
    Animation_Change(&skel, (AnimationHeader*)nativePath, 1, 0, 1, ANIMMODE_LOOP, 0);
    assert(joints[1].x == 100);
    fittedEnabled = true;
    skel.curFrame = 0.5f;
    Animation_Change(&skel, (AnimationHeader*)nativePath, 1, 0, 1, ANIMMODE_LOOP, -4);
    assert(morph[1].x == 100);
    skel.curFrame = 0;
    SkelAnime_AnimateFrame(&skel);
    assert(joints[1].x == 350);
    fittedEnabled = false;
    skel.morphWeight = 0;
    SkelAnime_AnimateFrame(&skel);
    assert(joints[1].x == 100);
    puts("PASS actual animation sampler: fitted clip, fractional frames, positive/negative morphs, native identity, "
         "timing, root motion and live fallback.");
}
