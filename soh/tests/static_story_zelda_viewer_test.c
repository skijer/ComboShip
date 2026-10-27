/* The runner inserts unmodified production functions at the markers. */
/* PRODUCTION_INCLUDES */
#include <stdlib.h>
#include "vt.h"
#include "tests/test_require.h"

uintptr_t gSegments[16];
static bool replacement;
static const char* expectedSkeleton;
static const char* expectedEye;
static Gfx commands[32], nativeDisplayList[1], customDisplayList[1];
static void* nativeLimbs[17];
static void* customLimbs[17];
static FlexSkeletonHeader loadedSkeleton;
Gfx D_80116280[3];
static unsigned draws, adultDraws, auraDraws;
static Vec3f auraOrigin;
static MtxF modelMatrix;
static MtxF* sCurrentMatrix = &modelMatrix;
#ifdef COMBO_BUILD
void ComboTempleSages_DrawZeldaAura(Actor* actor, PlayState* play) {
    REQUIRE(actor->id == ACTOR_EN_VIEWER);
    REQUIRE(actor->params == 0x7D23 || actor->params == 0x7F34);
    auraOrigin = actor->world.pos;
    ++auraDraws;
}
void ComboTempleSages_DrawZeldaAuraAt(Actor* actor, PlayState* play, float x, float y, float z) {
    REQUIRE(actor->id == ACTOR_EN_VIEWER && actor->params == 0x7D23);
    auraOrigin = (Vec3f){ x, y, z };
    ++auraDraws;
}
#endif
/* PRODUCTION_MATRIX_MULTIPLY */

uint8_t ResourceMgr_FileExists(const char* path) {
    return 0;
}
uint8_t ResourceMgr_FileAltExists(const char* path) {
    return 0;
}
int ResourceMgr_OTRSigCheck(char* path) {
    return memcmp(path, "__OTR__", 7) == 0;
}
SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path, SkelAnime* skelAnime) {
    REQUIRE(strcmp(path, expectedSkeleton) == 0);
    loadedSkeleton.sh.segment = replacement ? customLimbs : nativeLimbs;
    loadedSkeleton.sh.limbCount = strstr(path, "object_zl4/") ? 17 : 14;
    loadedSkeleton.dListCount = replacement ? 9 : loadedSkeleton.sh.limbCount - 1;
    return &loadedSkeleton.sh;
}
void* ZeldaArena_MallocDebug(size_t size, const char* file, s32 line) {
    return calloc(1, size);
}
void lusprintf(const char* file, int32_t line, int32_t level, const char* format, ...) {
}
s32 SkelAnime_Init(PlayState* play, SkelAnime* skelAnime, SkeletonHeader* header, AnimationHeader* animation,
                   Vec3s* joints, Vec3s* morphs, s32 count) {
    REQUIRE(false);
    return 0;
}
void Animation_PlayLoop(SkelAnime* skelAnime, AnimationHeader* animation) {
    REQUIRE(false);
}
void Animation_PlayLoopSetSpeed(SkelAnime* skelAnime, AnimationHeader* animation, f32 speed) {
    skelAnime->animation = animation;
    skelAnime->playSpeed = speed;
}
void FrameInterpolation_RecordOpenChild(const void* source, int line) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Graph_OpenDisps(Gfx** commands, GraphicsContext* gfx, const char* file, s32 line) {
}
void Graph_CloseDisps(Gfx** commands, GraphicsContext* gfx, const char* file, s32 line) {
}
void Gfx_SetupDL_25Opa(GraphicsContext* gfx) {
}
void gSPSegment(void* command, int segment, uintptr_t target) {
    __gSPSegment((Gfx*)command, segment, target);
}
static uintptr_t segmentAtDraw(const PlayState* play, unsigned segment) {
    uintptr_t address = 0;
    for (const Gfx* command = commands; command < play->state.gfxCtx->polyOpa.p; ++command)
        if (command->words.w0 == (0xDB060000u | (segment * 4)))
            address = command->words.w1;
    return address;
}

/* PRODUCTION_NATIVE_INIT */
/* PRODUCTION_ANIMATION_AND_SKELETON */

