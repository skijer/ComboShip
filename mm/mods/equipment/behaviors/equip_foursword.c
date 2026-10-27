/**
 * equip_foursword.c - Four Sword (extended sword slot 2)
 *
 * R + B held for 15 frames summons three clone actors (four_sword_clone.c) that hold formation
 * around Link and mirror the item he just used. Included by ext_equip_behavior.c (unity build).
 */
#include "../../../../combo/NeiHeldSword.h"

#include "overlays/actors/ovl_En_Arrow/z_en_arrow.h"

#define FOURSWORD_BLADE_DL "__OTR__objects/object_nei_four_sword/gNeiFourSwordBladeDL"
#define FOURSWORD_HILT_DL "__OTR__objects/object_nei_four_sword/gNeiFourSwordHiltDL"

#define FS_CHARGE_HOLD 15
#define FS_ITEM_COOLDOWN 10

// Read from the slot itself, not from a flag the behavior refreshes: turning the cheat off never
// runs FourSword_Cleanup, and a stale flag would leave the clones orphaned in the world.
u8 FourSword_IsEquipped(void) {
    return ExtEquip_IsEnabled() && (gExtEquipState.currentExtSword == 2);
}

static void FourSword_ApplyChargeAnim(Player* player, PlayState* play) {
    AnimationContext_SetLoadFrame(play, &gPlayerAnim_link_fighter_power_kiru_wait, 0, player->skelAnime.limbCount,
                                  player->skelAnimeUpper.jointTable);
    for (s32 j = PLAYER_LIMB_UPPER; j < PLAYER_LIMB_MAX; j++) {
        player->skelAnime.jointTable[j] = player->skelAnimeUpper.jointTable[j];
    }
}

// Ivan's dispatch (z_en_partner.inc.c), not an actor scan: watch the player state for the frame a
// shot/throw leaves Link's hands, then spawn the same projectile at every clone.
static void FourSword_SpawnCloneProjectiles(Player* player, PlayState* play) {
    if (gExtEquipBehavior.fourSwordItemCooldown > 0) {
        gExtEquipBehavior.fourSwordItemCooldown--;
        goto update_prev;
    }
    if (FourSwordClone_Count() == 0) {
        goto update_prev;
    }

    // unk_D57 is MM's OoT unk_A73: set to 4 on the firing frame, then counted down.
    if ((player->unk_D57 == 4) && (gExtEquipBehavior.fourSwordPrevFireTimer != 4)) {
        static const s16 kBowArrows[] = { ARROW_TYPE_NORMAL, ARROW_TYPE_FIRE, ARROW_TYPE_ICE, ARROW_TYPE_LIGHT };
        s16 arrowType = ARROW_TYPE_NORMAL;
        PlayerItemAction ia = player->heldItemAction;

        if ((ia >= PLAYER_IA_BOW) && (ia <= PLAYER_IA_BOW_LIGHT)) {
            arrowType = kBowArrows[ia - PLAYER_IA_BOW];
        } else if (ia == PLAYER_IA_SLINGSHOT) {
            arrowType = ARROW_TYPE_SLINGSHOT;
        } else if (player->zoraBoomerangActor != NULL && gExtEquipBehavior.fourSwordPrevBoomerang == 0) {
            goto skip_arrow; // boomerang raises this same flag; its own branch spawns it
        } else {
            arrowType = ARROW_TYPE_DEKU_NUT;
        }

        for (u8 i = 0; i < FSC_MAX; i++) {
            Vec3f cp;
            if (!FourSwordClone_GetPos(i, &cp)) {
                continue;
            }
            // parent stays NULL so EnArrow_Shoot fires it; unk_D57 is already 4 this frame.
            Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ARROW, cp.x, cp.y + 7.0f, cp.z,
                        (arrowType == ARROW_TYPE_DEKU_NUT) ? 0x1000 : 0, player->actor.shape.rot.y, 0, arrowType);
        }
        gExtEquipBehavior.fourSwordItemCooldown = FS_ITEM_COOLDOWN;
    }
skip_arrow :

