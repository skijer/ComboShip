#include "global.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "tests/test_require.h"
#include <string.h>

SaveContext gSaveContext;
static PlayState play;
static Player player;
static int enabled;
static s32 hazard;
static s16 sPictoState;

int32_t CVarGetInteger(const char* name, int32_t fallback) {
    return !strcmp(name, "gCheats.UnrestrictedItems") ? enabled : fallback;
}
// These tests isolate scene restrictions on Human Link. Form and special-mask hooks
// keep their native result; the existing form cheat is registered separately.
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
    return result;
}
s32 Player_GetEnvironmentalHazard(PlayState* context) {
    return hazard;
}
s16 CutsceneManager_GetCurrentCsId(void) {
    return CS_ID_NONE;
}
void Interface_SetHudVisibility(u16 visibility) {
    gSaveContext.hudVisibility = visibility;
}
void Interface_LoadItemIconImpl(PlayState* context, u8 button) {
}

#include "unrestricted_items_production.inc"

static void setup(s16 scene, u8 item) {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    enabled = 0;
    hazard = PLAYER_ENV_HAZARD_NONE;
    sPictoState = PICTO_BOX_STATE_OFF;
    play.sceneId = scene;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.actor.id = ACTOR_PLAYER;
    player.transformation = gSaveContext.save.playerForm = PLAYER_FORM_HUMAN;
    BUTTON_ITEM_EQUIP(CUR_FORM, EQUIP_SLOT_B) = ITEM_SWORD_KOKIRI;
    for (s16 i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; ++i) {
        BUTTON_ITEM_EQUIP(0, i) = item;
    }
    for (s16 i = EQUIP_SLOT_D_RIGHT; i <= EQUIP_SLOT_D_UP; ++i) {
        DPAD_BUTTON_ITEM_EQUIP(0, i) = item;
    }
    Interface_SetSceneRestrictions(&play);
}

static void checkItems(u8 status) {
    for (s16 i = EQUIP_SLOT_C_LEFT; i <= EQUIP_SLOT_C_RIGHT; ++i) {
        if (gSaveContext.buttonStatus[i] != status) {
            fprintf(stderr, "C-button %d item %u: got %u, expected %u, cheat %d\n", i, GET_CUR_FORM_BTN_ITEM(i),
                    gSaveContext.buttonStatus[i], status, enabled);
        }
        REQUIRE(gSaveContext.buttonStatus[i] == status);
    }
    for (s16 i = EQUIP_SLOT_D_RIGHT; i <= EQUIP_SLOT_D_UP; ++i) {
        REQUIRE(gSaveContext.shipSaveContext.dpad.status[i] == status);
    }
}

static void indoorToggle(void) {
    const u8 items[] = { ITEM_BOW, ITEM_BOMB, ITEM_HOOKSHOT };
    for (size_t i = 0; i < ARRAY_COUNT(items); ++i) {
        setup(SCENE_SONCHONOIE, items[i]);
        u8 original[sizeof(play.interfaceCtx.restrictions)];
        memcpy(original, &play.interfaceCtx.restrictions, sizeof(original));
        Interface_UpdateButtonsPart2(&play);
        checkItems(BTN_DISABLED);
        REQUIRE(gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED);
        enabled = 1;
        Interface_UpdateButtonsPart2(&play);
        checkItems(BTN_ENABLED);
        REQUIRE(gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_ENABLED);
        REQUIRE(!memcmp(original, &play.interfaceCtx.restrictions, sizeof(original)));
        enabled = 0;
        Interface_UpdateButtonsPart2(&play);
        checkItems(BTN_DISABLED);
        REQUIRE(gSaveContext.buttonStatus[EQUIP_SLOT_B] == BTN_DISABLED);
        REQUIRE(!memcmp(original, &play.interfaceCtx.restrictions, sizeof(original)));
    }
}

static void itemGroups(void) {
    const u8 items[] = { ITEM_BOW,       ITEM_MOONS_TEAR,    ITEM_BOTTLE, ITEM_OCARINA_OF_TIME,
                         ITEM_MASK_DEKU, ITEM_PICTOGRAPH_BOX };
    for (size_t i = 0; i < ARRAY_COUNT(items); ++i) {
        setup(SCENE_TOWN, items[i]);
        // Exercise each restriction group without altering the real form table.
        REQUIRE(gPlayerFormItemRestrictions[PLAYER_FORM_HUMAN][items[i]]);
        memset(&play.interfaceCtx.restrictions, 3, sizeof(play.interfaceCtx.restrictions));
        u8 original[sizeof(play.interfaceCtx.restrictions)];
        memcpy(original, &play.interfaceCtx.restrictions, sizeof(original));
        Interface_UpdateButtonsPart2(&play);
        checkItems(BTN_DISABLED);
        enabled = 1;
        for (int frame = 0; frame < 120; ++frame) {
            Interface_UpdateButtonsPart2(&play);
            checkItems(BTN_ENABLED);
            REQUIRE(!memcmp(original, &play.interfaceCtx.restrictions, sizeof(original)));
        }
        enabled = 0;
        Interface_UpdateButtonsPart2(&play);
        checkItems(BTN_DISABLED);
    }
}

static void nativeSpecialStates(void) {
    setup(SCENE_TOWN, ITEM_BOW);
    Interface_UpdateButtonsPart2(&play);
    checkItems(BTN_ENABLED);
    enabled = 1;
    player.stateFlags1 = PLAYER_STATE1_200000; // First-person view
    Interface_UpdateButtonsPart2(&play);
    checkItems(BTN_DISABLED);
    setup(SCENE_TOWN, ITEM_BOW);
    enabled = 1;
    gSaveContext.save.saveInfo.weekEventReg[WEEKEVENTREG_82_08 >> 8] |= WEEKEVENTREG_82_08 & 0xFF;
    Interface_UpdateButtonsPart2(&play);
    checkItems(BTN_DISABLED);
    setup(SCENE_TOWN, ITEM_BOW);
    enabled = 1;
    hazard = PLAYER_ENV_HAZARD_UNDERWATER_FLOOR;
    Interface_UpdateButtonsPart2(&play);
    checkItems(BTN_DISABLED);
    setup(SCENE_TOWN, ITEM_MASK_GIANT);
    REQUIRE(gPlayerFormItemRestrictions[PLAYER_FORM_HUMAN][ITEM_MASK_GIANT]);
    enabled = 1;
    Interface_UpdateButtonsPart2(&play);
    checkItems(BTN_DISABLED);
}

int main(void) {
    indoorToggle();
    itemGroups();
    nativeSpecialStates();
    puts("PASS MM unrestricted location items: B/C/D-pad, live toggle, native restrictions and special states");
    return 0;
}
