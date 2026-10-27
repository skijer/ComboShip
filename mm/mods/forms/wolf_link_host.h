#ifndef WOLF_LINK_HOST_H
#define WOLF_LINK_HOST_H

#include "z64.h"

#ifdef __cplusplus
extern "C" {
#endif

// Capture before custom-item listeners; dispatch after MM has resolved native damage.
void WolfLinkHost_PreUpdate(PlayState* play, Player* player);
void WolfLinkHost_RestorePlayerInput(Player* player, Input* input);
void WolfLinkHost_FilterInput(Player* player, Input* input);
void WolfLinkHost_BeforeAction(PlayState* play, Player* player, Input* input);
void WolfLinkHost_ApplyCollisionShape(Player* player);
// Sound follows the visible form even while native ledges/cutscenes own actions.
u8 WolfLinkHost_PlayFootstep(Player* player);
u8 WolfLinkHost_PlayClimbVoice(Player* player, u16 sfxId);
// 0: native player; 1: submitted Wolf; 2: selected Wolf skipped this frame.
u8 WolfLinkHost_Draw(PlayState* play, Player* player);
void WolfLinkHost_OnUseItem(PlayState* play, Player* player, s32 item);
void WolfLinkHost_Destroy(PlayState* play, Player* player);

#ifdef __cplusplus
}
#endif

#endif
