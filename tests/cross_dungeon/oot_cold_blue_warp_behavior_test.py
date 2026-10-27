#!/usr/bin/env python3
"""Exercise the registered native-init blue-warp hook with the real route owner.

The source blue-warp animation has already completed before this hook runs in
Play_Init. Cover first/repeat aliases without manually completing a transition.
The real GameInteractor registry, entrance dispatch, grotto materializer and
cross runtime execute; peer DLL loading and unrelated flag services are boundaries.

Red control for the first fade-gated candidate (current runtime plus its original
hook): --hook-ref 86582f5a. That hook leaves a foreign route pending forever;
the original full 86582f5a runtime used immediate handoff and is not implicated.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from


def function(source, marker):
    return block_from(source, source.index(marker))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--hook-ref", help="use an older hook with the current runtime as a red control")
    args = parser.parse_args()
    fixture_path = ROOT / "tests/cross_dungeon/oot_pool_runtime_behavior_test.py"
    env = {"__file__": str(fixture_path)}
    previous = sys.argv
    try:
        sys.argv = [str(fixture_path)]
        prefix = fixture_path.read_text().split("with tempfile.TemporaryDirectory(")[0]
        exec(compile(prefix, str(fixture_path), "exec"), env)
    finally:
        sys.argv = previous
    source = env["source"].split("int main()")[0]
    source = source.replace("void* Combo_ResolveSym(const char*,const char*) { return nullptr; }", r'''
static int queued=-1,queues=0;
static int QueuePeer(int entrance) { queued=entrance;++queues;return 1; }
void* Combo_ResolveSym(const char*,const char*) { return reinterpret_cast<void*>(QueuePeer); }
''')
    native = (ROOT / "soh/soh/Enhancements/randomizer/randomizer_entrance.c").read_text()
    source += "\ns16 Entrance_GetOverride(s16 index) { return index; }\n"
    source += function(native, "s16 Entrance_OverrideNextIndex(") + "\n"
    source += function(native, "void Entrance_OverrideBlueWarp(") + "\n"
    hook_path = "soh/soh/Enhancements/TimeSavers/SkipCutscene/Story/SkipBlueWarp.cpp"
    hook = (subprocess.check_output(["git", "show", args.hook_ref + ":" + hook_path], cwd=ROOT, text=True)
            if args.hook_ref else (ROOT / hook_path).read_text())
    policy = hook[hook.index("static bool sEnteredBlueWarp"):hook.index("extern void TimeSaverQueueItem")]
    source += (ROOT / "tests/cross_dungeon/oot_cold_blue_warp_cases.inc").read_text().replace(
        "/* BLUE_WARP_HOOK */", policy + "\n" + function(hook, "void RegisterShouldPlayBlueWarp()"))
    command = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-Wno-narrowing", "-DCOMBO_BUILD",
               "-DF3DEX_GBI_2", "-DCONTROLLERBUTTONS_T=uint32_t", "-DLOG_LEVEL_GAME_PRINTS=0",
               "-I" + str(ROOT.parent / "build-deps/usr/include")]
    command += ["-I" + str(ROOT / path) for path in ("combo", "soh", "soh/include", "soh/include/PR",
                                                   "soh/src", "soh/assets", "soh/mods", "soh/soh",
                                                   "libultraship/include")]
    if args.sanitize:
        command += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="oot-cold-blue-warp-") as temporary:
        directory = Path(temporary)
        cpp = directory / "test.cpp"
        cpp.write_text(source)
        binary = directory / "test"
        subprocess.run(command + [str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
