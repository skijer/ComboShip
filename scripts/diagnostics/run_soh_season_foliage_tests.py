#!/usr/bin/env python3
"""Exercise real Fast::Texture pixel ownership under seasonal changes."""
from pathlib import Path
import argparse
import subprocess

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("output", type=Path)
parser.add_argument("--spdlog-include", type=Path)
parser.add_argument("--sanitize", action="store_true")
parser.add_argument("--texture-fixtures", type=Path, help="Optional JSON array of [serialized OTEX path, surface] pairs")
args = parser.parse_args()
assert (root / "soh/soh/Enhancements/Graphics/SeasonFoliageTint.h").exists(), "Seasonal foliage implementation is missing"
args.output.mkdir(parents=True, exist_ok=True)
# Match spdlog::spdlog's compiled-library boundary. Trace logging is disabled
# for these pixel/cache tests, so no logging or external fmt bodies are linked.
flags = ["-std=c++20", "-O1", "-g", "-DSPDLOG_ACTIVE_LEVEL=6", "-DSPDLOG_COMPILED_LIB", "-I" + str(root / "soh"),
         "-I" + str(root / "libultraship/include")]
if args.spdlog_include:
    flags += ["-I" + str(args.spdlog_include)]
if args.sanitize:
    flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
for test in ["season_foliage_test", "season_foliage_binding_test", "season_foliage_cohort_test",
             "season_ground_tint_polish_test"]:
    exe = args.output / test
    boundary = ["-I" + str(root / "soh/tests/season_foliage_stubs")] if "binding" in test or "cohort" in test else []
    subprocess.run(["c++", *boundary, *flags, str(root / f"soh/tests/{test}.cpp"),
                    str(root / "libultraship/src/fast/resource/type/Texture.cpp"),
                    str(root / "libultraship/src/ship/resource/Resource.cpp"), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
if args.texture_fixtures:
    import json
    fixtures = json.loads(args.texture_fixtures.read_text())
    exe = args.output / "season_foliage_archive_test"
    subprocess.run(["c++", *flags, str(root / "soh/tests/season_foliage_archive_test.cpp"),
                    str(root / "libultraship/src/fast/resource/type/Texture.cpp"),
                    str(root / "libultraship/src/ship/resource/Resource.cpp"), "-o", str(exe)], check=True)
    arguments = []
    for fixture in fixtures:
        if len(fixture) not in (2, 6):
            raise ValueError("Texture fixtures require [path, surface] or [path, surface, left, top, right, bottom]")
        arguments += [str(fixture[0]), str(fixture[1])]
        if len(fixture) == 6:
            arguments += ["--region", *map(str, fixture[2:])]
    subprocess.run([str(exe), *arguments], check=True)
