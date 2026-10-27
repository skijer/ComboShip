// Exercise the production MM attack setter, collision resolution, actor tables,
// drops and ice gates. din_fire_native.inc contains unchanged native functions
// extracted at build time; enemy tables come from their actor translation units.
#include "din_fire_fixture.h"
#include <stdarg.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "mods/combo_rpg.h"
#include "mods/extended_player.h"
#include "overlays/actors/ovl_Boss_Hakugin/z_boss_hakugin.h"
#include "overlays/actors/ovl_Obj_Ice_Poly/z_obj_ice_poly.h"
#include "overlays/actors/ovl_Obj_Aqua/z_obj_aqua.h"
#include "overlays/actors/ovl_Obj_Mine/z_obj_mine.h"

extern void Test_SetSoulState(int enabled, int owned);
extern int Test_SoulGate(Collider* at, Collider* ac);

bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t result, ...) {
    if (behavior == VB_PERFORM_AC_COLLISION) {
        va_list args;
        va_start(args, result);
        Collider* at = va_arg(args, Collider*);
        Collider* ac = va_arg(args, Collider*);
        result = Test_SoulGate(at, ac);
        va_end(args);
    }
    return result;
}
void CollisionCheck_HitEffects(PlayState* state, Collider* at, ColliderElement* atElem, Collider* ac,
                               ColliderElement* acElem, Vec3f* hitPos) {
    (void)state;
    (void)at;
    (void)atElem;
    (void)ac;
    (void)acElem;
    (void)hitPos;
}
void AudioSfx_StopByPosAndId(Vec3f* position, u16 sound) {
}
static unsigned fixedSoundRequests;
static Vec3f fixedSoundPosition;
static u16 fixedSound;
void SoundSource_PlaySfxAtFixedWorldPos(PlayState* state, Vec3f* position, u32 duration, u16 sound) {
    REQUIRE(state == &play && position != NULL);
    REQUIRE(duration == 24);
    REQUIRE(sound == NA_SE_IT_EXPLOSION_ICE || sound == NA_SE_EV_BURN_OUT);
    ++fixedSoundRequests;
    fixedSoundPosition = *position; // The native sound source copies the world position.
    fixedSound = sound;
}
u8 TridentChargeBall_GetFierceDamage(Actor* actor) {
    (void)actor;
    return 0;
}
u8 gIvanPossessActive;
u8 Sm64Mario_IsReady(void) {
    return 0;
}
int ComboRpg_IsEnabled(int stat) {
    (void)stat;
    return 0;
}
uint8_t ComboRpg_ApplyPower(uint8_t damage, float roll) {
    (void)roll;
    return damage;
}
f32 Rand_ZeroOne(void) {
    return 0.5f;
}

static int gohtThawed, queuedCutscenes;
Vec3f gZeroVec3f;
static Color_RGBA8 D_80932378, D_8093237C;
void BossHakugin_SetupCutsceneStart(BossHakugin* boss) {
    (void)boss;
    ++gohtThawed;
}
void func_80931E58(ObjIcePoly* ice, PlayState* state) {
    (void)ice;
    (void)state;
}
s32 CollisionCheck_SetAT(PlayState* state, CollisionCheckContext* context, Collider* collider) {
    (void)state;
    (void)context;
    (void)collider;
    return 1;
}
s32 CollisionCheck_SetAC(PlayState* state, CollisionCheckContext* context, Collider* collider) {
    (void)state;
    (void)context;
    (void)collider;
    return 1;
}
s32 CollisionCheck_SetOC(PlayState* state, CollisionCheckContext* context, Collider* collider) {
    (void)state;
    (void)context;
    (void)collider;
    return 1;
}
void CutsceneManager_Queue(s16 id) {
    (void)id;
    ++queuedCutscenes;
}
s32 Flags_GetSwitch(PlayState* state, s32 flag) {
    (void)state;
    (void)flag;
    return 0;
}
Actor* SubS_FindActor(PlayState* state, Actor* actor, u8 category, s16 id) {
    (void)state;
    (void)actor;
    (void)category;
    (void)id;
    return NULL;
}
void EffectSsKirakira_SpawnDispersed(PlayState* state, Vec3f* position, Vec3f* velocity, Vec3f* acceleration,
                                     Color_RGBA8* primary, Color_RGBA8* environment, s16 scale, s32 life) {
    (void)state;
    (void)position;
    (void)velocity;
    (void)acceleration;
    (void)primary;
    (void)environment;
    (void)scale;
    (void)life;
}

