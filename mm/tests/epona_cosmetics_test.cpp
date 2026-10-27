// Engine/archive services are boundaries; the MM wrapper, shared DL builder,
// masks, native command streams and real MM Skin/Gfx structures are production.
#include <array>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "EponaCosmeticsDL.h"
#include "EponaCosmeticMasks.h"
#include "EponaCosmeticsNativeTemplates.h"
#include "test_require.h"
#include <ship/utils/StrHash64.h>
extern "C" {
#include "global.h"
}

namespace Ship {
struct IResource {
    virtual ~IResource() = default;
    struct InitData {
        bool IsCustom = false;
    } init;
    InitData* GetInitData() {
        return &init;
    }
};
static std::string activeGame = "oot";
struct OwnRMScope {
    std::string previous = activeGame;
    explicit OwnRMScope(const char* game) {
        activeGame = game;
    }
    ~OwnRMScope() {
        activeGame = previous;
    }
};
struct ArchiveManager {
    std::map<std::string, std::shared_ptr<IResource>> files;
    bool HasFile(const std::string& path) {
        return files.contains(path);
    }
};
struct ResourceManager {
    std::map<std::string, std::shared_ptr<IResource>> resources;
    ArchiveManager archive;
    bool altAssetsEnabled = false;
    unsigned loadCalls = 0;
    bool OtrSignatureCheck(const char* path) {
        return std::strncmp(path, "__OTR__", 7) == 0;
    }
    bool IsAltAssetsEnabled() {
        return altAssetsEnabled;
    }
    ArchiveManager* GetArchiveManager() {
        return &archive;
    }
    std::shared_ptr<IResource> GetCachedResource(const std::string& path, bool loadExact = false) {
        REQUIRE(activeGame == "mm");
        // Match the host cache: an unloaded Alt falls through to cached native data.
        if (!loadExact && altAssetsEnabled && !path.starts_with("alt/")) {
            auto alt = resources.find("alt/" + path);
            if (alt != resources.end())
                return alt->second;
        }
        auto i = resources.find(path);
        return i == resources.end() ? nullptr : i->second;
    }
    std::shared_ptr<IResource> LoadResource(const std::string& path, bool loadExact = false) {
        ++loadCalls;
        auto load = [&](const std::string& resolved) -> std::shared_ptr<IResource> {
            if (auto cached = GetCachedResource(resolved, true))
                return cached;
            auto file = archive.files.find(resolved);
            if (file == archive.files.end())
                file = archive.files.find(resolved + ".meta");
            if (file == archive.files.end())
                return nullptr;
            return resources[resolved] = file->second;
        };
        if (!loadExact && altAssetsEnabled && !path.starts_with("alt/")) {
            if (auto alt = load("alt/" + path))
                return alt;
        }
        return load(path);
    }
};
struct Context {
    ResourceManager rm;
    static Context* GetRawInstance() {
        static Context context;
        return &context;
    }
    ResourceManager* GetResourceManager() {
        return &rm;
    }
};
} // namespace Ship
namespace Fast {
struct DisplayList : Ship::IResource {
    std::vector<Gfx> Instructions;
};
} // namespace Fast

