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
           gSaveContext.inventory.items[slots[element]] == items[element];
}

// Called only at a real pickup, before its existing grant mutates inventory.
static inline uint8_t SwordElement_MessageElementForPickup(uint16_t item) {
    uint8_t element = item == ITEM_ARROW_FIRE ? SWORD_ELEMENT_FIRE : item == ITEM_ARROW_ICE ? SWORD_ELEMENT_ICE :
                      item == ITEM_ARROW_LIGHT ? SWORD_ELEMENT_LIGHT : SWORD_ELEMENT_NONE;
    return SwordElement_MessageEnabled(element) && !SwordElement_MessageOwnsArrow(element) ? element : SWORD_ELEMENT_NONE;
}

// Called after the existing message loader and grant, before any text is drawn.
static inline void SwordElement_MessageAppend(PlayState* play, uint8_t element) {
    if (!play || !SwordElement_MessageEnabled(element) || !SwordElement_MessageOwnsArrow(element) ||
        gSaveContext.language == LANGUAGE_JPN)
        return;
    MessageContext* msgCtx = &play->msgCtx;
    const size_t length = CwSwordElementReceipt_Append(
        (uint8_t*)msgCtx->font.msgBuf, msgCtx->msgLength, sizeof(msgCtx->font.msgBuf), 0, element);
    if (length != (size_t)msgCtx->msgLength)
        msgCtx->msgLength = msgCtx->font.msgLength = length;
}
#endif
