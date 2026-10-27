/* Uses the production state machine with real engine save/pause structures. */
#ifdef SWORD_KALEIDO_MM
#include "global.h"
#else
#include "z64.h"
#endif
#include "sword_element.h"
#include "sword_element_kaleido.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

SaveContext gSaveContext;
static uint8_t sAvailable;
static uint8_t sSaved;
static unsigned sWrites;
static const uint16_t kSword = ITEM_SWORD_KOKIRI;

int SwordElement_SlotForItem(uint16_t item) { return item == kSword ? 0 : -1; }
uint8_t SwordElement_IsAvailable(uint8_t element) {
    return element == SWORD_ELEMENT_NONE || (element < SWORD_ELEMENT_COUNT && (sAvailable & (1 << element)));
}
uint8_t SwordElement_Get(uint16_t item) { return sSaved; }
uint8_t SwordElement_Set(uint16_t item, uint8_t element) {
    if (SwordElement_SlotForItem(item) < 0 || !SwordElement_IsAvailable(element)) return 0;
    sWrites++;
    sSaved = element;
    return 1;
}

static void fresh(SwordElementMenu* menu) {
    memset(&gSaveContext, 0x5a, sizeof(gSaveContext));
    SwordElementKaleido_Reset(menu);
    sAvailable = (1 << SWORD_ELEMENT_FIRE) | (1 << SWORD_ELEMENT_LIGHT);
    sSaved = SWORD_ELEMENT_NONE;
    sWrites = 0;
}
static void open(SwordElementMenu* menu) {
    assert(SwordElementKaleido_Open(menu, kSword, 1, 120, 440));
    assert(menu->phase == SWORD_ELEMENT_MENU_CHOOSER);
}
static void move(SwordElementMenu* menu, int8_t direction) {
    assert(SwordElementKaleido_Update(menu, 1, direction, 0, 0) & SWORD_ELEMENT_MENU_EVENT_CURSOR);
    for (int i = 0; i < 8; ++i) SwordElementKaleido_Update(menu, 1, 0, 0, 0);
}
static void test_open_and_fallback(void) {
    SwordElementMenu menu;
    fresh(&menu);
    assert(!SwordElementKaleido_CanOpen(ITEM_BOW, 1));
    assert(!SwordElementKaleido_CanOpen(kSword, 0));
    sAvailable = 0;
    assert(!SwordElementKaleido_CanOpen(kSword, 1));
    assert(!SwordElementKaleido_Open(&menu, kSword, 1, 0, 0));
    assert(!menu.phase);
    sAvailable = 1 << SWORD_ELEMENT_FIRE;
    sSaved = SWORD_ELEMENT_ICE; /* A disabled saved choice remains visible, gray, and cannot confirm. */
    open(&menu);
    assert(menu.selection == SWORD_ELEMENT_ICE);
}
static void test_cancel_never_changes_save_or_equipment(void) {
    SwordElementMenu menu;
    SaveContext before;
    fresh(&menu);
    before = gSaveContext;
    open(&menu);
    move(&menu, 1);
    assert(SwordElementKaleido_Update(&menu, 1, 0, 0, 1) == SWORD_ELEMENT_MENU_EVENT_CANCEL);
    assert(!menu.phase && sSaved == SWORD_ELEMENT_NONE && sWrites == 0);
    assert(memcmp(&before, &gSaveContext, sizeof(before)) == 0);
    open(&menu);
    move(&menu, 1);
    assert(SwordElementKaleido_Update(&menu, 1, 0, 1, 0) == SWORD_ELEMENT_MENU_EVENT_ARROW);
    for (int i = 0; i < 12; ++i) SwordElementKaleido_Update(&menu, 1, 0, 0, 0);
    assert(SwordElementKaleido_Update(&menu, 1, 0, 0, 1) == SWORD_ELEMENT_MENU_EVENT_CANCEL);
    assert(!menu.phase && sWrites == 0);
    assert(memcmp(&before, &gSaveContext, sizeof(before)) == 0);
}
static void test_locked_entries_and_none(void) {
    SwordElementMenu menu;
    fresh(&menu);
    open(&menu);
    move(&menu, 1);
    move(&menu, 1);
    assert(menu.selection == SWORD_ELEMENT_ICE);
    assert(SwordElementKaleido_Update(&menu, 1, 0, 1, 0) == SWORD_ELEMENT_MENU_EVENT_ERROR);
    assert(menu.phase == SWORD_ELEMENT_MENU_CHOOSER && sWrites == 0);
    move(&menu, -1);
    move(&menu, -1);
    assert(SwordElementKaleido_Update(&menu, 1, 0, 1, 0) == SWORD_ELEMENT_MENU_EVENT_APPLY);
    assert(!menu.phase && sSaved == SWORD_ELEMENT_NONE && sWrites == 1);
}
static void test_native_fusion_timing_and_no_button_mutations(void) {
    SwordElementMenu menu;
    SaveContext before;
    fresh(&menu);
    before = gSaveContext;
    open(&menu);
    move(&menu, 1);
    assert(SwordElementKaleido_Update(&menu, 1, 0, 1, 0) == SWORD_ELEMENT_MENU_EVENT_ARROW);
    assert(menu.phase == SWORD_ELEMENT_MENU_GROW_ORB && menu.alpha == 0);
    for (int i = 0; i < 18; ++i) assert(SwordElementKaleido_Update(&menu, 1, 0, 0, 0) == 0);
    assert(menu.alpha == 252 && menu.phase == SWORD_ELEMENT_MENU_GROW_ORB);
    SwordElementKaleido_Update(&menu, 1, 0, 0, 0);
    assert(menu.alpha == 254 && menu.phase == SWORD_ELEMENT_MENU_HOLD_SOURCE);
    for (int i = 0; i < 5; ++i) SwordElementKaleido_Update(&menu, 1, 0, 0, 0);
    assert(menu.phase == SWORD_ELEMENT_MENU_MOVE_TO_SWORD);
    for (int i = 0; i < 6; ++i) SwordElementKaleido_Update(&menu, 1, 0, 0, 0);
    assert(menu.phase == SWORD_ELEMENT_MENU_HOLD_SWORD && menu.x == 120 && menu.y == 440);
    for (int i = 0; i < 3; ++i) assert(SwordElementKaleido_Update(&menu, 1, 0, 0, 0) == 0);
    assert(sWrites == 0);
    assert(SwordElementKaleido_Update(&menu, 1, 0, 0, 0) ==
           (SWORD_ELEMENT_MENU_EVENT_FUSE | SWORD_ELEMENT_MENU_EVENT_APPLY));
    assert(!menu.phase && sSaved == SWORD_ELEMENT_FIRE && sWrites == 1);
    assert(memcmp(&before, &gSaveContext, sizeof(before)) == 0);
}
static void test_page_close_reset_and_revalidation(void) {
    SwordElementMenu menu;
    fresh(&menu);
    open(&menu);
    move(&menu, 1);
    SwordElementKaleido_Update(&menu, 1, 0, 1, 0);
    assert(SwordElementKaleido_Update(&menu, 0, 0, 0, 0) == SWORD_ELEMENT_MENU_EVENT_CANCEL);
    assert(!menu.phase && sWrites == 0);
    open(&menu);
    assert(menu.selection == SWORD_ELEMENT_NONE && menu.alpha == 0);
    move(&menu, 1);
    SwordElementKaleido_Update(&menu, 1, 0, 1, 0);
    sAvailable &= ~(1 << SWORD_ELEMENT_FIRE);
    for (int i = 0; i < 34; ++i) SwordElementKaleido_Update(&menu, 1, 0, 0, 0);
    assert(menu.phase == SWORD_ELEMENT_MENU_CHOOSER && sWrites == 0);
    SwordElementKaleido_Reset(&menu);
    assert(!menu.phase && !menu.timer && !menu.alpha && !menu.moveCooldown);
}
int main(void) {
    test_open_and_fallback();
    test_cancel_never_changes_save_or_equipment();
    test_locked_entries_and_none();
    test_native_fusion_timing_and_no_button_mutations();
    test_page_close_reset_and_revalidation();
    puts("sword elemental Kaleido state: PASS");
}
