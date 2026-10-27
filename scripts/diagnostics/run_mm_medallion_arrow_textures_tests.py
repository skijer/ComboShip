"""Exercise MM's six medallion-arrow lifecycles, texture pairs and native geometry fallbacks."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def check_editor_controls():
    source = (ROOT / "mm/2s2h/BenGui/CosmeticEditor.cpp").read_text()
    header = (ROOT / "mm/2s2h/BenGui/CosmeticEditor.h").read_text()
    rows = re.findall(r'COSMETIC_OPTION\("([^\"]+)",\s*"([^\"]+)"', source)
    ids = [f"Arrows.Medallion{element}{channel}"
           for element in ("Fire", "Water", "Forest", "Shadow", "Light", "Spirit")
           for channel in ("Primary", "Secondary")]
    ids += [f"Magic.{element}{channel}" for element in
            ("MedallionFire", "MedallionWater", "MedallionForest", "Dins")
            for channel in ("Primary", "Secondary")]
    for cosmetic in ids:
        assert sum(row[0] == cosmetic for row in rows) == 1, cosmetic
    assert 'CVAR_COSMETIC(id ".Color")' in header
    assert 'CVarColorPicker(option.label, option.valuesCvar' in source
    assert 'CVarSetInteger(option.changedCvar, 1)' in source
    assert 'CVarSetColor(option.valuesCvar, color)' in source
    assert 'CVarGetColor(option.valuesCvar, option.defaultColor)' in source
    effects = source[source.index('if (ImGui::BeginTabItem("Effects"))'):]
    assert '"Elemental impact sounds", CVAR_COSMETIC("Arrows.ElementalImpactSounds")' in effects
    assert '.DefaultValue(false)' in effects.split('CosmeticEditorDrawGroup(COSMETICS_GROUP_TRAILS)')[0]
    assert '#define CVAR_PREFIX_COSMETIC "gCosmetic"' in header
    print("PASS MM menu color picker/rainbow RGBA .Color storage and all 20 medallion/Din registrations")


def main():
    check_editor_controls()
    flags = ["-std=gnu2x", "-O1", "-g", "-ffunction-sections", "-fdata-sections", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
             "-Werror=implicit-function-declaration", "-Wno-incompatible-pointer-types", "-Wno-int-conversion"]
    flags += ["-I" + str(ROOT / path) for path in
              ("mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm", "mm/2s2h",
               "libultraship/include", "libultraship/src", "combo")]
    with tempfile.TemporaryDirectory(prefix="mm-medallion-arrows-") as temp:
        binary = Path(temp) / "test"
        compiled = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *flags,
                        "mm/tests/medallion_arrow_textures_test.c",
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)], cwd=ROOT, capture_output=True, text=True)
        if compiled.returncode:
            raise RuntimeError(compiled.stdout + compiled.stderr)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
