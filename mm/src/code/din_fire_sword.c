#include "global.h"
#include "spin_attack_style.h"
#include "overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.h"
#define CW_SPIN_OWNERSHIP_MM
#include "../../../combo/gameplay/SpinAttackOwnership.h"
#undef CW_SPIN_OWNERSHIP_MM
#include "align_asset_macro.h"
#include "din_fire_sword.h"
#include "sword_element.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include "soh/ResourceManagerHelpers.h"
#include "mods/nei_save.h"
#include "mods/forms/custom_forms.h"
#include "mods/items/logic/adult_link_render.h"
#include "mods/boss_remains/boss_remains.h"
#include "mods/extended_equipment.h"
#include <libultraship/bridge/resourcebridge.h>
#include <libultraship/log/luslog.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <math.h>
#include "../../../combo/render/sword_element_aura.h"
#include "../../../combo/render/sword_element_aura_identity.h"
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
    return player->heldItemId == ITEM_SWORD_RAZOR || player->heldItemId == ITEM_SWORD_GILDED ||
           player->heldItemId == ITEM_SWORD_GREAT_FAIRY ||
           (player->heldItemAction == PLAYER_IA_SWORD_KOKIRI && WeaponUpgrade_KokiriLevel()) ||
           (player->heldItemAction == PLAYER_IA_SWORD_TWO_HANDED && WeaponUpgrade_HasGreatFairy());
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
    // Custom items reuse sword actions (notably the net and Four Sword).
    // They retain ownership of their hand geometry and attack flags.
    if (player->heldItemId != ITEM_SWORD_KOKIRI && player->heldItemId != ITEM_SWORD_MASTER &&
        player->heldItemId != ITEM_SWORD_BGS && player->heldItemId != ITEM_SWORD_KNIFE &&
        player->heldItemId != ITEM_SWORD_RAZOR && player->heldItemId != ITEM_SWORD_GILDED &&
        player->heldItemId != ITEM_SWORD_GREAT_FAIRY)
        return -1;
    if (player->heldItemAction == PLAYER_IA_SWORD_TWO_HANDED &&
        player->leftHandType == PLAYER_MODELTYPE_LH_TWO_HAND_SWORD)
        return 2;
    if (player->leftHandType == PLAYER_MODELTYPE_LH_ONE_HAND_SWORD &&
        (player->heldItemAction == PLAYER_IA_SWORD_MASTER || player->heldItemAction == PLAYER_IA_SWORD_KOKIRI ||
         player->heldItemAction == PLAYER_IA_SWORD_RAZOR || player->heldItemAction == PLAYER_IA_SWORD_GILDED))
        return AdultLink_IsActive() ? 0 : 1;
    return -1;
}

static s32 DinFireSword_Eligible(PlayState* play, Player* player) {
    if (play == NULL || player == NULL || player != GET_PLAYER(play) ||
        ((SwordElement_Override() != SWORD_ELEMENT_NONE || SwordElement_HasSelection(player->heldItemId)) &&
         SwordElement_Active(play, player) != SWORD_ELEMENT_FIRE) ||
        !CVarGetInteger(CVAR_ENHANCEMENT("DinFireSword"), 0) || !ResourceMgr_IsAltAssetsEnabled() ||
        (player->transformation != PLAYER_FORM_HUMAN || CustomForms_ActiveForm() != CUSTOM_FORM_NONE) ||
        BossRemains_IsOdolwaWorn() || BossRemains_IsGohtWorn() || player->actor.scale.y <= 0.0f ||
        player->csAction != 0 || (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_8000000)) ||
        (player->stateFlags2 & PLAYER_STATE2_20000000) || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        ExtEquip_ShouldHideSwordDL() || FourSword_IsEquipped() || DinFireSword_Profile(player) < 0) {
        return false;
    }
    if (DinFireSword_IsUpgrade(player)) {
        const char* path = sProgressiveEquipment[DinFireSword_Profile(player)];
        if (!ResourceMgr_FileExists(path) || ResourceMgr_LoadGfxByName(path) == NULL)
            return false;
    }
    return ResourceMgr_FileExists(sEquipment[(AdultLink_IsActive() ? 0 : 1)]);
}

