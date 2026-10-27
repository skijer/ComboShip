#include <cassert>
#include <cstdio>
#include "z64.h"
#include "macros.h"
#include "mods/extended_player.h"
#include "mods/extended_inventory.h"
#include "mods/extended_equipment.h"
#include "mods/nei_save.h"

// Registry and bow-mode boundaries are unrelated to the sword aliases. A lookup
// here exposes a missing alias instead of manufacturing an action for that item.
static unsigned registryLookups;
const NeiItem* Nei_FindByItem(int32_t) {
    ++registryLookups;
    return nullptr;
}
uint8_t Sw97_EffectiveElement(uint8_t) {
    return 0;
}

SaveContext gSaveContext;
static Input input;
static Input* sControlInput = &input;
static unsigned breakCalls, chargeCalls;
u8 TransformMasks_IsTransformed() {
    return 0;
}
u8 TransformMasks_IsFDSkinMode() {
    return 0;
}
u8 WeaponUpgrade_KokiriLevel() {
    return 0;
}
s32 GerudoMhr_MeleeWeaponIndex(Player*) {
    return 0;
}
void EffectSsStick_Spawn(PlayState*, Vec3f*, s16) {
}
void func_800849EC(PlayState*) {
    ++breakCalls;
}
void Player_PlaySfx(Player*, u16) {
}
void func_808377DC(PlayState*, Player*) {
    ++chargeCalls;
}

#include "oot_equipment_action.inc"

int main() {
    const int action = ExtPlayer_GetItemAction(ITEM_EXT_SWORD_1);
    Player player{};
    player.heldItemAction = action;
    input.cur.button = BTN_B;
    for (float knifeHealth : { 0.0f, 1.0f, 8.0f }) {
        gSaveContext.bgsFlag = 0;
        gSaveContext.swordHealth = knifeHealth;
        player.unk_844 = 1;
        if (func_80842B7C(nullptr, &player) != 0 || gSaveContext.swordHealth != knifeHealth || breakCalls != 0) {
            std::fputs("FAIL: Byrna swing entered the real Giant's Knife durability/break path\n", stderr);
            return 1;
        }
        if (!Player_ActionHandler_8(&player, nullptr)) {
            std::fputs("FAIL: Byrna charge was blocked by an unrelated native sword state\n", stderr);
            return 1;
        }
    }
    assert(chargeCalls == 3);
    if (action != PLAYER_IA_SWORD_MASTER) {
        std::fprintf(stderr, "FAIL: OoT Byrna button resolves to action %d; expected ordinary melee action %d\n",
                     action, PLAYER_IA_SWORD_MASTER);
        return 1;
    }
    assert(registryLookups == 0);
    assert(ExtPlayer_GetItemAction(ITEM_EXT_SWORD_2) == PLAYER_IA_SWORD_KOKIRI);
    assert(ExtPlayer_GetItemAction(ITEM_EXT_SWORD_3) == PLAYER_IA_SWORD_KOKIRI);
    assert(ExtPlayer_GetItemAction(ITEM_SWORD_KOKIRI) == PLAYER_IA_SWORD_KOKIRI);
    assert(ExtPlayer_GetItemAction(ITEM_SWORD_MASTER) == PLAYER_IA_SWORD_MASTER);
    assert(ExtPlayer_GetItemAction(ITEM_SWORD_BGS) == PLAYER_IA_SWORD_BIGGORON);
    assert(ExtPlayer_GetItemAction(ITEM_NET) == PLAYER_IA_SWORD_MASTER);
    assert(ExtPlayer_GetItemAction(ITEM_NONE) == PLAYER_IA_NONE);
    // The same native paths still apply durability/charge rules to the real knife.
    player.heldItemAction = PLAYER_IA_SWORD_BIGGORON;
    gSaveContext.swordHealth = 1.0f;
    assert(func_80842B7C(nullptr, &player) == 1);
    assert(gSaveContext.swordHealth == 0.0f && breakCalls == 1);
    assert(Player_ActionHandler_8(&player, nullptr) == 0);
    std::puts(
        "PASS: real OoT Byrna attack/charge bypasses knife durability; real knife and other sword aliases preserved");
}
