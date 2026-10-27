#include "SeasonParticles.h"
#include "SeasonParticleState.h"
#include "SeasonSummerState.h"
#include "SeasonSummerTextures.h"
#include "SeasonWinterState.h"
#include "SeasonWinterMist.h"
#include "soh/Enhancements/audio/SeasonWeather.h"
#include "soh/frame_interpolation.h"
#include "z64.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
#include "objects/gameplay_keep/gameplay_keep.h"
}
#include "../../../../combo/NeiSpringBlossoms.h"

#include <algorithm>
#include <cmath>

extern "C" float OTRGetAspectRatio(void);

using namespace SoHSeasonParticles;
static State sFalling;
static SoHSeasonSummer::State sSummer;
static SoHSeasonWinter::State sWinter;
static PlayState* sPlay = nullptr;
static int sScene = -1, sRoom = -1, sSeason = -1;
static uint32_t sLastSummerFrame = 0;
static bool sSummerInitialized = false;
static uint32_t sLastWinterFrame = 0;
static bool sWinterInitialized = false;
static std::array<float, SoHSeasonWinter::kMistCount> sMistGround;
static std::array<uint32_t, SoHSeasonWinter::kMistCount> sMistGroundGeneration{};

static Vec3 Vector(Vec3f value) {
    return { value.x, value.y, value.z };
}
static Vec3 Cross(Vec3 a, Vec3 b) {
    return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}
static SoHSeasonParticles::View ReadView(PlayState* play) {
    SoHSeasonParticles::View view;
    const Camera* camera = GET_ACTIVE_CAM(play);
    view.eye = Vector(play->view.eye);
    view.forward = Normalize(Vector(play->view.lookAt) - view.eye);
    auto right = Cross(view.forward, Vector(play->view.up));
    if (Length(right) < 0.0001f)
        right = { 1, 0, 0 };
    view.right = Normalize(right);
    view.up = Normalize(Cross(view.right, view.forward));
    const float fov = play->view.fovy > 0 ? play->view.fovy : camera->fov > 0 ? camera->fov : 60;
    view.tanHalfFov = std::tan(std::clamp(fov, 10.0f, 120.0f) * 0.00872664626f);
    view.aspect = std::clamp(OTRGetAspectRatio(), 0.5f, 5.0f);
    return view;
}
static bool HasNativeSnow(const PlayState* play) {
    for (const Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].head; actor; actor = actor->next) {
        // SoH params3 owns native snow. Fairy/lightning/beam variants retain
        // their own actors and never inhibit this independent layer.
        if (actor->id == ACTOR_OBJECT_KANKYO && actor->params == 3 && actor->update != nullptr)
            return true;
    }
    return false;
}
static bool CompactScene(int scene) {
    return scene == SCENE_MARKET_DAY || scene == SCENE_MARKET_NIGHT || scene == SCENE_MARKET_RUINS ||
           scene == SCENE_MARKET_ENTRANCE_DAY || scene == SCENE_MARKET_ENTRANCE_NIGHT ||
           scene == SCENE_MARKET_ENTRANCE_RUINS || scene == SCENE_BACK_ALLEY_DAY || scene == SCENE_BACK_ALLEY_NIGHT;
}

// Preserve the exact native ComboShip980e6fe09 autumn quad and artwork.
static Vtx sLeafQuad[] = {
    VTX(-160, -160, 0, 0, 1024, 255, 255, 255, 255),
    VTX(160, -160, 0, 1024, 1024, 255, 255, 255, 255),
    VTX(160, 160, 0, 1024, 0, 255, 255, 255, 255),
    VTX(-160, 160, 0, 0, 0, 255, 255, 255, 255),
};
static Gfx sLeafGeometry[] = { gsSPVertex(sLeafQuad, 4, 0), gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
                               gsSPEndDisplayList() };
