"""Exercise the production post-archive GUI font selection at service boundaries."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    source = (ROOT / "mm/2s2h/BenPort.cpp").read_text()
    name = "OTRGlobals::LoadModGuiFonts"
    # The pre-fix control reaches the assertions using the existing no-op behavior.
    body = function(source, name) if name + "(" in source else "void OTRGlobals::LoadModGuiFonts() {}"
    fixture = r'''
#include <cstdio>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
struct ImFont { int value; } fonts[20];
namespace ImGui {
struct IO { ImFont* FontDefault = &fonts[0]; } io;
IO& GetIO() { return io; }
}
struct Archive {
    std::set<std::string> files;
    bool HasFile(const std::string& p) { return files.count(p); }
};
struct Resources {
    std::shared_ptr<Archive> archive = std::make_shared<Archive>();
    auto GetArchiveManager() { return archive; }
};
struct Overlay {
    std::vector<std::string> paths;
    void LoadFont(const std::string&, float, const std::string& path) { paths.push_back(path); }
};
struct Gui {
    int rebuilds = 0;
    std::shared_ptr<Overlay> overlay = std::make_shared<Overlay>();
    auto GetGameOverlay() { return overlay; }
    void RebuildFontTexture() { ++rebuilds; }
};
struct Window {
    std::shared_ptr<Gui> gui = std::make_shared<Gui>();
    auto GetGui() { return gui; }
};
struct Context {
    std::shared_ptr<Resources> rm = std::make_shared<Resources>();
    std::shared_ptr<Window> window = std::make_shared<Window>();
    auto GetResourceManager() { return rm; }
    auto GetWindow() { return window; }
};
struct OTRGlobals {
    Context* context;
    ImFont *fontStandard=&fonts[0], *fontStandardLarger=&fonts[0], *fontStandardLargest=&fonts[0];
    ImFont *fontMono=&fonts[0], *fontMonoLarger=&fonts[0], *fontMonoLargest=&fonts[0];
    std::vector<std::pair<float, std::string>> loads;
    ImFont* CreateFontWithSize(float size, const std::string& path) {
        loads.emplace_back(size, path); return &fonts[loads.size()];
    }
    void LoadModGuiFonts();
};
'''
    checks = r'''
int main() {
    int failures = 0, cases = 0;
    for (int standard : {0, 1}) for (int mono : {0, 1}) {
        Context context;
        OTRGlobals app; app.context = &context;
        ImGui::io.FontDefault = &fonts[0];
        if (standard) context.rm->archive->files.insert("fonts/mods/standard.ttf");
        if (mono) context.rm->archive->files.insert("fonts/mods/mono.ttf");
        app.LoadModGuiFonts();
        bool good = app.loads.size() == static_cast<size_t>(3 * (standard + mono));
        good &= (app.fontStandard != &fonts[0]) == static_cast<bool>(standard);
        good &= (app.fontMono != &fonts[0]) == static_cast<bool>(mono);
        good &= ImGui::io.FontDefault == (standard ? app.fontStandardLarger : &fonts[0]);
        good &= context.window->gui->rebuilds == (standard || mono ? 1 : 0);
        good &= context.window->gui->overlay->paths.size() == static_cast<size_t>(standard || mono ? 2 : 0);
        for (size_t i=0; i<app.loads.size(); ++i) {
            good &= app.loads[i].first == 16.0f + 4.0f * static_cast<float>(i % 3);
            good &= context.rm->archive->files.count(app.loads[i].second) != 0;
        }
        for (const auto& path : context.window->gui->overlay->paths)
            good &= context.rm->archive->files.count(path) != 0;
        ++cases;
        if (!good) { ++failures; std::printf("FAIL standard=%d mono=%d\n", standard, mono); }
    }
    std::printf("%s mod GUI font selection: %d configurations, %d failures\n", failures ? "FAIL" : "PASS", cases, failures);
    return failures != 0;
}
'''
    with tempfile.TemporaryDirectory(prefix="mm-mod-font-") as tmp:
        directory = Path(tmp)
        cpp = directory / "mod_font.cpp"
        cpp.write_text(fixture + body + checks)
        binary = directory / "mod_font_test"
        subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    init = function(source, "InitOTR")
    assert init.index("ModMenu_LoadArchives();") < init.index("OTRGlobals::Instance->LoadModGuiFonts();")
    assert init.index("OTRGlobals::Instance->LoadModGuiFonts();") < init.index("BenGui::SetupGuiElements();")
    assert "void LoadModGuiFonts();" in (ROOT / "mm/2s2h/BenPort.h").read_text()
    print("PASS font load occurs after mod selection and before GUI setup")


if __name__ == "__main__":
    main()
