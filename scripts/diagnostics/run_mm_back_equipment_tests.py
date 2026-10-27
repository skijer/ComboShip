"""Run MM's real native/adult back-equipment limb callbacks."""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def function(source: str, name: str) -> str:
    marker = name + "("
    name_at = source.index(marker)
    start = source.rfind("\n", 0, name_at) + 1
    brace = source.index("{", name_at)
    depth = 0
    for index in range(brace, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start : index + 1]
    raise AssertionError(f"unterminated function {name}")


def main() -> None:
    player_source = ROOT / "mm/src/code/z_player_lib.c"
    adult_source = (ROOT / "mm/mods/items/logic/adult_link_render.cpp").read_text()
    extended_source = (ROOT / "mm/mods/extended_equipment.c").read_text()
    adult_hand_helper = function(adult_source, "AdultLink_ApplyNeiHeldHand")
    adult_callback = function(adult_source, "AdultLink_OverrideLimb")
    extended_back_draw = function(extended_source, "ExtEquip_DrawShieldBackDL")
    cc = shlex.split(os.environ.get("CC", "cc"))
    cxx = shlex.split(os.environ.get("CXX", "c++"))
    includes = [
        "-I" + str(ROOT / "mm/assets"),
        "-I" + str(ROOT / "mm/include"),
        "-I" + str(ROOT / "mm/include/PR"),
        "-I" + str(ROOT / "mm/src"),
        "-I" + str(ROOT / "libultraship/include"),
        "-I" + str(ROOT / "ZAPDTR/ZAPD/resource/type"),
        "-I" + str(ROOT / "mm"),
        "-I" + str(ROOT / "mm/2s2h"),
        "-I" + str(ROOT / "mm/mods"),
        "-I" + str(ROOT / "combo"),
        "-I" + str(ROOT / "combo/menu"),
    ]
    strong_symbols = {
        "BossRemains_IsGohtWorn",
        "BossRemains_IsOdolwaWorn",
        "CVarGetInteger",
        "CustomForms_ActiveForm",
        "DinFireShield_HandDL",
        "DinFireSword_HandDL",
        "DinFireSword_SetHandModel",
        "DinFireSword_CopyHandModel",
        "DinFireSword_DrawHand",
        "ExtEquip_GetShieldDLOverride",
        "ExtEquip_IsDekuSkinActive",
        "ExtEquip_IsOotMirrorSkinActive",
        "ExtEquip_ShouldHideSwordDL",
        "FourSword_HeldSwordDL",
        "FourSword_IsEquipped",
        "ItemEquip_HoldsEmptyHand",
        "KiteSurf_AdjustLimb",
        "Nei_HeldItemUsesOotHookshotModel",
        "ResourceMgr_FileExists",
        "Trident_GoldenArmor",
        "WeaponUpgrade_ApplyHeldSwordDL",
        "WeaponUpgrade_HasGreatFairy",
        "WeaponUpgrade_HasHammerAxe",
        "gEquipMasks",
        "gEquipShifts",
        "gSaveContext",
    }

    with tempfile.TemporaryDirectory(prefix="mm-back-equipment-") as temporary:
        build = Path(temporary)
        (build / "back_equipment_callbacks_production.inc").write_text(
            adult_hand_helper + "\n\n" + adult_callback + "\n\n" + extended_back_draw + "\n"
        )
        native_object = build / "z_player_lib.o"
        subprocess.run(
            [
                *cc,
                "-std=gnu2x",
                "-O0",
                "-ffunction-sections",
                "-fdata-sections",
                "-w",
                "-DF3DEX_GBI_2",
                *includes,
                "-c",
                str(player_source),
                "-o",
                str(native_object),
            ],
            check=True,
        )

        undefined = subprocess.run(
            ["nm", "-u", str(native_object)], check=True, capture_output=True, text=True
        ).stdout.splitlines()
        weak_names = []
        for line in undefined:
            name = line.split()[-1]
            if name not in strong_symbols and not name.startswith("_") and name not in { "sqrtf", "strcmp" }:
                weak_names.append(name)
        weak_source = ["#include <stdint.h>"]
        weak_source.extend(
            f"__attribute__((weak)) uintptr_t {name}(void) {{ return 0; }}" for name in weak_names
        )
        weak_file = build / "weak_stubs.c"
        weak_file.write_text("\n".join(weak_source) + "\n")
        weak_object = build / "weak_stubs.o"
        subprocess.run([*cc, "-std=gnu2x", "-w", "-c", str(weak_file), "-o", str(weak_object)], check=True)

        test_object = build / "back_equipment_visibility_test.o"
        subprocess.run(
            [
                *cxx,
                "-std=c++20",
                "-O0",
                "-ffunction-sections",
                "-fdata-sections",
                "-w",
                "-DF3DEX_GBI_2",
                *includes,
                "-I" + str(build),
                "-c",
                str(ROOT / "mm/tests/back_equipment_visibility_test.cpp"),
                "-o",
                str(test_object),
            ],
            check=True,
        )
        binary = build / "back_equipment_visibility_test"
        subprocess.run(
            [
                *cxx,
                "-Wl,--gc-sections",
                str(test_object),
                str(native_object),
                str(weak_object),
                "-lm",
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
