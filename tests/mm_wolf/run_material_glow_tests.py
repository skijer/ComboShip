#!/usr/bin/env python3
"""Execute each host's actual Wolf material through real native GBI macros."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def body(source, signature):
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    renderer = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    header = (ROOT / "libultraship/include/fast/interpreter.h").read_text()
    backend = (ROOT / "libultraship/src/fast/backends/gfx_direct3d11.cpp").read_text()
    definitions = body(header, "enum {\n    SHADER_0") + ";\n"
    definitions += body(header, "enum class ShaderOpts") + ";\n"
    definitions += "#define SHADER_OPT(opt) ((uint64_t)(1 << static_cast<int>(ShaderOpts::opt)))\n"
    definitions += body(header, "struct ColorCombinerKey") + ";\n"
    definitions += body(header, "struct ColorCombiner {") + ";\n"
    generator = body(renderer, "void Interpreter::GenerateCC").replace("Interpreter::GenerateCC", "GenerateCC")
    shader_input = body(backend, "static const char* prism_shader_item_to_str")
    shader_input = '#define RAND_NOISE "unused noise"\n' + shader_input + '\n#undef RAND_NOISE\n'
    with tempfile.TemporaryDirectory(prefix="wolf-material-glow-") as temporary:
        directory = Path(temporary)
        (directory / "wolf-renderer.inc").write_text(definitions + generator + "\n" + shader_input)
        failed = False
        for host in ("mm", "soh"):
            source = (ROOT / host / "mods/transformation_masks/wolf_link_form.cpp").read_text()
            material = body(source, "static u32 Log2(") + "\n" + body(source, "static void BuildMaterialDisplayList(")
            (directory / "wolf-material.inc").write_text(material)
            binary = directory / host
            subprocess.run([
                os.environ.get("CXX", "c++"), "-std=c++20", "-DF3DEX_GBI_2",
                "-I" + str(ROOT / "libultraship/include"), "-I" + str(directory),
                "-O1", "-g", "-fsanitize=address,undefined" if args.sanitize else "-fsanitize=undefined",
                "-fno-sanitize-recover=all",
                str(ROOT / "tests/mm_wolf/material_glow_test.cpp"), "-o", str(binary)
            ], check=True)
            print(host + ":", flush=True)
            failed |= subprocess.run([str(binary)]).returncode != 0
        if failed:
            raise SystemExit(1)


if __name__ == "__main__":
    main()
