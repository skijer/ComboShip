// Real market presentation, native update/draw and GBI command expansion run.
// Resource availability, GPU setup, frame recording and cosmetics are boundaries.
#include "global.h"
#include "gameplay/CrossGameTitleCard.h"
#ifdef OOT_TITLE_DRAW_TEST
#include "soh/Enhancements/game-interactor/vanilla-behavior/GIVanillaBehavior.h"
#include "soh/Enhancements/cosmetics/cosmeticsTypes.h"
#else
#include "2s2h/GameInteractor/GameInteractor_VanillaBehavior.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#endif
#include <cassert>
#include <cstdio>
#include <cstring>

SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
#ifdef OOT_TITLE_DRAW_TEST
static GameInfo gameInfo{};
GameInfo* gGameInfo = &gameInfo;
extern "C" s32 Player_InCsMode(PlayState*) { return false; }
extern "C" Color_RGB8 CVarGetColor24(const char*, Color_RGB8 value) { return value; }
extern "C" void GameInteractor_ExecuteOnPresentTitleCard() {}
extern "C" f32 OTRGetDimensionFromLeftEdge(f32 value) { return value; }
extern "C" f32 OTRGetDimensionFromRightEdge(f32 value) { return 320.f - value; }
#define CVAR_COSMETIC(name) name
extern "C" Gfx* Gfx_SetupDL_52NoCD(Gfx* gfx) { return gfx; }
#else
extern "C" bool Player_InCsMode(PlayState*) { return false; }
extern "C" Gfx* Gfx_SetupDL52_NoCD(Gfx* gfx) { return gfx; }
#endif
extern "C" s32 CVarGetInteger(const char*, s32 value) { return value; }
extern "C" bool GameInteractor_Should(GIVanillaBehavior, uint32_t value, ...) { return value; }
extern "C" void FrameInterpolation_RecordOpenChild(const void*, int) {}
extern "C" void FrameInterpolation_RecordCloseChild() {}
u8 ResourceMgr_FileExists(const char*) { return true; }
bool ResourceMgr_IsAltAssetsEnabled() { return false; }
u8 ResourceMgr_FileAltExists(const char*) { return false; }
bool Interior() { return true; }
bool Active() { return true; }
bool Safe(PlayState* play) { return play && play->state.running; }
bool sTitlePending;

/* CARD_RESOURCE */
/* NATIVE_PRESENTATION_SCOPE */
/* NATIVE_TITLE_CARD */
/* MARKET_PRESENTER */

static GraphicsContext graphics;
static Gfx overlay[128], opaque[128], translucent[128];

static void Buffers() {
    std::memset(overlay, 0, sizeof(overlay));
    std::memset(opaque, 0, sizeof(opaque));
    std::memset(translucent, 0, sizeof(translucent));
    graphics.overlay.p = overlay;
    graphics.polyOpa.p = opaque;
    graphics.polyXlu.p = translucent;
}

