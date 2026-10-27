#pragma once

#include "CrossDungeonSeed.h"
#include "CrossEntranceBossMap.h"
#include <array>
#include <utility>

namespace ComboRando::CrossDungeon {

struct HostFns {
    const char* (*DumpCatalogue)() = nullptr;
    int (*SetSeed)(const char*) = nullptr;
    void (*SetAccess)(const char*) = nullptr;
    const char* (*GetExits)() = nullptr;
    int (*SetRuntimeSeed)(const char*) = nullptr;
    // Pure seed-file catalogue: OoT needs the recorded native layout rather than the active graph.
    const char* (*DumpSeedCatalogue)(const char*) = nullptr;
    const char* (*DumpCatalogueForOptions)(const char*) = nullptr;
    uint16_t (*GetBossDefeated)() = nullptr;
    int (*CanEnterArrival)(int32_t) = nullptr;
};

// The oracle's candidate map and the playing slot's map have deliberately separate lifetimes.
// In particular, generating or previewing a seed never changes runtime routing.
class Coordinator {
  public:
    std::array<HostFns, 2> hosts{};
    PoolOptions selectedPools;
    void SetOptions(PoolOptions options) {
        selectedPools = options;
    }

    static nlohmann::json SeedOf(const nlohmann::json& consolidated) {
        return consolidated.is_object() && consolidated.contains("crossDungeonEntrances")
                   ? consolidated.at("crossDungeonEntrances")
                   : nlohmann::json();
    }
    static bool Enabled(const nlohmann::json& seed) {
        return !Decode(seed).empty(); // Includes strict validation of explicitly present data.
    }
    const nlohmann::json& LogicSeed() const {
        return logicSeed;
    }
    const nlohmann::json& RuntimeSeed() const {
        return runtimeSeed;
    }

    std::vector<Dungeon> Catalogue() const {
        return Catalogue(selectedPools);
    }
    std::vector<Dungeon> Catalogue(const PoolOptions& options) const {
        std::vector<Dungeon> all;
        const auto raw = OptionsJson(options).dump();
        for (int game = 0; game < 2; ++game) {
            if (!hosts[game].DumpCatalogue)
                throw std::runtime_error("cross-dungeon catalogue export missing; rebuild both game modules");
            if ((options.bosses || options.interiors || options.grottos || options.expanded || options.shops ||
                 options.doorInteriors) &&
                !hosts[game].DumpCatalogueForOptions)
                throw std::runtime_error("cross-entrance pool export missing; rebuild both game modules");
            const auto* catalogue = hosts[game].DumpCatalogueForOptions
                                        ? hosts[game].DumpCatalogueForOptions(raw.c_str())
                                        : hosts[game].DumpCatalogue();
            if (!catalogue)
                throw std::invalid_argument("native cross-entrance catalogue rejected");
            const auto entries = nlohmann::json::parse(catalogue);
            if (!entries.is_array() || entries.empty())
                throw std::invalid_argument("empty cross-dungeon host catalogue");
            for (const auto& entry : entries) {
                auto d = ParseDungeon(entry);
                if (d.entrance.game != game)
                    throw std::invalid_argument("cross-dungeon catalogue belongs to the wrong game");
                all.push_back(std::move(d));
            }
        }
        Validate(all);
        return all;
    }

    // Validate against the current native layout, after replaying its recorded entrance overrides.
    void ValidateCatalogue(const nlohmann::json& seed) const {
        if (!Enabled(seed))
            return;
        ValidateCatalogue(seed, Catalogue(OptionsFromSeed(seed)));
    }

    // Reload must reject a candidate before changing native seed, settings, prices, goals or graph.
    // Only the separate temporary oracle route state is installed during this preflight.
    void PreflightConsolidated(const nlohmann::json& consolidated) {
        const auto seed = SeedOf(consolidated);
        if (!Enabled(seed)) {
            InstallLogic(seed);
            return;
        }
        if (!hosts[0].DumpSeedCatalogue || !hosts[1].DumpCatalogue)
            throw std::runtime_error("cross-dungeon reload preflight export missing; rebuild both game modules");
        const auto raw = consolidated.dump();
        std::vector<Dungeon> dungeons;
        for (int game = 0; game < 2; ++game) {
            const char* catalogue = hosts[game].DumpSeedCatalogue ? hosts[game].DumpSeedCatalogue(raw.c_str())
                                                                  : hosts[game].DumpCatalogue();
            if (!catalogue)
                throw std::invalid_argument("native cross-dungeon seed catalogue rejected");
            const auto entries = nlohmann::json::parse(catalogue);
            if (!entries.is_array() || entries.empty())
                throw std::invalid_argument("invalid cross-dungeon seed catalogue");
            for (const auto& entry : entries) {
                auto dungeon = ParseDungeon(entry);
                if (dungeon.entrance.game != game)
                    throw std::invalid_argument("cross-dungeon seed catalogue belongs to the wrong game");
                dungeons.push_back(std::move(dungeon));
            }
        }
        Validate(dungeons);
        ValidateCatalogue(seed, dungeons);
        InstallLogic(seed, false);
    }

