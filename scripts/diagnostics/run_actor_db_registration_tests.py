"""Compile and execute production SoH actor registration without permissive conversions."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    flags = ["-std=c++20", "-fno-permissive", "-DF3DEX_GBI_2", "-g", "-Wall", "-Wextra", "-Werror",
             "-Wno-unused-parameter", "-Wno-unused-variable", "-Wno-missing-field-initializers",
             "-Wno-missing-braces"]
    for directory in ("soh/include", "soh/src", "soh", "soh/assets", "libultraship/include", "libultraship/src"):
        flags += ["-I" + str(ROOT / directory)]
    flags += shlex.split(os.environ.get("SOH_ACTOR_DB_CXXFLAGS", ""))
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer",
                  "-fno-pie", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="soh-actor-db-") as temp:
        executable = Path(temp) / "actor-db"
        subprocess.run([*compiler, *flags, str(ROOT / "soh/tests/actor_db_registration_test.cpp"),
                        str(ROOT / "soh/soh/ActorDB.cpp"), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