static void InspectDraw(const TitleCardContext& card) {
    if (!card.alpha) {
        assert(graphics.overlay.p == overlay && "transparent native title must submit no drawing commands");
        return;
    }
    assert(graphics.overlay.p > overlay && graphics.overlay.p < overlay + 128);
    int textures = 0, renderTiles = 0, tileSizes = 0, rectangles = 0, colors = 0;
#ifdef OOT_TITLE_DRAW_TEST
    const u32 expectedColor = (u32(card.intensityR) << 24) | (u32(card.intensityG) << 16) |
                              (u32(card.intensityB) << 8) | u32(card.alpha);
#else
    const u32 expectedColor = u32(card.intensity) * 0x01010100u | u32(card.alpha);
#endif
    for (Gfx* command = overlay; command < graphics.overlay.p; ++command) {
        const u32 first = command->words.w0, second = command->words.w1;
        const u8 opcode = first >> 24;
        if (opcode == G_SETTIMG) {
            ++textures;
            assert(command->words.w1 == reinterpret_cast<uintptr_t>(card.texture));
            assert(((first >> 21) & 7) == G_IM_FMT_IA);
        } else if (opcode == G_SETTILE && ((second >> 24) & 7) == G_TX_RENDERTILE) {
            ++renderTiles;
            assert(((first >> 21) & 7) == G_IM_FMT_IA && ((first >> 19) & 3) == G_IM_SIZ_8b);
        } else if (opcode == G_SETTILESIZE) {
            ++tileSizes;
            assert(((second >> 12) & 0xFFF) == 956 && (second & 0xFFF) == 92);
        } else if (opcode == G_SETPRIMCOLOR) {
            ++colors;
            assert(second == expectedColor && "native draw must submit the current frame's fade color/alpha");
        } else if (opcode == G_TEXRECT) {
            ++rectangles;
            // Native MM coordinates are quarter-pixels, including its native -1 bottom edge.
            assert(((second >> 12) & 0xFFF) == 160 && (second & 0xFFF) == 432);
            assert(((first >> 12) & 0xFFF) == 1116 && (first & 0xFFF) == 527);
            assert(command + 2 < graphics.overlay.p);
            assert(command[1].words.w1 == 0 && command[2].words.w1 == 0x04000400);
        } else if (opcode == G_TEXRECT_WIDE) {
            ++rectangles;
            // Native OoT uses the wide command with its full native bottom edge.
            assert((first & 0xFFFFFF) == 1116 && (second & 0xFFFFFF) == 528);
            assert(command + 2 < graphics.overlay.p);
            assert((command[1].words.w0 & 0xFFFFFF) == 160 && (command[1].words.w1 & 0xFFFFFF) == 432);
            assert(command[2].words.w0 == 0 && command[2].words.w1 == 0x04000400);
            command += 2;
        }
    }
    assert(textures == 1 && renderTiles == 1 && tileSizes == 1 && rectangles == 1 && colors == 1);
    assert(graphics.polyOpa.p == opaque && graphics.polyXlu.p == translucent);
}

static void NativeFadeCheck(int frame, const TitleCardContext& card) {
    int expectedAlpha = -1, expectedIntensity = -1;
    switch (frame) {
        case 0: case 18: expectedAlpha = 0; expectedIntensity = 0; break;
        case 19: expectedAlpha = 10; expectedIntensity = 20; break;
        case 31: expectedAlpha = 130; expectedIntensity = 255; break;
        case 44: case 97: expectedAlpha = expectedIntensity = 255; break;
        case 98: expectedAlpha = 225; expectedIntensity = 185; break;
        case 106: expectedAlpha = expectedIntensity = 0; break;
    }
    if (expectedAlpha < 0)
        return;
    assert(card.alpha == expectedAlpha);
#ifdef OOT_TITLE_DRAW_TEST
    assert(card.intensityR == expectedIntensity && card.intensityG == expectedIntensity &&
           card.intensityB == expectedIntensity);
#else
    assert(card.intensity == expectedIntensity);
#endif
}

int main() {
    assert(__alignof__(kCard) >= 2 && "raw native texture pointers reserve the low bit for segmented addresses");
    assert(!std::strcmp(kCard, "__OTR__custom/oot_location_titles/v1/market"));
    for (int language = 0; language < 3; ++language) {
        PlayState play{};
        gSaveContext = {};
#ifdef OOT_TITLE_DRAW_TEST
        gSaveContext.language = language;
#else
        gSaveContext.options.language = language;
#endif
        gSaveContext.fileNum = 0;
        gPlayState = &play;
        gGameState = &play.state;
        play.state.running = true;
        play.state.gfxCtx = &graphics;
        sTitlePending = true;
        PresentTitle();
        auto& card = play.actorCtx.titleCtx;
        assert(card.texture == kCard && card.x == 160 && card.y == 120 && card.width == 240 && card.height == 24);
        assert(card.delayTimer == 20 && card.durationTimer == 80 && !sTitlePending);
        for (int frame = 0; frame < 120; ++frame) {
#ifdef OOT_TITLE_DRAW_TEST
            TitleCard_Update(&play, &card);
#else
            TitleCard_Update(&play.state, &card);
#endif
            NativeFadeCheck(frame, card);
            Buffers();
            const auto before = card;
#ifdef OOT_TITLE_DRAW_TEST
            TitleCard_Draw(&play, &card);
#else
            TitleCard_Draw(&play.state, &card);
#endif
            InspectDraw(card);
            assert(!std::memcmp(&card, &before, sizeof(card)) && "native drawing must not restart or mutate the card");
        }
        assert(card.alpha == 0 && card.durationTimer == 0 && card.delayTimer == 0);
    }
    std::puts("PASS native market TitleCard_Draw: actual IA8 resource, centered240x24 rectangle, native fade commands and language parity");
}
