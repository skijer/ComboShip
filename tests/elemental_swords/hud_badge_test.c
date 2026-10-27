// Execute the production badge helper and inspect real native GBI commands.
#include "global.h"
#include "sword_element.h"
#include "din_fire_sword.h"
#include "../../combo/render/sword_element_ui_art.h"
#include "tests/test_require.h"
#include <string.h>
#ifdef TEST_MM
#include "archives/icon_item_static/icon_item_static_yar.h"
#include "mods/forms/custom_forms.h"
#define FIRE_TEXTURE gItemIconFireArrowTex
#define ICE_TEXTURE gItemIconIceArrowTex
#define LIGHT_TEXTURE gItemIconLightArrowTex
#define SWORD_ITEM ITEM_SWORD_KOKIRI
#else
#include "textures/icon_item_static/icon_item_static.h"
#define FIRE_TEXTURE gItemIconArrowFireTex
#define ICE_TEXTURE gItemIconArrowIceTex
#define LIGHT_TEXTURE gItemIconArrowLightTex
#define SWORD_ITEM ITEM_SWORD_KOKIRI
#endif

static GraphicsContext gfx;
static PlayState play;
static Player player;
static Gfx commands[128];
static u8 selected, transformed, mario;
static u8 hiddenSword, fourSword;
static s32 bossOwner;
SaveContext gSaveContext;
u8 SwordElement_ForItem(u16 item) { return item == SWORD_ITEM ? selected : SWORD_ELEMENT_NONE; }
int SwordElement_SlotForItem(u16 item) { return item == SWORD_ITEM ? 0 : -1; }
u32 DinFireSword_DamageFlags(PlayState* state, Player* owner, u32 original) { return original; }
u8 TransformMasks_IsTransformedAny(void) { return transformed; }
u8 Sm64Mario_IsActive(void) { return mario; }
u8 ExtEquip_ShouldHideSwordDL(void) { return hiddenSword; }
u8 FourSword_IsEquipped(void) { return fourSword; }
s32 BossRemains_IsOdolwaWorn(void) { return bossOwner == 1; }
s32 BossRemains_IsGohtWorn(void) { return bossOwner == 2; }
void FrameInterpolation_RecordOpenChild(const void* a, int b) {}
void FrameInterpolation_RecordCloseChild(void) {}
#ifdef TEST_MM
static s32 forcedForm = CUSTOM_FORM_NONE;
s32 CustomForms_ActiveForm(void) {
    return forcedForm != CUSTOM_FORM_NONE ? forcedForm : Nei_Save()->activeCustomForm ? 0 : CUSTOM_FORM_NONE;
}
void Graph_OpenDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {}
void Graph_CloseDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {}
NeiSaveData* Nei_Save(void) { return &gSaveContext.save.shipSaveInfo.nei; }
#endif

#include "sword_badge_production.inc"

