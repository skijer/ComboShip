"""Execute the complete MM generator plus its real computed starting inventory."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions, block_from
from run_mm_weather_tests import production_function

with tempfile.TemporaryDirectory(prefix="seasons-mode-pool-") as td:
    build = Path(td)
    generator = functions((ROOT / "mm/2s2h/Rando/Logic/GeneratePools.cpp").read_text())["GeneratePools"]
    constraints = functions((ROOT / "mm/2s2h/Rando/Logic/PlacementConstraints.cpp").read_text())
    helpers = "\n".join(constraints[name] for name in
                        ("DungeonItemToDungeon", "DungeonItemPlacementOption", "StaysAtVanillaCheck",
                         "IsSongLocationItem"))
    start_source = (ROOT / "mm/2s2h/Rando/StartingItems.cpp").read_text()
    starting = block_from(start_source, start_source.index("std::vector<RandoItemId> GetComputedStartingItems("))
    (build / "pool.inc").write_text("namespace Rando {\n" + starting + "\nnamespace Logic {\n" + helpers + "\n" + generator + "\n}}\n")
    binary = build / "pool"
    result = subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-w", "-DFMT_HEADER_ONLY",
                             *flags(), "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
                             "-I" + str(build), "-include", "nlohmann/json.hpp",
                             str(ROOT / "tests/seasons/mode_pool_test.cpp"), "-o", str(binary)],
                            capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(binary)], check=True)
