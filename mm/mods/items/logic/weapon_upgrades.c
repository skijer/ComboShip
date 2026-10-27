/**
 * weapon_upgrades.c - NEI Weapon Upgrade bit accessors.
 *
 * This translation unit just owns the Nei_Save()->weaponUpgrades bit
 * accessors so other code (randomizer give/logic, the menu, the IK Axe hammer
 * behavior) doesn't need to know the field layout.
 *
 * #included into mods/items/logic/custom_items.c (the host TU pulled in by
 * z_player.c) — the CMake mods glob only compiles *.cpp/*.h, not *.c.
 */
#include "weapon_upgrades.h"
#include "../../nei_save.h" // Skijer's NEI
#include "../../extended_equipment.h"
#include "../../equipment/nei_equipment_presentation.h"
#include "../../../../combo/NeiHeldSword.h"
#include "din_fire_sword.h"
#include "adult_link_render.h"

u8 WeaponUpgrade_HasHammerAxe(void) {
    return (Nei_Save()->weaponUpgrades & WEAPON_UPGRADE_HAMMER_AXE) != 0;
}

u8 WeaponUpgrade_HasRazor(void) {
    return (Nei_Save()->weaponUpgrades & WEAPON_UPGRADE_KOKIRI_RAZOR) != 0;
}

u8 WeaponUpgrade_HasGilded(void) {
    return (Nei_Save()->weaponUpgrades & WEAPON_UPGRADE_KOKIRI_GILDED) != 0;
}

u8 WeaponUpgrade_HasTrueMaster(void) {
    return (Nei_Save()->weaponUpgrades & WEAPON_UPGRADE_MASTER_TRUE) != 0;
}

u8 WeaponUpgrade_HasGreatFairy(void) {
    return (Nei_Save()->weaponUpgrades & WEAPON_UPGRADE_BGS_GREAT_FAIRY) != 0;
}

u8 WeaponUpgrade_KokiriLevel(void) {
    if (Nei_Save()->weaponUpgrades & WEAPON_UPGRADE_KOKIRI_GILDED) {
        return 2;
    }
    if (Nei_Save()->weaponUpgrades & WEAPON_UPGRADE_KOKIRI_RAZOR) {
        return 1;
    }
    return 0;
}

static void WeaponUpgrade_SetBit(u8 bit, u8 on) {
    if (on) {
        Nei_Save()->weaponUpgrades |= bit;
    } else {
        Nei_Save()->weaponUpgrades &= ~bit;
    }
}

void WeaponUpgrade_SetHammerAxe(u8 on) {
    WeaponUpgrade_SetBit(WEAPON_UPGRADE_HAMMER_AXE, on);
}

void WeaponUpgrade_SetRazor(u8 on) {
    WeaponUpgrade_SetBit(WEAPON_UPGRADE_KOKIRI_RAZOR, on);
}

void WeaponUpgrade_SetGilded(u8 on) {
    WeaponUpgrade_SetBit(WEAPON_UPGRADE_KOKIRI_GILDED, on);
}

void WeaponUpgrade_SetTrueMaster(u8 on) {
    WeaponUpgrade_SetBit(WEAPON_UPGRADE_MASTER_TRUE, on);
}

void WeaponUpgrade_SetGreatFairy(u8 on) {
    WeaponUpgrade_SetBit(WEAPON_UPGRADE_BGS_GREAT_FAIRY, on);
}

void WeaponUpgrade_GiveProgressiveKokiri(void) {
    // First give → Razor, second give → Gilded. Gilded implies Razor was earned.
    if (!(Nei_Save()->weaponUpgrades & WEAPON_UPGRADE_KOKIRI_RAZOR)) {
        Nei_Save()->weaponUpgrades |= WEAPON_UPGRADE_KOKIRI_RAZOR;
    } else {
        Nei_Save()->weaponUpgrades |= WEAPON_UPGRADE_KOKIRI_GILDED;
    }
}

void WeaponUpgrade_GrantAll(void) {
    Nei_Save()->weaponUpgrades |= WEAPON_UPGRADE_ALL;
}

// ---------------------------------------------------------------------------
// Iron Knuckle's Axe prop-smash side table
//
// Lets the Axe (hammer upgrade) break gauntlet-tier props after N swings, with no per-actor
// struct changes: hit counts live in a small static table keyed by Actor*. A "swing" is counted
// at most once via a global swing id (rising edge of the player's melee state), so holding the
// hammer out doesn't rack up hits — only an actual swing in range/facing does.
// ---------------------------------------------------------------------------
#define IKAXE_PROP_SLOTS 12

