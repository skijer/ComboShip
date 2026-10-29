#include "GameModule.h"

#include <cstddef>

#include <ship/GameRegistry.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

std::filesystem::path ModulePath(const std::filesystem::path& installDirectory, const std::string& baseName) {
#if defined(_WIN32)
    return installDirectory / (baseName + ".dll");
#elif defined(__APPLE__)
    return installDirectory / ("lib" + baseName + ".dylib");
#else
    return installDirectory / ("lib" + baseName + ".so");
#endif
}

// The game's own dependencies resolve from its folder; libultraship is already loaded, so it binds to Fleet's.
void* OpenModule(const std::filesystem::path& path) {
#ifdef _WIN32
    return LoadLibraryExW(path.wstring().c_str(), nullptr,
                          LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
#else
    return dlopen(path.string().c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}

void* FindSymbol(void* handle, const char* name) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), name));
#else
    return dlsym(handle, name);
#endif
}

void CloseModule(void* handle) {
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

} // namespace

std::unique_ptr<GameModule> GameModule::Load(const std::filesystem::path& installDirectory,
                                             const std::string& moduleBaseName, const FleetHostApi& host) {
    const std::filesystem::path path = ModulePath(installDirectory, moduleBaseName);
    void* handle = OpenModule(path);
    if (handle == nullptr) {
        SPDLOG_ERROR("[Fleet] Could not load {}", path.string());
        return nullptr;
    }
    auto getGameApi = reinterpret_cast<FleetGetGameApiFn>(FindSymbol(handle, FLEET_GET_GAME_API_SYMBOL));
    const FleetGameApi* api = getGameApi != nullptr ? getGameApi(FLEET_GAME_API_MAJOR) : nullptr;
    if (api == nullptr) {
        SPDLOG_ERROR("[Fleet] {} offers no Fleet contract {}.x", path.string(), FLEET_GAME_API_MAJOR);
        CloseModule(handle);
        return nullptr;
    }
    std::unique_ptr<GameModule> game(new GameModule(handle, api, installDirectory.string()));
    game->mAttachInfo = { .structSize = sizeof(FleetAttachInfo),
                          .host = &host,
                          .installDirectory = game->mInstallDirectory.c_str(),
                          .saveDirectory = nullptr };
    if (!api->Attach(&game->mAttachInfo)) {
        SPDLOG_ERROR("[Fleet] {} refused to attach", path.string());
        return nullptr;
    }
    SPDLOG_INFO("[Fleet] Attached {} ({}.{}) from {}", api->name, api->major, api->minor, path.string());
    return game;
}

GameModule::GameModule(void* handle, const FleetGameApi* api, std::string installDirectory)
    : mHandle(handle), mApi(api), mInstallDirectory(std::move(installDirectory)), mAttachInfo() {
}

// The module stays mapped until Fleet exits: its threads may still run on the shared libultraship.
GameModule::~GameModule() {
    mApi->Detach();
}

const FleetGameApi& GameModule::Api() const {
    return *mApi;
}

const char* GameModule::Name() const {
    return mApi->name;
}

std::string GameModule::TakeString(char* text) const {
    if (text == nullptr) {
        return "";
    }
    std::string copy = text;
    mApi->FreeString(text);
    return copy;
}

std::string GameModule::Request(const std::string& op, const std::string& json) const {
    // RandoRequest arrived in 1.1; an older game simply has none of these services.
    if (mApi->structSize <= offsetof(FleetGameApi, RandoRequest) || mApi->RandoRequest == nullptr) {
        return "";
    }
    Ship::GameScope scope(Name());
    return TakeString(mApi->RandoRequest(op.c_str(), json.c_str()));
}
