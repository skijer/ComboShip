#!/usr/bin/env python3
"""Exercise real MM arrival/camera/passage math without a game archive or GPU."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(path, name):
    source = (ROOT / path).read_text()
    match = re.search(r"^[^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    assert match, name
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    bodies = []
    for source, names in [
        ("mm/src/code/z_lib.c", ["Math_CosS", "Math_SinS", "Math_Vec3f_Copy", "Math_Vec3f_Diff", "Math_Vec3f_DistXYZ"]),
        ("mm/src/code/z_skin_matrix.c", ["SkinMatrix_SetTranslate"]),
        ("mm/src/code/sys_matrix.c", ["Matrix_Translate", "Matrix_RotateYS", "Matrix_MultVec3f"]),
        ("mm/src/code/z_actor.c", ["Player_GetHeight", "Actor_GetFocus", "Actor_GetWorldPosShapeRot"]),
        ("mm/src/code/z_olib.c", ["OLib_VecSphToVec3f", "OLib_VecGeoToVec3f", "OLib_AddVecGeoToVec3f"]),
        ("mm/src/code/z_camera.c", ["Camera_GetFocalActorHeight", "Camera_SetStateFlag", "Camera_InitFocalActorSettings",
                                  "Camera_fabsf", "Camera_ScaledStepToCeilF", "Camera_ClampDist2"]),
    ]:
        bodies.extend(function(source, name) for name in names)
    passage = (ROOT / "mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp").read_text()
    geometry = passage[passage.index("constexpr int kArchSegments"):passage.index("} // namespace")]
    drawing = function("mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp", "DrawPassage")
    # Select the unchanged Woods-of-Mystery branch; the opening forest uses its native spawn.
    transform_start = drawing.index("        Matrix_Translate(160.0f")
    transform = drawing[transform_start:drawing.index("    }", transform_start)]
    assert "G_LIGHTING | G_CULL_BOTH" in drawing, "Update visibility check if passage culling changes"
    player_init = function("mm/src/overlays/actors/ovl_player_actor/z_player.c", "Player_Init")
    begin = player_init.index("                    Math_Vec3f_Copy(&this->actor.world.pos,")
    end = player_init.index("                    this->actor.params =", begin)
    respawn = player_init[begin:player_init.index(";", end) + 1]
    respawn = re.sub(r"\bthis\b", "player", respawn)
    fixture = (ROOT / "tests/lost_woods_entrance/mm_camera_arrival_test.cpp").read_text()
    fixture = fixture.replace("/* NATIVE_BODIES */", "\n".join(bodies))
    camera_data = (ROOT / "mm/src/code/z_camera_data.inc").read_text()
    normal_data = re.search(r"CameraModeValue sSetNormal0ModeNormalData\[\] = \{.*?\n\};", camera_data, re.S)[0]
    normal = function("mm/src/code/z_camera.c", "Camera_Normal1")
    normal_reads = normal[normal.index("    roData->unk_00 ="):normal.index("    if (RELOAD_PARAMS(camera))")]
    normal_reads += re.search(r"        roData->unk_20 = [^;]+;", normal)[0]
    fixture = fixture.replace("/* NORMAL_PRESET */", "typedef struct { s16 val, param; } CameraModeValue;\n"
                              + normal_data + "\n#define GET_NEXT_RO_DATA(values) ((values++)->val)\n"
                              "static Normal1ReadOnlyData NativeNormalPreset(Camera* camera) {\n"
                              "Normal1ReadOnlyData result{}, *roData = &result;\n"
                              "CameraModeValue* values = sSetNormal0ModeNormalData;\n"
                              "f32 focalActorHeight = Camera_GetFocalActorHeight(camera);\n"
                              + normal_reads + "\nreturn result;\n}\n")
    fixture = fixture.replace("/* ARRIVAL_BODY */", function("mm/2s2h/BenPort.cpp", "MM_ApplyComboPortalArrival"))
    fixture = fixture.replace("/* PASSAGE_GEOMETRY */", geometry)
    fixture = fixture.replace("/* PASSAGE_TRANSFORM */", "static void ApplyPassageTransform() {\n" + transform + "}\n")
    fixture = fixture.replace("/* PLAYER_RESPAWN */", "static void ApplyNativeReturnRespawn(Player* player) {\n"
                              "s32 respawnIndex = RESPAWN_MODE_RETURN;\n" + respawn + "}\n")
    includes = ["-I" + str(ROOT / path) for path in
                ("mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "libultraship/include", "combo")]
    options = ["-w", "-DF3DEX_GBI_2", "-ffunction-sections", "-fdata-sections", *includes]
    if args.sanitize:
        options += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="mm-forest-camera-") as temporary:
        directory = Path(temporary)
        source = directory / "arrival.cpp"
        source.write_text(fixture)
        objects = []
        for native in ("mm/src/code/sys_math3d.c", "mm/src/libultra/gu/sins.c", "mm/src/libultra/gu/coss.c"):
            obj = directory / (Path(native).stem + ".o")
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *options, "-include", "global.h",
                            "-c", str(ROOT / native), "-o", str(obj)], check=True)
            objects.append(str(obj))
        binary = directory / "arrival"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *options, str(source), *objects,
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