static s32 EnViewer_StaticChildZeldaOverrideLimbDraw(PlayState*, s32, Gfx**, Vec3f*, Vec3s*, void*);
void SkelAnime_DrawSkeletonOpa(PlayState* play, SkelAnime* skelAnime, OverrideLimbDrawOpa override,
                               PostLimbDrawOpa post, void* pointer) {
    EnViewer* actor = pointer;
    REQUIRE(skelAnime == &actor->skin.skelAnime);
    REQUIRE(skelAnime->skeleton == (replacement ? customLimbs : nativeLimbs));
    REQUIRE(override == EnViewer_StaticChildZeldaOverrideLimbDraw && post == NULL);
    REQUIRE(strcmp((char*)segmentAtDraw(play, 0x08), expectedEye) == 0);
    REQUIRE(strcmp((char*)segmentAtDraw(play, 0x09), expectedEye) == 0);
    REQUIRE(strcmp((char*)segmentAtDraw(play, 0x0A), gChildZeldaMouthNeutralTex) == 0);
    for (s32 limb = 1; limb <= 17; ++limb) {
        Gfx* mesh = replacement ? customDisplayList : nativeDisplayList;
        Gfx* before = mesh;
        Vec3f pos = { 0 };
        Vec3s rot = { 0 };
        REQUIRE(!override(play, limb, &mesh, &pos, &rot, actor));
        REQUIRE(mesh == (limb >= 3 && limb < 7 ? NULL : before));
    }
    ++draws;
}

/* PRODUCTION_CHILD_DRAW */

static s32 EnViewer_StaticAdultZeldaOverrideLimbDraw(PlayState* play, s32 limb, Gfx** list, Vec3f* pos, Vec3s* rot,
                                                     void* actor, Gfx** gfx) {
    return 0;
}
Gfx* SkelAnime_DrawFlex(PlayState* play, void** skeleton, Vec3s* joints, s32 count, OverrideLimbDraw override,
                        PostLimbDraw post, void* pointer, Gfx* gfx) {
    EnViewer* actor = pointer;
    REQUIRE(skeleton == (replacement ? customLimbs : nativeLimbs));
    REQUIRE(count == loadedSkeleton.dListCount && joints == actor->skin.skelAnime.jointTable);
    REQUIRE(override == EnViewer_StaticAdultZeldaOverrideLimbDraw && post == NULL);
    REQUIRE(strcmp((char*)segmentAtDraw(play, 0x08), gZelda2EyeOpenTex) == 0);
    REQUIRE(strcmp((char*)segmentAtDraw(play, 0x09), gZelda2EyeOpenTex) == 0);
    REQUIRE(strcmp((char*)segmentAtDraw(play, 0x0A), gZelda2MouthSeriousTex) == 0);
    ++adultDraws;
    return gfx;
}
/* PRODUCTION_ADULT_DRAW */