static const ALIGN_ASSET(2) char sLeafCrimson[] = "__OTR__objects/nei_autumn/leaves/crimson_tex";
static const ALIGN_ASSET(2) char sLeafOrange[] = "__OTR__objects/nei_autumn/leaves/orange_tex";
static const ALIGN_ASSET(2) char sLeafGold[] = "__OTR__objects/nei_autumn/leaves/gold_tex";
static const ALIGN_ASSET(2) char sLeafCopper[] = "__OTR__objects/nei_autumn/leaves/copper_tex";
static const char* sLeafTextures[] = { sLeafCrimson, sLeafOrange, sLeafGold, sLeafCopper };
static Vtx sSummerQuad[] = {
    VTX(-100, -100, 0, 0, 512, 255, 255, 255, 255),
    VTX(100, -100, 0, 512, 512, 255, 255, 255, 255),
    VTX(100, 100, 0, 512, 0, 255, 255, 255, 255),
    VTX(-100, 100, 0, 0, 0, 255, 255, 255, 255),
};
static Gfx sSummerGeometry[] = {
    gsSPVertex(sSummerQuad, 4, 0),
    gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSPEndDisplayList(),
};
static void SetParticleMaterial(GraphicsContext* gfxCtx) {
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL_25Xlu(gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetCycleType(POLY_XLU_DISP++, G_CYC_1CYCLE);
    gDPSetRenderMode(POLY_XLU_DISP++, G_RM_AA_ZB_XLU_SURF, G_RM_AA_ZB_XLU_SURF2);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BACK | G_CULL_FRONT | G_FOG);
    gSPSetGeometryMode(POLY_XLU_DISP++, G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH);
    gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
    gDPSetCombineMode(POLY_XLU_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    CLOSE_DISPS(gfxCtx);
}
static void RestoreParticleMaterial(GraphicsContext* gfxCtx) {
    OPEN_DISPS(gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(gfxCtx);
    CLOSE_DISPS(gfxCtx);
}
static void DrawFalling(PlayState* play, int season, const SoHSeasonParticles::View& view) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    bool material = false;
    for (size_t i = 0; i < sFalling.Count(); ++i) {
        const auto& p = sFalling.Particles()[i];
        const auto screen = Project(p.position, view);
        if (season != 3 && (screen.depth <= 0 || std::abs(screen.x) >= 1 || std::abs(screen.y) >= 1))
            continue;
        float alpha = 180;
        if (season != 3) {
            const float distance = Length(p.position - view.eye);
            const Actor* player = play->actorCtx.actorLists[ACTORCAT_PLAYER].head;
            const float nearestDistance =
                player ? std::min(distance, Length(p.position - Vector(player->world.pos))) : distance;
            alpha = 220 * std::clamp((8000 - distance) / 2000, 0.0f, 1.0f) *
                    std::clamp((nearestDistance - 150) / 200, 0.0f, 1.0f) * std::min(p.age / 24, 1.0f) *
                    std::clamp((1 - std::abs(screen.x)) / 0.2f, 0.0f, 1.0f) *
                    std::clamp((1 - std::abs(screen.y)) / 0.2f, 0.0f, 1.0f);
        }
        if (alpha < 1)
            continue;
        if (!material) {
            SetParticleMaterial(gfxCtx);
            material = true;
            if (season == 0) {
                gSPTexture(POLY_XLU_DISP++, 0, 0, 0, G_TX_RENDERTILE, G_OFF);
                gDPSetCombineLERP(POLY_XLU_DISP++, SHADE, 0, PRIMITIVE, 0, 0, 0, 0, PRIMITIVE, SHADE, 0, PRIMITIVE, 0,
                                  0, 0, 0, PRIMITIVE);
            } else if (season == 3) {
                gSPSegment(POLY_XLU_DISP++, 0x08, reinterpret_cast<uintptr_t>(gDust5Tex));
            }
        }
        FrameInterpolation_RecordOpenChild(&p, static_cast<int>(p.generation));
        Matrix_Push();
        Matrix_Translate(p.position.x, p.position.y, p.position.z, MTXMODE_NEW);
        Matrix_Mult(&play->billboardMtxF, MTXMODE_APPLY);
        if (season == 3) {
            Matrix_Scale(0.05f, 0.05f, 0.05f, MTXMODE_APPLY);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 180);
        } else {
            Matrix_RotateZ(std::fmod(p.rotation, 65536.0f) * 0.0000958738f, MTXMODE_APPLY);
            const float scale = (0.035f + (i & 7) * 0.002f) * (1 + p.band * 0.7f);
            Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
            if (season == 0) {
                const auto* color = NeiSpringBlossoms_Color(static_cast<unsigned>(i));
                const float motionFrame = (p.rotation - i * 1300) / 180;
                const float flutter = 0.65f + std::sin(motionFrame * 0.045f + i * 2.4f) * 0.25f;
                Matrix_Scale(flutter, 1, 1, MTXMODE_APPLY);
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, color[0], color[1], color[2], static_cast<uint8_t>(alpha));
            } else {
                // Native OTR resources keep the donor's128x128 art and scale
                // it through the same32x32 display-list texture coordinates.
                gDPPipeSync(POLY_XLU_DISP++);
                gDPLoadTextureBlock(POLY_XLU_DISP++, sLeafTextures[i & 3], G_IM_FMT_RGBA, G_IM_SIZ_32b, 32, 32, 0,
                                    G_TX_CLAMP, G_TX_CLAMP, 5, 5, G_TX_NOLOD, G_TX_NOLOD);
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, static_cast<uint8_t>(alpha));
            }
        }
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gSPDisplayList(POLY_XLU_DISP++, season == 0   ? NeiSpringBlossoms_Geometry(static_cast<unsigned>(i))
                                        : season == 2 ? sLeafGeometry
                                                      : reinterpret_cast<Gfx*>(const_cast<char*>(gEffDustDL)));
        Matrix_Pop();
        FrameInterpolation_RecordCloseChild();
    }
    // Empty, disabled, and fully culled layers change no render state.
    if (material)
        RestoreParticleMaterial(gfxCtx);
    CLOSE_DISPS(gfxCtx);
}
static SoHSeasonSummer::View SummerView(const SoHSeasonParticles::View& view) {
    return { { view.eye.x, view.eye.y, view.eye.z },
             { view.forward.x, view.forward.y, view.forward.z },
             { view.right.x, view.right.y, view.right.z },
             { view.up.x, view.up.y, view.up.z },
             view.tanHalfFov,
             view.aspect };
}
static void DrawSummer(PlayState* play, const SoHSeasonParticles::View& view) {
    auto summerView = SummerView(view);
    SoHSeasonSummer::Input input;
    input.eligible = true;
    input.paused = play->pauseCtx.state != 0 || play->pauseCtx.debugState != 0 ||
                   (sSummerInitialized && play->gameplayFrames == sLastSummerFrame);
    input.seconds = std::clamp<int>(R_UPDATE_RATE, 1, 3) / 60.0f;
    input.hour = gSaveContext.dayTime * (24.0f / 65536);
    input.dayVisibility = play->envCtx.unk_EE[1] > 0 ? 0 : 1;
    input.sunbeams = play->skyboxId == SKYBOX_NORMAL_SKY && !play->envCtx.skyboxDisabled &&
                     !play->envCtx.sunMoonDisabled && SeasonWeather_SummerEffectEnabled(2);
    input.sunDirection = { play->envCtx.sunPos.x, play->envCtx.sunPos.y, play->envCtx.sunPos.z };
    sSummer.Step(input, summerView);
    sSummerInitialized = true;
    sLastSummerFrame = play->gameplayFrames;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    bool material = false;
    const uint8_t* activeTexture = nullptr;
    for (const auto& p : sSummer.Particles()) {
        const bool firefly = p.kind == SoHSeasonSummer::Kind::Firefly;
        if (p.alpha < 1.0f / 255 || !SeasonWeather_SummerEffectEnabled(firefly ? 1 : 0) ||
            !SoHSeasonSummer::InView(p.position, summerView, 1))
            continue;
        if (!material) {
            SetParticleMaterial(gfxCtx);
            material = true;
        }
        const auto* texture =
            firefly ? SoHSeasonSummer::kGlowTexture.data() : SoHSeasonSummer::kDandelionTexture.data();
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
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, firefly ? 238 : 255, firefly ? 255 : 244, firefly ? 154 : 204,
                        static_cast<uint8_t>(std::lround(p.alpha * 255)));
        gSPDisplayList(POLY_XLU_DISP++, sSummerGeometry);
        Matrix_Pop();
        FrameInterpolation_RecordCloseChild();
    }
    if (input.sunbeams && !sSummer.Beams().empty()) {
        if (!material) {
            SetParticleMaterial(gfxCtx);
            material = true;
        }
        gDPPipeSync(POLY_XLU_DISP++);
        gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF);
        gDPSetCombineMode(POLY_XLU_DISP++, G_CC_SHADE, G_CC_SHADE);
        Matrix_Push();
        Matrix_Translate(view.eye.x, view.eye.y, view.eye.z, MTXMODE_NEW);
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        // Donor state produces at most3 beams, each12 vertices/6 triangle pairs.
        for (const auto& beam : sSummer.Beams()) {
            auto* vertices = static_cast<Vtx*>(Graph_Alloc(gfxCtx, sizeof(Vtx) * beam.vertices.size()));
            for (size_t i = 0; i < beam.vertices.size(); ++i) {
                const auto relative = beam.vertices[i].position - summerView.eye;
                const Vtx vertex =
                    VTX(static_cast<s16>(std::lround(relative.x)), static_cast<s16>(std::lround(relative.y)),
                        static_cast<s16>(std::lround(relative.z)), 0, 0, 255, 244, 204,
                        static_cast<u8>(std::lround(beam.vertices[i].alpha * 255)));
                vertices[i] = vertex;
            }
            gSPVertex(POLY_XLU_DISP++, reinterpret_cast<uintptr_t>(vertices), 12, 0);
            for (int row = 0; row < 3; ++row)
                for (int column = 0; column < 2; ++column) {
                    const int index = row * 3 + column;
                    gSP2Triangles(POLY_XLU_DISP++, index, index + 3, index + 4, 0, index, index + 4, index + 1, 0);
                }
        }
        Matrix_Pop();
    }
    if (material)
        RestoreParticleMaterial(gfxCtx);
    CLOSE_DISPS(gfxCtx);
}

