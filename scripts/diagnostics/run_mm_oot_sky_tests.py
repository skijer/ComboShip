"""Exercise real MM sky setup, geometry and emitted draw commands without game assets."""
import os
import argparse
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pack", type=Path, help="Optional original OoT sky O2R for resource-path verification")
    args = parser.parse_args()
    flags = ["-std=gnu11", "-DNDEBUG", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
             "-Wno-int-conversion", "-Wno-incompatible-pointer-types", "-Werror=implicit-function-declaration",
             "-ffunction-sections",
             "-fdata-sections"]
    flags += ["-I" + str(ROOT / p) for p in
              ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h", "mm/assets",
               "libultraship/include", "libultraship/src", "combo")]
    with tempfile.TemporaryDirectory(prefix="mm-oot-sky-") as tmp:
        binary = Path(tmp) / "sky_test"
        command = [*shlex.split(os.environ.get("CC", "cc")), *flags,
                   *shlex.split(os.environ.get("MM_SKY_TEST_CFLAGS", "")),
                   str(ROOT / "mm/tests/oot_sky_test.c"),
                   str(ROOT / "mm/src/code/z_vr_box.c"),
                   str(ROOT / "mm/src/code/z_vr_box_draw.c"),
                   "-Wl,--gc-sections", "-lm", "-o", str(binary)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        run = [str(binary)]
        if args.pack:
            with zipfile.ZipFile(args.pack) as archive:
                manifest = Path(tmp) / "pack-files.txt"
                manifest.write_text("\n".join(archive.namelist()))
            run.append(str(manifest))
        subprocess.run(run, check=True)
    subprocess.run([sys.executable, str(ROOT / "tests/skybox/run_oot_sky_star_tests.py")], check=True)


if __name__ == "__main__":
    main()
