#pragma once

#include "CrossDungeonCoordinator.h"
#include "CrossWorldRando.h"

namespace ComboRando::CrossDungeon {

inline constexpr size_t kStructuralLayoutAttempts = 128;
class StructureExhausted : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

inline void CheckStructuralCancellation(const ComboGenProgress* progress) {
    if (GenCancelled(progress))
        throw std::runtime_error("generation cancelled during entrance validation");
}

// Preserve counts: two Hookshots or several dungeon keys are not the same inventory as one.
inline std::vector<std::string> StructuralOwned(const nlohmann::json& dump) {
    std::vector<std::string> owned;
    for (const auto& item : dump.at("pool"))
        owned.push_back(item.at("name").get<std::string>());
    return owned;
}

inline std::unordered_set<std::string> StructuralQuery(const OracleFns& oracle, const std::vector<std::string>& owned) {
    if (!oracle.Reset || !oracle.SetOwnedItems || !oracle.GetReachableChecks)
        throw std::runtime_error("cross-dungeon structural oracle exports missing");
    oracle.Reset();
    oracle.SetOwnedItems(nlohmann::json(owned).dump().c_str());
    const auto reachable = nlohmann::json::parse(oracle.GetReachableChecks());
    if (!reachable.is_array())
        throw std::invalid_argument("invalid structural oracle check array");
    std::unordered_set<std::string> result;
    for (const auto& name : reachable)
        result.insert(name.get<std::string>());
    return result;
}

struct StructureProbe {
    struct Fixed {
        std::string check, item;
    };
    std::vector<std::string> ootOwned, mmOwned;
    std::vector<Fixed> ootFixed, mmFixed;
    std::unordered_set<std::string> ootRequired, mmRequired;
    bool mmInitiallyOpen = false;
    const ComboGenProgress* progress = nullptr;

    StructureProbe(const std::string& ootDump, const std::string& mmDump, const OracleFns& oot, const OracleFns& mm,
                   bool mmStart, const ComboGenProgress* progress = nullptr)
        : progress(progress) {
        const auto od = nlohmann::json::parse(ootDump), md = nlohmann::json::parse(mmDump);
        ootOwned = StructuralOwned(od);
        mmOwned = StructuralOwned(md);
        auto fixed = [](const nlohmann::json& dump) {
            std::vector<Fixed> placements;
            for (const auto& entry : dump.value("fixed", nlohmann::json::array()))
                placements.push_back({ entry.at("check").get<std::string>(), entry.at("item").get<std::string>() });
            return placements;
        };
        ootFixed = fixed(od);
        mmFixed = fixed(md);
        mmInitiallyOpen = mmStart || OotAccessFromDump(ootDump) == OotAccess::NO_LOGIC;
        auto required = [](const nlohmann::json& dump, const auto& reach) {
            std::unordered_set<std::string> checks;
            for (const auto& check : dump.at("checks")) {
                const auto name = check.at("name").template get<std::string>();
                if (reach.count(name))
                    checks.insert(name);
            }
            for (const auto& fixed : dump.value("fixed", nlohmann::json::array())) {
                const auto name = fixed.at("check").template get<std::string>();
                if (fixed.value("advancement", true) && reach.count(name))
                    checks.insert(name);
            }
            return checks;
        };
        // Native logic intentionally omits some goal-gated or under-modelled checks even with every
        // item. Compare with its actual baseline, rather than demanding impossible named checks.
        const auto native = Closure(oot, mm, false);
        ootRequired = required(od, native.oot);
        mmRequired = required(md, native.mm);
    }

