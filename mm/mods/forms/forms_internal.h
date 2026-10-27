/*
 * forms_internal.h - shared plumbing for the form movesets (Skijer's NEI). C++ only.
 * MM names differ from soh's: player->speedXZ is the INTENT (OoT linearVelocity) and
 * player->actor.speed the momentum (OoT actor.speedXZ); PAUSE_ACTION_FUNC is PLAYER_STATE3_4.
 */
#ifndef MODS_FORMS_FORMS_INTERNAL_H
#define MODS_FORMS_FORMS_INTERNAL_H

#include <libultraship/bridge/consolevariablebridge.h>
#include <spdlog/spdlog.h>
#include <math.h>
#include <string.h>

extern "C" {
#include "macros.h"
#include "functions.h"
#include "variables.h"
#include "mods/nei_oot_compat.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/helpers/equip_helper.h"

#ifndef PLAYER_STATE3_PAUSE_ACTION_FUNC
#define PLAYER_STATE3_PAUSE_ACTION_FUNC PLAYER_STATE3_4
#endif
#define FORMS_STATE1_DAMAGED PLAYER_STATE1_4000000
#define FORMS_STATE1_CLIMBING_LEDGE PLAYER_STATE1_4
#define FORMS_STATE1_HANGING PLAYER_STATE1_200000
#define FORMS_STATE1_GETTING_ITEM PLAYER_STATE1_400
#define FORMS_STATE1_LOADING PLAYER_STATE1_200
#define FORMS_STATE1_DEATH_CS (PLAYER_STATE1_DEAD | PLAYER_STATE1_10000000 | PLAYER_STATE1_20000000)

// z_player.c internals (non-static in the decomp, undeclared in headers).
void Player_SetAction(PlayState* play, Player* player, PlayerActionFunc actionFunc, s32 arg3);
void Player_Action_84(Player* player, PlayState* play);
void Player_Action_25(Player* player, PlayState* play);
void Player_Action_26(Player* player, PlayState* play);
void Player_Action_Idle(Player* player, PlayState* play);
void func_80833864(PlayState* play, Player* player, PlayerMeleeWeaponAnimation meleeWeaponAnim);
void func_80833728(Player* player, s32 index, u32 dmgFlags, s32 damage);
void func_80833B18(PlayState* play, Player* player, s32 hitResponse, f32 speed, f32 velocityY, s16 yaw,
                   s32 invincibilityTimer);
void func_80834DB8(Player* player, PlayerAnimationHeader* anim, f32 speed, PlayState* play);
void func_80836B3C(PlayState* play, Player* player, f32 arg2);
void func_80839E74(Player* player, PlayState* play);
void func_8082DC38(Player* player);
void func_8082FA5C(PlayState* play, Player* player, PlayerMeleeWeaponState meleeWeaponState);
void Player_Anim_PlayOnceAdjusted(PlayState* play, Player* player, PlayerAnimationHeader* anim);
bool Player_IsZTargeting(Player* player);
void Player_AnimSfx_PlayVoice(Player* player, u16 sfxId);
void Player_RequestRumble(PlayState* play, Player* player, s32 sourceIntensity, s32 decayTimer, s32 decayStep,
                          s32 distSq);
s32 func_80126440(PlayState* play, ColliderQuad* collider, WeaponInfo* weaponInfo, Vec3f* newTip, Vec3f* newBase);
extern PlayerAgeProperties sPlayerAgeProperties[PLAYER_FORM_MAX];

// Extended-player animation tables (z_player.c, exposed for the ext equipment).
PlayerAnimationHeader* ExtPlayer_GetAnimGroupAnim(s32 group, s32 animType);
void ExtPlayer_SetAnimGroupAnim(s32 group, s32 animType, PlayerAnimationHeader* anim);
void ExtPlayer_GetMeleeAnim(s32 mwa, PlayerAnimationHeader** swing, PlayerAnimationHeader** end,
                            PlayerAnimationHeader** endLockOn, u8* hitStart, u8* hitEnd);
void ExtPlayer_SetMeleeAnim(s32 mwa, PlayerAnimationHeader* swing, PlayerAnimationHeader* end,
                            PlayerAnimationHeader* endLockOn, u8 hitStart, u8 hitEnd);

// BenPort animation loaders (misc/link_animetion clips are raw SOH_PlayerAnimation payloads).
PlayerAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeader(const char* animPath);
PlayerAnimationHeader* ResourceMgr_LoadPlayerAnimAsHeaderInPlaceRange(const char* animPath, uint8_t stripY,
                                                                      int16_t firstFrame, int16_t lastFrame,
                                                                      int16_t targetFrames);
uint8_t ResourceMgr_FileExists(const char* path);
void ResourceMgr_PatchGfxByName(const char* path, const char* patchName, int index, Gfx instruction);
void ResourceMgr_UnpatchGfxByName(const char* path, const char* patchName);
// mm_asset_loader.cpp: CRC64 -> archive path through the global hash index (mm.o2r is indexed).
const char* MmAssets_HashToPath(unsigned long long hash);
Gfx* ResourceMgr_LoadGfxByName(const char* path);
SkeletonHeader* ResourceMgr_LoadSkeletonByName(const char* path, SkelAnime* skelAnime);
void* OotAssets_LoadGfxDirect(const char* otrPath);

s32 AdultLink_IsActive(void);
void AdultLink_OnFormChanged(void);

// OPEN_DISPS re-declares these with C++ linkage inside a .cpp; the first declaration must be C.
void FrameInterpolation_RecordOpenChild(const void* a, int b);
void FrameInterpolation_RecordCloseChild(void);
}