static s32 DinFireSword_AuraModel(Player* player) {
    if (player->heldItemId != ITEM_SWORD_KOKIRI && player->heldItemId != ITEM_SWORD_MASTER &&
        player->heldItemId != ITEM_SWORD_BGS && player->heldItemId != ITEM_SWORD_KNIFE &&
        player->heldItemId != ITEM_SWORD_RAZOR && player->heldItemId != ITEM_SWORD_GILDED &&
        player->heldItemId != ITEM_SWORD_GREAT_FAIRY)
        return -1;
    if (player->leftHandType == PLAYER_MODELTYPE_LH_ONE_HAND_SWORD) {
        if (player->heldItemId == ITEM_SWORD_RAZOR && player->heldItemAction == PLAYER_IA_SWORD_RAZOR)
            return SWORD_AURA_RAZOR_SWORD;
        if (player->heldItemId == ITEM_SWORD_GILDED && player->heldItemAction == PLAYER_IA_SWORD_GILDED)
            return SWORD_AURA_GILDED_SWORD;
        if (player->heldItemId == ITEM_SWORD_KOKIRI && player->heldItemAction == PLAYER_IA_SWORD_KOKIRI) {
            u8 level = WeaponUpgrade_KokiriLevel();
            return level >= 2 && CVarGetInteger("gEnhancements.SkijerNEI.GildedUsesGildedLook", 1)
                       ? SWORD_AURA_GILDED_SWORD
                   : level ? SWORD_AURA_RAZOR_SWORD
                           : SWORD_AURA_MM_KOKIRI_SWORD;
        }
        if (player->heldItemId == ITEM_SWORD_MASTER && player->heldItemAction == PLAYER_IA_SWORD_MASTER)
            return WeaponUpgrade_HasTrueMaster() ? SWORD_AURA_TRUE_MASTER_SWORD : SWORD_AURA_MASTER_SWORD;
    }
    if (player->heldItemAction == PLAYER_IA_SWORD_TWO_HANDED &&
        player->leftHandType == PLAYER_MODELTYPE_LH_TWO_HAND_SWORD) {
        if (player->heldItemId == ITEM_SWORD_GREAT_FAIRY ||
            (WeaponUpgrade_HasGreatFairy() && CVarGetInteger("gEnhancements.SkijerNEI.BgsUsesGfsLook", 1)))
            return SWORD_AURA_GREAT_FAIRY_SWORD;
        if (player->heldItemId == ITEM_SWORD_KNIFE)
            return SWORD_AURA_BROKEN_KNIFE;
        if (player->heldItemId == ITEM_SWORD_BGS)
            return SWORD_AURA_BIGGORON_SWORD;
    }
    return -1;
}

static s32 DinFireSword_AuraRig(Player* player) {
    return AdultLink_UsesAdultPresentation(player) ? 0 : 2;
}

static u8 DinFireSword_SelectedAura(PlayState* play, Player* player) {
    if (play == NULL || player == NULL || player != GET_PLAYER(play) ||
        (player->transformation != PLAYER_FORM_HUMAN || CustomForms_ActiveForm() != CUSTOM_FORM_NONE) ||
        BossRemains_IsOdolwaWorn() || BossRemains_IsGohtWorn() || ExtEquip_ShouldHideSwordDL() ||
        FourSword_IsEquipped() || player->actor.scale.y <= 0.0f || player->csAction != 0 ||
        (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_8000000)) ||
        (player->stateFlags2 & PLAYER_STATE2_20000000) || play->transitionTrigger != TRANS_TRIGGER_OFF ||
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
    return sSword.play == play && sSword.player == player && sSword.scene == play->sceneId &&
           sSword.age == (AdultLink_IsActive() ? 0 : 1);
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
        sSword.scene = play->sceneId;
        sSword.age = (AdultLink_IsActive() ? 0 : 1);
    }
    if (play->pauseCtx.state != 0 || play->pauseCtx.debugEditor != 0 || sSword.lastFrame == play->gameplayFrames)
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

