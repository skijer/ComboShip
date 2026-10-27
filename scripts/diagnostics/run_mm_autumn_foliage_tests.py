"""Exercise MM's real native tree/leaf actor and its submitted material commands."""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    flags = ["-std=gnu17", "-O1", "-g", "-ffunction-sections", "-fdata-sections",
             "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
             "-Werror=implicit-function-declaration", "-Wno-incompatible-pointer-types",
             "-Wno-int-conversion"]
    flags += ["-I" + str(ROOT / path) for path in
              ("mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm", "mm/2s2h",
               "libultraship/include", "libultraship/src", "combo")]
    flags += shlex.split(os.environ.get("MM_FOLIAGE_TEST_CFLAGS", ""))
    compiler = shlex.split(os.environ.get("CC", "cc"))
    with tempfile.TemporaryDirectory(prefix="mm-autumn-foliage-") as temporary:
        binary = Path(temporary) / "test"
        actors = [ROOT / "mm/src/overlays/actors" / path for path in
                  ("ovl_En_Wood02/z_en_wood02.c", "ovl_Obj_Tree/z_obj_tree.c")]
        for inputs in (["-fsyntax-only", *map(str, actors)],
                       [str(ROOT / "mm/tests/autumn_foliage_test.c"),
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)]):
            result = subprocess.run([*compiler, *flags, *inputs], cwd=ROOT,
                                    capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
        print("PASS real-header native tree/leaf syntax", flush=True)
        environment = os.environ.copy()
        options = environment.get("ASAN_OPTIONS", "")
        # The harness allocates no heap memory. LeakSanitizer cannot inspect
        # /proc tasks in some CI sandboxes; retain all explicit caller settings.
        if "detect_leaks=" not in options:
            environment["ASAN_OPTIONS"] = options + (":" if options else "") + "detect_leaks=0"
        subprocess.run([str(binary), *sys.argv[1:]], check=True, env=environment)


if __name__ == "__main__":
    main()
