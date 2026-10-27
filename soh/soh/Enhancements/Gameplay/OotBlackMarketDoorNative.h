#pragma once

#include "global.h"
#include <cmath>

// A same-room plain knob door marks the finished private forest's shop.
// OoT transition yaw is already binary angle, unlike MM's packed degree yaw.
namespace OotBlackMarketDoorNative {
struct Pose {
    Vec3f position{};
    s16 yaw = 0;
    int index = -1;
};
inline bool Marker(const TransitionActorEntry& entry) {
    return (entry.id == ACTOR_EN_DOOR || entry.id == -ACTOR_EN_DOOR) && entry.sides[0].room == 0 &&
           entry.sides[1].room == 0 && entry.params == 0;
}
inline bool HasDoor(const PlayState* play) {
    if (!play || !play->transiActorCtx.list || play->transiActorCtx.numActors > 64)
        return false;
    for (int i = 0; i < play->transiActorCtx.numActors; ++i)
        if (Marker(play->transiActorCtx.list[i]))
            return true;
    return false;
}
inline Pose Placement(const TransitionActorEntry& entry, int index) {
    return { { static_cast<float>(entry.pos.x), static_cast<float>(entry.pos.y), static_cast<float>(entry.pos.z) },
             entry.rotY,
             index };
}
inline bool Find(const PlayState* play, Pose& result) {
    if (!play || !play->transiActorCtx.list || play->transiActorCtx.numActors > 64)
        return false;
    bool found = false;
    for (int i = 0; i < play->transiActorCtx.numActors; ++i) {
        const auto& entry = play->transiActorCtx.list[i];
        if (!Marker(entry))
            continue;
        if (found)
            return false; // Never choose between multiple shop markers.
        result = Placement(entry, i);
        found = true;
    }
    return found;
}
inline bool Matches(const Actor* actor, const Pose& pose) {
    return actor && actor->id == ACTOR_EN_DOOR && actor->category == ACTORCAT_DOOR &&
           GET_TRANSITION_ACTOR_INDEX(actor) == pose.index && (actor->params & 0x3FF) == 0 &&
           actor->home.rot.y == pose.yaw && std::abs(actor->home.pos.x - pose.position.x) < 1 &&
           std::abs(actor->home.pos.y - pose.position.y) < 1 && std::abs(actor->home.pos.z - pose.position.z) < 1;
}
inline bool MarkerActor(const PlayState* play, const Actor* actor) {
    if (!play || !actor || !play->transiActorCtx.list || play->transiActorCtx.numActors > 64)
        return false;
    const int index = GET_TRANSITION_ACTOR_INDEX(actor);
    if (index >= play->transiActorCtx.numActors)
        return false;
    const auto& entry = play->transiActorCtx.list[index];
    return Marker(entry) && Matches(actor, Placement(entry, index));
}
inline Vec3f Outside(const Pose& pose) {
    const float angle = pose.yaw * (6.28318530718f / 65536.0f);
    return { pose.position.x + std::sin(angle) * 226, pose.position.y, pose.position.z + std::cos(angle) * 226 };
}
} // namespace OotBlackMarketDoorNative
