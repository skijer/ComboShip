#!/usr/bin/env python3
"""Run OoT shop interaction through native scene data, setting, and Data4 frames.

Only floor collision, debug camera, and enhancement-only boundaries are stubbed.
Data4, its at calculation, background-camera lookup, native trig and all setting
changes are extracted verbatim from the checked-out production sources.
"""
from pathlib import Path
import os, re, subprocess, sys, tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def function(source, signature):
    match = re.search(re.escape(signature) + r"[^;{]*\{", source)
    if match is None:
        raise ValueError("Missing production function: " + signature)
    return block_from(source, match.start())


def native_camera_path(camera, play):
    data = (ROOT / "soh/src/code/z_camera_data.inc").read_text()
    array = lambda signature: block_from(data, data.index(signature)) + ";\n"
    prefix = data[:data.index("} CameraSetting;") + len("} CameraSetting;")]
    prefix += "\n#define CAM_SETTING_MODE_ENTRY(func, data) { func, ARRAY_COUNT(data), data }\n"
    for values, modes in (("sSetFree0ModeNormalData", "sCamSetFree0Modes"),
                          ("sSetPivotShopBrowsingModeNormalData", "sCamSetPivotShopBrowsingModes")):
        prefix += array("CameraModeValue " + values + "[") + array("CameraMode " + modes + "[")
    prefix += "CameraSetting sCameraSettings[CAM_SET_MAX]{};\nvoid InitNativeCameraTable() {\n"
    for setting, modes in (("CAM_SET_FREE0", "sCamSetFree0Modes"),
                           ("CAM_SET_PIVOT_SHOP_BROWSING", "sCamSetPivotShopBrowsingModes")):
        entry = re.search(r"^\s*(\{[^\n]*" + modes + r"[^\n]*\}),", data, re.M).group(1)
        prefix += "sCameraSettings[" + setting + "] = " + entry + ";\n"
    prefix += "}\n"
    prefix += "\n".join((
        "#define osSyncPrintf(...) ((void)0)",
        "static GameInfo cameraRegisters{}; GameInfo* gGameInfo = &cameraRegisters;",
        "SaveContext gSaveContext{}; uintptr_t gSegments[NUM_SEGMENTS]{};",
        "static s32 sInitRegs = 1, sDbgModeIdx = -1, sCameraInterfaceFlags = 1;",
        "static s32 sCameraInterfaceAlpha, sCameraShrinkWindowVal, D_8011D3F0;",
        "static s16 sNextUID; PlayState* D_8015BD7C; DbCamera D_8015BD80;",
        "void DbCamera_Reset(Camera*, DbCamera*) {}",
        "void DbCamera_Init(DbCamera*, Camera*) {}",
        "#define PCT(x) ((x) * .01f)",
        "#define NEXTSETTING ((values++)->val)",
        "#define NEXTPCT PCT(NEXTSETTING)",
        "#define RELOAD_PARAMS (camera->animState == 0 || camera->animState == 10 || camera->animState == 20 || R_RELOAD_CAM_PARAMS)",
        "#define BGCAM_POS(v) ((v)[0])",
        "#define BGCAM_ROT(v) ((v)[1])",
        "#define BGCAM_FOV(v) ((v)[2].x)",
        "#define BGCAM_JFIFID(v) ((v)[2].y)",
        "char sCameraSettingNames[CAM_SET_MAX][12]{};",
        "s32 Camera_CopyPREGToModeValues(Camera*) { assert(false && \"debug camera parameter reload is disabled\"); return 0; }",
        "s32 Camera_UpdateWater(Camera*) { assert(false && \"fixture only initializes subcamera player settings\"); return 0; }",
        "s32 Camera_ChangeDoorCam(Camera*, Actor*, s16, f32, s16, s16, s16) { assert(false && \"fixture only initializes subcamera player settings\"); return 0; }",
        "f32 Camera_GetFloorYNorm(Camera*, Vec3f* normal, Vec3f*, s32* bgId) { *normal = { 0, 1, 0 }; *bgId = BGCHECK_SCENE; return 0; }",
        "f32 TransformMasks_GetFormHeight() { return 0; }",
        "s32 MinishTiny_IsActive() { return 0; }",
        "s32 SetCameraManual(Camera*) { assert(false && \"free look is disabled\"); return 0; }",
        "s32 gUseAtanContFrac; f32 qNaN0x10000 = NAN;",
    )) + "\n"
    prefix += array("s16 sOREGInit[") + "s16 sOREGInitCnt = ARRAY_COUNT(sOREGInit);\n"
    prefix += array("s16 sCamDataRegsInit[") + "s16 sCamDataRegsInitCount = ARRAY_COUNT(sCamDataRegsInit);\n"
    lib = (ROOT / "soh/src/code/z_lib.c").read_text()
    actor = (ROOT / "soh/src/code/z_actor.c").read_text()
    olib = (ROOT / "soh/src/code/z_olib.c").read_text()
    atan = (ROOT / "soh/src/code/code_800FCE80.c").read_text()
    bgcheck = (ROOT / "soh/src/code/z_bgcheck.c").read_text()
    trig = (ROOT / "soh/src/libultra/gu/sintable.c").read_text()
    prefix += block_from(trig, trig.index("static s16 sintable[")) + ";\n"
    for filename, signature in (("sins.c", "s16 sins("), ("coss.c", "s16 coss(")):
        prefix += function((ROOT / "soh/src/libultra/gu" / filename).read_text(), signature) + "\n"
    for source, signatures in (
        (lib, ("f32 Math_CosS(", "f32 Math_SinS(", "f32 Math_AccurateCosS(", "f32 Math_AccurateSinS(", "void Math_Vec3f_Copy(")),
        (actor, ("f32 Player_GetHeight(", "PosRot* Actor_GetWorldPosShapeRot(")),
        (atan, ("f32 Math_FAtanTaylorQF(", "f32 Math_FAtanTaylorF(", "f32 Math_FAtanContFracF(", "f32 Math_FAtanF(", "f32 Math_FAtan2F(")),
        (olib, ("f32 OLib_Vec3fDist(", "f32 OLib_ClampMaxDist(", "Vec3f* OLib_VecSphToVec3f(", "Vec3f* OLib_VecSphGeoToVec3f(", "VecSph* OLib_Vec3fToVecSph(", "VecSph* OLib_Vec3fToVecSphGeo(", "VecSph* OLib_Vec3fDiffToVecSphGeo(")),
        (bgcheck, ("CollisionHeader* BgCheck_GetCollisionHeader(", "u16 BgCheck_GetBgCamSettingImpl(", "Vec3s* BgCheck_GetBgCamFuncDataImpl(")),
        (camera, ("f32 Camera_LERPCeilF(", "void Camera_LERPCeilVec3f(", "void func_80043ABC(", "void func_80043B60(", "Vec3f* Camera_Vec3sToVec3f(", "Vec3f* Camera_Vec3fVecSphGeoAdd(", "void Camera_CopyDataToRegs(", "f32 Camera_CalcSlopeYAdj(", "s32 Camera_CalcAtDefault(", "s32 func_8005A02C(", "s32 Camera_RequestModeImpl(", "s16 Camera_ChangeSettingFlags(", "s32 Camera_RequestSetting(", "s16 Camera_GetBgCamSetting(", "Vec3s* Camera_GetCamBGData(", "s32 Camera_ChangeDataIdx(", "s32 Camera_QRegInit(", "void func_80057FC4(", "void Camera_InitPlayerSettings(", "s16 Camera_ChangeStatus(", "void Camera_SetCameraData(", "s32 Camera_Data4(")),
        (play, ("void Play_SetShopBrowsingViewpoint(", "void Play_RequestViewpointBgCam(")),
    ):
        prefix += "\n".join(function(source, signature) for signature in signatures) + "\n"
    prefix += function(camera, "void Camera_Init(").replace("Camera_Init", "NativeCameraInit", 1)
    prefix += "\n" + function(play, "s16 Play_ChangeCameraStatus(").replace("Play_ChangeCameraStatus", "NativeStatus", 1)
    prefix += "\n" + function(play, "void Play_ClearCamera(").replace("Play_ClearCamera", "NativeClearCamera", 1)
    return prefix


