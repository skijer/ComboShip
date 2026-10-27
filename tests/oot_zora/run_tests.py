#!/usr/bin/env python3
"""Exercise the OoT Zora port's production action/collision bodies on engine types.

Animation, matrix, audio and effect services are controlled boundaries. This is
CPU regression evidence; visible arcs, collisions and feel still need a game run.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_item_stow"))
from run_ballchain_tests import flags

def function(source, name):
    match = re.search(r"^[^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    assert match, "Missing production function: " + name
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]

source = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
names = ["MmForm_SetAction", "MmForm_CheckDolphinJump", "MmForm_Action_DolphinJump",
         "MmForm_Action_JumpKick", "MmForm_Action_Punch"]
for name in ["MmForm_ZoraMeleeLimb", "MmForm_UpdateZoraMelee", "MmForm_DrawZoraMelee",
             "MmForm_ClearZoraSwimTrails", "MmForm_UpdateZoraSwimTrails", "MmForm_DrawZoraSwimTrail",
             "MmForm_ActionTicksAnimation", "MmForm_TickActionAnimation"]:
    if re.search(r"^static [^\n]+\b" + name + r"\(", source, re.M):
        names.append(name)
enum = re.search(r"typedef enum GoronActionId \{.*?\} GoronActionId;", source, re.S).group(0)
bodies = function((ROOT / "soh/mods/transformation_masks/mm_form_combat.c").read_text(),
                  "MmForm_DisableJumpKickQuads") + "\n" + "\n".join(function(source, name) for name in names)
if "MmForm_TickActionAnimation" not in names:
    footer = re.search(r"// Always tick animation.*?\n    \{(.*?)\n    \}\n\}", source, re.S).group(1)
    bodies += "\nstatic void MmForm_TickActionAnimation(PlayState* play, s32 dispatchedAction) {" + footer + "\n}\n"
native = function((ROOT / "soh/src/code/z_player_lib.c").read_text(), "func_80090480")
with tempfile.TemporaryDirectory(prefix="oot-zora-") as temp:
    directory = Path(temp)
    (directory / "zora_actions.inc").write_text(enum + "\n" + bodies)
    (directory / "native_melee.inc").write_text(native)
    binary = str(directory / "zora")
    compiler_flags = [*flags(), "-I" + temp, "-g", "-O1", "-fno-pie", "-no-pie",
                      "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                      "-Werror=implicit-function-declaration"]
    if "MmForm_UpdateZoraMelee" in names:
        compiler_flags.append("-DZORA_COLLISION_FIX")
    subprocess.run([os.environ.get("CC", "cc"), *compiler_flags,
                    str(ROOT / "tests/oot_zora/actions_test.c"), "-lm", "-o", binary], check=True)
    cases = sys.argv[1:] or ["timing", "dolphin", "jump"]
    if not sys.argv[1:] and "MmForm_UpdateZoraMelee" in names:
        cases += ["melee", "trails", "ticks", "landing-trail", "trail-allocation", "punch-order", "dolphin-landing-tick"]
    for case in cases:
        subprocess.run([binary, case], check=True, timeout=10,
                       env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
