/*
 * custom_forms.cpp - form registry, gameplay properties, input blockers and per-frame dispatch
 * (Skijer's NEI). See custom_forms.h. New .cpp -> registered in build/x64/mm/2ship.vcxproj (no cmake).
 */
#include "forms_internal.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "mods/nei_save.h"
#include "mods/extended_inventory.h"
void func_8082E1F0(Player* player, u16 sfxId);
}

#define OOT_MASK_WHEEL_GERUDO 6

typedef struct FormDef {
    const char* name;
    const char* basePath;
    s32 playerMask; // -1 = no native MM mask (Gerudo -> menu / console)
    const char* cvarName;
    s32 cvarDefault;
    CustomFormProps props;
} FormDef;

// Values are soh's sFormProps / sMmAgeProps rows verbatim (mm_player_form.cpp:389-450, :3122-3178).
static const FormDef sForms[CUSTOM_FORM_MAX] = {
    { "kafei",
      "objects/forms/kafei/object_link_boy/",
      PLAYER_MASK_KAFEIS_MASK,
      "gForms.Kafei",
      1,
      { 40.0f, 60.0f, 14.0f, 50, 12.0f, 50.0f, 1.0f, 0 } },
    { "keaton",
      "objects/forms/keaton/object_link_boy/",
      PLAYER_MASK_KEATON,
      "gForms.Keaton",
      1,
      { 35.0f, 50.0f, 14.0f, 20, 12.0f, 60.0f, 1.0f, 1 } },
    { "gerudo",
      "objects/forms/gerudo/object_link_boy/",
      -1,
      "gForms.Gerudo",
      1,
      { 40.0f, 60.0f, 14.0f, 55, 12.0f, 50.0f, 1.0f, 1 } },
    { "garo",
      "objects/forms/garo/object_link_boy/",
      PLAYER_MASK_GARO,
      "gForms.Garo",
      1,
      { 60.0f, 70.0f, 18.0f, 70, 18.0f, 60.0f, 2.0f, 1 } },
};

#define KEATON_ROOT_SCALE_ADULT 0.335f
#define KEATON_ROOT_SCALE_CHILD 0.476f
#define KEATON_ROOT_DROP 500.0f
#define GERUDO_ROOT_SCALE_CHILD 0.71f
#define GERUDO_ROOT_DROP 200.0f

static Input sRawInput;
static s32 sLastForm = CUSTOM_FORM_NONE;
static u8 sWasDead = 0;

extern "C" s32 CustomForms_ActiveForm(void) {
    s32 forced = CVarGetInteger("gForms.ForceForm", 0) - 1;
    if (forced >= 0 && forced < CUSTOM_FORM_MAX) {
        return forced;
    }
    s32 worn = (s32)Nei_Save()->activeCustomForm - 1;
    return (worn >= 0 && worn < CUSTOM_FORM_MAX) ? worn : CUSTOM_FORM_NONE;
}

extern "C" const char* CustomForms_BasePath(void) {
    s32 form = CustomForms_ActiveForm();
    return (form != CUSTOM_FORM_NONE) ? sForms[form].basePath : NULL;
}

extern "C" const CustomFormProps* CustomForms_Props(void) {
    s32 form = CustomForms_ActiveForm();
    return (form != CUSTOM_FORM_NONE) ? &sForms[form].props : NULL;
}

extern "C" f32 CustomForms_RootScale(void) {
    s32 form = CustomForms_ActiveForm();
    u8 adult = Forms_IsAdultAge();
    if (form == CUSTOM_FORM_KEATON) {
        return adult ? KEATON_ROOT_SCALE_ADULT : KEATON_ROOT_SCALE_CHILD;
    }
    if (form == CUSTOM_FORM_GERUDO && !adult) {
        return GERUDO_ROOT_SCALE_CHILD;
    }
    return 1.0f;
}

extern "C" f32 CustomForms_RootDropBefore(void) {
    return (CustomForms_ActiveForm() == CUSTOM_FORM_GERUDO) ? GERUDO_ROOT_DROP : 0.0f;
}

extern "C" f32 CustomForms_RootDropAfter(void) {
    if (CustomForms_ActiveForm() != CUSTOM_FORM_KEATON) {
        return 0.0f;
    }
    return CVarGetFloat("gForms.KeatonRootDrop", KEATON_ROOT_DROP);
}

