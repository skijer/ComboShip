"""Run the complete native MM pool generator for all Grace seed policies."""
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions

source = (ROOT / "mm/2s2h/Rando/Logic/GeneratePools.cpp").read_text()
constraints = functions((ROOT / "mm/2s2h/Rando/Logic/PlacementConstraints.cpp").read_text())
with tempfile.TemporaryDirectory(prefix="mm-grace-pool-") as temporary:
    build = Path(temporary)
    helpers = "\n".join(constraints[name] for name in
                        ("DungeonItemToDungeon", "DungeonItemPlacementOption", "StaysAtVanillaCheck",
                         "IsSongLocationItem"))
    (build / "pool.inc").write_text("namespace Rando::Logic {\n" + helpers + "\n" +
                                  functions(source)["GeneratePools"] + "\n}")
    binary = build / "pool"
    # Ubuntu's spdlog uses external fmt. This standalone harness does not link
    # the game's library targets, so compile fmt inline as the save harness does.
    result = subprocess.run([
        os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DFMT_HEADER_ONLY", *flags(), "-DCOMBO_BUILD",
        "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t", "-I" + str(build), "-include", "nlohmann/json.hpp",
        str(ROOT / "tests/mm_grace/pool_test.cpp"), "-o", str(binary)
    ], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(binary)], check=True)