static void DrawWinter(PlayState* play, const SoHSeasonParticles::View& view) {
    const Actor* player = play->actorCtx.actorLists[ACTORCAT_PLAYER].head;
    SoHSeasonWinter::Input input;
    input.eligible = true;
    input.paused = play->pauseCtx.state != 0 || play->pauseCtx.debugState != 0 ||
                   (sWinterInitialized && play->gameplayFrames == sLastWinterFrame);
    input.seconds = std::clamp<int>(R_UPDATE_RATE, 1, 3) / 60.0f;
    input.hour = gSaveContext.dayTime * (24.0f / 65536);
    input.mist = player && SeasonWeather_WinterEffectEnabled(0);
    input.aurora = SeasonWeather_WinterEffectEnabled(1);
    input.skyVisible = play->skyboxId == SKYBOX_NORMAL_SKY && !play->envCtx.skyboxDisabled;
    input.clipFar = play->view.zFar > 200 ? play->view.zFar : 12800;
    input.origin = player ? Vector(player->world.pos) : view.eye;
    sWinter.Step(input, view);
    sWinterInitialized = true;
    sLastWinterFrame = play->gameplayFrames;
    Vec3 right = { view.right.x, 0, view.right.z };
    if (Length(right) < 0.001f)
        right = { 1, 0, 0 };
    right = Normalize(right);
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    bool material = false;
    const auto prepare = [&]() {
        if (material)
            return;
        Gfx_SetupDL_25Xlu(gfxCtx);
        POLY_XLU_DISP = SoHSeasonWinterMist::Prepare(POLY_XLU_DISP);
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
    for (size_t i = 0; i < sWinter.Wisps().size(); ++i) {
        const auto& wisp = sWinter.Wisps()[i];
        if (wisp.alpha < 1.0f / 255)
            continue;
        if (sMistGroundGeneration[i] != wisp.generation || (!input.paused && play->gameplayFrames % 4 == 0)) {
            CollisionPoly floor{};
            Vec3f sample = { wisp.position.x, input.origin.y + 600, wisp.position.z };
            sMistGround[i] = BgCheck_AnyRaycastFloor1(&play->colCtx, &floor, &sample);
            sMistGroundGeneration[i] = wisp.generation;
        }
        if (!std::isfinite(sMistGround[i]) || sMistGround[i] <= BGCHECK_Y_MIN)
            continue;
        const Vec3 center = { wisp.position.x, sMistGround[i] + wisp.height + 2, wisp.position.z };
        const auto projected = Project(center, view);
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
        gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
        auto* vertices = static_cast<Vtx*>(Graph_Alloc(gfxCtx, sizeof(Vtx) * 15));
        SoHSeasonWinterMist::Vertices(vertices, right.x, right.z, wisp.width, wisp.height, wisp.alpha * edge);
        POLY_XLU_DISP = SoHSeasonWinterMist::Draw(POLY_XLU_DISP, vertices);
        Matrix_Pop();
        FrameInterpolation_RecordCloseChild();
    }
    bool skyMatrix = false;
    for (const auto& ribbon : sWinter.Ribbons()) {
        if (std::none_of(ribbon.vertices.begin(), ribbon.vertices.end(),
                         [](const auto& vertex) { return vertex.alpha >= 1.0f / 255; }))
            continue;
        prepare();
        if (!skyMatrix) {
            FrameInterpolation_RecordOpenChild(sWinter.Ribbons().data(), sScene);
            Matrix_Push();
            Matrix_Translate(view.eye.x, view.eye.y, view.eye.z, MTXMODE_NEW);
            gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(gfxCtx), G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
            skyMatrix = true;
        }
        auto* vertices = static_cast<Vtx*>(Graph_Alloc(gfxCtx, sizeof(Vtx) * ribbon.vertices.size()));
        for (size_t column = 0; column < SoHSeasonWinter::kRibbonColumns; ++column)
            for (size_t row = 0; row < 3; ++row) {
                const auto& source = ribbon.vertices[column * 3 + row];
                const auto relative = source.position - view.eye;
                const Vtx vertex =
                    VTX(static_cast<s16>(std::lround(relative.x)), static_cast<s16>(std::lround(relative.y)),
                        static_cast<s16>(std::lround(relative.z)), 0, 0, source.red, source.green, source.blue,
                        static_cast<u8>(std::lround(source.alpha * 255)));
                vertices[row * SoHSeasonWinter::kRibbonColumns + column] = vertex;
            }
        mesh(vertices, SoHSeasonWinter::kRibbonColumns, ribbon.vertices.size());
    }
    if (skyMatrix) {
        Matrix_Pop();
        FrameInterpolation_RecordCloseChild();
    }
    if (material)
        RestoreParticleMaterial(gfxCtx);
    CLOSE_DISPS(gfxCtx);
}

extern "C" void SeasonParticles_Reset(void) {
    sFalling.Reset();
    sSummer.Reset();
    sWinter.Reset();
    sMistGround.fill(BGCHECK_Y_MIN);
    sMistGroundGeneration.fill(0);
    sPlay = nullptr;
    sScene = sRoom = sSeason = -1;
    sLastSummerFrame = 0;
    sSummerInitialized = false;
    sLastWinterFrame = 0;
    sWinterInitialized = false;
}
extern "C" void SeasonParticles_Draw(PlayState* play) {
    const int season = SeasonWeather_ParticleSeasonForPlay(play);
    // Summer has its own master and3 independent effects; the falling-particle
    // predicate intentionally recognizes only spring, autumn, and winter.
    const bool enabled =
        season == 1
            ? SeasonWeather_SummerEffectEnabled(0) || SeasonWeather_SummerEffectEnabled(1) ||
                  SeasonWeather_SummerEffectEnabled(2)
            : SeasonWeather_ParticlesEnabled(season) ||
                  (season == 3 && (SeasonWeather_WinterEffectEnabled(0) || SeasonWeather_WinterEffectEnabled(1)));
    if (!play || !play->state.gfxCtx || play->activeCamera < 0 || play->activeCamera >= 4 || !GET_ACTIVE_CAM(play) ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        (GET_ACTIVE_CAM(play)->unk_14C & 0x100) || season < 0 || season > 3) {
        if (sPlay)
            SeasonParticles_Reset();
        return;
    }
    if (!enabled) {
        // Keep winter's cosmetic positions while all its boxes are off, so
        // toggling mist back on in the pause menu can reuse the frozen view.
        if (sPlay && season != 3)
            SeasonParticles_Reset();
        return;
    }
    if (sPlay != play || sScene != play->sceneNum || sRoom != play->roomCtx.curRoom.num || sSeason != season ||
        (sSummerInitialized && play->gameplayFrames < sLastSummerFrame) ||
        (sWinterInitialized && play->gameplayFrames < sLastWinterFrame)) {
        SeasonParticles_Reset();
        sPlay = play;
        sScene = play->sceneNum;
        sRoom = play->roomCtx.curRoom.num;
        sSeason = season;
    }
    const SoHSeasonParticles::View view = ReadView(play);
    if (season == 1) {
        DrawSummer(play, view);
        return;
    }
    SoHSeasonParticles::Input input;
    input.play = play;
    input.scene = play->sceneNum;
    input.room = play->roomCtx.curRoom.num;
    input.season = season;
    input.frame = play->gameplayFrames;
    input.eligible = SeasonWeather_ParticlesEnabled(season);
    input.paused = play->pauseCtx.state != 0 || play->pauseCtx.debugState != 0;
    input.compact = CompactScene(play->sceneNum);
    input.flurries = SeasonWeather_FlurriesEnabled();
    input.nativeSnow = season == 3 && HasNativeSnow(play);
    input.wind = { static_cast<float>(play->envCtx.windDirection.x), static_cast<float>(play->envCtx.windDirection.y),
                   static_cast<float>(play->envCtx.windDirection.z) };
    input.windSpeed = play->envCtx.windSpeed;
    sFalling.Tick(input, view);
    DrawFalling(play, season, view);
    if (season == 3)
        DrawWinter(play, view);
}
