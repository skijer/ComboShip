#pragma once
#include "CrossDungeonSeed.h"
#include "CrossEntranceBossMap.h"
#include "MagicMirrorShuffle.h"

#include <unordered_set>

namespace ComboRando::CrossDungeon {
struct JointReach {
    std::unordered_set<std::string> oot, mm;
};

// Re-root both native searches until no additional foreign arrival can be reached.
// Owning items is insufficient to open a destination overworld: its real portal must be reachable.
template <typename Oracle, typename Query>
JointReach QueryJoint(const Oracle& oot, const Oracle& mm, const std::vector<std::string>& ootOwned,
                      const std::vector<std::string>& mmOwned, bool mmWorldInitiallyOpen, Query query) {
    std::set<int32_t> roots[2];
    bool mmWorld = mmWorldInitiallyOpen;
    uint16_t logicalClears = 0;
    JointReach result;
    struct ResetAccess {
        const Oracle& oot;
        const Oracle& mm;
        ~ResetAccess() noexcept {
            try {
                constexpr auto empty = "{\"overworld\":true,\"arrivals\":[],\"logicalClears\":0}";
                oot.SetDungeonAccess(empty);
                mm.SetDungeonAccess(empty);
            } catch (...) { std::terminate(); }
        }
    } reset{ oot, mm };
    auto access = [&](const Oracle& oracle, int game, bool world) {
        nlohmann::json j = { { "overworld", world }, { "arrivals", roots[game] }, { "logicalClears", logicalClears } };
        oracle.SetDungeonAccess(j.dump().c_str());
    };
    auto exits = [&](const Oracle& oracle) {
        auto outgoing = nlohmann::json::parse(oracle.GetDungeonExits());
        if (!outgoing.is_array() || outgoing.size() > 1024)
            throw std::invalid_argument("invalid cross-dungeon oracle exits");
        return outgoing;
    };
    auto collect = [&](const nlohmann::json& outgoing, int sourceGame) {
        bool changed = false;
        for (const auto& j : outgoing) {
            auto e = ParseEndpoint(j);
            if (e.game == sourceGame)
                throw std::invalid_argument("cross-dungeon oracle returned a local exit");
            if constexpr (HasArrivalPolicy<Oracle>::value) {
                const auto& destination = e.game == 0 ? oot : mm;
                if (destination.CanEnterArrival && !destination.CanEnterArrival(e.index))
                    continue;
            }
            changed |= roots[e.game].insert(e.index).second;
        }
        return changed;
    };
    auto collectBosses = [&](const Oracle& oracle) {
        const auto previous = logicalClears;
        if constexpr (HasBossOracle<Oracle>::value)
            if (oracle.GetBossDefeated)
                logicalClears |= LogicalBossClears(oracle.GetBossDefeated(), oracle.CrossBossSources);
        return previous != logicalClears;
    };
    for (int pass = 0; pass < 2050; ++pass) {
        access(oot, 0, true);
        result.oot = query(oot, ootOwned);
        bool changed = collectBosses(oot);
        const auto ootExits = exits(oot);
        bool portal = !oot.GetPortalOpen || oot.GetPortalOpen() != 0;
        if (portal && !mmWorld) {
            mmWorld = true;
            changed = true;
        }
        access(mm, 1, mmWorld);
        result.mm = query(mm, mmOwned);
        changed |= collectBosses(mm);
        const auto mmExits = exits(mm);
        // Destination safety uses this query's inventory/time settings in both native hosts.
        changed |= collect(ootExits, 0);
        changed |= collect(mmExits, 1);
        if (!changed) {
            MagicMirrorShuffle::AddOotRoute(result.oot, result.mm);
            return result;
        }
    }
    throw std::runtime_error("cross-dungeon oracle did not converge");
}
} // namespace ComboRando::CrossDungeon
