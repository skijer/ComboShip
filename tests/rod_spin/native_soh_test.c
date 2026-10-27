// Production OoT melee classification and spin spawn; renderer/collision remain
// outside this probe.
#include "functions.h"
#include "macros.h"
#include "mods/extended_player.h"
#include "z64.h"
#include <assert.h>
#include <stdio.h>

static int fdSkin, kokiriLevel, spawnCount;
static Actor spawned;
u8 TransformMasks_IsFDSkinMode(void) { return fdSkin; }
u8 TransformMasks_IsTransformed(void) { return fdSkin; }
s32 GerudoMhr_MeleeWeaponIndex(Player *player) { return 0; }
s32 WeaponUpgrade_KokiriLevel(void) { return kokiriLevel; }
Actor *Actor_Spawn(ActorContext *actorCtx, PlayState *play, s16 actorId, f32 x,
                   f32 y, f32 z, s16 rx, s16 ry, s16 rz, s16 params) {
  assert(actorId == ACTOR_EN_M_THUNDER);
  ++spawnCount;
  spawned.params = params;
  return &spawned;
}

#include "production_spin.inc"

static void check_spawn(PlayState *play, Player *player, s32 cost,
                        int expectedType) {
  spawnCount = 0;
  func_80837530(play, player, cost);
  assert(spawnCount == 1);
  int type = (spawned.params & 0xFF) - 1;
  assert(type >= 0 && type < ARRAY_COUNT(sSpinAttackDmgFlags) &&
         type < ARRAY_COUNT(sJumpAttackDmgFlags));
  assert(type == expectedType && (spawned.params & 0xFF00) == cost);
}

int main(void) {
  Player player = {0};
  PlayState play = {0};
  player.actor.category = ACTORCAT_PLAYER;
  const int swords[] = {PLAYER_IA_SWORD_MASTER, PLAYER_IA_SWORD_KOKIRI,
                        PLAYER_IA_SWORD_BIGGORON};
  const int rods[] = {PLAYER_IA_ROD_FIRE, PLAYER_IA_ROD_ICE,
                      PLAYER_IA_ROD_LIGHT};
  for (unsigned s = 0; s < ARRAY_COUNT(swords); ++s) {
    player.heldItemAction = swords[s];
    for (int cost = 0; cost <= 2; ++cost)
      check_spawn(&play, &player, cost << 8, s);
  }
  kokiriLevel = 1;
  player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
  check_spawn(&play, &player, 2 << 8, 0);
  kokiriLevel = 0;
  fdSkin = 1;
  for (unsigned s = 0; s < ARRAY_COUNT(swords); ++s) {
    player.heldItemAction = swords[s];
    check_spawn(&play, &player, 0, 2);
  }
  fdSkin = 0;
  for (unsigned r = 0; r < ARRAY_COUNT(rods); ++r) {
    player.heldItemAction = rods[r];
    assert(Player_GetMeleeWeaponHeld(&player) == 4);
    for (int cost = 0; cost <= 2; ++cost)
      check_spawn(&play, &player, cost << 8, 2);
  }
  player.actor.category = ACTORCAT_NPC;
  spawnCount = 0;
  func_80837530(&play, &player, 2 << 8);
  assert(spawnCount == 0);
  puts("PASS OoT: rod spin types bound real damage-flag arrays; native swords, "
       "sword upgrades, FD identity, actor gate and cost bits preserved");
  return 0;
}
