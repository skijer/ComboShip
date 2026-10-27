#!/usr/bin/env python3
"""Compare extracted production vertex loading bit-for-bit with the accepted baseline.

Uses the triangle fixture's lightweight backend and production interpreter types.
--benchmark alternates candidate/baseline CPU vertex batches, not GPU/game FPS.
VERTEX_TEST_MUTATE=1 proves position comparison rejects an incorrect candidate.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^(?:template\s*<[^>]+>\s*)?(?:[\w:*<>]+\s+)*" + re.escape(name) +
                      r"\([^;{}]*\)\s*(?:const\s*)?\{", source, re.M)
    if not match:
        raise RuntimeError(f"Missing production function: {name}")
    # Ignore braces in comments and literals while retaining source offsets.
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"',
                    lambda m: " " * len(m.group()), source)
    pos, depth = match.end(), 1
    while depth:
        depth += (masked[pos] == "{") - (masked[pos] == "}")
        pos += 1
    return source[match.start():pos] + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark", action="store_true")
    parser.add_argument("--skip-null-test", action="store_true",
                        help="Skip the existing null-member-address UB case for sanitizer runs only")
    args = parser.parse_args()
    def read(path):
        return (ROOT / path).read_text(encoding="utf-8")
    source = read("libultraship/src/fast/interpreter.cpp")
    header = read("libultraship/include/fast/interpreter.h").replace("void GfxSpVertex(", "void GfxSpVertexBaseline(size_t, size_t, const F3DVtx*);\n    void GfxSpVertex(")
    api = (ROOT / "libultraship/include/fast/backends/gfx_rendering_api.h").read_text()
    with tempfile.TemporaryDirectory(prefix="vertex-batch-") as temporary:
        build = Path(temporary)
        stubs = {
            "fast/interpreter.h": header,
            "fast/backends/gfx_rendering_api.h": api,
            "fast/resource/type/Texture.h": """#pragma once
#include "ship/resource/Resource.h"
namespace Fast {
enum class TextureType { RGBA32bpp = 1, Palette4bpp = 3, Palette8bpp = 4 };
class Texture {
  public:
    TextureType Type = TextureType::Palette8bpp;
    uint16_t Width = 64, Height = 32;
    uint8_t* ImageData = nullptr;
    uint32_t ImageDataSize = 0;
    std::shared_ptr<Ship::ResourceInitData> init = std::make_shared<Ship::ResourceInitData>();
    std::shared_ptr<Ship::ResourceInitData> GetInitData() { return init; }
};
}
""",
            "ship/resource/Resource.h": """#pragma once
