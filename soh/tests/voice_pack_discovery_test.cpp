// Filesystem discovery is production code; the archive parser/audio/CVars are fixtures.
#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>
using s32 = int;
static const std::string appShortName = "soh";
static std::string fixtureMods;
#define VP_LOG(...) ((void)0)
namespace Ship {
struct Context {
    static Context* GetRawInstance() {
        static Context c;
        return &c;
    }
    static std::string LocateFileAcrossAppDirs(const char*, const std::string&) {
        return fixtureMods;
    }
    Context* GetWindow() {
        return this;
    }
    Context* GetGui() {
        return this;
    }
    void SaveConsoleVariablesNextFrame() {
    }
};
} // namespace Ship
struct VoicePack {
    std::string path, displayName;
    std::vector<int> oggEntryByHex;
};
static bool sInitialized = false;
static constexpr int VOICE_SLOT_COUNT = 4;
struct Slot {
    std::atomic<int> playing;
    void* data;
    int len;
};
static Slot sSlots[VOICE_SLOT_COUNT];
static std::vector<VoicePack> sPacks;
static std::set<std::string> sClaimedPaths;
static int CVarGetInteger(const char*, int fallback) {
    return fallback;
}
static void CVarSetInteger(const char*, int) {
}
static void VoicePack_Select(int) {
}
static bool ScanOnePak(const std::string& name, VoicePack& pack) {
    if (std::filesystem::path(name).stem() == "not-a-voice")
        return false;
    pack.path = name;
    return true;
}
#include "voice_init.inc"
int main(int argc, char** argv) {
    fixtureMods = argv[1];
    const std::filesystem::path root = fixtureMods;
    const std::vector<std::string> files = { "root.pak",
                                             "uppercase.PAK",
                                             "soh/child.pak",
                                             "soh/voices/adult.pak",
                                             "voices/mixed.PaK",
                                             "2ship/mm-only.pak",
                                             "nested/2ship/mm-only.pak",
                                             "ignored.txt",
                                             "not-a-voice.pak" };
    for (const auto& name : files) {
        const auto path = root / name;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream(path) << "fixture";
    }
    VoicePack_Init();
    std::set<std::string> found;
    for (const auto& pack : sPacks)
        found.insert(std::filesystem::relative(pack.path, root).generic_string());
    const std::set<std::string> expected = { "root.pak", "uppercase.PAK", "soh/child.pak", "soh/voices/adult.pak",
                                             "voices/mixed.PaK" };
    if (found != expected) {
        for (const auto& name : expected)
            if (!found.count(name))
                std::fprintf(stderr, "FAIL missing voice candidate: %s\n", name.c_str());
        for (const auto& name : found)
            if (!expected.count(name))
                std::fprintf(stderr, "FAIL unexpected voice candidate: %s\n", name.c_str());
        return 1;
    }
    VoicePack_Init();
    if (sPacks.size() != expected.size() || sClaimedPaths.size() != expected.size())
        return 1;
    std::puts("PASS voice discovery: root/nested/game folders, mixed-case extension, sibling exclusion, parser "
              "rejection, no duplicate init");
}