static void Begin(void) {
    memset(&gfx, 0, sizeof(gfx));
    memset(commands, 0, sizeof(commands));
    gfx.overlay.p = commands;
    play.state.gfxCtx = &gfx;
}
static int Count(u8 op) {
    int n = 0;
    for (Gfx* g = commands; g < gfx.overlay.p; ++g) n += (g->words.w0 >> 24) == op;
    return n;
}
static void Draw(u16 item, s16 button, s16 width, s16 height) {
    Begin();
    Interface_DrawSwordElementBadge(&play, item, button, 200, 20, width, height);
}
static void CheckDraw(const SwordElementUiRgbaTile* tiles, int left, int top, int width, int height) {
    REQUIRE(Count(G_TEXRECT_WIDE) == 4);
    REQUIRE(Count(G_SETTIMG) == 4);
    int tile = 0, rectangle = 0;
    for (Gfx* g = commands; g < gfx.overlay.p; ++g) {
        if ((g->words.w0 >> 24) == G_SETTIMG) {
            REQUIRE(g->words.w1 == (uintptr_t)tiles[tile].pixels);
            REQUIRE(tiles[tile].before == SWORD_ELEMENT_UI_CANARY && tiles[tile].after == SWORD_ELEMENT_UI_CANARY);
            REQUIRE(((g->words.w0 >> 21) & 7) == G_IM_FMT_RGBA);
            REQUIRE(((g->words.w0 >> 19) & 3) == G_IM_SIZ_32b);
            ++tile;
        }
        if ((g->words.w0 >> 24) == G_LOADBLOCK)
            REQUIRE((((g->words.w1 >> 12) & 0xfff) + 1) * 4 == sizeof(tiles[0].pixels));
        if ((g->words.w0 >> 24) == G_SETTILESIZE) {
            REQUIRE(((g->words.w1 >> 12) & 0xfff) == 31 * 4);
            REQUIRE((g->words.w1 & 0xfff) == 31 * 4);
        }
        if ((g->words.w0 >> 24) == G_SETPRIMCOLOR) REQUIRE(g->words.w1 == 0xFFFFFF7B);
        if ((g->words.w0 >> 24) == G_TEXRECT_WIDE) {
            int x4 = left * 4 + (rectangle & 1) * width * 2;
            int y4 = top * 4 + (rectangle >> 1) * height * 2;
            REQUIRE((g->words.w0 & 0xFFFFFF) == x4 + width * 2);
            REQUIRE(g->words.w1 == y4 + height * 2);
            REQUIRE((g[1].words.w0 & 0xFFFFFF) == x4);
            REQUIRE(g[1].words.w1 == y4);
            // Fast3D decodes these as signed S5.10, not unsigned u16. A tiny
            // HUD badge must never sample backward or wrap its step to zero.
            const int16_t stepS = (int16_t)(g[2].words.w1 >> 16);
            const int16_t stepT = (int16_t)(g[2].words.w1 & 0xFFFF);
            REQUIRE(stepS > 0 && stepT > 0);
            if (width >= 3) REQUIRE(stepS == 64 * 1024 / width);
            if (height >= 3) REQUIRE(stepT == 64 * 1024 / height);
            ++rectangle;
        }
    }
    REQUIRE(tile == 4 && rectangle == 4);
}
int main(void) {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
#ifdef TEST_MM
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#endif
    play.interfaceCtx.bAlpha = 123;
#ifdef TEST_MM
    player.transformation = PLAYER_FORM_HUMAN;
#endif
    const SwordElementUiRgbaTile* textures[] = { NULL, sSwordElementUiFireArt, sSwordElementUiIceArt, sSwordElementUiLightArt };
    for (selected = 1; selected < SWORD_ELEMENT_COUNT; ++selected) {
        Draw(SWORD_ITEM, 0, 30, 30);
        CheckDraw(textures[selected], 220, 18, 12, 12);
        Draw(SWORD_ITEM, 0, 60, 60);
        CheckDraw(textures[selected], 240, 16, 24, 24);
        Draw(SWORD_ITEM, 0, 33, 29);
        CheckDraw(textures[selected], 222, 19, 13, 11);
        Draw(SWORD_ITEM, 0, 1, 1);
        CheckDraw(textures[selected], 201, 19, 1, 1);
        Draw(SWORD_ITEM, 0, 5, 5);
        CheckDraw(textures[selected], 204, 19, 2, 2);
    }
    selected = SWORD_ELEMENT_FIRE;
    Draw(ITEM_BOW, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    Draw(SWORD_ITEM, 1, 30, 30); REQUIRE(gfx.overlay.p == commands);
    Draw(SWORD_ITEM, 0, 0, 30); REQUIRE(gfx.overlay.p == commands);
    Draw(SWORD_ITEM, 0, 30, 0); REQUIRE(gfx.overlay.p == commands);
    gSaveContext.buttonStatus[0] = BTN_DISABLED;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    gSaveContext.buttonStatus[0] = BTN_ENABLED;
    play.interfaceCtx.bAlpha = 0;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    play.interfaceCtx.bAlpha = 123;
    mario = 1;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    mario = 0;
    hiddenSword = 1;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    hiddenSword = 0;
    fourSword = 1;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    fourSword = 0;
    for (bossOwner = 1; bossOwner <= 2; ++bossOwner) {
        Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    }
    bossOwner = 0;
#ifdef TEST_MM
    play.interfaceCtx.bButtonInterfaceDoActionActive = 1;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    play.interfaceCtx.bButtonInterfaceDoActionActive = 0;
    play.interfaceCtx.bButtonPlayerDoActionActive = 1;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    play.interfaceCtx.bButtonPlayerDoActionActive = 0;
    player.transformation = PLAYER_FORM_FIERCE_DEITY;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    player.transformation = PLAYER_FORM_HUMAN;
    Nei_Save()->activeCustomForm = 1;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    Nei_Save()->activeCustomForm = 0;
    forcedForm = 1;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    forcedForm = CUSTOM_FORM_NONE;
#else
    play.interfaceCtx.unk_1FA = 1;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    play.interfaceCtx.unk_1FA = 0;
    transformed = 1;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    transformed = 0;
#endif
    selected = 0;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    selected = 9;
    Draw(SWORD_ITEM, 0, 30, 30); REQUIRE(gfx.overlay.p == commands);
    puts("PASS sword HUD: bundled RGBA art/load canaries, alpha, scaled/odd/tiny rectangles and steps and inactive/action/transformation gates");
}
