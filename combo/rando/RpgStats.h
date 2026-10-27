#ifndef COMBO_RPG_STATS_H
#define COMBO_RPG_STATS_H

#include <stdint.h>

// Stable save order. Pickup ownership stays in OoT's randomizer; MM mirrors the
// result, so a handoff/reload is an idempotent merge rather than another grant.
typedef enum ComboRpgStat {
    COMBO_RPG_DEFENSE,
    COMBO_RPG_SPEED,
    COMBO_RPG_POWER,
    COMBO_RPG_MAGIC,
    COMBO_RPG_CRAWL,
    COMBO_RPG_CLIMB,
    COMBO_RPG_PUSH,
    COMBO_RPG_COUNT
} ComboRpgStat;

typedef struct ComboRpgState {
    uint8_t knownMask;
    uint8_t enabledMask;
    uint8_t level[COMBO_RPG_COUNT];
    uint8_t required[COMBO_RPG_COUNT]; // 0: metadata has not arrived yet
    uint8_t magicTotal;                // 96 normally, 100 for adjustable magic
    uint8_t quarterHeartsEnabled;
    uint8_t nativeMagicLevel; // actual native magic pickups, independent of RPG-derived ownership flags
} ComboRpgState;

static inline uint8_t ComboRpgState_Required(const ComboRpgState* state, int stat) {
    return state->required[stat] ? state->required[stat] : (stat == COMBO_RPG_MAGIC ? 8 : 5);
}

static inline int ComboRpgState_Enabled(const ComboRpgState* state, int stat) {
    return (state->knownMask & state->enabledMask & (1 << stat)) != 0;
}

static inline float ComboRpgState_Fraction(const ComboRpgState* state, int stat) {
    uint8_t required = ComboRpgState_Required(state, stat);
    uint8_t level = state->level[stat];
    if (!ComboRpgState_Enabled(state, stat))
        return 0.0f;
    return (float)(level < required ? level : required) / required;
}

static inline float ComboRpgState_SpeedMultiplier(const ComboRpgState* state) {
    return 1.0f + 0.4f * ComboRpgState_Fraction(state, COMBO_RPG_SPEED);
}

static inline float ComboRpgState_CrawlMultiplier(const ComboRpgState* state) {
    return 1.0f + 4.0f * ComboRpgState_Fraction(state, COMBO_RPG_CRAWL);
}

static inline float ComboRpgState_MovementBonus(const ComboRpgState* state, int stat) {
    return 5.0f * ComboRpgState_Fraction(state, stat);
}

static inline int16_t ComboRpgState_Defense(const ComboRpgState* state, int16_t healthChange) {
    return healthChange < 0 ? (int16_t)(healthChange * (1.0f - 0.5f * ComboRpgState_Fraction(state, COMBO_RPG_DEFENSE)))
                            : healthChange;
}

static inline uint8_t ComboRpgState_Power(const ComboRpgState* state, uint8_t damage, float roll) {
    float chance = ComboRpgState_Fraction(state, COMBO_RPG_POWER);
    if (chance <= 0.0f || roll > chance)
        return damage;
    return damage > 127 ? 255 : (uint8_t)(damage * 2);
}

static inline int16_t ComboRpgState_MagicCapacity(const ComboRpgState* state, int16_t nativeCapacity) {
    if (!ComboRpgState_Enabled(state, COMBO_RPG_MAGIC))
        return nativeCapacity;
    if (state->level[COMBO_RPG_MAGIC] == 0 && state->nativeMagicLevel == 0)
        return nativeCapacity;
    int required = ComboRpgState_Required(state, COMBO_RPG_MAGIC);
    int level = state->level[COMBO_RPG_MAGIC];
    int total = state->magicTotal == 100 ? 100 : 96;
    if (level > required)
        level = required;
    int statCapacity = (2 * level * total + required) / (2 * required);
    int floorCapacity = (state->nativeMagicLevel > 2 ? 2 : state->nativeMagicLevel) * 48;
    return (int16_t)(statCapacity > floorCapacity ? statCapacity : floorCapacity);
}

static inline int16_t ComboRpgState_HeartCount(const ComboRpgState* state, int16_t capacity) {
    return (capacity + (state->quarterHeartsEnabled ? 15 : 0)) / 16;
}

#endif