#include "din_fire_native.inc"

extern DamageTable* Test_ChuchuTable(void);
extern DamageTable* Test_DekuBabaTable(void);
extern DamageTable* Test_WolfosTable(void);
extern DamageTable* Test_WhiteWolfosTable(void);
extern DamageTable* Test_ArmosTable(void);
extern DamageTable* Test_KeeseTable(void);
extern DamageTable* Test_DinolfosTable(void);
extern DamageTable* Test_RedeadTable(void);
extern DamageTable* Test_FreezardTable(void);
extern DamageTable* Test_WallmasterTable(void);
extern DamageTable* Test_FloormasterTable(void);
extern DamageTable* Test_CrowTable(void);
extern DamageTable* Test_DekuNutsTable(void);
extern DamageTable* Test_PeahatTable(void);
extern u32 Test_KusaMask(void);
extern u32 Test_GrassMask(void);

static void SetupDamage(void) {
    Setup();
    swordOption = damageOption = 1;
    for (int i = 0; i < 2; ++i) {
        player.meleeWeaponQuads[i].base.actor = &player.actor;
        player.meleeWeaponQuads[i].base.atFlags = AT_ON | AT_TYPE_PLAYER;
    }
}

static void Strike(int weapon, int strong) {
    switch (weapon) {
        case 0:
            player.heldItemId = ITEM_SWORD_KOKIRI;
            player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
            player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
            break;
        case 1:
            player.heldItemId = ITEM_SWORD_MASTER;
            player.heldItemAction = PLAYER_IA_SWORD_MASTER;
            player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
            break;
        case 2:
            player.heldItemId = ITEM_SWORD_BGS;
            player.heldItemAction = PLAYER_IA_SWORD_TWO_HANDED;
            player.leftHandType = PLAYER_MODELTYPE_LH_TWO_HAND_SWORD;
            break;
    }
    func_8083375C(&player, strong ? PLAYER_MWA_JUMPSLASH_START : PLAYER_MWA_FORWARD_SLASH_1H);
}

static void AcceptedIceAudio(void) {
    SetupDamage();
    elementalSelection = elementalActive = 2;
    Strike(0, 0);
    ColliderElement* attack = &player.meleeWeaponQuads[0].elem;
    Actor target = { 0 };
    target.id = ACTOR_EN_WF;
    target.category = ACTORCAT_ENEMY;
    target.world.pos = (Vec3f){ 10, 20, 30 };
    target.colChkInfo.health = 8;
    target.colChkInfo.damageTable = Test_WolfosTable();
    ColliderCylinder receiver = { 0 };
    receiver.base.actor = &target;
    receiver.base.acFlags = AC_HIT;
    receiver.base.shape = COLSHAPE_CYLINDER;
    receiver.dim = (Cylinder16){ 20, 60, 0, { 10, 20, 30 } };
    receiver.elem.acElemFlags = ACELEM_HIT;
    receiver.elem.acDmgInfo.dmgFlags = 0xFFFFFFFF;
    receiver.elem.acHit = &player.meleeWeaponQuads[0].base;
    receiver.elem.acHitElem = attack;
    REQUIRE(!CollisionCheck_NoSharedFlags(attack, &receiver.elem));
    fixedSoundRequests = 0;
    CollisionCheck_ApplyDamage(&play, &play.colChkCtx, &receiver.base, &receiver.elem);
    REQUIRE(target.colChkInfo.damage == 1 && target.colChkInfo.damageEffect == 3 && target.colChkInfo.health == 8);
    REQUIRE(fixedSoundRequests == 1 && fixedSound == NA_SE_IT_EXPLOSION_ICE);
    REQUIRE(fixedSoundPosition.x == 10 && fixedSoundPosition.y == 20 && fixedSoundPosition.z == 30);
    CollisionCheck_ApplyDamage(&play, &play.colChkCtx, &receiver.base, &receiver.elem);
    REQUIRE(target.colChkInfo.damage == 1 && fixedSoundRequests == 1);
    DinFireSword_FinishTeardown(&play);
}

