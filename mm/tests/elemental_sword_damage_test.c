#define ELEMENTAL_SWORD_DAMAGE_FIXTURE
#include "din_fire_damage_test.c"

static const u32 elementFlags[] = { 0, DMG_FIRE_ARROW, DMG_ICE_ARROW, DMG_LIGHT_ARROW };
static int elementalFailures;
#define CHECK_NATIVE(label, condition)                                                                            \
    do {                                                                                                          \
        if (!(condition)) {                                                                                       \
            fprintf(stderr, "FAIL %s, actor %d element %d (line %d)\n", label, targets[i].id, element, __LINE__); \
            ++elementalFailures;                                                                                  \
        }                                                                                                         \
    } while (0)
int Test_MM_WolfosHealth(PlayState* play, ColliderElement* toucher, u8 damage, u8 effect, int rear, int frozen,
                         int* visual);
int Test_MM_WallmasterFrozenHealth(PlayState*, ColliderElement*, u8, u8, int);
int Test_MM_FloormasterFrozenHealth(PlayState*, ColliderElement*, u8, u8, int);
int Test_MM_DekuNutsFrozenHealth(PlayState*, ColliderElement*, u8, u8, int);
int Test_MM_PeahatFrozenHealth(PlayState*, ColliderElement*, u8, u8, int);
int Test_MM_DinolfosFrozenHealth(PlayState*, ColliderElement*, u8, u8, int);
int Test_MM_DekuBabaFrozenHealth(PlayState*, ColliderElement*, u8, u8, int);
int Test_MM_ChuchuFrozenHealth(PlayState*, ColliderElement*, u8, u8, int);
int Test_MM_IronKnuckleFrozenHealth(PlayState*, ColliderElement*, u8, u8, int);
int Test_MM_DekuBabaCutRoute(void);
DamageTable* Test_IronKnuckleTable(void);
static void SelectElement(int element) {
    elementalSelection = elementalActive = element;
}

