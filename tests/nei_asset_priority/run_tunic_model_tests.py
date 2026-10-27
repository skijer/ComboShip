#!/usr/bin/env python3
"""Check the tunic override query against real native resource types and scope."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function

fixture = (ROOT / "tests/nei_asset_priority/tunic_model_available_test.cpp").read_text()
scope = (ROOT / "libultraship/include/ship/resource/ResourceManagerScope.h").read_text()
query = function((ROOT / "soh/soh/ResourceManagerHelpers.cpp").read_text(), "ResourceMgr_IsGiModelAvailableForGame")
fixture = fixture.replace("/* PRODUCTION_SCOPE */", scope[scope.index("namespace Ship {"):])
fixture = fixture.replace("/* PRODUCTION_QUERY */", query)
# System spdlog may select external fmt even with logging disabled. Keep this
# resource-only executable self-contained with either system or bundled fmt.
flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DFMT_HEADER_ONLY", "-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_OFF", "-Wall", "-Wextra", "-Werror",
         "-I" + str(ROOT / "libultraship/include")]
if "--sanitize" in sys.argv:
    flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
sources = [ROOT / "libultraship/src/ship/resource/Resource.cpp",
           ROOT / "libultraship/src/fast/resource/type/DisplayList.cpp",
           ROOT / "libultraship/src/fast/resource/type/Texture.cpp"]
with tempfile.TemporaryDirectory(prefix="tunic-model-tests-") as temporary:
    source = Path(temporary) / "test.cpp"
    source.write_text(fixture)
    for combo in (False, True):
        output = Path(temporary) / ("combo" if combo else "native")
        mode = ["-DCOMBO_BUILD"] if combo else []
        subprocess.run([os.environ.get("CXX", "c++"), *flags, *mode, str(source), *map(str, sources), "-o", str(output)], check=True)
        subprocess.run([str(output)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
