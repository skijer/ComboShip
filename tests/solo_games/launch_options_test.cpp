#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include "combo/ComboLaunchOptions.h"
#include "combo/ComboLaunchPreferences.h"

int main() {
    using namespace ComboLaunch;
    auto parse = [](std::vector<std::string> values, Options& options) {
        std::vector<char*> args;
        for (auto& value : values) args.push_back(value.data());
        args.push_back(nullptr);
        int argc = static_cast<int>(values.size());
        options = ParseOptions(argc, args.data());
        std::vector<std::string> remaining;
        for (int i = 0; i < argc; ++i) remaining.emplace_back(args[i]);
        assert(args[argc] == nullptr);
        return remaining;
    };
    Options o;
    assert((parse({"Fleet", "--game=oot", "--rom-setup", "--other", "value"}, o) ==
            std::vector<std::string>{"Fleet", "--other", "value"}));
    assert(o.game == COMBO_GAME_OOT && o.explicitGame && o.romSetup && o.error.empty());
    parse({"Fleet", "--game", "mm", "--game-select"}, o);
    assert(o.game == COMBO_GAME_MM && o.chooseGame && o.error.empty());
    parse({"Fleet", "--game=combo"}, o);
    assert(o.game == COMBO_GAME_BOTH && o.explicitGame && !o.chooseGame && !o.romSetup);
    parse({"Fleet", "--game=nonsense"}, o);
    assert(!o.error.empty());
    parse({"Fleet", "--game"}, o);
    assert(!o.error.empty());
    parse({"Fleet", "--game=oot", "--game=mm"}, o);
    assert(!o.error.empty() && "contradictory mode flags must fail before boot");
    parse({"Fleet"}, o);
    assert(!o.explicitGame && o.game == COMBO_GAME_BOTH);

    const auto file = std::filesystem::path("launch-test.json");
    assert(LoadGame(file) == COMBO_GAME_BOTH);
    assert(SaveGame(file, COMBO_GAME_MM));
    assert(LoadGame(file) == COMBO_GAME_MM);
    assert(SaveGame(file, COMBO_GAME_OOT));
    assert(LoadGame(file) == COMBO_GAME_OOT);
    assert(!SaveGame(file, static_cast<ComboGameMode>(5)));
    assert(LoadGame(file) == COMBO_GAME_OOT);
    std::ofstream(file) << "{broken";
    assert(LoadGame(file) == COMBO_GAME_BOTH);
    std::ofstream(file) << R"({"game":123})";
    assert(LoadGame(file) == COMBO_GAME_BOTH);
    std::ofstream(file) << R"({"game":"not-a-game"})";
    assert(LoadGame(file) == COMBO_GAME_BOTH);
    assert(!SaveGame("missing-dir/launch.json", COMBO_GAME_OOT));
    std::filesystem::remove(file);
}
