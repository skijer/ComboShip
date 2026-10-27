"""Exercise MM's real Epona wrapper and native archive command streams."""
import argparse
from pathlib import Path
import os
import shlex
import struct
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mm", type=Path, help="Optional native archive for all 11 true material streams")
    args = parser.parse_args()
    wrapper = (ROOT / "mm/2s2h/Enhancements/Graphics/EponaCosmetics.cpp").read_text()
    wrapper = wrapper[wrapper.index("namespace {"):]
    native = "static void LoadNativeDisplayLists(Ship::ResourceManager& rm) {\n"
    if args.mm is not None:
        archive = zipfile.ZipFile(args.mm)
        for name in archive.namelist():
            if not name.startswith("objects/object_horse_link_child/") or "DL_" not in name:
                continue
            data = archive.read(name)
            commands = list(struct.iter_unpack("<II", data[72:]))
            native += '{ auto list = std::make_shared<Fast::DisplayList>(); list->Instructions = {\n'
            native += ",\n".join(f"Command(0x{w0:08X}, 0x{w1:08X})" for w0, w1 in commands)
            native += '}; rm.resources["' + name + '"] = list; }\n'
        archive.close()
    else:
        native += ('auto list = std::make_shared<Fast::DisplayList>(); '
                   'list->Instructions = { Command(0xDF000000, 0) }; '
                   'rm.resources["objects/object_horse_link_child/object_horse_link_child_DL_000C70"] = list;\n')
    native += "}\n"
    flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DNON_EQUIVALENT", "-DNON_MATCHING"]
    flags += ["-I" + str(ROOT / p) for p in ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h",
              "mm/assets", "mm/tests", "libultraship/include", "libultraship/src", "combo")]
    if args.mm is not None:
        flags.append("-DMM_EPONA_NATIVE_FIXTURE")
    with tempfile.TemporaryDirectory(prefix="mm-epona-cosmetics-") as temporary:
        build = Path(temporary)
        (build / "mm_epona_production.inc").write_text(wrapper)
        (build / "mm_epona_native.inc").write_text(native)
        binary = build / "test"
        command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-O1", "-g",
                   "-fsanitize=undefined", "-fno-sanitize-recover=all", *flags, "-I" + str(build),
                   str(ROOT / "mm/tests/epona_cosmetics_test.cpp"),
                   str(ROOT / "libultraship/src/ship/utils/StrHash64.cpp"), "-o", str(binary)]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(binary)], check=True)
        actor = ROOT / "mm/src/overlays/actors/ovl_En_Horse/z_en_horse.c"
        result = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), "-std=gnu2x", *flags,
                                 "-Werror=implicit-function-declaration", "-Wno-int-conversion",
                                 "-Wno-incompatible-pointer-types", "-fsyntax-only", str(actor)],
                                capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        print("PASS complete MM horse actor syntax against real integration headers")


if __name__ == "__main__":
    main()
