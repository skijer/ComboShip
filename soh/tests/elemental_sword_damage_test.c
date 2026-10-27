// Real combat/collision/drop code with deterministic selection-state boundaries.
#define ELEMENTAL_SWORD_DAMAGE_FIXTURE
#include "din_fire_sword_damage_test.c"

static const u32 elementFlags[] = { 0, DMG_ARROW_FIRE, DMG_ARROW_ICE, DMG_ARROW_LIGHT };
int Test_LizalfosHealth(PlayState* play, ColliderInfo* toucher, int alreadyStunned, int* stunned);
int Test_WallmasterLightStun(u8 effect);
int Test_FloormasterLightStun(u8 effect);

static void selectElement(int element) {
    elementalSelection = elementalActive = element;
}

static void testElementalPower(void) {
    const struct {
        s16 id;
        DamageTable* (*table)(void);
    } targets[] = {
        { ACTOR_EN_DEKUBABA, Test_DekuBabaTable },
        { ACTOR_EN_RD, Test_ReDeadTable },
        { ACTOR_EN_WF, Test_WolfosTable },
        { ACTOR_EN_ZF, Test_LizalfosTable },
        { ACTOR_EN_AM, Test_ArmosTable },
        { ACTOR_EN_FIREFLY, Test_FireflyTable },
        { ACTOR_EN_TP, Test_TailpasaranTable },
        { ACTOR_EN_WALLMAS, Test_WallmasterTable },
        { ACTOR_EN_FLOORMAS, Test_FloormasterTable },
        { ACTOR_EN_CROW, Test_CrowTable },
        { ACTOR_EN_DEKUNUTS, Test_DekuNutsTable },
        { ACTOR_EN_PEEHAT, Test_PeahatTable },
    };
    for (int element = 1; element <= 3; ++element) {
        for (int sword = 0; sword < 3; ++sword) {
            for (int strike = 0; strike < 3; ++strike) {
                setupSword(sword);
                selectElement(element);
                enabled = alt = assets = 0;
                const u32 nativeFlags = swordFlags[sword][strike];
                ColliderInfo* attack = setStrike(nativeFlags);
                CHECK("generic selected element needs no Din assets/Alt",
                      attack->toucher.dmgFlags == (nativeFlags | elementFlags[element]));
                CHECK("owned element recovers native sword classification",
                      DinFireSword_OriginalDamageFlags(&play, attack) == nativeFlags);
                ColliderInfo native = *attack;
                native.toucher.dmgFlags = nativeFlags;
                for (size_t i = 0; i < ARRAY_COUNT(targets); ++i) {
                    Actor baseline = hit(targets[i].id, targets[i].table(), 0xFFCFFFFF, &native);
                    Actor candidate = hit(targets[i].id, targets[i].table(), 0xFFCFFFFF, attack);
                    CHECK("element never replaces an admitted sword's damage",
                          candidate.colChkInfo.damage == baseline.colChkInfo.damage);
                    Actor swordOnly = hit(targets[i].id, targets[i].table(), nativeFlags, attack);
                    CHECK("sword-only receiver retains native entry",
                          swordOnly.colChkInfo.damage == baseline.colChkInfo.damage &&
                              swordOnly.colChkInfo.damageEffect == baseline.colChkInfo.damageEffect);
                }
                Actor baba = hit(ACTOR_EN_DEKUBABA, Test_DekuBabaTable(), 0xFFCFFFFF, attack);
                CHECK("all elements preserve Baba cutting/stick route", baba.colChkInfo.damageEffect == 15);
                ColliderInfo receiver = { 0 };
                receiver.bumper.dmgFlags = Test_BushMask();
                CHECK("all elements still cut grass", !CollisionCheck_NoSharedFlags(attack, &receiver));
                receiver.bumper.dmgFlags = Test_WitheredMask();
                CHECK("all elements still cut withered Baba", !CollisionCheck_NoSharedFlags(attack, &receiver));
                receiver.acHitInfo = attack;
                Actor_SetDropFlag(&baba, &receiver, 1);
                CHECK("enchanted melee retains sword drops", baba.dropFlag == 0);
                CHECK("enchanted melee retains sword-finisher power",
                      CollisionCheck_GetSwordDamage(attack->toucher.dmgFlags, &play) == swordDamage[sword][strike]);
                for (int before = 0; before < 2; ++before) {
                    int stunned;
                    CHECK("native Lizalfos first/repeated health consumes unchanged sword damage",
                          Test_LizalfosHealth(&play, attack, before, &stunned) == 30 - swordDamage[sword][strike]);
                    CHECK("native Lizalfos first ice hit stuns", stunned == (element == 2 && !before));
                }
                // Checkbox/inventory gating is supplied by the selector boundary.
                elementalActive = 0;
                enabled = alt = assets = 1;
                DinFireSword_RefreshDamage(&play, &player);
                CHECK("disabled selected element reverses exact write without legacy fallback",
                      attack->toucher.dmgFlags == nativeFlags);
            }
        }
    }
}

