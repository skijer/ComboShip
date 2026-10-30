#include "GameModule.h"

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

GameFunctions LookUpFunctions(FleetGetFunctionFn gameFunction) {
    const auto find = [gameFunction]<class Function>(Function& slot, const char* name) {
        slot = reinterpret_cast<Function>(gameFunction(name));
    };
    GameFunctions functions;
    find(functions.Name, "Name");
    find(functions.Attach, "Attach");
    find(functions.Run, "Run");
    find(functions.Arrive, "Arrive");
    find(functions.Detach, "Detach");
    find(functions.FreeString, "FreeString");
    find(functions.DumpRandoStaticData, "DumpRandoStaticData");
    find(functions.RandoReset, "RandoReset");
    find(functions.RandoSetOwnedItems, "RandoSetOwnedItems");
    find(functions.RandoGetReachableChecks, "RandoGetReachableChecks");
    find(functions.RandoPlaceItem, "RandoPlaceItem");
    find(functions.RandoRequest, "RandoRequest");
    return functions;
}

} // namespace

std::unique_ptr<GameModule> GameModule::Load(const std::filesystem::path& relativeInstallDirectory,
                                             const std::string& moduleBaseName, FleetGetFunctionFn hostFunction) {
    // Searching the module's own folder for its dependencies needs an absolute path.
    const std::filesystem::path installDirectory = std::filesystem::absolute(relativeInstallDirectory);
    const std::filesystem::path path = ModulePath(installDirectory, moduleBaseName);
    void* handle = OpenModule(path);
    if (handle == nullptr) {
        SPDLOG_ERROR("[Fleet] Could not load {}", path.string());
        return nullptr;
    }
    auto gameFunction = reinterpret_cast<FleetGetFunctionFn>(FindSymbol(handle, FLEET_GET_FUNCTION_SYMBOL));
    const GameFunctions functions = gameFunction != nullptr ? LookUpFunctions(gameFunction) : GameFunctions{};
    if (functions.Name == nullptr || functions.Attach == nullptr) {
        SPDLOG_ERROR("[Fleet] {} offers no Fleet services", path.string());
        CloseModule(handle);
        return nullptr;
    }
    std::unique_ptr<GameModule> game(new GameModule(handle, functions, installDirectory.string()));
    const FleetAttachInfo attach = { .hostFunction = hostFunction,
                                     .installDirectory = game->mInstallDirectory.c_str() };
    if (!functions.Attach(&attach)) {
        SPDLOG_ERROR("[Fleet] {} refused to attach", path.string());
        return nullptr;
    }
    SPDLOG_INFO("[Fleet] Attached {} from {}", functions.Name(), path.string());
    return game;
}

GameModule::GameModule(void* handle, GameFunctions functions, std::string installDirectory)
    : mHandle(handle), mFunctions(functions), mInstallDirectory(std::move(installDirectory)) {
}

// The module stays mapped until Fleet exits: its threads may still run on the shared libultraship.
GameModule::~GameModule() {
    if (mFunctions.Detach != nullptr) {
        mFunctions.Detach();
    }
}

const GameFunctions& GameModule::Functions() const {
    return mFunctions;
}

const char* GameModule::Name() const {
    return mFunctions.Name();
}

std::string GameModule::TakeString(char* text) const {
    if (text == nullptr) {
        return "";
    }
    std::string copy = text;
    mFunctions.FreeString(text);
    return copy;
}

std::string GameModule::Request(const std::string& op, const std::string& json) const {
    if (mFunctions.RandoRequest == nullptr) {
        return "";
    }
    Ship::GameScope scope(Name());
    return TakeString(mFunctions.RandoRequest(op.c_str(), json.c_str()));
}
