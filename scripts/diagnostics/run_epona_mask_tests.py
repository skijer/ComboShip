"""Compile and run the shared immutable Epona material masks."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    if not (ROOT / "combo/EponaCosmeticMasks.h").exists():
        raise AssertionError("Epona semantic material masks have not been implemented")
    with tempfile.TemporaryDirectory(prefix="epona-masks-") as temporary:
        binary = Path(temporary) / "test"
        subprocess.run([
            *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
            "-I" + str(ROOT / "combo"), "-I" + str(ROOT / "mm/tests"),
            str(ROOT / "mm/tests/epona_masks_test.cpp"), "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
