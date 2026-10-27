#!/usr/bin/env python3
"""Characterize native OoT ocarina cameras in both solo and combined modes.

Uses prepared OoT forest room/collision metadata and actual player camera
requests, camera setting/mode selection, KeepOn4 frames and cancellation.
Animation services and clear collision queries are controlled boundaries.
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


def array(source, signature):
    return block_from(source, source.index(signature)) + ";\n"


def build_source():
    data = (ROOT / "soh/src/code/z_camera_data.inc").read_text()
    camera = (ROOT / "soh/src/code/z_camera.c").read_text()
    player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    native = data[:data.index("} CameraSetting;") + len("} CameraSetting;")]
    native += "\n#define CAM_SETTING_MODE_ENTRY(func, data) { func, ARRAY_COUNT(data), data }\n"
    modes = ("sCamSetNormal0Modes", "sCamSetFree0Modes", "sCamSetTurnAroundModes")
    values = set()
    tables = ""
    for mode in modes:
        table = array(data, "CameraMode " + mode + "[")
        values.update(re.findall(r"CAM_SETTING_MODE_ENTRY\([^,]+, (\w+)\)", table))
        tables += table
    for value in sorted(values):
        native += array(data, "CameraModeValue " + value + "[")
    native += tables + "CameraSetting sCameraSettings[CAM_SET_MAX]{};\nvoid InitNativeCameraTable() {\n"
    for setting, mode in zip(("CAM_SET_NORMAL0", "CAM_SET_FREE0", "CAM_SET_TURN_AROUND"), modes):
        entry = re.search(r"^\s*(\{[^\n]*" + mode + r"[^\n]*\}),", data, re.M)[1]
        native += "sCameraSettings[" + setting + "] = " + entry + ";\n"
    native += "}\n"
    for name in ("sOREGInit", "D_8011D3B0", "D_8011D3CC"):
        native += array(data, "s16 " + name + "[")
    trig = (ROOT / "soh/src/libultra/gu/sintable.c").read_text()
    native += array(trig, "static s16 sintable[")
    for path, names in (
        ("soh/src/libultra/gu/sins.c", ("sins",)),
        ("soh/src/libultra/gu/coss.c", ("coss",)),
        ("soh/src/code/z_lib.c", ("Math_SinS", "Math_CosS", "Math_AccurateSinS", "Math_AccurateCosS", "Math_Vec3f_Copy")),
        ("soh/src/code/code_800FCE80.c", ("Math_FAtanTaylorQF", "Math_FAtanTaylorF", "Math_FAtanContFracF", "Math_FAtanF", "Math_FAtan2F")),
        ("soh/src/code/z_olib.c", ("OLib_VecSphToVec3f", "OLib_VecSphGeoToVec3f", "OLib_Vec3fToVecSph", "OLib_Vec3fToVecSphGeo", "OLib_Vec3fDiffToVecSphGeo")),
        ("soh/src/code/z_actor.c", ("Player_GetHeight", "Actor_GetWorld", "Actor_GetWorldPosShapeRot")),
        ("soh/src/code/z_bgcheck.c", ("BgCheck_GetCollisionHeader", "BgCheck_GetBgCamSettingImpl")),
        ("soh/src/code/z_camera.c", ("Camera_LERPCeilF", "Camera_LERPCeilS", "Camera_LERPCeilVec3f", "func_80043ABC",
                                    "Camera_Vec3fVecSphGeoAdd", "Camera_XZAngle", "Camera_CopyDataToRegs", "func_8005A02C",
                                    "Camera_RequestModeImpl", "Camera_ChangeSettingFlags", "Camera_RequestSetting",
                                    "Camera_GetBgCamSetting", "Camera_ChangeDataIdx", "Camera_SetCameraData", "Camera_SetParam",
                                    "Camera_BGCheck", "Camera_KeepOn4", "func_8005B1A4")),
        ("soh/src/code/z_play.c", ("Play_GetCamera", "Play_CamIsNotFixed")),
        ("soh/src/overlays/actors/ovl_player_actor/z_player.c", ("Player_RequestCameraSetting", "Player_SetTurnAroundCamera", "Player_Action_8084E3C4")),
    ):
        source = (ROOT / path).read_text()
        bodies = functions(source, set(names))
        assert set(bodies) == set(names), (path, set(names) - set(bodies))
        native += "\n".join(bodies[name] for name in names) + "\n"
    pull_start = player.index("                    Player_SetupActionPreserveItemAction(play, this, Player_Action_8084E3C4, 0);")
    pull_end = player.index("\n                }\n            } else if", pull_start)
    native += "\nvoid NativePullOutOcarina(Player* self, PlayState* play) {\n" + re.sub(r"\bthis\b", "self", player[pull_start:pull_end]) + "\n}\n"
    # Preserve the actual per-frame reset before invoking the native handler.
    begin = camera.index("    camera->unk_14A = 0;", camera.index("Vec3s Camera_Update("))
    end = camera.index("\n\n", begin)
    native += "\nvoid NativeOcarinaCameraFrame(Camera* camera) {\n" + camera[begin:end] + "\nCamera_KeepOn4(camera);\n}\n"
    native = re.sub(r"\bthis\b", "self", native)
    native = array(player, "static s16 sWarpSongEntrances[") + native
    fixture = (ROOT / "tests/forest_ocarina/oot_native_test.cpp").read_text().replace("/* NATIVE CODE */", native)
    resources = prepare_native_forest.prepared_resources(prepare_native_forest.DEFAULT_MANIFEST)
    prefix = native_forest.OOT
    room_types = [native_forest.read_mesh(native_forest.read_scene(resources[prefix + "_room_" + str(room)], oot=True)[10])["type"] for room in range(3)]
    fixture = fixture.replace("/* FOREST ROOM TYPES */", ",".join(map(str, room_types)))
    commands = native_forest.read_scene(resources[prefix], oot=True)
    collision = resources[native_forest.read_string(commands[3])]
    reader = native_forest.Reader(collision, 64)
    reader.take("6h")
    for width in (6, 16, 8):
        reader.skip(reader.take("I") * width)
    records = [reader.take("Hhi") for _ in range(reader.take("I"))]
    assert records and all(count == 0 and points == 0 for _, count, points in records)
    fixture = fixture.replace("/* FOREST CAMERA RECORDS */", ",".join("{" + str(setting) + ",0,nullptr}" for setting, _, _ in records))
    return fixture


def main():
    paths = (".", "soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/soh", "libultraship/include", "combo")
    with tempfile.TemporaryDirectory(prefix="oot-forest-ocarina-") as temporary:
        source = Path(temporary) / "native.cpp"
        source.write_text(build_source())
        for combo in (True, False):
            binary = Path(temporary) / ("combo" if combo else "standalone")
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2",
                            *(["-DCOMBO_BUILD"] if combo else []), '-DCVAR_PREFIX_SETTING="gSettings"',
                            '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', "-g", "-fsanitize=address,undefined",
                            "-fno-sanitize-recover=all", "-no-pie", "-ffunction-sections", "-fdata-sections", "-Wl,--gc-sections",
                            *["-I" + str(ROOT / path) for path in paths], str(source), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
