#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <fleet/FleetGameApi.h>

// The game's services, looked up by name when it loads. Any of them is null when the game does not offer it.
struct GameFunctions {
    FleetNameFn Name = nullptr;
    FleetAttachFn Attach = nullptr;
    FleetRunFn Run = nullptr;
    FleetArriveFn Arrive = nullptr;
    FleetDetachFn Detach = nullptr;
    FleetFreeStringFn FreeString = nullptr;
    FleetDumpRandoStaticDataFn DumpRandoStaticData = nullptr;
    FleetRandoResetFn RandoReset = nullptr;
    FleetRandoSetOwnedItemsFn RandoSetOwnedItems = nullptr;
    FleetRandoGetReachableChecksFn RandoGetReachableChecks = nullptr;
    FleetRandoPlaceItemFn RandoPlaceItem = nullptr;
    FleetRandoRequestFn RandoRequest = nullptr;
};

// One game's shared module, loaded from that game's own install folder and attached to Fleet.
class GameModule {
  public:
    // Null when the module is missing, offers no Fleet services, or refuses to attach.
    static std::unique_ptr<GameModule> Load(const std::filesystem::path& installDirectory,
                                            const std::string& moduleBaseName, FleetGetFunctionFn hostFunction);
    ~GameModule();
    GameModule(const GameModule&) = delete;
    GameModule& operator=(const GameModule&) = delete;

    const GameFunctions& Functions() const;
    // The GameRegistry name; every call into the game must run in a Ship::GameScope of it.
    const char* Name() const;
    // Copies a string the game returned and hands it back to the game. Empty for null.
    std::string TakeString(char* text) const;
    // The game's reply to a RandoRequest, or an empty string when it has no such service.
    std::string Request(const std::string& op, const std::string& json = "{}") const;

  private:
    GameModule(void* handle, GameFunctions functions, std::string installDirectory);

    void* mHandle;
    GameFunctions mFunctions;
    std::string mInstallDirectory;
};