    JointReach Closure(const OracleFns& oot, const OracleFns& mm, bool joint) const {
        auto oo = ootOwned, mo = mmOwned;
        std::vector<bool> ootCredited(ootFixed.size(), false), mmCredited(mmFixed.size(), false);
        const size_t maxQueries = ootFixed.size() + mmFixed.size() + 1;
        const auto query = [&](const OracleFns& oracle, const std::vector<std::string>& owned) {
            CheckStructuralCancellation(progress);
            auto reached = StructuralQuery(oracle, owned);
            CheckStructuralCancellation(progress);
            return reached;
        };
        for (size_t pass = 0; pass < maxQueries; ++pass) {
            auto reach = joint ? QueryJoint(oot, mm, oo, mo, mmInitiallyOpen, query)
                               : JointReach{ query(oot, oo), query(mm, mo) };
            auto collect = [](const auto& fixed, auto& credited, const auto& reached, auto& owned) {
                bool changed = false;
                for (size_t i = 0; i < fixed.size(); ++i)
                    if (!credited[i] && reached.count(fixed[i].check)) {
                        owned.push_back(fixed[i].item);
                        credited[i] = true;
                        changed = true;
                    }
                return changed;
            };
            // A confined key/song/reward is still earned at its real check. Granting every fixed
            // item up front would certify a dungeon that locks its own required song behind it.
            const bool ootChanged = collect(ootFixed, ootCredited, reach.oot, oo);
            const bool mmChanged = collect(mmFixed, mmCredited, reach.mm, mo);
            if (!ootChanged && !mmChanged)
                return reach;
        }
        throw std::runtime_error("cross-dungeon fixed-item structural closure did not converge");
    }