static Actor RequireHit(s16 actorId, DamageTable* table, ColliderElement* attack, u32 receiverFlags, int damage,
                        int effect) {
    Actor target = { 0 };
    Collider receiver = { 0 };
    ColliderElement element = { 0 };
    target.id = actorId;
    target.category = ACTORCAT_ENEMY;
    target.colChkInfo.damageTable = table;
    receiver.actor = &target;
    receiver.acFlags = AC_HIT;
    element.acElemFlags = ACELEM_HIT;
    element.acDmgInfo.dmgFlags = receiverFlags;
    element.acHit = &player.meleeWeaponQuads[0].base;
    element.acHitElem = attack;
    REQUIRE(!CollisionCheck_NoSharedFlags(attack, &element));
    CollisionCheck_ApplyDamage(&play, &play.colChkCtx, &receiver, &element);
    if (target.colChkInfo.damage != damage || target.colChkInfo.damageEffect != effect) {
        fprintf(stderr, "actor %d: damage/effect %d/%d, expected %d/%d\n", actorId, target.colChkInfo.damage,
                target.colChkInfo.damageEffect, damage, effect);
    }
    REQUIRE(target.colChkInfo.damage == damage);
    REQUIRE(target.colChkInfo.damageEffect == effect);
    return target;
}

static void NativeDamageAndReactions(void) {
    static const int normalDamage[] = { 1, 3, 4 };
    struct {
        s16 actor;
        DamageTable* (*table)(void);
        int swordEffect, fireSwordEffect;
    } targets[] = {
        { ACTOR_EN_DEKUBABA, Test_DekuBabaTable, 15, 15 }, { ACTOR_EN_WF, Test_WolfosTable, 0, 2 },
        { ACTOR_EN_WF, Test_WhiteWolfosTable, 0, 2 },      { ACTOR_EN_AM, Test_ArmosTable, 0, 0 },
        { ACTOR_EN_FIREFLY, Test_KeeseTable, 0, 2 },       { ACTOR_EN_DINOFOS, Test_DinolfosTable, 0, 0 },
        { ACTOR_EN_RD, Test_RedeadTable, 15, 15 },         { ACTOR_EN_FZ, Test_FreezardTable, 15, 15 },
        { ACTOR_EN_WALLMAS, Test_WallmasterTable, 0, 2 },  { ACTOR_EN_FLOORMAS, Test_FloormasterTable, 0, 2 },
        { ACTOR_EN_CROW, Test_CrowTable, 0, 2 },           { ACTOR_EN_DEKUNUTS, Test_DekuNutsTable, 0, 2 },
        { ACTOR_EN_PEEHAT, Test_PeahatTable, 0, 2 },
    };
    for (int weapon = 0; weapon < 3; ++weapon) {
        for (int strong = 0; strong < 2; ++strong) {
            for (int enabled = 0; enabled < 2; ++enabled) {
                SetupDamage();
                adult = weapon == 1;
                damageOption = enabled;
                Strike(weapon, strong);
                const int damage = normalDamage[weapon] * (strong ? 2 : 1);
                for (int quad = 0; quad < 2; ++quad) {
                    ColliderElement* attack = &player.meleeWeaponQuads[quad].elem;
                    REQUIRE(attack->atDmgInfo.damage == damage);
                    REQUIRE(attack->atDmgInfo.dmgFlags == (DMG_SWORD | (enabled ? DMG_FIRE_ARROW : 0)));
                    REQUIRE(attack->atElemFlags == (ATELEM_ON | ATELEM_NEAREST));
                    for (size_t i = 0; i < ARRAY_COUNT(targets); ++i) {
                        RequireHit(targets[i].actor, targets[i].table(), attack, 0xFFFFFFFF, damage,
                                   enabled ? targets[i].fireSwordEffect : targets[i].swordEffect);
                        // A sword-only receiver keeps its ordinary reaction.
                        RequireHit(targets[i].actor, targets[i].table(), attack, DMG_SWORD, damage,
                                   targets[i].swordEffect);
                    }
                    if (enabled) {
                        RequireHit(ACTOR_EN_FZ, Test_FreezardTable(), attack, DMG_FIRE_ARROW, damage * 2, 2);
                    }
                }
            }
        }
    }
    // Actual projectiles retain MM's fire multipliers; an unowned combined
    // collider must not inherit the local player's damage normalization.
    ColliderElement projectile = { 0 };
    projectile.atDmgInfo.damage = 4;
    projectile.atDmgInfo.dmgFlags = DMG_FIRE_ARROW;
    RequireHit(ACTOR_EN_WF, Test_WolfosTable(), &projectile, 0xFFFFFFFF, 8, 2);
    projectile.atDmgInfo.dmgFlags |= DMG_SWORD;
    RequireHit(ACTOR_EN_WF, Test_WolfosTable(), &projectile, 0xFFFFFFFF, 8, 2);
}

