#!/usr/bin/env python3
"""Replay the native player's deferred shield matrix reads, without a GPU/ROM."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_back_equipment_tests import function
from run_mm_nei_tests import flags as mm_flags
sys.path.insert(0, str(ROOT / "tests/nei_held"))
from run_articulated_tests import flags as oot_flags


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--host", choices=("mm", "oot"))
    args = parser.parse_args()
    renderer = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    with tempfile.TemporaryDirectory(prefix="ikana-shield-render-") as directory:
        build = Path(directory)
        for host in ([args.host] if args.host else ["mm", "oot"]):
            engine = "soh" if host == "oot" else "mm"
            graph = (ROOT / engine / "src/code/graph.c").read_text()
            stubs = (ROOT / ("soh/soh/GbiWrap.cpp" if host == "oot" else "mm/src/code/stubs.c")).read_text()
            equipment = (ROOT / engine / "mods/extended_equipment.c").read_text()
            player = (ROOT / engine / "src/code/z_player_lib.c").read_text()
            helpers = 'extern "C" {\n'
            for name in ("Graph_OpenDisps", "Graph_CloseDisps"):
                helpers += function(graph, name) + "\n"
            for name in ("gSPDisplayList", "gSPSegment"):
                helpers += function(stubs, name) + "\n"
            helpers += function(equipment, "DrawCachedShieldDL") + "\n}\n"
            helpers += function(renderer, "Interpreter::SegAddr") + "\n"
            # Execute the exact skeleton-dispatch statement from Player_DrawImpl;
            # its other operations set eyes, mouth and material state.
            call = re.findall(r'\bSkelAnime_DrawFlexLod\w*\([^;]+;', function(player, "Player_DrawImpl"))
            assert len(call) == 1
            helpers += '\nstatic void DrawPlayer(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount, '
            helpers += ('PostLimbDrawOpa' if host == 'oot' else 'PostLimbDrawFlex')
            helpers += ' postLimbDraw, Actor* actor, s32 lod) {\n'
            helpers += 'void* data = actor;\n'
            helpers += ('OverrideLimbDrawOpa' if host == 'oot' else 'OverrideLimbDrawFlex')
            helpers += ' overrideLimbDraw = nullptr;\n' + call[0] + '\n}\n'
            (build / "production_helpers.inc").write_text(helpers)
            for combo in (False, True):
                base_flags = oot_flags()[1:] if host == "oot" else mm_flags()
                native_flags = [*base_flags, "-O0", "-g", "-ffunction-sections", "-fdata-sections"]
                if host == "oot":
                    native_flags += ["-DHOST_OOT"]
                if combo:
                    native_flags += ["-DCOMBO_BUILD"]
                if args.sanitize:
                    native_flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                                     "-fno-omit-frame-pointer", "-fno-pie"]
                objects = []
                sources = [engine + "/src/code/z_skelanime.c", engine + "/src/code/sys_matrix.c",
                           "soh/soh/gu_pc.c" if host == "oot" else "mm/2s2h/gu_pc.c"]
                if host == "oot":
                    sources += ["soh/src/code/z_skin_matrix.c"]
                for source in sources:
                    output = build / (Path(source).stem + ".o")
                    source_path = ROOT / source
                    if host == "oot" and source.endswith("sys_matrix.c"):
                        # Expose only fixture initialization of SoH's private stack;
                        # all production matrix functions are compiled unchanged.
                        source_path = build / "oot_matrix.c"
                        source_path.write_text((ROOT / source).read_text() +
                            '\nvoid IkanaTest_SetMatrixStack(MtxF* stack) { sMatrixStack = sCurrentMatrix = stack; }\n')
                    source_flags = list(native_flags)
                    if args.sanitize and source.endswith("gu_pc.c"):
                        # Both unchanged native guMtxF2L implementations rely on
                        # signed fixed-point shifts (e.g. 65536 << 16). Keep all
                        # other ASan/UBSan checks, excluding shift checks ONLY in
                        # that legacy conversion translation unit.
                        source_flags += ["-fno-sanitize=shift"]
                    subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *source_flags,
                                    "-w", "-c", str(source_path), "-o", str(output)], check=True)
                    objects.append(str(output))
                binary = build / (host + ("-combo" if combo else "-original"))
                command = [os.environ.get("CXX", "c++"), "-std=c++20", *native_flags, "-w",
                           "-I" + str(build), str(Path(__file__).with_name("native_segment_test.cpp")),
                           *objects, "-Wl,--gc-sections", "-lm", "-o", str(binary)]
                if args.sanitize:
                    command += ["-no-pie"]
                subprocess.run(command, check=True)
                subprocess.run([str(binary)], check=True)
    print("PASS native player shield matrix bindings and non-player isolation in all requested builds")


if __name__ == "__main__":
    main()