{
    u8 curCarrying = (player->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) ? 1 : 0;
    if (gExtEquipBehavior.fourSwordPrevCarrying && !curCarrying) {
        for (u8 i = 0; i < FSC_MAX; i++) {
            Vec3f cp;
            if (!FourSwordClone_GetPos(i, &cp)) {
                continue;
            }
            Actor_Spawn(&play->actorCtx, play, ACTOR_EN_BOM, cp.x, cp.y + 7.0f, cp.z, 0, 0, 0, 0);
        }
        gExtEquipBehavior.fourSwordItemCooldown = FS_ITEM_COOLDOWN;
    }
}

    {
        u8 curBoom = (player->zoraBoomerangActor != NULL) ? 1 : 0;
        if (curBoom && !gExtEquipBehavior.fourSwordPrevBoomerang) {
            for (u8 i = 0; i < FSC_MAX; i++) {
                Vec3f cp;
                if (!FourSwordClone_GetPos(i, &cp)) {
                    continue;
                }
                f32 px = Math_SinS(player->actor.shape.rot.y) * 1.0f + cp.x;
                f32 pz = Math_CosS(player->actor.shape.rot.y) * 1.0f + cp.z;
                EnBoom* boom =
                    (EnBoom*)Actor_Spawn(&play->actorCtx, play, ACTOR_EN_BOOM, px, cp.y + 7.0f, pz,
                                         player->actor.focus.rot.x, player->actor.shape.rot.y, 0, OOT_BOOMERANG);
                if (boom != NULL) {
                    // MM's En_Boom starts returning at unk_1CC <= 16, so OoT's 20 outbound frames
                    // are 20 + 16 (item_oot_boomerang.c).
                    boom->unk_1CC = 36;
                }
            }
            gExtEquipBehavior.fourSwordItemCooldown = FS_ITEM_COOLDOWN;
        }
    }

update_prev:
    gExtEquipBehavior.fourSwordPrevFireTimer = player->unk_D57;
    gExtEquipBehavior.fourSwordPrevCarrying = (player->stateFlags1 & PLAYER_STATE1_CARRYING_ACTOR) ? 1 : 0;
    gExtEquipBehavior.fourSwordPrevBoomerang = (player->zoraBoomerangActor != NULL) ? 1 : 0;
}

// Held-sword model, queried from the L_HAND limb override in z_player_lib.c. Returns 1 and fills
// blade/handle while the Four Sword is equipped and its resources resolved.
u8 FourSword_HeldSwordDLForFrame(void** blade, void** handle, int frame) {
    if (FourSword_IsEquipped() && (NeiEquipment_IsLegacyMod(NEI_EQUIPMENT_FOUR_BLADE, FOURSWORD_BLADE_DL) ||
                                   NeiEquipment_IsLegacyMod(NEI_EQUIPMENT_FOUR_HILT, FOURSWORD_HILT_DL))) {
        // Keep the entire legacy pair if either half belongs to a selected mod.
        // The base counterpart is loaded through its canonical path as well.
        static Gfx modPair[2][2];
        const char* paths[2] = { FOURSWORD_BLADE_DL, FOURSWORD_HILT_DL };
        for (int i = 0; i < 2; ++i) {
            if (!ResourceMgr_FileExists(paths[i]) &&
                !(ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(paths[i]))
#ifdef NEI_EQUIPMENT_MM
                && !NeiResource_Available(paths[i])
#endif
            )
                return 0;
#ifdef NEI_EQUIPMENT_MM
            // MM's existing legacy pair is local; an OoT-only donor mod is explicitly routed.
            const u8 localAvailable = ResourceMgr_FileExists(paths[i]) ||
                                      (ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(paths[i]));
            const char* route = (!ResourceMgr_IsModAsset(paths[i]) && (NeiResource_IsMod(paths[i]) || !localAvailable))
                                    ? NeiResource_Route(paths[i])
                                    : paths[i];
#else
            const char* route = paths[i];
#endif
            gDma1p(&modPair[i][0], G_DL_OTR_FILEPATH, route, 0, G_DL_PUSH);
            gSPEndDisplayList(&modPair[i][1]);
        }
        *blade = modPair[0];
        *handle = modPair[1];
        return 1;
    }

    if (FourSword_IsEquipped()) {
        Gfx* exactSword = NeiHeldSword_ModelDL(NEI_HELD_SWORD_FOUR, frame);
        if (exactSword != NULL) {
            *blade = exactSword;
            *handle = NULL;
            return 1;
        }
    }
    if (FourSword_IsEquipped() && !NeiHeldSword_UseVanilla() && NeiEquipment_HasFourSword()) {
        Gfx* authoredBlade = NeiEquipment_ModelDL(NEI_EQUIPMENT_FOUR_BLADE, FOURSWORD_BLADE_DL);
        Gfx* authoredHilt = NeiEquipment_ModelDL(NEI_EQUIPMENT_FOUR_HILT, FOURSWORD_HILT_DL);
        if (authoredBlade != NULL && authoredHilt != NULL) {
            *blade = authoredBlade;
            *handle = authoredHilt;
            return 1;
        }
    }
    static void* sBlade = NULL;
    static void* sHilt = NULL;
    static u8 sTried = 0;

    if (!FourSword_IsEquipped()) {
        return 0;
    }
    if (!sTried) {
        sTried = 1;
        // ResourceMgr_LoadGfxByName crashes on a path that isn't in the archive, so gate on
        // FileExists first — same as Byrna_GetCaneDL.
        if (ResourceMgr_FileExists(FOURSWORD_BLADE_DL)) {
            sBlade = ResourceMgr_LoadGfxByName(FOURSWORD_BLADE_DL);
        }
        if (ResourceMgr_FileExists(FOURSWORD_HILT_DL)) {
            sHilt = ResourceMgr_LoadGfxByName(FOURSWORD_HILT_DL);
        }
    }
    if (sBlade == NULL) {
        return 0; // stale 2ship.o2r → keep the vanilla sword rather than nothing
    }
    *blade = sBlade;
    *handle = sHilt;
    return 1;
}

