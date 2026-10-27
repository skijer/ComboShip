#!/usr/bin/env python3
"""Reproduce eye/mouth OTR strings reaching production segmented DL handlers.

Runs the real address/signature/dispatch code without a GPU. Only resource lookup
and the execution-stack endpoint are substituted; resources use their real types.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^(?:static )?(?:void\*|F3DGfx\*|bool|int32_t)\s+" + re.escape(name) +
                      r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError(f"Missing production function: {name}")
    index, depth = match.end(), 1
    while depth:
        depth += (source[index] == "{") - (source[index] == "}")
        index += 1
    return source[match.start():index] + "\n"


def main():
    source = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    manager = (ROOT / "libultraship/src/ship/resource/ResourceManager.cpp").read_text()
    names = ["Interpreter::SegAddr", "gfx_check_image_signature", "ComboIsUnresolvedSegmentTarget"]
    # Optional only to allow this fixture to reproduce the pre-fix unsafe call.
    if "ComboResolveDisplayListTarget(" in source:
        names.append("ComboResolveDisplayListTarget")
    names += ["gfx_dl_handler_common", "gfx_dl_index_handler"]
    with tempfile.TemporaryDirectory(prefix="gfx-display-list-address-") as temporary:
        build = Path(temporary)
        (build / "dl_signature.inc").write_text(function(manager, "ResourceManager::OtrSignatureCheck"))
        (build / "dl_address_production.inc").write_text("\n".join(function(source, name) for name in names))
        (build / "spdlog").mkdir()
        (build / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n")
        binary = build / "gfx_display_list_address_test"
        subprocess.run([
            *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
            "-Wall", "-Wextra", "-Werror", "-Wno-parentheses",
            *shlex.split(os.environ.get("GFX_DL_TEST_CXXFLAGS", "")),
            "-I" + str(build), "-I" + str(ROOT / "libultraship/include"),
            str(ROOT / "libultraship/tests/gfx_display_list_address_test.cpp"),
            str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
            str(ROOT / "libultraship/src/fast/resource/type/DisplayList.cpp"),
            str(ROOT / "libultraship/src/fast/resource/type/Texture.cpp"),
            "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
