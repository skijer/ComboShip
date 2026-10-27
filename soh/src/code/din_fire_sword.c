#include "global.h"
#include "spin_attack_style.h"
#include "overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.h"
#include "../../../combo/gameplay/SpinAttackOwnership.h"
#include "align_asset_macro.h"
#include "din_fire_sword.h"
#include "sword_element.h"
#include "overlays/actors/ovl_En_Zf/z_en_zf.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "mods/transformation_masks/transformation_masks.h"
#include <libultraship/bridge/resourcebridge.h>
#include <libultraship/log/luslog.h>
#include <math.h>
#include "../../../combo/render/sword_element_aura.h"
#include "../../../combo/render/sword_element_aura_identity.h"
#include "mods/extended_equipment.h"
#include "mods/items/logic/weapon_upgrades.h"
extern s32 BossRemains_IsOdolwaWorn(void);
extern s32 BossRemains_IsGohtWorn(void);
extern u8 FourSword_IsEquipped(void);

#define SWORD_ROOT "objects/din_fire_sword/poc1/"
static const ALIGN_ASSET(2) char sCoreTexture[] = "__OTR__" SWORD_ROOT "CoreTex";
static const ALIGN_ASSET(2) char sFlameTexture[] = "__OTR__" SWORD_ROOT "FlameTex";
static const char* sCoreDL[] = { SWORD_ROOT "adult/CoreDL", SWORD_ROOT "child/CoreDL", SWORD_ROOT "bgs/CoreDL",
                                 SWORD_ROOT "broken/CoreDL" };
static const char* sFlameDL[] = { SWORD_ROOT "adult/FlameDL", SWORD_ROOT "child/FlameDL", SWORD_ROOT "bgs/FlameDL",
                                  SWORD_ROOT "broken/FlameDL" };
static const char* sCoreVertices[] = { SWORD_ROOT "adult/CoreVertices", SWORD_ROOT "child/CoreVertices",
                                       SWORD_ROOT "bgs/CoreVertices", SWORD_ROOT "broken/CoreVertices" };
static const char* sFlameVertices[] = { SWORD_ROOT "adult/FlameVertices", SWORD_ROOT "child/FlameVertices",
                                        SWORD_ROOT "bgs/FlameVertices", SWORD_ROOT "broken/FlameVertices" };
static const char* sEquipment[] = {
    "objects/object_link_boy/DinSleekEquipmentPOC1_OOT_Adult/SwordDL",
    "objects/object_link_child/DinSleekEquipmentPOC1_OOT_Child/SwordDL",
};
static const char* sProgressiveEquipment[] = {
    "objects/din_fire_sword/progressive/adult/SwordDL",
    "objects/din_fire_sword/progressive/child/SwordDL",
    "objects/din_fire_sword/progressive/bgs/SwordDL",
};

static s32 DinFireSword_IsUpgrade(Player* player) {
    return (player->heldItemAction == PLAYER_IA_SWORD_KOKIRI && WeaponUpgrade_KokiriLevel()) ||
           (player->heldItemAction == PLAYER_IA_SWORD_BIGGORON && WeaponUpgrade_HasGreatFairy());
}

static struct {
    PlayState* play;
    Player* player;
    u32 lastFrame;
    u16 phase;
    s16 scene;
    s32 age;
} sSword;

static struct {
    PlayState* play;
    Player* player;
    Mtx* handMatrix;
    u32 frame;
    s32 model, rig, logicalModel;
    void* fittedHand;
    s32 fittedModel;
    u8 element;
} sDraw;

void DinFireSword_BeginPlayerDraw(PlayState* play, Player* player) {
    sDraw.play = play != NULL && player == GET_PLAYER(play) ? play : NULL;
    sDraw.player = sDraw.play != NULL ? player : NULL;
    sDraw.handMatrix = NULL;
    sDraw.fittedHand = NULL;
    sDraw.fittedModel = -1;
}

void DinFireSword_SetHandModel(Player* player, void* handDL, int model) {
    if (sDraw.play == NULL || sDraw.player == NULL || (player != NULL && player != sDraw.player) || handDL == NULL ||
        model < 0 || model >= SWORD_AURA_MODEL_COUNT)
        return;
    sDraw.fittedHand = handDL;
    sDraw.fittedModel = model;
}