static void ElementalPower(void) {
    const struct {
        s16 id;
        DamageTable* (*table)(void);
        int effects[4];
    } targets[] = {
        { ACTOR_EN_DEKUBABA, Test_DekuBabaTable, { 15, 15, 15, 15 } },
        { ACTOR_EN_WF, Test_WolfosTable, { 0, 2, 3, 4 } },
        { ACTOR_EN_WF, Test_WhiteWolfosTable, { 0, 2, 3, 4 } },
        { ACTOR_EN_AM, Test_ArmosTable, { 0, 0, 0, 0 } },
        { ACTOR_EN_FIREFLY, Test_KeeseTable, { 0, 2, 3, 4 } },
        { ACTOR_EN_DINOFOS, Test_DinolfosTable, { 0, 0, 3, 4 } },
        { ACTOR_EN_RD, Test_RedeadTable, { 15, 15, 15, 4 } },
        { ACTOR_EN_FZ, Test_FreezardTable, { 15, 15, 15, 4 } },
        { ACTOR_EN_WALLMAS, Test_WallmasterTable, { 0, 2, 3, 4 } },
        { ACTOR_EN_FLOORMAS, Test_FloormasterTable, { 0, 2, 3, 4 } },
        { ACTOR_EN_CROW, Test_CrowTable, { 0, 2, 3, 4 } },
        { ACTOR_EN_DEKUNUTS, Test_DekuNutsTable, { 0, 2, 3, 4 } },
        { ACTOR_EN_PEEHAT, Test_PeahatTable, { 0, 2, 3, 4 } },
        { ACTOR_EN_SLIME, Test_ChuchuTable, { 0, 0, 0, 0 } },
        { ACTOR_EN_IK, Test_IronKnuckleTable, { 0, 0, 0, 0 } },
    };
    const int normalDamage[] = { 1, 3, 4 };
    for (int element = 1; element <= 3; ++element) {
        for (int weapon = 0; weapon < 3; ++weapon) {
            for (int strong = 0; strong < 2; ++strong) {
                SetupDamage();
                SelectElement(element);
                swordOption = alt = assets = 0;
                Strike(weapon, strong);
                const int damage = normalDamage[weapon] * (strong ? 2 : 1);
                for (int quad = 0; quad < 2; ++quad) {
                    ColliderElement* attack = &player.meleeWeaponQuads[quad].elem;
                    REQUIRE(attack->atDmgInfo.dmgFlags == (DMG_SWORD | elementFlags[element]));
                    REQUIRE(attack->atDmgInfo.damage == damage);
                    REQUIRE(DinFireSword_OriginalDamageFlags(&play, attack) == DMG_SWORD);
                    for (size_t i = 0; i < ARRAY_COUNT(targets); ++i) {
                        const int effect = targets[i].effects[element];
                        Actor result =
                            RequireHit(targets[i].id, targets[i].table(), attack, 0xFFFFFFFF, damage, effect);
                        if (targets[i].id == ACTOR_EN_WF) {
                            for (int rear = 0; rear < 2; ++rear) {
                                for (int frozen = 0; frozen < 2; ++frozen) {
                                    int visual;
                                    CHECK_NATIVE("Wolfos native frozen health",
                                                 Test_MM_WolfosHealth(&play, attack, result.colChkInfo.damage,
                                                                      result.colChkInfo.damageEffect, rear, frozen,
                                                                      &visual) == 100 - damage * (rear ? 4 : 1));
                                    CHECK_NATIVE("Wolfos native element visual",
                                                 visual == (element == 1   ? ACTOR_DRAW_DMGEFF_FIRE
                                                            : element == 2 ? ACTOR_DRAW_DMGEFF_FROZEN_NO_SFX
                                                                           : ACTOR_DRAW_DMGEFF_LIGHT_ORBS));
                                }
                            }
                        }
                        int (*reaction)(PlayState*, ColliderElement*, u8, u8, int) = NULL;
                        switch (targets[i].id) {
                            case ACTOR_EN_WALLMAS:
                                reaction = Test_MM_WallmasterFrozenHealth;
                                break;
                            case ACTOR_EN_FLOORMAS:
                                reaction = Test_MM_FloormasterFrozenHealth;
                                break;
                            case ACTOR_EN_DEKUNUTS:
                                reaction = Test_MM_DekuNutsFrozenHealth;
                                break;
                            case ACTOR_EN_PEEHAT:
                                reaction = Test_MM_PeahatFrozenHealth;
                                break;
                            case ACTOR_EN_DINOFOS:
                                reaction = Test_MM_DinolfosFrozenHealth;
                                break;
                            case ACTOR_EN_DEKUBABA:
                                reaction = Test_MM_DekuBabaFrozenHealth;
                                break;
                            case ACTOR_EN_SLIME:
                                reaction = Test_MM_ChuchuFrozenHealth;
                                break;
                            case ACTOR_EN_IK:
                                reaction = Test_MM_IronKnuckleFrozenHealth;
                                break;
                        }
                        if (reaction) {
                            for (int frozen = 0; frozen < 2; ++frozen) {
                                CHECK_NATIVE("native frozen health",
                                             reaction(&play, attack, result.colChkInfo.damage,
                                                      result.colChkInfo.damageEffect, frozen) == 100 - damage);
                                if (targets[i].id == ACTOR_EN_DEKUBABA)
                                    CHECK_NATIVE("frozen Baba retains sword cutting", Test_MM_DekuBabaCutRoute());
                                ColliderElement copied = *attack;
                                REQUIRE(reaction(&play, &copied, result.colChkInfo.damage,
                                                 result.colChkInfo.damageEffect,
                                                 frozen) == (frozen && element != 1 ? 100 : 100 - damage));
                            }
                        }
                        RequireHit(targets[i].id, targets[i].table(), attack, DMG_SWORD, damage, targets[i].effects[0]);
                    }
                    ColliderElement receiver = { 0 };
                    receiver.acDmgInfo.dmgFlags = Test_KusaMask();
                    REQUIRE(!CollisionCheck_NoSharedFlags(attack, &receiver));
                    receiver.acDmgInfo.dmgFlags = Test_GrassMask();
                    REQUIRE(!CollisionCheck_NoSharedFlags(attack, &receiver));
                    receiver.acHitElem = attack;
                    Actor target = { 0 };
                    Actor_SetDropFlag(&target, &receiver);
                    REQUIRE(target.dropFlag == DROPFLAG_NONE);
                    REQUIRE(DinFireSword_IsFireHit(&play, attack) == (element == 1));
                }
                elementalActive = 0;
                swordOption = alt = assets = 1;
                DinFireSword_RefreshDamage(&play, &player);
                REQUIRE(player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags == DMG_SWORD);
                REQUIRE(player.meleeWeaponQuads[0].elem.atDmgInfo.damage == damage);
            }
        }
    }
}