static void GrassAndOwnership(void) {
    SetupDamage();
    Strike(0, 0);
    ColliderElement receiver = { 0 };
    const u32 masks[] = { Test_KusaMask(), Test_GrassMask() };
    for (size_t i = 0; i < ARRAY_COUNT(masks); ++i) {
        receiver.acDmgInfo.dmgFlags = masks[i];
        REQUIRE(!CollisionCheck_NoSharedFlags(&player.meleeWeaponQuads[0].elem, &receiver));
        ColliderElement fireArrow = { 0 };
        fireArrow.atDmgInfo.dmgFlags = DMG_FIRE_ARROW;
        REQUIRE(CollisionCheck_NoSharedFlags(&fireArrow, &receiver));
    }
    for (int guard = 0; guard < 11; ++guard) {
        SetupDamage();
        switch (guard) {
            case 0:
                swordOption = -1;
                break;
            case 1:
                damageOption = -1;
                break;
            case 2:
                alt = 0;
                break;
            case 3:
                assets = 0;
                break;
            case 4:
                customForm = CUSTOM_FORM_GARO;
                break;
            case 5:
                player.transformation = PLAYER_FORM_ZORA;
                break;
            case 6:
                player.heldItemId = ITEM_NET;
                break;
            case 7:
                hideSword = 1;
                break;
            case 8:
                kokiriUpgrade = 1;
                missing = "/progressive/";
                break;
            case 9:
                bossOwner = 1;
                break;
            case 10:
                loadFailure = 1;
                break;
        }
        func_8083375C(&player, PLAYER_MWA_JUMPSLASH_START);
        REQUIRE(player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags == DMG_SWORD);
        REQUIRE(player.meleeWeaponQuads[0].elem.atDmgInfo.damage == (guard == 5 ? 8 : 2));
    }
    SetupDamage();
    Player remote = player;
    func_8083375C(&remote, PLAYER_MWA_JUMPSLASH_START);
    REQUIRE(remote.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags == DMG_SWORD);
    REQUIRE(remote.meleeWeaponQuads[0].elem.atDmgInfo.damage == 2);
    // Fire augments recognition while native Razor/Gilded jump damage stays 4/6.
    for (int gilded = 0; gilded < 2; ++gilded) {
        player.heldItemId = gilded ? ITEM_SWORD_GILDED : ITEM_SWORD_RAZOR;
        player.heldItemAction = gilded ? PLAYER_IA_SWORD_GILDED : PLAYER_IA_SWORD_RAZOR;
        func_8083375C(&player, PLAYER_MWA_JUMPSLASH_START);
        REQUIRE(player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags == (DMG_SWORD | DMG_FIRE_ARROW));
        REQUIRE(player.meleeWeaponQuads[0].elem.atDmgInfo.damage == (gilded ? 6 : 4));
        missing = "/progressive/";
        func_8083375C(&player, PLAYER_MWA_JUMPSLASH_START);
        REQUIRE(player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags == DMG_SWORD);
        REQUIRE(player.meleeWeaponQuads[0].elem.atDmgInfo.damage == (gilded ? 6 : 4));
        missing = NULL;
    }
    SetupDamage();
    REQUIRE(DinFireSword_DamageFlags(&play, &player, DMG_SPIN_ATTACK) == DMG_SPIN_ATTACK);
    REQUIRE(DinFireSword_DamageFlags(&play, &player, DMG_SWORD_BEAM) == DMG_SWORD_BEAM);
    REQUIRE(DinFireSword_DamageFlags(&play, &player, DMG_SWORD | DMG_ICE_ARROW) == (DMG_SWORD | DMG_ICE_ARROW));
}

