#!/usr/bin/env python3
"""Exercise the regional cave and native draw/audio submission boundaries."""
from pathlib import Path
import argparse
import os
import re
import sys
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def validate_resources():
    # The actual host material consumes RGBA16 32x32 data. Catch a misspelled
    # resource or a format/size mismatch before it reaches the native load macro.
    for path, name in (
        ("soh/assets/yml/ntsc_1-0/objects/gameplay_keep.yml", "gRockFragment1Tex"),
        ("soh/assets/yml/ntsc_1-0/objects/object_spot08_obj.yml", "gZorasFountainIceRampMiddleSlopeTex"),
    ):
        data = (ROOT / path).read_text()
        record = re.search(r"^" + name + r":\n(.*?)(?=^\S|\Z)", data, re.S | re.M).group(1)
        assert re.search(r"format: RGBA16\n\s+width: 32\n\s+height: 32", record)
    for path, name in (
        ("mm/assets/xml/N64_US/objects/gameplay_keep.xml", "gBoulderFragmentsTex"),
        ("mm/assets/xml/N64_US/objects/object_icefloe.xml", "gIcefloePlatformTex"),
    ):
        record = ET.parse(ROOT / path).find(f".//Texture[@Name='{name}']")
        assert record is not None and record.attrib["Format"] == "rgba16"
        assert record.attrib["Width"] == "32" and record.attrib["Height"] == "32"
    table = (ROOT / "mm/include/tables/sfx/environmentbank_table.h").read_text()
    assert re.search(r"/\* 0x28F2 \*/\s*DEFINE_SFX\(NA_SE_EV_WAVE,", table)


def native_includes(host):
    return [".", host, host + "/include", host + "/include/PR", host + "/src", host + "/assets", host + "/mods",
            host + ("/soh" if host == "soh" else "/2s2h"), "libultraship/include", "combo", "combo/menu"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--core-only", action="store_true")
    parser.add_argument("--syntax", action="store_true", help="also compile both complete native graphics translation units")
    args = parser.parse_args()
    env = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
    if os.environ.get("COMBO_NEI_FIXTURE_CPATH"):
        env["CPATH"] = os.pathsep.join((os.environ["COMBO_NEI_FIXTURE_CPATH"], env.get("CPATH", "")))
    validate_resources()
    with tempfile.TemporaryDirectory(prefix="regional-presentation-") as temporary:
        for name in ("presentation_test", "shimmer_coverage_test"):
            binary = Path(temporary) / name
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                            "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                            "-I" + str(ROOT / "combo"), str(ROOT / "tests/regional_passages" / (name + ".cpp")),
                            "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env=env, timeout=20)
        if args.core_only:
            return
        for host in ("soh", "mm"):
            path = ROOT / host / ("soh" if host == "soh" else "2s2h") / "Enhancements/Graphics/ComboRegionalPassages.cpp"
            source = path.read_text() if path.is_file() else ""
            names = ("bool RegionalPresentationSafe(", "void DrawRegionalPassage(")
            if host == "mm":
                names = ("bool RegionalPresentationSafe(", "void DrawRegionalShell(", "void DrawRegionalPassage(")
            if host == "soh":
                names += ("void StopRegionalWaves(", "void UpdateRegionalWaves(")
            bodies = "\n".join(block_from(source, source.index(name)) for name in names) if source else """
void DrawRegionalPassage() {}
void StopRegionalWaves() {}
void UpdateRegionalWaves() {}
"""
            fixture = (ROOT / "tests/regional_passages/native_presentation_test.cpp").read_text()
            graph = (ROOT / host / "src/code/graph.c").read_text()
            native_graph = "\n".join(block_from(graph, graph.index(name)) for name in (
                "void Graph_OpenDisps(", "void Graph_CloseDisps("))
            rcp = (ROOT / host / "src/code/z_rcp.c").read_text()
            setup = "Gfx_SetupDL_25Opa" if host == "soh" else "Gfx_SetupDL25_Opa"
            native_setup = block_from(rcp, rcp.index("void " + setup + "(")).replace(setup, "NativeSetupOpa", 1)
            renderer = (ROOT / "soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc").read_text()
            first = renderer.index("struct NeiGi_ArenaBudget {")
            native_arena = renderer[first:renderer.index("// Child meshes", first)]
            native_arena += block_from(renderer, renderer.index("bool NeiGi_CanDrawLayers(")).replace(
                "NeiGi_CanDrawLayers", "NativeCanDrawLayers", 1)
            fixture = fixture.replace("/* NATIVE GRAPH */", native_graph).replace("/* NATIVE SETUP */", native_setup)
            fixture = fixture.replace("/* NATIVE ARENA */", native_arena)
            cpp = Path(temporary) / (host + ".cpp")
            cpp.write_text(fixture.replace("/* PRODUCTION */", bodies))
            binary = Path(temporary) / host
            includes = native_includes(host)
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                            *(["-DOOT_REGIONAL_TEST"] if host == "soh" else ["-w"]),
                            "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                            *["-I" + str(ROOT / item) for item in includes], str(cpp), "-o", str(binary)], check=True, env=env)
            subprocess.run([str(binary)], check=True, env=env, timeout=20)
            if host == "soh":
                engine = Path(temporary) / "engine.cpp"
                engine.write_text('''#include "mods/sound_translator/mm_audio_sfx.h"
#undef SFX_FLAG
#undef SFX_BANK_SHIFT
#undef SFX_BANK_MASK
#undef SFX_INDEX
#undef SFX_BANK
#include "mods/sound_translator/mm_audio_sfx.cpp"
''')
                bank = Path(temporary) / "wave-bank"
                subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                                "-DOOT_REGIONAL_TEST", "-DREGIONAL_WAVE_BANK_TEST", "-DLOG_LEVEL_GAME_PRINTS=0", "-w",
                                "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                                "-I" + str(ROOT / "tests/nei_leaf/stubs"),
                                *["-I" + str(ROOT / item) for item in includes], str(cpp), str(engine),
                                str(ROOT / "soh/mods/sound_translator/mm_audio_sfx_dispatch.cpp"),
                                str(ROOT / "soh/mods/sound_translator/mm_audio_sfx_params.cpp"),
                                "-o", str(bank)], check=True, env=env)
                subprocess.run([str(bank)], check=True, env=env, timeout=20)
            if args.syntax:
                subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                                "-DLOG_LEVEL_GAME_PRINTS=0", "-DCONTROLLERBUTTONS_T=uint32_t", "-w",
                                *["-I" + str(ROOT / item) for item in includes], "-fsyntax-only", str(path)],
                               check=True, env=env)
                print("PASS complete native graphics translation unit:", host, flush=True)


if __name__ == "__main__":
    main()
