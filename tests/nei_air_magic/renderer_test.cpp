// Reuse the real MM renderer/GBI fixture; only engine and archive boundaries
// are replaced. The renamed baseline main is discarded by section GC.
#define main BaselineRendererMain
#include "../mm_nei/renderer_runtime_test.cpp"
#undef main
#include "2s2h/Rando/NeiAirMagicPresentation.h"
#include <limits>

static float playerHeight = 44;
extern "C" f32 Player_GetHeight(Player*) { return playerHeight; }

int main() {
    PlayState play{};
    Player player{};
    GraphicsContext gfx{};
    alignas(16) static Gfx opa[0x6700], xlu[0x1000], overlay[0x800];
    play.state.gfxCtx = &gfx;
    gPlayState = &play;
    play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz = 1;
    const Vec3f origin{10, 20, 30}, direction{0, 0, 18};
    player.actor.world.pos = origin;
    player.actor.velocity = {3, 0, 2};
    const char* wind = "__OTR__objects/nei_air_magic/silver_wisp";
    const char* lightning = "__OTR__objects/nei_air_magic/lightning_filament";
    auto reset = [&]() {
        gfx.polyOpa.p = opa; gfx.polyOpa.d = std::end(opa);
        gfx.polyXlu.p = xlu; gfx.polyXlu.d = std::end(xlu);
        gfx.overlay.p = overlay; gfx.overlay.d = std::end(overlay);
        children.clear();
        Matrix_Translate(7, 8, 9, MTXMODE_NEW);
    };
    size_t maxBytes = 0, maxCommands = 0;
    for (bool present : {false, true}) {
        ownerBase.clear();
        if (present) { ownerBase.insert(wind); ownerBase.insert(lightning); }
        for (unsigned frame = 0; frame < 180; ++frame) {
            reset(); play.gameplayFrames = frame;
            const auto saved = current;
            NeiAirMagic_DrawLightning(&play, &origin, &direction);
            NeiAirMagic_DrawEnvelope(&play, &player);
            NeiAirMagic_DrawGust(&play, &origin, &direction, 200, 100, false, 0xFFFFFF);
            NeiAirMagic_DrawGust(&play, &origin, &direction, 200, 100, true, 0xDDE5EF);
            assert(matrices.empty() && !std::memcmp(&saved, &current, sizeof(saved)));
            const auto effectChildren = std::count_if(children.begin(), children.end(), [](const auto& child) {
                return (child.second & 0xFFFFFF00) == 0x41495200;
            });
            assert(effectChildren == 4);
            assert(gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d);
            unsigned pushes = 0, vertices = 0;
            int depth = 0;
            for (Gfx* cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
                const unsigned op = cmd->words.w0 >> 24;
                if (op == G_COMBO_RM_PUSH) { ++pushes; ++depth; }
                if (op == G_COMBO_RM_POP) --depth;
                if (op == G_VTX) ++vertices;
                assert(depth >= 0 && depth <= 1);
            }
            assert(depth == 0 && vertices > 0 && pushes == (present ? 4u : 0u));
            const size_t bytes = reinterpret_cast<uintptr_t>(std::end(opa)) - reinterpret_cast<uintptr_t>(gfx.polyOpa.d);
            maxBytes = std::max(maxBytes, bytes);
            maxCommands = std::max(maxCommands, size_t(gfx.polyXlu.p - xlu));
        }
    }
    // Native fast-math must reject malformed transforms before matrix entry.
    for (float bad : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
        reset(); const auto saved = current;
        const Vec3f invalid{bad, 0, 0};
        NeiAirMagic_DrawLightning(&play, &invalid, &direction);
        NeiAirMagic_DrawLightning(&play, &origin, &invalid);
        NeiAirMagic_DrawGust(&play, &origin, &direction, bad, 100, true, 0xFFFFFF);
        NeiAirMagic_DrawGust(&play, &origin, &direction, 200, bad, true, 0xFFFFFF);
        playerHeight = bad; NeiAirMagic_DrawEnvelope(&play, &player);
        playerHeight = 44; player.actor.velocity = invalid;
        NeiAirMagic_DrawEnvelope(&play, &player);
        player.actor.velocity = {};
        assert(children.empty() && matrices.empty() && !std::memcmp(&saved, &current, sizeof(saved)));
        assert(gfx.polyOpa.p == opa && gfx.polyOpa.d == std::end(opa) && gfx.polyXlu.p == xlu);
    }
    for (bool present : {false, true}) {
        ownerBase.clear(); if (present) { ownerBase.insert(wind); ownerBase.insert(lightning); }
        for (int arena = 0; arena < 2; ++arena) {
            reset(); const auto saved = current;
            if (arena == 0) gfx.polyOpa.d = opa + 1; else gfx.polyXlu.d = xlu + 1;
            const auto opaTail = gfx.polyOpa.d, xluTail = gfx.polyXlu.d;
            NeiAirMagic_DrawLightning(&play, &origin, &direction);
            NeiAirMagic_DrawEnvelope(&play, &player);
            NeiAirMagic_DrawGust(&play, &origin, &direction, 200, 100, true, 0xFFFFFF);
            assert(matrices.empty() && !std::memcmp(&saved, &current, sizeof(saved)));
            assert(gfx.polyOpa.p == opa && gfx.polyOpa.d == opaTail && gfx.polyXlu.p == xlu && gfx.polyXlu.d == xluTail);
        }
    }
    assert(maxBytes < 96 * 1024 && maxCommands < 1200);
    std::cout << "PASS native MM air magic: textured/fallback draws, resource-owner balance, matrix restoration, invalid inputs and arena decline; four-effect peak "
              << maxBytes << " OPA bytes / " << maxCommands << " XLU commands\n";
}
