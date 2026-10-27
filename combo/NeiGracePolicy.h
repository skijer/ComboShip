#pragma once
#include <stdint.h>

// Saved option values: zero keeps pre-option seeds usable.
#define NEI_GRACE_ON 0
#define NEI_GRACE_OFF 1
#define NEI_GRACE_GATED 2
#define NEI_GRACE_MAX_REWARDS 13

static inline unsigned NeiGrace_SeedMode(int mode) {
    return mode >= NEI_GRACE_ON && mode <= NEI_GRACE_GATED ? (unsigned)mode : NEI_GRACE_OFF;
}

static inline unsigned NeiGrace_SeedRequired(int required) {
    return required < 0 ? 0 : required > NEI_GRACE_MAX_REWARDS ? NEI_GRACE_MAX_REWARDS : (unsigned)required;
}

static inline unsigned NeiGrace_RewardCount(uint32_t ootQuest, uint32_t mmQuest) {
    // Six medallions, three stones, four remains. Songs and soul flags do not count.
    uint32_t oot = ootQuest & 0x001C003Fu;
    uint32_t mm = mmQuest & 0xFu;
    unsigned count = 0;
    for (; oot; oot &= oot - 1)
        ++count;
    for (; mm; mm &= mm - 1)
        ++count;
    return count;
}

static inline int NeiGrace_InPool(unsigned mode) {
    return mode == NEI_GRACE_ON || mode == NEI_GRACE_GATED;
}

static inline int NeiGrace_CanActivate(unsigned mode, unsigned required, uint32_t ootQuest, uint32_t mmQuest) {
    if (mode == NEI_GRACE_ON)
        return 1;
    if (mode != NEI_GRACE_GATED || required > NEI_GRACE_MAX_REWARDS)
        return 0;
    return NeiGrace_RewardCount(ootQuest, mmQuest) >= required;
}
