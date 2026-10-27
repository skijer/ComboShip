#!/usr/bin/env python3
"""Verify the production mixed-atlas copy using real texture ownership."""
import argparse
import hashlib
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from run_gfx_texture_address_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--atlas", type=Path, help="Exact POC4 checkpoint tl_ki_02.png")
    args = parser.parse_args()
    source = (ROOT / "mm/2s2h/Enhancements/Graphics/AutumnSceneFoliage.cpp").read_text()
    bridge = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
    with tempfile.TemporaryDirectory(prefix="mm-autumn-surface-") as temporary:
        build = Path(temporary)
        (build / "autumn_surface_texture_production.inc").write_text(
            function(source, "ExpandIndexedSurface") + "\n" + function(source, "BuildSurfaceTexture") + "\n" +
            function(bridge, "MM_LoadRgba32Texture") + "\n" + function(bridge, "MM_GetTextureVisibleCenter"))
        (build / "spdlog").mkdir()
        (build / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n")
        binary = build / "test"
        flags = shlex.split(os.environ.get("MM_FOLIAGE_TEST_CXXFLAGS", ""))
        subprocess.run([
            *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
            *flags, "-I" + str(build), "-I" + str(ROOT / "libultraship/include"),
            str(ROOT / "mm/tests/autumn_surface_texture_test.cpp"),
            str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
            str(ROOT / "libultraship/src/fast/resource/type/Texture.cpp"), "-o", str(binary),
        ], check=True)
        command = [str(binary)]
        if args.atlas:
            from PIL import Image
            with Image.open(args.atlas) as image:
                if image.size != (1024, 2048):
                    raise ValueError("The supplied image is not the POC4 stump/arch atlas")
                pixels = image.convert("RGBA").tobytes()
            if hashlib.sha256(pixels).hexdigest() != "cd21e5fec64adfa7c0dc2b3f482e3f66bd39cadb0453627a234ab0493c7899e9":
                raise ValueError("The supplied atlas pixels differ from the verified POC4 checkpoint")
            payload = build / "tl_ki_02.rgba"
            payload.write_bytes(pixels)
            command.append(str(payload))
        subprocess.run(command, check=True)


if __name__ == "__main__":
    main()
