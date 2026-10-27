#include "MMSummerAtmosphere.h"
#include "MMSummerAtmosphereState.h"
#include "MMSummerAtmosphereTextures.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "global.h"
#include <libultraship/bridge/consolevariablebridge.h>

#include <algorithm>
#include <cmath>

// BenPort.h exposes this bridge only to C translation units.
extern "C" float OTRGetAspectRatio(void);

namespace {
MMSummer::State sState;
MMSummer::View sView;
PlayState* sPlay = nullptr;
int sScene = -1, sRoom = -1;

MMSummer::Vec3 Vector(Vec3f value) {
    return { value.x, value.y, value.z };
}
MMSummer::Vec3 Cross(MMSummer::Vec3 a, MMSummer::Vec3 b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
MMSummer::View ReadView(PlayState* play) {
    const Camera* camera = GET_ACTIVE_CAM(play);
    MMSummer::View view;
    view.eye = Vector(camera->eye);
    view.forward = MMSummer::Normalize(Vector(camera->at) - view.eye);
    // guLookAtF stores a backward look axis, so screen right is forward × up.
    auto right = Cross(view.forward, Vector(play->view.up));
    if (MMSummer::Length(right) < 0.0001f)
        right = { 1, 0, 0 };
    view.right = MMSummer::Normalize(right);
    view.up = MMSummer::Normalize(Cross(view.right, view.forward));
    view.tanHalfFov = std::tan(std::clamp(camera->fov, 10.0f, 120.0f) * 0.00872664626f);
    view.aspect = std::clamp(OTRGetAspectRatio(), 0.5f, 5.0f);
    return view;
}

Vtx sQuad[] = {
    VTX(-100, -100, 0, 0, 512, 255, 255, 255, 255),
    VTX(100, -100, 0, 512, 512, 255, 255, 255, 255),
    VTX(100, 100, 0, 512, 0, 255, 255, 255, 255),
    VTX(-100, 100, 0, 0, 0, 255, 255, 255, 255),
};
Gfx sQuadGeometry[] = {
    gsSPVertex(sQuad, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};
} // namespace

extern "C" void MMSummerAtmosphere_Reset() {
    sState.Reset();
    sPlay = nullptr;
    sScene = sRoom = -1;
}
extern "C" void MMSummerAtmosphere_Update(PlayState* play) {
    if (!play) {
        MMSummerAtmosphere_Reset();
        return;
    }
    if (sPlay != play || sScene != play->sceneId || sRoom != play->roomCtx.curRoom.num) {
        MMSummerAtmosphere_Reset();
        sPlay = play;
        sScene = play->sceneId;
        sRoom = play->roomCtx.curRoom.num;
    }
    MMSummer::Input input;
    input.eligible =
        MMWeather_SeasonForPlay(play) == SEASON_SUMMER && CVarGetInteger(MM_SUMMER_CVAR("Enabled"), 1) != 0;
    if (!input.eligible) {
        sState.Reset();
        return;
    }
    sView = ReadView(play);
    input.paused = play->pauseCtx.state != PAUSE_STATE_OFF;
    input.seconds = std::clamp<int>(R_UPDATE_RATE, 1, 3) / 60.0f;
    input.hour = CURRENT_TIME * (24.0f / 65536);
    const bool rain = MMWeather_RainDensity() > 0 ||
                      (!MMWeather_SeasonClearsRain() && play->envCtx.precipitation[PRECIP_RAIN_CUR] > 0);
    input.dayVisibility = rain ? 0 : std::clamp(1 - MMWeather_Overcast() * 1.5f, 0.0f, 1.0f);
    input.sunbeams = !play->envCtx.sunDisabled && CVarGetInteger(MM_SUMMER_CVAR("Sunbeams"), 0) != 0;
    input.sunDirection = Vector(play->envCtx.sunPos);
    sState.Step(input, sView);
}
extern "C" void MMSummerAtmosphere_Draw(PlayState* play) {
    if (!play || sPlay != play || MMWeather_SeasonForPlay(play) != SEASON_SUMMER ||
        !CVarGetInteger(MM_SUMMER_CVAR("Enabled"), 1))
        return;

    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetCycleType(POLY_XLU_DISP++, G_CYC_1CYCLE);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BACK | G_CULL_FRONT | G_FOG);
    gSPSetGeometryMode(POLY_XLU_DISP++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    const uint8_t* activeTexture = nullptr;
    for (const auto& p : sState.Particles()) {
        if (p.alpha < 1.0f / 255)
            continue;
        const bool firefly = p.kind == MMSummer::Kind::Firefly;
        // Read live controls at draw time so paused changes take effect without
        // restarting particle motion or changing the independent sunbeams.
        if (!CVarGetInteger(firefly ? MM_SUMMER_CVAR("Fireflies") : MM_SUMMER_CVAR("Fluff"), 1))
            continue;
        const auto* texture = firefly ? MMSummer::kGlowTexture.data() : MMSummer::kDandelionTexture.data();
        if (texture != activeTexture) {
            gDPPipeSync(POLY_XLU_DISP++);
            gDPLoadTextureBlock(POLY_XLU_DISP++, texture, G_IM_FMT_IA, G_IM_SIZ_8b, 16, 16, 0, G_TX_CLAMP, G_TX_CLAMP,
                                4, 4, G_TX_NOLOD, G_TX_NOLOD);
            activeTexture = texture;
        }
        FrameInterpolation_RecordOpenChild(&p, static_cast<int>(p.generation));
        Matrix_Push();
        Matrix_Translate(p.position.x, p.position.y, p.position.z, MTXMODE_NEW);
        Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
        Matrix_Scale(p.radius / 100, p.radius / 100, 1, MTXMODE_APPLY);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, firefly ? 238 : 255, firefly ? 255 : 244, firefly ? 154 : 204,
                        static_cast<uint8_t>(std::lround(p.alpha * 255)));
        gSPDisplayList(POLY_XLU_DISP++, sQuadGeometry);
        Matrix_Pop();
        FrameInterpolation_RecordCloseChild();
    }
    if (CVarGetInteger(MM_SUMMER_CVAR("Sunbeams"), 0) && !sState.Beams().empty()) {
        gDPPipeSync(POLY_XLU_DISP++);
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_SHADE, G_CC_SHADE);
        Matrix_Push();
        Matrix_Translate(sView.eye.x, sView.eye.y, sView.eye.z, MTXMODE_NEW);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
        for (const auto& beam : sState.Beams()) {
            auto* vertices = static_cast<Vtx*>(GRAPH_ALLOC(gfxCtx, sizeof(Vtx) * beam.vertices.size()));
            for (size_t i = 0; i < beam.vertices.size(); ++i) {
                const auto relative = beam.vertices[i].position - sView.eye;
                const Vtx vertex =
                    VTX(static_cast<s16>(std::lround(relative.x)), static_cast<s16>(std::lround(relative.y)),
                        static_cast<s16>(std::lround(relative.z)), 0, 0, 255, 244, 204,
                        static_cast<u8>(std::lround(beam.vertices[i].alpha * 255)));
                vertices[i] = vertex;
            }
            gSPVertex(POLY_XLU_DISP++, (uintptr_t)vertices, 12, 0);
            for (int row = 0; row < 3; ++row) {
                for (int column = 0; column < 2; ++column) {
                    const int index = row * 3 + column;
                    gSP2Triangles(POLY_XLU_DISP++, index, index + 3, index + 4, 0, index, index + 4, index + 1, 0);
                }
            }
        }
        Matrix_Pop();
    }
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL25_Xlu(gfxCtx);
    CLOSE_DISPS(gfxCtx);
}
