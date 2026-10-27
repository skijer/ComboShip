#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "2s2h/Rando/NeiArticulatedPresentation.h"
#include "2s2h/Rando/NeiLanternPresentation.h"

extern "C" {
#include "global.h"
#include "din_fire_shield.h"
#include "din_fire_sword.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/logic/weapon_upgrades.h"

SaveContext gSaveContext;
u16 gEquipMasks[] = { 0xF, 0xF0, 0xF00, 0xF000 };
u8 gEquipShifts[] = { 0, 4, 8, 12 };

static s32 sConfiguredValue = -1;
static const char* sRequestedCVar = nullptr;
static s32 sRequestedDefault = -1;

s32 CVarGetInteger(const char* name, s32 defaultValue) {
    sRequestedCVar = name;
    sRequestedDefault = defaultValue;
    return sConfiguredValue < 0 ? defaultValue : sConfiguredValue;
}

void KiteSurf_AdjustLimb(s32 limbIndex, Vec3s* rot) {
    (void)limbIndex;
    (void)rot;
}

u8 ExtEquip_ShouldHideSwordDL(void) {
    return 0;
}

s32 BossRemains_IsOdolwaWorn(void) {
    return 0;
}

s32 BossRemains_IsGohtWorn(void) {
    return 0;
}

u8 ItemEquip_HoldsEmptyHand(void) {
    return 0;
}

u8 WeaponUpgrade_HasHammerAxe(void) {
    return 0;
}

u8 WeaponUpgrade_HasGreatFairy(void) {
    return 0;
}

// This visibility fixture supplies no authored sword replacement resources.
u8 WeaponUpgrade_ApplyHeldSwordDL(Gfx**, void*, Player*, u8, u8, u8) {
    return 0;
}

u8 Trident_GoldenArmor(void) {
    return 0;
}

s32 CustomForms_ActiveForm(void) {
    return CUSTOM_FORM_NONE;
}

const char* ExtEquip_GetShieldDLOverride(void) {
    return nullptr;
}

u8 ExtEquip_IsDekuSkinActive(void) {
    return 0;
}

u8 ExtEquip_IsOotMirrorSkinActive(void) {
    return 0;
}

u8 Nei_HeldItemUsesOotHookshotModel(Player*) {
    return 0;
}

Gfx* FourSword_HeldSwordDL(void) {
    return nullptr;
}

u8 FourSword_IsEquipped(void) {
    return 0;
}

// Keep optional Din hand replacements disabled for native/adult visibility checks.
void* DinFireSword_HandDL(PlayState*, Player*, void*) {
    return nullptr;
}
void DinFireSword_SetHandModel(Player*, void*, int) {
}
void DinFireSword_CopyHandModel(Player*, void*, void*) {
}
void DinFireSword_DrawHand(PlayState*, Player*, void*) {
}

void* DinFireShield_HandDL(PlayState*, Player*, void*) {
    return nullptr;
}

u8 ResourceMgr_FileExists(const char*) {
    return 0;
}

static bool sHookHandEnabled = false;
static bool sLanternGripEnabled = false;
static Gfx* sResolvedHookHand = nullptr;
static Gfx sHookReplacement;

bool NeiArticulated_ApplySwitchHookHand(PlayState*, Player*, Gfx** dList, Gfx* resolvedHand) {
    if (!sHookHandEnabled || resolvedHand == nullptr) {
        return false;
    }
    sResolvedHookHand = resolvedHand;
    *dList = &sHookReplacement;
    return true;
}

bool NeiLantern_UsesGrip(const Player*) {
    return sLanternGripEnabled;
}

f32 CustomForms_RootScale(void) {
    return 1.0f;
}

f32 CustomForms_RootDropBefore(void) {
    return 0.0f;
}

f32 CustomForms_RootDropAfter(void) {
    return 0.0f;
}

Gfx* CustomForms_HandDL(Player*, s32, u8* claimed) {
    *claimed = 0;
    return nullptr;
}

u8 CustomForms_HidesSheath(Player*) {
    return 0;
}

void Player_ApplyBackEquipmentVisibility(s32 limbIndex, Gfx** dList);
u8 Player_ShouldHideBackEquipment(s32 limbIndex);

extern Vec3f* sPlayerCurBodyPartPos;
extern s32 sPlayerLod;
extern s32 D_801F59E0;
extern Gfx* D_801C018C[];
extern Gfx* gPlayerSheathedSwords[];
extern Gfx* gPlayerSwordSheaths[];
}

