"""Execute both hosts' Grace drawer with native matrix functions."""
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

for host, flags in (("soh", oot_flags()), ("mm", mm_flags())):
    parsed = functions((ROOT / host / "mods/items/logic/item_hylias_grace.c").read_text())
    matrices = functions((ROOT / host / "src/code/sys_matrix.c").read_text())
    skin = functions((ROOT / host / "src/code/z_skin_matrix.c").read_text())
    with tempfile.TemporaryDirectory(prefix=f"{host}-grace-orientation-") as temporary:
        build = Path(temporary)
        names = ["HGrace_OverrideLimbDraw", "HGrace_DrawFairy"]
        (build / "grace_draw.inc").write_text("\n".join(parsed[n] for n in names if n in parsed))
        (build / "grace_matrix.inc").write_text("\n".join(
            [skin[n] for n in ("SkinMatrix_SetTranslate", "SkinMatrix_SetScale")] +
            [matrices[n] for n in ("Matrix_Translate", "Matrix_Scale",
                                   "Matrix_RotateYF" if host == "mm" else "Matrix_RotateY",
                                   "Matrix_MultVec3f")]))
        binary = build / "orientation"
        result = subprocess.run([
            os.environ.get("CXX", "c++"), "-std=c++20", "-w", *flags,
            *(["-DGRACE_DRAW_MM"] if host == "mm" else []),
            *shlex.split(os.environ.get("GRACE_TEST_CXXFLAGS", "")),
            "-I" + str(build), str(ROOT / "tests/mm_grace/orientation_test.cpp"),
            "-o", str(binary)], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(binary)], check=True)
