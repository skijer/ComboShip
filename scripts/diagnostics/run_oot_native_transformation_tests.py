#!/usr/bin/env python3
"""Run SoH's production transformation controllers without either game engine.

The animation, camera, resource and audio services are the fixture boundary.
Player/PlayState, mask IDs, animation IDs and both transition bodies are real.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

from run_mask_transformation_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    form = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    native = ROOT / "soh/mods/transformation_masks/mm_native_transform.inc.h"
    enums = form[form.index("typedef enum MmFormStateId"):]
    enums = enums[:enums.index("} MmFormStateId;") + len("} MmFormStateId;")]
    source = (ROOT / "tests/native_transformations/sequence_test.cpp").read_text()
    source = source.replace("/* PRODUCTION_STATE_ENUM */", enums)
    host_camera = (ROOT / "soh/src/code/z_camera.c").read_text()
    source = source.replace("/* PRODUCTION_CAMERA_MATH */", "\n".join(
        function(host_camera, name) for name in
        ("Camera_LERPCeilF", "Camera_LERPCeilS", "Camera_LERPCeilVec3f")))
    donor_header = (ROOT / "mm/include/z64camera.h").read_text()
    start = donor_header.rfind("typedef struct", 0, donor_header.index("} Demo4ReadOnlyData;"))
    end = donor_header.index("} Demo5;") + len("} Demo5;")
    donor = (ROOT / "mm/src/code/z_camera.c").read_text()
    reference = (ROOT / "tests/native_transformations/camera_reference.inc.h").read_text()
    reference = reference.replace("/* DONOR_CAMERA_TYPES */", donor_header[start:end])
    reference = reference.replace("/* DONOR_CAMERA_FUNCTIONS */", "\n".join(
        function(donor, name) for name in ("Camera_Demo4", "Camera_Demo5")))
    source = source.replace("/* DONOR_CAMERA_REFERENCE */", reference)
    # The Windows SDK defines this legacy macro to nothing. Keep that expansion
    # active while compiling the actual controller on every test host.
    source = source.replace("/* PRODUCTION_NATIVE_SEQUENCE */",
                            "#define near\n" + native.read_text() + "\n#undef near\n" if native.exists() else "")
    player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    camera = (ROOT / "soh/mods/items/helpers/camera_helper.c").read_text()
    # These C functions use `this` as an ordinary identifier; compile their
    # unchanged logic in this C++ fixture with only that identifier renamed.
    source = source.replace("/* PRODUCTION_ITEM_INIT */", re.sub(r"\bthis\b", "self", function(player, "Player_InitItemAction")))
    source = source.replace("/* PRODUCTION_BUBBLE_CLEANUP */", re.sub(r"\bthis\b", "self", function(player, "Player_DekuBubbleCleanup")))
    source = source.replace("/* PRODUCTION_CAMERA_EXIT */", "\n".join(function(camera, name) for name in ("FirstPerson_Exit", "ItemCamera_Exit")))
    source = source.replace("/* PRODUCTION_CONTROLLERS */", "\n".join(
        function(form, name) for name in ("MmForm_UpdateTransforming", "MmForm_UpdateDetransforming")))
    source = source.replace("/* PRODUCTION_INIT_ENTRY */", function(form, "MmForm_Init").split("{", 1)[1]
                            .split("MmForm_StopGoronRollSfx();", 1)[0])
    source = source.replace("/* PRODUCTION_DEATH_ENTRY */", function(form, "MmForm_OnDeath").split("{", 1)[1]
                            .split("MmForm_StopGoronRollSfx();", 1)[0])
    flags = ["-std=c++20", "-O1", "-g", "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DNON_EQUIVALENT", "-DNON_MATCHING", "-ffunction-sections", "-fdata-sections",
             "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"]
    flags += ["-I" + str(ROOT / p) for p in
              ("soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/soh",
               "libultraship/include", "combo", "combo/menu")]
    with tempfile.TemporaryDirectory(prefix="oot-native-transformation-") as temp:
        path = Path(temp)
        (path / "sequence.cpp").write_text(source)
        for combined in (False, True):
            binary = path / ("combo" if combined else "solo")
            defines = ["-DCOMBO_BUILD", "-DSOH_BUILD_DLL"] if combined else []
            subprocess.run([os.environ.get("CXX", "c++"), *flags, *defines, str(path / "sequence.cpp"),
                            "-Wl,--gc-sections", "-o", str(binary)], check=True)
            environment = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
            for case in ("mask-on", "mask-off", "audio", "skip", "instant", "failure", "toggle",
                         "missing-animation", "missing-mask", "missing-animation-off", "missing-mask-off",
                         "scene-lifecycle", "scene-address-reuse", "destroy-after-reveal", "camera-lighting",
                         "effects", "audio-off", "death-after-reveal", "drawn-item", "bubble-aim",
                         "camera-donor-on", "camera-donor-off", "camera-skip", "camera-facing"):
                forms = range(4) if case in ("mask-on", "mask-off", "audio", "audio-off") or case.startswith("camera-donor") else (3,)
                for form_id in forms:
                    ages = (0, 1) if case in ("mask-on", "mask-off") or case.startswith("camera-donor") else (1,)
                    for age in ages:
                        subprocess.run([str(binary), case], check=True,
                                       env={**environment, "NATIVE_FORM": str(form_id), "NATIVE_AGE": str(age)})
            print("PASS native transformation sequence with Windows near macro: " +
                  ("ComboShip OoT" if combined else "solo SoH"), flush=True)


if __name__ == "__main__":
    main()