void DinFireSword_CopyHandModel(Player* player, void* sourceDL, void* destinationDL) {
    if (sDraw.play != NULL && sDraw.player == player && sourceDL == sDraw.fittedHand && destinationDL != NULL)
        sDraw.fittedHand = destinationDL;
}

static s32 DinFireSword_Profile(Player* player) {
    if (player->heldItemAction == PLAYER_IA_SWORD_BIGGORON && player->leftHandType == PLAYER_MODELTYPE_LH_BGS)
        return !WeaponUpgrade_HasGreatFairy() && gSaveContext.swordHealth <= 0.0f ? 3 : 2;
    if (player->leftHandType == PLAYER_MODELTYPE_LH_SWORD &&
        player->heldItemAction == (LINK_IS_ADULT ? PLAYER_IA_SWORD_MASTER : PLAYER_IA_SWORD_KOKIRI))
        return gSaveContext.linkAge;
    return -1;
}

static s32 DinFireSword_Eligible(PlayState* play, Player* player) {
    if (play == NULL || player == NULL || player != GET_PLAYER(play) ||
        ((SwordElement_Override() != SWORD_ELEMENT_NONE || SwordElement_HasSelection(player->heldItemId)) &&
         SwordElement_Active(play, player) != SWORD_ELEMENT_FIRE) ||
        !CVarGetInteger(CVAR_ENHANCEMENT("DinFireSword"), 0) || !CVarGetInteger(CVAR_SETTING("AltAssets"), 1) ||
        TransformMasks_IsTransformedAny() || BossRemains_IsOdolwaWorn() || BossRemains_IsGohtWorn() ||
        ExtEquip_ShouldHideSwordDL() || FourSword_IsEquipped() || GameInteractor_InvisibleLinkActive() ||
        player->actor.scale.y <= 0.0f || player->csAction != 0 ||
        (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_WATER)) ||
        (player->stateFlags2 & PLAYER_STATE2_DISABLE_DRAW) || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        DinFireSword_Profile(player) < 0) {
        return false;
    }
    if (DinFireSword_IsUpgrade(player)) {
        const char* path = sProgressiveEquipment[DinFireSword_Profile(player)];
        if (!ResourceMgr_FileExists(path) || ResourceMgr_LoadGfxByName(path) == NULL)
            return false;
    }
    return ResourceMgr_FileExists(sEquipment[gSaveContext.linkAge]);
}

static s32 DinFireSword_AuraModel(Player* player) {
    if (player->heldItemId != ITEM_SWORD_KOKIRI && player->heldItemId != ITEM_SWORD_MASTER &&
        player->heldItemId != ITEM_SWORD_BGS && player->heldItemId != ITEM_SWORD_KNIFE)
        return -1;
    if (player->heldItemAction == PLAYER_IA_SWORD_KOKIRI && player->leftHandType == PLAYER_MODELTYPE_LH_SWORD) {
        u8 level = WeaponUpgrade_KokiriLevel();
        return level >= 2 && CVarGetInteger("gEnhancements.SkijerNEI.GildedUsesGildedLook", 1) ? SWORD_AURA_GILDED_SWORD
               : level                                                                         ? SWORD_AURA_RAZOR_SWORD
                       : SWORD_AURA_KOKIRI_SWORD;
    }
    if (player->heldItemAction == PLAYER_IA_SWORD_MASTER && player->leftHandType == PLAYER_MODELTYPE_LH_SWORD)
        return WeaponUpgrade_HasTrueMaster() ? SWORD_AURA_TRUE_MASTER_SWORD : SWORD_AURA_MASTER_SWORD;
    if (player->heldItemAction == PLAYER_IA_SWORD_BIGGORON && player->leftHandType == PLAYER_MODELTYPE_LH_BGS) {
        if (WeaponUpgrade_HasGreatFairy() && CVarGetInteger("gEnhancements.SkijerNEI.BgsUsesGfsLook", 1))
            return SWORD_AURA_GREAT_FAIRY_SWORD;
        if ((player->heldItemId == ITEM_SWORD_KNIFE || !gSaveContext.bgsFlag) && gSaveContext.swordHealth <= 0.f)
            return SWORD_AURA_BROKEN_KNIFE;
        return SWORD_AURA_BIGGORON_SWORD;
    }
    return -1;
}

