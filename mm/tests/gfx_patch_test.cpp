#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
#include <unordered_map>

#include "fast/resource/type/DisplayList.h"

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
            std::exit(1);                                                     \
        }                                                                     \
    } while (0)

static std::shared_ptr<Ship::IResource> activeResource;
static int warnings = 0;
template <class... Args> static void RecordWarning(const char*, Args&&...) {
    ++warnings;
}
#define SPDLOG_WARN(...) RecordWarning(__VA_ARGS__)

namespace Ship {
class ResourceManager {
  public:
    std::shared_ptr<IResource> LoadResource(const char*) {
        return activeResource;
    }
};
class Context {
  public:
    static Context* GetRawInstance() {
        static Context context;
        return &context;
    }
    std::shared_ptr<ResourceManager> GetResourceManager() {
        static auto manager = std::make_shared<ResourceManager>();
        return manager;
    }
};
} // namespace Ship

#include "gfx_patch_production.inc"

static constexpr const char* kPath = "objects/object_link_goron/gLinkGoronWaistDL";
static Gfx colorDl[] = { gsDPSetPrimColor(0, 0, 0, 0, 0, 0), gsDPPipeSync(), gsSPEndDisplayList() };
static Gfx colorCall = gsSPDisplayList(colorDl);

static Gfx Command(uintptr_t value) {
    Gfx instruction = {};
    instruction.words.w0 = value;
    instruction.words.w1 = value + 1;
    return instruction;
}

static bool Equal(const Gfx& a, const Gfx& b) {
    return a.words.w0 == b.words.w0 && a.words.w1 == b.words.w1;
}

static std::shared_ptr<Fast::DisplayList> List(size_t size, uintptr_t value = 100, bool custom = false) {
    auto metadata = std::make_shared<Ship::ResourceInitData>();
    metadata->Path = kPath;
    metadata->IsCustom = custom;
    auto resource = std::make_shared<Fast::DisplayList>(metadata);
    resource->Instructions.assign(size, Command(value));
    activeResource = resource;
    return resource;
}

static void ShortUnmarkedList() {
    // The real Goron-waist cosmetic patch writes index 16. A native-name mod
    // with IsCustom=false and fewer commands must never be indexed there.
    auto resource = List(16);
    ResourceMgr_PatchGfxByName(kPath, "setPrim", 16, colorCall);
    CHECK(ResourceMgr_GetPatchCountForDL(kPath) == 0);
    CHECK(Equal(resource->Instructions.back(), Command(100)));
}

static void InvalidIndices() {
    for (int index : { -1, 0, 16, 17, std::numeric_limits<int>::max() }) {
        auto resource = List(index == 0 ? 0 : 16);
        ResourceMgr_PatchGfxByName(kPath, "invalid", index, colorCall);
        CHECK(ResourceMgr_GetPatchCountForDL(kPath) == 0);
    }
}

static void ValidPatch() {
    auto resource = List(18);
    ResourceMgr_PatchGfxByName(kPath, "setPrim", 16, colorCall);
    ResourceMgr_PatchGfxByName(kPath, "setPrim", 16, Command(200));
    CHECK(ResourceMgr_GetPatchCountForDL(kPath) == 1);
    CHECK(Equal(resource->Instructions[16], Command(200)));
    ResourceMgr_UnpatchGfxByName(kPath, "setPrim");
    CHECK(Equal(resource->Instructions[16], Command(100)));
    CHECK(ResourceMgr_GetPatchCountForDL(kPath) == 0);
    ResourceMgr_PatchGfxByName(kPath, "last", 17, colorCall);
    CHECK(Equal(resource->Instructions[17], colorCall));
    ResourceMgr_ResetAllPatchesForDL(kPath);
    CHECK(Equal(resource->Instructions[17], Command(100)));
}

static void CopyBounds() {
    auto resource = List(18);
    resource->Instructions[0] = Command(400);
    for (int index : { -1, 18, std::numeric_limits<int>::max() }) {
        ResourceMgr_PatchGfxCopyCommandByName(kPath, "source", 17, index);
        ResourceMgr_PatchGfxCopyCommandByName(kPath, "destination", index, 0);
        CHECK(ResourceMgr_GetPatchCountForDL(kPath) == 0);
        CHECK(Equal(resource->Instructions[17], Command(100)));
    }
    ResourceMgr_PatchGfxCopyCommandByName(kPath, "copy", 17, 0);
    CHECK(Equal(resource->Instructions[17], Command(400)));
    ResourceMgr_UnpatchGfxByName(kPath, "copy");
    CHECK(Equal(resource->Instructions[17], Command(100)));
}