extern "C" void CustomForms_Toggle(s32 formId) {
    if (formId < 0 || formId >= CUSTOM_FORM_MAX) {
        return;
    }
    uint8_t* worn = &Nei_Save()->activeCustomForm;
    *worn = (*worn == (uint8_t)(formId + 1)) ? 0 : (uint8_t)(formId + 1);
    SPDLOG_INFO("[CustomForms] {} -> {}", sForms[formId].name, *worn ? "ON" : "OFF");
    AdultLink_OnFormChanged();
}

extern "C" void CustomForms_Deactivate(void) {
    if (Nei_Save()->activeCustomForm != 0) {
        Nei_Save()->activeCustomForm = 0;
        AdultLink_OnFormChanged();
    }
}

// ---- shared helpers -------------------------------------------------------------------------------
const Input* Forms_RawInput(void) {
    return &sRawInput;
}

u8 Forms_IsAdultAge(void) {
    return AdultLink_IsActive() != 0;
}

u8 Forms_OnGround(Player* player) {
    return (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) != 0;
}

u8 Forms_CanAct(Player* player) {
    if (player->transformation != PLAYER_FORM_HUMAN || player->csAction != PLAYER_CSACTION_NONE) {
        return 0;
    }
    if (player->stateFlags1 &
        (PLAYER_STATE1_DEAD | PLAYER_STATE1_TALKING | PLAYER_STATE1_CARRYING_ACTOR | PLAYER_STATE1_IN_CUTSCENE |
         FORMS_STATE1_LOADING | PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_IN_WATER | PLAYER_STATE1_CLIMBING_LADDER |
         FORMS_STATE1_CLIMBING_LEDGE | FORMS_STATE1_GETTING_ITEM | FORMS_STATE1_DAMAGED | PLAYER_STATE1_FIRST_PERSON |
         PLAYER_STATE1_10000000)) {
        return 0;
    }
    return 1;
}

f32 Forms_StickMagnitude(const Input* input) {
    f32 x = input->rel.stick_x;
    f32 y = input->rel.stick_y;
    return sqrtf(x * x + y * y);
}

s16 Forms_StickWorldYaw(PlayState* play, const Input* input) {
    return Camera_GetInputDirYaw(GET_ACTIVE_CAM(play)) + Math_Atan2S_XY(input->rel.stick_y, -input->rel.stick_x);
}

void Forms_Pause(Player* player) {
    player->stateFlags3 |= PLAYER_STATE3_PAUSE_ACTION_FUNC;
}

void Forms_Release(Player* player) {
    player->stateFlags3 &= ~PLAYER_STATE3_PAUSE_ACTION_FUNC;
}

// A clip on player->skelAnime under PAUSE: the pause must be raised the same frame the clip lands,
// or the vanilla action re-samples its own animation over it.
void Forms_PlayClip(PlayState* play, Player* player, PlayerAnimationHeader* anim, f32 speed, f32 start, f32 end,
                    u8 loop, f32 morph) {
    Forms_Pause(player);
    PlayerAnimation_Change(play, &player->skelAnime, anim, speed, start, end, loop ? ANIMMODE_LOOP : ANIMMODE_ONCE,
                           morph);
}

PlayerAnimationHeader* Forms_LoadAnim(const char* otrPath) {
    const char* p = otrPath;
    if (strncmp(p, "__OTR__", 7) == 0) {
        p += 7;
    }
    if (!ResourceMgr_FileExists(p)) {
        static u8 warned = 0;
        if (!warned) {
            warned = 1;
            SPDLOG_WARN("[CustomForms] anim missing: {} (is mods/nei_forms_anims.o2r present?)", otrPath);
        }
        return NULL;
    }
    return ResourceMgr_LoadPlayerAnimAsHeader(otrPath);
}

PlayerAnimationHeader* Forms_LoadAnimRange(const char* otrPath, s16 first, s16 last, s16 frames) {
    const char* p = otrPath;
    if (strncmp(p, "__OTR__", 7) == 0) {
        p += 7;
    }
    if (!ResourceMgr_FileExists(p)) {
        return NULL;
    }
    return ResourceMgr_LoadPlayerAnimAsHeaderInPlaceRange(otrPath, 1, first, last, frames);
}