static s32 DinFireSword_AuraRig(Player* player) {
    return gSaveContext.linkAge;
}

static u8 DinFireSword_SelectedAura(PlayState* play, Player* player) {
    if (play == NULL || player == NULL || player != GET_PLAYER(play) || TransformMasks_IsTransformedAny() ||
        BossRemains_IsOdolwaWorn() || BossRemains_IsGohtWorn() || ExtEquip_ShouldHideSwordDL() ||
        FourSword_IsEquipped() || GameInteractor_InvisibleLinkActive() || player->actor.scale.y <= 0.0f ||
        player->csAction != 0 || (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_WATER)) ||
        (player->stateFlags2 & PLAYER_STATE2_DISABLE_DRAW) || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        DinFireSword_AuraModel(player) < 0)
        return SWORD_ELEMENT_NONE;
    return SwordElement_Active(play, player);
}

static s32 DinFireSword_ShouldDraw(PlayState* play, Player* player) {
    if (play == NULL || player == NULL || player != GET_PLAYER(play))
        return false;
    if (SwordElement_Override() != SWORD_ELEMENT_NONE || SwordElement_HasSelection(player->heldItemId))
        return DinFireSword_SelectedAura(play, player) != SWORD_ELEMENT_NONE;
    return DinFireSword_Eligible(play, player);
}

static s32 DinFireSword_SameContext(PlayState* play, Player* player) {
    return sSword.play == play && sSword.player == player && sSword.scene == play->sceneNum &&
           sSword.age == gSaveContext.linkAge;
}

void DinFireSword_Reset(void) {
    sSword.play = NULL;
    sSword.player = NULL;
    sSword.phase = 0;
    sSword.lastFrame = UINT32_MAX;
    DinFireSword_BeginPlayerDraw(NULL, NULL);
}

void DinFireSword_Update(PlayState* play, Player* player) {
    if (play == NULL || player == NULL || player != GET_PLAYER(play))
        return;
    if (!DinFireSword_ShouldDraw(play, player)) {
        DinFireSword_Reset();
        return;
    }
    if (!DinFireSword_SameContext(play, player) ||
        (sSword.lastFrame != UINT32_MAX && play->gameplayFrames < sSword.lastFrame)) {
        DinFireSword_Reset();
        sSword.play = play;
        sSword.player = player;
        sSword.scene = play->sceneNum;
        sSword.age = gSaveContext.linkAge;
    }
    if (play->pauseCtx.state != 0 || play->pauseCtx.debugState != 0 || sSword.lastFrame == play->gameplayFrames)
        return;
    sSword.lastFrame = play->gameplayFrames;
    sSword.phase = (sSword.phase + 1) & 1023;
}

static s32 DinFireSword_Load(s32 age, Gfx** core, Gfx** flame) {
    const char* paths[] = { sCoreDL[age],        sFlameDL[age],        sCoreVertices[age],
                            sFlameVertices[age], SWORD_ROOT "CoreTex", SWORD_ROOT "FlameTex" };
    for (size_t i = 0; i < ARRAY_COUNT(paths); ++i) {
        if (!ResourceMgr_FileExists(paths[i]))
            return false;
    }
    *core = ResourceMgr_LoadGfxByName(sCoreDL[age]);
    *flame = ResourceMgr_LoadGfxByName(sFlameDL[age]);
    return *core != NULL && *flame != NULL && ResourceGetDataByName(sCoreVertices[age]) != NULL &&
           ResourceGetDataByName(sFlameVertices[age]) != NULL && ResourceGetDataByName(SWORD_ROOT "CoreTex") != NULL &&
           ResourceGetDataByName(SWORD_ROOT "FlameTex") != NULL;
}

