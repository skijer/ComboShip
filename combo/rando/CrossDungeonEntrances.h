#pragma once

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace ComboRando::CrossDungeon {

inline int32_t ResolveNativeIndex(int32_t source, int32_t replacement, const std::map<int32_t, int32_t>& recorded) {
    auto it = recorded.find(source);
    return it == recorded.end() ? replacement : it->second;
}

inline int32_t CanonicalNativeReturn(int32_t resolved, const std::map<int32_t, int32_t>& blueWarpExits) {
    auto it = blueWarpExits.find(resolved);
    return it == blueWarpExits.end() ? resolved : it->second;
}

struct Endpoint {
    int game = 0; // 0 = OoT, 1 = MM; never reinterpret an entrance in the other host.
    int32_t index = 0;
    bool operator==(const Endpoint& other) const {
        return game == other.game && index == other.index;
    }
    bool operator<(const Endpoint& other) const {
        return std::tie(game, index) < std::tie(other.game, other.index);
    }
};

enum class Phase { BeforeNative, AfterNative };
enum class Pool { Dungeon = 0, Boss = 1, Interior = 2, Grotto = 3, Shop = 4, DoorInterior = 5 };
inline bool IsInteriorPool(Pool pool) {
    return pool == Pool::Interior || pool == Pool::Shop || pool == Pool::DoorInterior;
}
struct PoolOptions {
    bool dungeons = true, bosses = false, interiors = false, grottos = false, expanded = false;
    bool shops = false, doorInteriors = false;
    bool legacyInteriors = false; // Catalogue policy for exact replay of schema v1-v3 seeds.
    bool Any() const {
        return dungeons || bosses || interiors || grottos || shops || doorInteriors;
    }
};
enum class InteriorKind { Passage, Door, ShopDoor, ShopPassage };
inline std::optional<Pool> InteriorPool(InteriorKind kind, const PoolOptions& options) {
    if (options.legacyInteriors) {
        if (options.interiors)
            return Pool::Interior;
        if (kind == InteriorKind::ShopDoor && options.shops)
            return Pool::Shop;
    } else if (kind == InteriorKind::ShopDoor || kind == InteriorKind::ShopPassage) {
        return options.shops ? std::optional<Pool>(Pool::Shop) : std::nullopt;
    } else if (options.interiors) {
        return Pool::Interior;
    }
    if (kind == InteriorKind::Door && options.doorInteriors)
        return Pool::DoorInterior;
    return std::nullopt;
}

struct Dungeon {
    std::string id;
    Endpoint entrance;
    Endpoint exit;
    Endpoint returnTo; // Already resolved through this slot's native exit shuffle.
    std::vector<Endpoint> escapeExits = {};
    Pool pool = Pool::Dungeon;
    bool expanded = false;
    int boss = -1;
    std::string owner = {};
    bool hasReverse = true;
    std::optional<Endpoint> warpReturn = std::nullopt, reverseCheckpoint = std::nullopt, warpCheckpoint = std::nullopt,
                            deathCheckpoint = std::nullopt;
};

struct Route {
    Phase phase;
    Endpoint from;
    Endpoint to;
    std::string dungeon;
    std::optional<Endpoint> arrivalCheckpoint = std::nullopt;
    bool operator==(const Route& other) const {
        return phase == other.phase && from == other.from && to == other.to && dungeon == other.dungeon &&
               arrivalCheckpoint == other.arrivalCheckpoint;
    }
};

inline bool Valid(Endpoint e) {
    return (e.game == 0 || e.game == 1) && e.index >= 0 && e.index <= 0xFFFF;
}

inline void Validate(const std::vector<Dungeon>& dungeons) {
    std::set<std::string> ids;
    std::set<Endpoint> entries;
    std::map<Endpoint, Pool> exits;
    std::set<int> bosses;
    auto claim = [&](Endpoint e, Pool pool, bool alias) {
        if (!Valid(e))
            throw std::invalid_argument("invalid cross-entrance exit");
        auto [it, inserted] = exits.emplace(e, pool);
        if (!inserted && !(alias && ((it->second == Pool::Dungeon && pool == Pool::Boss) ||
                                     (it->second == Pool::Boss && pool == Pool::Dungeon))))
            throw std::invalid_argument("duplicate cross-entrance exit");
    };
    for (const auto& d : dungeons) {
        if (d.id.empty() || !ids.insert(d.id).second || !Valid(d.entrance) || !Valid(d.exit) || !Valid(d.returnTo) ||
            d.entrance.game != d.exit.game || d.exit.game != d.returnTo.game || !entries.insert(d.entrance).second ||
            static_cast<int>(d.pool) < 0 || static_cast<int>(d.pool) > 5) {
            throw std::invalid_argument("invalid or duplicate cross-dungeon endpoint");
        }
        if (d.hasReverse)
            claim(d.exit, d.pool, false);
        if (d.pool == Pool::Boss &&
            (d.boss < 0 || d.boss >= 12 || d.owner.empty() || !d.warpReturn ||
             (d.boss < 8 ? d.entrance.game != 0 : d.entrance.game != 1) || !bosses.insert(d.boss).second))
            throw std::invalid_argument("invalid or duplicate boss metadata");
        if (d.pool != Pool::Boss && (d.boss != -1 || !d.owner.empty() || !d.hasReverse || d.warpReturn ||
                                     d.reverseCheckpoint || d.warpCheckpoint || d.deathCheckpoint))
            throw std::invalid_argument("boss metadata on another entrance category");
        for (const auto& e : { d.warpReturn, d.reverseCheckpoint, d.warpCheckpoint, d.deathCheckpoint })
            if (e && (!Valid(*e) || e->game != d.entrance.game))
                throw std::invalid_argument("invalid cross-entrance recovery endpoint");
        for (auto e : d.escapeExits) {
            if (!Valid(e) || e.game != d.exit.game)
                throw std::invalid_argument("invalid or duplicate dungeon escape endpoint");
            claim(e, d.pool, true);
        }
    }
}

inline std::vector<Route> FromPermutation(const std::vector<Dungeon>& dungeons,
                                          const std::vector<size_t>& permutation) {
    Validate(dungeons);
    if (permutation.size() != dungeons.size())
        throw std::invalid_argument("cross-dungeon permutation size mismatch");
    std::set<size_t> seen;
    std::vector<Route> routes;
    for (size_t i = 0; i < dungeons.size(); ++i) {
        if (permutation[i] >= dungeons.size() || !seen.insert(permutation[i]).second ||
            dungeons[i].pool != dungeons[permutation[i]].pool)
            throw std::invalid_argument("cross-dungeon permutation is not bijective");
        const auto& source = dungeons[i];
        const auto& target = dungeons[permutation[i]];
        if (source.pool == Pool::Boss)
            continue;
        routes.push_back({ Phase::AfterNative, source.entrance, target.entrance, target.id });
        routes.push_back({ Phase::BeforeNative, target.exit, source.returnTo, source.id });
        for (auto escape : target.escapeExits)
            routes.push_back({ Phase::BeforeNative, escape, source.returnTo, source.id });
    }
    const auto nonboss = routes;
    std::set<Endpoint> bossEscapes;
    for (const auto& d : dungeons)
        if (d.pool == Pool::Boss)
            bossEscapes.insert(d.escapeExits.begin(), d.escapeExits.end());
    routes.erase(
        std::remove_if(routes.begin(), routes.end(),
                       [&](const Route& r) { return r.phase == Phase::BeforeNative && bossEscapes.count(r.from); }),
        routes.end());
    for (size_t i = 0; i < dungeons.size(); ++i) {
        const auto& source = dungeons[i];
        if (source.pool != Pool::Boss)
            continue;
        const auto& target = dungeons[permutation[i]];
        routes.push_back({ Phase::AfterNative, source.entrance, target.entrance, target.id, target.entrance });
        if (target.hasReverse)
            routes.push_back(
                { Phase::BeforeNative, target.exit, source.returnTo, source.id, source.reverseCheckpoint });
        Endpoint warp = *source.warpReturn;
        auto recovery = source.warpCheckpoint;
        bool composed = false;
        // Odolwa's first completion remains inside the owning temple's prison.
        const bool internalWarp = source.boss == 8 && source.warpCheckpoint.has_value();
        for (auto escape : source.escapeExits) {
            if (internalWarp)
                break;
            for (const auto& r : nonboss)
                if (r.phase == Phase::BeforeNative && r.from == escape) {
                    warp = r.to;
                    recovery = r.arrivalCheckpoint;
                    composed = true;
                    break;
                }
            if (composed)
                break;
        }
        if (!internalWarp && !composed)
            for (size_t next = 0; next < dungeons.size(); ++next)
                if (dungeons[next].entrance == warp) {
                    warp = dungeons[permutation[next]].entrance;
                    recovery = warp;
                    break; // The native target receives exactly one cross-pool overlay.
                }
        for (auto escape : target.escapeExits)
            routes.push_back({ Phase::BeforeNative, escape, warp, source.id, recovery });
    }
    std::set<std::tuple<Phase, int, int32_t>> unique;
    for (const auto& r : routes)
        if (!unique.emplace(r.phase, r.from.game, r.from.index).second)
            throw std::invalid_argument("ambiguous composed cross-entrance route");
    return routes;
}

inline std::vector<Route> Generate(const std::vector<Dungeon>& dungeons, uint64_t seed, bool enabled) {
    if (!enabled)
        return {}; // No native graph or RNG work on the existing disabled path.
    std::vector<size_t> permutation;
    for (size_t i = 0; i < dungeons.size(); ++i)
        permutation.push_back(i);
    uint64_t state = seed ^ 0x43524F535344554EULL;
    std::map<Pool, std::vector<size_t>> pools;
    for (size_t i = 0; i < dungeons.size(); ++i)
        pools[dungeons[i].pool].push_back(i);
    for (const auto& [pool, members] : pools) {
        if (pool != Pool::Dungeon)
            state = seed ^ (0x43524F535344554EULL + static_cast<int>(pool));
        for (size_t i = members.size(); i > 1; --i) {
            state = state * 6364136223846793005ULL + 1442695040888963407ULL;
            std::swap(permutation[members[i - 1]], permutation[members[(state >> 33) % i]]);
        }
    }
    return FromPermutation(dungeons, permutation);
}

inline Endpoint Resolve(const std::vector<Route>& routes, Phase phase, Endpoint source) {
    for (const auto& route : routes) {
        if (route.phase == phase && route.from == source)
            return route.to; // Exactly one leg. A second lookup would undo a permutation cycle.
    }
    return source;
}

} // namespace ComboRando::CrossDungeon
