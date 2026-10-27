#!/usr/bin/env python3
"""Run both games' segment binders through the production Alt/cache decisions.

Only archive I/O, deserialization, and thread scheduling are fixtures. The cache
decision functions, per-game resource helpers, and gSPSegment bodies are real.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from run_child_ruto_face_test import function

ROOT = Path(__file__).resolve().parents[2]


def helper_function(source, name):
    declaration = re.search(r'^(?:extern "C" )?(?:static )?(?:bool|uint8_t|char\*|void|std::shared_ptr<Ship::IResource>)\s+'
                            + re.escape(name) + r'\(', source, re.M)
    if declaration is None:
        raise RuntimeError("Missing production helper: " + name)
    return function(source[declaration.start():], name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--benchmark", action="store_true", help="Build optimized fixture; set RESOURCE_BENCHMARK=1 to time it")
    parser.add_argument("--expect-allocation-regression", action="store_true", help="Keep the zero-allocation gate on a baseline revision")
    parser.add_argument("--baseline", help="Git revision to use for ResourceManager bodies")
    parser.add_argument("--game", choices=("oot", "mm", "all"), default="all")
    args = parser.parse_args()
    manager = (ROOT / "libultraship/src/ship/resource/ResourceManager.cpp").read_text()
    if args.baseline:
        manager = subprocess.check_output(["git", "show", args.baseline + ":libultraship/src/ship/resource/ResourceManager.cpp"], cwd=ROOT, text=True)
    load_start = manager.index("std::shared_ptr<IResource> ResourceManager::LoadResourceProcess(const ResourceIdentifier&")
    load_end = manager.index("std::shared_ptr<IResource> ResourceManager::LoadResource(uint64_t", load_start)
    cache_start = manager.index("std::variant<ResourceManager::ResourceLoadError, std::shared_ptr<IResource>>\n"
                                "ResourceManager::CheckCache")
    cache_end = manager.index("std::shared_ptr<std::vector<std::shared_ptr<IResource>>>", cache_start)
    cache = manager[load_start:load_end] + manager[cache_start:cache_end]
    failed = False
    with tempfile.TemporaryDirectory(prefix="alt-segment-binding-") as temporary:
        build = args.build_dir or Path(temporary)
        build.mkdir(parents=True, exist_ok=True)
        (build / "alt_segment_cache.inc").write_text(cache)
        (build / "spdlog").mkdir(exist_ok=True)
        (build / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_TRACE(...) ((void)0)\n")
        for game in ("oot", "mm"):
            if args.game not in (game, "all"):
                continue
            helper_path = "soh/soh/ResourceManagerHelpers.cpp" if game == "oot" else "mm/2s2h/BenPort.cpp"
            wrapper_path = "soh/soh/GbiWrap.cpp" if game == "oot" else "mm/src/code/stubs.c"
            helper = (ROOT / helper_path).read_text()
            names = ["ResourceMgr_IsAltAssetsEnabled"]
            if game == "oot":
                names += ["ResourceMgr_FileAltExists", "ResourceMgr_GetResourceByNameHandlingMQ"]
            else:
                names += ["GetResourceByName"]
            if "static void ResourceMgr_PreloadAltWhenItExists(" in helper:
                names += ["ResourceMgr_PreloadAltWhenItExists"]
            names += ["ResourceMgr_LoadIfDListByName"]
            production = "\n".join(helper_function(helper, name) for name in names)
            production += helper_function((ROOT / wrapper_path).read_text(), "gSPSegment")
            (build / "alt_segment_helpers.inc").write_text(production)
            binary = build / (game + "_alt_segment_test")
            subprocess.run([
                *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                "-Wno-unused-parameter", "-Wno-unused-function", "-Wno-pointer-arith", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                *(["-O3", "-DNDEBUG"] if args.benchmark else ["-fsanitize=undefined", "-fno-sanitize-recover=all"]), "-I" + str(build),
                "-I" + str(ROOT / "libultraship/include"),
                *shlex.split(os.environ.get("PERFORMANCE_TRACE_CXXFLAGS", "")),
                "-include", str(ROOT / "libultraship/include/ship/diagnostics/PerformanceTrace.h"),
                str(ROOT / "libultraship/src/ship/diagnostics/PerformanceTrace.cpp"), "-pthread",
                str(ROOT / "soh/tests/alt_segment_binding_test.cpp"),
                str(ROOT / "libultraship/src/ship/resource/Resource.cpp"),
                str(ROOT / "libultraship/src/fast/resource/type/DisplayList.cpp"),
                str(ROOT / "libultraship/src/fast/resource/type/Texture.cpp"), "-o", str(binary),
            ], check=True)
            result = subprocess.run([str(binary), game], env={**os.environ,
                **({"RESOURCE_BASELINE": "1"} if args.baseline and not args.expect_allocation_regression else {})})
            failed |= result.returncode != 0
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
