#include "mods/items/mm_bottles_behavior.h"
#include <cassert>
#include <cstdint>
#include <iostream>
using u8 = uint8_t;
struct PlayState {
  struct {
    int cursorItem[1]{};
  } pauseCtx;
};
constexpr int ITEM_CHATEAU_ROMANI = 0xB6, ITEM_BOTTLE = 0x14,
              ITEM_MILK_BOTTLE = 0x1A, ITEM_MILK_HALF = 0x1F;
constexpr int PAUSE_ITEM = 0, BTN_ENABLED = 0, VB_EMPTY_BOTTLE_TO_HALF_MILK = 1,
              VB_UPDATE_BOTTLE_ITEM = 2;
struct {
  struct {
    u8 items[16]{};
  } inventory;
  struct {
    u8 cButtonSlots[3]{}, buttonItems[4]{};
  } equips;
  u8 buttonStatus[4]{};
} gSaveContext;
#define osSyncPrintf(...) ((void)0)
#define BUTTON_STATUS_INDEX(x) (x)
bool GameInteractor_Should(int, bool value, ...) { return value; }
int activations = 0, reloads = 0;
extern "C" void MmMaskWear_ActivateChateauRomani() { ++activations; }
void Interface_LoadItemIcon1(PlayState *, u8) { ++reloads; }
/* CONSUME_BOTTLE */
int main() {
  assert(MmBottle_FromItemId(ITEM_CHATEAU_ROMANI) == MM_BOTTLE_CHATEAU_ROMANI);
  assert(MmBottle_GetUseBehavior(MM_BOTTLE_CHATEAU_ROMANI) ==
         MM_BOTTLE_USE_NATIVE);
  assert(MmBottle_GetUseBehavior(MM_BOTTLE_HOT_SPRING_WATER) ==
         MM_BOTTLE_USE_BLUE_FIRE);
  assert(MmBottle_GetUseBehavior(MM_BOTTLE_GOLD_DUST) ==
         MM_BOTTLE_USE_CANT_USE);
  assert(MmBottle_GetUseBehavior(static_cast<MmBottleContent>(-1)) ==
         MM_BOTTLE_USE_CANT_USE);
  PlayState play;
  gSaveContext.equips.cButtonSlots[0] = 4;
  gSaveContext.inventory.items[4] = ITEM_CHATEAU_ROMANI;
  gSaveContext.equips.buttonItems[1] = ITEM_CHATEAU_ROMANI;
  Inventory_UpdateBottleItem(&play, ITEM_BOTTLE, 1);
  assert(activations == 1 && reloads == 1);
  assert(gSaveContext.inventory.items[4] == ITEM_BOTTLE &&
         gSaveContext.equips.buttonItems[1] == ITEM_BOTTLE);
  Inventory_UpdateBottleItem(&play, ITEM_BOTTLE, 1);
  assert(activations == 1); // an empty bottle cannot reactivate the buff
  gSaveContext.inventory.items[4] = ITEM_MILK_BOTTLE;
  Inventory_UpdateBottleItem(&play, ITEM_BOTTLE, 1);
  assert(gSaveContext.inventory.items[4] == ITEM_MILK_HALF && activations == 1);
  std::cout << "PASS Chateau use, native consumption, infinite-magic "
               "activation and ordinary milk\n";
}