    std::string Missing(const OracleFns& oot, const OracleFns& mm) const {
        const auto joint = Closure(oot, mm, true);
        auto missing = [](const auto& expected, const auto& reached, const char* game) {
            std::vector<std::string> lost;
            for (const auto& check : expected)
                if (!reached.count(check))
                    lost.push_back(check);
            std::sort(lost.begin(), lost.end());
            return lost.empty() ? std::string()
                                : std::string(game) + " loses " + std::to_string(lost.size()) +
                                      " native-reachable checks (e.g. " + lost.front() + ")";
        };
        const auto ootLost = missing(ootRequired, joint.oot, "OOT");
        const auto mmLost = missing(mmRequired, joint.mm, "MM");
        return ootLost + (ootLost.empty() || mmLost.empty() ? "" : "; ") + mmLost;
    }
};

inline void ClearStructuralAccess(const Coordinator& coordinator) {
    // The all-owned fixed point must not leave arrival roots in the next item-fill query (whose
    // portal prerequisite derivation begins with an OoT-only search before its first joint query).
    for (const auto& host : coordinator.hosts)
        if (host.SetAccess)
            host.SetAccess("{\"overworld\":true,\"arrivals\":[]}");
}

// Sparse valid layouts become expensive to find by replacing every pool at once. Keep
// native reachability after each accepted cycle, then split a rejected cycle into swaps.
// Every candidate is still a full, bijective, pool-isolated map checked by the real oracle.
template <typename IsValid>
std::optional<nlohmann::json> GenerateConstrainedSeed(const std::vector<Dungeon>& dungeons, uint64_t masterSeed,
                                                      IsValid isValid) {
    const auto desired = GenerateSeed(dungeons, masterSeed).at("permutation").get<std::vector<size_t>>();
    std::vector<size_t> current(dungeons.size());
    std::iota(current.begin(), current.end(), 0);
    if (!isValid(Encode(dungeons, current)))
        return std::nullopt;
    std::vector<bool> visited(dungeons.size(), false);
    bool changed = false;
    for (size_t first = 0; first < desired.size(); ++first) {
        if (visited[first])
            continue;
        std::vector<size_t> cycle;
        for (size_t at = first; !visited[at]; at = desired[at]) {
            visited[at] = true;
            cycle.push_back(at);
        }
        if (cycle.size() < 2)
            continue;
        auto candidate = current;
        for (size_t member : cycle)
            candidate[member] = desired[member];
        if (isValid(Encode(dungeons, candidate))) {
            current = std::move(candidate);
            changed = true;
            continue;
        }
        // Adjacent pairs let a constrained member stay fixed while its neighbors shuffle.
        for (size_t pair = 1; pair < cycle.size(); ++pair) {
            candidate = current;
            std::swap(candidate[cycle[pair - 1]], candidate[cycle[pair]]);
            if (isValid(Encode(dungeons, candidate))) {
                current = std::move(candidate);
                changed = true;
            }
        }
    }
    if (!changed)
        return std::nullopt;
    auto result = Encode(dungeons, current);
    // The last rejected candidate may still be installed in the temporary oracle.
    if (!isValid(result))
        throw std::runtime_error("constrained cross-entrance validation was not deterministic");
    return result;
}

// Native entrances are already shuffled. Only the cross-map's isolated stream is retried; no native
// RNG, item-fill RNG, settings or gameplay routing is changed by this layout validation.
inline nlohmann::json GenerateValidatedSeed(Coordinator& coordinator, bool enabled, uint64_t masterSeed,
                                            const std::string& ootDump, const std::string& mmDump, OracleFns& oot,
                                            OracleFns& mm, bool mmStart, size_t maxAttempts = kStructuralLayoutAttempts,
                                            const ComboGenProgress* progress = nullptr) {
    CheckStructuralCancellation(progress);
    if (!enabled) {
        auto seed = coordinator.GenerateAndInstall(false, masterSeed);
        coordinator.ConfigureOracles(oot, mm);
        return seed;
    }
    const auto previous = coordinator.LogicSeed();
    try {
        const auto dungeons = coordinator.Catalogue();
        coordinator.InstallLogic(nullptr);
        coordinator.ConfigureOracles(oot, mm);
        const StructureProbe probe(ootDump, mmDump, oot, mm, mmStart, progress);
        std::string missing;
        for (size_t attempt = 0; attempt < maxAttempts; ++attempt) {
            const uint64_t layoutSeed = masterSeed + static_cast<uint64_t>(attempt) * 0x9E3779B97F4A7C15ULL;
            const auto seed = GenerateSeed(dungeons, layoutSeed);
            coordinator.InstallLogic(seed, false);
            coordinator.ConfigureOracles(oot, mm);
            missing = probe.Missing(oot, mm);
            if (missing.empty()) {
                ClearStructuralAccess(coordinator);
                if (attempt)
                    std::cout << "[CrossDungeon] structural layout accepted after " << (attempt + 1) << " attempts\n";
                return seed;
            }
        }
        const bool expandedCodec = std::any_of(dungeons.begin(), dungeons.end(), [](const Dungeon& dungeon) {
            return dungeon.pool != Pool::Dungeon || dungeon.expanded;
        });
        if (expandedCodec) {
            auto repaired = GenerateConstrainedSeed(dungeons, masterSeed, [&](const nlohmann::json& candidate) {
                coordinator.InstallLogic(candidate, false);
                coordinator.ConfigureOracles(oot, mm);
                missing = probe.Missing(oot, mm);
                return missing.empty();
            });
            if (repaired) {
                ClearStructuralAccess(coordinator);
                std::cout << "[CrossDungeon] expanded layout accepted with validated cycles after " << maxAttempts
                          << " whole-map attempts\n";
                return *repaired;
            }
        }
        throw StructureExhausted("no structurally valid cross-dungeon layout after " + std::to_string(maxAttempts) +
                                 " attempts: " + missing);
    } catch (...) {
        coordinator.InstallLogic(previous, false);
        coordinator.ConfigureOracles(oot, mm);
        if (Coordinator::Enabled(previous))
            ClearStructuralAccess(coordinator);
        throw;
    }
}

// Replay is an exact-map operation: never replace an incompatible recorded layout with a new roll.
inline void ValidateSavedStructure(Coordinator& coordinator, const nlohmann::json& seed, const std::string& ootDump,
                                   const std::string& mmDump, OracleFns& oot, OracleFns& mm, bool mmStart) {
    if (!Coordinator::Enabled(seed)) {
        coordinator.InstallLogic(seed);
        coordinator.ConfigureOracles(oot, mm);
        return;
    }
    const auto previous = coordinator.LogicSeed();
    try {
        coordinator.ValidateCatalogue(seed);
        coordinator.InstallLogic(nullptr);
        coordinator.ConfigureOracles(oot, mm);
        const StructureProbe probe(ootDump, mmDump, oot, mm, mmStart);
        coordinator.InstallLogic(seed, false);
        coordinator.ConfigureOracles(oot, mm);
        const auto missing = probe.Missing(oot, mm);
        if (!missing.empty())
            throw std::invalid_argument("recorded cross-dungeon layout is structurally invalid: " + missing);
        ClearStructuralAccess(coordinator);
    } catch (...) {
        coordinator.InstallLogic(previous, false);
        coordinator.ConfigureOracles(oot, mm);
        if (Coordinator::Enabled(previous))
            ClearStructuralAccess(coordinator);
        throw;
    }
}

} // namespace ComboRando::CrossDungeon
