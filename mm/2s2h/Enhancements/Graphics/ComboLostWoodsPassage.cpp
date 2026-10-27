// A separate tree passage between the entry clearing's maze corridors.
// Drawn in the active MM resource manager, so the native bark can resolve through Alt Assets.
#ifdef COMBO_BUILD
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "../../../../combo/entrances/LostWoodsEntrance.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Gameplay/ComboSoloForest.h"
#include "2s2h/Enhancements/Gameplay/ComboWoodfallPassage.h"
#include "actors/ForestGiParticles.h"
#include "entrances/NativeForest.h"
#include "../../../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "WoodsMysteryMakeover.h"
#include "WoodsMysteryNativeBridge.h"
#include "AutumnSceneFoliage.h"
#include <array>
#include <cmath>

#include "global.h"

namespace {
constexpr int kArchSegments = 10;
constexpr int kQuadCount = 2 + kArchSegments * 2 + 1;
std::array<Vtx, kQuadCount * 4> sVertices;

void Vertex(Vtx& vertex, float x, float y, float z, unsigned char shade) {
    vertex = {};
    vertex.v.ob[0] = (s16)x;
    vertex.v.ob[1] = (s16)y;
    vertex.v.ob[2] = (s16)z;
    vertex.v.tc[0] = (s16)(x * 32);
    vertex.v.tc[1] = (s16)(-y * 32);
    vertex.v.cn[0] = vertex.v.cn[1] = vertex.v.cn[2] = shade;
    vertex.v.cn[3] = 255;
}

void BuildPassage() {
    int index = 0;
    auto quad = [&](float ax, float ay, float az, float bx, float by, float bz, float cx, float cy, float cz, float dx,
                    float dy, float dz, unsigned char shade) {
        Vertex(sVertices[index++], ax, ay, az, shade);
        Vertex(sVertices[index++], bx, by, bz, shade);
        Vertex(sVertices[index++], cx, cy, cz, shade);
        Vertex(sVertices[index++], dx, dy, dz, shade);
    };
    // Two roots form the jambs. The curved roof and its dark inner reveal form a short tunnel.
    quad(-68, 0, 0, -44, 0, 0, -44, 65, 0, -68, 65, 0, 200);
    quad(44, 0, 0, 68, 0, 0, 68, 65, 0, 44, 65, 0, 200);
    for (int i = 0; i < kArchSegments; ++i) {
        const float a = i * 3.14159265f / kArchSegments;
        const float b = (i + 1) * 3.14159265f / kArchSegments;
        const float ax = std::cos(a), ay = std::sin(a), bx = std::cos(b), by = std::sin(b);
        quad(ax * 44, 65 + ay * 44, 0, ax * 68, 65 + ay * 68, 0, bx * 68, 65 + by * 68, 0, bx * 44, 65 + by * 44, 0,
             200);
        quad(ax * 44, 65 + ay * 44, 0, bx * 44, 65 + by * 44, 0, bx * 44, 65 + by * 44, -45, ax * 44, 65 + ay * 44, -45,
             65);
    }
    // Black mouth, behind the frame. The native wall/collision beyond stays intact.
    quad(-44, 0, -2, 44, 0, -2, 44, 110, -2, -44, 110, -2, 0);
}
} // namespace

