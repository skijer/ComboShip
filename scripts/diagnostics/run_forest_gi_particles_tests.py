#!/usr/bin/env python3
"""Run actual Saria/portal draw bodies at the native GI mesh submission boundary."""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--baseline-ref", help="Use production bodies at this ref; current particle expectations must fail on rings")
parser.add_argument("--policy-only", action="store_true")
args = parser.parse_args()

def source(path):
    return (subprocess.check_output(["git", "show", args.baseline_ref + ":" + path], cwd=ROOT, text=True)
            if args.baseline_ref else (ROOT / path).read_text())

cases = [
    ("soh", "saria", "soh/soh/Enhancements/Gameplay/ComboForestActors.cpp",
     ['extern "C" void ComboForestActors_DrawSariaParticles('], "OOT_SARIA_TEST"),
    ("mm", "saria", "mm/2s2h/Enhancements/Gameplay/ComboSaria.cpp", ["static void DrawShimmer("], "MM_SARIA_TEST"),
    ("mm", "passage", "mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp",
     ["static void DrawPortalFinish(", "static void DrawWoodsMysteryHexes("], "MM_PASSAGE_TEST"),
    ("mm", "solo", "mm/2s2h/Enhancements/Gameplay/ComboSoloForest.cpp", ["void DrawForestPortal("], ""),
    ("soh", "solo", "soh/soh/Enhancements/Gameplay/ComboSoloForest.cpp", ["void DrawForestPortal("], ""),
]
with tempfile.TemporaryDirectory(prefix="forest-gi-particles-") as temporary:
    if not args.baseline_ref:
        binary = Path(temporary) / "policy"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        "-I" + str(ROOT / "combo"), str(ROOT / "tests/forest_gi_particles/policy_test.cpp"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
    if args.policy_only:
        raise SystemExit(0)
    viewer = source("soh/src/overlays/actors/ovl_En_Viewer/z_en_viewer.c")
    fixture = (ROOT / "tests/forest_gi_particles/oot_viewer_test.c").read_text().replace(
        "/* PRODUCTION */", block_from(viewer, viewer.index("void EnViewer_DrawStaticSaria(")))
    cfile, binary = Path(temporary) / "viewer.c", Path(temporary) / "viewer"
    cfile.write_text(fixture)
    includes = ["soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets", "soh/mods",
                "soh/soh", "libultraship/include", "combo"]
    subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                    "-DCONTROLLERBUTTONS_T=uint32_t", "-g", "-fsanitize=address,undefined",
                    "-fno-sanitize-recover=all", "-no-pie", *["-I" + str(ROOT / p) for p in includes],
                    str(cfile), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
    for host, name, path, functions, mode in cases:
        production = source(path)
        body = "\n".join(block_from(production, production.index(function)) for function in functions)
        fixture = (ROOT / "tests/forest_gi_particles/draw_test.cpp").read_text().replace("/* PRODUCTION */", body)
        cpp = Path(temporary) / (host + name + ".cpp")
        binary = Path(temporary) / (host + name)
        cpp.write_text(fixture)
        includes = [".", host, host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                    host + "/mods", host + ("/2s2h" if host == "mm" else "/soh"),
                    "libultraship/include", "combo", "combo/menu"]
        command = [os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                   "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                   *(["-D" + mode] if mode else []), *["-I" + str(ROOT / path) for path in includes],
                   str(cpp), "-o", str(binary)]
        subprocess.run(command, cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})

if not args.baseline_ref:
    subprocess.run([sys.executable, "-B", str(ROOT / "scripts/diagnostics/run_mask_shimmer_tests.py"),
                    "--sanitize", "--forest-particles"], check=True)
