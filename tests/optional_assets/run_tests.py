"""Exercise optional icon fallback and enforce the asset-only delivery boundary."""
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^[^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    assert match, name
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def icon_fallback():
    item = (ROOT / "soh/soh/Enhancements/randomizer/item.cpp").read_text()
    gui = (ROOT / "soh/soh/SohGui/ImGuiUtils.cpp").read_text()
    gui_header = (ROOT / "soh/soh/SohGui/ImGuiUtils.h").read_text()
    entry_type = re.search(r"typedef struct \{[^}]+\} ItemMapEntry;", gui_header).group(0)
    registration = re.search(r"    for \(const auto& entry : customItemsMapping\) \{.*?\n    \}",
                             function(gui, "RegisterImGuiItemIcons"), re.S).group(0)
    # Keep the actual entry type and registration body; replace only the GUI
    # service receiver with a recorder at the texture-loading boundary.
    registration, receivers = re.subn(
        r"(?:std::dynamic_pointer_cast<Fast::Fast3dGui>\(Ship::Context::GetRawInstance\(\)->GetWindow\(\)->GetGui\(\)\)|gui)\s*->",
        "recorder.", registration)
    assert receivers == 2
    helper = ROOT / "soh/soh/Enhancements/randomizer/OptionalNeiIcons.h"
    with tempfile.TemporaryDirectory(prefix="optional-nei-icons-") as tmp:
        directory = Path(tmp)
        (directory / "soh").mkdir()
        (directory / "soh/ResourceManagerHelpers.h").write_text('''#pragma once
#include <cstdint>
uint8_t ResourceMgr_FileExists(const char*);
uint8_t ResourceMgr_FileAltExists(const char*);
bool ResourceMgr_IsAltAssetsEnabled();
''')
        code = '''#include <cassert>
#include <cstring>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
static bool basePresent, altPresent, altEnabled;
uint8_t ResourceMgr_FileExists(const char*) { return basePresent; }
uint8_t ResourceMgr_FileAltExists(const char*) { return altPresent; }
bool ResourceMgr_IsAltAssetsEnabled() { return altEnabled; }
'''
        if helper.exists():
            code += '#include "' + str(helper) + '"\n'
        code += '''enum CustomIconSize { ICON_SIZE_24, ICON_SIZE_32 };
struct Item {
 const char* customIcon;
 CustomIconSize iconSize;
 const char* GetCustomIcon();
 CustomIconSize GetCustomIconSize();
};
'''
        code += function(item, "Item::GetCustomIcon") + '\n'
        code += function(item, "Item::GetCustomIconSize") + '\n'
        code += entry_type + '''
std::map<uint32_t, ItemMapEntry> customItemsMapping;
struct ImVec4 { float x, y, z, w; };
struct Load { std::string name, path; float alpha; };
struct Recorder {
 std::vector<Load> loads;
 void LoadGuiTexture(const std::string& name, const std::string& path,
                     const std::string&, ImVec4 color) {
   loads.push_back({name, path, color.w});
 }
} recorder;
void RegisterOptionalIcons() {
''' + registration + '\n}\n'
        code += '''int main() {
 const char* optional = "__OTR__textures/icon_item_static/gStatMagicTex";
 const char* native = "__OTR__textures/icon_item_24_static/gQuestIconMagicJarSmallTex";
 Item magic{optional, ICON_SIZE_32};
 customItemsMapping = {{0, {0, "magic", "magic_faded", optional}},
                       {1, {1, "ordinary", "ordinary_faded", "ordinary_path"}}};
 for (int bits = 0; bits < 8; ++bits) {
   basePresent = bits & 1; altPresent = bits & 2; altEnabled = bits & 4;
   const bool selected = basePresent || (altPresent && altEnabled);
   assert(std::strcmp(magic.GetCustomIcon(), selected ? optional : native) == 0);
   assert(magic.GetCustomIconSize() == (selected ? ICON_SIZE_32 : ICON_SIZE_24));
   recorder.loads.clear();
   RegisterOptionalIcons();
   assert(recorder.loads.size() == 4);
   assert(recorder.loads[0].name == "magic" && recorder.loads[0].alpha == 1.0f);
   assert(recorder.loads[1].name == "magic_faded" && recorder.loads[1].alpha == 0.3f);
   assert(recorder.loads[0].path == (selected ? optional : native));
   assert(recorder.loads[1].path == recorder.loads[0].path);
   assert(recorder.loads[2].path == "ordinary_path" && recorder.loads[3].path == "ordinary_path");
 }
 basePresent = altPresent = altEnabled = false;
 assert(std::strcmp(magic.GetCustomIcon(), native) == 0);
 Item ordinary{"ordinary", ICON_SIZE_32};
 assert(ordinary.GetCustomIcon() == ordinary.customIcon && ordinary.GetCustomIconSize() == ICON_SIZE_32);
 Item none{nullptr, ICON_SIZE_24};
 assert(none.GetCustomIcon() == nullptr && none.GetCustomIconSize() == ICON_SIZE_24);
}
'''
        source, binary = directory / "test.cpp", directory / "test"
        source.write_text(code)
        subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I", str(directory),
                        str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("Optional Magic icon: native 24px fallback, selected base/Alt 32px override, unload and unrelated items pass")
    print("Production GUI registration: actual string entry type, normal/faded fallback and unrelated textures pass")


def optional_layout():
    manifest = json.loads((ROOT / "tools/nei_icons/manifest.json").read_text())
    for host in ("soh", "mm"):
        for entry in manifest["icons"]:
            target = ROOT / host / "assets/custom" / entry["resource"]
            assert not target.exists(), (target, "HD icon is bundled in the game")
        assert not (ROOT / host / "assets/custom/objects/nei_gi_redesign/cojiro").exists(), \
            (host, "optional Cojiro is bundled in the game")
    print("Cojiro and HD icon replacement resources are absent from both built-in game asset trees")


if __name__ == "__main__":
    icon_fallback()
    optional_layout()
