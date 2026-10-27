#ifndef COMBO_GAMEPLAY_MAGIC_MIRROR_ITEM_H
#define COMBO_GAMEPLAY_MAGIC_MIRROR_ITEM_H

#include <stdint.h>

// Permanent, unshuffled travel reward. Time Gate keeps its original item and slot IDs.
#define EXT_ITEM_MAGIC_MIRROR 0x0224
#define SLOT_MAGIC_MIRROR 31
#define MAGIC_MIRROR_ICON_TEX "__OTR__textures/icon_item_custom/gItemIconMagicMirrorTex"
#define MAGIC_MIRROR_NAME_TEX "__OTR__textures/item_name_custom/gMagicMirrorNameTex"

#ifdef __cplusplus
extern "C" {
#endif
struct PlayState;
// Native randomizer GI coordinates: OoT includes its established 2.25 fit;
// MM's caller already owns the standard GetItem/CheckQueue scale.
void MagicMirror_DrawGI(struct PlayState* play);
void TravelTools_Heal(void);
uint8_t TravelTools_IsOwned(uint16_t item);
void TravelTools_Grant(uint16_t item);
uint8_t MagicMirror_IsOwned(void);
#ifdef __cplusplus
}
#endif

#endif
