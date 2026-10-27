#pragma once
#include "CrossDungeonSeed.h"
#include <optional>
#include <mutex>

namespace ComboRando::CrossDungeon {
// OoT's native Off/On dungeon and master entrance flags are 0/1. Gameplay needs
// their safety behavior for a bound cross map, while stored settings stay native.
inline uint8_t EffectiveEntranceSetting(uint8_t nativeValue, bool crossEnabled, bool entranceRule) {
    return crossEnabled && entranceRule && nativeValue == 0 ? 1 : nativeValue;
}
// Optional native save metadata is fail-closed and backward compatible with saves predating the feature.
inline int32_t ReadCheckpoint(const nlohmann::json& save) {
    auto it = save.find("crossDungeonCheckpoint");
    if (it == save.end() || !it->is_number_integer())
        return -1;
    const auto value = it->get<int64_t>();
    return value >= 0 && value <= 0xFFFF ? static_cast<int32_t>(value) : -1;
}
// One field serves a frozen source checkpoint and an active destination dungeon checkpoint.
// Restoring a dungeon does not release its ownership; only an actual outside scene does.
class CheckpointState {
  public:
    void Load(int32_t index) {
        std::lock_guard lock(mutex);
        saved = recovery = index;
    }
    void SetSource(int32_t index) {
        std::lock_guard lock(mutex);
        saved = index;
    }
    void Arrive(int32_t index, bool dungeon) {
        std::lock_guard lock(mutex);
        saved = dungeon ? index : -1;
        recovery = -1;
    }
    int32_t Saved() const {
        std::lock_guard lock(mutex);
        return saved;
    }
    template <typename IsDungeon> int32_t ConsumeRecovery(IsDungeon isDungeon) {
        std::lock_guard lock(mutex);
        const int32_t result = recovery;
        recovery = -1;
        if (result >= 0)
            saved = isDungeon(result) ? result : -1;
        return result;
    }
    void ObserveScene(bool insideDungeon) {
        if (!insideDungeon) {
            std::lock_guard lock(mutex);
            saved = -1;
        }
    }

  private:
    mutable std::mutex mutex;
    int32_t saved = -1;
    int32_t recovery = -1;
};
// Gameplay owns this state independently of the temporary reachability/oracle access state.
class RuntimeState {
  public:
    struct Transition {
        Endpoint to;
        Endpoint checkpoint; // Safe native source endpoint persisted before another host runs.
        std::optional<Endpoint> arrivalCheckpoint = std::nullopt;
    };
    struct BossRecovery {
        Endpoint arena;  // Physical identity retained independently of source recovery.
        Endpoint source; // The boss ingress used to reach the physical arena.
        Endpoint parent; // Safe recovery within that source's native parent.
        bool operator==(const BossRecovery&) const = default;
    };
    explicit RuntimeState(int host) : game(host) {
    }