static void DrawPassage() {
    if (!Combo_IsCombinedGame() && !ComboSoloForestMm_PassageAvailable())
        return;
    if (ComboOpeningForest_UsesPackTunnel(gPlayState))
        return; // The active scene mesh owns the doorway; never draw a second arch.
    const bool openingForest = ComboOpeningForest_IsExploration(gPlayState) && gPlayState->roomCtx.curRoom.num == 0 &&
                               gPlayState->linkActorEntry;
    if (!openingForest && (!gPlayState || gSaveContext.gameMode != GAMEMODE_NORMAL ||
                           gPlayState->sceneId != SCENE_26SARUNOMORI || gPlayState->roomCtx.curRoom.num != 1))
        return;
    if (!openingForest &&
        (WoodsMysteryMakeover::OwnsBridgeMesh(gPlayState) || WoodsMysteryNativeBridge::Active(gPlayState)))
        return;
    static const bool built = (BuildPassage(), true);
    (void)built;
    OPEN_DISPS(gPlayState->state.gfxCtx);
    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    Matrix_Push();
    if (openingForest) {
        Matrix_Translate(COMBO_OPENING_FOREST_STUMP_X, 0.0f, COMBO_OPENING_FOREST_STUMP_Z, MTXMODE_NEW);
        Matrix_RotateYS(COMBO_OPENING_FOREST_STUMP_YAW, MTXMODE_APPLY);
    } else {
        Matrix_Translate(160.0f, 0.0f, -160.0f, MTXMODE_NEW);
        Matrix_RotateYS(-8192, MTXMODE_APPLY);
    }
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gPlayState->state.gfxCtx);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BOTH);
    gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEI, G_CC_MODULATEI);
    gDPLoadTextureBlock(POLY_OPA_DISP++, "__OTR__scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0", G_IM_FMT_RGBA,
                        G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
    // Arrival remains in the clearing, away from the stump's black mouth.
    for (int i = 0; i < kQuadCount; ++i) {
        gSPVertex(POLY_OPA_DISP++, (uintptr_t)&sVertices[i * 4], 4, 0);
        gSP2Triangles(POLY_OPA_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    Matrix_Pop();
    gDPPipeSync(POLY_OPA_DISP++);
    Gfx_SetupDL25_Opa(gPlayState->state.gfxCtx);
    CLOSE_DISPS(gPlayState->state.gfxCtx);
}

static void DrawWoodfallPassage() {
    PlayState* play = gPlayState;
    Vec3f anchor;
    s16 yaw;
    if (!ComboWoodfallPassage_Get(play, &anchor, &yaw))
        return;
    static const auto vertices = [] {
        BuildPassage();
        auto result = sVertices;
        // The entrance platform's angled wall is narrower than a forest trunk.
        for (auto& vertex : result)
            vertex.v.ob[0] = (s16)(vertex.v.ob[0] * COMBO_WOODFALL_PASSAGE_HALF_WIDTH / 68.0f);
        // One unit behind the frame, still in front of the intact native wall.
        for (int i = (kQuadCount - 1) * 4; i < kQuadCount * 4; ++i)
            result[i].v.ob[2] = -1;
        return result;
    }();
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(anchor.x, anchor.y, anchor.z, MTXMODE_NEW);
    Matrix_RotateYS(yaw, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BOTH);
    gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEI, G_CC_MODULATEI);
    gDPLoadTextureBlock(POLY_OPA_DISP++, "__OTR__scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORITex_00BAC0", G_IM_FMT_RGBA,
                        G_IM_SIZ_16b, 32, 32, 0, G_TX_WRAP, G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
    for (int i = 0; i < kQuadCount; ++i) {
        gSPVertex(POLY_OPA_DISP++, (uintptr_t)&vertices[i * 4], 4, 0);
        gSP2Triangles(POLY_OPA_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    Matrix_Pop();
    gDPPipeSync(POLY_OPA_DISP++);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

static void DrawPortalFinish() {
    PlayState* play = gPlayState;
    if (!ComboOpeningForest_IsExploration(play) || play->roomCtx.curRoom.num != 0 || !play->linkActorEntry ||
        play->csCtx.state != CS_STATE_IDLE || play->pauseCtx.state != PAUSE_STATE_OFF)
        return;
    const int version = ComboOpeningForest_ReturnPackVersion(play);
    if (version == 1 || version == 2)
        return; // Older meshes retain their own visible location and treatment.
    Matrix_Push();
    const Vec3f anchor = ComboOpeningForest_ReturnAnchor(play);
    Matrix_Translate(anchor.x, anchor.y, anchor.z, MTXMODE_NEW);
    Matrix_RotateYS(ComboOpeningForest_ReturnYaw(play), MTXMODE_APPLY);
    const bool packed =
        (version >= 3 && version <= 8) || (version == 9 && ResourceMgr_FileExists(NativeForest::kAuthoredPortalMarker));
    const auto mesh = ForestGiParticles::Portal(play->gameplayFrames, packed, NeiGi_CameraBasis(play));
    NeiGi_DrawWorldParticles(play, mesh);
    Matrix_Pop();
}

static void DrawWoodsMysteryHexes() {
    if (!Combo_IsCombinedGame() && !ComboSoloForestMm_PassageAvailable())
        return;
    PlayState* play = gPlayState;
    if (!play || (!WoodsMysteryMakeover::Active(play) && !WoodsMysteryNativeBridge::Active(play)) ||
        gSaveContext.gameMode != GAMEMODE_NORMAL || play->csCtx.state != CS_STATE_IDLE ||
        play->pauseCtx.state != PAUSE_STATE_OFF)
        return;
    Matrix_Push();
    Matrix_Translate(WoodsMysteryMakeover::kBlackPortalX, 0.0f, WoodsMysteryMakeover::kBlackPortalZ, MTXMODE_NEW);
    Matrix_RotateYS(WoodsMysteryMakeover::kMouthYaw, MTXMODE_APPLY);
    const auto mesh = ForestGiParticles::Mystery(play->gameplayFrames, NeiGi_CameraBasis(play));
    NeiGi_DrawWorldParticles(play, mesh);
    Matrix_Pop();
}

namespace {
void RegisterPassage() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldStart>([] {
        const bool canopyWasPatched = WoodsMysteryNativeBridge::sActive && WoodsMysteryNativeBridge::PatchedCanopy();
        WoodsMysteryNativeBridge::Update(gPlayState, Combo_IsCombinedGame() || ComboSoloForestMm_PassageAvailable());
        const bool canopyIsPatched = WoodsMysteryNativeBridge::sActive && WoodsMysteryNativeBridge::PatchedCanopy();
        if (canopyWasPatched != canopyIsPatched) {
            // Weather updated before this hook. Rebind its retained copy now
            // so the first world draw uses the helper's current vertex layout.
            MMAutumnSceneFoliage_Update(gPlayState);
        }
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(
        [] { WoodsMysteryNativeBridge::Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldEnd>([] {
        DrawPassage();
        DrawWoodfallPassage();
        DrawPortalFinish();
        DrawWoodsMysteryHexes();
    });
}
static RegisterShipInitFunc initFunc(RegisterPassage, {});
} // namespace
#endif
