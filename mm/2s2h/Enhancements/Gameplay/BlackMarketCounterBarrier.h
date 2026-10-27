#pragma once
// Include after MM's global.h. This fixture owns collision only; the existing
// room supplies the visible counter, floor, walls, and ceiling.
#include "entrances/BlackMarketEntrance.h"
#include <array>

namespace BlackMarketCounterBarrier {
inline constexpr s16 kFrontZ = 130;
inline constexpr float kCustomerMinZ = 132.0f;

class Fixture {
  public:
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;
    Fixture() {
        // +Z faces the customer aisle. The ends overlap the room's X +/-160
        // walls and Y -7..213 bounds, so there is no reachable edge to bypass.
        // There is deliberately no floor/top face or invisible ledge.
        for (size_t i = 0; i < mPolygons.size(); ++i) {
            auto& poly = mPolygons[i];
            poly.flags_vIA = COLPOLY_VTX(0, COLPOLY_IGNORE_CAMERA);
            poly.flags_vIB = COLPOLY_VTX(i ? 2 : 1, COLPOLY_IGNORE_NONE);
            poly.vIC = i ? 3 : 2;
            poly.normal = { 0, 0, COLPOLY_SNORMAL(1.0f) };
            poly.dist = -kFrontZ;
        }
        mCollision.minBounds = { -170, -16, kFrontZ };
        mCollision.maxBounds = { 170, 300, kFrontZ };
        mCollision.numVertices = mVertices.size();
        mCollision.vtxList = mVertices.data();
        mCollision.numPolygons = mPolygons.size();
        mCollision.polyList = mPolygons.data();
        mCollision.surfaceTypeList = mSurface.data();
        mCollision.bgCamList = mCamera.data();
    }

    // Scene teardown owns the native registration and may have already freed
    // its arena. Touch only our persistent owner; never retain/dereference a
    // retired PlayState. Ensure rediscovers identity if the context is live.
    void Reset() {
        DynaPolyActor_Init(&mOwner, 0);
    }

    bool Owns(const CollisionContext* context, s32 bgId) const {
        if (!context || bgId < 0 || bgId >= BG_ACTOR_MAX)
            return false;
        const auto& dyna = context->dyna;
        return (dyna.bgActorFlags[bgId] & BGACTOR_IN_USE) && !(dyna.bgActorFlags[bgId] & BGACTOR_1) &&
               dyna.bgActors[bgId].actor == &mOwner.actor && dyna.bgActors[bgId].colHeader == &mCollision;
    }

    int OwnedBgId(const PlayState* play) const {
        if (!play)
            return -1;
        for (int id = 0; id < BG_ACTOR_MAX; ++id) {
            if (Owns(&play->colCtx, id))
                return id;
        }
        return -1;
    }

    bool Ensure(PlayState* play) {
        if (!play)
            return false;
        auto& dyna = play->colCtx.dyna;
        const int owned = OwnedBgId(play);
        if (play->sceneId != COMBO_BLACK_MARKET_SCENE || play->roomCtx.curRoom.num != 0 || !play->colCtx.colHeader) {
            if (owned >= 0)
                DynaPoly_DeleteBgActor(play, &dyna, owned);
            return false;
        }
        if (owned >= 0) {
            mOwner.bgId = owned;
            return true;
        }
        if (!HasCapacity(dyna))
            return false;
        mOwner = {};
        DynaPolyActor_Init(&mOwner, 0);
        mOwner.actor.id = -1;
        mOwner.actor.category = ACTORCAT_BG;
        mOwner.actor.room = 0;
        mOwner.actor.scale = { 1, 1, 1 };
        mOwner.actor.update = [](Actor*, PlayState*) {};
        const int id = DynaPoly_SetBgActor(play, &dyna, &mOwner.actor, &mCollision);
        if (id < 0 || id >= BG_ACTOR_MAX)
            return false;
        mOwner.bgId = id;
        return true;
    }

  private:
    bool HasCapacity(const DynaCollisionContext& dyna) const {
        if (!dyna.polyList || !dyna.vtxList || !dyna.polyNodes.tbl)
            return false;
        bool freeSlot = false;
        size_t polygons = mPolygons.size(), vertices = mVertices.size();
        for (int id = 0; id < BG_ACTOR_MAX; ++id) {
            const auto flags = dyna.bgActorFlags[id];
            if (!(flags & BGACTOR_IN_USE)) {
                freeSlot = true;
            } else if (!(flags & (BGACTOR_1 | BGACTOR_COLLISION_DISABLED))) {
                const auto* header = dyna.bgActors[id].colHeader;
                if (!header)
                    return false;
                polygons += header->numPolygons;
                vertices += header->numVertices;
            }
        }
        // Each polygon contributes at most one dynamic node. Reserve against
        // all active headers, including registrations not yet indexed this frame.
        return freeSlot && dyna.polyListMax > 0 && dyna.vtxListMax > 0 && dyna.polyNodes.maxNodes > 0 &&
               polygons <= size_t(dyna.polyListMax) && vertices <= size_t(dyna.vtxListMax) &&
               polygons <= size_t(dyna.polyNodes.maxNodes);
    }

    std::array<Vec3s, 4> mVertices{
        { { -170, -16, kFrontZ }, { 170, -16, kFrontZ }, { 170, 300, kFrontZ }, { -170, 300, kFrontZ } }
    };
    std::array<CollisionPoly, 2> mPolygons{};
    // Native WALL_TYPE_1 resolves to WALL_FLAG_0: the player ledge-climb gate
    // rejects it, with no ordinary climb flags, damage, or hookshot surface.
    std::array<SurfaceType, 1> mSurface{ { { { SURFACETYPE0(0, 0, 0, 0, WALL_TYPE_1, 0, false, false), 0 } } } };
    std::array<CamData, 1> mCamera{ { { CAM_SET_NONE, 0, nullptr } } };
    CollisionHeader mCollision{};
    DynaPolyActor mOwner{};
};
} // namespace BlackMarketCounterBarrier
