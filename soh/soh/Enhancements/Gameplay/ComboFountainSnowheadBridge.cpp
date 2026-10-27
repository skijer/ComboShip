#ifdef COMBO_BUILD
#include "ComboFountainSnowheadBridge.h"
#include "ComboGameMode.h"
#include "ComboFountainValley.h"
#include "render/RegionalSnow.h"
#include "soh/Enhancements/Graphics/SeasonWinterMist.h"
#include "objects/gameplay_keep/gameplay_keep.h"

#include "entrances/FountainSnowheadBridge.h"
#include "entrances/LostWoodsEntrance.h"
#include "soh/ActorDB.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#include "soh/frame_interpolation.h"
#include <algorithm>
#include <array>
#include <cmath>
extern "C" u8 ResourceMgr_FileExists(const char*);
extern "C" bool NeiGi_CanDrawLayers(PlayState*, size_t, size_t, size_t);
namespace {
// OPEN/CLOSE_DISPS redeclare these at block scope; retain their C linkage.
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int);
extern "C" void FrameInterpolation_RecordCloseChild(void);
using namespace ComboFountainSnowhead;
struct SnowheadActor {
    DynaPolyActor dyna;
    uint32_t snowTick;
};
// Process-owned arrays survive native Actor_Destroy/DynaPoly_DeleteBgActor.
// Draw and collision are generated from exactly the same authored triangles.
const Mesh sMesh = BuildMesh();
std::array<Vec3s, 192 * 3> sVertices{};
std::array<CollisionPoly, 192> sPolys{};
std::array<Vtx, 192 * 3> sDrawVertices{};
SurfaceType sSurface[1]{};
Vec3s sCameraPos[1]{};
CamData sCamera[1] = { { CAM_SET_NONE, 0, sCameraPos } };
CollisionHeader sCollision{};
bool sBuilt = false;
SnowheadActor* sActor = nullptr;
PlayState* sOwner = nullptr;
int sActorId = -1;
bool sSpawnReported = false;
bool Fountain(const PlayState* play) {
    return play && Combo_IsCombinedGame() && gSaveContext.gameMode == GAMEMODE_NORMAL &&
           ComboLostWoods_IsPlayableSlot(gSaveContext.fileNum) && play->sceneNum == SCENE_ZORAS_FOUNTAIN &&
           play->roomCtx.curRoom.num == 0;
}
void Build() {
    if (sBuilt)
        return;
    std::array<float, kStationCount> distance{};
    for (size_t i = 1; i < kStationCount; ++i)
        distance[i] =
            distance[i - 1] + std::hypot(kStations[i].x - kStations[i - 1].x, kStations[i].z - kStations[i - 1].z);
    for (size_t i = 0; i < sMesh.count; ++i) {
        const auto& t = sMesh.triangles[i];
        const Point points[] = { t.a, t.b, t.c };
        for (size_t j = 0; j < 3; ++j) {
            auto& v = sVertices[i * 3 + j];
            v = { s16(std::lround(points[j].x)), s16(std::lround(points[j].y)), s16(std::lround(points[j].z)) };
            auto& draw = sDrawVertices[i * 3 + j].v;
            draw.ob[0] = v.x;
            draw.ob[1] = v.y;
            draw.ob[2] = v.z;
            // Snow is world X/Z. Side S follows station arclength and side T
            // follows height, continuing through corners and both quad halves.
            const size_t segment = i / 6;
            const float start = std::hypot(points[j].x - kStations[segment].x, points[j].z - kStations[segment].z);
            const float end =
                std::hypot(points[j].x - kStations[segment + 1].x, points[j].z - kStations[segment + 1].z);
            const float sideS = distance[segment + (end < start)];
            draw.tc[0] = s16(std::lround((t.snow ? points[j].x : sideS) * 4));
            draw.tc[1] = s16(std::lround((t.snow ? points[j].z : -points[j].y) * 4));
            const auto color = VertexColor(t, points[j]);
            draw.cn[0] = color[0];
            draw.cn[1] = color[1];
            draw.cn[2] = color[2];
            draw.cn[3] = 255;
        }
        const auto a = sVertices[i * 3], b = sVertices[i * 3 + 1], c = sVertices[i * 3 + 2];
        float ux = b.x - a.x, uy = b.y - a.y, uz = b.z - a.z, vx = c.x - a.x, vy = c.y - a.y, vz = c.z - a.z;
        float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
        float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        nx /= len;
        ny /= len;
        nz /= len;
        auto& p = sPolys[i];
        p.type = 0;
        p.flags_vIA = i * 3;
        p.flags_vIB = i * 3 + 1;
        p.vIC = i * 3 + 2;
        p.normal = { s16(std::lround(nx * 32767)), s16(std::lround(ny * 32767)), s16(std::lround(nz * 32767)) };
        p.dist = s16(std::lround(-(nx * a.x + ny * a.y + nz * a.z)));
    }
    sCollision.minBounds = sCollision.maxBounds = sVertices.front();
    for (size_t i = 1; i < sMesh.count * 3; ++i) {
        const auto& v = sVertices[i];
        auto& min = sCollision.minBounds;
        auto& max = sCollision.maxBounds;
        min.x = std::min(min.x, v.x);
        min.y = std::min(min.y, v.y);
        min.z = std::min(min.z, v.z);
        max.x = std::max(max.x, v.x);
        max.y = std::max(max.y, v.y);
        max.z = std::max(max.z, v.z);
    }
    sCollision.numVertices = sMesh.count * 3;
    sCollision.vtxList = sVertices.data();
    sCollision.numPolygons = sMesh.count;
    sCollision.polyList = sPolys.data();
    sCollision.surfaceTypeList = sSurface;
    // Native cameras query dynamic floors even with surface0/no camera override.
    // Keep the indexed CAM_SET_NONE record alive for the entire process.
    sCollision.cameraDataList = sCamera;
    sCollision.cameraDataListLen = 1;
    // Native water, exits, actor lists and scene collision are never edited.
    sBuilt = true;
}
void Destroy(Actor* actor, PlayState* play) {
    auto* self = reinterpret_cast<SnowheadActor*>(actor);
    if (self->dyna.bgId >= 0 && self->dyna.bgId < BG_ACTOR_MAX)
        DynaPoly_DeleteBgActor(play, &play->colCtx.dyna, self->dyna.bgId);
    if (sActor == self) {
        sActor = nullptr;
        sOwner = nullptr;
    }
}
void Init(Actor* actor, PlayState* play) {
    auto* self = reinterpret_cast<SnowheadActor*>(actor);
    DynaPolyActor_Init(&self->dyna, 0);
    Actor_SetScale(actor, 1.0f);
    actor->room = 0;
    self->snowTick = 0;
    Build();
    self->dyna.bgId = DynaPoly_SetBgActor(play, &play->colCtx.dyna, actor, &sCollision);
    LUSLOG_INFO("[SnowheadBridge] collision actor initialized: age=%d layer=%d bgId=%d polygons=%u",
                gSaveContext.linkAge, gSaveContext.sceneLayer, self->dyna.bgId, unsigned(sCollision.numPolygons));
    if (self->dyna.bgId < 0 || self->dyna.bgId >= BG_ACTOR_MAX) {
        Actor_Kill(actor);
        return;
    }
    sActor = self;
    sOwner = play;
}
void Update(Actor* actor, PlayState* play) {
    if (!Fountain(play) || !ComboFountainValley_Ready(play)) {
        Actor_Kill(actor);
        return;
    }
    if (!play->state.running || play->pauseCtx.state || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        play->transitionMode != TRANS_MODE_OFF || play->gameOverCtx.state != GAMEOVER_INACTIVE || Player_InCsMode(play))
        return;
    auto* player = GET_PLAYER(play);
    if (!player)
        return;
    ++reinterpret_cast<SnowheadActor*>(actor)->snowTick;
}
void DrawLocalChill(Actor* actor, PlayState* play);