uint32_t DinFireSword_DamageFlags(PlayState* play, Player* player, uint32_t original) {
    if (!(original & DMG_SWORD) || (original & ~((u32)DMG_SWORD)))
        return original;
    // The selector owns arrow, checkbox and local regular-sword gates. Generic
    // enchantments have no dependency on the optional Din render resources.
    switch (SwordElement_Active(play, player)) {
        case SWORD_ELEMENT_FIRE:
            return original | DMG_ARROW_FIRE;
        case SWORD_ELEMENT_ICE:
            return original | DMG_ARROW_ICE;
        case SWORD_ELEMENT_LIGHT:
            return original | DMG_ARROW_LIGHT;
    }
    if ((player != NULL &&
         (SwordElement_Override() != SWORD_ELEMENT_NONE || SwordElement_HasSelection(player->heldItemId))) ||
        !SwordElement_IsAvailable(SWORD_ELEMENT_FIRE) || !CVarGetInteger(CVAR_ENHANCEMENT("DinFireSwordDamage"), 0) ||
        !DinFireSword_Eligible(play, player))
        return original;
    Gfx *core, *flame;
    if (!DinFireSword_Load(DinFireSword_Profile(player), &core, &flame))
        return original;
    // Keep sword-only collision masks and sword recognition intact. The owned
    // hit's table lookup below resolves fire without replacing sword power.
    return original | DMG_ARROW_FIRE;
}

void* DinFireSword_HandDL(PlayState* play, Player* player, void* closedHand, u8 r, u8 g, u8 b) {
    if (closedHand == NULL ||
        (player != NULL &&
         (SwordElement_Override() != SWORD_ELEMENT_NONE || SwordElement_HasSelection(player->heldItemId))) ||
        !DinFireSword_Eligible(play, player) || !DinFireSword_IsUpgrade(player))
        return NULL;
    s32 profile = DinFireSword_Profile(player);
    Gfx *core, *flame;
    Gfx* blade = ResourceMgr_LoadGfxByName(sProgressiveEquipment[profile]);
    if (blade == NULL || !DinFireSword_Load(profile, &core, &flame))
        return NULL;
    Gfx* dl = Graph_Alloc(play->state.gfxCtx, 6 * sizeof(Gfx));
    Gfx* p = dl;
    gSPDisplayList(p++, blade);
    gSPDisplayList(p++, closedHand);
    gDPPipeSync(p++);
    gDPSetEnvColor(p++, r, g, b, 0);
    gSPLoadGeometryMode(p++, G_ZBUFFER | G_SHADE | G_CULL_BACK | G_LIGHTING | G_SHADING_SMOOTH);
    gSPEndDisplayList(p);
    return dl;
}

// Crouch stabs intentionally reuse the previous strike's damage in vanilla.
// Remember that original value separately so toggling fire never poisons it.
static struct {
    PlayState* play;
    Player* player;
    s16 scene;
    s32 age;
    u32 original[2], applied[2];
    u8 known[2];
} sDamage;

static void DinFireSword_DamageContext(PlayState* play, Player* player) {
    if (sDamage.play != play || sDamage.player != player || sDamage.scene != play->sceneNum ||
        sDamage.age != gSaveContext.linkAge) {
        // The Time Gate can change age on the same live player. Undo only our
        // exact prior writes before dropping that context, so disabling fire
        // during the change cannot leave a combined hit behind.
        if (sDamage.play == play && sDamage.player == player) {
            for (int quad = 0; quad < 2; ++quad) {
                if (sDamage.known[quad] &&
                    player->meleeWeaponQuads[quad].info.toucher.dmgFlags == sDamage.applied[quad])
                    player->meleeWeaponQuads[quad].info.toucher.dmgFlags = sDamage.original[quad];
            }
        }
        sDamage.play = play;
        sDamage.player = player;
        sDamage.scene = play->sceneNum;
        sDamage.age = gSaveContext.linkAge;
        sDamage.known[0] = sDamage.known[1] = false;
    }
}

uint32_t DinFireSword_SetDamageFlags(PlayState* play, Player* player, int quad, uint32_t original) {
    if (play == NULL || player == NULL || player != GET_PLAYER(play) || quad < 0 || quad > 1)
        return original;
    DinFireSword_DamageContext(play, player);
    sDamage.original[quad] = original;
    sDamage.applied[quad] = DinFireSword_DamageFlags(play, player, original);
    sDamage.known[quad] = true;
    return sDamage.applied[quad];
}

