#!/usr/bin/env python3
"""Run OoT's production equipment-page cycle and owned equip/load boundaries.

The pause control block, transform handler, ownership accessors and equipment
management functions are read from production. Only CVars, player refresh,
audio, rendering and item behavior side effects are fixture boundaries.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^(?:static\s+)?(?:u8|u16|u32|s16|s32|void|int)\s+" + re.escape(name) +
                      r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise ValueError(name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


def fixture():
    equipment = (ROOT / "soh/mods/extended_equipment.c").read_text()
    pause = (ROOT / "soh/src/overlays/misc/ovl_kaleido_scope/z_kaleido_equipment.c").read_text()
    first = pause.index("        // Tick ExtEquip cooldown here")
    cycle = pause[first:pause.index("        // On extended page", first)]
    prefix = r'''
#include <cstdio>
#include <cstring>
#include <initializer_list>
struct Player;
struct PlayState;
struct LinkAnimationHeader;
#include "soh/mods/extended_equipment.h"
#include "soh/mods/nei_save.h"
#define EXT_EQUIP_PAGE_SWITCH_COOLDOWN 15
#define TRADE_ADULT_PENDANT 1
#define MM_PLAYER_FORM_PIKACHU 9
#define PAUSE_EQUIP 1
#define CHECK_BTN_ALL(value, bits) (((value) & (bits)) == (bits))
#define CVAR_ENHANCEMENT(key) "gEnhancements." key
#define CVAR_SETTING(key) "gSettings." key
#define ABS(value) ((value) < 0 ? -(value) : (value))
#define NA_SE_SY_DECIDE 1
#define NA_SE_SY_CURSOR 2
static int checks, failures;
void check(bool ok, const char* name) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", name); ++failures; }
}
struct Input { struct { u16 button = 0; } press; };
struct PauseContext {
    s16 stickRelX = 0, cursorColorSet = 0;
    u16 cursorItem[4] = {}, cursorSlot[4] = {};
    void* equipVtx = nullptr;
};
struct PlayState { PauseContext pauseCtx; };
ExtendedEquipmentState gExtEquipState{};
ExtEquipBehaviorState gExtEquipBehavior{};
static NeiSaveData save;
NeiSaveData* Nei_Save() { return &save; }
static int enabled, ngc, crossover, transformed, form, selectedForm;
static int setCalls, cleanupCalls, behaviorForgets;
static bool ageAllowed = true, retired, shieldCompatible = true;
static u8 sExtEquipInitInProgress;
static s16 sBrokenTransformPage, sTransformCursor, sTransformCooldown;
static int gSfxDefaultPos, gSfxDefaultFreqAndVolScale, gSfxDefaultReverb;
int CVarGetInteger(const char* key, int fallback) {
    if (!std::strcmp(key, CVAR_EXT_EQUIP_ENABLED)) return enabled;
    if (!std::strcmp(key, CVAR_ENHANCEMENT("NGCKaleidoSwitcher"))) return ngc;
    return fallback;
}
void CVarSetInteger(const char* key, int value) {
    if (!std::strcmp(key, CVAR_EXT_EQUIP_ENABLED)) enabled = value;
}
void ByrnaOrb_Forget() { ++behaviorForgets; }
void ByrnaOrb_Tick() {}
void TridentChargeBall_Tick() {}
void Byrna_Cleanup() { ++cleanupCalls; gExtEquipBehavior.byrnaActive = 0; }
void TradeAdult_GiveIndex(int) { save.pendantOwned = 1; }
u8 TradeAdult_IsOwnedIndex(int) { return 0; }
void ExtEquip_GenerateIcons() {}
u8 TransformMasks_IsTransformedAny() { return transformed; }
u8 MmForm_GetCurrentForm() { return form; }
u8 ExtEquip_SlotRetired(s16, u8) { return retired; }
u8 ExtEquip_CheckAgeReq(s16, u8) { return ageAllowed; }
u8 ExtEquip_TridentAllowsShield(u8, u16) { return shieldCompatible; }
u8 ExtEquip_GetCurrent(s16 type) {
    const u8 slots[] = {gExtEquipState.currentExtSword, gExtEquipState.currentExtShield,
                        gExtEquipState.currentExtTunic, gExtEquipState.currentExtBoots};
    return type >= 0 && type < 4 ? slots[type] : 0;
}
void ExtEquip_SetSlot(s16 type, u8 index) {
    ++setCalls;
    u8* slots[] = {&gExtEquipState.currentExtSword, &gExtEquipState.currentExtShield,
                  &gExtEquipState.currentExtTunic, &gExtEquipState.currentExtBoots};
    u8* saved[] = {&save.extEquipSword, &save.extEquipShield,
                  &save.extEquipTunic, &save.extEquipBoots};
    if (type >= 0 && type < 4) *slots[type] = *saved[type] = index;
}
bool BrokenItems_Enabled() { return crossover; }
s16 BrokenItems_CurrentForm() { return selectedForm; }
s32 BrokenItems_FormCount() { return 3; }
void BrokenItems_EquipForm(PlayState*, s16 value) { selectedForm = value; }
u16 BrokenItems_FormItem(s16 index) { return 100 + index; }
template<class... Args> void Audio_PlaySoundGeneral(Args...) {}
void KaleidoScope_SetCursorVtx(PauseContext*, s16, void*) {}
'''
    production = "\n".join(function(equipment, name) for name in (
        "ExtEquip_IsEnabled", "ExtEquip_GetBit", "ExtEquip_HasItem", "ExtEquip_GiveItem",
        "ExtEquip_RemoveItem", "ExtEquip_Init", "ExtEquip_Update", "ExtEquip_GetPage",
        "ExtEquip_SwitchPage", "ExtEquip_CanSwitch", "ExtEquip_Equip",
        "ExtEquip_CapeOwned", "ExtEquip_GivePendant", "ExtEquip_PendantOwned"))
    production += "\n" + function(pause, "KaleidoEquip_UpgradeCellAvailable")
    production += "\n" + function(pause, "KaleidoEquip_TransformInput")
    production += "\nstatic void Cycle(Input* input) { u8 extEquipPage = (ExtEquip_GetPage() == 1);\n"
    production += cycle + "\n}\n"
    checks = r'''
static void reset(int enable, int ngcMode, int cross) {
    save = {}; save.extTunicLayoutVersion = save.extBootsLayoutVersion = 1;
    gExtEquipState = {}; gExtEquipBehavior = {};
    enabled = enable; ngc = ngcMode; crossover = cross;
    sBrokenTransformPage = sTransformCursor = sTransformCooldown = 0;
    transformed = form = selectedForm = setCalls = cleanupCalls = 0;
    ageAllowed = shieldCompatible = true; retired = false;
}
static void press(u16 button) {
    Input input; input.press.button = button;
    if (sBrokenTransformPage) {
        PlayState play;
        KaleidoEquip_TransformInput(&play, &input);
    } else Cycle(&input);
}
int main() {
    for (int cape : {0, 1}) for (int pendant : {0, 1}) {
        reset(0, 0, 0); save.capeOwned=cape; save.pendantOwned=pendant;
        check(KaleidoEquip_UpgradeCellAvailable(0)==cape, "Cape cursor entry follows real ownership");
        check(KaleidoEquip_UpgradeCellAvailable(1)==pendant, "Pendant cursor entry follows real ownership");
        check(!KaleidoEquip_UpgradeCellAvailable(2), "third equipment-column placeholder cannot receive cursor");
        check(!KaleidoEquip_UpgradeCellAvailable(3), "fourth equipment-column placeholder cannot receive cursor");
    }
    for (int enable : {0, 1}) for (int ngcMode : {0, 1}) for (int cross : {0, 1}) {
        reset(enable, ngcMode, cross);
        u16 freed = ngcMode ? BTN_Z : BTN_L;
        u16 tabButton = ngcMode ? BTN_L : BTN_Z;
        press(tabButton);
        check(ExtEquip_GetPage() == 0 && !sBrokenTransformPage, "tab button does not cycle equipment");
        press(freed);
        check(ExtEquip_GetPage() == 1 && !sBrokenTransformPage,
              "first freed-button press reaches additional equipment with legacy toggle off or on");
        check(enabled == enable && save.extEquipOwnedBits == 0 && setCalls == 0,
              "browsing does not enable behaviors, grant items or equip gear");
        press(freed);
        check(ExtEquip_GetPage() == 1 && !sBrokenTransformPage, "cooldown prevents immediate second switch");
        gExtEquipState.pageSwitchTimer = 0;
        press(freed);
        check(ExtEquip_GetPage() == 0 && sBrokenTransformPage == cross,
              "additional equipment advances to crossover when enabled, otherwise vanilla");
        if (cross) {
            press(freed);
            check(!sBrokenTransformPage && ExtEquip_GetPage() == 0, "crossover returns to vanilla");
        }
        gExtEquipState.pageSwitchTimer = 0;
        press(freed);
        check(ExtEquip_GetPage() == 1 && !sBrokenTransformPage, "cycle is repeatable");
    }
    for (s16 type = 0; type < 4; ++type) {
        reset(0, 0, 1);
        ExtEquip_Equip(type, 1);
        check(!enabled && setCalls == 0, "unowned items remain unequippable and cannot enable behavior");
        ExtEquip_GiveItem(type, 1);
        u32 ownership = save.extEquipOwnedBits;
        ageAllowed = false; ExtEquip_Equip(type, 1);
        check(!enabled && setCalls == 0, "age restriction cannot enable behavior");
        ageAllowed = true; retired = true; ExtEquip_Equip(type, 1);
        check(!enabled && setCalls == 0, "retired slot cannot enable behavior");
        retired = false; transformed = 1; form = MM_PLAYER_FORM_PIKACHU;
        ExtEquip_Equip(type, 1);
        check(!enabled && setCalls == 0, "Pikachu restriction remains intact");
        transformed = 0; ExtEquip_Equip(type, 1);
        check(enabled && ExtEquip_GetCurrent(type) == 1 && save.extEquipOwnedBits == ownership,
              "explicit equip of owned gear enables behavior without changing ownership");
        ExtEquip_Update();
        check(ExtEquip_GetCurrent(type) == 1, "chosen loadout survives the next gameplay frame");
        ExtEquip_Equip(type, 1);
        check(ExtEquip_GetCurrent(type) == 0, "equipping worn gear still toggles it off");
        ExtEquip_Equip(type, 1);
        enabled = 0; ExtEquip_Update();
        check(ExtEquip_GetCurrent(type) == 0 && !enabled, "explicitly disabling behavior cleans up worn gear");

        reset(0, 0, 0);
        ExtEquip_GiveItem(type, 1);
        u8* saved[] = {&save.extEquipSword, &save.extEquipShield,
                      &save.extEquipTunic, &save.extEquipBoots};
        *saved[type] = 1;
        ExtEquip_Init();
        check(enabled && ExtEquip_GetCurrent(type) == 1, "persisted owned loadout restores behavior at scene initialization");
        ExtEquip_Update();
        check(ExtEquip_GetCurrent(type) == 1, "restored loadout survives gameplay update");
    }
    reset(0, 0, 0); ExtEquip_Init();
    check(!enabled && !save.extEquipOwnedBits, "empty save initialization grants nothing and stays disabled");
    reset(0, 0, 0); save.extEquipSword = 255; ExtEquip_Init();
    check(!enabled && !gExtEquipState.currentExtSword, "invalid saved index cannot activate behavior");
    reset(0, 0, 0); ExtEquip_GiveItem(EQUIP_TYPE_SHIELD, 1);
    gExtEquipState.currentExtSword = 3; shieldCompatible = false;
    ExtEquip_Equip(EQUIP_TYPE_SHIELD, 1);
    check(!enabled && !setCalls, "Trident shield incompatibility remains guarded");
    std::printf("OoT equipment access: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
'''
    return prefix + production + checks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="oot-equipment-access-") as folder:
        source = Path(folder) / "access.cpp"
        source.write_text(fixture())
        binary = Path(folder) / "access"
        flags = ["-std=c++20", "-I" + str(ROOT), "-I" + str(ROOT / "soh"),
                 "-I" + str(ROOT / "soh/include"), "-I" + str(ROOT / "libultraship/include")]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
        subprocess.run([os.environ.get("CXX", "c++"), *flags, str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