static void testActorEffects(void) {
    setupSword(1);
    selectElement(2);
    ColliderInfo* attack = setStrike(DMG_SLASH_MASTER);
    const struct {
        s16 id;
        DamageTable* (*table)(void);
        u8 effect;
    } ice[] = {
        { ACTOR_EN_CROW, Test_CrowTable, 3 },
        { ACTOR_EN_FIREFLY, Test_FireflyTable, 3 },
        { ACTOR_EN_ZF, Test_LizalfosTable, 15 },
    };
    for (size_t i = 0; i < ARRAY_COUNT(ice); ++i) {
        Actor result = hit(ice[i].id, ice[i].table(), 0xFFCFFFFF, attack);
        CHECK("ice uses this actor's native ice effect", result.colChkInfo.damageEffect == ice[i].effect);
    }
    Actor immune = hit(ACTOR_EN_RD, Test_ReDeadTable(), 0xFFCFFFFF, attack);
    CHECK("ice-immune row cannot erase sword damage",
          immune.colChkInfo.damage == 2 && immune.colChkInfo.damageEffect == 15);
    Actor zero = hit(ACTOR_EN_RD, Test_ReDeadTable(), DMG_ARROW_ICE, attack);
    CHECK("ice-only receiver retains native zero damage",
          zero.colChkInfo.damage == 0 && zero.colChkInfo.damageEffect == 0);
    selectElement(3);
    DinFireSword_RefreshDamage(&play, &player);
    Actor wall = hit(ACTOR_EN_WALLMAS, Test_WallmasterTable(), 0xFFCFFFFF, attack);
    Actor floor = hit(ACTOR_EN_FLOORMAS, Test_FloormasterTable(), 0xFFCFFFFF, attack);
    CHECK("light uses native Wallmaster stun", wall.colChkInfo.damage == 2 && wall.colChkInfo.damageEffect == 4);
    CHECK("light uses native Floormaster stun", floor.colChkInfo.damage == 2 && floor.colChkInfo.damageEffect == 4);
    CHECK("Wallmaster executes native light stun state", Test_WallmasterLightStun(wall.colChkInfo.damageEffect));
    CHECK("Floormaster executes native light stun state", Test_FloormasterLightStun(floor.colChkInfo.damageEffect));
    Actor lizalfos = hit(ACTOR_EN_ZF, Test_LizalfosTable(), 0xFFCFFFFF, attack);
    CHECK("light keeps Lizalfos melee recoil", lizalfos.colChkInfo.damageEffect == 0);
}

static void testSwitchAndIsolation(void) {
    setupSword(1);
    ColliderInfo* attack = setStrike(DMG_JUMP_MASTER);
    const int order[] = { 2, 3, 1, 0, 2, 0 };
    for (size_t i = 0; i < ARRAY_COUNT(order); ++i) {
        selectElement(order[i]);
        DinFireSword_RefreshDamage(&play, &player);
        CHECK("switching selection removes the prior element bit",
              attack->toucher.dmgFlags == (DMG_JUMP_MASTER | elementFlags[order[i]]));
        CHECK("switching leaves numeric damage untouched", attack->toucher.damage == 7);
    }
    selectElement(2);
    DinFireSword_RefreshDamage(&play, &player);
    ColliderInfo copied = *attack;
    CHECK("foreign identical flags are not owned",
          DinFireSword_OriginalDamageFlags(&play, &copied) == copied.toucher.dmgFlags);
    player.meleeWeaponQuads[1].info.toucher.dmgFlags = DMG_ARROW_LIGHT;
    selectElement(0);
    DinFireSword_RefreshDamage(&play, &player);
    CHECK("reversal leaves independent collision writes",
          player.meleeWeaponQuads[1].info.toucher.dmgFlags == DMG_ARROW_LIGHT);
    CHECK("explicit None suppresses legacy fire", attack->toucher.dmgFlags == DMG_JUMP_MASTER);
    selectElement(3);
    DinFireSword_RefreshDamage(&play, &player);
    const u32 applied = attack->toucher.dmgFlags;
    ++play.sceneNum;
    CHECK("stale scene never recovers an owned element", DinFireSword_OriginalDamageFlags(&play, attack) == applied);
    gSaveContext.linkAge = LINK_AGE_CHILD;
    elementalActive = 0;
    DinFireSword_RefreshDamage(&play, &player);
    CHECK("context handoff reverses exact elemental write", attack->toucher.dmgFlags == DMG_JUMP_MASTER);
    CHECK("context handoff preserves numeric damage", attack->toucher.damage == 7);
    for (int element = 0; element <= 3; ++element) {
        setupSword(1);
        player.heldItemId = ITEM_SWORD_MASTER;
        selectElement(element);
        tick();
        CHECK("effective selection draws only its blade aura", (draw() > 0) == (element != 0));
    }
}

int main(void) {
    testElementalPower();
    testActorEffects();
    testSwitchAndIsolation();
    if (failures)
        return 1;
    puts("PASS elemental sword damage: native power, actor effects, assets off, grass/drops, exact switching and "
         "legacy suppression");
    return 0;
}