static void DropsAndRefresh(void) {
    SetupDamage();
    Strike(1, 1);
    ColliderElement* owned = &player.meleeWeaponQuads[0].elem;
    Actor target = { 0 };
    ColliderElement receiver = { 0 };
    ColliderJntSphElement elements[2] = { 0 };
    ColliderJntSph sphere = { 0 };
    sphere.count = 2;
    sphere.elements = elements;
    receiver.acHitElem = elements[0].base.acHitElem = owned;
    Actor_SetDropFlag(&target, &receiver);
    REQUIRE(target.dropFlag == DROPFLAG_NONE);
    Actor_SetDropFlagJntSph(&target, &sphere);
    REQUIRE(target.dropFlag == DROPFLAG_NONE);
    ColliderElement projectile = { 0 };
    const u32 flags[] = { DMG_FIRE_ARROW, DMG_ICE_ARROW, DMG_LIGHT_ARROW, DMG_SWORD | DMG_FIRE_ARROW };
    const u8 drops[] = { DROPFLAG_1, DROPFLAG_2, DROPFLAG_20, DROPFLAG_1 };
    for (size_t i = 0; i < ARRAY_COUNT(flags); ++i) {
        projectile.atDmgInfo.dmgFlags = flags[i];
        receiver.acHitElem = elements[1].base.acHitElem = &projectile;
        Actor_SetDropFlag(&target, &receiver);
        REQUIRE(target.dropFlag == drops[i]);
        Actor_SetDropFlagJntSph(&target, &sphere);
        REQUIRE(target.dropFlag == drops[i]);
    }
    for (int i = 0; i < 4; ++i) {
        damageOption = i & 1;
        DinFireSword_RefreshDamage(&play, &player);
        for (int quad = 0; quad < 2; ++quad) {
            REQUIRE(player.meleeWeaponQuads[quad].elem.atDmgInfo.dmgFlags ==
                    (DMG_SWORD | (damageOption ? DMG_FIRE_ARROW : 0)));
            REQUIRE(player.meleeWeaponQuads[quad].elem.atDmgInfo.damage == 6);
        }
    }
    customForm = CUSTOM_FORM_KAFEI;
    DinFireSword_RefreshDamage(&play, &player);
    REQUIRE(owned->atDmgInfo.dmgFlags == DMG_SWORD);
    REQUIRE(owned->atDmgInfo.damage == 6);

    // A forced-adult change can happen without replacing the live player.
    // Disabling fire at that boundary must restore the native flags before
    // retiring the previous age's ownership record.
    for (int before = 0; before < 2; ++before) {
        for (int enabled = 0; enabled < 2; ++enabled) {
            SetupDamage();
            adult = before;
            Strike(1, 1);
            adult = !before;
            damageOption = enabled;
            DinFireSword_RefreshDamage(&play, &player);
            for (int quad = 0; quad < 2; ++quad) {
                ColliderElement* attack = &player.meleeWeaponQuads[quad].elem;
                REQUIRE(attack->atDmgInfo.dmgFlags == (DMG_SWORD | (enabled ? DMG_FIRE_ARROW : 0)));
                REQUIRE(attack->atDmgInfo.damage == 6);
                RequireHit(ACTOR_EN_WF, Test_WolfosTable(), attack, 0xFFFFFFFF, 6, enabled ? 2 : 0);
            }
        }
    }
}

