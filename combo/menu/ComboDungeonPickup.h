#pragma once
#include <string_view>

namespace ComboDungeonPickup {
// Exact catalog names also cover old seeds/plando without category metadata.
inline bool IsMapOrCompass(std::string_view name) {
    constexpr std::string_view dungeons[] = {
        "Great Deku Tree", "Dodongo's Cavern", "Jabu-Jabu's Belly", "Forest Temple",      "Fire Temple",
        "Water Temple",    "Spirit Temple",    "Shadow Temple",     "Bottom of the Well", "Ice Cavern",
        "Woodfall",        "Snowhead",         "Great Bay",         "Stone Tower",
    };
    for (const auto dungeon : dungeons) {
        if (name.size() > dungeon.size() && name.substr(0, dungeon.size()) == dungeon &&
            (name.substr(dungeon.size()) == " Map" || name.substr(dungeon.size()) == " Compass"))
            return true;
    }
    return false;
}
} // namespace ComboDungeonPickup
