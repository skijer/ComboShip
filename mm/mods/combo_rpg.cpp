#include "combo_rpg.h"
#include "nei_save.h"

extern "C" int ComboRpg_IsEnabled(int stat) {
    return stat >= 0 && stat < COMBO_RPG_COUNT && ComboRpgState_Enabled(&Nei_Save()->comboRpg, stat);
}
extern "C" float ComboRpg_SpeedMultiplier(void) {
    return ComboRpgState_SpeedMultiplier(&Nei_Save()->comboRpg);
}
extern "C" float ComboRpg_CrawlMultiplier(void) {
    return ComboRpgState_CrawlMultiplier(&Nei_Save()->comboRpg);
}
extern "C" float ComboRpg_ClimbBonus(void) {
    return ComboRpgState_MovementBonus(&Nei_Save()->comboRpg, COMBO_RPG_CLIMB);
}
extern "C" float ComboRpg_PushBonus(void) {
    return ComboRpgState_MovementBonus(&Nei_Save()->comboRpg, COMBO_RPG_PUSH);
}
extern "C" int16_t ComboRpg_ApplyDefense(int16_t healthChange) {
    return ComboRpgState_Defense(&Nei_Save()->comboRpg, healthChange);
}
extern "C" uint8_t ComboRpg_ApplyPower(uint8_t damage, float roll) {
    return ComboRpgState_Power(&Nei_Save()->comboRpg, damage, roll);
}
extern "C" int16_t ComboRpg_MagicCapacity(int16_t nativeCapacity) {
    return ComboRpgState_MagicCapacity(&Nei_Save()->comboRpg, nativeCapacity);
}
extern "C" int ComboRpg_NativeMagicTier(int nativeTier) {
    return ComboRpg_IsEnabled(COMBO_RPG_MAGIC) ? Nei_Save()->comboRpg.nativeMagicLevel : nativeTier;
}
extern "C" void ComboRpg_RecordNativeMagic(uint8_t tier) {
    if (tier > 2)
        tier = 2;
    if (tier > Nei_Save()->comboRpg.nativeMagicLevel)
        Nei_Save()->comboRpg.nativeMagicLevel = tier;
}
extern "C" int16_t ComboRpg_HeartCount(int16_t capacity) {
    return ComboRpgState_HeartCount(&Nei_Save()->comboRpg, capacity);
}
