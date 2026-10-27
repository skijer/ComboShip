#ifndef SWORD_ELEMENT_H
#define SWORD_ELEMENT_H

#include <stdint.h>

struct PlayState;
struct Player;

typedef enum SwordElement {
    SWORD_ELEMENT_NONE,
    SWORD_ELEMENT_FIRE,
    SWORD_ELEMENT_ICE,
    SWORD_ELEMENT_LIGHT,
    SWORD_ELEMENT_COUNT
} SwordElement;

// Stable save slots: Kokiri, Master, Biggoron, Giant's Knife, Razor,
// Gilded, True Master, Great Fairy. Never reorder or reuse these slots.
#define SWORD_ELEMENT_SLOT_COUNT 8

#ifdef __cplusplus
extern "C" {
#endif
int SwordElement_SlotForItem(uint16_t item);
uint8_t SwordElement_IsAvailable(uint8_t element);
// Menu selection uses the arrow unlock; cheats are an independent temporary override.
uint8_t SwordElement_Override(void);
uint8_t SwordElement_Get(uint16_t item);
uint8_t SwordElement_HasSelection(uint16_t item);
uint8_t SwordElement_Set(uint16_t item, uint8_t element);
uint8_t SwordElement_ForItem(uint16_t item);
uint8_t SwordElement_IsRegularSword(struct PlayState* play, struct Player* player);
uint8_t SwordElement_Active(struct PlayState* play, struct Player* player);
#ifdef __cplusplus
}
#endif
#endif