// Empty-handed strikes: z_player_lib only stamps the melee quads off a held weapon, so a fist form
// stamps them itself at the hand limb (same tip/base rows as the sword, reach replacing the blade).
void Forms_StampFistQuads(PlayState* play, Player* player, s32 limbIndex, f32 reach, u32 dmgFlags, s32 damage) {
    static const Vec3f sBases[3] = { { 0.0f, 400.0f, 0.0f }, { 0.0f, 1400.0f, -1000.0f }, { 0.0f, -400.0f, 1000.0f } };
    s32 quadIndex = (limbIndex == PLAYER_LIMB_LEFT_HAND) ? 0 : 1;
    Vec3f tipLocal = { reach, sBases[quadIndex + 1].y, sBases[quadIndex + 1].z };
    Vec3f tip;
    Vec3f base;

    if (player->meleeWeaponState == PLAYER_MELEE_WEAPON_STATE_0) {
        player->meleeWeaponQuads[quadIndex].base.atFlags &= ~AT_ON;
        return;
    }
    Matrix_MultVec3f(&tipLocal, &tip);
    Matrix_MultVec3f((Vec3f*)&sBases[quadIndex + 1], &base);
    func_80833728(player, quadIndex, dmgFlags, damage);
    player->meleeWeaponQuads[quadIndex].base.atFlags |= AT_ON;
    func_80126440(play, &player->meleeWeaponQuads[quadIndex], &player->meleeWeaponInfo[quadIndex + 1], &tip, &base);
}

// ---- transitions ----------------------------------------------------------------------------------
static void CustomForms_ExitForm(Player* player, s32 form, PlayState* play) {
    switch (form) {
        case CUSTOM_FORM_KAFEI:
            KafeiForm_Reset();
            break;
        case CUSTOM_FORM_KEATON:
            KeatonForm_Reset(play);
            break;
        case CUSTOM_FORM_GERUDO:
            GerudoForm_Exit();
            break;
        case CUSTOM_FORM_GARO:
            GaroForm_Reset(player);
            break;
        default:
            break;
    }
    if (player != NULL) {
        Forms_Release(player);
        player->meleeWeaponState = PLAYER_MELEE_WEAPON_STATE_0;
    }
}

static void CustomForms_EnterForm(Player* player, s32 form, PlayState* play) {
    switch (form) {
        case CUSTOM_FORM_KAFEI:
            KafeiForm_Reset();
            break;
        case CUSTOM_FORM_KEATON:
            KeatonForm_Reset(play);
            break;
        case CUSTOM_FORM_GERUDO:
            GerudoForm_Enter();
            break;
        case CUSTOM_FORM_GARO:
            GaroForm_Reset(player);
            break;
        default:
            break;
    }
}

static void CustomForms_TrackTransition(Player* player, PlayState* play) {
    s32 form = CustomForms_ActiveForm();
    if (form == sLastForm) {
        return;
    }
    if (sLastForm != CUSTOM_FORM_NONE) {
        CustomForms_ExitForm(player, sLastForm, play);
    }
    if (form != CUSTOM_FORM_NONE) {
        CustomForms_EnterForm(player, form, play);
    }
    sLastForm = form;
}

// ---- z_player.c seams -----------------------------------------------------------------------------
// Runs BEFORE Player_UpdateCommon: keeps the raw pad for the movesets and strips the buttons a form
// owns so the vanilla handlers never see them (soh TransformMasks_FilterB).
extern "C" void CustomForms_FilterInput(Player* player, Input* input) {
    sRawInput = *input;
    CustomForms_TrackTransition(player, gPlayState);
    if (player->transformation != PLAYER_FORM_HUMAN) {
        return;
    }
    switch (CustomForms_ActiveForm()) {
        case CUSTOM_FORM_KEATON:
            KeatonForm_FilterInput(player, input);
            break;
        case CUSTOM_FORM_GERUDO:
            GerudoForm_FilterInput(player, input);
            break;
        case CUSTOM_FORM_GARO:
            GaroForm_FilterInput(player, input);
            break;
        default:
            break;
    }
}

