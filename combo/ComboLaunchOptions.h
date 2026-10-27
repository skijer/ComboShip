#pragma once

#include "ComboGameMode.h"
#include <string>

namespace ComboLaunch {

inline const char* GameName(ComboGameMode mode) {
    switch (mode) {
        case COMBO_GAME_OOT:
            return "oot";
        case COMBO_GAME_MM:
            return "mm";
        default:
            return "combo";
    }
}

inline bool ParseGame(const std::string& text, ComboGameMode& mode) {
    if (text == "combo")
        mode = COMBO_GAME_BOTH;
    else if (text == "oot")
        mode = COMBO_GAME_OOT;
    else if (text == "mm")
        mode = COMBO_GAME_MM;
    else
        return false;
    return true;
}

struct Options {
    ComboGameMode game = COMBO_GAME_BOTH;
    bool explicitGame = false;
    bool chooseGame = false;
    bool romSetup = false;
    std::string error;
};

// Consume launcher arguments only. Preserve native game arguments in order.
inline Options ParseOptions(int& argc, char** argv) {
    Options options;
    int kept = 1;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--rom-setup") {
            options.romSetup = true;
        } else if (arg == "--game-select") {
            options.chooseGame = true;
        } else if (arg == "--game" || arg.rfind("--game=", 0) == 0) {
            std::string value;
            if (arg == "--game") {
                if (i + 1 < argc)
                    value = argv[++i];
            } else {
                value = arg.substr(7);
            }
            ComboGameMode parsed = COMBO_GAME_BOTH;
            if (!ParseGame(value, parsed)) {
                options.error = "--game requires combo, oot, or mm";
            } else if (options.explicitGame && options.game != parsed) {
                options.error = "Choose only one --game mode";
            } else {
                options.game = parsed;
                options.explicitGame = true;
            }
        } else {
            argv[kept++] = argv[i];
        }
    }
    argc = kept;
    argv[argc] = nullptr;
    return options;
}

} // namespace ComboLaunch