static void ExactIceGate(void) {
    SetupDamage();
    Strike(0, 0);
    ColliderElement* owned = &player.meleeWeaponQuads[0].elem;
    ColliderElement copied = *owned;
    ColliderElement arrow = { 0 }, sword = { 0 };
    arrow.atDmgInfo.dmgFlags = DMG_FIRE_ARROW;
    sword.atDmgInfo.dmgFlags = DMG_SWORD;
    ColliderElement* attacks[] = { owned, &copied, &arrow, &sword, NULL };
    const int accepted[] = { 1, 0, 1, 0, 0 };
    for (size_t i = 0; i < ARRAY_COUNT(attacks); ++i) {
        REQUIRE(DinFireSword_IsFireHit(&play, attacks[i]) == accepted[i]);
        BossHakugin goht = { 0 };
        goht.iceCollider.base.acFlags = AC_HIT;
        goht.iceCollider.elem.acHitElem = attacks[i];
        gohtThawed = 0;
        BossHakugin_FrozenBeforeFight(&goht, &play);
        REQUIRE(gohtThawed == accepted[i]);
        REQUIRE((goht.iceCollider.base.acFlags & AC_HIT) == (accepted[i] ? 0 : AC_HIT));
        for (int index = 0; index < 2; ++index) {
            ObjIcePoly ice = { 0 };
            ice.switchFlag = OBJICEPOLY_SWITCH_FLAG_NONE;
            ice.colliders2[index].base.acFlags = AC_HIT;
            ice.colliders2[index].base.ac = &player.actor;
            ice.colliders2[index].elem.acHitElem = attacks[i];
            queuedCutscenes = 0;
            func_80931A38(&ice, &play);
            REQUIRE(queuedCutscenes == accepted[i]);
            REQUIRE((ice.actionFunc == func_80931E58) == accepted[i]);
        }
    }
    // Hot water keeps its independent native route; cold water cannot melt ice.
    Actor water = { 0 };
    water.id = ACTOR_OBJ_AQUA;
    for (int hot = 0; hot < 2; ++hot) {
        ObjIcePoly ice = { 0 };
        ice.switchFlag = OBJICEPOLY_SWITCH_FLAG_NONE;
        water.params = hot ? AQUA_TYPE_HOT : AQUA_TYPE_COLD;
        ice.colliders2[0].base.acFlags = AC_HIT;
        ice.colliders2[0].base.ac = &water;
        queuedCutscenes = 0;
        func_80931A38(&ice, &play);
        REQUIRE(queuedCutscenes == hot);
    }
    ++play.sceneId;
    REQUIRE(!DinFireSword_IsFireHit(&play, owned));
    REQUIRE(!DinFireSword_IsFireHit(NULL, owned));
    REQUIRE(DinFireSword_IsFireHit(&play, &arrow));
}

static void MineReactions(void) {
    SetupDamage();
    Strike(0, 0);
    player.actor.shape.rot.y = 0x4000;
    ObjMine mine = { 0 };
    mine.actor.world.pos.z = 50.0f;
    mine.collider.elements = mine.colliderElements;
    mine.collider.base.ac = &player.actor;
    mine.colliderElements[0].dim.worldSphere.center = (Vec3s){ 0, 10, 50 };
    mine.colliderElements[0].base.acDmgInfo.hitPos = (Vec3s){ 50, 0, 50 };
    ColliderElement ordinary = { 0 }, fireArrow = { 0 };
    ordinary.atDmgInfo.dmgFlags = DMG_SWORD;
    fireArrow.atDmgInfo.dmgFlags = DMG_FIRE_ARROW;
    ColliderElement copied = player.meleeWeaponQuads[0].elem;
    ColliderElement* attacks[] = { &ordinary, &player.meleeWeaponQuads[0].elem, &player.meleeWeaponQuads[1].elem,
                                   &fireArrow, &copied };
    s16 angles[5], torques[5];
    Vec3f knockbacks[5];
    for (size_t i = 0; i < ARRAY_COUNT(attacks); ++i) {
        mine.colliderElements[0].base.acHitElem = attacks[i];
        ObjMine_Air_CheckAC(&mine, &angles[i], &torques[i]);
        ObjMine_Water_CheckAC(&mine, &knockbacks[i]);
        REQUIRE(matrixDepth == 0);
    }
    // Real yaw, normalization and matrix functions establish distinct native
    // sword/projectile responses instead of substituting a branch sentinel.
    REQUIRE(angles[0] == 0 && torques[0] == 0x2000);
    REQUIRE(fabsf(knockbacks[0].x) < 0.00001f);
    REQUIRE(fabsf(knockbacks[0].y - 0.19611614f) < 0.00001f);
    REQUIRE(fabsf(knockbacks[0].z - 0.98058068f) < 0.00001f);
    for (int i = 1; i <= 2; ++i) {
        REQUIRE(angles[i] == angles[0] && torques[i] == torques[0]);
        REQUIRE(!memcmp(&knockbacks[i], &knockbacks[0], sizeof(Vec3f)));
    }
    for (int i = 3; i <= 4; ++i) {
        REQUIRE(angles[i] == 0x4000 && torques[i] == 0x4000);
        REQUIRE(knockbacks[i].x > 0.999f);
        REQUIRE(fabsf(knockbacks[i].y) < 0.00001f && fabsf(knockbacks[i].z) < 0.00001f);
    }
    // Coincident melee contacts retain the mine's native upward fallback.
    mine.colliderElements[0].dim.worldSphere.center = (Vec3s){ 0, 0, 0 };
    mine.colliderElements[0].base.acHitElem = &player.meleeWeaponQuads[0].elem;
    Vec3f fallback;
    ObjMine_Water_CheckAC(&mine, &fallback);
    REQUIRE(fallback.x == 0.0f && fallback.y == 1.0f && fallback.z == 0.0f);
}