void DinFireSword_RefreshDamage(PlayState* play, Player* player) {
    if (play == NULL || player == NULL || player != GET_PLAYER(play))
        return;
    DinFireSword_DamageContext(play, player);
    for (int quad = 0; quad < 2; ++quad) {
        u32 flags = player->meleeWeaponQuads[quad].info.toucher.dmgFlags;
        if (sDamage.known[quad] && flags == sDamage.applied[quad])
            flags = sDamage.original[quad];
        player->meleeWeaponQuads[quad].info.toucher.dmgFlags = DinFireSword_SetDamageFlags(play, player, quad, flags);
    }
}

uint32_t DinFireSword_OriginalDamageFlags(PlayState* play, const ColliderInfo* hitInfo) {
    if (hitInfo == NULL)
        return 0;
    u32 flags = hitInfo->toucher.dmgFlags;
    u32 spinOriginal = SpinAttack_OriginalDamageFlags(play, hitInfo);
    if (spinOriginal != flags)
        return spinOriginal;
    if (play == NULL || sDamage.play != play || sDamage.player == NULL || sDamage.player != GET_PLAYER(play) ||
        sDamage.scene != play->sceneNum || sDamage.age != gSaveContext.linkAge)
        return flags;
    for (int quad = 0; quad < 2; ++quad) {
        if (hitInfo == &sDamage.player->meleeWeaponQuads[quad].info && sDamage.known[quad] &&
            flags == sDamage.applied[quad] &&
            (flags == (sDamage.original[quad] | DMG_ARROW_FIRE) || flags == (sDamage.original[quad] | DMG_ARROW_ICE) ||
             flags == (sDamage.original[quad] | DMG_ARROW_LIGHT)))
            return sDamage.original[quad];
    }
    return flags;
}

uint8_t DinFireSword_DamageEntry(PlayState* play, Actor* target, const ColliderInfo* hitInfo, uint32_t receiverFlags,
                                 uint8_t vanillaEntry) {
    if (target == NULL || target->colChkInfo.damageTable == NULL || hitInfo == NULL)
        return vanillaEntry;
    u32 original = DinFireSword_OriginalDamageFlags(play, hitInfo);
    if (original == hitInfo->toucher.dmgFlags)
        return vanillaEntry;

    DamageTable* table = target->colChkInfo.damageTable;
    u32 elementFlag = hitInfo->toucher.dmgFlags ^ original;
    int elementIndex = elementFlag == DMG_ARROW_FIRE ? 0x0B : elementFlag == DMG_ARROW_ICE ? 0x0C : 0x0D;
    u8 elementEntry = table->table[elementIndex];
    // A receiver that admits only the added bit keeps its native element row.
    if (!(original & receiverFlags))
        return (receiverFlags & elementFlag) ? elementEntry : vanillaEntry;

    int index = 0;
    for (u32 flags = original; flags > 1; flags >>= 1)
        ++index;
    u8 swordEntry = table->table[index];
    if (!(receiverFlags & elementFlag))
        return swordEntry;

    if (elementFlag != DMG_ARROW_FIRE) {
        // Element codes are actor-local. These native paths apply the same
        // numeric damage before adding ice/light; immunity and cutting routes
        // keep their sword row. Armos ice is an instant-kill state change, not
        // an additive freeze, so it deliberately keeps its native sword path.
        if (!(elementEntry & 0xF))
            return swordEntry;
        if (elementFlag == DMG_ARROW_ICE) {
            switch (target->id) {
                case ACTOR_EN_CROW:
                case ACTOR_EN_FIREFLY:
                    return (swordEntry & 0xF) | (elementEntry & 0xF0);
                case ACTOR_EN_ZF:
                    // Repeating the ice row while already stunned skips damage
                    // in the native actor. Its sword row applies damage and
                    // leaves stun normally, so retain that response on repeats.
                    return ((EnZf*)target)->action == ENZF_ACTION_STUNNED ? swordEntry
                                                                          : (swordEntry & 0xF) | (elementEntry & 0xF0);
            }
        } else {
            switch (target->id) {
                case ACTOR_EN_WALLMAS:
                case ACTOR_EN_FLOORMAS:
                    return (swordEntry & 0xF) | (elementEntry & 0xF0);
            }
        }
        return swordEntry;
    }
    u8 fireEntry = elementEntry;

    // Anubis ignores sword-table damage and only dies on its FIRE effect.
    // Freezard's zero-damage Kokiri row is inert despite its normal-hit effect.
    if (target->id == ACTOR_EN_ANUBICE ||
        (((swordEntry == 0) || (target->id == ACTOR_EN_FZ && !(swordEntry & 0xF))) && (fireEntry & 0xF)))
        return fireEntry;

    // These actors share the ordinary damage/recoil path with their fire
    // reaction. Keep the sword's power and add that actor's own fire effect.
    // Other effects are not interchangeable: Baba cutting, jellyfish shock,
    // Armos KILL and Lizalfos projectile recoil must retain the sword row.
    if ((swordEntry >> 4) == 0 && (fireEntry & 0xF)) {
        switch (target->id) {
            case ACTOR_EN_WF:       // FIRE = 14; normal hit plus burn timer.
            case ACTOR_EN_WALLMAS:  // FIRE = 2; normal hit plus flame effect.
            case ACTOR_EN_FLOORMAS: // FIRE = 2; normal hit plus flame effect.
            case ACTOR_EN_CROW:     // FIRE = 2; normal fall plus flames.
            case ACTOR_EN_DEKUNUTS: // FIRE = 2; normal hit plus flame effect.
            case ACTOR_EN_PEEHAT:   // FIRE = 12; normal hit plus flames/filter.
            case ACTOR_EN_FIREFLY:  // FIRE_ARROW = 15; normal fall, ice Keese combust.
                return (swordEntry & 0xF) | (fireEntry & 0xF0);
        }
    }
    return swordEntry;
}

