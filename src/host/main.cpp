// fleet: plays Ship of Harkinian and 2 Ship 2 Harkinian in one window, one game at a time.

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <SDL2/SDL.h>
#include <fast/Fast3dWindow.h>
#include <nlohmann/json.hpp>
#include <ship/Context.h>
#include <ship/GameRegistry.h>
#include <libultraship/controller/controldeck/ControlDeck.h>
#include <ship/debug/Console.h>

#include "GameFiber.h"
#include "GameModule.h"

#ifdef _WIN32
#include <Windows.h>
#endif

// Both games extend the N64 pad with the same bits.
#define BTN_CUSTOM_MODIFIER1 0x0040
#define BTN_CUSTOM_MODIFIER2 0x0080
#define BTN_CUSTOM_OCARINA_NOTE_D4 ((CONTROLLERBUTTONS_T)0x00010000)
#define BTN_CUSTOM_OCARINA_NOTE_F4 ((CONTROLLERBUTTONS_T)0x00020000)
#define BTN_CUSTOM_OCARINA_NOTE_A4 ((CONTROLLERBUTTONS_T)0x00040000)
#define BTN_CUSTOM_OCARINA_NOTE_B4 ((CONTROLLERBUTTONS_T)0x00080000)
#define BTN_CUSTOM_OCARINA_NOTE_D5 ((CONTROLLERBUTTONS_T)0x00100000)
#define BTN_CUSTOM_OCARINA_DISABLE_SONGS ((CONTROLLERBUTTONS_T)0x00200000)
#define BTN_CUSTOM_OCARINA_PITCH_UP ((CONTROLLERBUTTONS_T)0x00400000)
#define BTN_CUSTOM_OCARINA_PITCH_DOWN ((CONTROLLERBUTTONS_T)0x00800000)

