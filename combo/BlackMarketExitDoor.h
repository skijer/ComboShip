#pragma once

#include "global.h"
#include "entrances/BlackMarketEntrance.h"
#include <cmath>
#include <cstdint>

// Native Granny doorway, inside the unchanged room/collision. The build-time
// scene adapter reads these same constants for each host's transition actor.
namespace BlackMarketExitDoor {
inline constexpr int16_t kX = -10;
inline constexpr int16_t kY = 0;
inline constexpr int16_t kZ = 420;
inline constexpr int16_t kYaw = 0;
inline constexpr int16_t kMmPackedYaw = 127; // zero degrees, CS_ID_NONE
inline constexpr int16_t kMmActorId = 5;
inline constexpr int16_t kOotActorId = 9;
inline constexpr int16_t kMmParams = 7;    // WHOLE_DAY, all half-days
inline constexpr int16_t kOotParams = 384; // DOOR_SCENEEXIT
inline constexpr int16_t kFarExitZ = 440;  // behind the leaf; floor ends at 458

template <bool Oot, class Play> const TransitionActorEntry* Entry(const Play* play) {
    if (!play || play->roomCtx.curRoom.num != 0)
        return nullptr;
    const TransitionActorEntry* entries;
    int count;
    if constexpr (Oot) {
        entries = play->transiActorCtx.list;
        count = play->transiActorCtx.numActors;
    } else {
        entries = play->transitionActors.list;
        count = play->transitionActors.count;
    }
    if (!entries || count != 1)
        return nullptr;
    const auto& entry = entries[0];
    if ((entry.id != ACTOR_EN_DOOR && entry.id != -ACTOR_EN_DOOR) || entry.sides[0].room != 0 ||
        entry.sides[1].room != -1 || entry.pos.x != kX || entry.pos.y != kY || entry.pos.z != kZ ||
        entry.rotY != (Oot ? kYaw : kMmPackedYaw) || entry.params != (Oot ? kOotParams : kMmParams))
        return nullptr;
    return &entry;
}

template <bool Oot, class Play> bool Matches(const Play* play, const Actor* actor) {
    static_assert(ACTOR_EN_DOOR == (Oot ? kOotActorId : kMmActorId));
    return Entry<Oot>(play) && actor && actor->id == ACTOR_EN_DOOR && actor->category == ACTORCAT_DOOR &&
           actor->room == 0 && actor->params == (Oot ? kOotParams : kMmParams) && actor->home.rot.y == kYaw &&
           std::abs(actor->home.pos.x - kX) < 1 && std::abs(actor->home.pos.y - kY) < 1 &&
           std::abs(actor->home.pos.z - kZ) < 1;
}

template <bool Oot, class Play> const Actor* Live(const Play* play) {
    if (!Entry<Oot>(play))
        return nullptr;
    const Actor* actor;
    if constexpr (Oot)
        actor = play->actorCtx.actorLists[ACTORCAT_DOOR].head;
    else
        actor = play->actorCtx.actorLists[ACTORCAT_DOOR].first;
    // Inspect live native list membership rather than caching an actor across
    // scene reload/destruction. Killed or not-yet-drawable doors keep the exit.
    for (int budget = 64; actor && budget > 0; actor = actor->next, --budget)
        if (Matches<Oot>(play, actor) && actor->update && actor->draw)
            return actor;
    return nullptr;
}

template <bool Oot, class Play> bool WalkExit(const Play* play, const Vec3f& position) {
    // A normal native door leaf has no solid collision. Do not fire the old
    // trigger in front of its Open prompt, or strand a walk-through behind it.
    return ComboBlackMarket_AtExit(position.x, position.y, position.z) && (!Live<Oot>(play) || position.z >= kFarExitZ);
}
} // namespace BlackMarketExitDoor