static std::map<std::string, int32_t> integers;
static std::map<std::string, Color_RGBA8> colors;
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
    auto i = integers.find(name);
    return i == integers.end() ? fallback : i->second;
}
extern "C" Color_RGBA8 CVarGetColor(const char* name, Color_RGBA8 fallback) {
    auto i = colors.find(name);
    return i == colors.end() ? fallback : i->second;
}
extern "C" void* Lib_SegmentedToVirtual(void* address) {
    return address;
}
extern "C" void gSPSegment(void* packet, int segment, uintptr_t target) {
    *static_cast<Gfx*>(packet) = gsSPSegment(segment, target);
}
// Frame interpolation instrumentation is not part of the material policy.
#undef OPEN_DISPS
#undef CLOSE_DISPS
#define OPEN_DISPS(context) \
    {                       \
        GraphicsContext* __gfxCtx = (context)
#define CLOSE_DISPS(context) }

#include "mm_epona_production.inc"

static Gfx Command(uintptr_t w0, uintptr_t w1) {
    Gfx command{};
    command.words.w0 = w0;
    command.words.w1 = w1;
    return command;
}
#include "mm_epona_native.inc"

static Gfx frame[32768];
static GraphicsContext graphics;
static PlayState play;
static void Frame() {
    graphics.polyOpa.p = frame;
    graphics.polyOpa.d = frame + std::size(frame);
    play.state.gfxCtx = &graphics;
}

int main() {
    LoadNativeDisplayLists(Ship::Context::GetRawInstance()->rm);
    auto head = std::dynamic_pointer_cast<Fast::DisplayList>(Ship::Context::GetRawInstance()->rm.resources.at(
        "objects/object_horse_link_child/object_horse_link_child_DL_000C70"));
    const auto original = head->Instructions;
    char headName[] = "__OTR__objects/object_horse_link_child/object_horse_link_child_DL_000C70";
    SkinLimb headLimb{};
    headLimb.segmentType = SKIN_LIMB_TYPE_NORMAL;
    headLimb.segment = headName;
    SkinLimb* limbs[] = { &headLimb };
    SkeletonHeader skeleton{};
    skeleton.segment = reinterpret_cast<void**>(limbs);
    skeleton.limbCount = 1;
    Skin skin{};
    skin.skeletonHeader = &skeleton;

    Frame();
    REQUIRE(!MMEponaCosmetics_BeginDraw(&play, &skin));
    REQUIRE(headLimb.segment == headName && graphics.polyOpa.p == frame && sDisplayListCopies.empty());

    const auto eye = ResolveMaterial(0, true);
    REQUIRE(eye.targets.size() == 3 && eye.targets[2].inverseMasks[2] != nullptr);
    for (unsigned i = 0; i < 512; ++i)
        REQUIRE(eye.targets[2].inverseMasks[2][i] == 1);
#ifdef MM_EPONA_NATIVE_FIXTURE
    // Run all seven independently selectable part combinations on every native MM leaf.
    for (unsigned changed = 1; changed < 8; ++changed) {
        Ship::OwnRMScope scope("mm");
        for (const auto* leaf : kNativeLists) {
            const std::string path = "__OTR__" + std::string(kTexturePrefix) + leaf;
            REQUIRE(GetNativeListCopy(reinterpret_cast<Gfx*>(const_cast<char*>(path.c_str())), changed) != nullptr);
        }
    }
    integers[kChanged[0]] = integers[kChanged[1]] = integers[kChanged[2]] = 1;
    const auto copies = sDisplayListCopies.size(), masks = sMasks.size();
    void* cached = nullptr;
    for (unsigned tick = 0; tick < 120; ++tick) {
        Frame();
        colors[kColors[0]] = { static_cast<uint8_t>(tick), 117, 203, 255 };
        REQUIRE(MMEponaCosmetics_BeginDraw(&play, &skin));
        REQUIRE(Ship::activeGame == "oot"); // MM RM scope was restored on return
        REQUIRE(headLimb.segment != headName);
        if (cached)
            REQUIRE(headLimb.segment == cached);
        cached = headLimb.segment;
        REQUIRE(graphics.polyOpa.p - frame == 5); // RM push and four tiny color segments
        REQUIRE(frame[0].words.w0 >> 24 == G_COMBO_RM_PUSH);
        auto* coatColor = reinterpret_cast<Gfx*>(frame[1].words.w1);
        REQUIRE(coatColor[1].words.w1 == ((static_cast<uint32_t>(tick) << 24) | 0x0075CBFF));
        MMEponaCosmetics_EndDraw(&play, &skin);
        REQUIRE(headLimb.segment == headName && sDrawingSkin == nullptr && sDrawSwaps.empty());
        REQUIRE((graphics.polyOpa.p - 2)->words.w0 >> 24 == G_SETGRAYSCALE);
        REQUIRE((graphics.polyOpa.p - 2)->words.w1 == 0);
        REQUIRE((graphics.polyOpa.p - 1)->words.w0 >> 24 == G_COMBO_RM_POP);
        REQUIRE(sDisplayListCopies.size() == copies && sMasks.size() == masks);
    }
    REQUIRE(std::memcmp(original.data(), head->Instructions.data(), original.size() * sizeof(Gfx)) == 0);
    REQUIRE(Ship::Context::GetRawInstance()->rm.loadCalls == 0); // Cache hits do not create load futures.
    // A same-path binary replacement does not necessarily have IsCustom set.
    // Reject a larger texture footprint before registering a fixed-size mask.
    auto altered = std::make_shared<Fast::DisplayList>();
    altered->Instructions = original;
    for (auto& command : altered->Instructions) {
        if (command.words.w0 >> 24 == G_LOADBLOCK) {
            command.words.w1 |= 0x00FFF000;
            break;
        }
    }
    Ship::Context::GetRawInstance()->rm.resources[std::string(kTexturePrefix) + kNativeLists[0]] = altered;
    Frame();
    REQUIRE(!MMEponaCosmetics_BeginDraw(&play, &skin));
    REQUIRE(headLimb.segment == headName && graphics.polyOpa.p == frame);
    Ship::Context::GetRawInstance()->rm.resources[std::string(kTexturePrefix) + kNativeLists[0]] = head;
#else
    // Hermetic CI deliberately supplies a same-path, noncustom binary leaf that
    // is not the authenticated native stream. It must remain completely untouched.
    integers[kChanged[0]] = 1;
    REQUIRE(!MMEponaCosmetics_BeginDraw(&play, &skin));
    REQUIRE(headLimb.segment == headName && graphics.polyOpa.p == frame);
    REQUIRE(std::memcmp(original.data(), head->Instructions.data(), original.size() * sizeof(Gfx)) == 0);
#endif
    // Enabling Alt Assets must discover an uncached geometry replacement even
    // while the native list and its cosmetic variants remain cached. Alias-only
    // replacements have no file at altPath, only an altPath.meta entry.
    auto& rm = Ship::Context::GetRawInstance()->rm;
    const std::string nativePath = std::string(kTexturePrefix) + kNativeLists[0];
    const std::string altPath = "alt/" + nativePath;
    for (bool aliasOnly : { false, true }) {
        auto customAlt = std::make_shared<Fast::DisplayList>();
        customAlt->init.IsCustom = true;
        customAlt->Instructions = original;
        rm.archive.files[altPath + (aliasOnly ? ".meta" : "")] = customAlt;
        rm.altAssetsEnabled = true;
        const auto loads = rm.loadCalls;
        Frame();
        const bool began = MMEponaCosmetics_BeginDraw(&play, &skin);
        REQUIRE(rm.loadCalls == loads + 1);
        REQUIRE(!began && headLimb.segment == headName && graphics.polyOpa.p == frame);
        REQUIRE(rm.resources.at(altPath) == customAlt);
        REQUIRE(Ship::activeGame == "oot");
        REQUIRE(!MMEponaCosmetics_BeginDraw(&play, &skin));
        REQUIRE(rm.loadCalls == loads + 1); // The custom Alt remains cached too.
        rm.altAssetsEnabled = false;
#ifdef MM_EPONA_NATIVE_FIXTURE
        REQUIRE(MMEponaCosmetics_BeginDraw(&play, &skin));
        MMEponaCosmetics_EndDraw(&play, &skin);
#else
        REQUIRE(!MMEponaCosmetics_BeginDraw(&play, &skin));
#endif
        REQUIRE(headLimb.segment == headName && rm.loadCalls == loads + 1);
        rm.resources.erase(altPath);
        rm.archive.files.clear();
    }
    integers.clear();
    Frame();
    REQUIRE(!MMEponaCosmetics_BeginDraw(&play, &skin));
    REQUIRE(headLimb.segment == headName && graphics.polyOpa.p == frame);
    integers[kChanged[0]] = 1;
    head->init.IsCustom = true;
    REQUIRE(!MMEponaCosmetics_BeginDraw(&play, &skin));
    head->init.IsCustom = false;
    headLimb.segment = reinterpret_cast<void*>(0x08000001);
    REQUIRE(!MMEponaCosmetics_BeginDraw(&play, &skin));
    REQUIRE(headLimb.segment == reinterpret_cast<void*>(0x08000001));
#ifdef MM_EPONA_NATIVE_FIXTURE
    puts("PASS MM Epona: all 11 native DLs / 7 part combinations, 120 cached rainbow draws, no load futures");
#endif
    puts("PASS MM Epona: defaults/reset, blink masks, custom/segmented fallback, Alt files/aliases and game isolation");
}
