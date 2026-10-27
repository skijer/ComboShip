"""Exercise production rainbow callbacks, texture caching, and HUD draw commands.

The test uses the real color/pixel code and renderer cache operations. Only
archive loading, the window, config storage and GPU drawing are headless.
"""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    editor = (ROOT / "mm/2s2h/BenGui/CosmeticEditor.cpp").read_text()
    shading = (ROOT / "mm/2s2h/BenGui/CosmeticShading.cpp").read_text()
    shading_header = (ROOT / "mm/2s2h/BenGui/CosmeticShading.h").read_text()
    header = (ROOT / "mm/2s2h/BenGui/CosmeticEditor.h").read_text()
    renderer = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    renderer_header = (ROOT / "libultraship/include/fast/interpreter.h").read_text()
    declarations = header[header.index("typedef enum {"):header.index("#ifdef __cplusplus", header.index("typedef enum {"))]
    declarations += header[header.index("typedef struct {"):header.index("extern std::map<std::string, CosmeticOption>")]
    dynamic = (ROOT / "mm/2s2h/BenGui/DynamicCosmeticEditor.cpp").read_text()
    # Cover every built-in rainbow callback, not only the originally reported Hearts row.
    # Preserve multiline option arguments and their production build guards.
    options_start = editor.index("std::map<std::string, CosmeticOption> cosmeticOptions = {")
    options_end = editor.index("\n};", options_start) + len("\n};")
    declarations += function(editor, "ColorRGBA8")
    declarations += "\n" + editor[options_start:options_end] + "\n"
    declarations += editor[editor.index("static CosmeticOption& kHumanTunicOption"):editor.index("static bool CosmeticEditorIsSuppressed")]

    cache_types = renderer_header[renderer_header.index("struct TextureCacheKey {"):renderer_header.index("struct RGBA {")]
    cache_types += renderer_header[renderer_header.index("struct GfxTextureCache {"):renderer_header.index("struct ColorCombiner {")]
    cache = "\n".join(function(renderer, name) for name in [
        "Interpreter::TextureCacheClear", "Interpreter::TextureCacheDelete", "Interpreter::TextureCacheDeleteByPalette",
    ])
    # Upstream moved palette/HD editing into a separate translation unit. Run
    # that complete production code with the same native cache/pixel assertions.
    pixels = shading_header[shading_header.index("enum SHADE_MODE {"):shading_header.index("void ShadePaletteNewBase")]
    pixels += shading[shading.index("static std::shared_ptr<Fast::Texture> LoadTextureExact"):]
    pixels += editor[editor.index("Gfx backToWhite[] = {"):editor.index('extern "C" Color_RGBA8 CosmeticEditor_GetChangedColorEx')]
    colors = "\n".join(function(editor, name) for name in [
        "CosmeticEditor_GetChangedColorEx", "CosmeticEditor_GetChangedColor",
        "gDPSetPrimColorOverrideEx", "gDPSetPrimColorOverride", "gDPSetEnvColorOverrideEx", "gDPSetEnvColorOverride",
    ])
    # Equipment-driven colors use save data; the editor's actual tunic/hair callbacks do not.
    updates = editor[editor.index("// Player.HumanTunic\n"):editor.index("// Skijer's NEI: equipment-driven tunic color. MM")]
    updates += editor[editor.index("// Player.HumanHair\n"):editor.index("// Per-player tunic tint. The patch above")]
    updates += editor[editor.index("// Player.ZoraTunic\n"):]
    updates = 'static const char* kSillyDullRupeesCvar = CVAR_COSMETIC("Silly.DullRupees");\n' + updates
    custom = dynamic[dynamic.index("struct CustomCosmeticBinding {"):dynamic.index("struct ManifestEntry {")]
    custom += "\nstatic std::vector<CustomCosmeticEntry> customCosmeticEntries;\n"
    custom += "\n".join(function(dynamic, name) for name in [
        "GetCustomCosmeticColor", "ApplyDynamicCosmetics", "UpdateCustomCosmeticsRainbow",
    ])
    updates = custom + updates
    updates += "\n".join(function(editor, name) for name in ["CosmeticEditorRefreshElement", "CosmeticEditorUpdateTick"])
    life = (ROOT / "mm/src/code/z_lifemeter.c").read_text()
    magic = (ROOT / "mm/src/code/z_parameter.c").read_text()
    draw = life[life.index("s16 sHeartsPrimColors"):life.index("void LifeMeter_Init")]
    draw += "\n".join(function(life, name) for name in ["LifeMeter_Init", "LifeMeter_Draw"])
    draw += "\n" + function(magic, "Magic_DrawMeter")
    with tempfile.TemporaryDirectory(prefix="mm-hud-cosmetics-") as temporary:
        build = Path(temporary)
        for name, body in [("hud_declarations", declarations), ("cache_types", cache_types),
                           ("cache_production", cache), ("pixel_production", pixels),
                           ("color_production", colors), ("rainbow_production", updates), ("hud_draw_production", draw)]:
            (build / (name + ".inc")).write_text(body)
        cflags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
                  "-DNON_EQUIVALENT", "-DNON_MATCHING", "-Wno-int-conversion", "-Wno-incompatible-pointer-types"]
        cflags += ["-I" + str(ROOT / p) for p in
                   ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h", "mm/assets",
                    "libultraship/include", "libultraship/src", "combo")]
        cc = shlex.split(os.environ.get("CC", "cc"))
        draw_object = build / "hud_draw_test.o"
        result = subprocess.run([*cc, "-std=gnu11", *cflags, "-Werror=implicit-function-declaration", "-I" + str(build),
                                 "-c", str(ROOT / "mm/tests/hud_draw_colors_test.c"), "-o", str(draw_object)],
                                capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        binary = build / "hud_cosmetics_test"
        # Model the resource metadata exposed by the extracted shader. No HD
        # archive is mounted in this native fixture; its production HD paths
        # still compile and leave the native/custom boundaries under test intact.
        fixture = (ROOT / "mm/tests/hud_cosmetics_test.cpp").read_text()
        fixture = fixture.replace("#include <cstdio>", "#include <cstdio>\n#include <cstring>", 1)
        fixture = fixture.replace("namespace Fast {\nstruct DisplayList", """#define TEX_FLAG_LOAD_AS_RAW (1 << 0)
namespace Fast {
enum class ResourceType { Texture = 1 };
enum class TextureType { RGBA16bpp, Palette4bpp, Palette8bpp };
struct DisplayList""", 1)
        fixture = fixture.replace("struct Resource {", """struct IResource {
    inline static const std::string gAltAssetPrefix = "alt/";
};
struct ArchiveManager {
    bool HasFile(const std::string&) const { return false; }
};
struct Resource {""", 1)
        fixture = fixture.replace("bool IsCustom = false;", "bool IsCustom = false;\n        uint32_t Type = 1;", 1)
        fixture = fixture.replace("std::array<uint8_t, 2048> bytes{};", """std::array<uint8_t, 2048> bytes{};
    uint8_t* ImageData = bytes.data();
    uint32_t ImageDataSize = bytes.size();
    uint32_t Flags = 0;
    float HByteScale = 1.0f, VPixelScale = 1.0f;
    uint16_t Width = 32, Height = 32;
    Fast::TextureType Type = Fast::TextureType::RGBA16bpp;""", 1)
        fixture = fixture.replace("LoadResource(const char* path)", "LoadResource(const std::string& path, bool = false)", 1)
        fixture = fixture.replace("struct ResourceManager {", """struct ResourceManager {
    std::shared_ptr<ArchiveManager> GetArchiveManager() {
        static auto archive = std::make_shared<ArchiveManager>();
        return archive;
    }""", 1)
        fixture = fixture.replace("struct Fast3dWindow : Ship::Window {", "using Texture = Ship::Resource;\nstruct Fast3dWindow : Ship::Window {", 1)
        fixture = fixture.replace('#include "pixel_production.inc"', """void Gfx_TextureCacheDelete(const uint8_t* address) {
    interpreter->TextureCacheDelete(address);
}
bool IsCustomFierceDeityModelActive() { return customPlayer; }
#include "pixel_production.inc"
""", 1)
        fixture_path = build / "hud_cosmetics_test.cpp"
        fixture_path.write_text(fixture)
        subprocess.run([
            *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-O1", "-g",
            "-Wall", "-Wextra", "-Werror", "-Wno-unused-parameter", "-Wno-missing-field-initializers",
            "-DF3DEX_GBI_2", "-I" + str(build), "-I" + str(ROOT / "mm"),
            "-I" + str(ROOT / "libultraship/include"),
            "-I" + str(ROOT), str(fixture_path), str(ROOT / "mm/mods/combo_rpg.cpp"),
            str(draw_object), "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True)
        for path in ["mm/src/code/z_lifemeter.c", "mm/src/code/z_parameter.c"]:
            result = subprocess.run([*cc, "-std=gnu11", *cflags, "-Werror=implicit-function-declaration",
                                     "-fsyntax-only", str(ROOT / path)], capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
            print("PASS real-header syntax: " + path)


if __name__ == "__main__":
    main()
