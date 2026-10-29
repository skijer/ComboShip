#pragma once
#include "CrossDungeonEntrances.h"
#include <nlohmann/json.hpp>

namespace ComboRando::CrossDungeon {
inline nlohmann::json OptionsJson(const PoolOptions& p) {
    nlohmann::json options = { { "dungeons", p.dungeons },          { "bosses", p.bosses },
                               { "interiors", p.interiors },        { "grottos", p.grottos },
                               { "expanded", p.expanded },          { "shops", p.shops },
                               { "doorInteriors", p.doorInteriors } };
    if (p.legacyInteriors)
        options["legacyInteriors"] = true;
    return options;
}
inline PoolOptions ParseOptions(const nlohmann::json& j) {
    PoolOptions p;
    if (!j.is_object())
        throw std::invalid_argument("invalid cross-entrance pool options");
    auto flag = [&](const char* key, bool fallback) {
        auto it = j.find(key);
        if (it == j.end())
            return fallback;
        if (!it->is_boolean())
            throw std::invalid_argument("cross-entrance pool option must be boolean");
        return it->get<bool>();
    };
    p.dungeons = flag("dungeons", true);
    p.bosses = flag("bosses", false);
    p.interiors = flag("interiors", false);
    p.grottos = flag("grottos", false);
    p.expanded = flag("expanded", false);
    p.shops = flag("shops", false);
    p.doorInteriors = flag("doorInteriors", false);
    p.legacyInteriors = flag("legacyInteriors", false);
    return p;
}
inline PoolOptions OptionsFromSeed(const nlohmann::json& seed) {
    auto options = seed.is_object() && seed.contains("options") ? ParseOptions(seed.at("options")) : PoolOptions{};
    // The recorded seed version, never today's menu choices, owns membership on reload.
    options.legacyInteriors = seed.is_object() && seed.value("version", 4) < 4;
    return options;
}
inline bool Selected(const Dungeon& d, const PoolOptions& p) {
    switch (d.pool) {
        case Pool::Dungeon:
            return p.dungeons && (!d.expanded || p.expanded);
        case Pool::Boss:
            return p.bosses;
        case Pool::Interior:
            return p.interiors;
        case Pool::Grotto:
            return p.grottos;
        case Pool::Shop:
            return p.shops && (!p.interiors || !p.legacyInteriors);
        case Pool::DoorInterior:
            return p.doorInteriors && !p.interiors;
    }
    return false;
}
inline nlohmann::json EndpointJson(Endpoint e) {
    return { { "game", e.game }, { "index", e.index } };
}
inline Endpoint ParseEndpoint(const nlohmann::json& j) {
    const auto& g = j.at("game");
    const auto& i = j.at("index");
    if (!g.is_number_integer() || !i.is_number_integer() || g.get<int64_t>() < 0 || g.get<int64_t>() > 1 ||
        i.get<int64_t>() < 0 || i.get<int64_t>() > 0xFFFF)
        throw std::invalid_argument("invalid game-tagged dungeon endpoint");
    return { g.get<int>(), i.get<int32_t>() };
}
inline nlohmann::json DungeonJson(const Dungeon& d) {
    auto escapes = nlohmann::json::array();
    for (auto e : d.escapeExits)
        escapes.push_back(EndpointJson(e));
    nlohmann::json result = { { "escapeExits", escapes },
                              { "id", d.id },
                              { "entrance", EndpointJson(d.entrance) },
                              { "exit", EndpointJson(d.exit) },
                              { "returnTo", EndpointJson(d.returnTo) } };
    if (d.pool != Pool::Dungeon)
        result["pool"] = static_cast<int>(d.pool);
    if (d.expanded)
        result["expanded"] = true;
    if (d.pool == Pool::Boss) {
        result["boss"] = d.boss;
        result["owner"] = d.owner;
        result["hasReverse"] = d.hasReverse;
        if (d.warpReturn)
            result["warpReturn"] = EndpointJson(*d.warpReturn);
        if (d.reverseCheckpoint)
            result["reverseCheckpoint"] = EndpointJson(*d.reverseCheckpoint);
        if (d.warpCheckpoint)
            result["warpCheckpoint"] = EndpointJson(*d.warpCheckpoint);
        if (d.deathCheckpoint)
            result["deathCheckpoint"] = EndpointJson(*d.deathCheckpoint);
    }
    return result;
}
inline Dungeon ParseDungeon(const nlohmann::json& j) {
    Dungeon d{ j.at("id").get<std::string>(), ParseEndpoint(j.at("entrance")), ParseEndpoint(j.at("exit")),
               ParseEndpoint(j.at("returnTo")) };
    const auto escapes = j.value("escapeExits", nlohmann::json::array());
    if (!escapes.is_array())
        throw std::invalid_argument("cross-entrance escapes must be an array");
    for (const auto& e : escapes)
        d.escapeExits.push_back(ParseEndpoint(e));
    if (j.contains("pool")) {
        const auto& p = j.at("pool");
        if (!p.is_number_integer() || p.get<int64_t>() < 0 || p.get<int64_t>() > 5)
            throw std::invalid_argument("invalid cross-entrance category");
        d.pool = static_cast<Pool>(p.get<int>());
    }
    d.expanded = j.value("expanded", false);
    d.owner = j.value("owner", std::string{});
    if (j.contains("boss")) {
        const auto& boss = j.at("boss");
        if (!boss.is_number_integer() || (boss.is_number_unsigned() && boss.get<uint64_t>() > 11) ||
            boss.get<int64_t>() < 0 || boss.get<int64_t>() > 11)
            throw std::invalid_argument("invalid physical boss identity");
        d.boss = boss.get<int>();
    }
    d.hasReverse = j.value("hasReverse", true);
    if (j.contains("warpReturn"))
        d.warpReturn = ParseEndpoint(j.at("warpReturn"));
    if (j.contains("reverseCheckpoint"))
        d.reverseCheckpoint = ParseEndpoint(j.at("reverseCheckpoint"));
    if (j.contains("warpCheckpoint"))
        d.warpCheckpoint = ParseEndpoint(j.at("warpCheckpoint"));
    if (j.contains("deathCheckpoint"))
        d.deathCheckpoint = ParseEndpoint(j.at("deathCheckpoint"));
    return d;
}
inline nlohmann::json Encode(const std::vector<Dungeon>& dungeons, const std::vector<size_t>& permutation) {
    FromPermutation(dungeons, permutation); // Never persist a map that cannot be replayed.
    auto entries = nlohmann::json::array();
    for (const auto& d : dungeons)
        entries.push_back(DungeonJson(d));
    nlohmann::json result = {
        { "version", 1 }, { "enabled", true }, { "dungeons", entries }, { "permutation", permutation }
    };
    PoolOptions options;
    options.dungeons = false;
    for (const auto& d : dungeons) {
        if (d.pool == Pool::Dungeon) {
            options.dungeons = true;
            options.expanded |= d.expanded;
        }
        if (d.pool == Pool::Boss)
            options.bosses = true;
        if (d.pool == Pool::Interior)
            options.interiors = true;
        if (d.pool == Pool::Grotto)
            options.grottos = true;
        if (d.pool == Pool::Shop)
            options.shops = true;
        if (d.pool == Pool::DoorInterior)
            options.doorInteriors = true;
    }
    if (options.bosses || options.interiors || options.grottos || options.expanded || options.shops ||
        options.doorInteriors) {
        result["version"] = 4;
        result["options"] = OptionsJson(options);
    }
    return result;
}
inline std::vector<Route> Decode(const nlohmann::json& j) {
    if (j.is_null())
        return {}; // A seed written before this feature.
    if (!j.at("enabled").is_boolean())
        throw std::invalid_argument("cross-dungeon enabled flag must be boolean");
    if (!j.at("enabled").get<bool>())
        return {};
    if (!j.at("version").is_number_integer() ||
        (j.at("version").get<int64_t>() < 1 || j.at("version").get<int64_t>() > 4))
        throw std::invalid_argument("unsupported cross-dungeon seed version");
    const int version = j.at("version").get<int>();
    const bool expandedCodec = version >= 2;
    if (expandedCodec && !j.contains("options"))
        throw std::invalid_argument("cross-entrance v2 options missing");
    const auto options = OptionsFromSeed(j);
    const auto& entries = j.at("dungeons");
    const auto& p = j.at("permutation");
    if (!entries.is_array() || entries.size() < 2 || entries.size() > (expandedCodec ? 512 : 64) || !p.is_array())
        throw std::invalid_argument("invalid cross-dungeon catalogue");
    std::vector<Dungeon> dungeons;
    std::vector<size_t> permutation;
    for (const auto& entry : entries) {
        auto d = ParseDungeon(entry);
        if (!Selected(d, options) || (!expandedCodec && (d.pool != Pool::Dungeon || d.expanded)) ||
            (version < 3 && (d.pool == Pool::Shop || d.pool == Pool::DoorInterior)))
            throw std::invalid_argument("cross-entrance category disabled in recorded options");
        dungeons.push_back(std::move(d));
    }
    for (const auto& n : p) {
        if (!n.is_number_integer() || n.get<int64_t>() < 0 || n.get<uint64_t>() >= entries.size())
            throw std::invalid_argument("invalid cross-dungeon permutation index");
        permutation.push_back(n.get<size_t>());
    }
    return FromPermutation(dungeons, permutation);
}
inline nlohmann::json GenerateSeed(const std::vector<Dungeon>& dungeons, uint64_t seed) {
    auto routes = Generate(dungeons, seed, true);
    std::vector<size_t> permutation(dungeons.size());
    for (const auto& r : routes) {
        if (r.phase != Phase::AfterNative)
            continue;
        auto it = std::find_if(dungeons.begin(), dungeons.end(), [&](const Dungeon& d) { return d.entrance == r.to; });
        auto from =
            std::find_if(dungeons.begin(), dungeons.end(), [&](const Dungeon& d) { return d.entrance == r.from; });
        permutation[static_cast<size_t>(from - dungeons.begin())] = static_cast<size_t>(it - dungeons.begin());
    }
    return Encode(dungeons, permutation);
}
} // namespace ComboRando::CrossDungeon
