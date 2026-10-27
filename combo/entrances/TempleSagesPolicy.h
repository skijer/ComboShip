#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace TempleSages {
inline constexpr size_t kPadCount = 5;
inline constexpr int kScene = 0x43, kRoom = 0;
inline constexpr uint16_t kMarkerActor = 0x012E, kMarkerParams = 0xFC00;
inline constexpr float kHeightTolerance = 12.0f;
inline constexpr float kFaceHalfWidth = 38.0f, kFaceDepth = 18.0f;

enum class TravelMode { Off = 0, DungeonRewards = 1, Free = 2, WarpSongs = 3 };

inline constexpr TravelMode NormalizeTravelMode(int value) {
    return value == int(TravelMode::Off)         ? TravelMode::Off
           : value == int(TravelMode::Free)      ? TravelMode::Free
           : value == int(TravelMode::WarpSongs) ? TravelMode::WarpSongs
                                                 : TravelMode::DungeonRewards;
}

struct Point {
    float x, y, z;
};
struct Pad {
    Point position{};
    int16_t yaw = 0;
};
struct Visit {
    const void* owner = nullptr;
    const void* sceneData = nullptr;
    const void* roomData = nullptr;
    int scene = -1, room = -1, setup = -1;
    bool roomReady = false;
};
struct PlayerState {
    Visit visit;
    Point position;
    float floorHeight;
    int floorNormalY;
    uint32_t questItems;
    bool controllable, grounded, floorValid;
    TravelMode travelMode = TravelMode::DungeonRewards;
};

inline bool Finite(Point p) {
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
inline bool LoadedVisit(const Visit& visit) {
    return visit.owner && visit.sceneData && visit.roomData && visit.setup >= 0 && visit.setup < 4 && visit.roomReady;
}
inline bool NormalVisit(const Visit& visit) {
    return LoadedVisit(visit) && visit.scene == kScene && visit.room == kRoom;
}
inline bool Owned(int route, uint32_t questItems) {
    // The five elemental QUEST_MEDALLION_* rewards occupy bits 0..4.
    // Light has no portal: its physical bridge returns to the Temple.
    return route >= 0 && route < int(kPadCount) && (questItems & (uint32_t{ 1 } << route));
}
inline bool Unlocked(int route, uint32_t questItems, TravelMode mode) {
    if (route < 0 || route >= int(kPadCount))
        return false;
    switch (NormalizeTravelMode(int(mode))) {
        case TravelMode::Off:
            return false;
        case TravelMode::Free:
            return true;
        case TravelMode::DungeonRewards:
            return Owned(route, questItems);
        // QUEST_SONG_MINUET..NOCTURNE occupy bits 6..10 in the same route order.
        case TravelMode::WarpSongs:
            return (questItems & (uint32_t{ 1 } << (6 + route))) != 0;
    }
    return false;
}
inline constexpr int Entrance(int route) {
    constexpr int entrances[] = { 0x600, 0x4F6, 0x604, 0x1F1, 0x568 };
    return route >= 0 && route < int(kPadCount) ? entrances[route] : -1;
}

class PadCache {
  public:
    void Reset() {
        visit = {};
        pads = {};
        valid = armed = false;
        activeRoutes = 0;
        previousMode.reset();
    }

    // Accept native ActorEntry directly, keeping scene-owned buffers read-only.
    template <class Entry> void Capture(const Visit& incoming, const Entry* entries, size_t count) {
        Reset();
        if (!NormalVisit(incoming) || !entries || count > 255)
            return;
        unsigned seen = 0;
        for (size_t i = 0; i < count; ++i) {
            const auto& entry = entries[i];
            const auto params = static_cast<uint16_t>(entry.params);
            if (entry.id != kMarkerActor || (params & 0xFFF8u) != kMarkerParams)
                continue;
            const unsigned route = params & 7u;
            // Reject POC1 Light markers too, so old six-marker addons fail closed.
            if (route >= kPadCount || (seen & (1u << route)))
                return;
            const Point position{ float(entry.pos.x), float(entry.pos.y), float(entry.pos.z) };
            if (!Finite(position))
                return;
            pads[route] = { position, static_cast<int16_t>(entry.rot.y) };
            seen |= 1u << route;
        }
        if (seen != ((1u << kPadCount) - 1u))
            return;
        visit = incoming;
        valid = true;
        activeRoutes = (1u << kPadCount) - 1u;
    }

    // Destination placements are independently validated by the native adapter.
    void CaptureReturn(const Visit& incoming, int route, Pad pad) {
        Reset();
        if (!LoadedVisit(incoming) || route < 0 || route >= int(kPadCount) || !Finite(pad.position))
            return;
        visit = incoming;
        pads[route] = pad;
        activeRoutes = 1u << route;
        valid = true;
    }

    bool Ready(const Visit& current) const {
        return valid && LoadedVisit(current) && current.scene == visit.scene && current.room == visit.room &&
               current.owner == visit.owner && current.sceneData == visit.sceneData &&
               current.roomData == visit.roomData && current.setup == visit.setup;
    }

    bool Active(int route) const {
        return route >= 0 && route < int(kPadCount) && (activeRoutes & (1u << route));
    }

    const std::array<Pad, kPadCount>& Pads() const {
        return pads;
    }

    void ObserveMode(TravelMode incoming) {
        const auto mode = NormalizeTravelMode(int(incoming));
        // Observe even while control/readiness blocks interaction.
        if (!previousMode || *previousMode != mode) {
            armed = false;
            previousMode = mode;
        }
    }

    std::optional<int> Tick(const PlayerState& player) {
        const auto mode = NormalizeTravelMode(int(player.travelMode));
        ObserveMode(mode);
        if (!Ready(player.visit) || !player.controllable || !player.grounded || !player.floorValid ||
            player.floorNormalY < 23170 || !std::isfinite(player.floorHeight) || !Finite(player.position))
            return std::nullopt;
        bool outside = true;
        int selected = -1;
        for (int route = 0; route < int(kPadCount); ++route) {
            if (!Active(route))
                continue;
            const auto& point = pads[route].position;
            const float dx = player.position.x - point.x, dz = player.position.z - point.z;
            const float angle = pads[route].yaw * (6.28318530718f / 65536.0f);
            const float side = dx * std::cos(angle) - dz * std::sin(angle);
            const float front = dx * std::sin(angle) + dz * std::cos(angle);
            if (std::fabs(side) >= kFaceHalfWidth || front < -0.01f || front >= kFaceDepth)
                continue;
            outside = false;
            if (Unlocked(route, player.questItems, mode) && std::fabs(player.position.y - point.y) < kHeightTolerance &&
                std::fabs(player.floorHeight - point.y) < kHeightTolerance) {
                if (selected >= 0)
                    return std::nullopt; // An overlapping malformed layout cannot pick a route.
                selected = route;
            }
        }
        if (!armed) {
            armed = outside;
            return std::nullopt;
        }
        if (selected < 0)
            return std::nullopt;
        armed = false;
        return selected;
    }

  private:
    unsigned activeRoutes = 0;
    Visit visit{};
    std::array<Pad, kPadCount> pads{};
    std::optional<TravelMode> previousMode;
    bool valid = false, armed = false;
};
} // namespace TempleSages
