/* Run the actual pre-update hook and native equip continuation, not a parallel menu model. */
#ifdef SWORD_KALEIDO_MM
#include "overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_equipment.c"
#else
#include "overlays/misc/ovl_kaleido_scope/z_kaleido_equipment.c"
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

SaveContext gSaveContext;
u16 gEquipMasks[4] = { 15, 240, 3840, 61440 };
u16 gEquipNegMasks[4] = { 65520, 65295, 61695, 4095 };
u8 gEquipShifts[4] = { 0, 4, 8, 12 };

Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale = 1.0f;
s8 gSfxDefaultReverb;
static PlayState sPlay;
static uint8_t sAvailable = (1 << SWORD_ELEMENT_FIRE) | (1 << SWORD_ELEMENT_LIGHT);
static uint8_t sSaved;
static unsigned sWrites;
static u16 sLastSound;
static int sRemoveEquipment;
static int sSwordToggle;
static u8 sKokiriLevel;
static u8 sFairyOwned;
static unsigned sIconLoads;

int SwordElement_SlotForItem(uint16_t item) { return item == ITEM_SWORD_KOKIRI ? 0 : -1; }
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
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (strstr(key, "EquipmentCanBeRemoved")) return sRemoveEquipment;
    if (strstr(key, "SwordToggle")) return sSwordToggle;
    return fallback;
}
int ExtEquip_GetPage(void) { return 0; }
u8 ExtEquip_HasItem(s16 type, u8 index) { return 0; }
u8 ExtEquip_CheckAgeReq(s16 type, u8 index) { return 1; }
void ExtEquip_Unequip(s16 type) { assert(type == EQUIP_TYPE_SWORD); }
u8 ExtEquip_GetCurrent(s16 type) { return 0; }
u8 ExtEquip_TridentAllowsShield(u8 ext, u16 native) { return 1; }
u8 WeaponUpgrade_KokiriLevel(void) { return sKokiriLevel; }
u8 WeaponUpgrade_HasGreatFairy(void) { return sFairyOwned; }
u8 WeaponUpgrade_HasRazor(void) { return sKokiriLevel >= 1; }
u8 WeaponUpgrade_HasGilded(void) { return sKokiriLevel >= 2; }

#ifdef SWORD_KALEIDO_MM
u8 ExtEquip_SlotRetired(s16 type, u8 index) { return 0; }
u16 ExtEquip_GetItemId(s16 type, u8 index) { return ITEM_EXT_SWORD_1 + index - 1; }
static NeiSaveData sNei;
static int sActiveGame;
NeiSaveData* Nei_Save(void) { return &sNei; }
int FleetShipCombo_GetActiveGame(void) { return sActiveGame; }
void ExtEquip_RecordNativeSwordOwnership(void) {}
void ExtEquip_RecordNativeShieldOwnership(void) {}
void ExtEquip_RefreshPlayer(void) {}
void ExtEquip_Equip(s16 type, u8 index) { assert(0); }
void Interface_LoadItemIconImpl(PlayState* play, u8 button) { assert(button == EQUIP_SLOT_B); sIconLoads++; }
void Audio_PlaySfx(u16 sound) { sLastSound = sound; }
s32 BrokenItems_FormCount(void) { return 1; }
#else
uint8_t ExtInv_GetEquipAgeReq(uint8_t row, uint8_t col) { return 9; }
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) { return result; }
u32 gBitFlags[32] = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };
u8 gEquipAgeRequirements[4][4] = { { 9, 9, 9, 9 }, { 9, 9, 9, 9 }, { 9, 9, 9, 9 }, { 9, 9, 9, 9 } };
void Inventory_ChangeEquipment(s16 type, u16 value) {
    gSaveContext.equips.equipment = (gSaveContext.equips.equipment & gEquipNegMasks[type]) | value << gEquipShifts[type];
}
void Interface_LoadItemIcon1(PlayState* play, u16 button) { assert(button == 0); sIconLoads++; }
void Flags_SetInfTable(s32 flag) { gSaveContext.infTable[flag >> 4] |= 1 << (flag & 15); }
void Flags_UnsetInfTable(s32 flag) { gSaveContext.infTable[flag >> 4] &= ~(1 << (flag & 15)); }
void Audio_PlaySfxGeneral(u16 sound, Vec3f* pos, u8 token, f32* freq, f32* volume, s8* reverb) { sLastSound = sound; }
#endif

