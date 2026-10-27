#!/usr/bin/env python3
"""Replay native MM and OoT ocarina cameras in combined and both solo modes.

Scene resource decoding, actor allocation and unobstructed collision queries are
controlled boundaries. Player cutscene assignment/queue/start/stop, subcamera
allocation, settings and KeepOn4 use production code and native engine types.
This is source integration evidence, not configured gameplay verification.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path[:0] = [str(ROOT / "scripts/diagnostics"), str(ROOT / "scripts/assets"),
               str(ROOT / "tools/black_market")]
from run_time_pedestal_tests import block_from, functions
import native_forest
import prepare_native_forest
from run_oot_tests import main as run_oot_tests


def array(source, signature):
    return block_from(source, source.index(signature)) + ";\n"


def build_source():
    camera = (ROOT / "mm/src/code/z_camera.c").read_text()
    data = (ROOT / "mm/src/code/z_camera_data.inc").read_text()
    play = (ROOT / "mm/src/code/z_play.c").read_text()
    actor = (ROOT / "mm/src/code/z_actor.c").read_text()
    startup = (ROOT / "mm/2s2h/z_play_2SH.cpp").read_text()
    native = data[:data.index("} CameraSetting;") + len("} CameraSetting;")]
    native += "\n#define CAM_SETTING_MODE_ENTRY(func, data) { func, ARRAY_COUNT(data), data }\n"
    native += "#define CAM_SETTING_MODE_NULL {0, 0, nullptr}\n"
    native += array((ROOT / "mm/src/code/z_inventory.c").read_text(), "u32 gBitFlags[")
    for name in ("D_801B53EC", "D_801B5410", "D_801B5434", "D_801B5458", "D_801B547C",
                 "sSetCutscenesModeNormalData", "sSetNormal0ModeNormalData"):
        native += array(data, "CameraModeValue " + name + "[")
    for name in ("sCamSetItem1Modes", "sCamSetFree0Modes"):
        native += array(data, "CameraMode " + name + "[")
    native += "CameraMode sCamSetNormal0Modes[] = {CAM_SETTING_MODE_ENTRY(CAM_FUNC_NORMAL1, sSetNormal0ModeNormalData)};\n"
    native += "CameraSetting sCameraSettings[CAM_SET_MAX]{};\nvoid InitNativeCameraTable() {\n"
    for setting, modes in (("CAM_SET_ITEM1", "sCamSetItem1Modes"),
                           ("CAM_SET_FREE0", "sCamSetFree0Modes"),
                           ("CAM_SET_NORMAL0", "sCamSetNormal0Modes")):
        entry = re.search(r"^\s*(\{[^\n]*" + modes + r"[^\n]*\}),", data, re.M)[1]
        native += "sCameraSettings[" + setting + "] = " + entry + ";\n"
    native += "}\n" + array(data, "s16 sGlobalCamDataSettings[")
    native += "s16* sGlobalCamDataSettingsPtr = &sGlobalCamDataSettings[ARRAY_COUNT(sGlobalCamDataSettings) - 1];\n"
    trig = (ROOT / "mm/src/code/stubs.c").read_text()
    native += array(trig, "static s16 sintable[")
    native += "\n".join(functions(trig, {"sins", "coss"}).values())
    for path, names in (
        ("mm/src/code/z_lib.c", ("Math_SinS", "Math_CosS", "Math_Vec3f_Copy", "Lib_SegmentedToVirtual")),
        ("mm/src/code/z_olib.c", ("OLib_Vec3fDist", "OLib_VecSphToVec3f", "OLib_VecGeoToVec3f",
                                 "OLib_Vec3fToVecSph", "OLib_Vec3fToVecGeo", "OLib_Vec3fDiffToVecGeo", "OLib_AddVecGeoToVec3f")),
        ("mm/src/code/z_actor.c", ("Player_GetHeight", "Actor_GetFocus", "Actor_GetWorld", "Actor_GetWorldPosShapeRot", "Actor_SpawnEntry")),
        ("mm/src/code/z_camera.c", ("Camera_SetStateFlag", "Camera_UnsetStateFlag", "Camera_fabsf", "Camera_ScaledStepToCeilF",
                                   "Camera_ScaledStepToCeilS", "Camera_ScaledStepToCeilVec3f", "Camera_SetUpdateRatesSlow",
                                   "Camera_SetUpdateRatesFastPitch", "Camera_SetFocalActorAtOffset", "Camera_GetFocalActorHeight",
                                   "Camera_ResetActionFuncState", "func_800DF498", "Camera_ChangeModeFlags", "Camera_ChangeSettingFlags",
                                   "Camera_ChangeSetting", "Camera_GetBgCamOrActorCsCamSetting", "Camera_ChangeActorCsCamIndex",
                                   "Camera_Init", "Camera_ChangeStatus", "Camera_Copy", "Camera_CalcXZAngle", "Camera_KeepOn4",
                                   "Camera_UpdateInterface")),
        ("mm/src/code/z_play.c", ("Play_CreateSubCamera", "Play_GetActiveCamId", "Play_ChangeCameraStatus", "Play_ClearCamera",
                                 "Play_GetCamera", "Play_CopyCamera", "Play_GetActorCsCamSetting", "Play_AssignPlayerCsIdsFromScene")),
        ("mm/src/overlays/actors/ovl_player_actor/z_player.c", ("func_808323C0", "func_808525C4", "Player_Action_63")),
        ("mm/src/code/z_parameter.c", ("Interface_SetHudVisibility", "Interface_UpdateButtonAlphasByStatus",
                                      "Interface_UpdateButtonAlphas", "Interface_UpdateHudAlphas")),
    ):
        bodies = functions((ROOT / path).read_text(), set(names))
        assert set(bodies) == set(names), (path, set(names) - set(bodies))
        native += "\n".join(bodies[name] for name in names) + "\n"
    native = array(play, "s16 sPlayerCsIdToCsCamId[") + native
    manager = re.sub(r"^#include[^\n]*", "", (ROOT / "mm/src/code/z_eventmgr.c").read_text(), flags=re.M)
    native += manager
    message = (ROOT / "mm/src/code/z_message.c").read_text()
    for name in ("D_801D028C", "sOcarinaSongFlagsMap"):
        native += array(message, "u16 " + name + "[")
    native += "\n".join(functions(message, {"Message_DisplayOcarinaStaffImpl", "Message_DisplayOcarinaStaff"}).values())
    parameter = (ROOT / "mm/src/code/z_parameter.c").read_text()
    hud_update = functions(parameter, {"Interface_Update"})["Interface_Update"]
    native += "\nvoid NativeHudFrame(PlayState* play) { InterfaceContext* interfaceCtx = &play->interfaceCtx; s16 dimmingAlpha, risingAlpha;\n"
    native += block_from(hud_update, hud_update.index("    switch (gSaveContext.nextHudVisibility)")) + "\n}\n"
    mirror = (ROOT / "combo/gameplay/MagicMirrorRuntime.inc.h").read_text()
    mirror_state = mirror[mirror.index("struct Context {"):mirror.index("void QueueArrival()")]
    mirror_names = ("QueueArrival", "Mode", "Locate", "ArrivalDestination", "Landing", "PrepareArrival", "FinishArrival")
    mirror_functions = functions(mirror, set(mirror_names))
    native += "\n#ifdef COMBO_BUILD\n#define COMBO_MAGIC_MIRROR_MM\nnamespace MirrorRuntime {\n"
    native += mirror_state + "\n" + "\n".join(mirror_functions[name] for name in mirror_names)
    native += "\n}\n#undef COMBO_MAGIC_MIRROR_MM\n#endif\n"
    for name in ("ComboBlackMarket_PrepareSpawn",):
        native += "#ifdef COMBO_BUILD\n" + functions(startup, {name})[name] + "\n#endif\n"
    native += block_from(startup, startup.index('extern "C" void OTRPlay_InitScene('))
    # This native function contains a commented-out opening brace.
    room_startup = re.sub(r"//[^\n]*", "", startup)
    native += block_from(room_startup, room_startup.index('extern "C" s32 OTRfunc_800973FC('))
    native = re.sub(r"\bthis\b", "self", native)
    normalization = re.search(r"    actor->csId = csId & 0x7F;.*?\n    }", actor, re.S)[0]
    fixture = (ROOT / "tests/forest_ocarina/native_test.cpp").read_text()
    fixture = fixture.replace("/* NATIVE CODE */", native).replace("/* CS_ID_NORMALIZATION */", normalization)
    resources = prepare_native_forest.prepared_resources(prepare_native_forest.DEFAULT_MANIFEST)
    for mode, marker in (("combined", "COMBINED"), ("mm", "SOLO")):
        commands = native_forest.read_scene(resources[native_forest.SCENES[mode]])
        assert 27 not in commands, "Update the metadata-free forest control when the source bundle changes"
        spawn = native_forest.read_actor_list(commands[0])[0]
        literal = "{" + str(spawn[0]) + ",{" + ",".join(map(str, spawn[1:4])) + "},{" + ",".join(map(str, spawn[4:7])) + "}," + str(spawn[7]) + "}"
        fixture = fixture.replace("/* " + marker + " SPAWN */", literal)
        for room in range(3):
            room_commands = native_forest.read_scene(resources[native_forest.SCENES[mode] + f"_room_{room}"])
            assert 27 not in room_commands, "A room-authored cutscene list needs an explicit ownership control"
            assert native_forest.read_mesh(room_commands[10])["type"] == 2
    return fixture


def main():
    paths = (".", "mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "libultraship/include", "combo")
    with tempfile.TemporaryDirectory(prefix="forest-ocarina-") as temporary:
        source = Path(temporary) / "native.cpp"
        source.write_text(build_source())
        for combo in (True, False):
            binary = Path(temporary) / ("combo" if combo else "standalone")
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2",
                            *(["-DCOMBO_BUILD"] if combo else []), "-g", "-fsanitize=address,undefined",
                            "-fno-sanitize-recover=all", "-no-pie", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                            *["-I" + str(ROOT / path) for path in paths], str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
    run_oot_tests()


if __name__ == "__main__":
    main()