static void DinFireSword_Material(Gfx** display, s32 flame, Color_RGB8 core, Color_RGB8 outer, u16 phase) {
    s32 scroll = (phase * (flame ? 5 : 3)) & 127;
    u8 alpha = flame ? (u8)((0.96f + 0.04f * sinf(phase * 0.71f)) * 255.0f) : 255;
    gSPClearGeometryMode((*display)++, G_LIGHTING | G_FOG | G_CULL_BOTH | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR);
    gDPSetCycleType((*display)++, G_CYC_2CYCLE);
    gDPSetRenderMode((*display)++, G_RM_PASS, flame ? G_RM_AA_ZB_XLU_SURF2 : G_RM_AA_ZB_OPA_SURF2);
    gDPSetTextureLUT((*display)++, G_TT_NONE);
    gDPSetTextureFilter((*display)++, G_TF_BILERP);
    gDPSetAlphaCompare((*display)++, G_AC_NONE);
    gSPTexture((*display)++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    if (flame) {
        gDPSetCombineLERP((*display)++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, TEXEL0, 0, SHADE, 0, 0, 0, 0,
                          COMBINED, COMBINED, 0, PRIMITIVE, 0);
    } else {
        // A continuous hot blade beneath the transparent tongues. The source
        // sword stays intact; the private close-fitting core covers its metal.
        gDPSetCombineLERP((*display)++, PRIMITIVE, ENVIRONMENT, TEXEL0, ENVIRONMENT, 0, 0, 0, SHADE, 0, 0, 0, COMBINED,
                          COMBINED, 0, PRIMITIVE, 0);
    }
    // Logical dimensions stay native. Named resources carry physical HD size.
    gDPLoadTextureBlock((*display)++, flame ? sFlameTexture : sCoreTexture, G_IM_FMT_I, G_IM_SIZ_8b, 64, 32, 0,
                        G_TX_WRAP, G_TX_WRAP, 6, 5, G_TX_NOLOD, G_TX_NOLOD);
    gDPSetTileSize((*display)++, 0, 0, scroll, 63 << 2, scroll + (31 << 2));
    gDPSetPrimColor((*display)++, 0, 0, core.r, core.g, core.b, alpha);
    gDPSetEnvColor((*display)++, outer.r, outer.g, outer.b, 255);
}

static void DinFireSword_DrawLayers(PlayState* play, s32 profile, u16 phase, Mtx* matrix) {
    Gfx* coreDL;
    Gfx* flameDL;
    if (matrix == NULL || !DinFireSword_Load(profile, &coreDL, &flameDL))
        return;
    const Color_RGB8 core =
        CVarGetColor24(CVAR_COSMETIC("Custom.DinFireSwordCore.Value"), (Color_RGB8){ 255, 225, 122 });
    const Color_RGB8 outer =
        CVarGetColor24(CVAR_COSMETIC("Custom.DinFireSwordOuter.Value"), (Color_RGB8){ 255, 43, 3 });

    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    gSPMatrix(POLY_OPA_DISP++, matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    DinFireSword_Material(&POLY_OPA_DISP, false, core, outer, phase);
    gSPDisplayList(POLY_OPA_DISP++, coreDL);
    gDPPipeSync(POLY_OPA_DISP++);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);

    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPMatrix(POLY_XLU_DISP++, matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    DinFireSword_Material(&POLY_XLU_DISP, true, core, outer, phase);
    gSPDisplayList(POLY_XLU_DISP++, flameDL);
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

static void DinFireSword_DrawSelectedAura(PlayState* play, u8 element, s32 model, s32 rig, Mtx* matrix) {
    const SwordAuraGeometry* geometry = &sSwordAuraGeometry[model][rig];
    Color_RGB8 core = { 210, 245, 255 }, outer = { 54, 168, 255 };
    if (element == SWORD_ELEMENT_LIGHT) {
        core = (Color_RGB8){ 255, 255, 235 };
        outer = (Color_RGB8){ 255, 210, 106 };
    } else if (element == SWORD_ELEMENT_FIRE) {
        core = CVarGetColor24(CVAR_COSMETIC("Custom.DinFireSwordCore.Value"), (Color_RGB8){ 255, 225, 122 });
        outer = CVarGetColor24(CVAR_COSMETIC("Custom.DinFireSwordOuter.Value"), (Color_RGB8){ 255, 43, 3 });
    }

    OPEN_DISPS(play->state.gfxCtx);
    if (element == SWORD_ELEMENT_FIRE) {
        Gfx_SetupDL_25Opa(play->state.gfxCtx);
        gSPMatrix(POLY_OPA_DISP++, matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        SwordAura_Material(&POLY_OPA_DISP, element, false, core, outer, sSword.phase);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)geometry->core);
        gDPPipeSync(POLY_OPA_DISP++);
        Gfx_SetupDL_25Opa(play->state.gfxCtx);
    }
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPMatrix(POLY_XLU_DISP++, matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    if (element != SWORD_ELEMENT_FIRE) {
        SwordAura_Material(&POLY_XLU_DISP, element, false, core, outer, sSword.phase);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)geometry->core);
        gDPPipeSync(POLY_XLU_DISP++);
    }
    SwordAura_Material(&POLY_XLU_DISP, element, true, core, outer, sSword.phase);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)(element == SWORD_ELEMENT_FIRE ? geometry->flame : geometry->tight));
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

