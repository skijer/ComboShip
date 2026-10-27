#pragma once

#include "global.h"
#include "z64door.h"
#include <cmath>

// The final Prelude forest supplies a real knob door. Its same-room transition
// entry is the authoring beacon; native room-to-room doors retain their behavior.
namespace BlackMarketDoorNative {
struct Pose {
    Vec3f position{};
    s16 yaw = 0;
    int index = -1;
};
inline bool Marker(const TransitionActorEntry& entry) {
    return (entry.id == ACTOR_EN_DOOR || entry.id == -ACTOR_EN_DOOR) && entry.sides[0].room == 0 &&
           entry.sides[1].room == 0;
}
inline bool HasDoor(const PlayState* play) {
    if (!play || !play->transitionActors.list || play->transitionActors.count > 64)
        return false;
    for (int i = 0; i < play->transitionActors.count; ++i)
        if (Marker(play->transitionActors.list[i]))
            return true;
    return false;
}
inline bool Find(const PlayState* play, Pose& result) {
    if (!play || !play->transitionActors.list || play->transitionActors.count > 64)
        return false;
    bool found = false;
    for (int i = 0; i < play->transitionActors.count; ++i) {
        const auto& entry = play->transitionActors.list[i];
        if (!Marker(entry))
            continue;
        if (found)
            return false; // Never guess between multiple same-room shop doors.
        result = { { static_cast<float>(entry.pos.x), static_cast<float>(entry.pos.y),
                     static_cast<float>(entry.pos.z) },
                   DEG_TO_BINANG((entry.rotY >> 7) & 0x1FF),
                   i };
        found = true;
    }
    return found;
}
inline bool Matches(const Actor* actor, const Pose& pose) {
    return actor && actor->id == ACTOR_EN_DOOR && DOOR_GET_TRANSITION_ID(actor) == pose.index &&
           std::abs(actor->home.pos.x - pose.position.x) < 1 && std::abs(actor->home.pos.y - pose.position.y) < 1 &&
           std::abs(actor->home.pos.z - pose.position.z) < 1;
}
inline Vec3f Outside(const Pose& pose) {
    const float angle = pose.yaw * (6.28318530718f / 65536.0f);
    // Keep the native trailing camera in front of the authored building.
    return { pose.position.x + std::sin(angle) * 226, pose.position.y, pose.position.z + std::cos(angle) * 226 };
}
} // namespace BlackMarketDoorNative
