#include "objects/gameplay_keep/gameplay_keep.h"
#include "tests/test_require.h"
#include "z64.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

static MtxF current, wingPose, glowPose;
static MtxF* sCurrentMatrix = &current;
static Vec3f glowCenter;
static Gfx display[128], material[4];
static Gfx* fixtureXlu;
static SkelAnime sPinkFairySkel;
static u8 sPinkFairySkelInited;
static int updates, draws, glowOverrides;

void FrameInterpolation_RecordMatrixTranslate(...) {
}
void FrameInterpolation_RecordMatrixScale(...) {
}
void FrameInterpolation_RecordMatrixRotate1Coord(...) {
}
void FrameInterpolation_RecordMatrixMultVec3f(...) {
}
#include "grace_matrix.inc"

extern "C" {
f32 Math_SinS(s16 angle) {
    return std::sin(angle * (3.141592653589793 / 32768.0));
}
f32 Math_CosS(s16 angle) {
    return std::cos(angle * (3.141592653589793 / 32768.0));
}
s32 SkelAnime_Update(SkelAnime*) {
    ++updates;
    return 0;
}
#ifdef GRACE_DRAW_MM
void Gfx_SetupDL27_Xlu(GraphicsContext*) {
}
#else
void Gfx_SetupDL_27Xlu(GraphicsContext*) {
}
void* Graph_Alloc(GraphicsContext*, size_t size) {
    REQUIRE(size == sizeof(material));
    return material;
}
#endif
}

#undef OPEN_DISPS
#undef CLOSE_DISPS
#undef POLY_XLU_DISP
#define OPEN_DISPS(...) fixtureXlu = display
#define CLOSE_DISPS(...)
#define POLY_XLU_DISP fixtureXlu
#define gSPSegment(cmd, segment, address) ((void)(cmd))
#ifndef GRACE_DRAW_MM
#define Graph_Alloc(gfx, size) static_cast<Gfx*>(Graph_Alloc(gfx, size))
#endif
#ifdef GRACE_DRAW_MM
#undef GRAPH_ALLOC
#define GRAPH_ALLOC(gfx, size) material
#endif

static Gfx* CaptureSkeleton(PlayState* play, OverrideLimbDraw override, void* actor, Gfx* gfx) {
    ++draws;
    wingPose = current;
    // A glow limb inherits its animated parent transform before the native-style
    // override separates its quad orientation. Keep that world position intact.
    Matrix_Translate(150.0f, 25.0f, -90.0f, MTXMODE_APPLY);
    Vec3f zero{}, pos{};
    Vec3s rot{};
    Gfx* list = display;
    Matrix_MultVec3f(&zero, &glowCenter);
    if (override != nullptr) {
#ifdef GRACE_DRAW_MM
        REQUIRE(override(play, FAIRY_LIMB_6, &list, &pos, &rot, static_cast<Actor*>(actor), &gfx) == 0);
#else
        REQUIRE(override(play, 8, &list, &pos, &rot, actor, &gfx) == 0);
#endif
        REQUIRE(list == display);
        ++glowOverrides;
    }
    glowPose = current;
    return gfx;
}
extern "C" {
#ifdef GRACE_DRAW_MM
Gfx* SkelAnime_Draw(PlayState* play, void**, Vec3s*, OverrideLimbDraw override, PostLimbDraw, Actor* actor, Gfx* gfx) {
    return CaptureSkeleton(play, override, actor, gfx);
}
#else
Gfx* SkelAnime_DrawSkeleton2(PlayState* play, SkelAnime*, OverrideLimbDraw override, PostLimbDraw, void* actor,
                             Gfx* gfx) {
    return CaptureSkeleton(play, override, actor, gfx);
}
#endif
}
#include "grace_draw.inc"

static void Near(float actual, float expected) {
    if (std::fabs(actual - expected) >= 0.0001f)
        std::fprintf(stderr, "Matrix component actual %.6f, expected %.6f\n", actual, expected);
    REQUIRE(std::fabs(actual - expected) < 0.0001f);
}

int main() {
    Player player{};
    PlayState play{};
    GraphicsContext graphics{};
    play.state.gfxCtx = &graphics;
    player.actor.world.pos = { 50.0f, 100.0f, -75.0f };
    HGrace_DrawFairy(&player.actor, &play);
    REQUIRE(updates == 0 && draws == 0);
    sPinkFairySkelInited = 1;
    for (int yaw : { 0x4000, 0, 0x2000, 0x8000, 0xC000, 0xF123 }) {
        for (unsigned frame : { 0u, 2u, 5u, 13u }) {
            player.actor.shape.rot.y = static_cast<s16>(yaw);
            play.gameplayFrames = frame;
            const float scale = 0.008f * (1.0f + 0.1f * Math_SinS(frame * 4096));
            HGrace_DrawFairy(&player.actor, &play);
            Near(wingPose.xw, 50.0f);
            Near(wingPose.yw, 120.0f);
            Near(wingPose.zw, -75.0f);
            // Local +Z must follow the controller's facing across cardinal and
            // arbitrary headings, without moving or rescaling the fairy.
            Near(wingPose.xz, Math_SinS(player.actor.shape.rot.y) * scale);
            Near(wingPose.zz, Math_CosS(player.actor.shape.rot.y) * scale);
            Near(wingPose.yy, scale);
            REQUIRE(glowOverrides == draws);
            Near(glowPose.xw, glowCenter.x);
            Near(glowPose.yw, glowCenter.y);
            Near(glowPose.zw, glowCenter.z);
            Near(glowPose.xx, scale);
            Near(glowPose.yy, scale);
            Near(glowPose.zz, scale);
            Near(glowPose.xz, 0);
            Near(glowPose.zx, 0);
        }
    }
    REQUIRE(draws == 24 && updates == 24);
    puts("PASS Grace facing follows player yaw; glow remains aligned at its "
         "animated center; placement, pulse and wing updates preserved");
}
