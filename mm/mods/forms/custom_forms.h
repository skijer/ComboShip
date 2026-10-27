/*
 * custom_forms.h - OoT custom player forms in MM (Skijer's NEI): Kafei, Keaton, Gerudo, Garo.
 *
 * A form = model (adult-rigged mirror of object_link_boy under objects/forms/<name>/ in
 * mods/nei_forms.o2r) + placement (root scale/drop, collider, age properties) + moveset. This module
 * owns WHICH form is active, its gameplay properties, the input blockers and the per-frame dispatch;
 * each moveset lives in form_<name>.cpp and the visuals in adult_link_render.cpp.
 */
#ifndef MODS_FORMS_CUSTOM_FORMS_H
#define MODS_FORMS_CUSTOM_FORMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "z64.h"

typedef enum CustomFormId {
    CUSTOM_FORM_NONE = -1,
    CUSTOM_FORM_KAFEI,
    CUSTOM_FORM_KEATON,
    CUSTOM_FORM_GERUDO,
    CUSTOM_FORM_GARO,
    CUSTOM_FORM_MAX
} CustomFormId;

// Gameplay properties applied while a form is active (soh sFormProps / sMmAgeProps).
typedef struct CustomFormProps {
    f32 ceilingCheckHeight;
    f32 shadowScale;
    f32 wallCheckRadius;
    u8 mass;
    f32 cylinderRadius;
    f32 cylinderHeight;
    f32 incomingDamageMult;
    u8 overridesAgeProps; // 0 = keep OoT/adult numbers (Kafei keeps his running ledge vault)
} CustomFormProps;

s32 CustomForms_ActiveForm(void);
const char* CustomForms_BasePath(void);
void CustomForms_Toggle(s32 formId);
void CustomForms_Deactivate(void);
const CustomFormProps* CustomForms_Props(void);

// Placement of the animation root (soh MmForm_OverrideLimbDraw root branch): drop BEFORE the scale
// (Gerudo 200), multiply (Keaton 0.335 adult / 0.476 child, Gerudo child 0.71), drop AFTER (Keaton 500).
f32 CustomForms_RootScale(void);
f32 CustomForms_RootDropBefore(void);
f32 CustomForms_RootDropAfter(void);

// z_player.c seams (see custom_forms.cpp for the order they run in).
void CustomForms_FilterInput(Player* player, Input* input);
void CustomForms_Update(Player* player, PlayState* play);
u8 CustomForms_OwnsPlayerAction(void);
u8 CustomForms_SuppressRoll(Player* player);
u8 CustomForms_ReplacesJumpslash(Player* player);
f32 CustomForms_RunSpeedMul(void);
f32 CustomForms_RunAnimRateMul(void);
s32 CustomForms_NextComboMwa(Player* player, s32 requested);
u8 CustomForms_OwnsComboRow(Player* player);
u8 CustomForms_StartMeleeSwing(Player* player, PlayState* play);
void CustomForms_ScanMeleeHits(Player* player);
u8 CustomForms_HoldsChargeWindow(Player* player);
f32 CustomForms_ChargeRateMul(Player* player);
void CustomForms_AdjustJumpSlash(Player* player);
u8 CustomForms_UseItem(Player* player, ItemId item);
void CustomForms_ScaleIncomingDamage(Player* player);
void CustomForms_OnDeath(Player* player, PlayState* play);
u8 CustomForms_BlocksShieldButton(void);
u8 CustomForms_WantsClimbableSurface(void);
u8 CustomForms_VoiceOverride(u16 sfxId, u16* outSfxId);

// Renderer seams (adult_link_render.cpp).
Gfx* CustomForms_HandDL(Player* player, s32 limbIndex, u8* claimed);
u8 CustomForms_HidesSheath(Player* player);
void CustomForms_PostLimb(PlayState* play, Player* player, s32 limbIndex);
void CustomForms_DrawWorld(PlayState* play, Player* player);
void CustomForms_GaroPose(Player* player, Vec3s* garoJointTable, s32 garoLimbCount);
s32 CustomForms_GaroOverrideLimb(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* actor);
void CustomForms_GaroPostLimb(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, Actor* actor);

// Kafei's SW97 landmine rides the vanilla Bombchu actor (z_en_bom_chu.c seams).
u8 KafeiLandmine_Active(void);
void KafeiLandmine_OnSpawn(Actor* chu);
u8 KafeiLandmine_Settle(Actor* chu, PlayState* play);
u8 KafeiLandmine_ShouldDetonate(Actor* chu);
void KafeiLandmine_Draw(PlayState* play, Actor* chu, f32 colorIntensity);

// HUD readouts (forms_hud.cpp).
u8 KafeiForm_MeterVisible(void);
s32 KafeiForm_WheelCount(void);
f32 KafeiForm_WheelFill(s32 wheel);
u8 KafeiForm_IsWinded(void);
u8 GerudoForm_RageVisible(void);
f32 GerudoForm_RageFill(void);
u8 GerudoForm_RageReady(void);

#ifdef __cplusplus
}
#endif

#endif
