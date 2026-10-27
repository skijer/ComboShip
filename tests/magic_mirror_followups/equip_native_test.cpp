// Compiles the actual accessor TU. OoT's runner also includes the exact native
// pause commit block, so tests exercise the real call site and swap behavior.
#include "z64.h"
#include "gameplay/MagicMirrorItem.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

extern "C" { SaveContext gSaveContext; }
static int sIconLoads[8];
static int sSeparateArrows = 0;
void Interface_LoadItemIcon1(PlayState*, u16 button) { ++sIconLoads[button]; }
void Interface_LoadItemIcon2(PlayState*, u16 button) { ++sIconLoads[button]; }
void Interface_LoadItemIconImpl(PlayState*, u8 button) { ++sIconLoads[button]; }
void Interface_Dpad_LoadItemIconImpl(PlayState*, s32 button) { ++sIconLoads[button + 4]; }
int CVarGetInteger(const char*, int) { return sSeparateArrows; }

#ifdef MIRROR_FIXTURE_MM
#include "mods/ext_buttons/ext_buttons.cpp"
#else
#include "mods/ext_buttons/ext_buttons.cpp"
#include "oot_pause_commit.inc"
#endif

static void Check(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static void Reset() {
    gSaveContext = {};
    std::memset(gSaveContext.equips.buttonItems, ITEM_NONE, sizeof(gSaveContext.equips.buttonItems));
    std::memset(gSaveContext.equips.cButtonSlots, SLOT_NONE, sizeof(gSaveContext.equips.cButtonSlots));
    std::memset(gSaveContext.save.saveInfo.equips.buttonItems, ITEM_NONE,
                sizeof(gSaveContext.save.saveInfo.equips.buttonItems));
    std::memset(gSaveContext.save.saveInfo.equips.cButtonSlots, SLOT_NONE,
                sizeof(gSaveContext.save.saveInfo.equips.cButtonSlots));
    std::memset(gSaveContext.save.shipSaveInfo.dpadEquips.items, ITEM_NONE,
                sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.items));
    std::memset(gSaveContext.save.shipSaveInfo.dpadEquips.slots, SLOT_NONE,
                sizeof(gSaveContext.save.shipSaveInfo.dpadEquips.slots));
    std::memset(sIconLoads, 0, sizeof(sIconLoads));
    gSaveContext.equips.buttonItems[0] = 0x1A;
    gSaveContext.ship.childExtButtons.items[2] = 0x0311;
    gSaveContext.ship.adultExtButtons.items[2] = 0x0312;
    BUTTON_ITEM_EQUIP(0, 0) = 0x1B;
    BUTTON_ITEM_EQUIP(1, 1) = ITEM_EXT_BUTTON;
    gSaveContext.save.shipSaveInfo.extButtons.items[1][1] = 0x0313;
}
static u16 Item(int button) {
#ifdef MIRROR_FIXTURE_MM
    return button < 4 ? ExtButton_GetItem(0, button) : ExtButton_GetDpadItem(0, button - 4);
#else
    return ExtButton_GetItem(button);
#endif
}
static u8 Slot(int button) {
#ifdef MIRROR_FIXTURE_MM
    return button < 4 ? C_SLOT_EQUIP(0, button) : DPAD_SLOT_EQUIP(0, button - 4);
#else
    return gSaveContext.equips.cButtonSlots[button - 1];
#endif
}
static void Put(int button, u16 item, u8 slot) {
#ifdef MIRROR_FIXTURE_MM
    if (button >= 4) {
        ExtButton_SetDpadItem(0, button - 4, item);
        DPAD_SLOT_EQUIP(0, button - 4) = slot;
    } else {
        if (item >= 0x0200) ExtButton_SetItem(0, button, item);
        else { ExtButton_ClearItem(0, button); BUTTON_ITEM_EQUIP(0, button) = (u8)item; }
        C_SLOT_EQUIP(0, button) = slot;
    }
#else
    if (item >= 0x0200) ExtButton_SetItem(button, item);
    else { ExtButton_ClearItem(button); gSaveContext.equips.buttonItems[button] = (u8)item; }
    gSaveContext.equips.cButtonSlots[button - 1] = slot;
#endif
}
static void Equip(PlayState* play, int button, u16 item, u8 slot) {
#ifdef MIRROR_FIXTURE_MM
    Check(ExtButton_EquipItem(play, button - 1, item, slot), "Mirror interaction must be handled");
#else
    play->pauseCtx.equipTargetCBtn = button - 1;
    play->pauseCtx.equipTargetItem = item;
    play->pauseCtx.equipTargetSlot = slot;
    CommitPauseEquip(play);
#endif
    Check(gSaveContext.equips.buttonItems[0] == 0x1A && BUTTON_ITEM_EQUIP(0, 0) == 0x1B,
          "B-button mappings are preserved");
    Check(gSaveContext.ship.childExtButtons.items[2] == 0x0311 &&
              gSaveContext.ship.adultExtButtons.items[2] == 0x0312 &&
              BUTTON_ITEM_EQUIP(1, 1) == ITEM_EXT_BUTTON &&
              gSaveContext.save.shipSaveInfo.extButtons.items[1][1] == 0x0313,
          "Stored ages and inactive forms are preserved");
}
int main() {
    PlayState play;
    // Literal 31 is OoT's travel cell; MM uses its disjoint native equip band.
#ifdef MIRROR_FIXTURE_MM
    constexpr u8 travelSlot = 79;
#else
    constexpr u8 travelSlot = 31;
#endif
    for (int source = 1; source <= 7; ++source) {
        for (int target = 1; target <= 7; ++target) {
            if (source == target) continue;
            for (u16 displaced : { u16(ITEM_NONE), u16(9), u16(0x0209) }) {
                Reset();
                Put(source, EXT_ITEM_MAGIC_MIRROR, source < 4 ? SLOT_NONE : travelSlot);
                Put(target, displaced, displaced >= 0x0200 || displaced == ITEM_NONE ? SLOT_NONE : 11);
                Equip(&play, target, EXT_ITEM_MAGIC_MIRROR, travelSlot);
                Check(Item(target) == EXT_ITEM_MAGIC_MIRROR, "Mirror lands on requested button");
                Check(Item(source) == displaced, "Mirror move swaps or clears its previous button");
                Check(sIconLoads[target] > 0 && sIconLoads[source] > 0, "Both changed HUD icons refresh");
                for (int other = 1; other <= 7; ++other)
                    if (other != target && other != source)
                        Check(Item(other) == ITEM_NONE, "Unrelated empty buttons are preserved");
            }
            // Moving a native item onto Mirror must carry Mirror's full ID back.
            Reset();
            Put(target, EXT_ITEM_MAGIC_MIRROR, target < 4 ? SLOT_NONE : travelSlot);
            Put(source, 9, 11);
            Equip(&play, target, 9, 11);
            Check(Item(source) == EXT_ITEM_MAGIC_MIRROR && Item(target) == 9,
                  "Native item move preserves displaced Mirror's u16 ID");
        }
    }
    Reset();
    Put(1, EXT_ITEM_MAGIC_MIRROR, SLOT_NONE);
    Put(2, EXT_ITEM_MAGIC_MIRROR, SLOT_NONE);
    Put(5, EXT_ITEM_MAGIC_MIRROR, travelSlot);
    Put(3, 0x0209, SLOT_NONE);
    Put(6, 0x0210, SLOT_NONE);
    Put(7, 21, 18);
    Equip(&play, 3, EXT_ITEM_MAGIC_MIRROR, travelSlot);
    Check(Item(1) == 0x0209 && Item(2) == ITEM_NONE && Item(5) == ITEM_NONE,
          "Legacy Mirror duplicates are removed without duplicating the displaced item");
    Check(Item(7) == 21 && Slot(7) == 18, "Unrelated NEI/native slot is preserved");
    Check(Item(6) == 0x0210 && Slot(6) == SLOT_NONE, "Another extended item with SLOT_NONE is preserved");
    // Re-equipping the already selected button must not swap another Mirror back.
    Put(4, EXT_ITEM_MAGIC_MIRROR, travelSlot);
    Equip(&play, 3, EXT_ITEM_MAGIC_MIRROR, travelSlot);
    Check(Item(3) == EXT_ITEM_MAGIC_MIRROR && Item(4) == ITEM_NONE,
          "Re-equipping the same button heals duplicate Mirror mappings");
#ifdef MIRROR_FIXTURE_MM
    Reset();
    Put(1, 9, 11);
    Put(2, 21, 18);
    Check(!ExtButton_EquipItem(&play, 0, 21, 18), "Native-only MM equip retains native owner");
    Check(Item(1) == 9 && Item(2) == 21, "Native-only MM helper is a no-op");
#else
    Reset();
    Put(1, 9, 11);
    Put(2, 21, 18);
    Equip(&play, 1, 21, 18);
    Check(Item(1) == 21 && Slot(1) == 18 && Item(2) == 9 && Slot(2) == 11,
          "Native-only OoT swap retains the original behavior");
    Check(!ExtButton_EquipMagicMirror(&play, 0, EXT_ITEM_MAGIC_MIRROR, travelSlot) &&
              !ExtButton_EquipMagicMirror(&play, 8, EXT_ITEM_MAGIC_MIRROR, travelSlot),
          "Mirror helper rejects B and out-of-range indices");
#endif
    std::puts("PASS: native C/D Mirror moves, duplicate cleanup, full IDs, unrelated slots");
}
