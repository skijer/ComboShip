#!/usr/bin/env python3
"""Exercise the native cosmetic loops' visible effect-family boundaries.

The real group renderers control filtering and separators. Color-row rendering
and buttons are fixtures so no game or graphics context is required.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(source, signature):
    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
                    lambda match: re.sub(r'[^\n]', ' ', match[0]), source, flags=re.S)
    start = source.index(signature)
    end = masked.index("{", start) + 1
    depth = 1
    while depth:
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    return source[start:end]


FIXTURE = r'''
#include <iostream>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "gui/CosmeticGrouping.h"
struct ImVec2 { ImVec2(float, float) {} };
struct Size { float x = 1; };
namespace Fixture {
std::vector<std::string> rows;
bool advanced = false;
int failures = 0;
void Check(bool condition, const char* why) {
    if (!condition) { ++failures; std::cerr << "FAIL: " << why << '\n'; }
}
void Expect(std::initializer_list<const char*> expected, const char* why) {
    std::vector<std::string> wanted;
    for (auto row : expected) wanted.emplace_back(row);
    Check(rows == wanted, why);
    if (rows != wanted) {
        for (auto& row : rows) std::cerr << row << '\n';
    }
    rows.clear();
}
}
#define CVAR_COSMETIC(key) "gCosmetics." key
int CVarGetInteger(const char*, int fallback) { return Fixture::advanced || fallback; }
namespace ImGui {
void Text(const char*, const char*) {}
void SameLine(float = 0) {}
Size CalcTextSize(const char*) { return {}; }
}
namespace UIWidgets {
struct ButtonOptions {
    ButtonOptions& Size(ImVec2) { return *this; }
    ButtonOptions& Padding(ImVec2) { return *this; }
    ButtonOptions& Color(int) { return *this; }
};
bool Button(const char*, const ButtonOptions&) { return false; }
void Spacer() {}
void Separator(bool, bool, float, float) { Fixture::rows.emplace_back("|"); }
}
constexpr int THEME_COLOR = 0;
enum CosmeticGroup {
    COSMETICS_GROUP_MAGIC, COSMETICS_GROUP_ARROWS, COSMETICS_GROUP_SPIN_ATTACK,
    COSMETICS_GROUP_TRAILS, COSMETICS_GROUP_EFFECTS, COSMETICS_GROUP_LINK
};
struct CosmeticOption {
    std::string id;
    CosmeticGroup group;
    bool advancedOption = false;
    bool suppressed = false;
    const char* lockedCvar = "unused";
};

/* HOSTS */

int main() {
    using namespace Fixture;
    // The literal expectations catch missing boundaries and accidental pair splits.
    Oot::cosmeticOptions = {
        {"Magic.MedallionFirePrimary", {"Fire primary", COSMETICS_GROUP_MAGIC}},
        {"Magic.MedallionFireSecondary", {"Fire secondary", COSMETICS_GROUP_MAGIC, true}},
        {"Magic.MedallionForestPrimary", {"Forest primary", COSMETICS_GROUP_MAGIC}},
        {"Magic.MedallionForestSecondary", {"Forest secondary", COSMETICS_GROUP_MAGIC, true}},
        {"Magic.MedallionWaterPrimary", {"Water primary", COSMETICS_GROUP_MAGIC, true}},
    };
    advanced = true;
    Oot::DrawCosmeticGroup(COSMETICS_GROUP_MAGIC);
    Expect({"Fire primary", "Fire secondary", "|", "Forest primary", "Forest secondary", "|", "Water primary", "|"},
           "OoT advanced mode keeps pairs intact and divides families");
    advanced = false;
    Oot::DrawCosmeticGroup(COSMETICS_GROUP_MAGIC);
    Expect({"Fire primary", "|", "Forest primary", "|"},
           "OoT hidden advanced rows do not create stray separators");
    Oot::cosmeticOptions = {
        {"Custom.DinFireShieldCore", {"Shield core", COSMETICS_GROUP_MAGIC}},
        {"Custom.DinFireShieldOuter", {"Shield outer", COSMETICS_GROUP_MAGIC}},
        {"Custom.DinFireSwordCore", {"Sword core", COSMETICS_GROUP_MAGIC}},
        {"Custom.DinFireSwordOuter", {"Sword outer", COSMETICS_GROUP_MAGIC}},
        {"Custom.ZoraMagicShield", {"Barrier", COSMETICS_GROUP_MAGIC}},
        {"Custom.ZoraMagicShieldGlow", {"Glow", COSMETICS_GROUP_MAGIC}},
        {"Custom.ZoraMagicShieldHighlights", {"Highlights", COSMETICS_GROUP_MAGIC}},
    };
    Oot::DrawCosmeticGroup(COSMETICS_GROUP_MAGIC);
    Expect({"Shield core", "Shield outer", "|", "Sword core", "Sword outer", "|", "Barrier", "Glow", "Highlights", "|"},
           "core/outer pairs and the Zora barrier trio share their family");
    Mm::cosmeticOptions = {
        {"Arrows.MedallionFireSecondary", {"Fire secondary", COSMETICS_GROUP_EFFECTS}},
        {"Arrows.MedallionForestPrimary", {"Forest primary", COSMETICS_GROUP_EFFECTS, false, true}},
        {"Arrows.MedallionForestSecondary", {"Forest secondary", COSMETICS_GROUP_EFFECTS, false, true}},
        {"Effects.FireArrowPrim", {"Arrow primary", COSMETICS_GROUP_EFFECTS}},
        {"Effects.FireArrowSec", {"Arrow secondary", COSMETICS_GROUP_EFFECTS}},
        {"Effects.GreatSpinBurst", {"Spin burst", COSMETICS_GROUP_EFFECTS}},
        {"Effects.GreatSpinCharge", {"Spin charge", COSMETICS_GROUP_EFFECTS}},
    };
    Mm::CosmeticEditorDrawGroup(COSMETICS_GROUP_EFFECTS);
    Expect({"Fire secondary", "|", "Arrow primary", "Arrow secondary", "|", "Spin burst", "Spin charge", "|"},
           "MM preserves sorted pairs and skips fully suppressed families");
    Mm::cosmeticOptions = {
        {"Trails.KokiriSwordTrail", {"Kokiri", COSMETICS_GROUP_TRAILS}},
        {"Trails.ZoraFinTrail", {"Zora", COSMETICS_GROUP_TRAILS}},
    };
    Mm::CosmeticEditorDrawGroup(COSMETICS_GROUP_TRAILS);
    Expect({"Kokiri", "|", "Zora", "|"}, "single trail families remain separate");
    Mm::cosmeticOptions.begin()->second.suppressed = true;
    Mm::CosmeticEditorDrawGroup(COSMETICS_GROUP_TRAILS);
    Expect({"Zora", "|"}, "the first visible family has no leading divider");
    Mm::cosmeticOptions.rbegin()->second.suppressed = true;
    Mm::CosmeticEditorDrawGroup(COSMETICS_GROUP_TRAILS);
    Expect({}, "an entirely suppressed MM group draws nothing");
    Oot::cosmeticOptions = {
        {"Link.GoronTunic", {"Goron", COSMETICS_GROUP_LINK}},
        {"Link.KokiriTunic", {"Kokiri", COSMETICS_GROUP_LINK}},
    };
    Oot::DrawCosmeticGroup(COSMETICS_GROUP_LINK);
    Expect({"Goron", "Kokiri", "|"}, "ordinary color groups retain their existing layout");
    if (failures) return 1;
    std::cout << "PASS native OoT/MM visible effect families: advanced filtering, suppression, sorted pairs, core/outer, charge/burst, Zora trio and trails\n";
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-rev", help="run older production loops as a negative control")
    args = parser.parse_args()
    hosts = []
    for namespace, path, signature in (
        ("Oot", "soh/soh/Enhancements/cosmetics/CosmeticsEditor.cpp", "void DrawCosmeticGroup("),
        ("Mm", "mm/2s2h/BenGui/CosmeticEditor.cpp", "void CosmeticEditorDrawGroup("),
    ):
        source = subprocess.check_output(["git", "show", args.source_rev + ":" + path], cwd=ROOT, text=True) \
            if args.source_rev else (ROOT / path).read_text()
        hosts.append("namespace " + namespace + r''' {
std::map<std::string, CosmeticOption> cosmeticOptions;
const std::map<CosmeticGroup, const char*> groupLabels = {
    {COSMETICS_GROUP_MAGIC, "Magic"}, {COSMETICS_GROUP_ARROWS, "Arrows"},
    {COSMETICS_GROUP_SPIN_ATTACK, "Spin"}, {COSMETICS_GROUP_TRAILS, "Trails"},
    {COSMETICS_GROUP_EFFECTS, "Effects"}, {COSMETICS_GROUP_LINK, "Link"}
};
const auto& sCosmeticGroupLabels = groupLabels;
void DrawCosmeticRow(CosmeticOption& option) { Fixture::rows.push_back(option.id); }
void CosmeticEditorDrawRow(CosmeticOption& option) { Fixture::rows.push_back(option.id); }
void ApplyOrResetCustomGfxPatches() {}
void RandomizeColor(CosmeticOption&) {}
void ResetColor(CosmeticOption&) {}
void CosmeticEditorRandomizeGroup(CosmeticGroup) {}
void CosmeticEditorResetGroup(CosmeticGroup) {}
bool CosmeticEditorMatchesGroup(const CosmeticOption& option, CosmeticGroup group) { return option.group == group; }
bool CosmeticEditorIsSuppressed(const CosmeticOption& option) { return option.suppressed; }
''' + block(source, signature) + "\n}\n")
    with tempfile.TemporaryDirectory(prefix="cosmetics-ui-") as temporary:
        directory = Path(temporary)
        unit = directory / "groups.cpp"
        unit.write_text(FIXTURE.replace("/* HOSTS */", "\n".join(hosts)))
        binary = directory / "groups"
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-I" + str(ROOT / "combo"), str(unit), "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)]).returncode)


if __name__ == "__main__":
    main()
