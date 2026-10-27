#ifndef SWORD_ELEMENT_MESSAGE_H
#define SWORD_ELEMENT_MESSAGE_H

#include "variables.h"
#include "sword_element.h"
#include "ComboSwordElementReceipt.h"
#include <libultraship/bridge/consolevariablebridge.h>

static inline int SwordElement_MessageEnabled(uint8_t element) {
    // Receiving an arrow unlocks its imbue even with all cheat switches off.
    return element > SWORD_ELEMENT_NONE && element < SWORD_ELEMENT_COUNT;
}

static inline int SwordElement_MessageOwnsArrow(uint8_t element) {
    static const uint8_t items[] = { ITEM_NONE, ITEM_ARROW_FIRE, ITEM_ARROW_ICE, ITEM_ARROW_LIGHT };
    static const uint8_t slots[] = { 0, SLOT_ARROW_FIRE, SLOT_ARROW_ICE, SLOT_ARROW_LIGHT };
    return element > SWORD_ELEMENT_NONE && element < SWORD_ELEMENT_COUNT &&
           gSaveContext.save.saveInfo.inventory.items[slots[element]] == items[element];
}

// Eligibility is captured at the pickup, before native or randomizer granting.
static inline uint8_t SwordElement_MessageElementForPickup(uint16_t item) {
    uint8_t element = item == ITEM_ARROW_FIRE ? SWORD_ELEMENT_FIRE : item == ITEM_ARROW_ICE ? SWORD_ELEMENT_ICE :
                      item == ITEM_ARROW_LIGHT ? SWORD_ELEMENT_LIGHT : SWORD_ELEMENT_NONE;
    return SwordElement_MessageEnabled(element) && !SwordElement_MessageOwnsArrow(element) ? element : SWORD_ELEMENT_NONE;
}

// Ownership here verifies the grant succeeded; it does not recompute first-pickup
// eligibility, which the queued receipt carries by value across its animation.
static inline void SwordElement_MessageAppend(PlayState* play, uint8_t element) {
    if (!play || !SwordElement_MessageEnabled(element) || !SwordElement_MessageOwnsArrow(element))
        return;
    MessageContext* msgCtx = &play->msgCtx;
    msgCtx->msgLength = CwSwordElementReceipt_Append(msgCtx->font.msgBuf.schar, msgCtx->msgLength,
                                                  sizeof(msgCtx->font.msgBuf.schar), 1, element);
}
#endif
