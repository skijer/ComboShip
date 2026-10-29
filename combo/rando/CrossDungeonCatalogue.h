#pragma once
#include "CrossDungeonSeed.h"

namespace ComboRando::CrossDungeon {
// Derive the candidate's returns from recorded native data without touching the live graph or RNG.
inline std::map<int32_t, int32_t> ParseNativeOverrides(const nlohmann::json& raw,
                                                       const std::map<int32_t, int>& knownTypes, size_t capacity) {
    if (!raw.is_array() || raw.size() > capacity)
        throw std::invalid_argument("invalid native entrance override array");
    std::map<int32_t, int32_t> overrides;
    for (const auto& entry : raw) {
        auto integer = [&](const char* key) {
            const auto& value = entry.at(key);
            if (!value.is_number_integer() || (value.is_number_unsigned() && value.get<uint64_t>() > 0x7fff) ||
                value.get<int64_t>() < -1 || value.get<int64_t>() > 0x7fff)
                throw std::invalid_argument("invalid native entrance override integer");
            return value.get<int>();
        };
        const int index = integer("index"), target = integer("override"), type = integer("type");
        if (!knownTypes.count(index) || !knownTypes.count(target) || knownTypes.at(index) != type ||
            !overrides.emplace(index, target).second)
            throw std::invalid_argument("unknown or duplicate native entrance override");
        for (const char* key : { "destination", "overrideDestination" }) {
            const int destination = integer(key);
            if (destination != -1 && !knownTypes.count(destination))
                throw std::invalid_argument("unknown native entrance override destination");
        }
    }
    return overrides;
}

inline std::vector<Dungeon> ComposeNativeCatalogue(std::vector<Dungeon> base,
                                                   const std::map<int32_t, int32_t>& blueWarpExits,
                                                   const std::map<int32_t, int32_t>& overrides) {
    for (auto& dungeon : base) {
        dungeon.returnTo.index = ResolveNativeIndex(dungeon.exit.index, dungeon.exit.index, overrides);
        dungeon.escapeExits.clear();
        for (const auto& [warp, unused] : blueWarpExits) {
            (void)unused;
            const int target = ResolveNativeIndex(warp, warp, overrides);
            if (CanonicalNativeReturn(target, blueWarpExits) == dungeon.returnTo.index)
                dungeon.escapeExits.push_back({ 0, warp });
        }
    }
    Validate(base);
    return base;
}
} // namespace ComboRando::CrossDungeon
