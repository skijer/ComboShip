// The actual cache lookup/load decisions and game helpers are compiled below.
// Archive I/O/import and thread scheduling are replaced by deterministic fixtures.
#include <fast/resource/ResourceType.h>
#include <fast/resource/type/DisplayList.h>
#include <fast/resource/type/Texture.h>
#include <cstdio>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <variant>
#include <thread>

#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_TRACE(...) ((void)0)

static size_t trackedAllocations = 0;
static bool trackAllocations = false;
[[gnu::noinline]] void* operator new(std::size_t size) {
    if (trackAllocations)
        ++trackedAllocations;
    if (void* value = std::malloc(size))
        return value;
    throw std::bad_alloc();
}
[[gnu::noinline]] void operator delete(void* value) noexcept {
    std::free(value);
}
[[gnu::noinline]] void operator delete(void* value, std::size_t) noexcept {
    std::free(value);
}

namespace BS {
using priority_t = int;
namespace pr {
constexpr int highest = 0;
}
struct FixtureThreadPool {
    template <class F> auto submit_task(F callback, priority_t) {
        return std::async(std::launch::deferred, std::move(callback)).share();
    }
};
} // namespace BS

namespace Ship {
struct ResourceIdentifier {
    std::string Path;
    uintptr_t Owner = 0;
    std::shared_ptr<Archive> Parent;
    auto operator<=>(const ResourceIdentifier&) const = default;
};
class Archive {
  public:
    std::set<std::string> files;
    bool HasFile(const std::string& path) {
        return files.contains(path);
    }
};
using FixtureArchive = Archive;
struct FixtureLoader {
    std::shared_ptr<ResourceInitData> lastInitData;
    std::map<std::string, std::shared_ptr<IResource>> resources;
    std::shared_ptr<IResource> LoadResource(const std::string& path, std::shared_ptr<File>,
                                            std::shared_ptr<ResourceInitData> initData,
                                            std::shared_ptr<Archive> = nullptr) {
        lastInitData = initData;
        const auto found = resources.find(path);
        return found == resources.end() ? nullptr : found->second;
    }
};
class ResourceManager {
  public:
    enum class ResourceLoadError { None, NotCached, NotFound };
    using CacheLine = std::variant<ResourceLoadError, std::shared_ptr<IResource>>;
    std::map<ResourceIdentifier, CacheLine> mResourceCache;
    bool mAltAssetsEnabled = false;
    uintptr_t mDefaultCacheOwner = 0;
    std::shared_ptr<Archive> mDefaultCacheArchive;
    std::mutex mMutex;
    auto IsAltAssetsEnabled() const {
        return mAltAssetsEnabled;
    }
    std::shared_ptr<FixtureArchive> mArchiveManager = std::make_shared<FixtureArchive>();
    std::shared_ptr<FixtureLoader> loader = std::make_shared<FixtureLoader>();
    std::shared_ptr<BS::FixtureThreadPool> mThreadPool = std::make_shared<BS::FixtureThreadPool>();
    auto GetResourceLoader() {
        return loader;
    }
    bool OtrSignatureCheck(const char* path) {
        return path != nullptr && std::strncmp(path, "__OTR__", 7) == 0;
    }
    std::shared_ptr<File> LoadFileProcess(const std::string& path) {
        return mArchiveManager->HasFile(path) ? std::make_shared<File>() : nullptr;
    }
    std::shared_ptr<File> LoadFileProcess(const ResourceIdentifier& identifier) {
        auto archive = identifier.Parent ? identifier.Parent : mArchiveManager;
        return archive->HasFile(identifier.Path) ? std::make_shared<File>() : nullptr;
    }
    std::shared_ptr<IResource> LoadResourceProcess(const ResourceIdentifier&, bool = false,
                                                   std::shared_ptr<ResourceInitData> = nullptr);
    std::shared_ptr<IResource> LoadResourceProcess(const std::string&, bool = false,
                                                   std::shared_ptr<ResourceInitData> = nullptr);
    std::shared_future<std::shared_ptr<IResource>> LoadResourceAsync(const ResourceIdentifier&, bool, BS::priority_t,
                                                                     std::shared_ptr<ResourceInitData> = nullptr);
    std::shared_future<std::shared_ptr<IResource>> LoadResourceAsync(const std::string&, bool, BS::priority_t,
                                                                     std::shared_ptr<ResourceInitData> = nullptr);
    std::shared_ptr<IResource> LoadResource(const ResourceIdentifier&, bool = false,
                                            std::shared_ptr<ResourceInitData> = nullptr);
    std::shared_ptr<IResource> LoadResource(const std::string&, bool = false,
                                            std::shared_ptr<ResourceInitData> = nullptr);
    CacheLine CheckCache(const ResourceIdentifier&, bool);
    CacheLine CheckCache(const std::string&, bool);
    std::shared_ptr<IResource> GetCachedResource(const ResourceIdentifier&, bool = false);
    std::shared_ptr<IResource> GetCachedResource(const std::string&, bool = false);
    std::shared_ptr<IResource> GetCachedResource(CacheLine);
};
#include "alt_segment_cache.inc"

struct Context {
    std::shared_ptr<ResourceManager> manager = std::make_shared<ResourceManager>();
    static Context* GetRawInstance() {
        static Context context;
        return &context;
    }
    std::shared_ptr<ResourceManager> GetResourceManager() {
        return manager;
    }
};
} // namespace Ship