static void setup(void) {
    memset(&sPlay, 0, sizeof(sPlay));
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    sWrites = sIconLoads = 0;
    sSaved = SWORD_ELEMENT_NONE;
    sLastSound = 0;
    sKokiriLevel = sFairyOwned = 0;
    sRemoveEquipment = 1;
#ifdef SWORD_KALEIDO_MM
    memset(&sNei, 0, sizeof(sNei));
    sActiveGame = 0;
    sNei.comboObtainedFc[FCI_KOKIRI_SWORD] = 1;
    sEquipSubPage = EQUIP_SUBPAGE_VANILLA;
    sEquipCursorX = 1;
    sEquipCursorY = 0;
    sPlay.pauseCtx.state = PAUSE_STATE_MAIN;
    sPlay.pauseCtx.mainState = PAUSE_MAIN_STATE_IDLE;
    sPlay.pauseCtx.pageIndex = PAUSE_MASK;
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) = ITEM_SWORD_KOKIRI;
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) = ITEM_BOW;
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_DOWN) = ITEM_ARROW_LIGHT;
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_RIGHT) = ITEM_BOMB;
    C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) = SLOT_BOW;
    C_SLOT_EQUIP(0, EQUIP_SLOT_C_DOWN) = SLOT_ARROW_LIGHT;
    C_SLOT_EQUIP(0, EQUIP_SLOT_C_RIGHT) = SLOT_BOMB;
    SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI);
#else
    sSwordToggle = SWORD_TOGGLE_BOTH_AGES;
    sBrokenTransformPage = 0;
    sPlay.pauseCtx.state = 6;
    sPlay.pauseCtx.pageIndex = PAUSE_EQUIP;
    sPlay.pauseCtx.cursorX[PAUSE_EQUIP] = 1;
    sPlay.pauseCtx.cursorY[PAUSE_EQUIP] = 0;
    sPlay.pauseCtx.cursorItem[PAUSE_EQUIP] = ITEM_SWORD_KOKIRI;
    gSaveContext.linkAge = LINK_AGE_CHILD;
    gSaveContext.inventory.equipment = 1;
    gSaveContext.equips.equipment = EQUIP_VALUE_SWORD_KOKIRI;
    gSaveContext.equips.buttonItems[0] = ITEM_SWORD_KOKIRI;
    gSaveContext.equips.buttonItems[1] = ITEM_BOW;
    gSaveContext.equips.buttonItems[2] = ITEM_ARROW_LIGHT;
    gSaveContext.equips.buttonItems[3] = ITEM_BOMB;
    gSaveContext.equips.cButtonSlots[0] = SLOT_BOW;
    gSaveContext.equips.cButtonSlots[1] = SLOT_ARROW_LIGHT;
    gSaveContext.equips.cButtonSlots[2] = SLOT_BOMB;
#endif
    SwordElementKaleido_Reset(&sSwordElementMenu);
}
static void begin(void) {
    assert(SwordElementKaleido_Open(&sSwordElementMenu, ITEM_SWORD_KOKIRI, 1, 0, 0));
    sSwordElementRow = 0;
    sSwordElementCol = 1;
    sSwordElementPage = 0;
#ifndef SWORD_KALEIDO_MM
    sSwordElementCursorItem = ITEM_SWORD_KOKIRI;
#endif
}
static void press(u16 button) {
    sPlay.state.input[0].press.button = button;
    sPlay.state.input[0].cur.button = button;
    KaleidoEquip_UpdateSwordElement(&sPlay);
}
static void assert_c_buttons(void) {
#ifdef SWORD_KALEIDO_MM
    assert(BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_LEFT) == ITEM_BOW);
    assert(BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_DOWN) == ITEM_ARROW_LIGHT);
    assert(BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_RIGHT) == ITEM_BOMB);
    assert(C_SLOT_EQUIP(0, EQUIP_SLOT_C_LEFT) == SLOT_BOW);
    assert(C_SLOT_EQUIP(0, EQUIP_SLOT_C_DOWN) == SLOT_ARROW_LIGHT);
    assert(C_SLOT_EQUIP(0, EQUIP_SLOT_C_RIGHT) == SLOT_BOMB);
