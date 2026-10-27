#pragma once

#include <string>
#include <string_view>

namespace ComboDungeonKeyReceipt {
struct Style {
    const char* color = nullptr;
    bool isSmallKey = false;
};

// Canonical receipt colors, independent of the receiving game and inventory.
// OoT follows its randomizer catalog; MM follows its native dungeon palette.
inline Style Find(std::string_view name) {
    enum Kind { Small = 1, Boss = 2, Ring = 4 };
    constexpr struct {
        std::string_view suffix;
        int kind;
    } kinds[] = { { " Small Key", Small }, { " Boss Key", Boss }, { " Key Ring", Ring } };
    constexpr struct {
        std::string_view name;
        const char* color;
        int kinds;
    } dungeons[] = {
        { "Forest Temple", "%g", Small | Boss | Ring },
        { "Fire Temple", "%r", Small | Boss | Ring },
        { "Water Temple", "%b", Small | Boss | Ring },
        { "Spirit Temple", "%y", Small | Boss | Ring },
        { "Shadow Temple", "%p", Small | Boss | Ring },
        { "Bottom of the Well", "%p", Small | Ring },
        { "Training Ground", "%y", Small | Ring },
        { "Gerudo Fortress", "%y", Small | Ring },
        { "Ganon's Castle", "%r", Small | Boss | Ring },
        { "Chest Game", "%g", Small | Ring },
        { "Woodfall", "%p", Small | Boss },
        { "Snowhead", "%g", Small | Boss },
        { "Great Bay", "%b", Small | Boss },
        { "Stone Tower", "%y", Small | Boss },
    };
    for (const auto& kind : kinds) {
        if (name.size() <= kind.suffix.size() || name.substr(name.size() - kind.suffix.size()) != kind.suffix)
            continue;
        const auto dungeonName = name.substr(0, name.size() - kind.suffix.size());
        for (const auto& dungeon : dungeons)
            if (dungeon.name == dungeonName && (dungeon.kinds & kind.kind))
                return { dungeon.color, kind.kind == Small };
    }
    return {};
}

inline std::string Markup(std::string_view name) {
    const auto style = Find(name);
    if (!style.color)
        return {};
    std::string body = style.isSmallKey ? "You found a " : "You found the ";
    body += style.color;
    body.append(name);
    body += "%w!";
    return body;
}
} // namespace ComboDungeonKeyReceipt
