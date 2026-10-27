#!/usr/bin/env python3
"""Exercise MM's production fast-transform policy and Easy Mask Equip caller.

The helper, event flags, pending state and caller bodies are copied unchanged
into a fixture using native MM types. CVars and engine sound/item services are
the boundary. Bounds/undefined-behavior sanitizers run for both entry points.
"""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from run_mask_transformation_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    fast = (ROOT / "mm/2s2h/Enhancements/Masks/FastTransformation.cpp").read_text()
    easy = (ROOT / "mm/2s2h/Enhancements/Masks/EasyMaskEquip.cpp").read_text()
    table = re.search(r"^static u16 sFormCheckFlags\[\] = \{.*?^\};", fast, re.M | re.S)
    if table is None:
        raise RuntimeError("Missing production first-transformation flags")
    defines = re.findall(r"^#define CVAR(?:_NAME)? .*", fast, re.M)
    policy = "\n".join(defines) + "\n" + table[0] + "\n" + function(fast, "ShouldFastTransform")
    state = easy[easy.index("typedef enum PendingMode"):easy.index("static bool IsMaskItem(")]
    helpers = [
        "IsRegularMask", "ShouldMaintainMaskWithoutButton", "GetMaskItem",
        "SetVisualMaskOverride", "ClearVisualMaskOverride", "IsPendingPlayerActionMode",
        "ClearPendingActionOnly", "ResetPendingAction", "SavePendingSnapshot",
        "PlayAppliedMaskSfx", "ApplyRegularMaskTarget", "PlayerActionReachedTarget",
        "PlayerCanProcessFormChangingMaskAction", "QueueFormChangingMaskAction",
        "PlayerCanStartFollowUpAction", "ShouldAllowGiantMask", "ProcessPendingMaskEquip",
    ]
    flags = ["-std=c++20", "-O1", "-g", "-fsanitize=undefined,bounds",
             "-fno-sanitize-recover=all", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNDEBUG", "-DNON_EQUIVALENT", "-DNON_MATCHING"]
    flags += ["-I" + str(ROOT / p) for p in
              ("mm", "mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm/2s2h",
               "libultraship/include", "combo", "combo/menu")]
    with tempfile.TemporaryDirectory(prefix="mm-fast-transformation-") as temporary:
        build = Path(temporary)
        (build / "fast_transformation_production.inc").write_text(policy)
        (build / "easy_mask_equip_production.inc").write_text(
            state + "\n" + "\n".join(function(easy, name) for name in helpers))
        binary = build / "fast_transformation_test"
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags, "-I" + str(build),
                        str(ROOT / "tests/mm_mask_equip/fast_transformation_test.cpp"),
                        "-o", str(binary)], check=True)
        failures = []
        for entry in ("helper", "caller"):
            result = subprocess.run([str(binary), entry])
            if result.returncode:
                failures.append((entry, result.returncode))
        if failures:
            raise SystemExit("Failed production fast-transform entry points: " + str(failures))


if __name__ == "__main__":
    main()
