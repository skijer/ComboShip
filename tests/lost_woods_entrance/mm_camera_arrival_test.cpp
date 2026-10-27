// The arrival must be visible before the player supplies any movement input.
// Native camera placement, passage vertices, matrix transforms and triangle
// intersections are executed unchanged; floor/water/cutscene services are
// boundaries.
// Keep native declarations first: z64math's prototypes inherit their C linkage.
#include "global.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "entrances/LostWoodsEntrance.h"
#include "entrances/RegionalPassages.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>

SaveContext gSaveContext;
static bool sComboPortalArrivalPending = true;
static int sComboPortalRoute = COMBO_PORTAL_LOST_WOODS;
static MtxF sMatrix;
MtxF* sCurrentMatrix = &sMatrix;
static s16 sCameraInterfaceFlags;
extern "C" void ComboForestArrivalCamera_Queue(int) {}
extern "C" void MagicMirror_QueueArrival(void) {}
extern "C" int ComboRegionalPassagesMm_ApplyArrival(int) {
    assert(false && "forest arrival must retain its native route");
    return 0;
}

void FrameInterpolation_RecordMatrixTranslate(f32, f32, f32, MatrixMode) {
}
f32 Camera_GetFloorYNorm(Camera*, Vec3f*, Vec3f*, s32*) {
    return BGCHECK_Y_MIN;
}
void func_800DDFE0(Camera*) {
}
void Camera_ResetActionFuncState(Camera*, s32) {
}
void CutsceneManager_StoreCamera(Camera*) {
}
s32 Camera_UpdateWater(Camera*) {
    return 0;
}

/* NATIVE_BODIES */
/* NORMAL_PRESET */
/* ARRIVAL_BODY */
/* PASSAGE_GEOMETRY */
/* PASSAGE_TRANSFORM */
/* PLAYER_RESPAWN */

static std::array<Vec3f, kQuadCount * 4> WorldPassage() {
    BuildPassage();
    ApplyPassageTransform();
    std::array<Vec3f, kQuadCount * 4> world;
    for (size_t i = 0; i < world.size(); ++i) {
        Vec3f local = { (f32)sVertices[i].v.ob[0], (f32)sVertices[i].v.ob[1], (f32)sVertices[i].v.ob[2] };
        Matrix_MultVec3f(&local, &world[i]);
    }
    return world;
}

static int PassageIntersection(Camera& camera, std::array<Vec3f, kQuadCount * 4>& world) {
    for (int quad = 0; quad < kQuadCount; ++quad) {
        for (int half = 0; half < 2; ++half) {
            int a = quad * 4, b = a + (half ? 2 : 1), c = a + (half ? 3 : 2);
            TriNorm triangle;
            Math3D_TriNorm(&triangle, &world[a], &world[b], &world[c]);
            Vec3f hit;
            // The actual passage disables both cull directions. A rear-facing
            // mouth therefore occludes just as its front-facing surface does.
            if (Math3D_TriLineIntersect(&world[a], &world[b], &world[c], triangle.plane.normal.x,
                                        triangle.plane.normal.y, triangle.plane.normal.z, triangle.plane.originDist,
                                        &camera.eye, &camera.at, &hit, false)) {
                return quad;
            }
        }
    }
    return -1;
}

static void NativeCamera(PlayState& play, Player& player) {
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    play.mainCamera.play = &play;
    play.mainCamera.rUpdateRateInv = 10.0f; // Camera_Init's native initial rate.
    Camera_InitFocalActorSettings(&play.mainCamera, &player.actor);
}

int main() {
    gSaveContext.respawn[RESPAWN_MODE_TOP].pos = { 111, 222, 333 };
    gSaveContext.save.day = 2;
    gSaveContext.save.time = 0x8000;
    MM_ApplyComboPortalArrival();
    const RespawnData& arrival = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    assert(gSaveContext.respawnFlag == 2 && arrival.roomIndex == 1 && arrival.yaw == -8192);
    assert(arrival.playerParams == PLAYER_PARAMS(0xFF, PLAYER_START_MODE_E));
    assert(!ComboLostWoods_InMmPassage(SCENE_26SARUNOMORI, arrival.roomIndex, arrival.pos.x, arrival.pos.y,
                                       arrival.pos.z));
    assert(gSaveContext.respawn[RESPAWN_MODE_TOP].pos.y == 222 && gSaveContext.save.day == 2 &&
           gSaveContext.save.time == 0x8000);
    auto world = WorldPassage();

    // The old placement is a sensitivity control: the actual camera looks
    // through the exact black mouth, so a decorative-geometry-only test cannot
    // silently pass while the original visibility regression remains present.
    {
        PlayState play{};
        Player player{};
        player.transformation = PLAYER_FORM_HUMAN;
        player.actor.world.pos = { 90, 0, -90 };
        player.actor.shape.rot.y = -8192;
        NativeCamera(play, player);
        assert(PassageIntersection(play.mainCamera, world) == kQuadCount - 1);
    }

    for (int form = PLAYER_FORM_FIERCE_DEITY; form <= PLAYER_FORM_HUMAN; ++form) {
        PlayState play{};
        Player player{};
        player.transformation = form;
        ApplyNativeReturnRespawn(&player);
        NativeCamera(play, player);
        int quad = PassageIntersection(play.mainCamera, world);
        if (quad >= 0) {
            std::fprintf(stderr,
                         "FAIL forest arrival: native camera eye=(%.3f,%.3f,%.3f) "
                         "views Link through passage quad %d%s\n",
                         play.mainCamera.eye.x, play.mainCamera.eye.y, play.mainCamera.eye.z, quad,
                         quad == kQuadCount - 1 ? " (black mouth)" : "");
            return 1;
        }
        // Keep the spawn stationary, including when Player_Init's remembered
        // save-location handling replaces E with D. Probe the native NORMAL0
        // form-scaled range and flat-ground pitch while its real clamp settles.
        // This is a preset envelope, not a simulation of camera collision/AI.
        Camera& camera = play.mainCamera;
        const Normal1ReadOnlyData preset = NativeNormalPreset(&camera);
        Vec3f nativeAt = camera.at;
        for (int frame = 0; frame < 240; ++frame) {
            camera.dist = Camera_ClampDist2(&camera, camera.dist, preset.unk_04, preset.unk_08, frame < 120);
            VecGeo offset;
            offset.r = camera.dist;
            offset.pitch = preset.unk_20;
            offset.yaw = BINANG_ROT180(player.actor.shape.rot.y);
            // Both vertical endpoints of the native at-offset settle must clear.
            for (f32 yOffset : { 0.0f, preset.unk_00 }) {
                camera.at = nativeAt;
                camera.at.y += yOffset;
                camera.eye = OLib_AddVecGeoToVec3f(&camera.at, &offset);
                if (PassageIntersection(camera, world) >= 0) {
                    std::fprintf(stderr,
                                 "FAIL forest arrival: native NORMAL0 preset obscures "
                                 "stationary form %d "
                                 "at settling frame %d, radius %.3f\n",
                                 form, frame, camera.dist);
                    return 1;
                }
            }
        }
    }
    std::puts("PASS forest arrival: five native form heights/camera inits and "
              "stationary NORMAL0 settling envelope "
              "clear passage geometry; old black-mouth placement reproduced; "
              "room/yaw/trigger/Farore/cycle preserved");
}
