#pragma once
#include "CrossDungeonSeed.h"
#include <array>
#include <type_traits>
#include <utility>

namespace ComboRando::CrossDungeon {
using BossSourceMap = std::array<int, 12>;
template <class T, class = void> struct HasBossOracle : std::false_type {};
template <class T>
struct HasBossOracle<
    T, std::void_t<decltype(std::declval<T>().GetBossDefeated), decltype(std::declval<T>().CrossBossSources)>>
    : std::true_type {};
template <class T, class = void> struct HasArrivalPolicy : std::false_type {};
template <class T>
struct HasArrivalPolicy<T, std::void_t<decltype(std::declval<T>().CanEnterArrival)>> : std::true_type {};
inline BossSourceMap EmptyBossSources() {
    BossSourceMap result;
    result.fill(-1);
    return result;
}
// The physical destination reports its kill; completion belongs to the source boss door.
inline BossSourceMap BossSources(const nlohmann::json& seed) {
    auto result = EmptyBossSources();
    if (Decode(seed).empty())
        return result;
    std::vector<Dungeon> entries;
    for (const auto& entry : seed.at("dungeons"))
        entries.push_back(ParseDungeon(entry));
    const auto permutation = seed.at("permutation").get<std::vector<size_t>>();
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].pool == Pool::Boss)
            result.at(entries.at(permutation.at(i)).boss) = entries[i].boss;
    return result;
}
inline uint16_t LogicalBossClears(uint16_t physical, const BossSourceMap& sources) {
    uint16_t logical = 0;
    for (size_t id = 0; id < sources.size(); ++id)
        if ((physical & (1u << id)) && sources[id] >= 0 && sources[id] < 12)
            logical |= 1u << sources[id];
    return logical;
}
} // namespace ComboRando::CrossDungeon
