#!/usr/bin/env python3
"""Exercise production Goron attacks and interrupted body collision on engine types.

Native cylinder/quad intersections, hit flags, boulder and grotto reactions are
real. Rendering/audio and unrelated early-update services are external boundaries.
This is CPU regression evidence; the user's scene/model packs still need a game run.
"""
import argparse
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("cases", nargs="*")
    args = parser.parse_args()
    root = args.root
    form = (root / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    combat = (root / "soh/mods/transformation_masks/mm_form_combat.c").read_text()
    collision = (root / "soh/src/code/z_collision_check.c").read_text()
    bombiwa = (root / "soh/src/overlays/actors/ovl_Obj_Bombiwa/z_obj_bombiwa.c").read_text()
    grotto = (root / "soh/src/overlays/actors/ovl_Door_Ana/z_door_ana.c").read_text()
    collision_names = [
        "Collider_InitBase", "Collider_InitTouch", "Collider_InitBump", "Collider_InitInfo",
        "Collider_InitCylinderDim", "Collider_InitCylinder", "Collider_SetTouch", "Collider_SetBump",
        "Collider_SetInfo", "Collider_SetBase", "Collider_SetCylinderDim", "Collider_SetCylinder",
        "Collider_ResetATBase", "Collider_ResetATInfo", "Collider_ResetACBase", "Collider_ResetACInfo",
        "Collider_ResetOCBase", "Collider_ResetOCInfo", "Collider_ResetQuadACDist", "Collider_ResetQuadAT",
        "Collider_ResetCylinderAT", "Collider_ResetCylinderAC", "Collider_ResetCylinderOC",
        "Collider_QuadSetNearestAC", "Collider_SetQuadMidpoints", "Collider_SetQuadVertices",
        "Collider_UpdateCylinder", "CollisionCheck_SkipTouch", "CollisionCheck_SkipBump",
        "CollisionCheck_NoSharedFlags", "CollisionCheck_SetBounce", "CollisionCheck_SetATvsAC",
        "CollisionCheck_AC_CylVsCyl", "CollisionCheck_AC_QuadVsCyl", "CollisionCheck_GetMassType",
        "CollisionCheck_SetOCvsOC", "CollisionCheck_OC_CylVsCyl", "CollisionCheck_Incompatible", "Collider_ResetATInfo_Unk",
        "CollisionCheck_ApplyDamage",
    ]
    native_bodies = [function(collision, name) for name in collision_names]
    math = (root / "soh/src/code/z_lib.c").read_text()
    native_bodies += [function(math, name) for name in ("Math_Vec3f_Copy", "Math_Vec3s_ToVec3f", "Math_Vec3f_Diff", "Math_Vec3f_DistXYZ")]
    # Run the real body's textbox branch, including its real exit cleanup. The
    # rest of UpdateActive drives unrelated forms, assets, animations and input.
    active = function(form, "MmForm_UpdateActive")
    textbox = active[active.index("    // Freeze form during ANY textbox"):active.index("    // Crawlspace:")]
    form_bodies = []
    if re.search(r"^static void MmForm_RestoreBodyCollider\(", form, re.M):
        form_bodies.append(function(form, "MmForm_RestoreBodyCollider"))
    form_bodies += [function(form, name) for name in ("MmForm_SetAction", "MmForm_SetRollAttack", "MmForm_ClearRollAttack")]
    if re.search(r"^static void MmForm_LeaveGoronBall\(", form, re.M):
        form_bodies.append(function(form, "MmForm_LeaveGoronBall"))
    # The two real landing/recovery call sites must both remain grotto-compatible.
    pound_calls = re.findall(r"MmForm_SetRollAttack\(player, [^;\n]+, 4, 60\);", function(form, "MmForm_Action_GoronRoll"))
    assert len(pound_calls) == 2, "Expected both production ground-pound attack windows"
    form_bodies.extend("static void production_pound_%d(Player* player) { %s }" % (i, call)
                       for i, call in enumerate(pound_calls))
    form_bodies.append("static void production_textbox(Player* player, PlayState* play) {\n" + textbox + "\n}")
    # Full form exit's collider reset is an independent public lifecycle boundary.
    restore = function(form, "MmForm_RestoreOotState")
    restore = restore[restore.index("    // Restore default OOT Link collider"):restore.index("    // Reset boot data REGs")]
    form_bodies.append("static void production_restore_body(Player* player) {\n" + restore + "\n}")
    form_bodies.extend(function(combat, name) for name in ("MmForm_SetPunchQuadVertices", "MmForm_EnablePunchQuad", "MmForm_DisablePunchQuad"))
    form_bodies.append(function(form, "MmForm_Action_Punch"))
    enum = re.search(r"typedef enum GoronActionId \{.*?\} GoronActionId;", form, re.S).group(0)
    properties = re.search(r"typedef struct \{\n    const char\* skelPath;.*?\} MmFormProperties;", form, re.S).group(0)
    props = re.search(r"static const MmFormProperties sFormProps\[.*?\n\};", form, re.S).group(0)
    quad_params = re.search(r"static const f32 sGoronPunchQuadParams\[\]\[6\] = \{.*?\n\};", combat, re.S).group(0)
    punch_frames = "\n".join(re.search(r"static const u8 " + name + r"\[\]\[2\] = \{.*?\n\};", form, re.S).group(0)
                             for name in ("sGoronPunchFrames", "sZoraPunchFrames", "sGerudoStationaryFrames"))
    native_declarations = "\n".join(body[:body.index("{")].strip() + ";" for body in native_bodies)
    actors = []
    for label, source in (("boulder", bombiwa), ("grotto", grotto)):
        actors.append(re.search(r"static ColliderCylinderInit sCylinderInit = \{.*?\n\};", source, re.S).group(0)
                      .replace("sCylinderInit", label + "CylinderInit"))
    actors.extend((function(bombiwa, "ObjBombiwa_Update"), function(grotto, "DoorAna_WaitClosed")))
    with tempfile.TemporaryDirectory(prefix="goron-field-collision-") as temporary:
        build = Path(temporary)
        (build / "native_collision.inc").write_text(native_declarations + "\n" + "\n".join(native_bodies))
        (build / "form_collision.inc").write_text(enum + "\n" + properties + "\n" + props + "\n" + quad_params + "\n" + punch_frames + "\n" + "\n".join(form_bodies))
        (build / "actors.inc").write_text("\n".join(actors))
        binary = build / "collision"
        compiler_flags = [*flags(), "-I" + temporary, "-O1", "-g", "-ffunction-sections", "-fdata-sections",
                          "-fno-pie", "-no-pie", "-Werror=implicit-function-declaration"]
        if args.sanitize:
            compiler_flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
        subprocess.run([os.environ.get("CC", "cc"), *compiler_flags,
                        str(Path(__file__).with_name("collision_test.c")),
                        str(root / "soh/src/code/sys_math3d.c"), "-Wl,--gc-sections", "-lm", "-o", str(binary)], check=True)
        failures = 0
        for case in args.cases or ["walk", "punch", "pound-grotto", "pound-window", "pound-damage",
                                  "textbox", "post-textbox-punch", "restore", "controls"]:
            result = subprocess.run([str(binary), case], check=False, timeout=10,
                                    env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
            failures += result.returncode != 0
        return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