// ---- shared helpers (custom_forms.cpp) ----
const Input* Forms_RawInput(void);
u8 Forms_IsAdultAge(void);
u8 Forms_OnGround(Player* player);
u8 Forms_CanAct(Player* player);
f32 Forms_StickMagnitude(const Input* input);
s16 Forms_StickWorldYaw(PlayState* play, const Input* input);
void Forms_Pause(Player* player);
void Forms_Release(Player* player);
void Forms_PlayClip(PlayState* play, Player* player, PlayerAnimationHeader* anim, f32 speed, f32 start, f32 end,
                    u8 loop, f32 morph);
PlayerAnimationHeader* Forms_LoadAnim(const char* otrPath);
// Vanilla gPlayerAnim_* symbols are OTR path strings the engine resolves at play time; the loader
// only handles SOH_PlayerAnimation payloads, so these are passed through as-is.
inline PlayerAnimationHeader* Forms_VanillaAnim(const char* symbol) {
    return (PlayerAnimationHeader*)symbol;
}
PlayerAnimationHeader* Forms_LoadAnimRange(const char* otrPath, s16 first, s16 last, s16 frames);
void Forms_StampFistQuads(PlayState* play, Player* player, s32 limbIndex, f32 reach, u32 dmgFlags, s32 damage);

// ---- per-form modules ----
void KafeiForm_Reset(void);
void KafeiForm_Update(Player* player, PlayState* play);
u8 KafeiForm_SuppressRoll(Player* player);
u8 KafeiForm_StartMovingSlash(Player* player);
f32 KafeiForm_RunSpeedMul(void);
f32 KafeiForm_RunAnimRateMul(void);

void KeatonForm_Reset(PlayState* play);
void KeatonForm_FilterInput(Player* player, Input* input);
u8 KeatonForm_Update(Player* player, PlayState* play);
void KeatonForm_ScanBlock(Player* player);
u8 KeatonForm_OwnsAction(void);
u8 KeatonForm_ClimbActive(void);
void KeatonForm_PostLimb(PlayState* play, Player* player, s32 limbIndex);
void KeatonForm_OnActorUpdate(Actor* actor);

void GerudoForm_Enter(void);
void GerudoForm_Exit(void);
void GerudoForm_FilterInput(Player* player, Input* input);
u8 GerudoForm_Update(Player* player, PlayState* play);
u8 GerudoForm_OwnsAction(void);
u8 GerudoForm_SuppressRoll(Player* player);
s32 GerudoForm_NextComboMwa(Player* player, s32 requested);
u8 GerudoForm_OwnsComboRow(void);
void GerudoForm_ScanBladeHits(Player* player);
u8 GerudoForm_HoldsChargeWindow(Player* player);
f32 GerudoForm_RunSpeedMul(void);
f32 GerudoForm_RunAnimRateMul(void);
Gfx* GerudoForm_HandDL(Player* player, s32 limbIndex);
u8 GerudoForm_SwordsOut(Player* player);
void GerudoForm_AdjustJumpSlash(Player* player);
void GerudoForm_PostLimb(PlayState* play, Player* player, s32 limbIndex);

void GaroForm_Reset(Player* player);
void GaroForm_FilterInput(Player* player, Input* input);
void GaroForm_Update(Player* player, PlayState* play);
u8 GaroForm_OwnsAction(void);
u8 GaroForm_VanillaWantsAButton(Player* player);
void GaroForm_DrawWorld(PlayState* play, Player* player);
void GaroForm_Pose(Player* player, Vec3s* garoJointTable, s32 garoLimbCount);
s32 GaroForm_OverrideLimb(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* actor);
void GaroForm_PostLimb(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, Actor* actor);
void GaroForm_OnDeath(Player* player, PlayState* play);
u16 GaroForm_VoiceFor(u16 linkVoiceSfx);

#endif
