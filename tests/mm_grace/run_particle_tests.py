"""Capture both hosts' production Grace particles at the native effect boundary."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
sys.path.insert(0, str(ROOT / "tests/nei_held"))
from run_time_pedestal_tests import functions
from run_mm_nei_tests import flags as mm_flags
from run_articulated_tests import flags as oot_flags

def particle_block(source, name, marker):
    start = source.index("    if (", source.index(marker))
    opening = source.index("{", start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return f"static void {name}(Player* p, PlayState* play) {{\n" + source[start:end] + "\n}"

for host, flags in (("soh", oot_flags()), ("mm", mm_flags())):
    source = (ROOT / host / "mods/items/logic/item_hylias_grace.c").read_text()
    parsed = functions(source)
    with tempfile.TemporaryDirectory(prefix=f"{host}-grace-particles-") as temporary:
        build = Path(temporary)
        chunks = [parsed[name] for name in (
            "HGrace_SpawnFairySparkles", "HGrace_SpawnTrailSparkles", "HGrace_SpawnWarpSparkles")]
        # Capture the production emission blocks without running animation/state transitions.
        chunks += [particle_block(parsed["HGrace_StateCasting"], "TestCastingSparkles", "sparkles during casting"),
                   particle_block(parsed["HGrace_StateWarpEnter"], "TestWarpEnterSparkles", "warp sparkles")]
        (build / "grace_particles.inc").write_text("\n".join(chunks))
        binary = build / "particles"
        host_flags = ["-DGRACE_PARTICLE_MM"] if host == "mm" else []
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", *flags, *host_flags,
                        *shlex.split(os.environ.get("GRACE_TEST_CXXFLAGS", "")),
                        "-I" + str(build), str(ROOT / "tests/mm_grace/particle_test.cpp"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
