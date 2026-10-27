#!/usr/bin/env python3
"""Run actual native MM shop actions through the production adapter/camera path.

The complete EnOssan C translation unit and actual EnGirlA update functions run.
No CutsceneManager, Data4, mode/setting selection, camera allocation, or camera
copy routine is replaced. The fixture controls input/messages, actor animation,
player cs-action, HUD/audio, and an unobstructed collision-query boundary. This
is native source integration evidence, not configured game/runtime evidence.
"""
from pathlib import Path
import json
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def function(source, signature):
    match = re.search(r"^" + re.escape(signature) + r"[^;{]*\{", source, re.M)
    if match is None:
        raise AssertionError("native function missing: " + signature)
    return block_from(source, match.start())


def array(source, signature):
    return block_from(source, source.index(signature)) + ";\n"


def native_engine_source(observe=False):
    camera = (ROOT / "mm/src/code/z_camera.c").read_text()
    data = (ROOT / "mm/src/code/z_camera_data.inc").read_text()
    play = (ROOT / "mm/src/code/z_play.c").read_text()
    lib = (ROOT / "mm/src/code/z_lib.c").read_text()
    actor = (ROOT / "mm/src/code/z_actor.c").read_text()
    olib = (ROOT / "mm/src/code/z_olib.c").read_text()
    trig = (ROOT / "mm/src/code/stubs.c").read_text()
    skin = (ROOT / "mm/src/code/z_skin_matrix.c").read_text()
    prefix = data[:data.index("} CameraSetting;") + len("} CameraSetting;")]
    prefix += '\n#define CAM_SETTING_MODE_ENTRY(func, data) { func, ARRAY_COUNT(data), data }\n'
    for values, modes in (("D_801B5824", "sCamSetFixed1Modes"),
                          ("sSetBlackMarketOotModeNormalData", "sCamSetBlackMarketOotModes"),
                          ("sSetCutscenesModeNormalData", "sCamSetFree0Modes")):
        prefix += array(data, "CameraModeValue " + values + "[")
        prefix += array(data, "CameraMode " + modes + "[")
    prefix += "CameraSetting sCameraSettings[CAM_SET_MAX]{};\nvoid InitNativeCameraTable() {\n"
    for setting, modes in (("CAM_SET_FIXED1", "sCamSetFixed1Modes"), ("CAM_SET_FREE0", "sCamSetFree0Modes"),
                           ("CAM_SET_BLACK_MARKET_OOT", "sCamSetBlackMarketOotModes")):
        entry = re.search(r"^\s*(\{[^\n]*" + modes + r"[^\n]*\}),", data, re.M).group(1)
        prefix += "sCameraSettings[" + setting + "] = " + entry + ";\n"
    prefix += "}\n" + array(data, "s16 sGlobalCamDataSettings[")
    prefix += "s16* sGlobalCamDataSettingsPtr = &sGlobalCamDataSettings[ARRAY_COUNT(sGlobalCamDataSettings) - 1];\n"
    prefix += array(trig, "static s16 sintable[")
    prefix += "\n".join(function(trig, signature) for signature in ("s16 sins(", "s16 coss("))
    prefix += "\n".join(function(lib, signature) for signature in (
        "f32 Math_SinS(", "f32 Math_CosS(", "void Math_Vec3f_Copy(", "void* Lib_SegmentedToVirtual(",
        "void Math_ApproachF(", "s16 Math_SmoothStepToS(", "s32 Math_StepToIImpl(", "void Math_StepToIGet("))
    prefix += function(skin, "void SkinMatrix_Vec3fMtxFMultXYZW(")
    prefix += "\n".join(function(actor, signature) for signature in (
        "PosRot Actor_GetFocus(", "PosRot Actor_GetWorld(", "PosRot Actor_GetWorldPosShapeRot(",
        "void Actor_GetProjectedPos(", "void Actor_GetScreenPos(", "void Actor_SetFocus(", "s32 Actor_HasParent("))
    prefix += "\n".join(function(olib, signature) for signature in (
        "f32 OLib_Vec3fDist(", "Vec3f OLib_VecSphToVec3f(", "Vec3f OLib_VecGeoToVec3f(",
        "VecSph OLib_Vec3fToVecSph(", "VecGeo OLib_Vec3fToVecGeo(",
        "VecGeo OLib_Vec3fDiffToVecGeo(", "Vec3f OLib_AddVecGeoToVec3f("))
    prefix += "\n".join(function(camera, signature) for signature in (
        "void Camera_UpdateInterface(", "s16 Camera_SetStateFlag(", "s16 Camera_UnsetStateFlag(", "f32 Camera_fabsf(",
        "f32 Camera_ScaledStepToCeilF(", "s16 Camera_ScaledStepToCeilS(",
        "void Camera_ScaledStepToCeilVec3f(", "void Camera_SetUpdateRatesSlow(",
        "void Camera_SetUpdateRatesFastPitch(", "Vec3f Camera_Vec3sToVec3f(",
        "void Camera_SetFocalActorAtOffset(", "f32 Camera_GetFocalActorHeight(",
        "void Camera_ResetActionFuncState(", "s32 func_800DF498(",
        "s32 Camera_ChangeModeFlags(", "s16 Camera_ChangeSettingFlags(",
        "s32 Camera_ChangeSetting(", "s16 Camera_GetBgCamOrActorCsCamSetting(",
        "Vec3s* Camera_GetBgCamOrActorCsCamFuncData(", "s32 Camera_ChangeActorCsCamIndex(",
        "void Camera_Init(", "s32 Camera_ChangeStatus(", "s32 Camera_Copy(",
        "s32 Camera_BgCheck(", "s32 Camera_Fixed1(", "s32 Camera_Noop(",
        "void Camera_SetOotShopFaceAngle(", "s32 Camera_Data4("))
    prefix += "\n".join(function(play, signature) for signature in (
        "s16 Play_CreateSubCamera(", "s16 Play_GetActiveCamId(", "s32 Play_ChangeCameraStatus(",
        "void Play_ClearCamera(", "Camera* Play_GetCamera(", "void Play_CopyCamera(",
        "u16 Play_GetActorCsCamSetting(", "Vec3s* Play_GetActorCsCamFuncData("))
    manager = (ROOT / "mm/src/code/z_eventmgr.c").read_text()
    manager = re.sub(r"^#include[^\n]*", "", manager, flags=re.M)
    prefix += manager
    # C translation units use implicit void* conversion; spell the same cast
    # explicitly for this C++ fixture without weakening compiler diagnostics.
    prefix = prefix.replace("return Lib_SegmentedToVirtual(actorCsCamList->actorCsCamFuncData);",
                            "return static_cast<Vec3s*>(Lib_SegmentedToVirtual(actorCsCamList->actorCsCamFuncData));")
    if observe:
        names = ("Queue", "IsNext", "StartWithPlayerCsAndSetFlag", "Stop")
        for name in names:
            prefix = re.sub(r"^(void|s16) CutsceneManager_" + name + r"\(",
                            r"\1 Native_CutsceneManager_" + name + "(", prefix, flags=re.M)
        prefix += r"""
extern "C" void CutsceneManager_Queue(s16 id) {
    nativeCalls.push_back("queue" + std::to_string(id)); Native_CutsceneManager_Queue(id);
}
extern "C" s16 CutsceneManager_IsNext(s16 id) {
    const auto result = Native_CutsceneManager_IsNext(id);
    nativeCalls.push_back("next" + std::to_string(id) + ":" + std::to_string(result)); return result;
}
extern "C" s16 CutsceneManager_StartWithPlayerCsAndSetFlag(s16 id, Actor* actor) {
    nativeCalls.push_back("start" + std::to_string(id)); return Native_CutsceneManager_StartWithPlayerCsAndSetFlag(id, actor);
}
extern "C" s16 CutsceneManager_Stop(s16 id) {
    nativeCalls.push_back("stop" + std::to_string(id)); return Native_CutsceneManager_Stop(id);
}
"""
    return re.sub(r"\bthis\b", "self", prefix)


