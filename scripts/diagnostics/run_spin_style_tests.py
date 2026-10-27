#!/usr/bin/env python3
"""Run production spin style policy/resource/host tests without launching a game."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/spin_style"))
from run_native_tests import function as production_function


def compile_run(source, sanitize=False, extra=()):
    with tempfile.TemporaryDirectory(prefix="spin-style-") as directory:
        binary = Path(directory) / "test"
        flags = ["-std=gnu++20", "-O1", "-g", "-I" + str(ROOT)]
        if sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                      "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
        subprocess.run([os.environ.get("CXX", "c++"), *flags, *extra,
                        str(ROOT / source), "-o", str(binary)], cwd=ROOT, check=True)
        env = os.environ.copy()
        env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
        subprocess.run([str(binary)], cwd=ROOT, env=env, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for gate in ("policy", "resources", "render", "native", "combat"):
        parser.add_argument("--" + gate, action="store_true")
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    gates = [name for name in ("policy", "resources", "render", "native", "combat")
             if getattr(args, name)] or ["policy", "resources", "render", "native", "combat"]
    if "policy" in gates:
        compile_run("tests/spin_style/policy_test.cpp", args.sanitize)
    if "resources" in gates:
        subprocess.run(["python3", "-B", "tests/spin_style/resources_test.py"], cwd=ROOT, check=True)
    if "render" in gates:
        subprocess.run(["python3", "-B", "scripts/assets/prepare_spin_styles.py"], cwd=ROOT, check=True)
        compile_run("tests/spin_style/render_core_test.cpp", args.sanitize)
        flags = ["-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT",
                 "-DNON_MATCHING", "-DLOG_LEVEL_GAME_PRINTS=0", '-DCVAR_PREFIX_COSMETIC="gCosmetics"']
        for host in ("soh", "mm"):
            include_paths = [host + "/include", host + "/include/PR", host + "/assets", host + "/2s2h", host + "/soh",
                             host + "/src", host, "libultraship/include", "libultraship/src"]
            host_flags = flags + ["-I" + str(ROOT / p) for p in include_paths]
            if host == "mm":
                host_flags += ["-DHOST_MM", "-DCW_SPIN_HOST_MM"]
            with tempfile.TemporaryDirectory(prefix="spin-draw-source-") as directory:
                source = (ROOT / host / "src/overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.c").read_text()
                parts = [re.search(r'typedef enum \{.*?\} EnMThunderSubType;', source, re.S).group(0)] if host == "mm" else []
                parts += [production_function(source, n) for n in ("EnMThunder_VisualStyle", "EnMThunder_DrawChargeStyle")]
                (Path(directory) / "spin_draw_production.inc").write_text("\n".join(parts))
                graph = (ROOT / host / "src/code/graph.c").read_text()
                (Path(directory) / "spin_graph_production.inc").write_text("\n".join(
                    production_function(graph, name).replace(name, "SpinTest_" + name, 1)
                    for name in ("Graph_OpenDisps", "Graph_CloseDisps")))
                compile_run("tests/spin_style/render_test.cpp", args.sanitize, [*host_flags, "-I" + directory])
    if "native" in gates:
        compile_run("tests/spin_style/state_core_test.cpp", args.sanitize)
    if "native" in gates or "combat" in gates:
        subprocess.run(["python3", "-B", "tests/spin_style/run_native_tests.py",
                        *(["--sanitize"] if args.sanitize else [])], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
