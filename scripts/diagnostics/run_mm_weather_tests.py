"""Compile and exercise MM weather production code without game assets."""
import os
import pathlib
import re
import shlex
import subprocess
import tempfile
import sys

sys.dont_write_bytecode = True
from run_mm_audio_runtime_test import function_body

ROOT = pathlib.Path(__file__).resolve().parents[2]


def array_declaration(source, name):
    match = re.search(r"^[^\n;]*\b" + name + r"\b[^\n;]*=\s*\{.*?^\};", source, re.M | re.S)
    if match is None:
        raise RuntimeError("Cannot find production array: " + name)
    return match[0]


def production_function(source, name):
    # Decompiled sky code contains commented-out DMA blocks with unmatched
    # braces. Mask comments for balancing while retaining original offsets.
    masked = re.sub(r"//[^\n]*|/\*[\s\S]*?\*/",
                    lambda match: re.sub(r"[^\n]", " ", match[0]), source)
    start, end = function_body(masked, name)
    return source[start:end]


def main():
    with tempfile.TemporaryDirectory(prefix="mm-weather-") as temporary:
        build = pathlib.Path(temporary)
        compiler = shlex.split(os.environ.get("CXX", "c++"))
        extra = shlex.split(os.environ.get("MM_WEATHER_TEST_CXXFLAGS", ""))
        flags = [*extra, "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I" + str(ROOT / "mm")]
        executable = build / "weather_state_test"
        subprocess.run([*compiler, *flags,
                        str(ROOT / "mm/tests/weather_state_test.cpp"),
                        str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherState.cpp"),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
        executable = build / "weather_mixer_test"
        subprocess.run([*compiler, *flags,
                        str(ROOT / "mm/tests/weather_mixer_test.cpp"),
                        str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherMixer.cpp"),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
        game_flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
                      "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING"]
        game_flags += ["-I" + str(ROOT / path) for path in
                       ("mm/include", "mm/include/PR", "mm/src", "mm/2s2h", "mm/assets", "mm/mods",
                        "libultraship/include", "libultraship/src", "combo", "combo/menu")]
        for source in ("mm/src/code/z_kankyo.c", "mm/src/code/z_play.c", "mm/src/code/z_vr_box.c",
                       "mm/src/overlays/actors/ovl_En_Bom/z_en_bom.c",
                       "mm/src/overlays/actors/ovl_En_Bombf/z_en_bombf.c",
                       "mm/src/overlays/actors/ovl_En_Test6/z_en_test6.c",
                       "mm/src/overlays/actors/ovl_En_Wiz_Fire/z_en_wiz_fire.c",
                       "mm/src/overlays/actors/ovl_Door_Warp1/z_door_warp1.c",
                       "mm/src/overlays/actors/ovl_Dm_Hina/z_dm_hina.c",
                       "mm/src/overlays/actors/ovl_player_actor/z_player.c"):
            result = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), "-std=gnu17",
                                     "-fsyntax-only", "-DNDEBUG", "-Wno-int-conversion",
                                     "-Wno-incompatible-pointer-types", "-I" + str(ROOT / "mm"),
                                     *game_flags, str(ROOT / source)], capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
            print("PASS real-header weather syntax:", source, flush=True)
        executable = build / "weather_bridge_test"
        inventory = (ROOT / "mm/mods/extended_inventory.c").read_text()
        (build / "weather_seasons.inc").write_text("\n".join(
            production_function(inventory, name) for name in
            ("Seasons_HasRod", "Seasons_SeasonOwned", "Seasons_SeasonAt",
             "Seasons_GetSeason", "Seasons_SetSeason")))
        environment = (ROOT / "mm/src/code/z_kankyo.c").read_text()
        play = (ROOT / "mm/src/code/z_play.c").read_text()
        spin = (ROOT / "mm/src/overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.c").read_text()
        environment_functions = [re.search(r"^static s16 sNativeLightningAmbient\[3\];", environment, re.M)[0]]
        library = (ROOT / "mm/src/code/z_lib.c").read_text()
        for source, name in ((environment, "Environment_ClearNativeLightningAmbient"),
                             (environment, "Environment_GetNativeLightningAmbient"),
                             (environment, "Environment_RestoreNativeLightningAmbient"),
                             (environment, "Environment_GetAmbientAdjustment"),
                             (environment, "Environment_AdjustLights"),
                             (library, "Color_RGB8_Lerp"),
                             (environment, "Environment_LerpRGB8"),
                             (environment, "Environment_LerpAmbientColor"),
                             (environment, "Environment_GetStormState"),
                             (environment, "Environment_DrawRain"),
                             (spin, "EnMThunder_AdjustLights")):
            environment_functions.append(production_function(source, name))
        # Execute the actual final ambient composition without the unrelated
        # scene-light interpolation setup. Full translation-unit syntax is checked above.
        update_lights = production_function(environment, "Environment_UpdateLights")
        ambient_start = update_lights.index("        adjustment = Environment_GetAmbientAdjustment")
        ambient_end = update_lights.index("        if ((s16)(envCtx->lightSettings.light1Color", ambient_start)
        environment_functions.append(
            "void DisplayAmbientFromEnvironment(PlayState* play) {\n"
            "EnvironmentContext* envCtx = &play->envCtx; LightContext* lightCtx = &play->lightCtx;\n"
            "s32 i, adjustment; for (i = 0; i < 3; i++) {\n" + update_lights[ambient_start:ambient_end] +
            "lightCtx->ambientColor[i] = MMWeather_Shade(lightCtx->ambientColor[i]);\n}\n}\n")
        (build / "weather_environment.inc").write_text("\n".join(environment_functions))
        player = (ROOT / "mm/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
        transformation = production_function(player, "func_808388B8")
        snapshot = transformation[transformation.index("    D_80862B50 ="):transformation.index("    this->actor.velocity")]
        action87 = production_function(player, "Player_Action_87")
        restore = action87[action87.index("                play->envCtx.adjLightSettings ="):action87.index(
            "                func_8085B384", action87.index("                play->envCtx.adjLightSettings ="))]
        player_type = re.search(r"typedef struct struct_8085D848_unk_00 \{.*?\} struct_8085D848_unk_00;",
                                player, re.S)[0]
        (build / "weather_player.inc").write_text(
            player_type + "\nAdjLightSettings D_80862B50;\nstatic s16 sPlayerNativeLightningAmbient[3];\n" +
            re.search(r"^u8 D_8085D844\[\].*;$", player, re.M)[0] + "\n" +
            production_function(player, "func_80854CD0") + "\n" +
            production_function(player, "func_80854EFC") + "\n" +
            "void SnapshotPlayerAmbient(PlayState* play) {\n" + snapshot + "\n}\n" +
            "void RestorePlayerAmbient(PlayState* play) {\n" + restore + "\n}\n")
        bomb_sections = []
        for actor, suffix in (("En_Bom", "Bom"), ("En_Bombf", "Bombf")):
            bomb = (ROOT / f"mm/src/overlays/actors/ovl_{actor}/z_{actor.lower()}.c").read_text()
            replacement = re.search(
                r"(?:Environment_ClearNativeLightningAmbient\(\);\s*)?"
                r"play->envCtx.adjLightSettings.ambientColor\[0\] = "
                r"play->envCtx.adjLightSettings.ambientColor\[1\] =\s*"
                r"play->envCtx.adjLightSettings.ambientColor\[2\] = 250;", bomb)[0]
            fade_start = bomb.index("    if (play->envCtx.adjLightSettings.ambientColor[0] != 0)")
            fade_end = bomb.index("    if (this->timer == 0)", fade_start)
            bomb_sections.append(f"void Replace{suffix}Ambient(PlayState* play) {{\n{replacement}\n}}\n")
            bomb_sections.append(f"void Fade{suffix}Ambient(PlayState* play) {{\n{bomb[fade_start:fade_end]}\n}}\n")
        (build / "weather_bombs.inc").write_text("\n".join(bomb_sections))
        skybox = (ROOT / "mm/src/code/z_vr_box.c").read_text()
        skybox_draw = (ROOT / "mm/src/code/z_vr_box_draw.c").read_text()
        entry_end = environment.index("} TimeBasedSkyboxEntry;") + len("} TimeBasedSkyboxEntry;")
        entry_start = environment.rfind("typedef struct {", 0, entry_end)
        sky = [environment[entry_start:entry_end]]
        for name in ("sTimeBasedSkyboxConfigs", "sSkyboxPrimColors", "sSkyboxEnvColors"):
            sky.append(array_declaration(environment, name))
        # C permits these const resource-name arrays in TexturePtr (void*);
        # spell out the conversion only in the C++ fixture.
        sky.append(re.sub(r"\bgSkybox(?:Clear|Cloudy)\dTex\b", r"(TexturePtr)\g<0>",
                          array_declaration(skybox, "sSkyboxTextures")))
        for source, name in ((skybox, "Skybox_IsOotSkyActive"),
                             (skybox, "Skybox_GetOotCloudBlend"),
                             (environment, "Environment_LerpWeight"),
                             (skybox_draw, "Skybox_SetColors"),
                             (environment, "Environment_UpdateSkybox"),
                             (environment, "Environment_DrawSkyboxFilters"),
                             (environment, "Environment_SetupSkyboxStars")):
            sky.append(production_function(source, name))
        (build / "weather_sky.inc").write_text("\n".join(sky))
        gate = re.search(r"if \([^;{}]*precipitation\[PRECIP_RAIN_CUR\][^;{}]*\)\s*\{\s*"
                         r"Environment_DrawRain\(this, &this->view, gfxCtx\);\s*\}", play)
        if gate is None:
            raise RuntimeError("Cannot find actual Play rain draw gate")
        (build / "weather_draw_gate.inc").write_text(
            "void DrawWeatherFromPlay(PlayState* play) {\nGraphicsContext* gfxCtx = play->state.gfxCtx;\n" +
            gate[0].replace("this", "play") + "\n}\n")
        structs = environment[environment.index("typedef enum {"):environment.index("// Variables are put")]
        functions = []
        for name in ("Environment_AddLightningBolts", "MMWeather_StartBolt", "MMWeather_ClearBolts",
                     "Environment_UpdateLightningStrike", "Environment_DrawLightningImpl",
                     "Environment_DrawLightning", "Environment_Draw"):
            functions.append(production_function(environment, name))
        (build / "weather_bolts.inc").write_text(
            structs + "\nLightningBolt sLightningBolts[3];\n"
            "static LightningBolt sMMWeatherLightningBolt = { .state = LIGHTNING_BOLT_INACTIVE };\n" +
            "s16 sLightningFlashAlpha;\n" +
            re.sub(r"\bgEffLightning\dTex\b", r"(TexturePtr)\g<0>",
                   array_declaration(environment, "sLightningTextures")) + "\n" +
            "\n".join(functions))
        (build / "weather_random.inc").write_text((ROOT / "mm/src/boot/O2/rand.c").read_text())
        # The real sky-star helper includes its normal spdlog call; use the same
        # standalone fmt mode as the other production-header fixtures.
        result = subprocess.run([*compiler, *flags, *game_flags, "-Wno-error", "-DFMT_HEADER_ONLY",
                        "-include", "nlohmann/json.hpp", "-I" + str(build),
                        str(ROOT / "mm/tests/weather_bridge_test.cpp"),
                        str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeather.cpp"),
                        str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherState.cpp"),
                        str(ROOT / "mm/2s2h/Rando/NeiSeasons.cpp"),
                        "-o", str(executable)], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        case = os.environ.get("MM_WEATHER_TEST_CASE")
        subprocess.run([str(executable), *([case] if case else [])], check=True)
        executable = build / "weather_audio_test"
        port = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
        start = port.index("            MMWeatherAudio_Mix(audio_buffer,")
        end = port.index(";", port.index("AudioPlayer_Play(", start)) + 1
        (build / "weather_output.inc").write_text(
            "void SubmitWeatherAudio(int16_t* audio_buffer, unsigned int num_audio_samples) {\n" +
            "const unsigned int totalFrames = num_audio_samples; constexpr int kChannels = 2;\n" +
            port[start:end] + "\n}\n")
        result = subprocess.run([*compiler, *flags, *game_flags, "-Wno-error", "-pthread",
                                "-I" + str(build),
                                str(ROOT / "mm/tests/weather_audio_test.cpp"),
                                str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherAudio.cpp"),
                                str(ROOT / "mm/2s2h/Enhancements/Audio/MMWeatherMixer.cpp"),
                                "-o", str(executable)], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
