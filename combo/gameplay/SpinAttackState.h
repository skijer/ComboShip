#ifndef COMBO_SPIN_ATTACK_STATE_H
#define COMBO_SPIN_ATTACK_STATE_H
#include <stdint.h>
#include <string.h>
#include "../render/SpinAttackStyle.h"

typedef struct {
    CwSpinStyle style;
    uint8_t effectsEnabled, eligible, element, released;
    uintptr_t owner, context;
    int32_t scene;
    uint32_t originalFlags, appliedFlags;
} CwSpinAttackState;

static inline void CwSpinAttack_Reset(CwSpinAttackState* state) {
    memset(state, 0, sizeof(*state));
}

static inline void CwSpinAttack_Init(CwSpinAttackState* state, int style, int effects, int eligible, uintptr_t owner,
                                     uintptr_t context, int32_t scene) {
    CwSpinAttack_Reset(state);
    state->style = CwSpinStyle_Resolve(style, 0, 0, 1).style;
    state->effectsEnabled = effects != 0;
    state->eligible = eligible != 0 && owner != 0 && context != 0;
    state->owner = owner;
    state->context = context;
    state->scene = scene;
}

static inline void CwSpinAttack_Preview(CwSpinAttackState* state, uint8_t element, int eligible, int paused) {
    if (!state->released && !paused)
        state->element = state->eligible && eligible && element <= 3 ? element : 0;
}

// Called only after the host's native powered release and flags are established.
// A visual preference never participates in this combat decision.
static inline uint32_t CwSpinAttack_Release(CwSpinAttackState* state, uint8_t element, int eligible, uint32_t original,
                                            uint32_t fire, uint32_t ice, uint32_t light) {
    if (state->released)
        return state->appliedFlags;
    state->released = 1;
    state->eligible = state->eligible && eligible && original != 0;
    state->element = state->eligible && element <= 3 ? element : 0;
    state->originalFlags = original;
    uint32_t added = state->element == 1 ? fire : state->element == 2 ? ice : state->element == 3 ? light : 0;
    state->appliedFlags = original | added;
    return state->appliedFlags;
}

static inline uint32_t CwSpinAttack_Original(const CwSpinAttackState* state, uint32_t current) {
    return state->eligible && state->released && current == state->appliedFlags ? state->originalFlags : current;
}
#endif