void* DinFireSword_HandDL(PlayState* play, Player* player, void* closedHand) {
    if (closedHand == NULL ||
        (player != NULL &&
         (SwordElement_Override() != SWORD_ELEMENT_NONE || SwordElement_HasSelection(player->heldItemId))) ||
        !DinFireSword_Eligible(play, player))
        return NULL;
    s32 profile = DinFireSword_Profile(player);
    s32 upgraded = DinFireSword_IsUpgrade(player);
    if (profile > 1 && !upgraded)
        return NULL; // Biggoron keeps its matching native full-length mesh.
    Gfx *core, *flame;
    Gfx* blade = ResourceMgr_LoadGfxByName(upgraded ? sProgressiveEquipment[profile] : sEquipment[profile]);
    if (blade == NULL || !DinFireSword_Load(profile, &core, &flame))
        return NULL;
    Gfx* dl = GRAPH_ALLOC(play->state.gfxCtx, 5 * sizeof(Gfx));
    Gfx* p = dl;
    gSPDisplayList(p++, blade);
    gSPDisplayList(p++, closedHand);
    gDPPipeSync(p++);
    // Private equipment materials must not recolor later adult body limbs.
    if (AdultLink_IsActive()) {
        gDPSetEnvColor(p++, CVarGetInteger("gAdultLink.TunicR", 30), CVarGetInteger("gAdultLink.TunicG", 105),
                       CVarGetInteger("gAdultLink.TunicB", 27), 255);
    }
    gSPEndDisplayList(p);
    return dl;
}

