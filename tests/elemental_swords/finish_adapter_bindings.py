"""Verbatim native death/acceptance branches, excluding unrelated engine work."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import functions, block_from


def build(host, directory):
    def source(name):
        return (ROOT / host / f"src/overlays/actors/ovl_{name}/z_{name.lower()}.c").read_text()

    def body(name, names):
        parts = functions(source(name), names)
        return "\n".join(parts[n] for n in names)

    oot = host == "soh"
    code = '''#include "overlays/actors/ovl_En_Bubble/z_en_bubble.h"
static void EnBubble_Pop(EnBubble* this, PlayState* play) {}
static void EnBubble_Fly(EnBubble* this, PlayState* play) {}
static void EnBubble_DamagePlayer(EnBubble* this, PlayState* play) {}
#define CollisionCheck_SetAC(...) 0
#define CollisionCheck_SetOC(...) 0
'''
    count_fn, detect_fn, init_fn = ("func_809CC648", "EnBubble_DetectPop", "func_809CBCBC") if oot else (
        "func_8089FF30", "EnBubble_IsPopped", "func_8089F59C")
    code += f'static s16 {init_fn}(EnBubble* this) {{ return 7; }}\n'
    code += body("En_Bubble", [count_fn, detect_fn, "EnBubble_Wait"])
    code += '''
#undef CollisionCheck_SetAC
#undef CollisionCheck_SetOC
static void Test_FinishBubble(void) {
    World();
    EnBubble bubble = { 0 };
    bubble.actor.id = ACTOR_EN_BUBBLE; bubble.actor.category = ACTORCAT_ENEMY;
    bubble.actor.update = LiveUpdate;
    bubble.colliderSphere.base.actor = &bubble.actor;
    bubble.colliderSphere.base.shape = COLSHAPE_JNTSPH;
    bubble.colliderSphere.base.acFlags = AC_HIT;
    bubble.colliderSphere.count = 2;
'''
    fields = "colliderSphereItems" if oot else "colliderElements"
    info = "info" if oot else "base"
    hit = "acHitInfo" if oot else "acHitElem"
    code += f'''    bubble.colliderSphere.elements = bubble.{fields};
    bubble.{fields}[0].dim.worldSphere = (Sphere16){{ {{ 10, 20, 30 }}, 20 }};
    bubble.{fields}[0].{info}.{hit} = &player.meleeWeaponQuads[0].{'info' if oot else 'elem'};
'''
    code += '''    // Real sword provenance, but no numeric-damage observation: native
    // flags-only pop must still acquire exactly the accepted receiver's source.
'''
    if oot:
        code += '''    player.meleeWeaponQuads[0].info.toucher.dmgFlags =
        DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SLASH_MASTER);
    bubble.colliderSphereItems[0].info.bumper.dmgFlags = 0xFFFFFFFF;
    bubble.colliderSphereItems[0].info.bumperFlags = BUMP_HIT;
'''
    else:
        code += '''    player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags =
        DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SWORD);
    bubble.colliderElements[0].base.acDmgInfo.dmgFlags = 0xFFFFFFFF;
    bubble.colliderElements[0].base.acElemFlags = ACELEM_HIT;
'''
    code += '''    DinFireSword_FinishBeginActor(&play, &bubble.actor);
    EnBubble_Wait(&bubble, &play);
    DinFireSword_FinishEndActor(&play, &bubble.actor);
    REQUIRE(Effects() == 1 && bubble.actionFunc == EnBubble_Pop && bubble.explosionCountdown == 7);
    REQUIRE(bubble.actor.update == LiveUpdate);
    World();
    bubble.actionFunc = NULL;
    bubble.colliderSphere.base.acFlags = 0;
    bubble.colliderSphere.base.ocFlags2 = OC2_HIT_PLAYER;
'''
    code += ('    bubble.unk_208 = 0;\n' if oot else '    bubble.timer = 0;\n')
    code += ('    player.meleeWeaponQuads[0].info.toucher.dmgFlags = DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SLASH_MASTER);\n' if oot else
             '    player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags = DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SWORD);\n')
    code += '''    // An old active sword address plus OC player contact is not a sword kill.
    DinFireSword_FinishBeginActor(&play, &bubble.actor);
    EnBubble_Wait(&bubble, &play);
    DinFireSword_FinishEndActor(&play, &bubble.actor);
    REQUIRE(Effects() == 0 && bubble.actionFunc == EnBubble_Pop);
}
'''
    code += '#define Actor_PlaySfx(...) ((void)0)\n#define Audio_PlayActorSound2(...) ((void)0)\n'
    if oot:
        code += '''#include "overlays/actors/ovl_Boss_Dodongo/z_boss_dodongo.h"
#include "objects/object_kingdodongo/object_kingdodongo.h"
static void BossDodongo_Vulnerable(BossDodongo* this, PlayState* play) {}
static void BossDodongo_LayDown(BossDodongo* this, PlayState* play) {}
static void BossDodongo_Inhale(BossDodongo* this, PlayState* play) {}
static void BossDodongo_DeathCutscene(BossDodongo* this, PlayState* play) {}
static void BossDodongo_SetupDeathCutscene(BossDodongo* this) {}
static void BossDodongo_SetupWalk(BossDodongo* this) {}
static void BossDodongo_SetupDamaged(BossDodongo* this) {}
#define BossSuperDamage_IsActive(...) 0
#define BossSuperDamage_StartElectricSparks(...) ((void)0)
#define BossSuperDamage_FormDamage(...) 4
#define Actor_RequestQuakeAndRumble(...) ((void)0)
#define Animation_Change(...) ((void)0)
'''
        code += body("Boss_Dodongo", ["BossDodongo_UpdateDamage"])
        code += '''
static void Test_FinishKingDodongo(void) {
    for (int health = 2; health <= 4; health += 2) {
        World();
        BossDodongo boss = { 0 };
        boss.actor.id = ACTOR_BOSS_DODONGO; boss.actor.category = ACTORCAT_BOSS; boss.actor.update = LiveUpdate;
        boss.health = health; boss.actionFunc = BossDodongo_Vulnerable;
        boss.collider.base.actor = &boss.actor; boss.collider.base.shape = COLSHAPE_JNTSPH;
        boss.collider.count = 19; boss.collider.elements = boss.items;
        boss.items[0].info.bumperFlags = BUMP_HIT;
        boss.items[0].info.bumper.dmgFlags = 0xFFFFFFFF;
        boss.items[0].info.acHitInfo = &player.meleeWeaponQuads[0].info;
        player.meleeWeaponQuads[0].info.toucher.dmgFlags =
            DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SLASH_MASTER);
        DinFireSword_FinishBeginActor(&play, &boss.actor);
        BossDodongo_UpdateDamage(&boss, &play);
        DinFireSword_FinishEndActor(&play, &boss.actor);
        REQUIRE(boss.health == health - 2 && boss.unk_1C0 == 5);
        REQUIRE(Effects() == (health == 2));
        REQUIRE(boss.actor.update == LiveUpdate);
    }
}
#include "overlays/actors/ovl_Boss_Va/z_boss_va.h"
static s16 sFightPhase;
static void BossVa_BariDeath(BossVa* this, PlayState* play) {}
static void BossVa_SetupAction(BossVa* this, BossVaActionFunc action) { this->actionFunc = action; }
#define Rand_CenteredFloat(...) 0.0f
#define Rand_ZeroOne(...) 0.5f
#define Rand_S16Offset(...) 80
#define EffectSsDtBubble_SpawnColorProfile(...) ((void)0)
'''
        code += body("Boss_Va", ["BossVa_SetupBariDeath", "BossVa_KillBari"])
        code += '''
static void Test_FinishBari(void) {
    World();
    BossVa bari = { 0 };
    bari.actor.id = ACTOR_BOSS_VA; bari.actor.category = ACTORCAT_BOSS; bari.actor.update = LiveUpdate;
    bari.colliderSph.base.actor = &bari.actor; bari.colliderSph.base.shape = COLSHAPE_JNTSPH;
    bari.colliderSph.count = 1; bari.colliderSph.elements = bari.elements;
    bari.elements[0].info.acHitInfo = &player.meleeWeaponQuads[0].info;
    bari.elements[0].info.bumper.dmgFlags = 0xFFFFFFFF;
    player.meleeWeaponQuads[0].info.toucher.dmgFlags = DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SLASH_MASTER);
    DinFireSword_FinishAcceptedHit(&play, &bari.colliderSph.base, bari.elements[0].info.acHitInfo, 0xFFFFFFFF, 2);
    DinFireSword_FinishBeginActor(&play, &bari.actor);
    BossVa_KillBari(&bari, &play);
    DinFireSword_FinishEndActor(&play, &bari.actor);
    REQUIRE(Effects() == 1 && bari.timer == 30 && bari.isDead == 1 && bari.actionFunc == BossVa_BariDeath);
}
'''
        code += '''#include "overlays/actors/ovl_En_Fd_Fire/z_en_fd_fire.h"
static void EnFdFire_Disappear(EnFdFire* this, PlayState* play) {}
'''
        code += body("En_Fd_Fire", ["EnFdFire_CheckCollider"])
        mask = re.search(r'\{ (0x[0-9A-F]+), 0x00, 0x00 \}', source("En_Fd_Fire"))[1]
        code += f'''static void Test_FinishFlames(void) {{
    const u32 attacks[] = {{ DMG_SLASH_MASTER, DMG_SPIN_MASTER, DMG_JUMP_MASTER }};
    for (int i = 0; i < 3; ++i) {{
        World(); EnFdFire fire = {{ 0 }};
        fire.actor.id = ACTOR_EN_FD_FIRE; fire.actor.category = ACTORCAT_ENEMY; fire.actor.update = LiveUpdate;
        fire.collider.base.actor = &fire.actor; fire.collider.base.shape = COLSHAPE_CYLINDER;
        fire.collider.info.bumper.dmgFlags = {mask};
        fire.collider.info.acHitInfo = &player.meleeWeaponQuads[0].info;
        u32 flags = DinFireSword_SetDamageFlags(&play, &player, 0, attacks[i]);
        player.meleeWeaponQuads[0].info.toucher.dmgFlags = flags;
        if (flags & {mask}) fire.collider.base.acFlags = AC_HIT;
        DinFireSword_FinishBeginActor(&play, &fire.actor);
        int dying = EnFdFire_CheckCollider(&fire, &play);
        DinFireSword_FinishEndActor(&play, &fire.actor);
        REQUIRE(dying == (i != 0) && Effects() == (i != 0));
        REQUIRE(fire.actor.colChkInfo.damage == 0 && fire.actor.update == LiveUpdate);
    }}
}}
static void Test_FinishAdapters(void) {{ Test_FinishBubble(); Test_FinishKingDodongo(); Test_FinishBari(); Test_FinishFlames(); }}
'''
    else:
        code += '''#include "overlays/actors/ovl_En_Rat/z_en_rat.h"
static void EnRat_Bounced(EnRat* this, PlayState* play) {}
static void EnRat_Explode(EnRat* this, PlayState* play) { this->timer = 99; }
#define EN_RAT_DMGEFF_HOOKSHOT 1
#define EN_RAT_DMGEFF_STUN 0xF
#define EN_RAT_HOOK_STARTED -2
#define Actor_SetColorFilter(...) ((void)0)
'''
        rat = source("En_Rat")
        start = rat.index('    } else if (this->collider.base.acFlags & AC_HIT) {', rat.index('void EnRat_Update(Actor* thisx, PlayState* play) {'))
        branch = block_from(rat, start + 6)
        code += 'static void Test_RatAcceptedDamage(EnRat* this, PlayState* play) { if (0) {} ' + branch + '\n}\n'
        code += '''static void Test_FinishRat(void) {
    World(); EnRat rat = { 0 };
    rat.actor.id = ACTOR_EN_RAT; rat.actor.category = ACTORCAT_ENEMY; rat.actor.update = LiveUpdate;
    rat.collider.base.actor = &rat.actor; rat.collider.base.acFlags = AC_HIT;
    Accept(&rat.collider, 3); rat.collider.base.acFlags |= AC_HIT;
    DinFireSword_FinishBeginActor(&play, &rat.actor);
    Test_RatAcceptedDamage(&rat, &play);
    DinFireSword_FinishEndActor(&play, &rat.actor);
    REQUIRE(Effects() == 1 && rat.timer == 99 && rat.actor.update == LiveUpdate);
}
#include "overlays/actors/ovl_En_Tanron3/z_en_tanron3.h"
#include "overlays/actors/ovl_Boss_03/z_boss_03.h"
static Boss03 gyorg, *sGyorg = &gyorg;
static void EnTanron3_SetupDie(EnTanron3* this, PlayState* play) { this->deathTimer = 15; }
#define func_800B8D50(...) ((void)0)
'''
        code += body("En_Tanron3", ["EnTanron3_CheckCollisions"])
        code += '''static void Test_FinishFry(void) {
    World(); EnTanron3 fry = { 0 };
    fry.actor.id = ACTOR_EN_TANRON3; fry.actor.category = ACTORCAT_ENEMY; fry.actor.update = LiveUpdate;
    fry.acCollider.base.actor = &fry.actor; fry.acCollider.base.acFlags = AC_HIT;
    Accept(&fry.acCollider, 3); fry.acCollider.base.acFlags |= AC_HIT;
    DinFireSword_FinishBeginActor(&play, &fry.actor);
    EnTanron3_CheckCollisions(&fry, &play);
    DinFireSword_FinishEndActor(&play, &fry.actor);
    REQUIRE(Effects() == 1 && fry.deathTimer == 15 && fry.actor.update == LiveUpdate);
}
#include "overlays/actors/ovl_En_Minideath/z_en_minideath.h"
#include "overlays/actors/ovl_En_Death/z_en_death.h"
static s16 sScatterTimer;
static u8 sPlayedDeathSfx;
static void EnMinideath_CrowdParent(EnMinideath* this, PlayState* play) {}
static void EnMinideath_Intro3(EnMinideath* this, PlayState* play) {}
static void EnMinideath_SwarmFollower(EnMinideath* this, PlayState* play) {}
static void EnMinideath_UpdateCrowdState(EnMinideath* this, s32 crowdParent) {}
#define DMGEFF_STUN 1
#define DMGEFF_LIGHT_ARROW 4
'''
        code += body("En_Minideath", ["EnMinideath_UpdateDamage"])
        code += '''static void Test_FinishMinibats(void) {
    World(); EnMinideath bats = { 0 };
    bats.actor.id = ACTOR_EN_MINIDEATH; bats.actor.category = ACTORCAT_ENEMY; bats.actor.update = LiveUpdate;
    bats.collider.base.actor = &bats.actor; bats.collider.base.acFlags = AC_HIT;
    bats.collider.elements = bats.colliderElements; bats.collider.count = MINIDEATH_NUM_EFFECTS;
    player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags = DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SWORD);
    ColliderElement foreign = player.meleeWeaponQuads[0].elem;
    for (int i = 0; i < MINIDEATH_NUM_EFFECTS; ++i) {
        bats.effects[i].pos = (Vec3f){ 10 + i * 50, 20, 30 };
        bats.colliderElements[i].base.acElemFlags = ACELEM_HIT;
        bats.colliderElements[i].base.acHitElem = i == 2 ? &foreign : &player.meleeWeaponQuads[0].elem;
    }
    DinFireSword_FinishBeginActor(&play, &bats.actor);
    EnMinideath_UpdateDamage(&bats, &play);
    DinFireSword_FinishEndActor(&play, &bats.actor);
    REQUIRE(Effects() == 2 && sFinish.effects[0].shape.count == 1 && sFinish.effects[1].shape.count == 1);
    REQUIRE(sFinish.effects[0].shape.origin.x == 10 && sFinish.effects[1].shape.origin.x == 60);
    REQUIRE(bats.effects[0].state == 1 && bats.effects[1].timer == 60 && bats.effects[2].state == 1);
    DinFireSword_FinishForget(&play, &bats.actor); memset(&bats, 0xDC, sizeof(bats));
    REQUIRE(sFinish.effects[1].shape.points[0].x == 60);
}
#include "overlays/actors/ovl_Boss_07/z_boss_07.h"
static void Boss07_Mask_Damaged(Boss07* this, PlayState* play) {}
'''
        code += body("Boss_07", ["Boss07_Mask_SetupDamaged"])
        code += '''static void Test_FinishMajoraOverkill(void) {
    World(); Boss07 boss = { 0 }; ColliderCylinder receiver = { 0 };
    boss.actor.id = ACTOR_BOSS_07; boss.actor.category = ACTORCAT_BOSS;
    boss.actor.update = LiveUpdate; boss.actor.colChkInfo.health = 1;
    receiver.base.actor = &boss.actor; receiver.base.acFlags = AC_HIT;
    Accept(&receiver, 3);
    DinFireSword_FinishBeginActor(&play, &boss.actor);
    Boss07_Mask_SetupDamaged(&boss, &play, 4, NULL);
    DinFireSword_FinishEndActor(&play, &boss.actor);
    REQUIRE(Effects() == 1 && boss.actor.colChkInfo.health == 253 && boss.timers[0] == 30);
    REQUIRE(boss.actor.update == LiveUpdate && boss.actionFunc == Boss07_Mask_Damaged);
}
'''
        code += '''#include "overlays/actors/ovl_En_Baguo/z_en_baguo.h"
static void EnBaguo_PostDetonation(EnBaguo* this, PlayState* play) {}
#define NEJIRON_ACTION_RETREATING 2
#define NEJIRON_ACTION_EXPLODING 3
#define NEJIRON_DMGEFF_KILL 14
#define Rand_ZeroFloat(...) 0.0f
#define EnBaguo_InitializeEffect(...) ((void)0)
#define Actor_Spawn(...) ((Actor*)0)
#define Actor_SetScale(...) ((void)0)
#define Item_DropCollectibleRandom(...) ((void)0)
'''
        code += body("En_Baguo", ["EnBaguo_CheckForDetonation"])
        code += '''static void Test_FinishNejiron(void) {
    World(); EnBaguo enemy = { 0 }; ColliderJntSphElement element = { 0 };
    enemy.actor.id = ACTOR_EN_BAGUO; enemy.actor.category = ACTORCAT_ENEMY; enemy.actor.update = LiveUpdate;
    enemy.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    enemy.collider.base.actor = &enemy.actor; enemy.collider.base.acFlags = AC_HIT;
    enemy.collider.base.shape = COLSHAPE_JNTSPH; enemy.collider.elements = &element; enemy.collider.count = 1;
    element.base.acDmgInfo.dmgFlags = 0xFFFFFFFF;
    element.base.acHitElem = &player.meleeWeaponQuads[0].elem;
    player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags = DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SWORD);
    enemy.actor.colChkInfo.damageEffect = NEJIRON_DMGEFF_KILL;
    DinFireSword_FinishBeginActor(&play, &enemy.actor);
    EnBaguo_CheckForDetonation(&enemy, &play);
    DinFireSword_FinishEndActor(&play, &enemy.actor);
    REQUIRE(Effects() == 1 && enemy.action == NEJIRON_ACTION_EXPLODING && enemy.timer == 30);
    World();
    enemy.action = 0;
    enemy.actor.bgCheckFlags = 0;
    enemy.actor.world.pos.y = -200;
    player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags = DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SWORD);
    DinFireSword_FinishBeginActor(&play, &enemy.actor);
    EnBaguo_CheckForDetonation(&enemy, &play);
    DinFireSword_FinishEndActor(&play, &enemy.actor);
    REQUIRE(Effects() == 0 && enemy.action == NEJIRON_ACTION_EXPLODING);
}
#include "overlays/actors/ovl_En_Invadepoh/z_en_invadepoh.h"
static void EnInvadepoh_Alien_Damaged(EnInvadepoh* this, PlayState* play) {}
static void EnInvadepoh_Alien_Draw(Actor* actor, PlayState* play) {}
#define Animation_PlayLoop(...) ((void)0)
'''
        code += body("En_Invadepoh", ["EnInvadepoh_Alien_SetupDamaged"])
        alien = source("En_Invadepoh")
        start = alien.index('    } else if (this->collider.base.acFlags & AC_HIT) {',
                            alien.index('void EnInvadepoh_Alien_Update(Actor* thisx, PlayState* play2) {'))
        branch = block_from(alien, start + 6)
        code += 'static void Test_AlienAcceptedDamage(EnInvadepoh* this, PlayState* play) { Actor* thisx = &this->actor; if (0) {} ' + branch + '\n}\n'
        code += '''static void Test_FinishAlien(void) {
    World(); EnInvadepoh alien = { 0 };
    alien.actor.id = ACTOR_EN_INVADEPOH; alien.actor.category = ACTORCAT_PROP; alien.actor.update = LiveUpdate;
    alien.collider.base.actor = &alien.actor; alien.collider.base.acFlags = AC_HIT;
    alien.collider.base.ac = &player.actor; alien.collider.base.shape = COLSHAPE_CYLINDER;
    alien.collider.elem.acDmgInfo.dmgFlags = 0x3820;
    alien.collider.elem.acHitElem = &player.meleeWeaponQuads[0].elem;
    player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags = DinFireSword_SetDamageFlags(&play, &player, 0, DMG_SWORD);
    DinFireSword_FinishBeginActor(&play, &alien.actor);
    Test_AlienAcceptedDamage(&alien, &play);
    DinFireSword_FinishEndActor(&play, &alien.actor);
    REQUIRE(Effects() == 1 && alien.timer == 8 && alien.actionFunc == EnInvadepoh_Alien_Damaged);
    REQUIRE(alien.actor.category == ACTORCAT_PROP && alien.actor.update == LiveUpdate);
}
static void Test_FinishAdapters(void) { Test_FinishBubble(); Test_FinishRat(); Test_FinishFry(); Test_FinishMinibats(); Test_FinishMajoraOverkill(); Test_FinishNejiron(); Test_FinishAlien(); }
'''
    (directory / "finish_adapters_native.inc").write_text(code)