// This reaches the production AC admission hook before damage resolution. Unlike
// RequireHit, it never fabricates AC_HIT/acHitElem, so soul immunity is observable.
static void NativeSoulAdmission(void) {
    for (int visual = 0; visual < 2; ++visual) {
        for (int fire = 0; fire < 2; ++fire) {
            for (int enabled = 0; enabled < 2; ++enabled) {
                for (int owned = 0; owned < 2; ++owned) {
                    for (int kind = 0; kind < 4; ++kind) {
                        SetupDamage();
                        swordOption = visual;
                        damageOption = fire;
                        adult = 1;
                        Test_SetSoulState(enabled, owned);
                        Strike(0, 0); // Runtime log: Kokiri item 0x4D, action 0x03, human form.
                        DinFireSword_Update(&play, &player);
                        DinFireSword_RefreshDamage(&play, &player);
                        Actor target = { 0 };
                        Collider receiver = { 0 };
                        ColliderElement element = { 0 };
                        Vec3f contact = { 0 };
                        const s16 ids[] = { ACTOR_EN_SLIME, ACTOR_EN_DEKUBABA, ACTOR_EN_KUSA, ACTOR_OBJ_GRASS };
                        target.id = ids[kind];
                        target.category = kind < 2 ? ACTORCAT_ENEMY : ACTORCAT_PROP;
                        target.colChkInfo.damageTable = kind == 0   ? Test_ChuchuTable()
                                                        : kind == 1 ? Test_DekuBabaTable()
                                                                    : NULL;
                        receiver.actor = &target;
                        receiver.acFlags = AC_ON | AC_TYPE_PLAYER;
                        element.acElemFlags = ACELEM_ON;
                        element.acDmgInfo.dmgFlags = kind == 0   ? 0xF7CFFFFF
                                                     : kind == 2 ? Test_KusaMask()
                                                     : kind == 3 ? Test_GrassMask()
                                                                 : DMG_DEFAULT;
                        ColliderQuad* attack = &player.meleeWeaponQuads[0];
                        REQUIRE(!CollisionCheck_NoSharedFlags(&attack->elem, &element));
                        const int allowed = kind >= 2 || !enabled || owned;
                        REQUIRE(CollisionCheck_SetATvsAC(&play, &attack->base, &attack->elem, &contact, &receiver,
                                                         &element, &contact, &contact) == allowed);
                        REQUIRE(!!(receiver.acFlags & AC_HIT) == allowed);
                        CollisionCheck_ApplyDamage(&play, &play.colChkCtx, &receiver, &element);
                        REQUIRE(target.colChkInfo.damage == allowed);
                    }
                }
            }
        }
    }
    Test_SetSoulState(0, 0);
}

#ifndef ELEMENTAL_SWORD_DAMAGE_FIXTURE
int main(void) {
    AcceptedIceAudio();
    NativeSoulAdmission();
    puts("PASS MM Din: native soul hook gates Chuchu/Deku Baba, permits Kusa/Grass, both fire checkboxes off/on");
    NativeDamageAndReactions();
    puts("PASS MM Din: native 1/3/4 and 2/6/8 sword damage, 13 real enemy tables, native reactions and fire-only "
         "receivers");
    GrassAndOwnership();
    puts("PASS MM Din: real Kusa/Grass masks, default/off/assets/forms/net/remote guards, native Razor/Gilded and "
         "spin/beam");
    DropsAndRefresh();
    puts("PASS MM Din: native normal/joint-sphere drops, real projectile drops, reversible refresh preserves strong "
         "attacks");
    ExactIceGate();
    puts("PASS MM Din: exact owned fire predicate, native Goht and both ice block colliders, hot/cold water and stale "
         "context");
    MineReactions();
    puts("PASS MM Din: native air/water mine sword knockback and torque, real fire-arrow responses and overlap "
         "fallback");
    return 0;
}
#endif
