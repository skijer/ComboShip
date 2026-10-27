#pragma once

#include "ComboLaunchOptions.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace ComboLaunch {

inline ComboGameMode LoadGame(const std::filesystem::path& path) {
    try {
        std::ifstream stream(path);
        if (!stream)
            return COMBO_GAME_BOTH;
        const auto json = nlohmann::json::parse(stream);
        ComboGameMode mode = COMBO_GAME_BOTH;
        if (json.is_object() && json.contains("game") && json["game"].is_string()) {
            ParseGame(json["game"].get<std::string>(), mode);
        }
        return mode;
    } catch (...) { return COMBO_GAME_BOTH; }
}

inline bool SaveGame(const std::filesystem::path& path, ComboGameMode mode) {
    if (mode < COMBO_GAME_BOTH || mode > COMBO_GAME_MM)
        return false;
    try {
        std::ofstream stream(path);
        stream << nlohmann::json{ { "game", GameName(mode) } }.dump(2) << '\n';
        stream.close();
        return !stream.fail();
    } catch (...) { return false; }
}

} // namespace ComboLaunch
