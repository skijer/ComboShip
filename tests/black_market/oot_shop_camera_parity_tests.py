#!/usr/bin/env python3
"""Compare the MM port with the actual OoT Data4 implementation and math.

Both camera update functions and both native geographic conversion paths are
compiled from production source. Only player-height/scene-data services and the
disabled developer-register/camera-drift enhancement boundaries are controlled.
This is source-level port evidence, not configured game/runtime evidence.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

from run_shop_camera_mm_tests import array, function

ROOT = Path(__file__).resolve().parents[2]


def build_source():
    mm = (ROOT / "mm/src/code/z_camera.c").read_text()
    oot = (ROOT / "soh/src/code/z_camera.c").read_text()
    mm_data = (ROOT / "mm/src/code/z_camera_data.inc").read_text()
    oot_data = (ROOT / "soh/src/code/z_camera_data.inc").read_text()
    olib = (ROOT / "mm/src/code/z_olib.c").read_text()
    oot_olib = (ROOT / "soh/src/code/z_olib.c").read_text()
    trig = (ROOT / "mm/src/code/stubs.c").read_text()
    lib = (ROOT / "mm/src/code/z_lib.c").read_text()
    native = array(trig, "static s16 sintable[")
    native += "\n".join(function(trig, s) for s in ("s16 sins(", "s16 coss("))
    native += "\n".join(function(lib, s) for s in ("f32 Math_SinS(", "f32 Math_CosS("))
    native += "\n".join(function(olib, s) for s in (
        "f32 OLib_Vec3fDist(", "Vec3f OLib_VecSphToVec3f(", "Vec3f OLib_VecGeoToVec3f(",
        "VecSph OLib_Vec3fToVecSph(", "VecGeo OLib_Vec3fToVecGeo(",
        "VecGeo OLib_Vec3fDiffToVecGeo(", "Vec3f OLib_AddVecGeoToVec3f("))
    native += "\n".join(function(mm, s) for s in (
        "f32 Camera_fabsf(", "f32 Camera_ScaledStepToCeilF(",
        "void Camera_ScaledStepToCeilVec3f(", "Vec3f Camera_Vec3sToVec3f(", "s32 Camera_Noop("))
    # The initial regression run also supports the old no-op: it must fail on
    # camera output, rather than fail to compile because the new API is absent.
    if "void Camera_SetOotShopFaceAngle(" in mm:
        native += function(mm, "void Camera_SetOotShopFaceAngle(")
    else:
        native += "void Camera_SetOotShopFaceAngle(Camera*, f32) {}\n"
    native += function(mm, "s32 Camera_Data4(")
    if "sSetBlackMarketOotModeNormalData" in mm_data:
        values = array(mm_data, "CameraModeValue sSetBlackMarketOotModeNormalData[")
    else:
        values = "CameraModeValue sSetBlackMarketOotModeNormalData[] = {{-40,0},{60,7},{0x3F00,9}};\n"
    donor = array(oot_data, "s16 sOREGInit[")
    donor += "\n".join(function(oot_olib, s) for s in (
        "f32 OLib_Vec3fDist(", "Vec3f* OLib_VecSphToVec3f(", "Vec3f* OLib_VecSphGeoToVec3f(",
        "VecSph* OLib_Vec3fToVecSph(", "VecSph* OLib_Vec3fToVecSphGeo(",
        "VecSph* OLib_Vec3fDiffToVecSphGeo("))
    donor += "\n".join(function(oot, s) for s in (
        "f32 Camera_LERPCeilF(", "void Camera_LERPCeilVec3f(", "void func_80043B60(",
        "Vec3f* Camera_Vec3sToVec3f(", "Vec3f* Camera_Vec3fVecSphGeoAdd(",
        "s32 Camera_CalcAtDefault(", "s32 Camera_Data4("))
    donor = donor.replace("OLib_Vec3fDist(", "Donor_Vec3fDist(").replace(
        "Player_GetHeight(", "Donor_PlayerGetHeight(")
    fixture = (ROOT / "tests/black_market/oot_shop_camera_parity_test.cpp").read_text()
    return fixture.replace("/* MM MODE VALUES */", values).replace("/* MM NATIVE SOURCE */", native).replace(
        "/* OOT MODE VALUES */", array(oot_data, "CameraModeValue sSetPivotShopBrowsingModeNormalData[")).replace(
        "/* OOT NATIVE SOURCE */", donor)


def main():
    with tempfile.TemporaryDirectory(prefix="market-oot-data4-parity-") as temporary:
        cpp = Path(temporary) / "parity.cpp"
        cpp.write_text(build_source())
        binary = Path(temporary) / "parity"
        includes = (".", "mm", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm/2s2h",
                    "libultraship/include", "combo")
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        *["-I" + str(ROOT / p) for p in includes], str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
