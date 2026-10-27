"""Exercise MM's actual display-list patch bridge with real resource/GBI types.

Only resource lookup and logging are fixtures. A short, unmarked replacement
list reproduces the unchecked Goron-tunic patch under AddressSanitizer.
"""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--case", default="all")
    args = parser.parse_args()
    source = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
    start = source.index("typedef struct {\n    int index;\n    Gfx instruction;")
    end = source.index('extern "C" char* ResourceMgr_LoadVtxArrayByName', start)
    with tempfile.TemporaryDirectory(prefix="mm-gfx-patch-") as temporary:
        build = Path(temporary)
        (build / "gfx_patch_production.inc").write_text(source[start:end])
        # Resource.cpp's trace logger is outside the patch/resource contract.
        (build / "spdlog").mkdir()
        (build / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n")
        binary = build / "gfx_patch_test"
        flags = shlex.split(os.environ.get("MM_GFX_PATCH_CXXFLAGS", ""))
        subprocess.run([
            *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
            "-DF3DEX_GBI_2", *flags, "-I" + str(build), "-I" + str(ROOT / "libultraship/include"),
            str(ROOT / "mm/tests/gfx_patch_test.cpp"),
            str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
            str(ROOT / "libultraship/src/fast/resource/type/DisplayList.cpp"),
            "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary), args.case], check=True)


if __name__ == "__main__":
    main()