static u8 sIsMod = 0;
static u8 sIsForm = 0;
static u8 sIsChildRig = 0;
static Gfx* sDL_LHClosed;
static Gfx* sDL_LHSword;
static Gfx* sDL_LHBgs;
static Gfx* sDL_RHClosed;
static Gfx* sDL_RHShield;
static Gfx* sDL_RHBow;
static Gfx* sDL_RHOcarina;
static Gfx* sDL_RHHookshot;
static Gfx* sDL_SheathEmpty;
static Gfx* sDL_SheathSword;
static Gfx* sDL_SheathShield;
static Gfx* sDL_SheathBoth;
static Gfx* sDL_RHMirrorShield;
static Gfx* sDL_SheathMirror;
static Gfx* sDL_SheathMirrorSword;

static int sBackShieldDraws = 0;
static u8 sLastOnBack = 0;

static void ExtEquip_DrawShieldCommon(void*, u8 onBack) {
    ++sBackShieldDraws;
    sLastOnBack = onBack;
}

#include "back_equipment_callbacks_production.inc"

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
            std::exit(1);                                                     \
        }                                                                     \
    } while (0)

static void ResetCVar(s32 value) {
    sConfiguredValue = value;
    sRequestedCVar = nullptr;
    sRequestedDefault = -1;
}

static Player MakePlayer(void) {
    static PlayerAnimationFrame frame = {};
    static Gfx* left[2];
    static Gfx* right[2];
    static Gfx* sheath[2];
    static Gfx nativeLeft;
    static Gfx nativeRight;
    static Gfx nativeSheath;

    left[0] = left[1] = &nativeLeft;
    right[0] = right[1] = &nativeRight;
    sheath[0] = sheath[1] = &nativeSheath;

    Player player = {};
    player.transformation = PLAYER_FORM_HUMAN;
    player.leftHandType = PLAYER_MODELTYPE_LH_OPEN;
    player.rightHandType = PLAYER_MODELTYPE_RH_OPEN;
    player.heldItemId = ITEM_NONE;
    player.heldItemAction = PLAYER_IA_NONE;
    player.skelAnime.jointTable = frame.frameTable;
    player.leftHandDLists = left;
    player.rightHandDLists = right;
    player.sheathDLists = sheath;
    return player;
}

static Gfx* DrawNative(Player* player, s32 limbIndex, Gfx* incoming) {
    PlayState play = {};
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player->actor;
    Vec3f pos = {};
    Vec3s rot = {};
    Gfx* dList = incoming;

    sPlayerCurBodyPartPos = &player->bodyPartsPos[0];
    sPlayerLod = 0;
    D_801F59E0 = 0;
    Player_OverrideLimbDrawGameplayDefault(&play, limbIndex, &dList, &pos, &rot, &player->actor);
    return dList;
}

static Gfx* DrawAdult(Player* player, s32 limbIndex, Gfx* incoming) {
    PlayState play = {};
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player->actor;
    Vec3f pos = {};
    Vec3s rot = {};
    Gfx* dList = incoming;

    sPlayerCurBodyPartPos = &player->bodyPartsPos[0];
    sPlayerLod = 0;
    D_801F59E0 = 0;
    AdultLink_OverrideLimb(&play, limbIndex, &dList, &pos, &rot, &player->actor);
    return dList;
}

