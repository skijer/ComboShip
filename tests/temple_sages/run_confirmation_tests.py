#!/usr/bin/env python3
"""Compile and exercise the production confirmation helper against OoT headers."""
from pathlib import Path
import argparse
import os
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from
INCLUDES = [ROOT / p for p in (".", "soh", "soh/include", "soh/include/PR", "soh/src", "soh/assets",
    "soh/mods", "soh/soh", "libultraship/include", "combo", "combo/menu")]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--helper-source", type=Path, help="Optional production-helper mutation for regression checks")
    args = parser.parse_args()
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
    if env.get("COMBO_NEI_FIXTURE_CPATH"):
        env["CPATH"] = os.pathsep.join((env["COMBO_NEI_FIXTURE_CPATH"], env.get("CPATH", "")))
    with tempfile.TemporaryDirectory(prefix="sages-confirm-") as temp:
        binary = Path(temp) / "confirmation"
        fixture = Path(temp) / "confirmation.cpp"
        message = (ROOT / "soh/src/code/z_message_PAL.c").read_text()
        native_state = block_from(message, message.index("u8 Message_GetState("))
        player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
        begin = player.index("        if (this->stateFlags1 & (PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_IN_CUTSCENE))")
        end = player.index("\n#ifdef COMBO_BUILD", begin)
        native_input = player[begin:end].replace("this->", "self->")
        fixture.write_text((ROOT / "tests/temple_sages/confirmation_native_test.cpp").read_text().replace(
            "/* ACTUAL MESSAGE STATE */", native_state).replace("/* ACTUAL PLAYER INPUT */", native_input).replace(
            '"entrances/TempleSagesConfirmationNative.h"',
            '"' + str((args.helper_source or ROOT / "combo/entrances/TempleSagesConfirmationNative.h").resolve()) + '"'))
        subprocess.run([env.get("CXX", "c++"), "-std=c++20", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
            "-DCONTROLLERBUTTONS_T=uint32_t", "-w", "-g", "-ffunction-sections", "-fdata-sections",
            "-Wl,--gc-sections", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
            *["-I"+str(p) for p in INCLUDES], str(fixture),
            "-o", str(binary)], check=True, env=env)
        subprocess.run([str(binary)], check=True, env=env)

if __name__ == "__main__":
    main()
