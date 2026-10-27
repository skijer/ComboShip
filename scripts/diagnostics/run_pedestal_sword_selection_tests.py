"""Exercise production pedestal sword selection against actual age-separated maps."""
import pathlib
import subprocess
import tempfile
from run_time_pedestal_tests import functions

ROOT = pathlib.Path(__file__).resolve().parents[2]


def main():
    source = (ROOT / "soh/mods/pak_loader/pak_loader.cpp").read_text().replace('extern "C" ', '')
    names = ("IsOtrPathString", "MakeMiniDL", "MergeTimePedestalSwordPieces", "PakLoader_GetTimePedestalSwordDL")
    selected = functions(source, set(names))
    with tempfile.TemporaryDirectory(prefix="pedestal-sword-") as directory:
        directory = pathlib.Path(directory)
        (directory / "pedestal_sword.inc").write_text("\n".join(selected[name] for name in names if name in selected))
        executable = directory / "test"
        subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-I" + str(directory),
                        str(ROOT / "soh/tests/pedestal_sword_selection_test.cpp"), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