def compile_native_shop(temporary, source_path=None):
    """Compile the complete actor as C; only declared service boundaries differ."""
    source_path = source_path or ROOT / "mm/src/overlays/actors/ovl_En_Ossan/z_en_ossan.c"
    native = Path(temporary) / "z_en_ossan.c"
    native.write_text(Path(source_path).read_text())
    native_object = Path(temporary) / "z_en_ossan.o"
    paths = (".", "mm", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm/2s2h",
             "libultraship/include", "combo", "mm/src/overlays/actors/ovl_En_Ossan", "tests/black_market")
    includes = ["-I" + str(ROOT / path) for path in paths]
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                    "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie",
                    "-ffunction-sections", "-fdata-sections", *includes,
                    "-include", str(ROOT / "tests/black_market/native_mm_shop_boundary.h"),
                    "-c", str(native), "-o", str(native_object)], check=True)
    return native_object, includes


def build_source():
    source = (ROOT / "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp").read_text()
    native_adapter = (ROOT / "mm/2s2h/Enhancements/Gameplay/BlackMarketMmNativeShop.h").read_text()
    assert '"BlackMarketBrowsingNative.h"' not in source and "BrowseInput(" not in source and "sBrowser" not in source
    assert "BlackMarketBrowsing::State" not in native_adapter
    assert not any(name in native_adapter for name in ("Camera_SetAtEye", "Camera_SetFov", "Camera_SetCameraData"))
    constants = "\n".join(re.findall(r"(?:alignas\(2\) )?constexpr (?:u16|char) k(?:BrowseText|KeeperMenuText|Keeper|Card)[^;]+;", source))
    constants += "\n" + array(source, "constexpr Vec3f kShelves[")
    production = '#include "mm/2s2h/Enhancements/Gameplay/BlackMarketMmNativeShop.h"\n'
    for signature in ("void KeeperUpdate(", "s32 KeeperLimb(", "void KeeperDraw(", "void InitKeeper(",
                      "void NativeKeeperDestroyed(", "void InitNativeKeeper(",
                      'extern "C" const ComboBlackMarketShopAdapter* ComboBlackMarketShop_GetAdapter('):
        production += function(source, signature) + "\n"
    item = (ROOT / "mm/src/overlays/actors/ovl_En_GirlA/z_en_girla.c").read_text()
    item = "\n".join(function(item, signature) for signature in (
        "void EnGirlA_DoNothing(", "s32 EnGirlA_TrySetMaskItemDescription(",
        "void EnGirlA_Update2(", "void EnGirlA_Update("))
    fixture = (ROOT / "tests/black_market/shop_camera_mm_test.cpp").read_text()
    # Exercise the real hook registry and Actor_Draw dispatch block. With the
    # baseline's absent registration this intentionally leaves the actor visible.
    hooks = "\n".join(block_from(source, match.start()) + ");" for match in re.finditer(
        r"GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldActorDraw>", source))
    interactor = (ROOT / "mm/2s2h/GameInteractor/GameInteractor.cpp").read_text()
    gate = function(interactor, "bool GameInteractor_ShouldActorDraw(")
    actor_source = (ROOT / "mm/src/code/z_actor.c").read_text()
    dispatch = block_from(actor_source, actor_source.index("if (GameInteractor_ShouldActorDraw(actor))"))
    fixture = fixture.replace("/* PRODUCTION PLAYER DRAW HOOK */", gate +
        "\nvoid RegisterPlayerDrawHook() {\n" + hooks + "\n}\n" +
        "void DrawPlayerBoundary(Actor* actor) { auto* play = gPlayState;\n" + dispatch + "\n}\n")
    support = (ROOT / "tests/black_market/native_mm_shop_support.h").read_text()
    shrink = (ROOT / "mm/src/code/z_shrink_window.c").read_text()
    native_shrink = '#include "z64shrink_window.h"\n'
    native_shrink += shrink[shrink.index("typedef struct {"):shrink.index("void ShrinkWindow_Letterbox_SetSizeTarget")]
    for signature in ("void ShrinkWindow_Letterbox_SetSizeTarget(", "s32 ShrinkWindow_Letterbox_GetSizeTarget(",
                      "void ShrinkWindow_Letterbox_SetSize(", "s32 ShrinkWindow_Letterbox_GetSize(",
                      "void ShrinkWindow_Init(", "void ShrinkWindow_Update("):
        body = function(shrink, signature)
        if "SetSizeTarget(" in signature:
            body = body.replace("{", "{\nlastLetterbox = target; ++letterboxCalls;", 1)
        native_shrink += body + "\n"
    support = support.replace(function(support, 'extern "C" void ShrinkWindow_Letterbox_SetSizeTarget('), native_shrink)
    fixture = fixture.replace("/* NATIVE MM SUPPORT */", support)
    inspection_hooks = "\n".join(block_from(source, match.start()) + ");" for match in re.finditer(
        r"GameInteractor::Instance->RegisterGameHookForID<GameInteractor::ShouldVanillaBehavior>", source))
    view = (ROOT / "mm/src/code/z_view.c").read_text()
    boundary = '#undef GameInteractor_Should\n' + function(interactor, "bool GameInteractor_Should(")
    boundary += "\nvoid RegisterInspectionLetterboxHook() {\n" + inspection_hooks + "\n}\n"
    boundary += 's32 gScreenWidth = 320, gScreenHeight = 240;\n'
    boundary += 'extern "C" int16_t OTRGetRectDimensionFromLeftEdge(float value) { return value; }\n'
    boundary += 'extern "C" int16_t OTRGetRectDimensionFromRightEdge(float value) { return value; }\n'
    boundary += function(view, "void View_SetScissor(") + "\n"
    boundary += function(view, "void View_ApplyLetterbox(") + "\n"
    boundary += function(shrink, "void ShrinkWindow_Draw(") + "\n"
    user_bars = (ROOT / "mm/2s2h/Enhancements/Graphics/DisableBlackBars.cpp").read_text()
    boundary += function(user_bars, "void RegisterDisableBlackBars(").replace(
        "CVAR", 'CVarGetInteger("gEnhancements.Graphics.DisableBlackBars", 0)') + "\n"
    fixture = fixture.replace("/* INSPECTION LETTERBOX BOUNDARY */", boundary)
    return fixture.replace("/* NATIVE CAMERA AND MANAGER */", native_engine_source(observe=True)).replace(
        "/* PRODUCTION CONSTANTS */", constants).replace("/* NATIVE STOCK UPDATE */", re.sub(r"\bthis\b", "self", item)).replace(
        "/* PRODUCTION ADAPTER */", production)


