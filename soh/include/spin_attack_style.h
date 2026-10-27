#ifndef SPIN_ATTACK_STYLE_H
#define SPIN_ATTACK_STYLE_H
#include <stdint.h>
struct PlayState;
struct ColliderInfo;
#ifdef __cplusplus
extern "C" {
#endif
uint32_t SpinAttack_OriginalDamageFlags(struct PlayState* play, const struct ColliderInfo* info);
#ifdef __cplusplus
}
#endif
#endif
