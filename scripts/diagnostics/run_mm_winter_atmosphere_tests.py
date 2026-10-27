"""Exercise production MM winter state, controller, terrain queries, and GBI output."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
from run_mm_weather_tests import production_function

ROOT = Path(__file__).resolve().parents[2]


def run(command):
    result = subprocess.run(command, text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    print(result.stdout, end="", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-I" + str(ROOT / "mm")]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                  "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    graphics = ROOT / "mm/2s2h/Enhancements/Graphics"
    with tempfile.TemporaryDirectory(prefix="mm-winter-") as temporary:
        build = Path(temporary)
        state_sources = [str(graphics / "MMSummerAtmosphereState.cpp")]
        winter = graphics / "MMWinterAtmosphereState.cpp"
        if winter.exists():
            state_sources.append(str(winter))
        executable = build / "winter_state_test"
        run([*compiler, *flags, str(ROOT / "mm/tests/winter_atmosphere_test.cpp"),
             *state_sources, "-o", str(executable)])
        run([str(executable)])

        # Exact production MM setup verifies texture/combiner restoration.
        setup = (ROOT / "mm/src/code/z_rcp.c").read_text()
        block = re.search(r"\{\s*/\* SETUPDL_25 \*/(.*?)\n    \},", setup, re.S)
        if block is None:
            raise RuntimeError("Cannot find native MM SETUPDL_25")
        (build / "winter_native_setup.inc").write_text(
            "static Gfx sNativeSetup25[] = {" + block[1] + "\n};\n")
        inventory = (ROOT / "mm/mods/extended_inventory.c").read_text()
        (build / "winter_seasons.inc").write_text("\n".join(
            production_function(inventory, name) for name in
            ("Seasons_HasRod", "Seasons_SeasonOwned", "Seasons_SeasonAt", "Seasons_GetSeason")))
        play = (ROOT / "mm/src/code/z_play.c").read_text()
        update = re.search(r"Environment_Update\(this,[\s\S]*?this->state.gfxCtx\);\s*"
                           r"(MMWinterAtmosphere_Update\(this\);)", play)
        draw = re.search(r"MMSummerAtmosphere_Draw\(this\);\s*(MMWinterAtmosphere_Draw\(this\);)", play)
        init = production_function(play, "Play_Init")
        if update is None or draw is None or "MMWinterAtmosphere_Reset();" not in init:
            raise RuntimeError("Missing MM Play update/draw/re-entry lifecycle integration")
        destroy = production_function(play, "Play_Destroy")
        if "MMWeather_Reset();" not in destroy:
            raise RuntimeError("Missing MM Play destruction reset integration")
        environment = production_function((ROOT / "mm/src/code/z_kankyo.c").read_text(), "Environment_Update")
        if "MMWeather_Update(play);" not in environment:
            raise RuntimeError("Missing native Environment winter eligibility/lifecycle refresh")
        main = production_function(play, "Play_Main")
        native_update = re.search(
            r"\{\s*GraphicsContext\* gfxCtx = this->state.gfxCtx;[\s\S]*?"
            r"Play_Update\(this\);\s*this->state.gfxCtx = gfxCtx;\s*\}", main)
        native_draw = re.search(r"Play_Draw\(this\);", main)
        if (native_update is None or "this->state.gfxCtx = NULL;" not in native_update[0] or
                native_draw is None or native_draw.start() < native_update.end()):
            raise RuntimeError("Cannot find native Play_Main null-graphics update/restore/draw envelope")
        (build / "winter_play_hooks.inc").write_text(
            "static void UpdateFromPlay(PlayState* play) { MMWeather_Update(play); " +
            update[1].replace("this", "play") + " }\n" +
            "static void DrawFromPlay(PlayState* play) { " + draw[1].replace("this", "play") + " }\n" +
            "static size_t FrameFromNativePlayMain(PlayState* play) {\n" +
            native_update[0].replace("this", "play").replace("Play_Update(play)", "UpdateFromPlay(play)") +
            "\n" + native_draw[0].replace("Play_Draw(this);", "return Draw(*play);") + "\n}\n" +
            "static void InitFromPlay() { MMWinterAtmosphere_Reset(); }\n" +
            "static void DestroyFromPlay() { MMWeather_Reset(); }\n")
        game_flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
                      "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
                      "-Wno-error", "-Wno-deprecated-enum-enum-conversion", "-fpermissive",
                      "-include", "nlohmann/json.hpp"]
        game_flags += ["-I" + str(ROOT / directory) for directory in
                       ("mm/include", "mm/include/PR", "mm/src", "mm/2s2h", "mm/assets",
                        "libultraship/include", "libultraship/src", "combo")]
        game_flags += shlex.split(os.environ.get("MM_WINTER_TEST_CXXFLAGS", ""))
        executable = build / "winter_renderer_test"
        run([*compiler, *flags, *game_flags, "-I" + str(build),
             str(ROOT / "mm/tests/winter_atmosphere_render_test.cpp"),
             str(graphics / "MMWinterAtmosphere.cpp"), *state_sources,
             str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeather.cpp"),
             str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherState.cpp"),
             str(ROOT / "mm/2s2h/Rando/NeiSeasons.cpp"), "-o", str(executable)])
        run([str(executable)])


if __name__ == "__main__":
    main()
