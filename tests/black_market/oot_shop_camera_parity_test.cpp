#include "global.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

// Source extraction supplies the two independent native implementations.
// These fixtures control only services external to the camera algorithms.
static f32 fixtureHeight;
static BgCamFuncData fixtureScene;
f32 Math_FAtan2F(f32 y, f32 x) { return atan2f(y, x); }
f32 Camera_GetFocalActorHeight(Camera*) { return fixtureHeight; }
Vec3s* Camera_GetBgCamOrActorCsCamFuncData(Camera*, u32) { return (Vec3s*)&fixtureScene; }
struct CameraModeValue { s16 val; s16 param; };
struct CameraMode { s16 funcId; s16 numValues; CameraModeValue* values; };
struct CameraSetting { u32 validModes; u32 flags; CameraMode* cameraModes; };
/* MM MODE VALUES */
static CameraMode modes[] = {{CAM_FUNC_DATA4, 3, sSetBlackMarketOotModeNormalData}};
static std::array<CameraSetting, CAM_SET_MAX + 1> sCameraSettings;
static s32 sCameraInterfaceFlags;
#define RELOAD_PARAMS(camera) ((camera->animState == 0) || (camera->animState == 10) || (camera->animState == 20))
#define CAM_RODATA_UNSCALE(x) ((x)*0.01f)
#define GET_NEXT_RO_DATA(values) ((values++)->val)
#define GET_NEXT_SCALED_RO_DATA(values) CAM_RODATA_UNSCALE(GET_NEXT_RO_DATA(values))
/* MM NATIVE SOURCE */
#undef RELOAD_PARAMS

namespace donor {
#define CAM_FUNCDATA_DATA4(yOffset, fov, flags) {yOffset, 0}, {fov, 7}, {flags, 9}
/* OOT MODE VALUES */
static CameraMode modes[] = {{CAM_FUNC_DATA4, 3, sSetPivotShopBrowsingModeNormalData}};
static std::array<CameraSetting, CAM_SET_MAX + 1> sCameraSettings;
struct Data4InitParams { PosRot eyePosRot; char unused[8]; s16 fov; s16 jfifId; };
struct Data4 { f32 yOffset; f32 fov; s16 interfaceFlags; Data4InitParams initParams; };
struct Camera {
    alignas(Data4) char paramData[sizeof(Data4)];
    Player* player;
    Vec3f eye, eyeNext, at, posOffset, floorNorm;
    PosRot playerPosRot;
    f32 data2, data3, dist, xzSpeed, fov, atLERPStepScale;
    f32 rUpdateRateInv, yawUpdateRateInv, pitchUpdateRateInv;
    f32 xzOffsetUpdateRate, yOffsetUpdateRate, fovUpdateRate;
    s16 setting, mode, animState, roll;
};
static s32 sCameraInterfaceFlags;
f32 Donor_PlayerGetHeight(Player*) { return fixtureHeight; }
Vec3s* Camera_GetCamBGData(Camera*) { return (Vec3s*)&fixtureScene; }
void Camera_CopyPREGToModeValues(Camera*) { assert(false); }
f32 Camera_CalcSlopeYAdj(Vec3f*, s16, s16, f32) { assert(false); return 0; }
f32 OLib_ClampMaxDist(f32, f32) { assert(false); return 0; }
int CVarGetInteger(const char*, int fallback) { return fallback; }
f32 Math_AccurateCosS(s16) { assert(false); return 0; }
f32 Math_AccurateSinS(s16) { assert(false); return 0; }
#define CVAR_ENHANCEMENT(x) x
#define OREG(x) sOREGInit[x]
#define R_CAM_YOFFSET_NORM OREG(46)
#define R_RELOAD_CAM_PARAMS 0
#define RELOAD_PARAMS (camera->animState == 0 || camera->animState == 10 || camera->animState == 20 || R_RELOAD_CAM_PARAMS)
#define PCT(x) ((x)*0.01f)
#define NEXTSETTING ((values++)->val)
#define NEXTPCT PCT(NEXTSETTING)
#define BGCAM_POS(v) ((v)[0])
#define BGCAM_ROT(v) ((v)[1])
#define BGCAM_FOV(v) ((v)[2].x)
#define BGCAM_JFIFID(v) ((v)[2].y)
#define DEGF_TO_BINANG(degreesf) (s16)(degreesf * 182.04167f + .5f)
#define RADF_TO_DEGF(radiansf) ((radiansf) * (180.0f / M_PI))
/* OOT NATIVE SOURCE */
#undef RELOAD_PARAMS
}

