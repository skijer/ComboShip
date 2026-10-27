#!/usr/bin/env python3
"""Run real shared-code behavior checks without a full game rebuild."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--compiler", default=os.environ.get("CXX", "c++"))
parser.add_argument("--msvc", action="store_true")
args = parser.parse_args()
manifest = json.loads((ROOT / "soh/assets/custom/emotes/manifest.json").read_text())
with tempfile.TemporaryDirectory(prefix="combo-emotes-") as temp:
    exe = Path(temp) / ("emote_test.exe" if args.msvc else "emote_test")
    if args.msvc:
        command = [args.compiler, "/nologo", "/std:c++20", "/EHsc", "/utf-8", "/W4", "/WX",
                   "/I" + str(ROOT / "combo"), str(ROOT / "tests/emotes/emote_test.cpp"),
                   "/Fe:" + str(exe), "/Fo:" + str(Path(temp) / "emote_test.obj")]
    else:
        command = [args.compiler, "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I" + str(ROOT / "combo"),
                   str(ROOT / "tests/emotes/emote_test.cpp"), "-o", str(exe)]
    subprocess.run(command, check=True)
    streams = [str(ROOT / "soh/assets/custom/emotes" / entry["animation"]) for entry in manifest]
    subprocess.run([str(exe), *streams], check=True)
assert len(manifest) == 8
for game in ("soh", "mm"):
    folder = ROOT / game / "assets/custom/emotes"
    for entry in manifest:
        data = (folder / entry["animation"]).read_bytes()
        assert len(data) == entry["frames"] * 134
        assert hashlib.sha256(data).hexdigest() == entry["animation_sha256"]
        if entry["audio"]:
            assert (folder / entry["audio"]).stat().st_size > 0
    assert (folder / "CREDITS.txt").read_text().count("Malon") > 0
assert (ROOT / "soh/assets/custom/emotes/manifest.json").read_bytes() == \
       (ROOT / "mm/assets/custom/emotes/manifest.json").read_bytes()
print("PASS all eight bundled animation streams, sound assets, checksums and credits in both games")