static std::set<std::string> ExtensionCache;
static bool ResourceMgr_IsGameMasterQuest() {
    return false;
}
static int ResourceMgr_OTRSigCheck(char* path) {
    return Ship::Context::GetRawInstance()->manager->OtrSignatureCheck(path);
}
#include "alt_segment_helpers.inc"
#include <fast/RenderResourceLookup.h>

static unsigned failures = 0;
#define REQUIRE(condition)                                                    \
    do {                                                                      \
        if (!(condition)) {                                                   \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
            ++failures;                                                       \
        }                                                                     \
    } while (0)

template <class T> static std::shared_ptr<T> Resource(const std::string& path, Fast::ResourceType type) {
    auto init = std::make_shared<Ship::ResourceInitData>();
    init->Path = path;
    init->Type = static_cast<uint32_t>(type);
    return std::make_shared<T>(init);
}

static uintptr_t Bind(const std::string& path, int segment = 8) {
    Gfx command{};
    gSPSegment(&command, segment, reinterpret_cast<uintptr_t>(path.c_str()));
    REQUIRE(command.words.w0 == (0xDB060000u | (segment * 4)));
    return command.words.w1;
}

static void CheckColdAltAfterSceneTravel() {
    // GameState_Destroy unloads alt/* while native entries can remain cached.
    // Every loading entry point must resolve the newly cold Alt override before
    // reusing a native display list or texture. Import/I/O and scheduling are
    // fixtures; travel is modeled by cache eviction. Cache, sync/async loader
    // and renderer shortcut decisions are production.
    for (bool texture : { false, true }) {
        const std::string name = texture ? "scenes/shared/spot00_scene/s00_yuka_01_78B00___Copie"
                                         : "scenes/shared/spot00_scene/spot00_room_0DL_002790";
        const std::string alt = "alt/" + name;
        const std::string tagged = "__OTR__" + name;
        for (bool alias : { false, true }) {
            for (int entry = 0; entry < 4; ++entry) {
                auto manager = std::make_shared<Ship::ResourceManager>();
                std::shared_ptr<Ship::IResource> native, authored;
                if (texture) {
                    native = Resource<Fast::Texture>(name, Fast::ResourceType::Texture);
                    authored = Resource<Fast::Texture>(alt, Fast::ResourceType::Texture);
                } else {
                    native = Resource<Fast::DisplayList>(name, Fast::ResourceType::DisplayList);
                    authored = Resource<Fast::DisplayList>(alt, Fast::ResourceType::DisplayList);
                }
                manager->loader->resources[name] = native;
                manager->loader->resources[alt] = authored;
                manager->mArchiveManager->files = { name, alt + (alias ? ".meta" : "") };
                REQUIRE(manager->LoadResource(name, true) == native);
                manager->mAltAssetsEnabled = true;
                for (int visit = 0; visit < 3; ++visit) {
                    std::shared_ptr<Ship::IResource> selected;
                    if (entry == 0)
                        selected = Fast::LoadRenderResource(*manager, tagged.c_str(), true, nullptr);
                    else if (entry == 1)
                        selected = manager->LoadResource(tagged);
                    else if (entry == 2)
                        selected = manager->LoadResourceAsync(tagged, false, BS::pr::highest, nullptr).get();
                    else
                        selected = manager->LoadResourceProcess(tagged);
                    REQUIRE(selected == authored && "cold Alt override lost to retained native cache after travel");
                    REQUIRE(manager->GetCachedResource(name, true) == native);
                    REQUIRE(manager->GetCachedResource(alt, true) == authored);
                    manager->mResourceCache.erase({ alt, 0, nullptr });
                }
                // Exact and Alt-off requests must still select the native asset.
                REQUIRE(manager->LoadResource(name, true) == native);
                manager->mAltAssetsEnabled = false;
                REQUIRE(manager->LoadResource(tagged) == native);
                REQUIRE(Fast::LoadRenderResource(*manager, tagged.c_str(), true, nullptr) == native);
            }
        }
    }
}

