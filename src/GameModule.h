#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include <fleet/FleetGameApi.h>

// One game's shared module, loaded from that game's own install folder and attached to Fleet.
class GameModule {
  public:
    // Null when the module is missing, exports no Fleet_GetGameApi of our major, or refuses to attach.
    static std::unique_ptr<GameModule> Load(const std::filesystem::path& installDirectory,
                                            const std::string& moduleBaseName, const FleetHostApi& host);
    ~GameModule();
    GameModule(const GameModule&) = delete;
    GameModule& operator=(const GameModule&) = delete;

    const FleetGameApi& Api() const;
    // The GameRegistry name; every call into the game must run in a Ship::GameScope of it.
    const char* Name() const;
    // Copies a string the game returned and hands it back to the game. Empty for null.
    std::string TakeString(char* text) const;
    // The game's reply to a RandoRequest, or an empty string when it has no such service.
    std::string Request(const std::string& op, const std::string& json = "{}") const;

  private:
    GameModule(void* handle, const FleetGameApi* api, std::string installDirectory);

    void* mHandle;
    const FleetGameApi* mApi;
    std::string mInstallDirectory;
    FleetAttachInfo mAttachInfo;
};
