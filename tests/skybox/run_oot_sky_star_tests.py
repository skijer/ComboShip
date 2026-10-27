"""Execute production MM sky activation and native-star setup without game assets."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile


def production_function(text, name):
    match = re.search(r"^[\w *]+\b" + re.escape(name) + r"\([^;{]*\)\s*\{", text, re.M)
    if match is None:
        raise RuntimeError("Missing production function: " + name)
    start = match.start()
    depth = 0
    for position in range(text.index("{", start), len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start:position + 1]
    raise RuntimeError("Unterminated production function: " + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    sky = (args.root / "mm/src/code/z_vr_box.c").read_text()
    environment = (args.root / "mm/src/code/z_kankyo.c").read_text()
    parts = []
    # The original source has no shared predicate. Include it when present so
    # the same fixture also reproduces the original duplicate-star failure.
    if re.search(r"\bSkybox_IsOotSkyActive\([^;{]*\)\s*\{", sky):
        parts.append(production_function(sky, "Skybox_IsOotSkyActive"))
    parts.extend((production_function(sky, "Skybox_PrepareOot"),
                  production_function(environment, "Environment_SetupSkyboxStars")))
    with tempfile.TemporaryDirectory(prefix="oot-sky-stars-") as temporary:
        build = Path(temporary)
        (build / "production_sky.inc").write_text("\n\n".join(parts))
        executable = build / "oot-sky-stars"
        command = [*shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-Wall", "-Wextra", "-Werror", "-O1", "-g"]
        if args.sanitize:
            command.extend(("-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer"))
        command.extend(("-I" + str(build), str(Path(__file__).with_name("oot_sky_star_test.c")),
                        "-o", str(executable)))
        subprocess.run(command, check=True)
        return subprocess.run([str(executable)], check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
