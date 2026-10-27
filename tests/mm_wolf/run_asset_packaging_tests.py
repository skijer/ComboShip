#!/usr/bin/env python3
"""Exercise the production optional Wolf deploy/install recipe in a temporary CMake project.

The small header fixture tests packaging mechanics only. It is not an actual
Wolf rig, is never a deliverable, and does not prove runtime model acceptance.
"""
from pathlib import Path
import struct
import subprocess
import tempfile
import sys
import hashlib

ROOT = Path(__file__).resolve().parents[2]


def run(*args, succeeds=True):
    result = subprocess.run(args, capture_output=True, text=True)
    if (result.returncode == 0) != succeeds:
        print(result.stdout + result.stderr)
        raise SystemExit("unexpected CMake result")
    return result


with tempfile.TemporaryDirectory(prefix="wolf-packaging-") as temporary:
    base = Path(temporary)
    source = base / "source"
    source.mkdir()
    (source / "main.c").write_text("int main(void) { return 0; }\n")
    module = (ROOT / "CMake/NeiWolfAsset.cmake").as_posix()
    (source / "CMakeLists.txt").write_text(
        'cmake_minimum_required(VERSION 3.20)\n'
        'project(WolfPackagingFixture LANGUAGES C)\n'
        'add_executable(ComboShip main.c)\n'
        'set_target_properties(ComboShip PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/runtime")\n'
        f'include("{module}")\n'
    )
    build = base / "build"
    run("cmake", "-S", str(source), "-B", str(build))
    run("cmake", "--build", str(build), "--target", "ComboShip")
    assert not (build / "runtime/nei").exists(), "no implicit or generated Wolf asset may be deployed"

    asset = base / "packaging-fixture.bin"
    content = b"NEIWOLF1" + struct.pack("<I", 1) + bytes(76)
    asset.write_bytes(content)
    configured = run("cmake", "-S", str(source), "-B", str(build), "-DNEI_WOLF_LINK_ASSET=" + str(asset))
    assert "SHA256" in configured.stdout
    run("cmake", "--build", str(build), "--target", "ComboShip")
    for game in ("soh", "2ship"):
        assert (build / "runtime/nei" / game / "wolf_link.bin").read_bytes() == content
    installed = base / "installed"
    run("cmake", "--install", str(build), "--component", "combo", "--prefix", str(installed))
    for game in ("soh", "2ship"):
        assert (installed / "nei" / game / "wolf_link.bin").read_bytes() == content

    executable = build / "runtime/ComboShip"
    timestamp = executable.stat().st_mtime_ns
    content = b"NEIWOLF1" + struct.pack("<I", 2) + bytes(80)
    asset.write_bytes(content)
    run("cmake", "--build", str(build), "--target", "ComboShip")
    assert executable.stat().st_mtime_ns == timestamp, "asset deployment must not require executable relinking"
    for game in ("soh", "2ship"):
        assert (build / "runtime/nei" / game / "wolf_link.bin").read_bytes() == asset.read_bytes()

    if len(sys.argv) == 2:
        real_asset = Path(sys.argv[1]).resolve()
        real_content = real_asset.read_bytes()
        run("cmake", "-S", str(source), "-B", str(build), "-DNEI_WOLF_LINK_ASSET=" + str(real_asset))
        run("cmake", "--build", str(build), "--target", "ComboShip")
        run("cmake", "--install", str(build), "--component", "combo", "--prefix", str(installed))
        for game in ("soh", "2ship"):
            assert (build / "runtime/nei" / game / "wolf_link.bin").read_bytes() == real_content
            assert (installed / "nei" / game / "wolf_link.bin").read_bytes() == real_content
        print("PASS actual Wolf optional copy/install SHA256 " + hashlib.sha256(real_content).hexdigest())

    asset.write_bytes(b"invalid asset" + bytes(76))
    rejected = run("cmake", "-S", str(source), "-B", str(base / "invalid"),
                   "-DNEI_WOLF_LINK_ASSET=" + str(asset), succeeds=False)
    assert "invalid NEIWOLF1" in rejected.stdout + rejected.stderr
    missing = run("cmake", "-S", str(source), "-B", str(base / "missing"),
                  "-DNEI_WOLF_LINK_ASSET=" + str(base / "absent.bin"), succeeds=False)
    assert "existing wolf_link.bin" in missing.stdout + missing.stderr

print("PASS optional Wolf packaging: explicit file, both game paths, install, replacement and invalid/missing inputs")
