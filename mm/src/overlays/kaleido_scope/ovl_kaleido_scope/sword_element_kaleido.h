#ifndef SWORD_ELEMENT_KALEIDO_H
#define SWORD_ELEMENT_KALEIDO_H

#include <stdint.h>

struct PlayState;

typedef enum {
    SWORD_ELEMENT_MENU_CLOSED,
    SWORD_ELEMENT_MENU_CHOOSER,
    SWORD_ELEMENT_MENU_GROW_ORB,
    SWORD_ELEMENT_MENU_HOLD_SOURCE,
    SWORD_ELEMENT_MENU_MOVE_TO_SWORD,
    SWORD_ELEMENT_MENU_HOLD_SWORD,
} SwordElementMenuPhase;

typedef struct {
    uint16_t item;
    uint8_t phase;
    uint8_t selection;
    uint8_t timer;
    uint8_t alpha;
    uint8_t moveCooldown;
    int16_t x;
    int16_t y;
    int16_t targetX;
    int16_t targetY;
} SwordElementMenu;

enum {
    SWORD_ELEMENT_MENU_EVENT_NONE = 0,
    SWORD_ELEMENT_MENU_EVENT_CURSOR = 1 << 0,
    SWORD_ELEMENT_MENU_EVENT_ERROR = 1 << 1,
    SWORD_ELEMENT_MENU_EVENT_CANCEL = 1 << 2,
    SWORD_ELEMENT_MENU_EVENT_ARROW = 1 << 3,
    SWORD_ELEMENT_MENU_EVENT_FUSE = 1 << 4,
    SWORD_ELEMENT_MENU_EVENT_APPLY = 1 << 5,
};

void SwordElementKaleido_Reset(SwordElementMenu* menu);
uint8_t SwordElementKaleido_CanOpen(uint16_t item, uint8_t ownedAndUsable);
uint8_t SwordElementKaleido_Open(SwordElementMenu* menu, uint16_t item, uint8_t ownedAndUsable,
                               int16_t swordX, int16_t swordY);
uint8_t SwordElementKaleido_Update(SwordElementMenu* menu, uint8_t contextValid, int8_t direction,
                                 uint8_t confirm, uint8_t cancel);
void SwordElementKaleido_Draw(struct PlayState* play, const SwordElementMenu* menu, void* swordIcon);
/* Called before the native pause update so B cancels the chooser instead of closing pause. */
void KaleidoEquip_UpdateSwordElement(struct PlayState* play);

#endif
