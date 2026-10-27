#!/usr/bin/env python3
"""Build/run real Mirror production logic with minimal native-engine fixtures.

No complete game build/assets are present in the partial recovery checkout.
These checks do not constitute in-game or Windows runtime evidence.
"""
from pathlib import Path
import os
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent


def main():
    failures = []
    with tempfile.TemporaryDirectory(prefix="magic-mirror-native-") as output:
        output = Path(output)
        # Execute precisely the native final equip block. The surrounding native
        # function animates UI coordinates; it is irrelevant to inventory writes.
        native = (ROOT / "soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_item.c").read_text()
        start = native.index("            // If the item is on another button already, swap the two")
        end = native.index("            pauseCtx->unk_1E4 = 0;", start)
        (output / "oot_pause_commit.inc").write_text(
            "void CommitPauseEquip(PlayState* play) {\n"
            "    PauseContext* pauseCtx = &play->pauseCtx;\n" + native[start:end] + "}\n"
        )
        for game, game_root in (("oot", "soh"), ("mm", "mm")):
            for fixture in ("equip", "player"):
                name = f"{game}_{fixture}"
                binary = output / name
                flags = ["-DMIRROR_FIXTURE_MM", "-DCOMBO_MAGIC_MIRROR_MM"] if game == "mm" else []
                command = [
                    os.environ.get("CXX", "g++"), "-std=c++17", "-g", "-O1", "-Wall", "-Wextra",
                    "-Werror", "-Wno-comment", "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                    "-DCOMBO_BUILD", *flags,
                    "-I", str(HERE / "stubs"), "-I", str(ROOT / "combo"), "-I", str(ROOT / game_root),
                    "-I", str(output), str(HERE / f"{fixture}_native_test.cpp"), "-o", str(binary),
                ]
                compiled = subprocess.run(command, text=True, capture_output=True)
                if compiled.returncode:
                    failures.append(name)
                    print(f"FAIL compile {name}\n{compiled.stdout}{compiled.stderr}")
                    continue
                tested = subprocess.run([str(binary)], text=True, capture_output=True,
                                        # This container cannot expose /proc tasks to LSan.
                                        env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0:halt_on_error=1",
                                             "UBSAN_OPTIONS": "halt_on_error=1:print_stacktrace=1"})
                print(f"{name}: {tested.stdout}{tested.stderr}", end="")
                if tested.returncode:
                    failures.append(name)
    if failures:
        raise SystemExit("Failed fixtures: " + ", ".join(failures))
    print("PASS: all four production fixtures under ASan/UBSan")
    subprocess.run([sys.executable, '-B', str(HERE/'run_camera.py')], check=True)


if __name__ == "__main__":
    main()