def run_oot(temporary):
    source = (ROOT / "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp").read_text()
    declarations = re.findall(r"constexpr u16 kBrowseText[^;]+;", source)
    declarations += re.findall(r"constexpr Vec3f kShelves\[8\] = \{.*?\};", source, re.S)
    native = (ROOT / "combo/BlackMarketBrowsingNative.h").read_text()
    signatures = ("void PlayBrowseCursorSound(", "bool BrowsingMessage(", "void ContinueBrowsingText(",
                  "void EndBrowsing(", "bool BeginBrowsing(", "void UpdateBrowsingCamera(PlayState* play) {",
                  "void MonitorBrowsing(", "void BrowseInput(", "void DrawBrowseCursor(")
    bodies = [block_from(native, native.index(s)) for s in signatures]
    bodies.append(block_from(source, source.index("void KeeperUpdate(")))
    bodies.append(function(source, 'extern "C" int ComboBlackMarketNative_PrepareViewpointCamera('))
    actor = (ROOT / "soh/src/code/z_actor.c").read_text()
    skin = (ROOT / "soh/src/code/z_skin_matrix.c").read_text()
    bodies[:0] = [function(skin, "void SkinMatrix_Vec3fMtxFMultXYZW("),
                  function(actor, "void Actor_ProjectPos("), function(actor, "void Actor_GetScreenPos(")]
    shop = (ROOT / "soh/src/overlays/actors/ovl_En_Ossan/z_en_ossan.c").read_text()
    cursor = function(shop, "void EnOssan_UpdateCursorPos(").replace("EnOssan_UpdateCursorPos", "NativeCursorPos", 1)
    joystick = function(shop, "void EnOssan_UpdateJoystickInputState(").replace("EnOssan_UpdateJoystickInputState", "NativeJoystick", 1)
    begin = shop.index("u16 dLeft = BTN_DLEFT;")
    dpad = shop[begin:shop.index("// Stick Left", begin)]
    bodies[3:3] = [re.sub(r"\bthis\b", "current", cursor), re.sub(r"\bthis\b", "current", joystick),
                   "int NativeDpadIntent(u16 buttons) { " + dpad + " return (buttons & dLeft) ? -1 : (buttons & dRight) ? 1 : 0; }"]
    fixture = (ROOT / "tests/black_market/browsing_native_test.cpp").read_text()
    graph = (ROOT / "soh/src/code/graph.c").read_text()
    fixture = fixture.replace("/* NATIVE DEBUG */", "\n".join(function(graph, s) for s in ("void Graph_OpenDisps(", "void Graph_CloseDisps(")))
    camera = (ROOT / "soh/src/code/z_camera.c").read_text()
    play = (ROOT / "soh/src/code/z_play.c").read_text()
    fixture = fixture.replace("/* NATIVE CAMERA */", re.sub(r"\bthis\b", "current", native_camera_path(camera, play)))
    fixture = fixture.replace("/* PRODUCTION */", "\n".join(declarations) + '\n#include "combo/BlackMarketShopCameraNative.h"\n' + "\n".join(bodies))
    cpp = Path(temporary) / "soh.cpp"; cpp.write_text(fixture)
    binary = Path(temporary) / "soh"
    paths = ["soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/soh", "libultraship/include", "combo"]
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                    "-DOOT_BROWSING_TEST", "-DBLACK_MARKET_BROWSING_OOT", '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', '-DCVAR_PREFIX_SETTING="gSettings"', "-g",
                    "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                    "-I" + str(ROOT), *["-I" + str(ROOT / p) for p in paths], str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="market-browsing-") as temporary:
        run_oot(temporary)
    mm = ROOT / "tests/black_market/run_shop_camera_mm_tests.py"
    if mm.exists():
        subprocess.run([sys.executable, str(mm)], check=True)