typedef struct {
    Actor* actor;
    u8 hits;
    u32 lastSwing;
} IKAxePropState;

static IKAxePropState sIKAxeProps[IKAXE_PROP_SLOTS];
static u32 sIKAxeSwingId = 0;

static void IKAxe_UpdateSwingId(Player* player, PlayState* play) {
    static s32 sLastFrame = -1;
    static u8 sPrevSwinging = 0;
    if ((s32)play->gameplayFrames == sLastFrame) {
        return; // already advanced this frame (this helper is called once per prop actor)
    }
    sLastFrame = (s32)play->gameplayFrames;
    u8 swinging = (player->meleeWeaponState != 0);
    if (swinging && !sPrevSwinging) {
        sIKAxeSwingId++; // new swing
    }
    sPrevSwinging = swinging;
}

static IKAxePropState* IKAxe_PropSlot(Actor* actor) {
    s32 i;
    for (i = 0; i < IKAXE_PROP_SLOTS; i++) {
        if (sIKAxeProps[i].actor == actor) {
            return &sIKAxeProps[i];
        }
    }
    for (i = 0; i < IKAXE_PROP_SLOTS; i++) {
        if (sIKAxeProps[i].actor == NULL) {
            sIKAxeProps[i].actor = actor;
            sIKAxeProps[i].hits = 0;
            sIKAxeProps[i].lastSwing = 0;
            return &sIKAxeProps[i];
        }
    }
    return NULL;
}