// Runs AFTER Player_UpdateCommon (KiteSurf precedent): the pad and the stick are current, the
// per-frame clears already happened, so a pause raised here survives to the gate that reads it.
extern "C" void CustomForms_Update(Player* player, PlayState* play) {
    s32 form = CustomForms_ActiveForm();
    u8 dead = (player->stateFlags1 & PLAYER_STATE1_DEAD) != 0;

    if (form != CUSTOM_FORM_NONE && dead && !sWasDead) {
        CustomForms_OnDeath(player, play);
    }
    sWasDead = dead;
    if (form == CUSTOM_FORM_NONE || player->transformation != PLAYER_FORM_HUMAN) {
        return;
    }
    player->actor.colChkInfo.mass = sForms[form].props.mass;
    player->actor.shape.yOffset = 0.0f;

    switch (form) {
        case CUSTOM_FORM_KAFEI:
            KafeiForm_Update(player, play);
            break;
        case CUSTOM_FORM_KEATON:
            KeatonForm_Update(player, play);
            break;
        case CUSTOM_FORM_GERUDO:
            GerudoForm_Update(player, play);
            break;
        case CUSTOM_FORM_GARO:
            GaroForm_Update(player, play);
            break;
        default:
            break;
    }
}

extern "C" u8 CustomForms_OwnsPlayerAction(void) {
    switch (CustomForms_ActiveForm()) {
        case CUSTOM_FORM_KEATON:
            return KeatonForm_OwnsAction();
        case CUSTOM_FORM_GERUDO:
            return GerudoForm_OwnsAction();
        case CUSTOM_FORM_GARO:
            return GaroForm_OwnsAction();
        default:
            return 0;
    }
}

extern "C" u8 CustomForms_SuppressRoll(Player* player) {
    switch (CustomForms_ActiveForm()) {
        case CUSTOM_FORM_KAFEI:
            return KafeiForm_SuppressRoll(player);
        case CUSTOM_FORM_GERUDO:
            return GerudoForm_SuppressRoll(player);
        case CUSTOM_FORM_KEATON:
        case CUSTOM_FORM_GARO:
            return 1;
        default:
            return 0;
    }
}

extern "C" u8 CustomForms_ReplacesJumpslash(Player* player) {
    return (CustomForms_ActiveForm() == CUSTOM_FORM_KAFEI) && (player->transformation == PLAYER_FORM_HUMAN);
}

extern "C" f32 CustomForms_RunSpeedMul(void) {
    switch (CustomForms_ActiveForm()) {
        case CUSTOM_FORM_KAFEI:
            return KafeiForm_RunSpeedMul();
        case CUSTOM_FORM_GERUDO:
            return GerudoForm_RunSpeedMul();
        default:
            return 1.0f;
    }
}

extern "C" f32 CustomForms_RunAnimRateMul(void) {
    switch (CustomForms_ActiveForm()) {
        case CUSTOM_FORM_KAFEI:
            return KafeiForm_RunAnimRateMul();
        case CUSTOM_FORM_GERUDO:
            return GerudoForm_RunAnimRateMul();
        default:
            return 1.0f;
    }
}

extern "C" s32 CustomForms_NextComboMwa(Player* player, s32 requested) {
    if (CustomForms_ActiveForm() == CUSTOM_FORM_GERUDO) {
        return GerudoForm_NextComboMwa(player, requested);
    }
    return requested;
}

extern "C" u8 CustomForms_OwnsComboRow(Player* player) {
    return (CustomForms_ActiveForm() == CUSTOM_FORM_GERUDO) && GerudoForm_OwnsComboRow();
}

extern "C" u8 CustomForms_StartMeleeSwing(Player* player, PlayState* play) {
    if (CustomForms_ActiveForm() == CUSTOM_FORM_KAFEI) {
        return KafeiForm_StartMovingSlash(player);
    }
    return 0;
}

extern "C" void CustomForms_ScanMeleeHits(Player* player) {
    switch (CustomForms_ActiveForm()) {
        case CUSTOM_FORM_KEATON:
            KeatonForm_ScanBlock(player);
            break;
        case CUSTOM_FORM_GERUDO:
            GerudoForm_ScanBladeHits(player);
            break;
        default:
            break;
    }
}