    static void ValidateCatalogue(const nlohmann::json& seed, const std::vector<Dungeon>& current) {
        const auto& recorded = seed.at("dungeons");
        if (current.size() != recorded.size())
            throw std::invalid_argument("stale cross-dungeon catalogue size");
        for (const auto& d : current) {
            const auto expected = DungeonJson(d);
            const bool found = std::any_of(recorded.begin(), recorded.end(), [&](auto entry) {
                // The codec accepts old fixtures without an optional empty escape list. Production
                // replays still require every nonempty native alias list to be explicitly recorded.
                if (!entry.contains("escapeExits") && expected.at("escapeExits").empty())
                    entry["escapeExits"] = nlohmann::json::array();
                return entry == expected;
            });
            if (!found)
                throw std::invalid_argument("stale cross-dungeon catalogue, escape aliases, or native entrance layout");
        }
    }

    nlohmann::json GenerateAndInstall(bool enabled, uint64_t masterSeed) {
        if (!enabled) {
            InstallLogic(nullptr); // Initial disabled path performs no host calls or RNG work.
            return nullptr;
        }
        auto seed = GenerateSeed(Catalogue(), masterSeed);
        InstallLogic(seed);
        return seed;
    }

    void InstallLogic(const nlohmann::json& seed, bool validateCatalogue = true) {
        const bool enabled = Enabled(seed);
        if (enabled) {
            if (OptionsFromSeed(seed).bosses && (!hosts[0].GetBossDefeated || !hosts[1].GetBossDefeated))
                throw std::runtime_error("cross-boss oracle exports missing; rebuild both game modules");
            for (const auto& host : hosts)
                if (!host.SetAccess || !host.GetExits)
                    throw std::runtime_error("cross-dungeon oracle exports missing; rebuild both game modules");
            if (validateCatalogue)
                ValidateCatalogue(seed);
        }
        Install(seed, logicSeed, false);
    }

    void BindRuntime(const nlohmann::json& consolidated) {
        // Native runtime setters validate their own entrance tables, independent of the oracle's
        // currently generated graph. A slot load must never compare with another candidate's layout.
        Install(SeedOf(consolidated), runtimeSeed, true);
    }

    template <class Oracle> void ConfigureOracles(Oracle& oot, Oracle& mm) const {
        if (Enabled(logicSeed)) {
            oot.SetDungeonAccess = hosts[0].SetAccess;
            oot.GetDungeonExits = hosts[0].GetExits;
            mm.SetDungeonAccess = hosts[1].SetAccess;
            mm.GetDungeonExits = hosts[1].GetExits;
            if constexpr (HasBossOracle<Oracle>::value && HasArrivalPolicy<Oracle>::value) {
                oot.GetBossDefeated = hosts[0].GetBossDefeated;
                mm.GetBossDefeated = hosts[1].GetBossDefeated;
                oot.CanEnterArrival = hosts[0].CanEnterArrival;
                mm.CanEnterArrival = hosts[1].CanEnterArrival;
                oot.CrossBossSources = mm.CrossBossSources = BossSources(logicSeed);
            }
        } else {
            oot.SetDungeonAccess = mm.SetDungeonAccess = nullptr;
            oot.GetDungeonExits = mm.GetDungeonExits = nullptr;
            if constexpr (HasBossOracle<Oracle>::value && HasArrivalPolicy<Oracle>::value) {
                oot.GetBossDefeated = mm.GetBossDefeated = nullptr;
                oot.CanEnterArrival = mm.CanEnterArrival = nullptr;
                oot.CrossBossSources = mm.CrossBossSources = EmptyBossSources();
            }
        }
    }

    // All generation/reload exits restore the previous oracle map. Destruction cannot unwind over
    // a C callback; native setters are transactional and restore an already accepted seed here.
    class LogicScope {
      public:
        explicit LogicScope(Coordinator& owner) : owner(owner), previous(owner.LogicSeed()) {
        }
        ~LogicScope() noexcept {
            try {
                owner.InstallLogic(previous, false);
            } catch (...) { std::terminate(); }
        }
        LogicScope(const LogicScope&) = delete;
        LogicScope& operator=(const LogicScope&) = delete;

      private:
        Coordinator& owner;
        nlohmann::json previous;
    };

  private:
    nlohmann::json logicSeed;
    nlohmann::json runtimeSeed;

    void Install(const nlohmann::json& seed, nlohmann::json& installed, bool runtime) {
        const bool enabled = Enabled(seed);
        auto next = enabled ? seed : nlohmann::json();
        if (next == installed)
            return;
        std::array<int (*)(const char*), 2> setters{ runtime ? hosts[0].SetRuntimeSeed : hosts[0].SetSeed,
                                                     runtime ? hosts[1].SetRuntimeSeed : hosts[1].SetSeed };
        if (!setters[0] || !setters[1])
            throw std::runtime_error("cross-dungeon seed exports missing; rebuild both game modules");
        const auto raw = next.dump();
        const auto old = installed.dump();
        try {
            if (!setters[0](raw.c_str()) || !setters[1](raw.c_str()))
                throw std::invalid_argument("cross-dungeon seed rejected by native host");
        } catch (...) {
            // Always restore both hosts, including a setter that returned failure after inspecting
            // the candidate. Never leave a half-installed map after a failed replay/attempt.
            const bool restoredOot = setters[0](old.c_str()) != 0;
            const bool restoredMm = setters[1](old.c_str()) != 0;
            if (!restoredOot || !restoredMm)
                throw std::runtime_error("cross-dungeon host rollback failed");
            throw;
        }
        installed = std::move(next);
    }
};

} // namespace ComboRando::CrossDungeon
