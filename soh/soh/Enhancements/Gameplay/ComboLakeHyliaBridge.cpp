#ifdef COMBO_BUILD
#include "ComboLakeHyliaBridge.h"
#include "ComboGameMode.h"
#include "render/RegionalBridgeMaterials.h"
#include "entrances/LakeHyliaBridge.h"
#include "entrances/LakeHyliaSurface.h"
#include "entrances/LostWoodsEntrance.h"
#include "soh/ActorDB.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/frame_interpolation.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "scenes/overworld/spot06/spot06_scene.h"
#include <array>
#include <cmath>
#include <vector>

extern "C" u8 ResourceMgr_FileExists(const char*);
extern "C" bool NeiGi_CanDrawLayers(PlayState*, size_t, size_t, size_t);

namespace {
// OPEN/CLOSE_DISPS redeclare these at block scope; retain their C linkage.
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int);
extern "C" void FrameInterpolation_RecordCloseChild(void);
namespace Lake = ComboLakeHylia;
struct CaveActor {
    DynaPolyActor dyna;
};
CaveActor* sCave = nullptr;
PlayState* sCavePlay = nullptr;
int sActorId = -1;
const Lake::Geometry& Geometry() {
    static const auto geometry = Lake::BuildGeometry();
    return geometry;
}
const Lake::SurfaceGeometry& SurfaceGeometry() {
    static const auto geometry = Lake::BuildSurfaceGeometry();
    return geometry;
}
struct CaveCollision {
    std::vector<Vec3s> vertices;
    std::vector<CollisionPoly> polygons;
    SurfaceType surface{};
    // Surface0 selects camera index0 even for dynamic floors. Native camera
    // lookup dereferences this table, so it must share collision lifetime.
    CamData cameras[1]{ { CAM_SET_NONE, 0, nullptr } };
    CollisionHeader header{};
    CaveCollision() {
        const auto& geometry = Geometry();
        for (const auto& q : geometry.solid) {
            if (!q.collides)
                continue;
            const u16 first = vertices.size();
            for (const auto p : { q.a, q.b, q.c, q.d })
                vertices.push_back({ static_cast<s16>(std::lround(p.x)), static_cast<s16>(std::lround(p.y)),
                                     static_cast<s16>(std::lround(p.z)) });
            auto triangle = [&](u16 a, u16 b, u16 c) {
                const auto va = vertices[a], vb = vertices[b], vc = vertices[c];
                const float ux = vb.x - va.x, uy = vb.y - va.y, uz = vb.z - va.z;
                const float vx = vc.x - va.x, vy = vc.y - va.y, vz = vc.z - va.z;
                float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
                const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
                if (length < .01f)
                    return;
                nx /= length;
                ny /= length;
                nz /= length;
                CollisionPoly polygon{};
                polygon.vtxData[0] = a;
                polygon.vtxData[1] = b;
                polygon.vtxData[2] = c;
                polygon.normal = { static_cast<s16>(std::lround(nx * 32767)), static_cast<s16>(std::lround(ny * 32767)),
                                   static_cast<s16>(std::lround(nz * 32767)) };
                polygon.dist = static_cast<s16>(std::lround(-(nx * va.x + ny * va.y + nz * va.z)));
                polygons.push_back(polygon);
            };
            triangle(first, first + 1, first + 2);
            triangle(first, first + 2, first + 3);
        }
        header.minBounds = { -2400, -1500, 3550 };
        header.maxBounds = { -1700, -1000, 4100 };
        header.numVertices = vertices.size();
        header.vtxList = vertices.data();
        header.numPolygons = polygons.size();
        header.polyList = polygons.data();
        header.surfaceTypeList = &surface;
        header.cameraDataList = cameras;
        header.cameraDataListLen = 1;
        // No waterboxes, exit list, native resource or progression state is changed.
    }
};
CaveCollision& Collision() {
    static CaveCollision collision;
    return collision;
}
bool Available(PlayState* play) {
    return play && Combo_IsCombinedGame() && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && play->sceneNum == SCENE_LAKE_HYLIA &&
           play->roomCtx.curRoom.num == 0 && play->colCtx.colHeader;
}
void CaveInit(Actor* actor, PlayState* play) {
    auto* cave = reinterpret_cast<CaveActor*>(actor);
    Actor_SetScale(actor, 1);
    DynaPolyActor_Init(&cave->dyna, 0);
    cave->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, actor, &Collision().header);
    if (cave->dyna.bgId < 0 || cave->dyna.bgId >= BG_ACTOR_MAX) {
        cave->dyna.bgId = -1;
        Actor_Kill(actor);
        return;
    }
    actor->room = 0;
    sCave = cave;
    sCavePlay = play;
}
void CaveDestroy(Actor* actor, PlayState* play) {
    auto* cave = reinterpret_cast<CaveActor*>(actor);
    if (cave->dyna.bgId >= 0 && cave->dyna.bgId < BG_ACTOR_MAX) {
        DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, cave->dyna.bgId);
        cave->dyna.bgId = -1;
    }
    if (sCave == cave) {
        sCave = nullptr;
        sCavePlay = nullptr;
    }
}
void CaveUpdate(Actor* actor, PlayState* play) {
    if (!Available(play))
        Actor_Kill(actor);
}
Vtx Vertex(Lake::Point p, u8 shade, s16 s, s16 t) {
    Vtx vertex{};
    vertex.v.ob[0] = std::lround(p.x);
    vertex.v.ob[1] = std::lround(p.y);
    vertex.v.ob[2] = std::lround(p.z);
    vertex.v.tc[0] = s;
    vertex.v.tc[1] = t;
    vertex.v.cn[0] = vertex.v.cn[1] = vertex.v.cn[2] = shade;
    vertex.v.cn[3] = 255;
    return vertex;
}
const std::vector<Vtx>& RockVertices(bool customStone = false, bool customSand = false) {
    static const auto vertices = [] {
        std::array<std::vector<Vtx>, 4> variants;
        for (size_t variant = 0; variant < variants.size(); ++variant) {
            auto& result = variants[variant];
            for (bool sand : { false, true }) {
                const auto& quads = sand ? SurfaceGeometry().sand : SurfaceGeometry().stone;
                const float sScale = sand && !(variant & 2) ? 2.f : 4.f;
                const float tScale = sand ? sScale : variant & 1 ? 8.f : 4.f;
                for (const auto& q : quads) {
                    const float ux = q.b.x - q.a.x, uy = q.b.y - q.a.y, uz = q.b.z - q.a.z;
                    const float vx = q.d.x - q.a.x, vy = q.d.y - q.a.y, vz = q.d.z - q.a.z;
                    const float nx = std::fabs(uy * vz - uz * vy), ny = std::fabs(uz * vx - ux * vz),
                                nz = std::fabs(ux * vy - uy * vx);
                    // Native bank-like texel density, anchored in world space. Adjacent
                    // cave rings continue the same rock instead of restarting moss bands.
                    const bool end = nx >= ny && nx >= nz;
                    const bool wall = end || nz >= ny;
                    for (const auto p : { q.a, q.b, q.c, q.d }) {
                        const float s = (end ? p.z - Lake::kMouth.z : p.x - Lake::kMouth.x) * sScale;
                        const float t =
                            (wall ? Lake::kMouth.y + Lake::kOpeningHeight - p.y : p.z - Lake::kMouth.z) * tScale;
                        result.push_back(Vertex(p, q.shade, static_cast<s16>(s), static_cast<s16>(t)));
                    }
                }
            }
        }
        return variants;
    }();
    return vertices[unsigned(customStone) | (unsigned(customSand) << 1)];
}
void CaveDraw(Actor* actor, PlayState* play) {
    if (!Available(play) || reinterpret_cast<CaveActor*>(actor) != sCave ||
        !NeiGi_CanDrawLayers(play, 3 + (Lake::kMaxWaterQuads * 4 * sizeof(Vtx) + sizeof(Mtx) - 1) / sizeof(Mtx),
                             (SurfaceGeometry().stone.size() + SurfaceGeometry().sand.size()) * 2 + 64,
                             Lake::kMaxWaterQuads * 3 + 40))
        return;
    const char* stone =
        RegionalBridgeMaterials::Resolve(RegionalBridgeMaterials::Material::LakeStone, ResourceMgr_FileExists);
    const char* sand =
        RegionalBridgeMaterials::Resolve(RegionalBridgeMaterials::Material::LakeSand, ResourceMgr_FileExists);
    const bool customStone =
        stone == RegionalBridgeMaterials::OverridePath(RegionalBridgeMaterials::Material::LakeStone);
    const bool customSand = sand == RegionalBridgeMaterials::OverridePath(RegionalBridgeMaterials::Material::LakeSand);
    const auto& vertices = RockVertices(customStone, customSand);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPComboRMPush(POLY_OPA_DISP++, "oot");
    Matrix_Push();
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BOTH);
    gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    // SetupDL25 uses two cycles. Keep this tile's modulation through cycle2;
    // repeating TEXEL0 there instead reads the inherited scene's second tile.
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEI, G_CC_PASS2);
    if (customStone) {
        // Preserve the established 32x64 optional stone replacement contract.
        gDPLoadTextureBlock(POLY_OPA_DISP++, stone, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 64, 0, G_TX_WRAP, G_TX_WRAP, 5, 6,
                            G_TX_NOLOD, G_TX_NOLOD);
    } else {
        gDPLoadTextureBlock(POLY_OPA_DISP++, stone, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, 5, 5,
                            G_TX_NOLOD, G_TX_NOLOD);
    }
    const size_t sandStart = SurfaceGeometry().stone.size() * 4;
    for (size_t i = 0; i < sandStart; i += 4) {
        gSPVertex(POLY_OPA_DISP++, (uintptr_t)&vertices[i], 4, 0);
        gSP2Triangles(POLY_OPA_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    if (customSand) {
        gDPLoadTextureBlock(POLY_OPA_DISP++, sand, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, 5, 5,
                            G_TX_NOLOD, G_TX_NOLOD);
    } else {
        gDPLoadTextureBlock(POLY_OPA_DISP++, sand, G_IM_FMT_RGBA, G_IM_SIZ_16b, 16, 16, 0, G_TX_WRAP, G_TX_WRAP, 4, 4,
                            G_TX_NOLOD, G_TX_NOLOD);
    }
    for (size_t i = sandStart; i < vertices.size(); i += 4) {
        gSPVertex(POLY_OPA_DISP++, (uintptr_t)&vertices[i], 4, 0);
        gSP2Triangles(POLY_OPA_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    gSPComboRMPop(POLY_OPA_DISP++);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    // Frame-owned stream vertices survive queued rendering; no mutable cached
    // resource or previous-frame vertex buffer is retained across Alt/re-entry.
    float waterY = Lake::kDrainedWaterY;
    WaterBox* water = nullptr;
    const bool inlet =
        WaterBox_GetSurface1(play, &play->colCtx, -1900, 3757, &waterY, &water) && Lake::HasInlet(waterY);
    const auto waterMesh = inlet ? Lake::BuildWaterGeometry(waterY) : std::vector<Lake::Quad>{};
    auto* stream =
        inlet ? static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, waterMesh.size() * 4 * sizeof(Vtx))) : nullptr;
    if (stream) {
        const char* waterTexture =
            RegionalBridgeMaterials::Resolve(RegionalBridgeMaterials::Material::LakeWater, ResourceMgr_FileExists);
        const bool customWater =
            waterTexture == RegionalBridgeMaterials::OverridePath(RegionalBridgeMaterials::Material::LakeWater);
        // Preserve the existing replacement pack's logical I8 64x64 contract;
        // the actual native lake wave tile is RGBA16 32x32.
        const float tilePeriod = (customWater ? 64.f : 32.f) * 32;
        const float scroll = (play->gameplayFrames % 54) * (tilePeriod / 54);
        size_t index = 0;
        for (const auto& q : waterMesh)
            for (auto p : { q.a, q.b, q.c, q.d }) {
                // Increasing path distance runs outward, then vertically
                // downward. X alone cannot animate the falling water face.
                stream[index++] = Vertex(p, 255, (p.z - 3757) * (tilePeriod / 48),
                                         s16(Lake::WaterFlowDistance(p) * (tilePeriod / 96) - scroll));
            }
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gSPComboRMPush(POLY_XLU_DISP++, "oot");
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BOTH);
        gDPSetRenderMode(POLY_XLU_DISP++, G_RM_FOG_SHADE_A, G_RM_AA_ZB_XLU_SURF2);
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA_PRIM, G_CC_PASS2);
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        if (customWater) {
            gDPLoadTextureBlock(POLY_XLU_DISP++, waterTexture, G_IM_FMT_I, G_IM_SIZ_8b, 64, 64, 0, G_TX_WRAP, G_TX_WRAP,
                                6, 6, G_TX_NOLOD, G_TX_NOLOD);
        } else {
            gDPLoadTextureBlock(POLY_XLU_DISP++, waterTexture, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP,
                                G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
        }
        for (size_t i = 0; i < index; i += 4) {
            const auto& q = waterMesh[i / 4];
            const float x = (q.a.x + q.d.x) / 2;
            const float light = std::clamp((x - Lake::kBackX) / (-1825 - Lake::kBackX), 0.f, 1.f);
            const bool falling = std::fabs(q.d.x - q.a.x) < .001f;
            const u8 shade = u8(140 + 80 * light);
            // Standard fog uses vertex alpha for its fog factor. Primitive
            // alpha retains water opacity in fog. A uniform curtain avoids
            // horizontal opacity bands across its separate falling quads.
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, shade, shade, shade, falling ? 180 : u8(120 + 35 * light));
            gSPVertex(POLY_XLU_DISP++, (uintptr_t)&stream[i], 4, 0);
            gSP2Triangles(POLY_XLU_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
        }
        gSPComboRMPop(POLY_XLU_DISP++);
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    }
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}
void RegisterCave() {
    ActorDBInit profile;
    profile.name = "ComboLakeHyliaCave";
    profile.desc = "Fixed bank cave below the Lake Hylia laboratory";
    profile.category = ACTORCAT_BG;
    profile.objectId = OBJECT_GAMEPLAY_KEEP;
    profile.instanceSize = sizeof(CaveActor);
    profile.flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    profile.init = CaveInit;
    profile.destroy = CaveDestroy;
    profile.update = CaveUpdate;
    profile.draw = CaveDraw;
    sActorId = ActorDB::Instance->AddEntry(profile).entry.id;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16) {
        // A preceding scene-init consumer may already have requested this cave.
        // Preserve that same-arena actor; ordinary destruction clears old ownership.
        if (sCavePlay != gPlayState) {
            sCave = nullptr;
            sCavePlay = nullptr;
        }
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        sCave = nullptr;
        sCavePlay = nullptr;
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>(
        [] { ComboLakeHyliaBridge_Get(gPlayState, nullptr, nullptr); });
}
RegisterShipInitFunc init(RegisterCave);
} // namespace
extern "C" int ComboLakeHyliaBridge_Get(PlayState* play, Vec3f* mouth, s16* yaw) {
    if (!Available(play) || sActorId < 0)
        return 0;
    if (!sCave || sCavePlay != play) {
        // Exactly one authored provider; no placement ray casts or archive prerequisites.
        Actor_Spawn(&play->actorCtx, play, sActorId, 0, 0, 0, 0, 0, 0, 0);
    }
    if (!sCave || sCavePlay != play || !sCave->dyna.actor.update || sCave->dyna.bgId < 0 ||
        sCave->dyna.bgId >= BG_ACTOR_MAX)
        return 0;
    if (mouth)
        *mouth = { Lake::kMouth.x, Lake::kMouth.y, Lake::kMouth.z };
    if (yaw)
        *yaw = Lake::kYaw;
    return 1;
}
#endif
