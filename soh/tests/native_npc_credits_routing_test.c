#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/overlays/actors/ovl_Demo_Ec/z_demo_ec.h"
#include "soh/Enhancements/cosmetics/NativeNpcModels.h"
#include "objects/object_kw1/object_kw1.h"
#include "objects/object_km1/object_km1.h"
#include "objects/object_ec/object_ec.h"

void DemoEc_InitKokiriGirl(DemoEc*, PlayState*);
void DemoEc_InitDancingKokiriGirl(DemoEc*, PlayState*);
void DemoEc_InitKokiriBoy(DemoEc*, PlayState*);
void DemoEc_DrawKokiriGirl(DemoEc*, PlayState*);
void DemoEc_DrawKokiriBoy(DemoEc*, PlayState*);
static bool pack, alt;
static const char* expectedSkeleton;
static const char* expectedAnimation;
static OverrideLimbDraw override;
static DemoEc* drawing;
static Vec3s joints[16];
static Gfx arena[4096];
static unsigned allocated, skinSelections;
uintptr_t gSegments[16];
Gfx D_80116280[3];
void FrameInterpolation_RecordOpenChild(const void* a, int b) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Graph_OpenDisps(Gfx** refs, GraphicsContext* gfx, const char* file, s32 line) {
}
void Graph_CloseDisps(Gfx** refs, GraphicsContext* gfx, const char* file, s32 line) {
}
void gSPSegment(void* pkt, s32 segment, uintptr_t base) {
    __gSPSegment((Gfx*)pkt, segment, base);
}

bool ResourceMgr_IsAltAssetsEnabled(void) {
    return alt;
}
uint8_t ResourceMgr_FileExists(const char* p) {
    return pack && strcmp(p, sKokiriGirlNpcSkel) == 0;
}
uint8_t ResourceMgr_FileAltExists(const char* p) {
    return pack &&
           (strcmp(p, sKokiriGirlNpcSkel) == 0 || strcmp(p, sKokiriGirlNpcHead) == 0 || strstr(p, "/Faces/") != NULL);
}
int ResourceMgr_OTRSigCheck(char* p) {
    return p && memcmp(p, "__OTR__", 7) == 0;
}
s32 SkelAnime_InitFlex(PlayState* play, SkelAnime* skel, FlexSkeletonHeader* skeleton, AnimationHeader* animation,
                       Vec3s* jointTable, Vec3s* morphTable, s32 count) {
    assert(strcmp((const char*)skeleton, expectedSkeleton) == 0);
    skel->jointTable = joints;
    skel->limbCount = 16;
    skel->dListCount = 15;
    return 0;
}
s16 Animation_GetLastFrame(void* animation) {
    assert(strcmp((const char*)animation, expectedAnimation) == 0);
    return 29;
}
void Animation_Change(SkelAnime* skel, AnimationHeader* animation, f32 speed, f32 start, f32 end, u8 mode, f32 morph) {
    assert(strcmp((const char*)animation, expectedAnimation) == 0);
    assert(speed == 1.f && start == 0.f && end == 29.f && mode == 0 && morph == 0.f);
    skel->animation = animation;
}
void AnimationContext_SetMoveActor(PlayState* play, Actor* actor, SkelAnime* skel, f32 scale) {
}
void ActorShape_Init(ActorShape* shape, f32 y, ActorShadowFunc shadow, f32 scale) {
}
void ActorShadow_DrawCircle(Actor* actor, Lights* lights, PlayState* play) {
}
void Gfx_SetupDL_25Opa(GraphicsContext* gfx) {
}
void* Graph_Alloc(GraphicsContext* gfx, size_t size) {
    unsigned n = (size + sizeof(Gfx) - 1) / sizeof(Gfx);
    assert(allocated + n < 4096);
    Gfx* result = arena + allocated;
    allocated += n;
    return result;
}
Gfx* SkelAnime_DrawFlex(PlayState* play, void** skeleton, Vec3s* table, s32 count, OverrideLimbDraw callback,
                        PostLimbDraw post, void* actor, Gfx* gfx) {
    assert(actor == &drawing->actor && count == 15);
    override = callback;
    return gfx;
}
Gfx* NativeNpcPose_SelectLimb(const SkelAnime* skel, s32 index, Gfx* original) {
    assert(skel == &drawing->skelAnime);
    ++skinSelections;
    return original;
}

int main(void) {
    PlayState* play = calloc(1, sizeof(*play));
    assert(play);
    GraphicsContext graphics = { 0 };
    play->state.gfxCtx = &graphics;
    DemoEc actor = { 0 };
    drawing = &actor;
    for (int hasPack = 0; hasPack < 2; ++hasPack)
        for (int useAlt = 0; useAlt < 2; ++useAlt) {
            pack = hasPack;
            alt = useAlt;
            for (int dance = 0; dance < 2; ++dance) {
                graphics.polyOpa.p = arena;
                allocated = 128;
                expectedSkeleton = pack ? sKokiriGirlNpcSkel : gKw1Skel;
                expectedAnimation = dance ? gDemoEcDancingKokiriAnim : gDemoEcKokiriAnim;
                if (dance)
                    DemoEc_InitDancingKokiriGirl(&actor, play);
                else
                    DemoEc_InitKokiriGirl(&actor, play);
                for (int eye = 0; eye < 3; ++eye) {
                    actor.eyeTexIndex = eye;
                    override = NULL;
                    DemoEc_DrawKokiriGirl(&actor, play);
                    assert(override != NULL);
                    Gfx native[1] = { 0 };
                    Gfx* head = native;
                    Gfx* output = arena;
                    Vec3f p = { 0 };
                    Vec3s rotation = { 0 };
                    unsigned selected = skinSelections;
                    assert(!override(play, 15, &head, &p, &rotation, &actor, &output));
                    assert(skinSelections == selected + 1);
                    if (pack && alt)
                        assert(strcmp((const char*)head, sKokiriGirlNpcFaces[eye][eye][0]) == 0);
                    else
                        assert(head == native);
                    Gfx* arm = native;
                    override(play, 9, &arm, &p, &rotation, &actor, &output);
                    assert(arm == native && skinSelections == selected + 2);
                }
            }
            expectedSkeleton = gKm1Skel;
            expectedAnimation = gDemoEcKokiriAnim;
            DemoEc_InitKokiriBoy(&actor, play);
            override = NULL;
            DemoEc_DrawKokiriBoy(&actor, play);
            assert(override == NULL);
        }
    free(play);
    puts("PASS real credits standing/dancing girl init, all blink states, skin draw callback, Alt/absent-pack fallback "
         "and boy isolation.");
}
