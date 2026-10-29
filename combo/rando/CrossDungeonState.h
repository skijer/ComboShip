#pragma once
#include "CrossDungeonSeed.h"

namespace ComboRando::CrossDungeon {
class HostState {
  public:
    explicit HostState(int host) : game(host) {
    }
    int game;
    std::vector<Route> routes;
    std::vector<Dungeon> dungeons;
    uint16_t logicalClears = 0;
    bool overworld = true;
    std::set<int32_t> arrivals;

    void Install(const nlohmann::json& seed) {
        auto next = Decode(seed);
        std::vector<Dungeon> catalogue;
        if (!next.empty())
            for (const auto& entry : seed.at("dungeons"))
                catalogue.push_back(ParseDungeon(entry));
        routes = std::move(next);
        dungeons = std::move(catalogue);
        logicalClears = 0;
        arrivals.clear();
        overworld = true;
    }
    void SetAccess(const nlohmann::json& access) {
        auto next = std::set<int32_t>{};
        bool world = access.at("overworld").get<bool>();
        const auto mask = access.value("logicalClears", nlohmann::json(0));
        if (!mask.is_number_integer() || mask.get<int64_t>() < 0 || mask.get<uint64_t>() > 0xFFF)
            throw std::invalid_argument("invalid logical dungeon clear mask");
        for (const auto& index : access.at("arrivals")) {
            if (!index.is_number_integer() || index.get<int64_t>() < 0 || index.get<int64_t>() > 0xFFFF)
                throw std::invalid_argument("invalid cross-dungeon arrival root");
            next.insert(index.get<int32_t>());
        }
        arrivals = std::move(next);
        overworld = world;
        logicalClears = mask.get<uint16_t>();
    }
    bool HasPool(Pool pool) const {
        return std::any_of(dungeons.begin(), dungeons.end(), [&](const Dungeon& d) { return d.pool == pool; });
    }
    const Dungeon* FindEntrance(int32_t index) const {
        for (const auto& d : dungeons)
            if (d.entrance == Endpoint{ game, index })
                return &d;
        return nullptr;
    }
    const Route* Lookup(int32_t original, int32_t resolved) const {
        for (const auto& route : routes) {
            if (route.from.game == game && route.phase == Phase::BeforeNative && route.from.index == original)
                return &route;
        }
        for (const auto& route : routes) {
            if (route.from.game == game && route.phase == Phase::AfterNative && route.from.index == resolved)
                return &route;
        }
        return nullptr;
    }
};
} // namespace ComboRando::CrossDungeon