#else
    assert(gSaveContext.equips.buttonItems[1] == ITEM_BOW);
    assert(gSaveContext.equips.buttonItems[2] == ITEM_ARROW_LIGHT);
    assert(gSaveContext.equips.buttonItems[3] == ITEM_BOMB);
    assert(gSaveContext.equips.cButtonSlots[0] == SLOT_BOW);
    assert(gSaveContext.equips.cButtonSlots[1] == SLOT_ARROW_LIGHT);
    assert(gSaveContext.equips.cButtonSlots[2] == SLOT_BOMB);
#endif
}
static void test_b_cancel_consumed_and_no_native_mutation(void) {
    SaveContext before;
    setup(); begin();
    before = gSaveContext;
    press(BTN_B);
    assert(sSwordElementMenu.phase == SWORD_ELEMENT_MENU_CLOSED);
    assert(!(sPlay.state.input[0].press.button & BTN_B));
    assert(memcmp(&before, &gSaveContext, sizeof(before)) == 0);
    assert(sWrites == 0 && sIconLoads == 0);
}
static void test_chooser_dpad_cannot_flip_page(void) {
    setup(); begin();
    press(BTN_DRIGHT);
    assert(sSwordElementMenu.selection == SWORD_ELEMENT_FIRE);
    assert(!(sPlay.state.input[0].press.button & BTN_DRIGHT));
    assert(!(sPlay.state.input[0].cur.button & BTN_DRIGHT));
    assert(sWrites == 0);
}
static void test_none_and_fusion_continue_through_native_equipping(void) {
    setup(); begin();
    press(BTN_A);
    assert(sWrites == 1 && sSaved == SWORD_ELEMENT_NONE && sIconLoads == 1);
#ifdef SWORD_KALEIDO_MM
    assert(BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) == ITEM_SWORD_KOKIRI);
    assert(GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_KOKIRI);
#else
    assert(gSaveContext.equips.buttonItems[0] == ITEM_SWORD_KOKIRI);
    assert(CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_KOKIRI);
#endif
    assert_c_buttons();
    setup(); begin();
    press(BTN_DRIGHT);
    for (int i = 0; i < 8; ++i) press(0);
    press(BTN_A);
    assert(sLastSound == NA_SE_SY_SET_FIRE_ARROW && sWrites == 0);
    for (int i = 0; i < 34; ++i) press(0);
    assert(sWrites == 1 && sSaved == SWORD_ELEMENT_FIRE && sIconLoads == 1);
    assert(sLastSound == NA_SE_SY_SYNTH_MAGIC_ARROW);
#ifdef SWORD_KALEIDO_MM
    assert(BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) == ITEM_SWORD_KOKIRI);
#else
    assert(gSaveContext.equips.buttonItems[0] == ITEM_SWORD_KOKIRI);