extern "C" u8 CustomForms_HoldsChargeWindow(Player* player) {
    return (CustomForms_ActiveForm() == CUSTOM_FORM_GERUDO) && GerudoForm_HoldsChargeWindow(player);
}

extern "C" f32 CustomForms_ChargeRateMul(Player* player) {
    return (CustomForms_ActiveForm() == CUSTOM_FORM_GERUDO) ? 3.0f : 1.0f;
}

// The Gerudo Mask has no MM item id: it is a slot of NEI's OoT mask wheel, so its C-button press
// arrives as the wheel placeholder and the cursor says which mask is showing.
extern "C" u8 CustomForms_UseItem(Player* player, ItemId item) {
    if (item != ITEM_OOT_MASK_PLACEHOLDER || OotMask_CursorIndex() != OOT_MASK_WHEEL_GERUDO) {
        return 0;
    }
    if (!CVarGetInteger(sForms[CUSTOM_FORM_GERUDO].cvarName, sForms[CUSTOM_FORM_GERUDO].cvarDefault)) {
        return 0;
    }
    CustomForms_Toggle(CUSTOM_FORM_GERUDO);
    func_8082E1F0(player, NA_SE_PL_CHANGE_ARMS);
    return 1;
}

extern "C" void CustomForms_AdjustJumpSlash(Player* player) {
    if (CustomForms_ActiveForm() == CUSTOM_FORM_GERUDO && player->transformation == PLAYER_FORM_HUMAN) {
        GerudoForm_AdjustJumpSlash(player);
    }
}

extern "C" void CustomForms_ScaleIncomingDamage(Player* player) {
    const CustomFormProps* props = CustomForms_Props();
    if (props == NULL || props->incomingDamageMult == 1.0f || player->transformation != PLAYER_FORM_HUMAN) {
        return;
    }
    f32 scaled = player->actor.colChkInfo.damage * props->incomingDamageMult;
    player->actor.colChkInfo.damage = (u8)((scaled > 255.0f) ? 255.0f : scaled);
}

extern "C" void CustomForms_OnDeath(Player* player, PlayState* play) {
    if (CustomForms_ActiveForm() == CUSTOM_FORM_GARO) {
        GaroForm_OnDeath(player, play);
    }
}

extern "C" u8 CustomForms_BlocksShieldButton(void) {
    s32 form = CustomForms_ActiveForm();
    return (form == CUSTOM_FORM_KEATON || form == CUSTOM_FORM_GARO) &&
           (gPlayState != NULL && GET_PLAYER(gPlayState)->transformation == PLAYER_FORM_HUMAN);
}

extern "C" u8 CustomForms_WantsClimbableSurface(void) {
    return (CustomForms_ActiveForm() == CUSTOM_FORM_KEATON) && KeatonForm_ClimbActive();
}

// Voice: Keaton borrows the Deku bank (soh MmForm_PlayAttackVoice), Garo speaks Igos du Ikana's lines,
// Gerudo is silent (her ogg pack has no mixer here). Returns 1 when the vanilla voice must not play.
extern "C" u8 CustomForms_VoiceOverride(u16 sfxId, u16* outSfxId) {
    if (sfxId < NA_SE_VO_LI_SWORD_N || sfxId > NA_SE_VO_LI_SWORD_N + 0x1F) {
        return 0;
    }
    *outSfxId = 0;
    switch (CustomForms_ActiveForm()) {
        case CUSTOM_FORM_KEATON:
            *outSfxId = sfxId + sPlayerAgeProperties[PLAYER_FORM_DEKU].voiceSfxIdOffset;
            return 1;
        case CUSTOM_FORM_GARO:
            *outSfxId = GaroForm_VoiceFor(sfxId);
            return 1;
        case CUSTOM_FORM_GERUDO:
            return 1;
        default:
            return 0;
    }
}

// ---- renderer seams -------------------------------------------------------------------------------
extern "C" Gfx* CustomForms_HandDL(Player* player, s32 limbIndex, u8* claimed) {
    *claimed = 0;
    if (CustomForms_ActiveForm() != CUSTOM_FORM_GERUDO) {
        return NULL;
    }
    Gfx* dl = GerudoForm_HandDL(player, limbIndex);
    *claimed = (dl != NULL);
    return dl;
}

