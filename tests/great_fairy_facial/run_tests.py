#!/usr/bin/env python3
"""Compile the real MM facial/update callbacks against real MM types.

Only resource loading, animation/cutscene services and movement/effects are
recorded by the fixture. The production adapter and blink logic are unchanged.
"""
import argparse
from collections import Counter
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags

ACTOR = Path("mm/src/overlays/actors/ovl_Bg_Dy_Yoseizo/z_bg_dy_yoseizo.c")
BASE = "a82852a3a02c8cabc541cce7c124da71c5755efb"


def function(source, name):
    match = re.search(r"^[^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    assert match, "Missing production function: " + name
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def run(command):
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode:
        print(result.stdout + result.stderr)
        raise SystemExit(result.returncode)
    if result.stdout:
        print(result.stdout, end="")
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--candidate", type=Path)
    parser.add_argument("--native", type=Path)
    args = parser.parse_args()
    source = (ROOT / ACTOR).read_text()
    selected = ["BgDyYoseizo_UsesMMFacialHeads", "BgDyYoseizo_LoadMMFacialHead",
                "BgDyYoseizo_UpdateEyes", "func_80A0B5F0", "BgDyYoseizo_Update",
                "BgDyYoseizo_OverrideLimbDraw"]
    bodies = [function(source, name) for name in selected]
    enum = re.search(r"typedef enum GreatFairyAnimation \{.*?\} GreatFairyAnimation;", source, re.S).group(0)
    animations = re.search(r"static AnimationHeader\* sAnimations\[.*?\n\};", source, re.S).group(0)
    with tempfile.TemporaryDirectory(prefix="mm-great-fairy-") as tmp:
        directory = Path(tmp)
        # The unchanged native table uses OTR strings typed as animation
        # pointers. Allow that existing convention only around that table.
        (directory / "great_fairy_production.inc").write_text(
            enum + '\n#pragma GCC diagnostic push\n'
            '#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"\n' + animations +
            '\n#pragma GCC diagnostic pop\n' + "\n".join(bodies))
        executable = str(directory / "facial_test")
        run([os.environ.get("CC", "cc"), "-std=gnu11", *flags(), "-I" + tmp,
             "-O1", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all",
             "-Werror=implicit-function-declaration", "-Werror=incompatible-pointer-types",
             str(ROOT / "tests/great_fairy_facial/facial_test.c"), "-o", executable])
        run([executable])

    # Prevent an adapter change from silently changing MM rewards, cutscene
    # actions, body-animation selection, native mouth binding or particles.
    baseline = subprocess.check_output(["git", "show", BASE + ":" + ACTOR.as_posix()], cwd=ROOT, text=True)
    names = re.findall(r"^(?:static\s+)?(?:void|s32)\s+(\w+)\([^;{}]*\)\s*\{", baseline, re.M)
    modified = {"BgDyYoseizo_Update", "BgDyYoseizo_OverrideLimbDraw"}
    for name in names:
        if name not in modified:
            assert function(source, name) == function(baseline, name), "Native routine changed: " + name
    assert animations == re.search(r"static AnimationHeader\* sAnimations\[.*?\n\};", baseline, re.S).group(0)
    print("PASS native preservation: all action/reward/effect/draw routines and nine-animation table unchanged")

    with tempfile.TemporaryDirectory(prefix="mm-great-fairy-object-") as tmp:
        original = Path(tmp) / "baseline.c"
        original.write_text(baseline)
        common = [os.environ.get("CC", "cc"), "-std=gnu11", *flags()]
        before = run([*common, "-I" + str((ROOT / ACTOR).parent), "-fsyntax-only", str(original)])
        current = run([*common, "-O2", "-c", str(ROOT / ACTOR), "-o", str(Path(tmp) / "actor.o")])
        messages = lambda result: Counter(re.findall(r"warning: (.*)", result.stderr))
        assert not messages(current) - messages(before), "New complete-actor compiler warnings"
        print("PASS complete actor object compilation on project headers; no new warnings versus baseline")

    importer = [sys.executable, "-B", str(ROOT / "tests/great_fairy_facial/run_importer_tests.py")]
    for label, path in [("--candidate", args.candidate), ("--native", args.native)]:
        if path is not None:
            importer.extend([label, str(path.resolve())])
    run(importer)


if __name__ == "__main__":
    main()