// Exact reviewed GI geometry in the native human/adult hand frame. The donor
// mesh graph is atomic; all modelview changes live in balanced resource pushes.
u8 WeaponUpgrade_ApplyHeldSwordDL(Gfx** dList, void* ootHand, Player* player, u8 bodyEnvR, u8 bodyEnvG, u8 bodyEnvB) {
    extern void* MmAssets_LoadResource(const char* path);
    extern s32 CVarGetInteger(const char* name, s32 defaultValue);
    extern u8 Player_IsCustomLinkModel(Player * player);
    extern u8 FourSword_IsEquipped(void);
    // Cached loaded pieces per variant.
    static void* sRazorBlade = NULL;
    static void* sRazorHandle = NULL;
    static void* sGildedBlade = NULL;
    static void* sGildedHandle = NULL;
    static void* sGfs = NULL;
    static u8 sRazorTried = 0, sGildedTried = 0, sGfsTried = 0;
    static Gfx sCompound[8];

    if (dList == NULL || ootHand == NULL || player == NULL) {
        return 0;
    }
    const u8 fourEquipped = FourSword_IsEquipped();
    if (player->transformation != PLAYER_FORM_HUMAN || (!fourEquipped && Player_IsCustomLinkModel(player)) ||
        (player->leftHandType != PLAYER_MODELTYPE_LH_ONE_HAND_SWORD &&
         player->leftHandType != PLAYER_MODELTYPE_LH_TWO_HAND_SWORD)) {
        return 0;
    }

    void* blade = NULL;
    void* handle = NULL;
    const int frame = AdultLink_UsesAdultPresentation(player) ? NEI_HELD_SWORD_OOT_ADULT : NEI_HELD_SWORD_MM_HUMAN;
    int model = -1;
    int auraModel = -1;
    if (player->heldItemId == ITEM_SWORD_KOKIRI && player->heldItemAction == PLAYER_IA_SWORD_KOKIRI) {
        model = WeaponUpgrade_KokiriLevel() == 0 ? NEI_HELD_SWORD_MM_KOKIRI
                : WeaponUpgrade_HasGilded() && CVarGetInteger("gEnhancements.SkijerNEI.GildedUsesGildedLook", 1)
                    ? NEI_HELD_SWORD_GILDED
                    : NEI_HELD_SWORD_RAZOR;
    } else if (player->heldItemId == ITEM_SWORD_RAZOR && player->heldItemAction == PLAYER_IA_SWORD_RAZOR) {
        model = NEI_HELD_SWORD_RAZOR;
    } else if (player->heldItemId == ITEM_SWORD_GILDED && player->heldItemAction == PLAYER_IA_SWORD_GILDED) {
        model = NEI_HELD_SWORD_GILDED;
    } else if (player->heldItemId == ITEM_SWORD_MASTER && player->heldItemAction == PLAYER_IA_SWORD_MASTER) {
        model = WeaponUpgrade_HasTrueMaster() ? NEI_HELD_SWORD_TRUE_MASTER : NEI_HELD_SWORD_MASTER;
    } else if (player->heldItemId == ITEM_SWORD_BGS && player->heldItemAction == PLAYER_IA_SWORD_BIGGORON) {
        model = WeaponUpgrade_HasGreatFairy() && CVarGetInteger("gEnhancements.SkijerNEI.BgsUsesGfsLook", 1)
                    ? NEI_HELD_SWORD_GREAT_FAIRY
                    : NEI_HELD_SWORD_BIGGORON;
    } else if (player->heldItemId == ITEM_SWORD_GREAT_FAIRY && player->heldItemAction == PLAYER_IA_SWORD_TWO_HANDED) {
        model = NEI_HELD_SWORD_GREAT_FAIRY;
    } else if (FourSword_IsEquipped() && player->heldItemId == ITEM_EXT_SWORD_2 &&
               player->heldItemAction == PLAYER_IA_SWORD_KOKIRI) {
        model = NEI_HELD_SWORD_FOUR;
    }
    extern u8 FourSword_HeldSwordDLForFrame(void** blade, void** handle, int frame);
    if (model < 0)
        return 0; // Custom items can reuse sword actions (net, rods, etc.).
    if (!fourEquipped && model < NEI_HELD_SWORD_FOUR)
        auraModel = model;
    // A selected native/Din sword does not own the separate Four Sword item.
    // Its getter still preserves a mod of the actual Four Sword blade/hilt.
    if (!fourEquipped && model >= 0 && NeiHeldSword_EquipmentSelected(model, frame))
        return 0;
    if (!fourEquipped && frame == NEI_HELD_SWORD_MM_HUMAN &&
        player->leftHandType == PLAYER_MODELTYPE_LH_ONE_HAND_SWORD) {
        extern u16 gEquipMasks[];
        extern u8 gEquipShifts[];
        // Native MM chooses its combined sword hand from the equipped nibble,
        // even when NEI's progressive blade advances beyond that native tier.
        const int equipped = GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD);
        const int nativeModel = equipped == EQUIP_VALUE_SWORD_RAZOR    ? NEI_HELD_SWORD_RAZOR
                                : equipped == EQUIP_VALUE_SWORD_GILDED ? NEI_HELD_SWORD_GILDED
                                                                       : NEI_HELD_SWORD_MM_KOKIRI;
        if (NeiHeldSword_EquipmentSelected(nativeModel, frame))
            return 0;
    } else if (!fourEquipped && frame == NEI_HELD_SWORD_MM_HUMAN &&
               player->leftHandType == PLAYER_MODELTYPE_LH_TWO_HAND_SWORD &&
               NeiHeldSword_EquipmentSelected(NEI_HELD_SWORD_GREAT_FAIRY, frame)) {
        // MM's native two-hand array always owns the GFS combined hand, even
        // when the imported longsword's selected visual tier is Biggoron.
        return 0;
    }

    if (FourSword_HeldSwordDLForFrame(&blade, &handle, frame)) {
        // The Four Sword pair getter keeps a selected legacy pair authoritative.
    } else if (model >= 0 && !NeiHeldSword_UpgradePiecesSelected(model) &&
               (blade = NeiHeldSword_ModelDL(model, frame)) != NULL) {
        // Whole exact authored blade+hilt, without get-item particles/shimmer.
    } else if (player->heldItemAction == PLAYER_IA_SWORD_KOKIRI && WeaponUpgrade_KokiriLevel() >= 1) {
        u8 gilded = WeaponUpgrade_HasGilded() && CVarGetInteger("gEnhancements.SkijerNEI.GildedUsesGildedLook", 1);
        auraModel = gilded ? SWORD_AURA_NATIVE_MM_GILDED : SWORD_AURA_NATIVE_MM_RAZOR;
        if (gilded) {
            if (!sGildedTried) {
                sGildedTried = 1;
                sGildedBlade = MmAssets_LoadResource("__OTR__objects/object_link_child/gLinkHumanGildedSwordBladeDL");
                sGildedHandle = MmAssets_LoadResource("__OTR__objects/object_link_child/gLinkHumanGildedSwordHandleDL");
            }
            blade = sGildedBlade;
            handle = sGildedHandle;
        } else {
            if (!sRazorTried) {
                sRazorTried = 1;
                sRazorBlade = MmAssets_LoadResource("__OTR__objects/gameplay_keep/gRazorSwordBladeDL");
                sRazorHandle = MmAssets_LoadResource("__OTR__objects/gameplay_keep/gRazorSwordHandleDL");
            }
            blade = sRazorBlade;
            handle = sRazorHandle;
        }
    } else if (player->heldItemAction == PLAYER_IA_SWORD_BIGGORON && WeaponUpgrade_HasGreatFairy() &&
               CVarGetInteger("gEnhancements.SkijerNEI.BgsUsesGfsLook", 1)) {
        auraModel = SWORD_AURA_NATIVE_MM_FAIRY;
        if (!sGfsTried) {
            sGfsTried = 1;
            sGfs = MmAssets_LoadResource("__OTR__objects/object_link_child/gLinkHumanGreatFairysSwordDL");
        }
        blade = sGfs; // single combined blade+hilt DL
        handle = NULL;
    } else {
        return 0;
    }

    if (blade == NULL) {
        return 0; // asset unavailable → keep vanilla DL
    }

    // Build [MM blade] + [MM handle?] + [OOT hand] + [end] — sword first, hand LAST. The hand DL
    // restores the vanilla skin material/render state, so the next limb (the torso) doesn't inherit
    // the MM sword's combiner/env and render black. Matches "sword luego la mano".
    Gfx* d = sCompound;
    gSPDisplayList(d++, (Gfx*)blade);
    if (handle != NULL) {
        gSPDisplayList(d++, (Gfx*)handle);
    }
    gSPDisplayList(d++, (Gfx*)ootHand);
    // Restore the standard player-limb render state so the next limb (the torso) doesn't inherit
    // the MM sword's material and render black.
    gDPPipeSync(d++);
    // Re-apply the tunic env color. The player body tints with env (set once at z_player_lib.c:1181),
    // and a combined MM DL like the Great Fairy's Sword sets its OWN env color and never restores it —
    // without this the torso inherits the GFS env and the chest renders black.
    gDPSetEnvColor(d++, bodyEnvR, bodyEnvG, bodyEnvB, 0);
    gSPLoadGeometryMode(d++, G_ZBUFFER | G_SHADE | G_CULL_BACK | G_LIGHTING | G_SHADING_SMOOTH);
    gSPEndDisplayList(d);
    *dList = sCompound;
    DinFireSword_SetHandModel(player, sCompound, auraModel);
    return 1;
}

