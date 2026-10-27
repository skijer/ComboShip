#pragma once
// Include after the host's global.h. All room-independent data is authored here.
#include "BlackMarketShelves.h"
#include <cmath>

namespace BlackMarketShelvesNative {
class Fixture {
  public:
    Fixture(const Fixture&) = delete;
    Fixture& operator=(const Fixture&) = delete;
    Fixture(Fixture&&) = delete;
    Fixture& operator=(Fixture&&) = delete;
    Fixture()
        : Fixture(BlackMarketShelves::Vertices(false), BlackMarketShelves::Vertices(true),
                  { BlackMarketShelves::Corners(BlackMarketShelves::kCollision[0]),
                    BlackMarketShelves::Corners(BlackMarketShelves::kCollision[1]) }) {
    }
    Fixture(const std::vector<BlackMarketShelves::Vertex>& standardVertices,
            const std::vector<BlackMarketShelves::Vertex>& grannyVertices,
            const std::array<std::array<BlackMarketShelves::Point, 8>, 2>& collisionCorners) {
        for (int granny = 0; granny < 2; ++granny) {
            const auto& source = granny ? grannyVertices : standardVertices;
            for (size_t i = 0; i < source.size(); ++i) {
                const auto& s = source[i];
                auto& v = mVertices[granny][i].v;
                v.ob[0] = s.position.x;
                v.ob[1] = s.position.y;
                v.ob[2] = s.position.z;
                v.tc[0] = s.s;
                v.tc[1] = s.t;
                for (int c = 0; c < 4; ++c)
                    v.cn[c] = s.color[c];
            }
        }
        size_t vertex = 0, poly = 0;
        for (const auto& corners : collisionCorners) {
            for (int i = 0; i < 8; ++i)
                mCollisionVertices[vertex + i] = { corners[i].x, corners[i].y, corners[i].z };
            for (const auto& face : BlackMarketShelves::kFaces)
                for (int triangle = 0; triangle < 2; ++triangle) {
                    const int indices[3] = { face[0], face[triangle ? 2 : 1], face[triangle ? 3 : 2] };
                    auto& p = mPolygons[poly++];
                    for (int i = 0; i < 3; ++i)
                        p.vtxData[i] = vertex + indices[i];
                    const auto a = corners[indices[0]], b = corners[indices[1]], c = corners[indices[2]];
                    const float ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z, vx = c.x - a.x, vy = c.y - a.y,
                                vz = c.z - a.z;
                    float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
                    const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
                    nx /= length;
                    ny /= length;
                    nz /= length;
                    p.normal = { s16(std::lround(nx * 32767)), s16(std::lround(ny * 32767)),
                                 s16(std::lround(nz * 32767)) };
                    p.dist = s16(std::lround(-(nx * a.x + ny * a.y + nz * a.z)));
                }
            vertex += 8;
        }
        mCollision.minBounds = mCollision.maxBounds = mCollisionVertices[0];
        for (const auto& v : mCollisionVertices) {
            mCollision.minBounds.x = std::min(mCollision.minBounds.x, v.x);
            mCollision.minBounds.y = std::min(mCollision.minBounds.y, v.y);
            mCollision.minBounds.z = std::min(mCollision.minBounds.z, v.z);
            mCollision.maxBounds.x = std::max(mCollision.maxBounds.x, v.x);
            mCollision.maxBounds.y = std::max(mCollision.maxBounds.y, v.y);
            mCollision.maxBounds.z = std::max(mCollision.maxBounds.z, v.z);
        }
        mCollision.numVertices = mCollisionVertices.size();
        mCollision.vtxList = mCollisionVertices.data();
        mCollision.numPolygons = mPolygons.size();
        mCollision.polyList = mPolygons.data();
        mCollision.surfaceTypeList = mSurface.data();
        AttachCamera(mCollision);
    }
    // The fixture owns a persistent collision-only DynaPolyActor. The native
    // collision context owns its registration; no actor overlay/inventory is used.
    // Scene destroy may run after MM's arena cleanup, so reset without dereferencing it.
    void Reset() {
        mRegistered = false;
        mOwner = {};
    }
    bool Ensure(PlayState* play) {
        if (mRegistered)
            return true;
        if (!play || !play->colCtx.colHeader || play->roomCtx.curRoom.num != 0)
            return false;
        mOwner = {};
        DynaPolyActor_Init(&mOwner, 0);
        mOwner.actor.id = -1;
        mOwner.actor.category = ACTORCAT_BG;
        mOwner.actor.scale = { 1, 1, 1 };
        mOwner.actor.update = [](Actor*, PlayState*) {};
        const int bg = DynaPoly_SetBgActor(play, &play->colCtx.dyna, &mOwner.actor, &mCollision);
        if (bg < 0 || bg >= BG_ACTOR_MAX)
            return false;
        mOwner.bgId = bg;
        mRegistered = true;
        return true;
    }
    void Draw(Gfx*& head, bool granny = false, const std::array<const char*, 2>& textures = {},
              std::array<uint8_t, 4> tint = BlackMarketShelves::kDefaultTint) {
        // An incomplete optional dependency cannot issue null texture loads.
        if (granny && (!textures[0] || !textures[1]))
            return;
        gDPPipeSync(head++);
        gDPSetCycleType(head++, G_CYC_2CYCLE);
        gDPSetRenderMode(head++, G_RM_PASS, G_RM_AA_ZB_OPA_SURF2);
        gSPClearGeometryMode(head++, G_LIGHTING | G_FOG | G_CULL_BOTH | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
        gSPSetGeometryMode(head++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
        gDPSetTextureLUT(head++, G_TT_NONE);
        gDPSetTextureFilter(head++, G_TF_BILERP);
        gDPSetCombineMode(head++, G_CC_MODULATERGB, G_CC_MODULATERGB_PRIM2);
        if (granny)
            tint = { 255, 255, 255, 255 };
        gDPSetPrimColor(head++, 0, 0, tint[0], tint[1], tint[2], 255);
        gSPTexture(head++, 0xffff, 0xffff, 0, G_TX_RENDERTILE, G_ON);
        auto bound = BlackMarketShelves::Material(-1);
        for (size_t box = 0; box < BlackMarketShelves::kBoxes.size(); ++box) {
            // Each variant has immutable native vertices. Frame interpolation
            // cannot observe a CPU rewrite of UVs/colors from another frame.
            gSPVertex(head++, uintptr_t(mVertices[granny ? 1 : 0].data() + box * 24), 24, 0);
            for (int face = 0; face < 6; ++face) {
                const auto material = BlackMarketShelves::FaceMaterial(box, face, granny);
                if (material != bound) {
                    gDPPipeSync(head++);
                    if (material == BlackMarketShelves::Material::Flow ||
                        material == BlackMarketShelves::Material::Veil) {
                        const bool veil = material == BlackMarketShelves::Material::Veil;
                        gDPLoadTextureBlock(head++, BlackMarketShelves::kFlowTexture, G_IM_FMT_RGBA, G_IM_SIZ_16b, 64,
                                            64, 0, G_TX_WRAP, G_TX_WRAP, 6, 6, G_TX_NOLOD, G_TX_NOLOD);
                        gSPScrollTexture(head++, 0, veil ? 1 : -1, veil ? 0 : 1);
                    } else if (material == BlackMarketShelves::Material::Wood) {
                        gDPLoadTextureBlock(head++, textures[0], G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 64, 0, G_TX_WRAP,
                                            G_TX_WRAP, 5, 6, G_TX_NOLOD, G_TX_NOLOD);
                    } else {
                        gDPLoadTextureBlock(head++, textures[1], G_IM_FMT_RGBA, G_IM_SIZ_16b, 64, 64, 0, G_TX_WRAP,
                                            G_TX_WRAP, 6, 6, G_TX_NOLOD, G_TX_NOLOD);
                    }
                    bound = material;
                }
                const int v = face * 4;
                gSP2Triangles(head++, v, v + 1, v + 2, 0, v, v + 2, v + 3, 0);
            }
        }
        gSPTexture(head++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        gDPPipeSync(head++);
    }

  private:
    // Floor-camera queries also visit dynamic polygons. Surface zero refers to
    // this owned, valid no-camera entry instead of a null camera-data list.
    void AttachCamera(auto& header) {
        if constexpr (requires { header.bgCamList; }) {
            header.bgCamList = mCamera.data();
        } else {
            header.cameraDataList = mCamera.data();
            header.cameraDataListLen = mCamera.size();
        }
    }
    std::array<std::array<Vtx, BlackMarketShelves::kBoxes.size() * 24>, 2> mVertices{};
    std::array<Vec3s, 16> mCollisionVertices{};
    std::array<CollisionPoly, 24> mPolygons{};
    std::array<SurfaceType, 1> mSurface{};
    std::array<CamData, 1> mCamera{ { { CAM_SET_NONE, 0, nullptr } } };
    CollisionHeader mCollision{};
    DynaPolyActor mOwner{};
    bool mRegistered = false;
};
} // namespace BlackMarketShelvesNative