static void Near(f32 actual, f32 expected, const char* name) {
    if (fabsf(actual - expected) > 0.003f) {
        fprintf(stderr, "%s: MM %.9f differs from OoT %.9f\n", name, actual, expected);
        abort();
    }
}
static void VecNear(const Vec3f& actual, const Vec3f& expected, const char* name) {
    Near(actual.x, expected.x, name); Near(actual.y, expected.y, name); Near(actual.z, expected.z, name);
}
static void Compare(const Camera& mm, const donor::Camera& oot) {
    VecNear(mm.eye, oot.eye, "fixed eye");
    VecNear(mm.at, oot.at, "look direction");
    VecNear(mm.focalActorAtOffset, oot.posOffset, "focal offset");
    Near(mm.fov, oot.fov, "FOV"); Near(mm.dist, oot.dist, "focal distance");
    Near(mm.xzSpeed, oot.xzSpeed, "speed"); Near(mm.atLerpStepScale, oot.atLERPStepScale, "at rate");
    Near(mm.xzOffsetUpdateRate, oot.xzOffsetUpdateRate, "xz rate");
    Near(mm.yOffsetUpdateRate, oot.yOffsetUpdateRate, "y rate");
    assert(mm.roll == oot.roll && mm.animState == oot.animState);
    assert(sCameraInterfaceFlags == donor::sCameraInterfaceFlags);
}
int main() {
    // 0x5B is the new setting appended after all original MM settings.
    constexpr s16 setting = 0x5B;
    sCameraSettings[setting] = {1, 0x80000005, modes};
    donor::sCameraSettings[setting] = {1, 0x80000005, donor::modes};
    constexpr f32 angles[] = {0, 10, 20, 25, 27.5f, 28.75f, 29.375f, 30, 30, 30,
                             20, 10, 5, 2.5f, 1.25f, 0.625f, 0,
                             -10, -20, -25, -27.5f, -28.75f, -29.375f, -30, -30, 0};
    int frames = 0;
    // Base values returned by MM Player_GetHeight, including Goron ball. This
    // fixture supplies height inputs; it does not exercise form dispatch.
    for (f32 height : {34.f, 36.f, 44.f, 68.f, 80.f, 124.f}) {
        fixtureHeight = height;
        for (s16 fov : {s16(-1), s16(60), s16(6000), s16(4500)}) {
            for (s16 pitch : {s16(0), s16(1092), s16(-1092)}) {
                Camera mm{};
                donor::Camera oot{};
                fixtureScene = {{0, 64, 205}, {pitch, -32768, 0}, fov, {3}, -1};
                mm.setting = oot.setting = setting;
                mm.eye = oot.eye = {15, 45, 280};
                mm.eyeNext = oot.eyeNext = mm.eye;
                mm.at = oot.at = {0, 35, 80};
                mm.focalActorPosRot.pos = oot.playerPosRot.pos = {0, 0, 150};
                mm.focalActorAtOffset = oot.posOffset = {0, 35, 0};
                mm.atLerpStepScale = oot.atLERPStepScale = 0.25f;
                for (s16 reload : {s16(0), s16(10), s16(20)}) {
                    mm.animState = oot.animState = reload;
                    for (f32 angle : angles) {
                        Camera_SetOotShopFaceAngle(&mm, angle);
                        oot.data2 = angle;
                        assert(Camera_Data4(&mm));
                        assert(donor::Camera_Data4(&oot));
                        Compare(mm, oot);
                        ++frames;
                    }
                    // New scene data on the next parameter reload must replace
                    // the fixed eye; shelf turns themselves must never move it.
                    fixtureScene.pos.z -= 7;
                }
            }
        }
    }
    // Unused legacy DATA4 dispatch and the setter retain the old no-op behavior.
    Camera legacy{};
    legacy.setting = CAM_SET_FIXED1;
    legacy.eye = {4, 5, 6};
    Camera before = legacy;
    Camera_SetOotShopFaceAngle(&legacy, 29.375f);
    assert(Camera_Data4(&legacy));
    assert(memcmp(&before, &legacy, sizeof(Camera)) == 0);
    printf("OoT Data4 parity: %d native donor/port frames; fixed pivot, fractional yaw, FOV, height inputs, reloads, legacy isolation passed\n", frames);
}
