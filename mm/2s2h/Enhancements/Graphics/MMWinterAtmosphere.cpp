#include "MMWinterAtmosphere.h"
#include "MMWinterAtmosphereState.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "global.h"
#include <libultraship/bridge/consolevariablebridge.h>

#include <algorithm>
#include <cmath>

extern "C" float OTRGetAspectRatio(void);

namespace {
MMWinter::State sState;
MMWinter::View sView;
PlayState* sPlay = nullptr;
int sScene = -1, sRoom = -1;
uint32_t sLastFrame = 0;
bool sFrameValid = false;
std::array<float, MMWinter::kMistCount> sGround;
std::array<uint32_t, MMWinter::kMistCount> sGroundGeneration{}, sGroundFrame{};

MMWinter::Vec3 Vector(Vec3f value) {
    return { value.x, value.y, value.z };
}
MMWinter::Vec3 Cross(MMWinter::Vec3 a, MMWinter::Vec3 b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
MMWinter::View ReadView(const PlayState* play) {
    const Camera* camera = GET_ACTIVE_CAM(play);
    MMWinter::View view;
    view.eye = Vector(camera->eye);
    view.forward = MMSummer::Normalize(Vector(camera->at) - view.eye);
    auto right = Cross(view.forward, Vector(play->view.up));
    if (MMSummer::Length(right) < 0.0001f)
        right = { 1, 0, 0 };
    view.right = MMSummer::Normalize(right);
    view.up = MMSummer::Normalize(Cross(view.right, view.forward));
    view.tanHalfFov = std::tan(std::clamp(camera->fov, 10.0f, 120.0f) * 0.00872664626f);
    view.aspect = std::clamp(OTRGetAspectRatio(), 0.5f, 5.0f);
    return view;
}
struct Projection {
    float depth, x, y;
};
Projection Project(MMWinter::Vec3 position) {
    const auto relative = position - sView.eye;
    const float depth = MMSummer::Dot(relative, sView.forward);
    const float height = std::max(depth, 0.001f) * sView.tanHalfFov;
    return { depth, MMSummer::Dot(relative, sView.right) / (height * sView.aspect),
             MMSummer::Dot(relative, sView.up) / height };
}
} // namespace

extern "C" void MMWinterAtmosphere_Reset() {
    sState.Reset();
    sGround.fill(BGCHECK_Y_MIN);
    sGroundGeneration.fill(0);
    sGroundFrame.fill(0);
    sPlay = nullptr;
    sScene = sRoom = -1;
    sLastFrame = 0;
    sFrameValid = false;
}
extern "C" void MMWinterAtmosphere_Update(PlayState* play) {
    // Play_Main clears gfxCtx during Play_Update. Simulation and terrain queries
    // must continue there; only Draw needs the restored graphics context.
    // Check camera indices before the shared weather eligibility resolver uses
    // GET_ACTIVE_CAM. No native precipitation or actor slots belong to this layer.
    if (!play || play->activeCamId < 0 || play->activeCamId >= NUM_CAMS || !GET_ACTIVE_CAM(play) ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        MMWeather_SeasonForPlay(play) != SEASON_WINTER) {
        MMWinterAtmosphere_Reset();
        return;
    }
    if (sPlay != play || sScene != play->sceneId || sRoom != play->roomCtx.curRoom.num ||
        (sFrameValid && play->gameplayFrames < sLastFrame)) {
        MMWinterAtmosphere_Reset();
        sPlay = play;
        sScene = play->sceneId;
        sRoom = play->roomCtx.curRoom.num;
    }
    sView = ReadView(play);
    const Actor* player = play->actorCtx.actorLists[ACTORCAT_PLAYER].first;
    MMWinter::Input input;
    input.eligible = true;
    // Update and draw can both refresh live controls. Motion and terrain queries
    // advance only once per gameplay frame, including frame advance and debug pause.
    input.paused = IS_PAUSED(&play->pauseCtx) || IREG(72) != 0 || (sFrameValid && play->gameplayFrames == sLastFrame);
    input.seconds = std::clamp<int>(R_UPDATE_RATE, 1, 3) / 60.0f;
    input.hour = CURRENT_TIME * (24.0f / 65536);
    input.mist = player && CVarGetInteger(MM_WEATHER_CVAR("Winter.Mist"), 1) != 0;
    input.aurora = CVarGetInteger(MM_WEATHER_CVAR("Winter.Aurora"), 1) != 0;
    input.skyVisible = play->skyboxId == SKYBOX_NORMAL_SKY;
    input.clipFar = play->view.zFar;
    input.origin = player ? Vector(player->world.pos) : sView.eye;
    sState.Step(input, sView);
    for (size_t i = 0; i < sState.Wisps().size(); ++i) {
        const auto& wisp = sState.Wisps()[i];
        if (wisp.alpha < 1.0f / 255)
            continue;
        if (sGroundGeneration[i] != wisp.generation ||
            (!input.paused && play->gameplayFrames % 4 == 0 && sGroundFrame[i] != play->gameplayFrames)) {
            CollisionPoly floor{};
            Vec3f sample = { wisp.position.x, input.origin.y + 600, wisp.position.z };
            sGround[i] = BgCheck_AnyRaycastFloor1(&play->colCtx, &floor, &sample);
            sGroundGeneration[i] = wisp.generation;
            sGroundFrame[i] = play->gameplayFrames;
        }
    }
    sFrameValid = true;
    sLastFrame = play->gameplayFrames;
}
extern "C" void MMWinterAtmosphere_Draw(PlayState* play) {
    // Paused menu changes take effect immediately without ageing or respawning
    // the frozen wisps. This also refreshes the draw's final view far clip.
    MMWinterAtmosphere_Update(play);
    if (!play || !play->state.gfxCtx || sPlay != play)
        return;

    auto right = MMWinter::Vec3{ sView.right.x, 0, sView.right.z };
    if (MMSummer::Length(right) < 0.001f)
        right = { 1, 0, 0 };
    right = MMSummer::Normalize(right);
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    bool material = false;
    const auto prepare = [&]() {
        if (material)
            return;
        Gfx_SetupDL25_Xlu(gfxCtx);
        gDPPipeSync(POLY_XLU_DISP++);
        gDPSetCycleType(POLY_XLU_DISP++, G_CYC_1CYCLE);
        gDPSetRenderMode(POLY_XLU_DISP++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
        gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BACK | G_CULL_FRONT | G_FOG);
        gSPSetGeometryMode(POLY_XLU_DISP++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
        gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_SHADE, G_CC_SHADE);
        material = true;
    };
    const auto mesh = [&](Vtx* vertices, int columns, int count) {
        gSPVertex(POLY_XLU_DISP++, reinterpret_cast<uintptr_t>(vertices), count, 0);
        for (int row = 0; row < 2; ++row)
            for (int column = 0; column < columns - 1; ++column) {
                const int index = row * columns + column;
                gSP2Triangles(POLY_XLU_DISP++, index, index + columns, index + columns + 1, 0, index,
                              index + columns + 1, index + 1, 0);
            }
    };
    for (size_t i = 0; i < sState.Wisps().size(); ++i) {
        const auto& wisp = sState.Wisps()[i];
        if (wisp.alpha < 1.0f / 255 || !std::isfinite(sGround[i]) || sGround[i] <= BGCHECK_Y_MIN)
            continue;
        const auto center = MMWinter::Vec3{ wisp.position.x, sGround[i] + wisp.height + 2, wisp.position.z };
        const auto projected = Project(center);
        if (projected.depth < 120 || projected.depth > 2400 || std::abs(projected.x) > 1.25f ||
            std::abs(projected.y) > 1.25f)
            continue;
        const float edge =
            std::clamp((1.25f - std::max(std::abs(projected.x), std::abs(projected.y))) / 0.25f, 0.0f, 1.0f);
        if (wisp.alpha * edge < 1.0f / 255)
            continue;
        prepare();
        FrameInterpolation_RecordOpenChild(&wisp, static_cast<int>(wisp.generation));
        Matrix_Push();
        Matrix_Translate(center.x, center.y, center.z, MTXMODE_NEW);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
        auto* vertices = static_cast<Vtx*>(GRAPH_ALLOC(gfxCtx, sizeof(Vtx) * 15));
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 5; ++column) {
                const float x = (column - 2) * 0.5f;
                auto relative = right * (x * wisp.width);
                relative.y += (row - 1) * wisp.height * (1 - 0.3f * std::abs(x));
                const float alpha =
                    row == 1 && column > 0 && column < 4 ? wisp.alpha * edge * (column == 2 ? 1 : 0.68f) : 0;
                // Keep the same soft alpha while separating the ice blue mist
                // from Termina's nearly white frost palette.
                const Vtx vertex =
                    VTX(static_cast<s16>(std::lround(relative.x)), static_cast<s16>(std::lround(relative.y)),
                        static_cast<s16>(std::lround(relative.z)), 0, 0, 150, 190, 225,
                        static_cast<u8>(std::lround(alpha * 255)));
                vertices[row * 5 + column] = vertex;
            }
        mesh(vertices, 5, 15);
        Matrix_Pop();
        FrameInterpolation_RecordCloseChild();
    }
    bool skyMatrix = false;
    for (const auto& ribbon : sState.Ribbons()) {
        if (std::none_of(ribbon.vertices.begin(), ribbon.vertices.end(),
                         [](const auto& vertex) { return vertex.alpha >= 1.0f / 255; }))
            continue;
        prepare();
        if (!skyMatrix) {
            FrameInterpolation_RecordOpenChild(sState.Ribbons().data(), static_cast<int>(sState.Generation()));
            Matrix_Push();
            Matrix_Translate(sView.eye.x, sView.eye.y, sView.eye.z, MTXMODE_NEW);
            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
            skyMatrix = true;
        }
        auto* vertices = static_cast<Vtx*>(GRAPH_ALLOC(gfxCtx, sizeof(Vtx) * ribbon.vertices.size()));
        for (size_t column = 0; column < MMWinter::kRibbonColumns; ++column)
            for (size_t row = 0; row < 3; ++row) {
                const auto& source = ribbon.vertices[column * 3 + row];
                const auto relative = source.position - sView.eye;
                const Vtx vertex =
                    VTX(static_cast<s16>(std::lround(relative.x)), static_cast<s16>(std::lround(relative.y)),
                        static_cast<s16>(std::lround(relative.z)), 0, 0, source.red, source.green, source.blue,
                        static_cast<u8>(std::lround(source.alpha * 255)));
                vertices[row * MMWinter::kRibbonColumns + column] = vertex;
            }
        mesh(vertices, MMWinter::kRibbonColumns, ribbon.vertices.size());
    }
    if (skyMatrix) {
        Matrix_Pop();
        FrameInterpolation_RecordCloseChild();
    }
    if (material) {
        gDPPipeSync(POLY_XLU_DISP++);
        // The native setup restores its textured combiner and G_ON texture state.
        Gfx_SetupDL25_Xlu(gfxCtx);
    }
    CLOSE_DISPS(gfxCtx);
}
