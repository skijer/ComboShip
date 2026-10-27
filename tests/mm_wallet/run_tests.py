#!/usr/bin/env python3
"""Run the production MM rupee draw paths against real headers under sanitizers."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function


def main():
    native = (ROOT / "mm/src/code/z_parameter.c").read_text()
    tycoon = (ROOT / "mm/2s2h/Rando/MiscBehavior/TycoonWallet.cpp").read_text()
    start = native.index("if (GameInteractor_Should(VB_DRAW_RUPEE_COUNTER, true))")
    opening = native.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (native[end] == "{") - (native[end] == "}")
        end += 1
    declarations = "\n".join(re.findall(r"s16 sRupeeDigits(?:First|Count)\[\].*?;", native))
    production = declarations + "\nvoid TestNativeDraw(PlayState* play) {\n"
    production += "InterfaceContext* interfaceCtx = &play->interfaceCtx;\n"
    production += "s16 counterDigits[5], sp2CC, sp2C8, sp2CE, sp2CA, magicAlpha;\n"
    production += "OPEN_DISPS(play->state.gfxCtx);\n" + native[start:end]
    production += "\nCLOSE_DISPS(play->state.gfxCtx);\n}\n"
    production += function(tycoon, "DrawTycoonRupeeCounter")
    flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DNON_EQUIVALENT", "-DNON_MATCHING"]
    flags += ["-I" + str(ROOT / p) for p in
              ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h", "mm/assets",
               "libultraship/include", "combo")]
    with tempfile.TemporaryDirectory(prefix="mm-wallet-") as temporary:
        build = Path(temporary)
        (build / "wallet_draw_production.inc").write_text(production)
        cc = shlex.split(os.environ.get("CC", "cc"))
        binary = build / "wallet_draw_test"
        subprocess.run([*cc, "-std=gnu11", "-O1", "-g", "-fsanitize=address,undefined,bounds",
                        "-fno-sanitize-recover=all", *flags, "-I" + str(build),
                        "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-unused-variable",
                        "-Wno-discarded-qualifiers",
                        "-Werror=implicit-function-declaration", str(ROOT / "tests/mm_wallet/rupee_draw_test.c"),
                        "-o", str(binary)], check=True)
        # This fixture allocates no heap memory; ptrace-backed runners cannot run LSan.
        environment = {**os.environ, "ASAN_OPTIONS": os.environ.get("ASAN_OPTIONS", "") + ":detect_leaks=0"}
        subprocess.run([str(binary), *sys.argv[1:]], check=True, env=environment)
        subprocess.run([*cc, "-std=gnu11", *flags, "-Werror=implicit-function-declaration",
                        "-Wno-int-conversion", "-Wno-incompatible-pointer-types", "-fsyntax-only",
                        str(ROOT / "mm/src/code/z_parameter.c")], check=True)
        print("PASS real-header syntax: mm/src/code/z_parameter.c")
        cxx = shlex.split(os.environ.get("CXX", "c++"))
        subprocess.run([*cxx, "-std=c++20", "-fpermissive", *flags,
                        "-I" + str(ROOT / "libultraship/src"), "-I" + str(ROOT / "combo/menu"),
                        "-include", "nlohmann/json.hpp", "-include", str(ROOT / "mm/include/global.h"),
                        "-fsyntax-only", str(ROOT / "mm/2s2h/Rando/MiscBehavior/TycoonWallet.cpp")], check=True)
        print("PASS real-header syntax: mm/2s2h/Rando/MiscBehavior/TycoonWallet.cpp")


if __name__ == "__main__":
    main()