#endif
    assert_c_buttons();
}
static void test_page_departure_and_shoulders_reset_without_consumption(void) {
    setup(); begin();
    press(BTN_R);
    assert(!sSwordElementMenu.phase && (sPlay.state.input[0].press.button & BTN_R));
    assert(!sWrites && !sIconLoads);
    setup(); begin();
#ifdef SWORD_KALEIDO_MM
    sPlay.pauseCtx.pageIndex = PAUSE_ITEM;
#else
    sPlay.pauseCtx.pageIndex = PAUSE_ITEM;
#endif
    press(0);
    assert(!sSwordElementMenu.phase && !sWrites);
}
static void test_native_sword_families_and_upgrades(void) {
    setup();
#ifdef SWORD_KALEIDO_MM
    sKokiriLevel = 2;
    KaleidoEquip_EquipCell(&sPlay, 0, 1);
    assert(BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) == ITEM_SWORD_GILDED);
    assert(GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_GILDED);
    sNei.comboObtained[FC_OOT_SWORD_MASTER] = 1;
    KaleidoEquip_EquipCell(&sPlay, 0, 2);
    assert(BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) == ITEM_SWORD_MASTER);
    sFairyOwned = 1;
    KaleidoEquip_EquipCell(&sPlay, 0, 3);
    assert(BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) == ITEM_SWORD_BGS);
    assert_c_buttons();
    /* The UI publishes the upgraded icon identity while the native continuation keeps its B identity. */
    EquipCell cell;
    KaleidoEquip_GetCell(0, 0, 1, &cell);
    assert(cell.item == ITEM_SWORD_GILDED);
    KaleidoEquip_GetCell(0, 0, 3, &cell);
    assert(cell.item == ITEM_SWORD_GREAT_FAIRY);
#else
    gSaveContext.inventory.equipment |= 6;
    sPlay.pauseCtx.cursorX[PAUSE_EQUIP] = 2;
    assert(KaleidoEquip_EquipNative(&sPlay, ITEM_SWORD_MASTER, false));
    assert(gSaveContext.equips.buttonItems[0] == ITEM_SWORD_MASTER);
    sPlay.pauseCtx.cursorX[PAUSE_EQUIP] = 3;
    gSaveContext.bgsFlag = 1;
    assert(KaleidoEquip_ElementItem(ITEM_HEART_PIECE_2) == ITEM_SWORD_BGS);
    assert(KaleidoEquip_EquipNative(&sPlay, ITEM_HEART_PIECE_2, false));
    assert(gSaveContext.equips.buttonItems[0] == ITEM_SWORD_BGS);
    assert(gSaveContext.swordHealth == 8);
    assert_c_buttons();
#endif
}
#ifndef SWORD_KALEIDO_MM
static void test_native_toggle_still_applies_outside_chooser(void) {
    setup();
    assert(KaleidoEquip_EquipNative(&sPlay, ITEM_SWORD_KOKIRI, true));
    assert(CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) == EQUIP_VALUE_SWORD_NONE);
    assert(gSaveContext.equips.buttonItems[0] == ITEM_NONE);
    assert_c_buttons();
}
#endif
#if defined(SWORD_KALEIDO_MM) && !defined(COMBO_BUILD)
static void test_legacy_free_cells_require_actual_ownership_for_enchanting(void) {
    EquipCell cell;
    setup();
    sActiveGame = -1;
    sNei.comboObtainedFc[FCI_KOKIRI_SWORD] = 0;
    SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_NONE);
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) = ITEM_NONE;
    KaleidoEquip_GetCell(0, 0, 1, &cell);
    assert(KaleidoEquip_CellOwned(&cell)); /* preserve the native standalone display/equip policy */
    assert(!KaleidoEquip_ElementUsable(&cell));
    sEquipCursorX = 2;
    KaleidoEquip_GetCell(0, 0, 2, &cell);
    assert(!KaleidoEquip_ElementUsable(&cell));
    sNei.comboObtained[FC_OOT_SWORD_MASTER] = 1;
    assert(KaleidoEquip_ElementUsable(&cell));
}
#endif
int main(void) {
#if defined(SWORD_KALEIDO_MM) && !defined(COMBO_BUILD)
    test_legacy_free_cells_require_actual_ownership_for_enchanting();
#endif
    test_b_cancel_consumed_and_no_native_mutation();
    test_chooser_dpad_cannot_flip_page();
    test_none_and_fusion_continue_through_native_equipping();
    test_native_sword_families_and_upgrades();
    test_page_departure_and_shoulders_reset_without_consumption();
#ifndef SWORD_KALEIDO_MM
    test_native_toggle_still_applies_outside_chooser();
#endif
    puts("sword elemental Kaleido native input/equip: PASS");
}