static void NativeSheathAndHandsFollowPolicy(void) {
    Player player = MakePlayer();
    Gfx incoming;

    gSaveContext.save.saveInfo.equips.equipment = EQUIP_VALUE_SWORD_KOKIRI;
    player.sheathType = PLAYER_MODELTYPE_SHEATH_14;

    ResetCVar(-1);
    CHECK(DrawNative(&player, PLAYER_LIMB_SHEATH, &incoming) == gPlayerSheathedSwords[0]);
    CHECK(std::strcmp(sRequestedCVar, "gEnhancements.HideBackEquipment") == 0);
    CHECK(sRequestedDefault == 0);

    ResetCVar(1);
    CHECK(DrawNative(&player, PLAYER_LIMB_SHEATH, &incoming) == nullptr);

    player.sheathType = PLAYER_MODELTYPE_SHEATH_15;
    ResetCVar(0);
    CHECK(DrawNative(&player, PLAYER_LIMB_SHEATH, &incoming) == gPlayerSwordSheaths[0]);
    ResetCVar(1);
    CHECK(DrawNative(&player, PLAYER_LIMB_SHEATH, &incoming) == nullptr);

    player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
    player.heldItemId = ITEM_SWORD_KOKIRI;
    CHECK(DrawNative(&player, PLAYER_LIMB_LEFT_HAND, &incoming) == D_801C018C[0]);

    player.rightHandType = PLAYER_MODELTYPE_RH_SHIELD;
    player.currentShield = PLAYER_SHIELD_HEROS_SHIELD;
    CHECK(DrawNative(&player, PLAYER_LIMB_RIGHT_HAND, &incoming) == player.rightHandDLists[0]);
}

static void AdultSheathAndHandsFollowPolicy(void) {
    static Gfx adultSword;
    static Gfx adultEmpty;
    static Gfx adultLeft;
    static Gfx adultRight;
    static Gfx adultClosedLeft;
    static Gfx adultClosedRight;
    static Gfx skeleton;
    Player player = MakePlayer();

    sDL_SheathSword = &adultSword;
    sDL_SheathEmpty = &adultEmpty;
    sDL_LHSword = &adultLeft;
    sDL_RHShield = &adultRight;
    sDL_LHClosed = &adultClosedLeft;
    sDL_RHClosed = &adultClosedRight;
    gSaveContext.save.saveInfo.equips.equipment = EQUIP_VALUE_SWORD_KOKIRI;

    player.sheathType = PLAYER_MODELTYPE_SHEATH_14;
    ResetCVar(0);
    CHECK(DrawAdult(&player, PLAYER_LIMB_SHEATH, &skeleton) == &adultSword);
    ResetCVar(1);
    CHECK(DrawAdult(&player, PLAYER_LIMB_SHEATH, &skeleton) == nullptr);

    player.sheathType = PLAYER_MODELTYPE_SHEATH_15;
    ResetCVar(0);
    CHECK(DrawAdult(&player, PLAYER_LIMB_SHEATH, &skeleton) == &adultEmpty);
    ResetCVar(1);
    CHECK(DrawAdult(&player, PLAYER_LIMB_SHEATH, &skeleton) == nullptr);

    player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
    player.heldItemId = ITEM_SWORD_KOKIRI;
    CHECK(DrawAdult(&player, PLAYER_LIMB_LEFT_HAND, &skeleton) == &adultLeft);

    player.rightHandType = PLAYER_MODELTYPE_RH_SHIELD;
    player.currentShield = PLAYER_SHIELD_HEROS_SHIELD;
    CHECK(DrawAdult(&player, PLAYER_LIMB_RIGHT_HAND, &skeleton) == &adultRight);

    // The production callback applies its final NEI hand selector after resolving adult equipment.
    sHookHandEnabled = true;
    sResolvedHookHand = nullptr;
    CHECK(DrawAdult(&player, PLAYER_LIMB_RIGHT_HAND, &skeleton) == &sHookReplacement);
    CHECK(sResolvedHookHand == &adultClosedRight);
    sHookHandEnabled = false;

    player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
    sLanternGripEnabled = true;
    CHECK(DrawAdult(&player, PLAYER_LIMB_LEFT_HAND, &skeleton) == &adultClosedLeft);
    sLanternGripEnabled = false;
}

static void ExtendedBackShieldFollowsPolicy(void) {
    ResetCVar(0);
    sBackShieldDraws = 0;
    sLastOnBack = 0;
    ExtEquip_DrawShieldBackDL(nullptr);
    CHECK(sBackShieldDraws == 1);
    CHECK(sLastOnBack == 1);

    ResetCVar(1);
    ExtEquip_DrawShieldBackDL(nullptr);
    CHECK(sBackShieldDraws == 1);
}

int main(void) {
    NativeSheathAndHandsFollowPolicy();
    AdultSheathAndHandsFollowPolicy();
    ExtendedBackShieldFollowsPolicy();
    return 0;
}