static void CustomList() {
    auto resource = List(18, 100, true);
    ResourceMgr_PatchGfxByName(kPath, "setPrim", 16, colorCall);
    ResourceMgr_PatchGfxCopyCommandByName(kPath, "copy", 17, 0);
    CHECK(ResourceMgr_GetPatchCountForDL(kPath) == 0);
    CHECK(Equal(resource->Instructions[16], Command(100)));
}

class OtherResource final : public Ship::Resource<int> {
  public:
    using Resource::Resource;
    int* GetPointer() override {
        return nullptr;
    }
    size_t GetPointerSize() override {
        return 0;
    }
};

static void MissingOrWrongResource() {
    for (auto resource : { std::shared_ptr<Ship::IResource>(),
                           std::shared_ptr<Ship::IResource>(
                               std::make_shared<OtherResource>(std::make_shared<Ship::ResourceInitData>())),
                           std::shared_ptr<Ship::IResource>(std::make_shared<Fast::DisplayList>()) }) {
        activeResource = resource;
        ResourceMgr_PatchGfxByName(kPath, "missing", 16, colorCall);
        ResourceMgr_PatchGfxCopyCommandByName(kPath, "missing", 17, 0);
        CHECK(ResourceMgr_GetPatchCountForDL(kPath) == 0);
    }
}

static void ReloadBeforeRestore(bool reset, bool shortList) {
    auto original = List(18);
    ResourceMgr_PatchGfxByName(kPath, "setPrim", 16, colorCall);
    auto replacement = List(shortList ? 1 : 18, 500);
    if (reset) {
        ResourceMgr_ResetAllPatchesForDL(kPath);
    } else {
        ResourceMgr_UnpatchGfxByName(kPath, "setPrim");
    }
    CHECK(ResourceMgr_GetPatchCountForDL(kPath) == 0);
    CHECK(Equal(replacement->Instructions.back(), Command(500)));
    if (!shortList) {
        CHECK(Equal(replacement->Instructions[16], Command(500)));
    }
    CHECK(Equal(original->Instructions[16], Command(100)));
}

static void RepatchReplacement() {
    auto original = List(18);
    ResourceMgr_PatchGfxByName(kPath, "setPrim", 16, colorCall);
    auto replacement = List(18, 500);
    ResourceMgr_PatchGfxByName(kPath, "setPrim", 16, colorCall);
    CHECK(Equal(original->Instructions[16], Command(100)));
    ResourceMgr_UnpatchGfxByName(kPath, "setPrim");
    CHECK(Equal(replacement->Instructions[16], Command(500)));
}

static void RestoreAfterResize() {
    auto resource = List(18);
    ResourceMgr_PatchGfxByName(kPath, "setPrim", 16, colorCall);
    std::vector<Gfx>(1, Command(500)).swap(resource->Instructions);
    ResourceMgr_UnpatchGfxByName(kPath, "setPrim");
    CHECK(Equal(resource->Instructions[0], Command(500)));
    CHECK(ResourceMgr_GetPatchCountForDL(kPath) == 0);
}

static void MissingBeforeRestore() {
    List(18);
    ResourceMgr_PatchGfxByName(kPath, "setPrim", 16, colorCall);
    activeResource.reset();
    ResourceMgr_UnpatchGfxByName(kPath, "setPrim");
    CHECK(ResourceMgr_GetPatchCountForDL(kPath) == 0);
}

int main(int argc, char** argv) {
    const std::string selected = argc > 1 ? argv[1] : "all";
    const std::pair<const char*, void (*)()> cases[] = {
        { "short-unmarked", ShortUnmarkedList },
        { "invalid-indices", InvalidIndices },
        { "valid-patch", ValidPatch },
        { "copy-bounds", CopyBounds },
        { "custom-list", CustomList },
        { "missing-or-wrong", MissingOrWrongResource },
        { "unpatch-short-reload", []() { ReloadBeforeRestore(false, true); } },
        { "unpatch-same-size-reload", []() { ReloadBeforeRestore(false, false); } },
        { "reset-short-reload", []() { ReloadBeforeRestore(true, true); } },
        { "reset-same-size-reload", []() { ReloadBeforeRestore(true, false); } },
        { "repatch-replacement", RepatchReplacement },
        { "restore-after-resize", RestoreAfterResize },
        { "missing-before-restore", MissingBeforeRestore },
    };
    int count = 0;
    for (auto& [name, test] : cases) {
        if (selected != "all" && selected != name) {
            continue;
        }
        originalGfx.clear();
        activeResource.reset();
        test();
        std::printf("PASS %s\n", name);
        ++count;
    }
    CHECK(count != 0);
    std::printf("PASS MM Gfx patch bridge: %d cases\n", count);
}
