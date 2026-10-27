#pragma once
// Closed-leaf collision belongs only to the controller's exact live native door.
// The visible native actor and its Open action remain the interaction authority.
#include <array>

namespace BlackMarketDoorCollision {
template <bool Oot> class Fixture {
  public:
    Fixture() {
        constexpr int faces[4][3] = { { 0, 1, 2 }, { 0, 2, 3 }, { 2, 1, 0 }, { 3, 2, 0 } };
        for (size_t i = 0; i < mPolygons.size(); ++i) {
            auto& p = mPolygons[i];
            p.flags_vIA = faces[i][0] | (1 << 13); // camera exclusion, both native formats
            p.flags_vIB = faces[i][1];
            p.vIC = faces[i][2];
            p.normal = { 0, 0, s16(i < 2 ? 32767 : -32767) };
        }
        mCollision.minBounds = { -30, -8, 0 };
        mCollision.maxBounds = { 30, 110, 0 };
        mCollision.numVertices = mVertices.size();
        mCollision.vtxList = mVertices.data();
        mCollision.numPolygons = mPolygons.size();
        mCollision.polyList = mPolygons.data();
        mCollision.surfaceTypeList = mSurface.data();
        AttachCamera(mCollision);
    }
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;

    // Called after native scene teardown; never dereference a retired PlayState.
    void Reset() {
        mOwner.bgId = -1;
    }
    int OwnedBgId(const PlayState* play) const {
        if (play)
            for (int id = 0; id < BG_ACTOR_MAX; ++id)
                if (Owns(&play->colCtx, id))
                    return id;
        return -1;
    }
    bool Owns(const CollisionContext* context, int id) const {
        if (!context || id < 0 || id >= BG_ACTOR_MAX)
            return false;
        const auto& dyna = context->dyna;
        return (dyna.bgActorFlags[id] & 1) && !(dyna.bgActorFlags[id] & 2) &&
               dyna.bgActors[id].actor == &mOwner.actor && dyna.bgActors[id].colHeader == &mCollision;
    }
    void Release(PlayState* play) {
        const int id = OwnedBgId(play);
        if (id >= 0)
            DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, id);
    }
    bool Ensure(PlayState* play, const Actor* closedDoor) {
        if (!play)
            return false;
        const int owned = OwnedBgId(play);
        if (!closedDoor || !closedDoor->update || !closedDoor->draw || play->roomCtx.curRoom.num != 0 ||
            !play->colCtx.colHeader) {
            Release(play);
            return false;
        }
        if (owned >= 0) {
            if (mOwner.actor.world.pos.x == closedDoor->home.pos.x &&
                mOwner.actor.world.pos.y == closedDoor->home.pos.y &&
                mOwner.actor.world.pos.z == closedDoor->home.pos.z &&
                mOwner.actor.shape.rot.y == closedDoor->home.rot.y)
                return true;
            Release(play);
        }
        if (!HasCapacity(play->colCtx.dyna))
            return false;
        mOwner = {};
        DynaPolyActor_Init(&mOwner, 0);
        mOwner.actor.id = -1;
        mOwner.actor.category = ACTORCAT_BG;
        mOwner.actor.room = 0;
        mOwner.actor.scale = { 1, 1, 1 };
        mOwner.actor.world.pos = mOwner.actor.home.pos = closedDoor->home.pos;
        mOwner.actor.shape.rot.y = closedDoor->home.rot.y;
        mOwner.actor.update = [](Actor*, PlayState*) {};
        const int id = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &mOwner.actor, &mCollision);
        if (id < 0 || id >= BG_ACTOR_MAX)
            return false;
        mOwner.bgId = id;
        return true;
    }

  private:
    static int MaxNodes(const auto& dyna) {
        if constexpr (requires { dyna.polyNodes.maxNodes; })
            return dyna.polyNodes.maxNodes;
        else
            return dyna.polyNodes.max;
    }
    bool HasCapacity(const DynaCollisionContext& dyna) const {
        if (!dyna.polyList || !dyna.vtxList || !dyna.polyNodes.tbl)
            return false;
        size_t polygons = mPolygons.size(), vertices = mVertices.size();
        bool free = false;
        for (int id = 0; id < BG_ACTOR_MAX; ++id) {
            const auto flags = dyna.bgActorFlags[id];
            if (!(flags & 1))
                free = true;
            else if (!(flags & (2 | 4))) {
                const auto* header = dyna.bgActors[id].colHeader;
                if (!header)
                    return false;
                polygons += header->numPolygons;
                vertices += header->numVertices;
            }
        }
        return free && dyna.polyListMax > 0 && dyna.vtxListMax > 0 && MaxNodes(dyna) > 0 &&
               polygons <= size_t(dyna.polyListMax) && vertices <= size_t(dyna.vtxListMax) &&
               polygons <= size_t(MaxNodes(dyna));
    }
    void AttachCamera(auto& header) {
        if constexpr (requires { header.bgCamList; })
            header.bgCamList = mCamera.data();
        else {
            header.cameraDataList = mCamera.data();
            header.cameraDataListLen = mCamera.size();
        }
    }
    // The native default wooden leaf is sixty units wide and one hundred high.
    // Overlap its lintel/floor by a few units; no floor/top/ledge faces are added.
    std::array<Vec3s, 4> mVertices{ { { -30, -8, 0 }, { 30, -8, 0 }, { 30, 110, 0 }, { -30, 110, 0 } } };
    std::array<CollisionPoly, 4> mPolygons{};
    // Both hosts encode WALL_TYPE_1 at bit21: native no-ledge-climb surface.
    std::array<SurfaceType, 1> mSurface{ { { { 1u << 21, 0 } } } };
    std::array<CamData, 1> mCamera{ { { CAM_SET_NONE, 0, nullptr } } };
    CollisionHeader mCollision{};
    DynaPolyActor mOwner{};
};
} // namespace BlackMarketDoorCollision
