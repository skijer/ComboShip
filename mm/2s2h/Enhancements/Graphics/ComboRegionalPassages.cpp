#ifdef COMBO_BUILD
#include "global.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "actors/RegionalPassagePresentation.h"
#include "2s2h/Enhancements/Gameplay/ComboRegionalPassages.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "../../../../soh/soh/Enhancements/randomizer/NeiGiRender.h"

namespace {
// Display-list macros redeclare these helpers at block scope in this namespace.
extern "C" void FrameInterpolation_RecordOpenChild(const void* a, int b);
extern "C" void FrameInterpolation_RecordCloseChild(void);
namespace RP = RegionalPassagePresentation;

bool RegionalPresentationSafe(PlayState* play) {
    return play && play->state.running && play->state.gfxCtx && play->pauseCtx.state == PAUSE_STATE_OFF &&
           play->csCtx.state == CS_STATE_IDLE && play->transitionTrigger == TRANS_TRIGGER_OFF &&
           play->transitionMode == TRANS_MODE_OFF && play->gameOverCtx.state == GAMEOVER_INACTIVE;
}

void DrawRegionalShell(PlayState* play, int route) {
    static const auto vertices = RP::NativeShell<Vtx>();
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    gSPComboRMPush(POLY_OPA_DISP++, "mm");
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPClearGeometryMode(POLY_OPA_DISP++, G_LIGHTING | G_CULL_BOTH);
    gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEI, G_CC_MODULATEI);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    gDPLoadTextureBlock(POLY_OPA_DISP++, RP::Texture(RP::Host::Mm, route), G_IM_FMT_RGBA, G_IM_SIZ_16b, 32, 32, 0,
                        G_TX_WRAP, G_TX_WRAP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
    for (size_t i = 0; i < RP::kShellQuads; ++i) {
        gSPVertex(POLY_OPA_DISP++, (uintptr_t)&vertices[i * 4], 4, 0);
        gSP2Triangles(POLY_OPA_DISP++, 0, 1, 2, 0, 0, 2, 3, 0);
    }
    gDPPipeSync(POLY_OPA_DISP++);
    gSPComboRMPop(POLY_OPA_DISP++);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

void DrawRegionalPassage() {
    auto* play = gPlayState;
    int route;
    Vec3f anchor;
    s16 yaw;
    if (!RegionalPresentationSafe(play) || !ComboRegionalPassagesMm_Get(play, &route, &anchor, &yaw) ||
        !RP::KnownRoute(route) || !NeiGi_CanDrawLayers(play, 1, RP::kShellOpaCommands, RP::kShellXluCommands))
        return;
    Matrix_Push();
    Matrix_Translate(anchor.x, anchor.y, anchor.z, MTXMODE_NEW);
    Matrix_RotateYS(yaw, MTXMODE_APPLY);
    DrawRegionalShell(play, route);
    const auto mesh = RP::Shimmer(play->gameplayFrames, RP::Host::Mm, route, NeiGi_CameraBasis(play));
    NeiGi_DrawWorldParticles(play, mesh);
    Matrix_Pop();
}

void RegisterRegionalPresentation() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldEnd>(DrawRegionalPassage);
}
RegisterShipInitFunc init(RegisterRegionalPresentation, {});
} // namespace
#endif
