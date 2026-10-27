// A grounded Boe's collision body reaches the bolt, despite its origin being below it.
#include "global.h"
#include "mods/items/helpers/target_select_helper.h"
#include <cassert>
#include <cmath>
#include <iostream>

#define TARGETSEL_LIST_HEAD(list) ((list).first)
const u8 gTargetSelectDefaultCats[4] = { ACTORCAT_ENEMY, ACTORCAT_PROP, ACTORCAT_CHEST, ACTORCAT_NPC };
f32 gSfxDefaultFreqAndVolScale = 1;
s8 gSfxDefaultReverb = 0;

f32 Math_SinS(s16 yaw) { return std::sin(yaw * M_PI / 32768.0); }
f32 Math_CosS(s16 yaw) { return std::cos(yaw * M_PI / 32768.0); }
s16 Math_Vec3f_Yaw(Vec3f* from, Vec3f* to) {
    return std::atan2(to->x - from->x, to->z - from->z) * 32768.0 / M_PI;
}
void Audio_PlaySoundGeneral(u16, Vec3f*, u8, f32*, f32*, s8*) {}
void Actor_SetColorFilter(Actor* actor, u16, u16, u16, u16 frames) { actor->colorFilterTimer = frames; }
void LiveEnemy(Actor*, PlayState*) {}

#include "shadow.inc"

void CheckGroundedBody() {
    Player player{};
    PlayState play{};
    Actor boe{};
    boe.update = LiveEnemy;
    boe.world.pos.z = 110;
    // Native EnMkk sColChkInfoInit: radius 15, body height 30; focus is +10.
    boe.colChkInfo.cylRadius = 15;
    boe.colChkInfo.cylHeight = 30;
    boe.focus.pos = {0, 10, 110};
    play.actorCtx.actorLists[ACTORCAT_ENEMY].first = &boe;
    WandShadow_Forget();
    assert(WandShadow_Cast(&player, &play));
    assert(!WandShadow_Cast(&player, &play));
    for (int frame = 0; frame < 12; ++frame) WandShadow_Tick(&play);
    assert(boe.freezeTimer == 120 && "a bolt passing through a grounded Boe body must stun");
    assert(boe.colorFilterTimer == 120 && !sShadowBolt.active);
}

void CheckVerticalAndHorizontalMisses() {
    Player player{};
    PlayState play{};
    Actor enemy{};
    enemy.update = LiveEnemy;
    enemy.world.pos = {0, 100, 110};
    enemy.colChkInfo.cylHeight = 30;
    play.actorCtx.actorLists[ACTORCAT_ENEMY].first = &enemy;
    WandShadow_Forget();
    assert(WandShadow_Cast(&player, &play));
    for (int frame = 0; frame < 12; ++frame) WandShadow_Tick(&play);
    assert(enemy.freezeTimer == 0 && sShadowBolt.active);

    WandShadow_Forget();
    play.actorCtx.actorLists[ACTORCAT_ENEMY].first = nullptr;
    assert(WandShadow_Cast(&player, &play));
    enemy.world.pos = {23, 0, 41};
    enemy.colChkInfo.cylHeight = 30;
    play.actorCtx.actorLists[ACTORCAT_ENEMY].first = &enemy;
    WandShadow_Tick(&play);
    assert(enemy.freezeTimer == 0 && "the horizontal radius must stay 22");
}

void CheckLiveAndUnsetBody() {
    Player player{};
    PlayState play{};
    Actor enemy{};
    enemy.world.pos = {0, 0, 41};
    enemy.colChkInfo.cylHeight = 30;
    play.actorCtx.actorLists[ACTORCAT_ENEMY].first = &enemy;
    WandShadow_Forget();
    assert(WandShadow_Cast(&player, &play));
    WandShadow_Tick(&play);
    assert(enemy.freezeTimer == 0 && sShadowBolt.target == nullptr);

    enemy.update = LiveEnemy;
    enemy.colChkInfo.cylHeight = 0;
    WandShadow_Forget();
    assert(WandShadow_Cast(&player, &play));
    WandShadow_Tick(&play);
    assert(enemy.freezeTimer == 0);
    enemy.world.pos.y = 25;
    WandShadow_Forget();
    assert(WandShadow_Cast(&player, &play));
    enemy.update = nullptr;
    WandShadow_Tick(&play);
    assert(enemy.freezeTimer == 0 && sShadowBolt.active);

    enemy.update = LiveEnemy;
    enemy.world.pos.z = sShadowBolt.pos.z + 11;
    WandShadow_Tick(&play);
    assert(enemy.freezeTimer == 120 && !sShadowBolt.active);
}

int main() {
    CheckGroundedBody();
    CheckVerticalAndHorizontalMisses();
    CheckLiveAndUnsetBody();
    std::cout << "PASS Shadow native body hit, vertical/horizontal misses, live/dead and zero-height targets\n";
}
