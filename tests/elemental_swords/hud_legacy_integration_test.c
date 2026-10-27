// Real selector, combat fallback and HUD helper share one state. Only native
// resources/settings are supplied by the existing core/combat fixture.
#define main StateFixtureMain
#include "state_test.c"
#undef main
#include "sword_element.h"
#include "../../combo/render/sword_element_ui_art.h"
#ifdef SWORD_ELEMENT_TEST_MM
#include "archives/icon_item_static/icon_item_static_yar.h"
#include "mods/forms/custom_forms.h"
#define FIRE_TEXTURE gItemIconFireArrowTex
#else
#include "textures/icon_item_static/icon_item_static.h"
#define FIRE_TEXTURE gItemIconArrowFireTex
#endif
static GraphicsContext gfx;
static Gfx commands[128];
u8 Sm64Mario_IsActive(void) { return 0; }
void FrameInterpolation_RecordOpenChild(const void* a, int b) {}
void FrameInterpolation_RecordCloseChild(void) {}
#ifdef SWORD_ELEMENT_TEST_MM
void Graph_OpenDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {}
void Graph_CloseDisps(Gfx** refs, Gfx* vals, GraphicsContext* context, const char* file, s32 line) {}
#endif
#include "sword_badge_production.inc"

static void CheckLegacy(int expected) {
    const u32 nativeFlags = DinFireSword_DamageFlags(&play, &player, BASE_STRIKE);
    if (player.heldItemId == ITEM_SWORD_KOKIRI) REQUIRE(!!(nativeFlags & FIRE_STRIKE) == expected);
    const u32 meleeFlags = MELEE_FLAGS;
    const u8 setMask = nei.swordElementSet;
    memset(commands, 0, sizeof(commands));
    gfx.overlay.p = commands;
    play.state.gfxCtx = &gfx;
    Interface_DrawSwordElementBadge(&play, ITEM_SWORD_KOKIRI, 0, 200, 20, 30, 30);
    int badges = 0;
    const char* texture = NULL;
    for (Gfx* g = commands; g < gfx.overlay.p; ++g) {
        badges += (g->words.w0 >> 24) == G_TEXRECT_WIDE;
        if ((g->words.w0 >> 24) == G_SETTIMG) texture = (const char*)g->words.w1;
    }
    REQUIRE(badges == expected * 4);
    if (expected) REQUIRE((const void*)texture == sSwordElementUiFireArt[3].pixels);
    REQUIRE(MELEE_FLAGS == meleeFlags && nei.swordElementSet == setMask && MAGIC == 37);
}
int main(void) {
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(&nei, 0, sizeof(nei));
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    transformed = NO_CUSTOM_FORM;
    legacyEnabled = nativeAssets = 0;
    enabled[1] = 1;
    MAGIC = 37;
    INVENTORY.items[SLOT_ARROW_FIRE] = ITEM_NONE;
    player.actor.scale.y = 1.0f;
    player.heldItemId = ITEM_SWORD_KOKIRI;
    player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
    play.interfaceCtx.bAlpha = 123;
#ifdef SWORD_ELEMENT_TEST_MM
    player.transformation = PLAYER_FORM_HUMAN;
    player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#else
    gSaveContext.linkAge = 1;
    player.leftHandType = PLAYER_MODELTYPE_LH_SWORD;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#endif
    REQUIRE(SwordElement_ForItem(ITEM_SWORD_KOKIRI) == SWORD_ELEMENT_FIRE);
    CheckLegacy(1); /* Forced Fire works with no arrow, appearance pack or Alt Assets. */
    nativeAssets = 1; CheckLegacy(1); nativeAssets = 0;
    legacyEnabled = 1; CheckLegacy(1); legacyEnabled = 0;
    enabled[1] = 0; CheckLegacy(0);
    INVENTORY.items[SLOT_ARROW_FIRE] = ITEM_ARROW_FIRE;
    CheckLegacy(0); /* Acquisition alone does not silently choose an element. */
    REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, SWORD_ELEMENT_FIRE));
    CheckLegacy(1); /* Normal saved enchantment works with every override off. */
    INVENTORY.items[SLOT_ARROW_FIRE] = ITEM_NONE; CheckLegacy(0);
    enabled[1] = 1; CheckLegacy(1);
    REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, SWORD_ELEMENT_NONE));
    CheckLegacy(1); /* Override does not rewrite the saved explicit None. */
    REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == SWORD_ELEMENT_NONE);
    enabled[1] = 0; CheckLegacy(0);
    player.heldItemId = ITEM_NONE; CheckLegacy(0);
    puts("PASS sword HUD core/combat: asset-free overrides, independent arrow acquisition, saved choices/None, reversibility and no mutations");
}
