"""Run the MM fairy controller with native structs and the real stick decoder."""
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions

def body(path, name):
    source = (ROOT / path).read_text()
    if path.endswith("item_hylias_grace.c") and "--baseline" in sys.argv:
        source = subprocess.check_output(["git", "show", "af54f452:" + path], cwd=ROOT, text=True)
    return re.sub(r"\bthis\b", "self", functions(source)[name])

grace = "mm/mods/items/logic/item_hylias_grace.c"
parts = [body("mm/src/code/z_lib.c", name) for name in
         ("Lib_GetControlStickData", "Math_SmoothStepToF", "Math_SmoothStepToS")]
parts += [body("mm/src/code/z_camera.c", name) for name in ("Camera_SetStateFlag", "Camera_SetFinishedFlag")]
player_source = (ROOT / "mm/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
parts.append(player_source[player_source.index("u16 sReturnEntranceGroupData[]"):
                           player_source.index("void func_808354A4(")])
parts.append(body("mm/src/overlays/actors/ovl_player_actor/z_player.c", "func_808354A4"))
parts += [body(grace, name) for name in
          ("HGrace_Stop", "HGrace_Start")]
if "--baseline" not in sys.argv:
    parts += [body(grace, name) for name in ("HGrace_UpdateRoomChange", "HGrace_ResetTransient")]
else:
    parts.append("void HGrace_ResetTransient(void) {} // The baseline has no lifecycle reset.")
parts += [body(grace, name) for name in
          ("HGrace_CheckDoorTransition", "HGrace_StateFairy", "Handle_HyliasGrace", "HGrace_WantsNoClip")]
# Preserve the baseline's real no-op dependency to reproduce the missing port.
parts.insert(0, body("mm/mods/nei_link_stubs.cpp", "func_80077D10").replace("func_80077D10()", "func_80077D10(...)"))
parts.insert(0, body("mm/mods/nei_link_stubs.cpp", "func_8009728C").replace("func_8009728C()", "func_8009728C(...)"))
parts.insert(0, body("mm/mods/nei_link_stubs.cpp", "func_80097534").replace("func_80097534()", "func_80097534(...)"))
with tempfile.TemporaryDirectory(prefix="mm-grace-flight-") as temporary:
    build = Path(temporary)
    (build / "flight_production.inc").write_text("\n".join(parts))
    binary = build / "flight"
    result = subprocess.run([
        os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-w",
        "-ftrivial-auto-var-init=zero", *flags(), "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
        "-DCONTROLLERBUTTONS_T=uint32_t", "-I" + str(build),
        "-include", "nlohmann/json.hpp", str(ROOT / "tests/mm_grace/flight_test.cpp"),
        str(ROOT / "mm/2s2h/Rando/NeiGrace.cpp"), "-o", str(binary)
    ], capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    subprocess.run([str(binary), *[arg for arg in sys.argv[1:] if arg != "--baseline"]], check=True)
