#ifndef MM_COMBO_RPG_H
#define MM_COMBO_RPG_H

#include "../../combo/rando/RpgStats.h"

#ifdef __cplusplus
extern "C" {
#endif
int ComboRpg_IsEnabled(int stat);
float ComboRpg_SpeedMultiplier(void);
float ComboRpg_CrawlMultiplier(void);
float ComboRpg_ClimbBonus(void);
float ComboRpg_PushBonus(void);
int16_t ComboRpg_ApplyDefense(int16_t healthChange);
uint8_t ComboRpg_ApplyPower(uint8_t damage, float roll);
int16_t ComboRpg_MagicCapacity(int16_t nativeCapacity);
int ComboRpg_NativeMagicTier(int nativeTier);
void ComboRpg_RecordNativeMagic(uint8_t tier);
int16_t ComboRpg_HeartCount(int16_t capacity);
#ifdef __cplusplus
}
#endif
#endif
