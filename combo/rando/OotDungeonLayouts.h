#pragma once

#include <nlohmann/json.hpp>

namespace ComboRando {
using OotLayoutSetter = int (*)(const char*);
using OotLayoutDumper = const char* (*)();

// Keep resolved layouts separate from editable menu preferences. An empty
// recorded array explicitly means twelve vanilla dungeons.
inline nlohmann::json OotDungeonLayoutSnapshot(const nlohmann::json& oot) {
    auto snapshot = nlohmann::json::object();
    for (const char* key : { "masterQuestDungeons", "dungeonLayoutAssets" }) {
        if (oot.contains(key)) {
            snapshot[key] = oot.at(key);
        }
    }
    return snapshot;
}

inline void RecordOotDungeonLayouts(nlohmann::json& oot, OotLayoutDumper dump) {
    if (dump) {
        oot.update(OotDungeonLayoutSnapshot(nlohmann::json::parse(dump())));
    }
}

inline bool ValidateOotDungeonLayouts(const nlohmann::json& oot, OotLayoutSetter validate) {
    const auto snapshot = OotDungeonLayoutSnapshot(oot);
    return snapshot.empty() || (validate && validate(snapshot.dump().c_str()) != 0);
}

inline bool RestoreOotDungeonLayouts(const nlohmann::json& oot, OotLayoutSetter restore) {
    const auto snapshot = OotDungeonLayoutSnapshot(oot);
    if (snapshot.empty()) {
        return !restore || restore(nullptr) != 0; // legacy seeded reconstruction
    }
    return restore && restore(snapshot.dump().c_str()) != 0;
}
} // namespace ComboRando