namespace {

enum GameIndex { Oot, Mm, GameCount };

struct HostedGame {
    std::unique_ptr<GameModule> module;
    std::unique_ptr<GameFiber> fiber;
    std::string arrivalPayload;
    FleetDestination arrival = {};
};

struct Options {
    std::string ootDirectory;
    std::string mmDirectory;
    GameIndex start = Oot;
    // Smoke test: switch games every this many frames and quit after a few round trips.
    int smokeFrames = 0;
};

constexpr int kSmokeSwitches = 4;

std::array<HostedGame, GameCount> sGames;
GameIndex sActive = Oot;
std::optional<GameIndex> sPendingSwitch;
int sSmokeFrames = 0;
std::array<int, GameCount> sFramesRun = {};
int sSmokeSwitchesDone = 0;

GameIndex IndexOf(const char* game) {
    return std::string(game) == "mm" ? Mm : Oot;
}

GameIndex Other(GameIndex game) {
    return game == Oot ? Mm : Oot;
}

// What the games reported while attaching; each line already names the file and folder at fault.
std::vector<std::string> sAttachProblems;

void LogFromGame(const char* game, FleetLogLevel level, const char* message) {
    std::cerr << "[" << game << "] " << message << "\n";
    if (level >= FLEET_LOG_ERROR) {
        sAttachProblems.push_back(message);
    }
}

void RequestSwitch(const char* fromGame, const FleetDestination* destination) {
    const GameIndex target = destination != nullptr && destination->game != nullptr ? IndexOf(destination->game)
                                                                                    : Other(IndexOf(fromGame));
    HostedGame& game = sGames[target];
    game.arrivalPayload = destination != nullptr && destination->payloadJson != nullptr ? destination->payloadJson : "";
    game.arrival = { .game = game.module->Name(),
                     .entrance = destination != nullptr ? destination->entrance : 0,
                     .payloadJson = game.arrivalPayload.empty() ? nullptr : game.arrivalPayload.c_str() };
    sPendingSwitch = target;
}

// The first activation boots the game inside its fiber; later ones resume it where it parked.
void Activate(GameIndex target) {
    HostedGame& game = sGames[target];
    sActive = target;
    if (game.fiber == nullptr) {
        game.fiber = std::make_unique<GameFiber>([&game] { game.module->Api().Run(&game.arrival); });
    } else {
        Ship::GameRegistry::SetActive(game.module->Name());
    }
    game.fiber->SwitchTo();
}

void EndProcess(int status);

void CountSmokeFrame(GameIndex game) {
    if (sSmokeFrames <= 0 || ++sFramesRun[game] % sSmokeFrames != 0) {
        return;
    }
    std::cout << "[fleet] " << sGames[game].module->Name() << " ran " << sFramesRun[game] << " frames" << std::endl;
    if (sSmokeSwitchesDone++ == kSmokeSwitches) {
        std::cout << "[fleet] smoke test passed" << std::endl;
        EndProcess(0);
    }
    const FleetDestination destination = { .game = sGames[Other(game)].module->Name() };
    RequestSwitch(sGames[game].module->Name(), &destination);
}

// Runs inside the game that just finished a frame: a pending switch parks it here until it is active again.
void FrameEnd(const char* game) {
    CountSmokeFrame(IndexOf(game));
    if (!sPendingSwitch || *sPendingSwitch == IndexOf(game)) {
        sPendingSwitch.reset();
        return;
    }
    const GameIndex target = *sPendingSwitch;
    sPendingSwitch.reset();
    Activate(target);
    Ship::GameRegistry::SetActive(game);
}

const FleetHostApi sHost = { .structSize = sizeof(FleetHostApi),
                             .major = FLEET_GAME_API_MAJOR,
                             .minor = FLEET_GAME_API_MINOR,
                             .Log = LogFromGame,
                             .RequestSwitch = RequestSwitch,
                             .FrameEnd = FrameEnd };

// A double-clicked fleet.exe finds the games through fleet.json beside it; relative paths start there.
void LoadInstallFile(Options& options) {
    char* basePath = SDL_GetBasePath();
    const std::filesystem::path folder = basePath != nullptr ? basePath : ".";
    SDL_free(basePath);
    std::ifstream file(folder / "fleet.json");
    const nlohmann::json config = nlohmann::json::parse(file, nullptr, false);
    if (!config.is_object()) {
        return;
    }
    for (auto [key, directory] : { std::pair{ "oot", &options.ootDirectory }, std::pair{ "mm", &options.mmDirectory } }) {
        const std::string path = config.value(key, "");
        if (!path.empty()) {
            *directory = (folder / path).lexically_normal().string();
        }
    }
}

bool ParseOptions(int argc, char* argv[], Options& options) {
    LoadInstallFile(options);
    for (int i = 1; i + 1 < argc; i += 2) {
        const std::string flag = argv[i];
        const std::string value = argv[i + 1];
        if (flag == "--oot") {
            options.ootDirectory = value;
        } else if (flag == "--mm") {
            options.mmDirectory = value;
        } else if (flag == "--start" && (value == "oot" || value == "mm")) {
            options.start = value == "mm" ? Mm : Oot;
        } else if (flag == "--smoke") {
            options.smokeFrames = std::atoi(value.c_str());
        } else {
            return false;
        }
    }
    return !options.ootDirectory.empty() && !options.mmDirectory.empty();
}

void CreateHostContext() {
    Ship::Context* context = Ship::Context::CreateUninitializedInstance("Fleet", "fleet", "fleetship.json");
    context->InitLogging();
    context->InitConfiguration();
    context->InitConsoleVariables();
}

// Each game checks its own install as it attaches; Fleet never extracts a ROM on a game's behalf.
bool AttachGames(const Options& options) {
    const std::array<std::pair<std::string, const char*>, GameCount> installs = {
        { { options.ootDirectory, "soh" }, { options.mmDirectory, "2ship" } }
    };
    for (int game = 0; game < GameCount; game++) {
        sGames[game].module = GameModule::Load(installs[game].first, installs[game].second, sHost);
        if (sGames[game].module == nullptr && sAttachProblems.empty()) {
            sAttachProblems.push_back(std::string(installs[game].second) + " could not be loaded from " +
                                      installs[game].first + ".");
        }
    }
    if (sAttachProblems.empty()) {
        return true;
    }
    std::string text;
    for (const std::string& problem : sAttachProblems) {
        text += problem + "\n\n";
    }
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fleet cannot start", text.c_str(), nullptr);
    return false;
}

// Fleet owns the window, the pad, the console and the shared shaders; each game joins with its own data.
void OpenWindow(const Options& options) {
    Ship::Context* context = Ship::Context::GetRawInstance();
    context->InitControlDeck(std::make_shared<LUS::ControlDeck>(std::vector<CONTROLLERBUTTONS_T>({
        BTN_CUSTOM_MODIFIER1,
        BTN_CUSTOM_MODIFIER2,
        BTN_CUSTOM_OCARINA_NOTE_D4,
        BTN_CUSTOM_OCARINA_NOTE_F4,
        BTN_CUSTOM_OCARINA_NOTE_A4,
        BTN_CUSTOM_OCARINA_NOTE_B4,
        BTN_CUSTOM_OCARINA_NOTE_D5,
        BTN_CUSTOM_OCARINA_DISABLE_SONGS,
        BTN_CUSTOM_OCARINA_PITCH_UP,
        BTN_CUSTOM_OCARINA_PITCH_DOWN,
    })));
    const std::string shaders = (std::filesystem::path(options.ootDirectory) / "soh.o2r").string();
    context->InitResourceManager({ shaders }, {}, 3, true);
    context->InitConsole();
    context->InitWindow(std::make_shared<Fast::Fast3dWindow>(std::vector<std::shared_ptr<Ship::GuiWindow>>{}));
}

void RegisterCommands() {
    Ship::Context::GetRawInstance()->GetConsole()->AddCommand(
        "fleet_switch", { [](std::shared_ptr<Ship::Console>, const std::vector<std::string>&, std::string*) {
                             const FleetDestination destination = { .game = sGames[Other(sActive)].module->Name() };
                             RequestSwitch(sGames[sActive].module->Name(), &destination);
                             return 0;
                         },
                          "Parks the running game and continues in the other one" });
}

int RunFleet(int argc, char* argv[]) {
    Options options;
    if (!ParseOptions(argc, argv, options)) {
        const char* usage = "Fleet needs your Ship of Harkinian and 2 Ship 2 Harkinian folders.\n\n"
                            "Set them in fleet.json next to fleet.exe:\n"
                            "{ \"oot\": \"../Ship\", \"mm\": \"../2Ship\" }\n\n"
                            "or pass --oot <folder> --mm <folder> [--start oot|mm] [--smoke frames].";
        std::cerr << usage << "\n";
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fleet", usage, nullptr);
        return 2;
    }
    sSmokeFrames = options.smokeFrames;
    CreateHostContext();
    if (!AttachGames(options)) {
        return 2;
    }
    OpenWindow(options);
    RegisterCommands();
    GameFiber::EnterMainThread();
    // Control comes back here only once a game's loop ends, which means the window closed.
    Activate(options.start);
    return 0;
}

// The parked game never unwinds its loop; ending the process is its teardown, as it is for fleet-seedgen.
void EndProcess(int status) {
    std::cout.flush();
    std::cerr.flush();
#ifdef _WIN32
    TerminateProcess(GetCurrentProcess(), static_cast<UINT>(status));
#endif
    std::_Exit(status);
}

} // namespace

int main(int argc, char* argv[]) {
    EndProcess(RunFleet(argc, argv));
}