    void Install(const nlohmann::json& seed) {
        auto next = Decode(seed); // Transactional: malformed seeds do not clear a queued handoff.
        std::vector<Dungeon> nextDungeons;
        if (!next.empty())
            for (const auto& d : seed.at("dungeons"))
                nextDungeons.push_back(ParseDungeon(d));
        if (routes != next) {
            arrival = -1;
            bossRecoveryResume.reset();
        }
        routes = std::move(next);
        dungeons = std::move(nextDungeons);
        if (bossRecoveryResume && RecoveryForBoss(bossRecoveryResume->arena) != bossRecoveryResume)
            bossRecoveryResume.reset(); // Recovery-only metadata can change without changing route endpoints.
    }
    bool Enabled() const {
        return !routes.empty();
    }
    std::optional<Transition> Lookup(Phase phase, int32_t index) const {
        for (const auto& r : routes) {
            if (r.phase != phase || r.from != Endpoint{ game, index })
                continue;
            if (phase == Phase::BeforeNative)
                for (const auto& d : dungeons)
                    if (d.pool == Pool::Boss && d.entrance.game == game &&
                        ((d.hasReverse && d.exit == r.from) ||
                         std::find(d.escapeExits.begin(), d.escapeExits.end(), r.from) != d.escapeExits.end()))
                        return Transition{ r.to, d.entrance, r.arrivalCheckpoint };
            for (const auto& d : dungeons) {
                if (phase == Phase::AfterNative && d.entrance == r.from)
                    return Transition{ r.to, d.returnTo, r.arrivalCheckpoint };
                if (phase == Phase::BeforeNative && d.entrance.game == game &&
                    (d.exit == r.from ||
                     std::find(d.escapeExits.begin(), d.escapeExits.end(), r.from) != d.escapeExits.end()))
                    return Transition{ r.to, d.entrance, r.arrivalCheckpoint };
            }
            throw std::logic_error("cross-dungeon route lacks its source checkpoint");
        }
        return std::nullopt;
    }
    bool IsDungeonEntrance(int32_t index) const {
        return std::any_of(dungeons.begin(), dungeons.end(), [&](const Dungeon& d) {
            return d.entrance == Endpoint{ game, index };
        });
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
    std::optional<BossRecovery> RecoveryForBoss(Endpoint arena) const {
        const auto target = std::find_if(dungeons.begin(), dungeons.end(),
                                         [&](const Dungeon& d) { return d.pool == Pool::Boss && d.entrance == arena; });
        if (target == dungeons.end())
            return std::nullopt;
        for (const auto& route : routes) {
            if (route.phase != Phase::AfterNative || route.to != arena)
                continue;
            for (const auto& source : dungeons)
                if (source.pool == Pool::Boss && source.entrance == route.from)
                    return BossRecovery{ arena, source.entrance,
                                         source.deathCheckpoint.value_or(
                                             source.reverseCheckpoint.value_or(source.returnTo)) };
        }
        return std::nullopt;
    }
    bool QueueBossRecovery(Endpoint arena) {
        const auto recovery = RecoveryForBoss(arena);
        // Unlike a general arrival queue, this can only resume the inverse ingress's own host.
        if (!recovery || arena.game == game || recovery->source.game != game)
            return false;
        bossRecoveryResume = recovery;
        return true;
    }
    std::optional<BossRecovery> ConsumeBossRecovery() {
        const auto recovery = bossRecoveryResume;
        bossRecoveryResume.reset();
        return recovery;
    }
    bool HasBossRecovery() const {
        return bossRecoveryResume.has_value();
    }
    void ClearBossRecovery() {
        bossRecoveryResume.reset();
    }
    bool IsRecoveryEntrance(int32_t index) const {
        if (IsDungeonEntrance(index))
            return true;
        for (const auto& r : routes)
            if (r.arrivalCheckpoint && *r.arrivalCheckpoint == Endpoint{ game, index })
                return true;
        for (const auto& d : dungeons)
            if (d.pool == Pool::Boss && d.entrance.game == game &&
                (d.returnTo.index == index || (d.reverseCheckpoint && d.reverseCheckpoint->index == index) ||
                 (d.deathCheckpoint && d.deathCheckpoint->index == index)))
                return true;
        return false;
    }
    int32_t RecoveryForArrival(int32_t index) const {
        for (const auto& r : routes)
            if (r.to == Endpoint{ game, index } && r.arrivalCheckpoint)
                return r.arrivalCheckpoint->game == game ? r.arrivalCheckpoint->index : -1;
        return IsRecoveryEntrance(index) ? index : -1;
    }
    bool QueueArrival(int32_t index) {
        // The peer may only queue a destination actually present in this seed for this host.
        for (const auto& r : routes) {
            if (r.to == Endpoint{ game, index }) {
                arrival = index;
                bossRecoveryResume.reset();
                return true;
            }
        }
        return false;
    }
    int32_t ConsumeArrival() {
        int32_t result = arrival;
        arrival = -1;
        return result;
    }

  private:
    int game;
    int32_t arrival = -1;
    std::vector<Route> routes;
    std::vector<Dungeon> dungeons;
    std::optional<BossRecovery> bossRecoveryResume;
};
} // namespace ComboRando::CrossDungeon
