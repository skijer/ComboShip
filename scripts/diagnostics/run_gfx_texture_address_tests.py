#!/usr/bin/env python3
"""Exercise the production SETTIMG address path without a GPU or game archives.

The fixture compiles the unchanged segment resolver, address validator, signature
check and SETTIMG handler. Only archive lookup and the final graphics-state sink
are substituted. --case segment8/segment9 reproduces the reported access violation.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'^(?:static )?(?:extern "C" )?(?:void\*|void|bool|uint8_t|int32_t|std::shared_ptr<Fast::Texture>)\s+' + re.escape(name) +
                      r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError(f"Missing production function: {name}")
    index, depth = match.end(), 1
    while depth:
        depth += (source[index] == "{") - (source[index] == "}")
        index += 1
    return source[match.start():index] + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", default="all")
    parser.add_argument("--low-module", action="store_true",
                        help="Linux non-PIE build: verify mapped static textures below 0x10000000")
    args = parser.parse_args()
    interpreter = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    header = (ROOT / "libultraship/include/fast/interpreter.h").read_text()
    manager = (ROOT / "libultraship/src/ship/resource/ResourceManager.cpp").read_text()
    with tempfile.TemporaryDirectory(prefix="gfx-texture-address-") as temporary:
        build = Path(temporary)
        metadata = re.search(r"struct RawTexMetadata \{.*?\n\};", header, re.S).group(0)
        (build / "texture_metadata.inc").write_text(metadata)
        (build / "texture_signature.inc").write_text(function(manager, "ResourceManager::OtrSignatureCheck"))
        (build / "texture_address_production.inc").write_text("\n".join(
            function(interpreter, name) for name in [
                "Interpreter::SegAddr", "IsValidResolvedAddress", "gfx_check_image_signature", "ComboLoadTextureResource",
                "ReportTextureLoadFailure", "gfx_set_timg_handler_rdp",
            ]))
        # Resource destruction's trace logger is outside the texture address contract.
        (build / "spdlog").mkdir()
        (build / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n")
        binary = build / "gfx_texture_address_test"
        flags = shlex.split(os.environ.get("GFX_TEXTURE_TEST_CXXFLAGS", ""))
        if args.low_module:
            flags += ["-fno-pie", "-no-pie", "-rdynamic", "-DTEST_LOW_MODULE"]
        subprocess.run([
            *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
            *flags, "-I" + str(build), "-I" + str(ROOT / "libultraship/include"),
            str(ROOT / "libultraship/tests/gfx_texture_address_test.cpp"),
            str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
            str(ROOT / "libultraship/src/fast/resource/type/Texture.cpp"),
            "-ldl", "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary), args.case], check=True)


if __name__ == "__main__":
    main()