int main(void) {
    static PlayState play;
    GraphicsContext gfx = { 0 };
    play.state.gfxCtx = &gfx;
    const char* eyes[] = { gChildZeldaEyeOpenTex, gChildZeldaEyeBlinkTex, gChildZeldaEyeShutTex };
    const char* childAnimations[] = { gChildZeldaAnim_000654, gChildZeldaAnim_01910C, gChildZeldaAnim_01910C };
    for (int model = 0; model < 2; ++model) {
        replacement = model;
        for (int pose = 0; pose < 3; ++pose) {
            EnViewer actor = { 0 };
            actor.staticState.type = STATIC_STORY_ACTOR_CHILD_ZELDA;
            actor.staticState.pose = pose;
            actor.staticState.staticMode = true;
            actor.actor.id = ACTOR_EN_VIEWER;
            actor.actor.params = 0x7D03 | (pose << 4);
            expectedSkeleton = gChildZeldaSkel;
            EnViewerStatic_InitSkeleton(&actor, &play,
                                        StaticStoryActor_ResolvePose(STATIC_STORY_ACTOR_CHILD_ZELDA, pose));
            REQUIRE(actor.skin.skelAnime.skeletonHeader == &loadedSkeleton.sh);
            REQUIRE(actor.skin.skelAnime.limbCount == 18);
            REQUIRE(actor.skin.skelAnime.dListCount == (replacement ? 9 : 16));
            REQUIRE(strcmp((char*)actor.skin.skelAnime.animation, childAnimations[pose]) == 0);
            actor.actor.world.pos = (Vec3f){ 100, 200, 300 };
            actor.actor.shape.rot.y = model ? 0x4000 : 0;
            actor.actor.scale = (Vec3f){ model ? .02f : .01f, .01f, .01f };
            actor.skin.skelAnime.jointTable[0] = (Vec3s){ 1570, 2460, model ? -200 : 0 };
            modelMatrix = (MtxF){ 0 };
            modelMatrix.xw = 100;
            modelMatrix.yw = 200;
            modelMatrix.zw = 300;
            modelMatrix.ww = 1;
            modelMatrix.yy = .01f;
            if (model) {
                modelMatrix.xz = .01f;
                modelMatrix.zx = -.02f;
            } else {
                modelMatrix.xx = .01f;
                modelMatrix.zz = .01f;
            }
            for (int eye = 0; eye < 3; ++eye) {
                actor.staticState.eyeIndex = eye;
                expectedEye = eyes[eye];
                gfx.polyOpa.p = commands;
                gSPSegment(gfx.polyOpa.p++, 0x08, (uintptr_t) "previous actor eye");
                gSPSegment(gfx.polyOpa.p++, 0x09, (uintptr_t) "previous actor eye");
                gSPSegment(gfx.polyOpa.p++, 0x0A, (uintptr_t) "previous actor mouth");
                EnViewer before = actor;
                unsigned previousAura = auraDraws;
                EnViewer_DrawStaticChildZelda(&actor, &play);
                REQUIRE(memcmp(&before, &actor, sizeof(actor)) == 0);
#ifdef COMBO_BUILD
                REQUIRE(auraDraws == previousAura + (pose == 2));
                if (pose == 2) {
                    REQUIRE(fabsf(auraOrigin.x - (model ? 98.f : 115.7f)) < .0001f);
                    REQUIRE(auraOrigin.y == 200.f);
                    REQUIRE(fabsf(auraOrigin.z - (model ? 268.6f : 300.f)) < .0001f);
                }
#else
                REQUIRE(auraDraws == previousAura);
#endif
            }
            free(actor.skin.skelAnime.jointTable);
            free(actor.skin.skelAnime.morphTable);
        }
        for (int pose = 0; pose < 4; ++pose) {
            EnViewer actor = { 0 };
            actor.staticState.type = STATIC_STORY_ACTOR_ADULT_ZELDA;
            actor.staticState.pose = pose;
            actor.staticState.staticMode = true;
            actor.actor.id = ACTOR_EN_VIEWER;
            actor.actor.params = 0x7F04 | (pose << 4);
            expectedSkeleton = gZelda2Skel;
            EnViewerStatic_InitSkeleton(&actor, &play,
                                        StaticStoryActor_ResolvePose(STATIC_STORY_ACTOR_ADULT_ZELDA, pose));
            REQUIRE(actor.skin.skelAnime.skeletonHeader == &loadedSkeleton.sh);
            REQUIRE(actor.skin.skelAnime.skeleton == (replacement ? customLimbs : nativeLimbs));
            REQUIRE(strcmp((char*)actor.skin.skelAnime.animation,
                           pose >= 2 ? gZelda2Anime2Anim_003FF8 : gZelda2Anime2Anim_009FBC) == 0);
            gfx.polyOpa.p = commands;
            const unsigned previousAura = auraDraws;
            EnViewer before = actor;
            EnViewer_DrawStaticAdultZelda(&actor, &play);
            REQUIRE(memcmp(&before, &actor, sizeof(actor)) == 0);
#ifdef COMBO_BUILD
            REQUIRE(auraDraws == previousAura + (pose == 3));
#else
            REQUIRE(auraDraws == previousAura);
#endif
            free(actor.skin.skelAnime.jointTable);
            free(actor.skin.skelAnime.morphTable);
        }
    }
    REQUIRE(draws == 18);
    REQUIRE(adultDraws == 8);
#ifdef COMBO_BUILD
    REQUIRE(auraDraws == 8);
#else
    REQUIRE(auraDraws == 0);
#endif
    puts("PASS Zelda production viewer: native/replacement skeleton resolution, distinct animations, blink/face "
         "segments and loaded meshes");
    return 0;
}
