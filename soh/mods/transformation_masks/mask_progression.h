#pragma once
#include <stdint.h>

struct PlayState;
#ifdef __cplusplus
extern "C" {
#endif

// Optional OoT-only progression. Original MM gameplay retains its native rules.
uint8_t MaskProgression_IsCompatible(void);
uint8_t MaskProgression_CanTransform(int32_t item);
void MaskProgression_Update(struct PlayState* play);

#ifdef __cplusplus
}
#endif