#include <memory>
#include <string>
namespace Ship {
class IResource { public: inline static const std::string gAltAssetPrefix = "alt/"; };
struct Archive { std::string path; const std::string& GetPath() { return path; } };
struct ResourceInitData { std::string Path; std::shared_ptr<Archive> Parent; };
}
""",
            "imconfig.h": "#pragma once\nusing ImTextureID = unsigned long long;\n",
            "nlohmann/json.hpp": "#pragma once\n#include <initializer_list>\nnamespace nlohmann { struct json { json() = default; json(std::initializer_list<json>) {} template<class T> json(const T&) {} static json array() { return {}; } }; }\n",
        }
        for path, content in stubs.items():
            target = build / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(content)
        # Unused graphics operations are no-ops; observed operations are overridden
        # in RecordingBackend. Keep the actual virtual ABI and production types.
        methods = re.findall(r"virtual\s+([^;{}]+?)\s*=\s*0\s*;", api, re.S)
        defaults = []
        for method in methods:
            defaults.append(method.strip() + " override {" + ("" if method.strip().startswith("void ") else "return {};") + "}")
        (build / "backend_defaults.inc").write_text("\n".join(defaults))
        helpers = source[source.index("static UcodeHandlers ucode_handler_index"):source.index("static std::string GetPathWithoutFileName")]
        names = ["IsPyramidLike", "Interpreter::AdjXForAspectRatio", "Interpreter::GfxSpVertex", "Interpreter::Interpreter", "Interpreter::~Interpreter", "Interpreter::Flush",
                 "Interpreter::LookupOrCreateShaderProgram", "Interpreter::GenerateCC",
                 "Interpreter::LookupOrCreateColorCombiner", "GetTileSizeFromCoordinates",
                 "Interpreter::GfxSpTri1", "gfx_tri1_otr_handler_f3dex2", "gfx_tri1_handler_f3dex2",
                 "gfx_tri1_handler_f3dex", "gfx_tri1_handler_f3d", "gfx_tri2_handler_f3dex",
                 "gfx_quad_handler_f3dex2", "gfx_quad_handler_f3dex", "Interpreter::SpReset",
                 "Interpreter::NormalizeVector", "Interpreter::TransposedMatrixMul", "Interpreter::CalculateNormalDir",
                 "gfx_set_prim_depth_handler_rdp", "gfx_set_key_r_handler_rdp", "gfx_set_key_gb_handler_rdp"]
        if "Interpreter::PrepareTriangleState(" in source:
            names.insert(names.index("Interpreter::GfxSpTri1"), "Interpreter::PrepareTriangleState")
        if "Interpreter::GfxSpTri1Impl(" in source:
            names.insert(names.index("Interpreter::GfxSpTri1"), "Interpreter::GfxSpTri1Impl")
        if "Interpreter::TryReusePreludeMaterialBinding(" in source:
            names += ["Interpreter::GetBaseTexturePath", "Interpreter::TryReusePreludeMaterialBinding"]
        if "TraceAutumnForestDraw(" in source:
            names.insert(names.index("Interpreter::GfxSpTri1Impl"), "AutumnForestLayer")
            names.insert(names.index("Interpreter::GfxSpTri1Impl"), "TraceAutumnForestDraw")
        # Destructor name needs the same parser, with its tilde included.
        generated = "namespace Ship { namespace Math {\n" + function(read("libultraship/src/ship/utils/Utils.cpp"), "clamp") + "}}\nnamespace Fast {\nconstexpr size_t MAX_TRI_BUFFER = 256;\nstatic constexpr float N64_PRIM_DEPTH_MAX = 32767.0f;\n" + helpers
        explicit_dispatch = "(*GfxOpcodeHandlerFunc)(Interpreter*" in source
        generated += "static std::weak_ptr<Interpreter> mInstance;\n"
        generated += re.search(r"typedef bool \(\*GfxOpcodeHandlerFunc\).*?;", source).group(0) + "\n"
        if explicit_dispatch:
            generated += "#define GFX_EXPLICIT_DISPATCH 1\n"
        generated += "\n".join(re.findall(r"^#define C[01].*$", source, re.M)) + "\n"
        generated += "\n".join(function(source, name) for name in names)
        if os.environ.get("VERTEX_TEST_MUTATE"):
            generated = generated.replace("d->x = x;", "d->x = x + 1.0f;")
        generated += source[source.index("class UcodeHandler {"):source.index("static constexpr UcodeHandler rdpHandlers")]
        baseline = subprocess.check_output(["git", "show", "9c084d74a7f7d0ae8cf88b18c0ed74abde03a950:libultraship/src/fast/interpreter.cpp"], cwd=ROOT, text=True)
        generated += function(baseline, "Interpreter::GfxSpVertex").replace("Interpreter::GfxSpVertex(", "Interpreter::GfxSpVertexBaseline(")
        generated += "\n}\n"
        generated += function(source, "gfx_cc_get_features")
        (build / "triangle_production.inc").write_text(generated)
        fixture = (ROOT / "libultraship/tests/gfx_triangle_run_test.cpp").read_text()
        (build / "vertex_fixture.inc").write_text(fixture[:fixture.index("static void reuseTest()")])
        binary = build / "vertex-test"
        flags = shlex.split(os.environ.get("GFX_VERTEX_TEST_CXXFLAGS", "-O3"))
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", *flags,
                        "-I" + str(build), "-I" + str(build / "fast"), "-I" + str(ROOT / "libultraship/include"),
                        str(ROOT / "libultraship/tests/gfx_vertex_batch_test.cpp"), "-o", str(binary)], check=True)
        subprocess.run([str(binary), *(["--benchmark"] if args.benchmark else []),
                        *(["--skip-null-test"] if args.skip_null_test else [])], check=True)


if __name__ == "__main__":
    main()