int main(int argc, char** argv) {
    // Match the runtime's shared_ptr atomic reference counting, not libc's single-thread shortcut.
    std::thread([] {}).join();
    CheckColdAltAfterSceneTravel();
    auto rm = Ship::Context::GetRawInstance()->manager;
    const std::string name = "objects/object_link_child/gLinkChildEyesOpenTex";
    const std::string path = "__OTR__" + name;
    const std::string alt = "alt/" + name;
    auto native = Resource<Fast::Texture>(name, Fast::ResourceType::Texture);
    auto material = Resource<Fast::DisplayList>(alt, Fast::ResourceType::DisplayList);
    material->Instructions = { gsSPEndDisplayList() };
    rm->loader->resources[name] = native;
    rm->loader->resources[alt] = material;
    rm->mArchiveManager->files = { name, alt }; // Supplied Din packs use a direct binary DL at the texture path.
    ExtensionCache.insert(alt);
    REQUIRE(rm->LoadResource(name) == native); // warm vanilla texture, cold Alt DL
    REQUIRE(rm->GetCachedResource(alt, true) == nullptr);
    REQUIRE(Bind(path) == reinterpret_cast<uintptr_t>(path.c_str()));
    rm->mAltAssetsEnabled = true;
    REQUIRE(Bind(path) == reinterpret_cast<uintptr_t>(material->Instructions.data()));
    REQUIRE(rm->GetCachedResource(name, true) == native); // Neither cache entry may be evicted.
    REQUIRE(rm->GetCachedResource(alt, true) == material);
    rm->mAltAssetsEnabled = false;
    REQUIRE(Bind(path) == reinterpret_cast<uintptr_t>(path.c_str()));
    rm->mAltAssetsEnabled = true;
    REQUIRE(Bind(path, 9) == reinterpret_cast<uintptr_t>(material->Instructions.data()));
    REQUIRE(ResourceMgr_LoadIfDListByName(name.c_str()) == reinterpret_cast<char*>(material->Instructions.data()));

    const auto checkOther = [&](const char* suffix, const std::shared_ptr<Ship::IResource>& replacement, bool alias,
                                bool enabled, Gfx* expected = nullptr) {
        const std::string base = name + suffix;
        const std::string tagged = "__OTR__" + base;
        const std::string selected = "alt/" + base;
        rm->mAltAssetsEnabled = false;
        auto original = Resource<Fast::Texture>(base, Fast::ResourceType::Texture);
        rm->loader->resources[base] = original;
        rm->mArchiveManager->files.insert(base);
        REQUIRE(rm->LoadResource(base) == original);
        if (replacement) {
            rm->loader->resources[selected] = replacement;
            rm->mArchiveManager->files.insert(selected + (alias ? ".meta" : ""));
            ExtensionCache.insert(selected);
        }
        rm->mAltAssetsEnabled = enabled;
        REQUIRE(Bind(tagged) ==
                (expected ? reinterpret_cast<uintptr_t>(expected) : reinterpret_cast<uintptr_t>(tagged.c_str())));
        REQUIRE(rm->GetCachedResource(base, true) == original);
    };
    checkOther("Alias", material, true, true, material->Instructions.data());
    checkOther("Missing", nullptr, false, true);
    checkOther("Disabled", material, false, false);
    checkOther("HD", Resource<Fast::Texture>("alt/hd", Fast::ResourceType::Texture), false, true);
    checkOther("Empty", Resource<Fast::DisplayList>("alt/empty", Fast::ResourceType::DisplayList), true, true);
    // A misleading metadata tag must not permit a Texture-to-DisplayList cast.
    checkOther("WrongType", Resource<Fast::Texture>("alt/wrong", Fast::ResourceType::DisplayList), true, true);
    REQUIRE(ResourceMgr_LoadIfDListByName("__OTR__objects/not_present") == nullptr);
    // Exercise the render shortcut against the production cache/load methods.
    Fast::RenderResourceLookupStats stats{};
    REQUIRE(Fast::LoadRenderResource(*rm, path.c_str(), true, &stats) == material);
    REQUIRE(stats.hits == 1 && stats.fallbacks == 0);
    REQUIRE(Fast::LoadRenderResource(*rm, path.c_str(), false, &stats) == material);
    REQUIRE(stats.hits == 1 && stats.fallbacks == 0); // disabled uses the original loader
    rm->mAltAssetsEnabled = false;
    REQUIRE(Fast::LoadRenderResource(*rm, path.c_str(), true, nullptr) == native);
    rm->mAltAssetsEnabled = true;
    auto replacement = Resource<Fast::DisplayList>(alt, Fast::ResourceType::DisplayList);
    rm->loader->resources[alt] = replacement;
    material->Dirty();
    REQUIRE(Fast::LoadRenderResource(*rm, path.c_str(), true, &stats) == replacement);
    REQUIRE(stats.fallbacks == 1); // dirty entries must reload, never return stale data
    rm->mResourceCache.erase({ alt, 0, nullptr });
    rm->mResourceCache.erase({ name, 0, nullptr });
    REQUIRE(Fast::LoadRenderResource(*rm, path.c_str(), true, &stats) == replacement);
    REQUIRE(stats.fallbacks == 2);
    REQUIRE(Fast::LoadRenderResource(*rm, "__OTR____OTR__objects/not_present", true, &stats) == nullptr);
    REQUIRE(stats.fallbacks == 3);
    auto foreign = std::make_shared<Ship::ResourceManager>();
    foreign->loader->resources[name] = native;
    foreign->mArchiveManager->files.insert(name);
    REQUIRE(Fast::LoadRenderResource(*foreign, path.c_str(), true, nullptr) == native);
    // Scope changes and externally replaced cache entries are visible on the very next lookup.
    rm->mDefaultCacheOwner = 7;
    rm->mResourceCache[{ alt, 7, nullptr }] = material;
    rm->mResourceCache[{ alt, 7, nullptr }] = native;
    REQUIRE(Fast::LoadRenderResource(*rm, path.c_str(), true, nullptr) == native);
    rm->mDefaultCacheOwner = 0;
    REQUIRE(Fast::LoadRenderResource(*rm, path.c_str(), true, nullptr) == replacement);
    rm->mAltAssetsEnabled = false;
    rm->mResourceCache[{ name, 0, nullptr }] = native;
    Ship::ResourceIdentifier warmIdentifier{ name, 0, nullptr };
    trackedAllocations = 0;
    trackAllocations = true;
    auto warmResult = rm->LoadResource(warmIdentifier);
    trackAllocations = false;
    REQUIRE(warmResult == native);
    if (!std::getenv("RESOURCE_BASELINE"))
        REQUIRE(trackedAllocations == 0); // Synchronous cache hits need no promise/future allocation.
    // Preserve signature/init-data behavior and archive-parent isolation.
    auto scoped = std::make_shared<Ship::ResourceManager>();
    scoped->loader->resources[name] = native;
    scoped->mArchiveManager->files.insert(name);
    auto initData = std::make_shared<Ship::ResourceInitData>();
    REQUIRE(scoped->LoadResource(name, true, initData) == native);
    REQUIRE(scoped->loader->lastInitData == initData);
    scoped->mResourceCache.clear();
    REQUIRE(scoped->LoadResource(path, true, initData) == native);
    REQUIRE(scoped->loader->lastInitData == nullptr); // Existing Async prefix contract.
    scoped->mResourceCache.clear();
    REQUIRE(scoped->LoadResourceAsync(path, true, BS::pr::highest, initData).get() == native);
    REQUIRE(scoped->loader->lastInitData == nullptr);
    auto parent = std::make_shared<Ship::Archive>();
    parent->files.insert(name);
    scoped->mDefaultCacheArchive = parent;
    scoped->mResourceCache[{ name, 0, parent }] = replacement;
    REQUIRE(scoped->LoadResource(path) == replacement);
    scoped->mDefaultCacheArchive.reset();
    REQUIRE(scoped->LoadResource(path) == native);
    scoped->mResourceCache[{ name, 0, nullptr }] = std::shared_ptr<Ship::IResource>{};
    REQUIRE(scoped->LoadResource(name) == native);
    scoped->mResourceCache[{ name, 0, nullptr }] = Ship::ResourceManager::ResourceLoadError::NotFound;
    REQUIRE(scoped->LoadResource(name) == native);
    if (std::getenv("RESOURCE_BENCHMARK")) {
        constexpr unsigned count = 500000;
        for (int mode : { 0, 1, 2 }) {
            const bool enabled = mode == 1;
            for (bool alternate : { false, true }) {
                rm->mAltAssetsEnabled = alternate;
                const auto begin = std::chrono::steady_clock::now();
                uintptr_t sum = 0;
                for (unsigned i = 0; i < count; ++i)
                    sum += reinterpret_cast<uintptr_t>(
                        (mode == 2 ? rm->LoadResourceProcess(name)
                                   : Fast::LoadRenderResource(*rm, path.c_str(), enabled, nullptr))
                            .get());
                const auto ns =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin)
                        .count();
                std::printf("BENCH mode=%d alt=%d ns/call=%.2f checksum=%zu\n", mode, alternate, double(ns) / count,
                            size_t(sum));
            }
        }
    }
    if (std::getenv("RESOURCE_BENCHMARK")) {
        for (bool alternate : { false, true }) {
            auto mixed = std::make_shared<Ship::ResourceManager>();
            mixed->mAltAssetsEnabled = alternate;
            mixed->loader->resources[name] = native;
            mixed->loader->resources[alt] = replacement;
            mixed->mArchiveManager->files = { name, alt };
            auto dirty = Resource<Fast::Texture>(name, Fast::ResourceType::Texture);
            dirty->Dirty();
            const auto begin = std::chrono::steady_clock::now();
            uintptr_t sum = 0;
            constexpr unsigned count = 100000;
            for (unsigned i = 0; i < count; ++i) {
                mixed->mDefaultCacheOwner = i % 2; // Scope switches on every lookup.
                if (i % 64 == 0)
                    mixed->mResourceCache.clear(); // Unload/reload.
                if (i % 64 == 16)
                    mixed->mResourceCache[{ alternate ? alt : name, i % 2, nullptr }] = dirty;
                const char* selected = i % 64 == 32 ? "objects/not_present" : path.c_str();
                sum += reinterpret_cast<uintptr_t>(mixed->LoadResource(selected).get());
            }
            const auto ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - begin).count();
            std::printf("BENCH mixed alt=%d ns/call=%.2f checksum=%zu\n", alternate, double(ns) / count, size_t(sum));
        }
    }
    if (failures)
        return 1;
    std::printf(
        "PASS %s Alt segment binding: cold direct/.meta DL over warm texture, on/off, retained caches, HD texture, "
        "missing/empty/wrong type; cold Alt after three visits, render lookup shortcut, dirty reload, manager and "
        "owner switching\n",
        argc > 1 ? argv[1] : "game");
}
