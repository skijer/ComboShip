#!/usr/bin/env python3
"""Verify rendering objects use native C interpolation or the shared GI renderer."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="lost-woods-renderer-linkage-") as temporary:
    # ResourceManager only needs these unrelated thread-pool declarations here.
    # Keep the native graphics macros and interpolation declarations unchanged.
    (Path(temporary) / "BS_thread_pool.hpp").write_text(
        "#pragma once\n#include <cstdint>\n#include <future>\nnamespace BS { class thread_pool; "
        "using priority_t = std::int8_t; namespace pr { constexpr priority_t normal = 0; } }\n"
    )
    failures = []
    sources = [
        ("soh", "soh/Enhancements/Graphics/ComboRegionalPassages.cpp"),
        ("mm", "2s2h/Enhancements/Graphics/ComboRegionalPassages.cpp"),
        ("mm", "2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp"),
        ("mm", "2s2h/Enhancements/Gameplay/ComboSoloForest.cpp"),
        ("mm", "2s2h/Enhancements/Gameplay/BlackMarket.cpp"),
        ("soh", "soh/Enhancements/Gameplay/ComboSoloForest.cpp"),
        ("soh", "soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp"),
        ("mm", "2s2h/Rando/NeiGiPresentation.cpp"),
        ("soh", "soh/Enhancements/randomizer/NeiGiPresentation.cpp"),
    ]
    renderers = {
        ("mm", "2s2h/Rando/NeiGiPresentation.cpp"),
        ("soh", "soh/Enhancements/randomizer/NeiGiPresentation.cpp"),
    }
    delegated_sources = {
        ("mm", "2s2h/Enhancements/Gameplay/ComboSoloForest.cpp"),
        ("soh", "soh/Enhancements/Graphics/ComboRegionalPassages.cpp"),
    }
    for index, (host, source) in enumerate(sources):
        obj = Path(temporary) / f"renderer-{index}.o"
        paths = (host, host + ("/2s2h" if host == "mm" else "/soh"),
                 host + "/include", host + "/include/PR", host + "/src", host + "/assets",
                 "libultraship/include", "combo", "combo/menu")
        # Shared render/browsing helpers consume the native CMake CVar prefixes.
        prefixes = re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',
                              (ROOT / "CMake" / ("soh-cvars.cmake" if host == "soh" else
                                               "2ship-cvars.cmake")).read_text())
        subprocess.run([
            os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-fmax-errors=5", "-fPIC",
            "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DFMT_HEADER_ONLY",
            *[f'-D{key}="{value}"' for key, value in prefixes],
            "-I" + temporary, "-I/usr/include/imgui", *["-I" + str(ROOT / path) for path in paths],
            # Ubuntu's ImGui keeps the UI-only ImGuiInputFlags type in its internal header.
            "-include", "imgui_internal.h",
            "-c", str(ROOT / host / source), "-o", str(obj),
        ], check=True)
        undefined = subprocess.check_output(["nm", "--undefined-only", str(obj)], text=True)
        symbols = {line.split()[-1] for line in undefined.splitlines() if line.split()}
        # These callers delegate particle submission to their host's shared
        # GI renderer. Compile both real owners and verify the C call/definition
        # boundary as well as the owners' native interpolation imports.
        delegated = (host, source) in delegated_sources
        if delegated and "NeiGi_DrawWorldParticles" not in symbols:
            failures.append(f"{host}/{source} does not request the native shared GI particle renderer")
        if (host, source) in renderers:
            defined = subprocess.check_output(["nm", "--defined-only", str(obj)], text=True)
            definitions = {line.split()[-1] for line in defined.splitlines() if line.split()}
            if "NeiGi_DrawWorldParticles" not in definitions:
                failures.append(f"{host}/{source} does not define the native shared GI particle renderer")
        for function in ["FrameInterpolation_RecordOpenChild", "FrameInterpolation_RecordCloseChild"]:
            requested = [symbol for symbol in symbols if function in symbol]
            if requested != ([] if delegated else [function]):
                failures.append(f"{host}/{source} requests a non-native interpolation ABI: {requested}")
        print(f"Compiled native interpolation boundary: {host}/{source}", flush=True)
    assert not failures, "\n".join(failures)
    print(f"PASS: native C interpolation ownership and both hosts' GI renderer delegation across {len(sources)} production objects")
