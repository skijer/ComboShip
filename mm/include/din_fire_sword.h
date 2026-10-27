#ifndef DIN_FIRE_SWORD_H
#define DIN_FIRE_SWORD_H

#include <stdint.h>
#include "../../combo/render/sword_element_aura_profiles.h"

struct PlayState;
struct Player;
struct Actor;
struct ColliderElement;

#ifdef __cplusplus
extern "C" {
#endif

uint32_t DinFireSword_DamageFlags(struct PlayState* play, struct Player* player, uint32_t original);
uint32_t DinFireSword_SetDamageFlags(struct PlayState* play, struct Player* player, int quad, uint32_t original);
void DinFireSword_RefreshDamage(struct PlayState* play, struct Player* player);
// Recover only this feature's exact player melee touchers. Other combined
// attacks keep their normal collision, damage-table and drop behavior.
uint32_t DinFireSword_OriginalDamageFlags(struct PlayState* play, const struct ColliderElement* hitInfo);
uint8_t DinFireSword_DamageEntry(struct PlayState* play, struct Actor* target, const struct ColliderElement* hitInfo,
                                 uint32_t receiverFlags, uint8_t vanillaEntry);
// Accepted-hit/fatal hooks are presentation observers; they never delay actor
// death or change damage, soul gates, drops or progression. Collider bounds are
// copied while live and discarded at the end of that actor's update.
void DinFireSword_FinishAcceptedHit(struct PlayState* play, const void* receiver,
                                    const struct ColliderElement* hitInfo, uint32_t receiverFlags, uint32_t damage);
// Narrow native adapters call these only at their own confirmed lethal entry.
// This also supports flags-only deaths and individual bat receivers.
void DinFireSword_FinishConfirmedHit(struct PlayState* play, const void* receiver,
                                     const struct ColliderElement* hitInfo, uint32_t receiverFlags);
void DinFireSword_FinishConfirmedPart(struct PlayState* play, struct Actor* actor,
                                      const struct ColliderElement* hitInfo, unsigned part,
                                      float x, float y, float z, float radius);
void DinFireSword_FinishBeginActor(struct PlayState* play, struct Actor* actor);
void DinFireSword_FinishEndActor(struct PlayState* play, struct Actor* actor);
void DinFireSword_FinishFatal(struct PlayState* play, struct Actor* actor);
void DinFireSword_FinishForget(struct PlayState* play, struct Actor* actor);
void DinFireSword_FinishTeardown(struct PlayState* play);
void DinFireSword_FinishDrawAll(struct PlayState* play);
// Native delayed ice spawns inherit only an accepted custom sword-ice hit.
int DinFireSword_UseIceMaterial(struct PlayState* play, const struct Actor* actor);
void DinFireSword_Reset(void);
void DinFireSword_Update(struct PlayState* play, struct Player* player);
void DinFireSword_BeginPlayerDraw(struct PlayState* play, struct Player* player);
// Gameplay left-hand matrix must be current. Ordinary sword hand only;
// other weapon owners and ceremonies keep their own draw path.
void DinFireSword_Draw(struct PlayState* play, struct Player* player);
// Final limb compositors register the actual blade and transfer that identity
// when copying a deferred hand list. NULL owner is the current local draw only.
void DinFireSword_SetHandModel(struct Player* player, void* handDL, int model);
void DinFireSword_CopyHandModel(struct Player* player, void* sourceDL, void* destinationDL);
void DinFireSword_DrawHand(struct PlayState* play, struct Player* player, void* finalHandDL);
// Submit the captured hand effect after body meshes and accessories, before XLU.
void DinFireSword_DrawAfterPlayer(struct PlayState* play, struct Player* player);
// Compose the private Din blade with the renderer's own empty hand.
void* DinFireSword_HandDL(struct PlayState* play, struct Player* player, void* closedHand);
int DinFireSword_IsFireHit(struct PlayState* play, const struct ColliderElement* hitInfo);

#ifdef __cplusplus
}
#endif
#endif