uint32_t DinFireSword_DamageFlags(PlayState* play, Player* player, uint32_t original) {
    if (!(original & DMG_SWORD) || (original & ~((u32)DMG_SWORD)))
        return original;
    switch (SwordElement_Active(play, player)) {
        case SWORD_ELEMENT_FIRE:
            return original | DMG_FIRE_ARROW;
        case SWORD_ELEMENT_ICE:
            return original | DMG_ICE_ARROW;
        case SWORD_ELEMENT_LIGHT:
            return original | DMG_LIGHT_ARROW;
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
    return original | DMG_FIRE_ARROW;
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
    if (sDamage.play != play || sDamage.player != player || sDamage.scene != play->sceneId ||
        sDamage.age != (AdultLink_IsActive() ? 0 : 1)) {
        // The Time Gate can change age on the same live player. Undo only our
        // exact prior writes before dropping that context, so disabling fire
        // during the change cannot leave a combined hit behind.
        if (sDamage.play == play && sDamage.player == player) {
            for (int quad = 0; quad < 2; ++quad) {
                if (sDamage.known[quad] &&
                    player->meleeWeaponQuads[quad].elem.atDmgInfo.dmgFlags == sDamage.applied[quad])
                    player->meleeWeaponQuads[quad].elem.atDmgInfo.dmgFlags = sDamage.original[quad];
            }
        }
        sDamage.play = play;
        sDamage.player = player;
        sDamage.scene = play->sceneId;
        sDamage.age = (AdultLink_IsActive() ? 0 : 1);
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
        u32 flags = player->meleeWeaponQuads[quad].elem.atDmgInfo.dmgFlags;
        if (sDamage.known[quad] && flags == sDamage.applied[quad])
            flags = sDamage.original[quad];
        player->meleeWeaponQuads[quad].elem.atDmgInfo.dmgFlags = DinFireSword_SetDamageFlags(play, player, quad, flags);
    }
}

uint32_t DinFireSword_OriginalDamageFlags(PlayState* play, const ColliderElement* hitInfo) {
    if (hitInfo == NULL)
        return 0;
    u32 flags = hitInfo->atDmgInfo.dmgFlags;
    u32 spinOriginal = SpinAttack_OriginalDamageFlags(play, hitInfo);
    if (spinOriginal != flags)
        return spinOriginal;
    if (play == NULL || sDamage.play != play || sDamage.player == NULL || sDamage.player != GET_PLAYER(play) ||
        sDamage.scene != play->sceneId || sDamage.age != (AdultLink_IsActive() ? 0 : 1))
        return flags;
    for (int quad = 0; quad < 2; ++quad) {
        if (hitInfo == &sDamage.player->meleeWeaponQuads[quad].elem && sDamage.known[quad] &&
            flags == sDamage.applied[quad] &&
            (flags == (sDamage.original[quad] | DMG_FIRE_ARROW) || flags == (sDamage.original[quad] | DMG_ICE_ARROW) ||
             flags == (sDamage.original[quad] | DMG_LIGHT_ARROW)))
            return sDamage.original[quad];
    }
    return flags;
}

uint8_t DinFireSword_DamageEntry(PlayState* play, Actor* target, const ColliderElement* hitInfo, uint32_t receiverFlags,
                                 uint8_t vanillaEntry) {
    if (target == NULL || target->colChkInfo.damageTable == NULL || hitInfo == NULL)
        return vanillaEntry;
    u32 original = DinFireSword_OriginalDamageFlags(play, hitInfo);
    if (original == hitInfo->atDmgInfo.dmgFlags)
        return vanillaEntry;

    DamageTable* table = target->colChkInfo.damageTable;
    u32 elementFlag = hitInfo->atDmgInfo.dmgFlags ^ original;
    int elementIndex = elementFlag == DMG_FIRE_ARROW ? 0x0B : elementFlag == DMG_ICE_ARROW ? 0x0C : 0x0D;
    u8 elementEntry = table->attack[elementIndex];
    if (!(original & receiverFlags))
        return (receiverFlags & elementFlag) ? elementEntry : vanillaEntry;

    int index = 0;
    for (u32 flags = original; flags > 1; flags >>= 1)
        ++index;
    u8 swordEntry = table->attack[index];
    if (!(receiverFlags & elementFlag))
        return swordEntry;

    if (elementFlag != DMG_FIRE_ARROW) {
        // Preserve the native sword multiplier. Element multipliers belong to
        // projectiles; only these actors' audited visual/state effect is added.
        // Baba's cut/stick route and inert/immune element rows stay native.
        if (!(elementEntry & 0xF))
            return swordEntry;
        switch (target->id) {
            case ACTOR_EN_WF:
            case ACTOR_EN_WALLMAS:
            case ACTOR_EN_FLOORMAS:
            case ACTOR_EN_CROW:
            case ACTOR_EN_DEKUNUTS:
            case ACTOR_EN_PEEHAT:
            case ACTOR_EN_FIREFLY:
            case ACTOR_EN_DINOFOS:
                return (swordEntry & 0xF) | (elementEntry & 0xF0);
            case ACTOR_EN_RD:
            case ACTOR_EN_FZ:
                if (elementFlag == DMG_LIGHT_ARROW)
                    return (swordEntry & 0xF) | (elementEntry & 0xF0);
                break;
        }
        return swordEntry;
    }
    u8 fireEntry = elementEntry;

    // A fire-vulnerable target can still accept a sword with an inert sword row.
    if (swordEntry == 0 && (fireEntry & 0xF))
        return fireEntry;

    // These actors share the ordinary damage/recoil path with their fire
    // reaction. Keep the sword's power and add that actor's own fire effect.
    // Other effects are not interchangeable: Baba cutting, jellyfish shock,
    // Armos KILL and Lizalfos projectile recoil must retain the sword row.
    if ((swordEntry >> 4) == 0 && (fireEntry & 0xF)) {
        switch (target->id) {
            case ACTOR_EN_WF:
            case ACTOR_EN_WALLMAS:
            case ACTOR_EN_FLOORMAS:
            case ACTOR_EN_CROW:
            case ACTOR_EN_DEKUNUTS:
            case ACTOR_EN_PEEHAT:
            case ACTOR_EN_FIREFLY:
                return (swordEntry & 0xF) | (fireEntry & 0xF0);
        }
    }
    return swordEntry;
}

int DinFireSword_IsFireHit(PlayState* play, const ColliderElement* hitInfo) {
    if (hitInfo == NULL)
        return false;
    u32 flags = hitInfo->atDmgInfo.dmgFlags;
    return flags == DMG_FIRE_ARROW ||
           (DinFireSword_OriginalDamageFlags(play, hitInfo) != flags && (flags & DMG_FIRE_ARROW));
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
    const Color_RGBA8 coreColor = CosmeticEditor_GetChangedColor(255, 225, 122, 255, "Custom.DinFireSwordCore");
    const Color_RGBA8 outerColor = CosmeticEditor_GetChangedColor(255, 43, 3, 255, "Custom.DinFireSwordOuter");
    const Color_RGB8 core = { coreColor.r, coreColor.g, coreColor.b };
    const Color_RGB8 outer = { outerColor.r, outerColor.g, outerColor.b };

    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    gSPMatrix(POLY_OPA_DISP++, matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    DinFireSword_Material(&POLY_OPA_DISP, false, core, outer, phase);
    gSPDisplayList(POLY_OPA_DISP++, coreDL);
    gDPPipeSync(POLY_OPA_DISP++);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    gSPMatrix(POLY_XLU_DISP++, matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    DinFireSword_Material(&POLY_XLU_DISP, true, core, outer, phase);
    gSPDisplayList(POLY_XLU_DISP++, flameDL);
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

static void DinFireSword_DrawSelectedAura(PlayState* play, u8 element, s32 model, s32 rig, Mtx* matrix) {
    const SwordAuraGeometry* geometry = &sSwordAuraGeometry[model][rig];
    Color_RGB8 core = { 210, 245, 255 }, outer = { 54, 168, 255 };
    if (element == SWORD_ELEMENT_LIGHT) {
        core = (Color_RGB8){ 255, 255, 235 };
        outer = (Color_RGB8){ 255, 210, 106 };
    } else if (element == SWORD_ELEMENT_FIRE) {
        Color_RGBA8 c = CosmeticEditor_GetChangedColor(255, 225, 122, 255, "Custom.DinFireSwordCore");
        Color_RGBA8 o = CosmeticEditor_GetChangedColor(255, 43, 3, 255, "Custom.DinFireSwordOuter");
        core = (Color_RGB8){ c.r, c.g, c.b };
        outer = (Color_RGB8){ o.r, o.g, o.b };
    }

    OPEN_DISPS(play->state.gfxCtx);
    if (element == SWORD_ELEMENT_FIRE) {
        Gfx_SetupDL25_Opa(play->state.gfxCtx);
        gSPMatrix(POLY_OPA_DISP++, matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        SwordAura_Material(&POLY_OPA_DISP, element, false, core, outer, sSword.phase);
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)geometry->core);
        gDPPipeSync(POLY_OPA_DISP++);
        Gfx_SetupDL25_Opa(play->state.gfxCtx);
    }
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    gSPMatrix(POLY_XLU_DISP++, matrix, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    if (element != SWORD_ELEMENT_FIRE) {
        SwordAura_Material(&POLY_XLU_DISP, element, false, core, outer, sSword.phase);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)geometry->core);
        gDPPipeSync(POLY_XLU_DISP++);
    }
    SwordAura_Material(&POLY_XLU_DISP, element, true, core, outer, sSword.phase);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)(element == SWORD_ELEMENT_FIRE ? geometry->flame : geometry->tight));
    gDPPipeSync(POLY_XLU_DISP++);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
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
                      "[SwordAura:MM] logical=%d fit=%d rig=%d element=%u matched=%d final=%s", sDraw.logicalModel,
                      sDraw.model, sDraw.rig, sDraw.element, matched,
                      path != NULL          ? path
                      : finalHandDL != NULL ? "<composed>"
                                            : "<logical fallback>");
        }
    }
    sDraw.handMatrix = Matrix_Finalize(play->state.gfxCtx);
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
