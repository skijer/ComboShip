#include "global.h"
#include "sword_element_message.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef SWORD_RECEIPT_MM
#include "soh/OTRGlobals.h"
#include "overlays/misc/ovl_kaleido_scope/z_kaleido_scope.h"
#include "soh/Enhancements/item-tables/ItemTableTypes.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#endif

SaveContext gSaveContext;
static PlayState play;
static Player player;
static int options[4], grants, starts, denyGrant;
static const char receipt[] = "Original arrow receipt";

int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (!strcmp(key, "gEnhancements.DinFireSwordDamage")) return options[1];
    if (!strcmp(key, "gEnhancements.IceSwordDamage")) return options[2];
    if (!strcmp(key, "gEnhancements.LightSwordDamage")) return options[3];
    return fallback;
}

static u8* FixtureInventory(void) {
#ifdef SWORD_RECEIPT_MM
    return gSaveContext.save.saveInfo.inventory.items;
#else
    return gSaveContext.inventory.items;
#endif
}

static u8* Buffer(void) {
#ifdef SWORD_RECEIPT_MM
    return play.msgCtx.font.msgBuf.schar;
#else
    return (u8*)play.msgCtx.font.msgBuf;
#endif
}

void Message_StartTextbox(PlayState* state, u16 textId, Actor* actor) {
    (void)actor;
    assert(grants == 0 && "the original receipt must still start before the grant");
    ++starts;
#ifdef SWORD_RECEIPT_MM
    assert(textId >= 0x25 && textId <= 0x27);
    const u8 header[] = { 2, 0, 7, 255, 255, 255, 255, 255, 255, 255, 255 };
    memcpy(Buffer(), header, sizeof(header));
    memcpy(Buffer() + sizeof(header), receipt, sizeof(receipt) - 1);
    state->msgCtx.msgLength = sizeof(header) + sizeof(receipt);
    Buffer()[state->msgCtx.msgLength - 1] = 0xBF;
#else
    assert(textId >= 0x70 && textId <= 0x72);
    memcpy(Buffer(), receipt, sizeof(receipt) - 1);
    state->msgCtx.msgLength = sizeof(receipt);
    Buffer()[state->msgCtx.msgLength - 1] = 2;
    state->msgCtx.font.msgLength = state->msgCtx.msgLength;
#endif
}

u8 Item_Give(PlayState* state, u8 item) {
    (void)state;
    assert(starts == 1 && "do not open a separate tutorial textbox");
    ++grants;
    if (!denyGrant) {
        if (item == ITEM_ARROW_FIRE) FixtureInventory()[SLOT_ARROW_FIRE] = item;
        if (item == ITEM_ARROW_ICE) FixtureInventory()[SLOT_ARROW_ICE] = item;
        if (item == ITEM_ARROW_LIGHT) FixtureInventory()[SLOT_ARROW_LIGHT] = item;
    }
    return ITEM_NONE;
}

#ifndef SWORD_RECEIPT_MM
static GetItemEntry entry;
u8 gItemAgeReqs[256];
GetItemEntry ItemTable_Retrieve(s16 item) { (void)item; return entry; }
u16 Randomizer_Item_Give(PlayState* state, GetItemEntry item) {
    (void)state; (void)item;
    assert(0 && "native and randomizer arrows use their real MOD_NONE item identity");
    return 0;
}
void Player_SetPendingFlag(Player* value, PlayState* state) { (void)value; (void)state; }
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t value, ...) { (void)flag; return value; }
#endif

#include "sword_element_native_pickup.inc"

static void Reset(void) {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(FixtureInventory(), ITEM_NONE, 24);
    memset(options, 0, sizeof(options));
    grants = starts = denyGrant = 0;
#ifndef SWORD_RECEIPT_MM
    gSaveContext.language = LANGUAGE_ENG;
#endif
}

int main(void) {
    const u8 items[] = { 0, ITEM_ARROW_FIRE, ITEM_ARROW_ICE, ITEM_ARROW_LIGHT };
    const u8 slots[] = { 0, SLOT_ARROW_FIRE, SLOT_ARROW_ICE, SLOT_ARROW_LIGHT };
    const int ids[] = { 0, GI_ARROW_FIRE, GI_ARROW_ICE, GI_ARROW_LIGHT };
    for (int element = 1; element <= 3; ++element) {
        for (int enabled = 0; enabled <= 1; ++enabled) {
            for (int owned = 0; owned <= 1; ++owned) {
                for (int denied = 0; denied <= 1; ++denied) {
                    Reset();
                    options[element] = enabled;
                    denyGrant = denied;
                    if (owned) FixtureInventory()[slots[element]] = items[element];
                    assert(SwordElement_MessageElementForPickup(ITEM_BOW) == 0);
                    assert(SwordElement_MessageElementForPickup(ITEM_NONE) == 0);
                    player.getItemId = ids[element];
#ifndef SWORD_RECEIPT_MM
                    entry = (GetItemEntry)GET_ITEM(items[element], OBJECT_GI_M_ARROW, 0,
                        0x70 + element - 1, 0x80, CHEST_ANIM_LONG, ITEM_CATEGORY_MAJOR, MOD_NONE, ids[element]);
                    player.getItemEntry = entry;
#endif
                    NativeReceive(&play, &player);
                    assert(grants == 1 && starts == 1);
                    const size_t prefix = sizeof(receipt) - 1;
#ifdef SWORD_RECEIPT_MM
                    const size_t header = 11;
#else
                    const size_t header = 0;
#endif
                    assert(memcmp(Buffer() + header, receipt, prefix) == 0);
                    const int tutorial = !owned && !denied;
                    assert((Buffer()[header + prefix] == (header ? 0x10 : 4)) == tutorial);
                    const size_t length = play.msgCtx.msgLength;
                    NativeReceive(&play, &player);
                    assert(grants == 1 && starts == 1 && play.msgCtx.msgLength == length);
                }
            }
        }
    }
    puts("PASS actual native pickup branch: first arrow only, options, duplicates, veto, grant and textbox order");
}