def check_selection_clearance(binary, temporary):
    """Bound actual native forward/return frame intervals against room boards."""
    import numpy as np
    sys.path.insert(0, str(ROOT / 'scripts/assets'))
    sys.path.insert(0, str(ROOT / 'tools/black_market'))
    from prepare_native_market import prepared_resources
    from keeper_lean import room_faces
    from presentation_preview import triangle_box_intersects
    import mm_shop_presentation

    class Pack:
        resources = prepared_resources(host='mm')
        def read(self, name):
            if name == mm_shop_presentation.ROOM_ROOT:
                name = mm_shop_presentation.CLONES[name]
            return self.resources[name]

    room = np.array(room_faces(Pack()), dtype=float)
    shelf_binary = Path(temporary) / 'shelves'
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-I' + str(ROOT),
                    str(ROOT / 'tools/black_market/mm_shop_layout_preview.cpp'), '-o', str(shelf_binary)], check=True)
    fixture = json.loads(subprocess.check_output([str(shelf_binary)], text=True))
    faces = ((4, 5, 6, 7), (1, 0, 3, 2), (5, 1, 2, 6),
             (0, 4, 7, 3), (7, 6, 2, 3), (0, 1, 5, 4))
    boards = np.array([np.array(points)[[face[n] for n in indices]]
                       for points in fixture['corners'] for face in faces
                       for indices in ((0, 1, 2), (0, 2, 3))], dtype=float)
    geometry = [('room', room), ('shelves', boards)]
    geometry = [(name, triangles, triangles.min(1), triangles.max(1)) for name, triangles in geometry]
    # Authored FrameFit envelope plus actual BuildLock minimum radii/margins.
    # Arbitrary external replacement models are not bounded by this proof.
    relative = np.array(fixture['envelope'], dtype=float)
    paths = json.loads(subprocess.check_output([str(binary), 'selection-paths'], text=True,
                                              env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'}))
    assert len(paths) == 5 * 8
    for case, frames in enumerate(paths):
        form, slot = divmod(case, 8)
        for first, last in zip(frames, frames[1:]):
            start, end = np.array(first['position']), np.array(last['position'])
            # Each hull encloses the entire interval, including linear render
            # interpolation between native updates. Subdivision reduces false positives.
            for step in range(32):
                a = start + (end - start) * step / 32
                b = start + (end - start) * (step + 1) / 32
                low, high = np.minimum(a, b) + relative[0], np.maximum(a, b) + relative[1]
                for name, triangles, minimum, maximum in geometry:
                    candidates = np.all(maximum >= low, axis=1) & np.all(minimum <= high, axis=1)
                    assert not any(triangle_box_intersects(face, low, high) for face in triangles[candidates]), (
                        form, slot, name, 'native selection/return interval crosses geometry', a, b)
                # Use the real ported Data4 poses. Bound rendering
                # interpolation at 1/32-frame resolution.
                from check_browsing_frame import project
                import itertools
                for fraction, point in ((step / 32, a), ((step + 1) / 32, b)):
                    view = {key: (np.array(first[key]) + fraction *
                                  (np.array(last[key]) - first[key])).tolist()
                            for key in ('eye', 'at')}
                    view['fov'] = first['fov']
                    projected = np.array([project(corner, view) for corner in
                                          itertools.product(*zip(*(relative + point)))])
                    lo, hi = projected.min(0), projected.max(0)
                    assert lo[0] > 8 and hi[0] < 312 and lo[1] > max(first['visibleTop'], last['visibleTop']) and hi[1] < 138, (
                        form, slot, 'actual native camera selection/return clips UI', fraction, lo, hi, view)
    print(f'PASS all5 forms/all8 native selection/return paths: full private item/seal envelopes clear '
          f'{len(room)} actual room and {len(boards)} shelf triangles, including frame interpolation')


def main():
    with tempfile.TemporaryDirectory(prefix="market-mm-native-camera-") as temporary:
        cpp = Path(temporary) / "mm.cpp"
        cpp.write_text(build_source())
        binary = Path(temporary) / "mm-camera"
        native_object, includes = compile_native_shop(temporary)
        command = [os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                   "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                   "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                   *includes, str(cpp), str(native_object), "-o", str(binary)]
        subprocess.run(command, check=True)
        subprocess.run([str(binary), *sys.argv[1:]], check=True,
                       env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
        if len(sys.argv) == 1:
            check_selection_clearance(binary, temporary)


if __name__ == "__main__":
    main()
