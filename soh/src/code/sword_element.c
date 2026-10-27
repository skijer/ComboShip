#include "global.h"
#include "sword_element.h"
#include "mods/nei_save.h"
#include "mods/extended_equipment.h"
#include "mods/items/logic/weapon_upgrades.h"
#include "mods/transformation_masks/transformation_masks.h"
extern s32 BossRemains_IsOdolwaWorn(void);
extern s32 BossRemains_IsGohtWorn(void);
extern u8 FourSword_IsEquipped(void);

enum {
    SLOT_KOKIRI,
    SLOT_MASTER,
    SLOT_BIGGORON,
    SLOT_KNIFE,
    SLOT_RAZOR,
    SLOT_GILDED,
    SLOT_TRUE_MASTER,
    SLOT_GREAT_FAIRY
};

int SwordElement_SlotForItem(uint16_t item) {
    switch (item) {
        case ITEM_SWORD_KOKIRI:
            return WeaponUpgrade_KokiriLevel() >= 2   ? SLOT_GILDED
                   : WeaponUpgrade_KokiriLevel() == 1 ? SLOT_RAZOR
                                                      : SLOT_KOKIRI;
        case ITEM_SWORD_MASTER:
            return WeaponUpgrade_HasTrueMaster() ? SLOT_TRUE_MASTER : SLOT_MASTER;
        case ITEM_SWORD_BGS:
            return WeaponUpgrade_HasGreatFairy() ? SLOT_GREAT_FAIRY : gSaveContext.bgsFlag ? SLOT_BIGGORON : SLOT_KNIFE;
        case ITEM_SWORD_KNIFE:
            return SLOT_KNIFE;
        default:
            return -1;
    }
}

uint8_t SwordElement_IsAvailable(uint8_t element) {
    static const uint8_t slots[] = { 0, SLOT_ARROW_FIRE, SLOT_ARROW_ICE, SLOT_ARROW_LIGHT };
    static const uint8_t items[] = { ITEM_NONE, ITEM_ARROW_FIRE, ITEM_ARROW_ICE, ITEM_ARROW_LIGHT };
    if (element == SWORD_ELEMENT_NONE)
        return true;
    if (element >= SWORD_ELEMENT_COUNT)
        return false;
    return gSaveContext.inventory.items[slots[element]] == items[element];
}

uint8_t SwordElement_Override(void) {
    // New menu callbacks keep these mutually exclusive. A deterministic choice
    // also handles old preferences or console writes containing multiple flags.
    static const char* options[] = { NULL, "gEnhancements.DinFireSwordDamage", "gEnhancements.IceSwordDamage",
                                     "gEnhancements.LightSwordDamage" };
    for (uint8_t element = SWORD_ELEMENT_LIGHT; element > SWORD_ELEMENT_NONE; --element)
        if (CVarGetInteger(options[element], 0))
            return element;
    return SWORD_ELEMENT_NONE;
}

// A progressive sword inherits its predecessor until the player explicitly
// chooses a new element (including None) for that upgrade.
static int SwordElement_SelectedSlot(int slot) {
    static const int8_t parents[] = { -1, -1, -1, -1, SLOT_KOKIRI, SLOT_RAZOR, SLOT_MASTER, SLOT_BIGGORON };
    while (slot >= 0 && !(Nei_Save()->swordElementSet & (1u << slot)))
        slot = parents[slot];
    return slot;
}

uint8_t SwordElement_HasSelection(uint16_t item) {
    return SwordElement_SelectedSlot(SwordElement_SlotForItem(item)) >= 0;
}

uint8_t SwordElement_Get(uint16_t item) {
    int slot = SwordElement_SelectedSlot(SwordElement_SlotForItem(item));
    if (slot < 0)
        return SWORD_ELEMENT_NONE;
    uint8_t element = Nei_Save()->swordElements[slot];
    return element < SWORD_ELEMENT_COUNT ? element : SWORD_ELEMENT_NONE;
}

uint8_t SwordElement_Set(uint16_t item, uint8_t element) {
    int slot = SwordElement_SlotForItem(item);
    if (slot < 0 || !SwordElement_IsAvailable(element))
        return false;
    Nei_Save()->swordElements[slot] = element;
    Nei_Save()->swordElementSet |= (uint8_t)(1u << slot);
    return true;
}

uint8_t SwordElement_ForItem(uint16_t item) {
    if (SwordElement_SlotForItem(item) < 0)
        return SWORD_ELEMENT_NONE;
    uint8_t forced = SwordElement_Override();
    if (forced != SWORD_ELEMENT_NONE)
        return forced;
    uint8_t element = SwordElement_Get(item);
    return SwordElement_IsAvailable(element) ? element : SWORD_ELEMENT_NONE;
}

uint8_t SwordElement_IsRegularSword(PlayState* play, Player* player) {
    if (play == NULL || player == NULL || player != GET_PLAYER(play) || TransformMasks_IsTransformedAny() ||
        BossRemains_IsOdolwaWorn() || BossRemains_IsGohtWorn() || ExtEquip_ShouldHideSwordDL() ||
        FourSword_IsEquipped() || player->actor.scale.y <= 0.0f || player->csAction != 0 ||
        (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_WATER)) ||
        (player->stateFlags2 & PLAYER_STATE2_DISABLE_DRAW) || play->transitionTrigger != TRANS_TRIGGER_OFF)
        return SWORD_ELEMENT_NONE;
    if (player->heldItemAction != PLAYER_IA_SWORD_KOKIRI && player->heldItemAction != PLAYER_IA_SWORD_MASTER &&
        player->heldItemAction != PLAYER_IA_SWORD_BIGGORON)
        return SWORD_ELEMENT_NONE;
    return SwordElement_SlotForItem(player->heldItemId) >= 0;
}

uint8_t SwordElement_Active(PlayState* play, Player* player) {
    return SwordElement_IsRegularSword(play, player) ? SwordElement_ForItem(player->heldItemId) : SWORD_ELEMENT_NONE;
}