u8 WeaponUpgrade_IKAxeStrike(Actor* actor, PlayState* play, u8 hitsNeeded, f32 range) {
    // The Power Keg's explosion one-shots these same gauntlet-tier props, bypassing the hammer-swing
    // requirement (it's a blast, not a swing). PowerKeg_BlastQuery lives in power_keg.c — same unity
    // TU, #included later, so a local extern is enough. Skijer's NEI
    extern u8 PowerKeg_BlastQuery(Actor * actor, PlayState * play);
    if ((actor != NULL) && (play != NULL) && PowerKeg_BlastQuery(actor, play)) {
        return 1;
    }
    if (!WeaponUpgrade_HasHammerAxe() || actor == NULL || play == NULL) {
        return 0;
    }
    Player* player = GET_PLAYER(play);
    IKAxe_UpdateSwingId(player, play);

    // Must be actively swinging the hammer (the Axe).
    if (player->heldItemAction != PLAYER_IA_HAMMER || player->meleeWeaponState == 0) {
        return 0;
    }
    // In reach and roughly in front of the player.
    if (Math_Vec3f_DistXYZ(&player->actor.world.pos, &actor->world.pos) > range) {
        return 0;
    }
    s16 yawToActor = Actor_WorldYawTowardActor(&player->actor, actor);
    if (ABS((s16)(yawToActor - player->actor.shape.rot.y)) > 0x3800) {
        return 0;
    }

    IKAxePropState* slot = IKAxe_PropSlot(actor);
    if (slot == NULL || slot->lastSwing == sIKAxeSwingId) {
        return 0; // table full, or this swing already counted for this actor
    }
    slot->lastSwing = sIKAxeSwingId;
    slot->hits++;

    Audio_PlaySoundGeneral(NA_SE_IT_HAMMER_HIT, &actor->world.pos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    CollisionCheck_SpawnShieldParticlesMetal(play, &actor->world.pos);

    if (slot->hits >= hitsNeeded) {
        slot->actor = NULL; // free the slot
        return 1;
    }
    return 0;
}