// Native callers stay in MM's human frame, including peers while the local
// Time Gate uses its adult overlay. The held handler supplies the actual frame.
u8 FourSword_HeldSwordDL(void** blade, void** handle) {
    return FourSword_HeldSwordDLForFrame(blade, handle, NEI_HELD_SWORD_MM_HUMAN);
}

// The sword action rides on ITEM_EXT_SWORD_2 itself (ExtEquip_SetSlot puts it on B and
// ExtPlayer_GetItemAction aliases it to the one-hand sword action). Nothing here touches the
// equipment nibble or the save.
static void FourSword_Behavior(Player* player, PlayState* play) {
    if (player->stateFlags1 & (PLAYER_STATE1_DEAD | PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_LOADING |
                               PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_GETTING_ITEM)) {
        return;
    }

    u8 isShielding = (player->stateFlags1 & PLAYER_STATE1_SHIELDING) ? 1 : 0;
    u8 bHeld = CHECK_BTN_ALL(play->state.input[0].cur.button, BTN_B) ? 1 : 0;

    if (isShielding && bHeld) {
        if (!gExtEquipBehavior.fourSwordCharging) {
            gExtEquipBehavior.fourSwordBHoldTimer++;
        }

        if (!gExtEquipBehavior.fourSwordCharging && gExtEquipBehavior.fourSwordBHoldTimer >= FS_CHARGE_HOLD) {
            gExtEquipBehavior.fourSwordCharging = 1;
            Sfx_PlaySfxCentered(NA_SE_SY_ATTENTION_ON);

            for (u8 i = 0; i < FSC_MAX; i++) {
                FourSwordClone_Spawn(play, player, i);
            }
        }
    } else {
        gExtEquipBehavior.fourSwordBHoldTimer = 0;
        gExtEquipBehavior.fourSwordCharging = 0;
    }

    if (gExtEquipBehavior.fourSwordCharging) {
        FourSword_ApplyChargeAnim(player, play);
    }

    FourSwordClone_Reconcile(play, player);
    FourSword_SpawnCloneProjectiles(player, play);
}

static void FourSword_Cleanup(void) {
    FourSwordClone_KillAll();
    gExtEquipBehavior.fourSwordCharging = 0;
    gExtEquipBehavior.fourSwordBHoldTimer = 0;
    gExtEquipBehavior.fourSwordItemCooldown = 0;
    gExtEquipBehavior.fourSwordPrevFireTimer = 0;
    gExtEquipBehavior.fourSwordPrevCarrying = 0;
    gExtEquipBehavior.fourSwordPrevBoomerang = 0;
}
