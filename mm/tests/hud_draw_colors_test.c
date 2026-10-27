#include "global.h"
#include "interface/parameter_static/parameter_static.h"
#include "2s2h/BenGui/HudEditor.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include "mods/combo_rpg.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "tests/test_require.h"
#include <string.h>

SaveContext gSaveContext;
HudEditorElementID hudEditorActiveElement;
HudEditorElement hudEditorElements[HUD_EDITOR_ELEMENT_MAX];
static Gfx commands[1024], arena[256];
static GraphicsContext gfx;
static PlayState play;
static s16 sMagicMeterOutlinePrimRed = 255, sMagicMeterOutlinePrimGreen = 255, sMagicMeterOutlinePrimBlue = 255;
void TestSetHudColor(const char* id, u8 r, u8 g, u8 b, int changed);
void TestResetHudColors(void);
NeiSaveData* Nei_Save(void) {
    return &gSaveContext.save.shipSaveInfo.nei;
}
void FrameInterpolation_RecordOpenChild(const void* source, int line) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Graph_OpenDisps(Gfx** refs, Gfx* vals, GraphicsContext* ctx, const char* file, s32 line) {
}
void Graph_CloseDisps(Gfx** refs, Gfx* vals, GraphicsContext* ctx, const char* file, s32 line) {
}
void gSPVertex(Gfx* command, uintptr_t vertices, int count, int first) {
    __gSPVertex(command, vertices, count, first);
}
void Gfx_SetupDL39_Overlay(GraphicsContext* ctx) {
}
void Gfx_SetupDL42_Overlay(GraphicsContext* ctx) {
}
void Mtx_SetTranslateScaleMtx(Mtx* mtx, f32 sx, f32 sy, f32 sz, f32 x, f32 y, f32 z) {
}
void HudEditor_SetActiveElement(HudEditorElementID id) {
    hudEditorActiveElement = id;
}
bool HudEditor_ShouldOverrideDraw(void) {
    return false;
}
f32 HudEditor_GetActiveElementScale(void) {
    return 1.0f;
}
void HudEditor_ModifyMatrixValues(f32* x, f32* y) {
}
void HudEditor_ModifyDrawValues(s16* x, s16* y, s16* w, s16* h, s16* dx, s16* dy) {
}
Gfx* Gfx_DrawTexRectIA8_DropShadow(Gfx* dl, TexturePtr tex, s16 tw, s16 th, s16 x, s16 y, s16 w, s16 h, u16 dx, u16 dy,
                                   s16 r, s16 g, s16 b, s16 a) {
    return dl;
}
Gfx* Gfx_DrawTexRectIA8_DropShadowOffset(Gfx* dl, TexturePtr tex, s16 tw, s16 th, s16 x, s16 y, s16 w, s16 h, u16 dx,
                                         u16 dy, s16 r, s16 g, s16 b, s16 a, s32 masks, s32 rects) {
    return dl;
}

#include "hud_draw_production.inc"

static void beginDraw(void) {
    memset(commands, 0, sizeof(commands));
    gfx.overlay.p = commands;
    gfx.polyOpa.d = arena + 256;
    play.state.gfxCtx = &gfx;
}
static int hasColor(unsigned op, uint32_t rgba) {
    int matches = 0;
    for (Gfx* cmd = commands; cmd < gfx.overlay.p; ++cmd) {
        if ((cmd->words.w0 >> 24) == op && cmd->words.w1 == rgba) {
            ++matches;
        }
    }
    return matches;
}
static int countDrawCommands(unsigned op) {
    int matches = 0;
    for (Gfx* cmd = commands; cmd < gfx.overlay.p; ++cmd) {
        matches += (cmd->words.w0 >> 24) == op;
    }
    return matches;
}
static void drawHearts(void) {
    beginDraw();
    LifeMeter_Draw(&play);
}
static void drawMagic(void) {
    beginDraw();
    Magic_DrawMeter(&play);
}