static void SwitchAndNativeGates(void) {
    SetupDamage();
    Strike(1, 1);
    ColliderElement* attack = &player.meleeWeaponQuads[0].elem;
    const int order[] = { 2, 3, 1, 0, 2, 0 };
    for (size_t i = 0; i < ARRAY_COUNT(order); ++i) {
        SelectElement(order[i]);
        DinFireSword_RefreshDamage(&play, &player);
        REQUIRE(attack->atDmgInfo.dmgFlags == (DMG_SWORD | elementFlags[order[i]]));
        REQUIRE(attack->atDmgInfo.damage == 6);
        REQUIRE(DinFireSword_IsFireHit(&play, attack) == (order[i] == 1));
        BossHakugin goht = { 0 };
        goht.iceCollider.base.acFlags = AC_HIT;
        goht.iceCollider.elem.acHitElem = attack;
        gohtThawed = 0;
        BossHakugin_FrozenBeforeFight(&goht, &play);
        REQUIRE(gohtThawed == (order[i] == 1));
    }
    SelectElement(2);
    DinFireSword_RefreshDamage(&play, &player);
    RequireHit(ACTOR_EN_FZ, Test_FreezardTable(), attack, DMG_ICE_ARROW, 0, 0);
    ColliderElement copy = *attack;
    REQUIRE(DinFireSword_OriginalDamageFlags(&play, &copy) == copy.atDmgInfo.dmgFlags);
    REQUIRE(!DinFireSword_IsFireHit(&play, &copy));
    player.meleeWeaponQuads[1].elem.atDmgInfo.dmgFlags = DMG_LIGHT_ARROW;
    SelectElement(0);
    DinFireSword_RefreshDamage(&play, &player);
    REQUIRE(player.meleeWeaponQuads[1].elem.atDmgInfo.dmgFlags == DMG_LIGHT_ARROW);
    REQUIRE(attack->atDmgInfo.dmgFlags == DMG_SWORD);
    SelectElement(3);
    DinFireSword_RefreshDamage(&play, &player);
    const u32 applied = attack->atDmgInfo.dmgFlags;
    ++play.sceneId;
    REQUIRE(DinFireSword_OriginalDamageFlags(&play, attack) == applied);
    adult = !adult;
    elementalActive = 0;
    DinFireSword_RefreshDamage(&play, &player);
    REQUIRE(attack->atDmgInfo.dmgFlags == DMG_SWORD);
    REQUIRE(attack->atDmgInfo.damage == 6);
}

static void SelectionRendering(void) {
    for (int element = 0; element <= 3; ++element) {
        SetupDamage();
        SelectElement(element);
        ++play.gameplayFrames;
        DinFireSword_Update(&play, &player);
        DinFireSword_BeginPlayerDraw(&play, &player);
        DinFireSword_Draw(&play, &player);
        DinFireSword_DrawAfterPlayer(&play, &player);
        REQUIRE(((gfx.polyOpa.p - opa) + (gfx.polyXlu.p - xlu) > 0) == (element != 0));
    }
}

int main(void) {
    ElementalPower();
    SwitchAndNativeGates();
    SelectionRendering();
    if (elementalFailures)
        return 1;
    puts("PASS MM elemental sword combat: native numeric power/multipliers, effects, grass/drops, exact switching and "
         "fire-only gates");
    return 0;
}