// Use the actual view, rather than player distance alone: a camera can cross
// an anchored particle while the player remains in the same place.
float ParticleViewOpacity(PlayState* play, const Point& position, float radius) {
    const auto& eye = play->view.eye;
    const auto& at = play->view.lookAt;
    const float dx = at.x - eye.x, dy = at.y - eye.y, dz = at.z - eye.z;
    const float length = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (length < .001f)
        return 0;
    const float depth = ((position.x - eye.x) * dx + (position.y - eye.y) * dy + (position.z - eye.z) * dz) / length;
    return RegionalSnow::DepthOpacity(depth, radius, std::max(0.f, play->view.zNear),
                                      play->view.zFar > 200 ? play->view.zFar : 12800.f);
}

void Draw(Actor* actor, PlayState* play) {
    if (!Fountain(play) ||
        !NeiGi_CanDrawLayers(play, 3 + RegionalSnow::kCount, sMesh.count * 9 + 40, RegionalSnow::kCount * 7 + 40))
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
    gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BACK);
    gSPComboRMPush(POLY_OPA_DISP++, "oot");
    gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    // Retain this material through the native two-cycle setup; repeating
    // TEXEL0 in cycle2 would sample the preceding scene's second tile.
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEI, G_CC_PASS2);
    for (size_t i = 0; i < sMesh.count; ++i) {
        const char* texture = RegionalBridgeMaterials::Resolve(sMesh.triangles[i].material, ResourceMgr_FileExists);
        gDPLoadTextureBlock(POLY_OPA_DISP++, texture, G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, 5,
                            5, G_TX_NOLOD, G_TX_NOLOD);
        gSPVertex(POLY_OPA_DISP++, reinterpret_cast<uintptr_t>(&sDrawVertices[i * 3]), 3, 0);
        gSP1Triangle(POLY_OPA_DISP++, 0, 1, 2, 0);
    }
    gSPComboRMPop(POLY_OPA_DISP++);
    gSPSetGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BACK);
    // Native dust5 is the soft flake sprite used by MM's snow presentation.
    // Actual falling billboards are local to this actor; no precipitation state.
    const auto* player = GET_PLAYER(play);
    if (player && play->state.running) {
        const auto& p = player->actor.world.pos;
        const auto& anchors = kWeatherAnchors;
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        gDPPipeSync(POLY_XLU_DISP++);
        gDPSetCycleType(POLY_XLU_DISP++, G_CYC_1CYCLE);
        gDPSetRenderMode(POLY_XLU_DISP++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
        gSPComboRMPush(POLY_XLU_DISP++, "oot");
        // I8 dust contributes only alpha. Multiplying its dark intensity into
        // RGB made the snow look like soot; scene fog must not recolor it.
        gDPSetCombineLERP(POLY_XLU_DISP++, 0, 0, 0, PRIMITIVE, TEXEL0, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, TEXEL0, 0,
                          PRIMITIVE, 0);
        gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BOTH | G_FOG);
        gSPSetGeometryMode(POLY_XLU_DISP++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
        gDPLoadTextureBlock(POLY_XLU_DISP++, gDust5Tex, G_IM_FMT_I, G_IM_SIZ_8b, 32, 32, 0, G_TX_CLAMP, G_TX_CLAMP, 5,
                            5, G_TX_NOLOD, G_TX_NOLOD);
        static Vtx quad[4] = { VTX(-2, -2, 0, 0, 1024, 255, 255, 255, 255),
                               VTX(2, -2, 0, 1024, 1024, 255, 255, 255, 255), VTX(2, 2, 0, 1024, 0, 255, 255, 255, 255),
                               VTX(-2, 2, 0, 0, 0, 255, 255, 255, 255) };
        for (size_t i = 0; i < RegionalSnow::kCount; ++i) {
            const auto center = anchors[i % anchors.size()];
            const float opacity = RegionalSnow::Opacity(std::hypot(p.x - center.x, p.z - center.z)) *
                                  RegionalSnow::Opacity(std::fabs(p.y - center.y) * 3);
            if (opacity <= 0)
                continue;
            const auto flake = RegionalSnow::Flake(reinterpret_cast<SnowheadActor*>(actor)->snowTick, i);
            const Point position = { center.x + flake.x, center.y + flake.y, center.z + flake.z };
            const u8 alpha = u8(220 * opacity * flake.alpha * ParticleViewOpacity(play, position, 3.f));
            if (!alpha)
                continue;
            Matrix_Translate(position.x, position.y, position.z, MTXMODE_NEW);
            Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
            gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                      G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 225, 242, 255, alpha);
            gSPVertex(POLY_XLU_DISP++, (uintptr_t)quad, 4, 0);
            gSP2Triangles(POLY_XLU_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
        }
        gSPComboRMPop(POLY_XLU_DISP++);
        Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    }
    CLOSE_DISPS(play->state.gfxCtx);
    DrawLocalChill(actor, play);
}
void DrawLocalChill(Actor* actor, PlayState* play) {
    // Winter's soft mesh uses no texture resource or OBJECT_FZ bank. Frame
    // vertices come from polyOpa's tail, so reserve those bytes as matrices too.
    constexpr size_t vertexMatrices = (SoHSeasonWinterMist::kVertexCount * sizeof(Vtx) + sizeof(Mtx) - 1) / sizeof(Mtx);
    if (!Fountain(play) || !play->state.running || !sActor || actor != &sActor->dyna.actor || sOwner != play ||
        !actor->update || !GET_PLAYER(play) ||
        !NeiGi_CanDrawLayers(play, RegionalSnow::kChillCount * (1 + vertexMatrices), 0,
                             RegionalSnow::kChillCount * 10 + 32))
        return;
    const auto& player = GET_PLAYER(play)->actor.world.pos;
    bool nearby = false;
    for (const auto& center : kWeatherAnchors) {
        nearby |=
            std::hypot(player.x - center.x, player.z - center.z) < 650 && std::fabs(player.y - center.y) * 3 < 650;
    }
    if (!nearby)
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    POLY_XLU_DISP = SoHSeasonWinterMist::Prepare(POLY_XLU_DISP);
    gSPComboRMPush(POLY_XLU_DISP++, "oot");
    const float dx = play->view.lookAt.x - play->view.eye.x;
    const float dz = play->view.lookAt.z - play->view.eye.z;
    const float length = std::hypot(dx, dz);
    const float rightX = length > .001f ? -dz / length : 1.f;
    const float rightZ = length > .001f ? dx / length : 0.f;
    for (size_t i = 0; i < RegionalSnow::kChillCount; ++i) {
        const auto center = kWeatherAnchors[i % kWeatherAnchors.size()];
        const float opacity = RegionalSnow::Opacity(std::hypot(player.x - center.x, player.z - center.z)) *
                              RegionalSnow::Opacity(std::fabs(player.y - center.y) * 3);
        const auto puff = RegionalSnow::Chill(sActor->snowTick, i);
        const float width = 180.f + float((i * 47) % 191), height = 10.f + float(i % 3) * 5.f;
        const Point position = { center.x + puff.x, center.y + puff.y + height, center.z + puff.z };
        const float alpha = .24f * opacity * puff.alpha * ParticleViewOpacity(play, position, height);
        if (alpha < 1.f / 255)
            continue;
        Matrix_Translate(position.x, position.y, position.z, MTXMODE_NEW);
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        auto* vertices =
            static_cast<Vtx*>(Graph_Alloc(play->state.gfxCtx, sizeof(Vtx) * SoHSeasonWinterMist::kVertexCount));
        SoHSeasonWinterMist::Vertices(vertices, rightX, rightZ, width, height, alpha);
        POLY_XLU_DISP = SoHSeasonWinterMist::Draw(POLY_XLU_DISP, vertices);
    }
    gSPComboRMPop(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}
void Ensure(PlayState* play) {
    if (!Fountain(play) || !ComboFountainValley_Ready(play))
        return;
    if (sActor && sOwner == play && sActor->dyna.actor.update)
        return;
    if (sActorId < 0) {
        ActorDBInit init;
        init.name = "Bg_ComboFountainSnowheadBridge";
        init.desc = "Snowhead glacial scramble";
        init.category = ACTORCAT_BG;
        init.objectId = OBJECT_GAMEPLAY_KEEP;
        init.instanceSize = sizeof(SnowheadActor);
        init.flags = ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
        init.init = Init;
        init.destroy = Destroy;
        init.update = Update;
        init.draw = Draw;
        sActorId = ActorDB::Instance->AddEntry(init).entry.id;
    }
    Actor* spawned = Actor_Spawn(&play->actorCtx, play, sActorId, 0, 0, 0, 0, 0, 0, 0);
    if (!sSpawnReported) {
        LUSLOG_INFO("[SnowheadBridge] spawn requested: actorId=%d actor=%p", sActorId, static_cast<void*>(spawned));
        sSpawnReported = true;
    }
}
void Register() {
    LUSLOG_INFO("[SnowheadBridge] hooks registered");
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayerUpdate>([] { Ensure(gPlayState); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([] {
        sActor = nullptr;
        sOwner = nullptr;
        sSpawnReported = false;
    });
}
RegisterShipInitFunc snowheadBridgeInit(Register);
} // namespace
extern "C" int ComboFountainSnowheadBridge_Get(PlayState* play, Vec3f* mouth, s16* yaw) {
    if (!Fountain(play) || !mouth || !yaw || !ComboFountainValley_Ready(play))
        return 0;
    Ensure(play);
    if (!sActor || sOwner != play || !sActor->dyna.actor.update)
        return 0;
    float floor = 0, returnFloor = 0;
    if (!ComboFountainValley::BodyClearAt(kMouth.x, kMouth.z) ||
        !ComboFountainValley::BodyClearAt(kReturn.x, kReturn.z) ||
        !ComboFountainValley::FloorAt(kMouth.x, kMouth.z, &floor) ||
        !ComboFountainValley::FloorAt(kReturn.x, kReturn.z, &returnFloor))
        return 0;
    *mouth = { kMouth.x, floor, kMouth.z };
    *yaw = kYaw;
    return 1;
}
#endif