// Keaton and Gerudo carry none of Link's back equipment (the Gerudo's blades are in her hands).
extern "C" u8 CustomForms_HidesSheath(Player* player) {
    s32 form = CustomForms_ActiveForm();
    return form == CUSTOM_FORM_GERUDO || form == CUSTOM_FORM_KEATON;
}

extern "C" void CustomForms_PostLimb(PlayState* play, Player* player, s32 limbIndex) {
    switch (CustomForms_ActiveForm()) {
        case CUSTOM_FORM_KEATON:
            KeatonForm_PostLimb(play, player, limbIndex);
            break;
        case CUSTOM_FORM_GERUDO:
            GerudoForm_PostLimb(play, player, limbIndex);
            break;
        default:
            break;
    }
}

extern "C" void CustomForms_DrawWorld(PlayState* play, Player* player) {
    if (CustomForms_ActiveForm() == CUSTOM_FORM_GARO) {
        GaroForm_DrawWorld(play, player);
    }
}

extern "C" void CustomForms_GaroPose(Player* player, Vec3s* garoJointTable, s32 garoLimbCount) {
    GaroForm_Pose(player, garoJointTable, garoLimbCount);
}

extern "C" s32 CustomForms_GaroOverrideLimb(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot,
                                            Actor* actor) {
    return GaroForm_OverrideLimb(play, limbIndex, dList, pos, rot, actor);
}

extern "C" void CustomForms_GaroPostLimb(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, Actor* actor) {
    GaroForm_PostLimb(play, limbIndex, dList, rot, actor);
}

// ---- hooks ----------------------------------------------------------------------------------------
static s32 FormForMask(PlayerMask maskId) {
    for (s32 i = 0; i < CUSTOM_FORM_MAX; i++) {
        if (sForms[i].playerMask == (s32)maskId && CVarGetInteger(sForms[i].cvarName, sForms[i].cvarDefault)) {
            return i;
        }
    }
    return CUSTOM_FORM_NONE;
}

void RegisterCustomForms() {
    // Wearing a form mask cancels the vanilla equip and toggles the form instead (PersistentMasks pattern;
    // the vanilla toggle fires this same VB on re-wear, so removal comes for free).
    COND_VB_SHOULD(VB_USE_ITEM_EQUIP_MASK, true, {
        PlayerMask* maskId = va_arg(args, PlayerMask*);
        s32 form = FormForMask(*maskId);
        if (form != CUSTOM_FORM_NONE) {
            *should = false;
            CustomForms_Toggle(form);
            func_8082E1F0(GET_PLAYER(gPlayState), NA_SE_PL_CHANGE_ARMS);
        }
    });
    COND_VB_SHOULD(VB_SHIELD_FROM_BUTTON_HOLD, true, {
        if (CustomForms_BlocksShieldButton()) {
            *should = false;
        }
    });
    COND_VB_SHOULD(VB_BE_CLIMBABLE_SURFACE, true, {
        if (CustomForms_WantsClimbableSurface()) {
            *should = true;
        }
    });
    // Keaton climbs at twice vanilla's rate: the arg is the +/-1 direction sign the rate is scaled by.
    COND_VB_SHOULD(VB_SET_CLIMB_SPEED, true, {
        f32* direction = va_arg(args, f32*);
        if (CustomForms_WantsClimbableSurface()) {
            *direction *= 2.0f;
        }
    });
    COND_ID_HOOK(OnActorInit, ACTOR_EN_BOM_CHU, true, [](Actor* actor) { KafeiLandmine_OnSpawn(actor); });
    COND_HOOK(OnActorUpdate, true, [](Actor* actor) {
        if (CustomForms_ActiveForm() == CUSTOM_FORM_KEATON) {
            KeatonForm_OnActorUpdate(actor);
        }
    });
    COND_HOOK(OnPlayDestroy, true, []() {
        if (sLastForm != CUSTOM_FORM_NONE) {
            CustomForms_ExitForm(NULL, sLastForm, NULL);
            sLastForm = CUSTOM_FORM_NONE;
        }
    });
}

static RegisterShipInitFunc initFunc(RegisterCustomForms, {});