void TestHudDrawColors(void) {
    TestResetHudColors();
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(&play, 0, sizeof(play));
    gSaveContext.save.saveInfo.playerData.healthCapacity = 6 * 16;
    gSaveContext.save.saveInfo.playerData.health = 3 * 16 + 8;
    gSaveContext.save.saveInfo.inventory.defenseHearts = 6;
    LifeMeter_Init(&play);
    sBeatingHeartsDDPrim[0] = sBeatingHeartsDDPrim[1] = sBeatingHeartsDDPrim[2] = 255;
    sBeatingHeartsDDEnv[0] = 200;
    play.interfaceCtx.healthAlpha = 77;
    drawHearts();
    REQUIRE(hasColor(G_SETENVCOLOR, 0xC80000FF) == 3);
    TestSetHudColor("HUD.Hearts", 100, 150, 200, 1);
    drawHearts();
    REQUIRE(hasColor(G_SETENVCOLOR, 0x2D5F91FF) == 3); // legacy subtract-55 fallback
    TestSetHudColor("HUD.DDHearts", 17, 34, 51, 1);
    drawHearts();
    REQUIRE(hasColor(G_SETENVCOLOR, 0x112233FF) == 3);  // full, beating and empty DD hearts
    REQUIRE(hasColor(G_SETPRIMCOLOR, 0xFFFFFF4D) == 3); // white borders + native HUD fade
    gSaveContext.save.saveInfo.inventory.defenseHearts = 0;
    play.interfaceCtx.beatingHeartPrim[0] = 255;
    play.interfaceCtx.beatingHeartPrim[1] = 70;
    play.interfaceCtx.beatingHeartPrim[2] = 50;
    drawHearts();
    REQUIRE(hasColor(G_SETPRIMCOLOR, 0x6496C84D) == 3);
    REQUIRE(hasColor(G_SETENVCOLOR, 0x112233FF) == 0);
    gSaveContext.save.saveInfo.inventory.defenseHearts = 6;
    TestSetHudColor("HUD.DDHearts", 17, 34, 51, 0);
    drawHearts();
    REQUIRE(hasColor(G_SETENVCOLOR, 0x2D5F91FF) == 3);
    puts("PASS: DD hex controls full/beating/empty fills; normal hearts, white border, fade and reset preserved");

    gSaveContext.save.saveInfo.playerData.healthCapacity = 3 * 16 + 4;
    gSaveContext.save.saveInfo.playerData.health = 16;
    drawHearts();
    REQUIRE(countDrawCommands(G_TEXRECT) == 2); // two static hearts plus one beating heart
    Nei_Save()->comboRpg.quarterHeartsEnabled = 1;
    drawHearts();
    REQUIRE(countDrawCommands(G_TEXRECT) == 3); // the fractional capacity adds a fourth heart
    Nei_Save()->comboRpg.quarterHeartsEnabled = 0;
    puts("PASS: saved RPG quarter-heart rule draws fractional capacity through the real MM adapter");

    TestResetHudColors();
    gSaveContext.save.saveInfo.playerData.magicLevel = 1;
    gSaveContext.save.saveInfo.playerData.magic = 48;
    gSaveContext.magicCapacity = 48;
    gSaveContext.magicToConsume = 8;
    play.interfaceCtx.magicAlpha = 91;
    for (int state = 0; state < 2; ++state) {
        gSaveContext.magicState = state ? MAGIC_STATE_METER_FLASH_2 : MAGIC_STATE_IDLE;
        gSaveContext.save.saveInfo.weekEventReg[14] |= 8;
        drawMagic();
        REQUIRE(hasColor(G_SETPRIMCOLOR, 0x0000C85B) == 1);
        TestSetHudColor("HUD.Magic", 0, 200, 0, 1);
        drawMagic();
        REQUIRE(hasColor(G_SETPRIMCOLOR, 0x0000C85B) == 1); // legacy green -> blue rotation
        TestSetHudColor("HUD.InfiniteMagic", 18, 52, 86, 1);
        drawMagic();
        REQUIRE(hasColor(G_SETPRIMCOLOR, 0x1234565B) == 1); // exact hex, no forced hue rotation
        gSaveContext.save.saveInfo.weekEventReg[14] &= ~8;
        drawMagic();
        REQUIRE(hasColor(G_SETPRIMCOLOR, 0x00C8005B) == 1);
        REQUIRE(hasColor(G_SETPRIMCOLOR, 0x1234565B) == 0);
        gSaveContext.save.saveInfo.weekEventReg[14] |= 8;
        TestSetHudColor("HUD.InfiniteMagic", 18, 52, 86, 0);
        drawMagic();
        REQUIRE(hasColor(G_SETPRIMCOLOR, 0x0000C85B) == 1);
        TestResetHudColors();
    }
    puts("PASS: Chateau hex works in idle/consumption HUD draws, preserves normal magic/fade, and resets");
}
