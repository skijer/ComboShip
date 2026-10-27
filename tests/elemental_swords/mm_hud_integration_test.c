// The actual icon renderer and HUD editor execute together; only CVar, item and
// screen-edge services are fixtures. The badge must follow the submitted icon.
#define main BadgeUnitMain
#include "hud_badge_test.c"
#undef main
#include "2s2h/BenGui/HudEditor.h"
#include "mods/extended_inventory.h"
#include <libultraship/bridge/consolevariablebridge.h>

HudEditorElementID hudEditorActiveElement = HUD_EDITOR_ELEMENT_NONE;
HudEditorElementMode hudEditorOverrideNextElemMode = HUD_EDITOR_ELEMENT_MODE_NONE;
HudEditorElement hudEditorElements[HUD_EDITOR_ELEMENT_MAX] = {
    HUD_EDITOR_ELEMENT(HUD_EDITOR_ELEMENT_B, "B Button", "B", 167, 17, 100, 255, 120, 255, "Buttons.B"),
};
static s32 mode, x = 220, y = 40;
static f32 scale = 2.0f;
s32 CVarGetInteger(const char* name, s32 fallback) {
    if (!name) return fallback;
    if (!strcmp(name, "gHudEditor.B.Mode")) return mode;
    if (!strcmp(name, "gHudEditor.B.Position.X")) return x;
    if (!strcmp(name, "gHudEditor.B.Position.Y")) return y;
    return fallback;
}
f32 CVarGetFloat(const char* name, f32 fallback) {
    return name && !strcmp(name, "gHudEditor.B.Scale") ? scale : fallback;
}
s16 OTRGetRectDimensionFromLeftEdge(f32 value) { return value - 80; }
s16 OTRGetRectDimensionFromRightEdge(f32 value) { return value + 80; }
u8 ExtInv_GetItemIconSize(u16 item) { return 32; }
void* ExtInv_GetItemIcon(u16 item) { return (void*)FIRE_TEXTURE; }
u8 Sw97_IsBowItem(u16 item) { return 0; }
u8 Sw97_IsSlingItem(u16 item) { return 0; }
u8 Sw97_EffectiveElement(u8 sling) { return 0; }
u16 Sw97_ElementIcon(u8 element) { return ITEM_NONE; }
u8 Sw97_ItemHasBombs(u16 item) { return 0; }
u8 SwitchHook_IsDepleted(void) { return 0; }
u8 Nei_HookshotLevel(void) { return 0; }

#include "sword_badge_icon_production.inc"

static void CheckIntegratedBadge(int left, int top, int width) {
    int rectangle = 0;
    const void* texture = NULL;
    for (Gfx* g = commands; g < gfx.overlay.p; ++g) {
        if ((g->words.w0 >> 24) == G_SETTIMG) texture = (const void*)g->words.w1;
        if ((g->words.w0 >> 24) != G_TEXRECT_WIDE) continue;
        /* Movable HUD modes also submit the native sword as a wide rectangle. */
        if (rectangle == 0 && texture != sSwordElementUiIceArt[0].pixels) continue;
        REQUIRE(texture == sSwordElementUiIceArt[rectangle].pixels);
        const int x4 = left * 4 + (rectangle & 1) * width * 2;
        const int y4 = top * 4 + (rectangle >> 1) * width * 2;
        REQUIRE((g->words.w0 & 0xFFFFFF) == x4 + width * 2);
        REQUIRE(g->words.w1 == y4 + width * 2);
        REQUIRE((g[1].words.w0 & 0xFFFFFF) == x4);
        REQUIRE(g[1].words.w1 == y4);
        ++rectangle;
    }
    REQUIRE(rectangle == 4);
}
int main(void) {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.transformation = PLAYER_FORM_HUMAN;
    gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
    play.interfaceCtx.bAlpha = 123;
    selected = SWORD_ELEMENT_ICE;
    SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_B, SWORD_ITEM);
    const char* swordTexture = "native sword texture boundary";
    for (mode = HUD_EDITOR_ELEMENT_MODE_MOVABLE_43; mode <= HUD_EDITOR_ELEMENT_MODE_MOVABLE_RIGHT; ++mode) {
        Begin();
        Interface_DrawItemIconTexture(&play, (TexturePtr)swordTexture, EQUIP_SLOT_B);
        const int offset = mode == HUD_EDITOR_ELEMENT_MODE_MOVABLE_LEFT ? -80 :
                           mode == HUD_EDITOR_ELEMENT_MODE_MOVABLE_RIGHT ? 80 : 0;
        CheckIntegratedBadge(x + offset + 40, y - 4, 24);
        REQUIRE(GET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_B) == SWORD_ITEM);
        REQUIRE(hudEditorActiveElement == HUD_EDITOR_ELEMENT_NONE);
    }
    mode = HUD_EDITOR_ELEMENT_MODE_VANILLA;
    Begin();
    Interface_DrawItemIconTexture(&play, (TexturePtr)swordTexture, EQUIP_SLOT_B);
    CheckIntegratedBadge(187, 15, 12);
    mode = HUD_EDITOR_ELEMENT_MODE_HIDDEN;
    Begin();
    Interface_DrawItemIconTexture(&play, (TexturePtr)swordTexture, EQUIP_SLOT_B);
    REQUIRE(Count(G_TEXRECT_WIDE) == 0 && Count(G_TEXRECT) == 0);
    mode = HUD_EDITOR_ELEMENT_MODE_VANILLA;
    SET_CUR_FORM_BTN_ITEM(EQUIP_SLOT_B, ITEM_BOW);
    Begin();
    Interface_DrawItemIconTexture(&play, (TexturePtr)swordTexture, EQUIP_SLOT_B);
    REQUIRE(Count(G_TEXRECT_WIDE) == 0);
    puts("PASS sword HUD integration: real MM icon/HUD editor, 3 anchors, 2x scale, vanilla/hidden/temp-bow states");
}