static void DinFireSword_CaptureHand(PlayState* play, Player* player, void* finalHandDL) {
    if (play == NULL || player == NULL || player != GET_PLAYER(play))
        return;
    if (player->actor.scale.y < 0.0f)
        return;
    if (!DinFireSword_ShouldDraw(play, player)) {
        DinFireSword_Reset();
        return;
    }
    if (!DinFireSword_SameContext(play, player) || sDraw.play != play || sDraw.player != player)
        return;
    // Record the exact hand transform here, inside its own interpolation scope.
    // Later mask/boot draws can change the interpolation base as well as the
    // current matrix. Reusing this recorded matrix preserves sword alignment.
    OPEN_DISPS(play->state.gfxCtx);
    sDraw.frame = play->gameplayFrames;
    sDraw.element = DinFireSword_SelectedAura(play, player);
    sDraw.logicalModel = sDraw.model = DinFireSword_AuraModel(player);
    if (finalHandDL != NULL) {
        if (finalHandDL == sDraw.fittedHand && sDraw.fittedModel >= 0) {
            sDraw.model = sDraw.fittedModel;
        } else if (memcmp(finalHandDL, "__OTR__", 7) == 0) {
            int native = SwordAura_ResourceModel((const char*)finalHandDL, NULL, 0);
            if (native >= 0)
                sDraw.model = native;
        }
    }
    sDraw.rig = DinFireSword_AuraRig(player);
    if (sDraw.element != SWORD_ELEMENT_NONE && CVarGetInteger("gDeveloperTools.SwordAuraDiagnostics", 0)) {
        static u8 reported[SWORD_AURA_MODEL_COUNT][3];
        const int matched = finalHandDL != NULL && finalHandDL == sDraw.fittedHand && sDraw.fittedModel >= 0;
        const char* path =
            finalHandDL != NULL && memcmp(finalHandDL, "__OTR__", 7) == 0 ? (const char*)finalHandDL : NULL;
        const u8 source = matched ? 1 : path != NULL ? 2 : 0;
        if (sDraw.model >= 0 && sDraw.model < SWORD_AURA_MODEL_COUNT && sDraw.rig >= 0 && sDraw.rig < 3 &&
            !(reported[sDraw.model][sDraw.rig] & (1 << source))) {
            reported[sDraw.model][sDraw.rig] |= 1 << source;
            lusprintf(__FILE__, __LINE__, LUSLOG_LEVEL_INFO,
                      "[SwordAura:OoT] logical=%d fit=%d rig=%d element=%u matched=%d final=%s", sDraw.logicalModel,
                      sDraw.model, sDraw.rig, sDraw.element, matched,
                      path != NULL          ? path
                      : finalHandDL != NULL ? "<composed>"
                                            : "<logical fallback>");
        }
    }
    sDraw.handMatrix = MATRIX_NEWMTX(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

void DinFireSword_Draw(PlayState* play, Player* player) {
    DinFireSword_CaptureHand(play, player, NULL);
}

void DinFireSword_DrawHand(PlayState* play, Player* player, void* finalHandDL) {
    if (finalHandDL != NULL)
        DinFireSword_CaptureHand(play, player, finalHandDL);
}

void DinFireSword_DrawAfterPlayer(PlayState* play, Player* player) {
    if (sDraw.play != play || sDraw.player != player)
        return;
    Mtx* matrix = sDraw.handMatrix;
    u32 frame = sDraw.frame;
    u8 element = sDraw.element;
    s32 model = sDraw.model, rig = sDraw.rig, logical = sDraw.logicalModel;
    DinFireSword_BeginPlayerDraw(NULL, NULL);
    if (matrix == NULL || frame != play->gameplayFrames || !DinFireSword_ShouldDraw(play, player) ||
        !DinFireSword_SameContext(play, player) || element != DinFireSword_SelectedAura(play, player))
        return;
    if (element != SWORD_ELEMENT_NONE) {
        if (logical != DinFireSword_AuraModel(player) || rig != DinFireSword_AuraRig(player))
            return;
        DinFireSword_DrawSelectedAura(play, element, model, rig, matrix);
    } else {
        DinFireSword_DrawLayers(play, DinFireSword_Profile(player), sSword.phase, matrix);
    }
}

// The custom pedestal calls this only while drawing its selected sword and
// with that sword's placement matrix current. Never creates its own actor.
void DinFireSword_DrawPedestal(PlayState* play) {
    if (play == NULL || !CVarGetInteger(CVAR_ENHANCEMENT("DinFireSword"), 0) ||
        !CVarGetInteger(CVAR_SETTING("AltAssets"), 1) || !ResourceMgr_FileExists(sEquipment[LINK_AGE_ADULT]))
        return;
    OPEN_DISPS(play->state.gfxCtx);
    DinFireSword_DrawLayers(play, LINK_AGE_ADULT, play->gameplayFrames & 1023, MATRIX_NEWMTX(play->state.gfxCtx));
    CLOSE_DISPS(play->state.gfxCtx);
}
