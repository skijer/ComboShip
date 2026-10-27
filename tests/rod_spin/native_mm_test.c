// Production MM spawn and native charge-entry gates; actor/input hooks are
// boundaries.
#include "functions.h"
#include "mods/extended_player.h"
#include "overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

SaveContext gSaveContext;
Input *sPlayerControlInput;
u8 gNeiMasterBeamFree;
u16 sPlayerItemButtons[] = {BTN_B, BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT};
u16 sDpadItemButtons[] = {BTN_DRIGHT, BTN_DLEFT, BTN_DDOWN, BTN_DUP};
enum {
  VB_MAGIC_SPIN_ATTACK_CHECK_FORM = 1,
  VB_CHECK_HELD_ITEM_BUTTON_PRESS = 2
};
static Player *localPlayer;
static int enhancement, trueMaster, spawnCount, spawnedFree, nativeCharges,
    zoraCharges, consumed;
static s16 spawnedPitch;
static s32 consumedType;
static Actor spawned;

s32 GameInteractor_Should(int behavior, s32 original, ...) {
  if (behavior != VB_CHECK_HELD_ITEM_BUTTON_PRESS || !enhancement)
    return original;
  if (localPlayer->heldItemButton < 0)
    return false;
  // Existing TwoHandedSwordSpinAttack input policy at its hook boundary.
  u16 mask =
      IS_HELD_DPAD(localPlayer->heldItemButton)
          ? sDpadItemButtons[HELD_ITEM_TO_DPAD(localPlayer->heldItemButton)]
          : sPlayerItemButtons[localPlayer->heldItemButton];
  return CHECK_BTN_ALL(sPlayerControlInput->cur.button, mask);
}
bool Player_IsZTargeting(Player *player) { return false; }
s16 Math_Vec3f_Pitch(Vec3f *a, Vec3f *b) { return 1234; }
s32 WeaponUpgrade_HasTrueMaster(void) { return trueMaster; }
s32 Magic_Consume(PlayState *play, s16 cost, s16 type) {
  consumed += cost;
  consumedType = type;
  return true;
}
Actor *Actor_Spawn(ActorContext *actorCtx, PlayState *play, s16 actorId, f32 x,
                   f32 y, f32 z, s16 rx, s16 ry, s16 rz, s32 params) {
  assert(actorId == ACTOR_EN_M_THUNDER);
  ++spawnCount;
  spawned.params = params;
  spawnedPitch = rx;
  spawnedFree = gNeiMasterBeamFree;
  return &spawned;
}
s32 func_80830E30(Player *player, PlayState *play) {
  ++zoraCharges;
  return true;
}
void func_808335B0(PlayState *play, Player *player) { ++nativeCharges; }

#include "production_spin.inc"

static void check_spawn(PlayState *play, Player *player, s32 cost, s32 beam,
                        int expectedType) {
  spawnCount = 0;
  func_808332A0(play, player, cost, beam);
  assert(spawnCount == 1);
  int type = ENMTHUNDER_GET_TYPE(&spawned);
  // Check bounds before evaluating either native damage table access.
  assert(type >= 0 && type < ARRAY_COUNT(sDamages) / 2);
  assert(type + ARRAY_COUNT(sDamages) / 2 < ARRAY_COUNT(sDamages));
  assert(type == expectedType &&
         ENMTHUNDER_GET_MAGIC_COST(&spawned) == (cost >> 8));
  assert(sDamages[type] == expectedType + 1 &&
         sDamages[type + 4] == expectedType + 1);
}

int main(int argc, char **argv) {
  Player player = {0};
  PlayState play = {0};
  Input input = {0};
  Actor target = {0};
  localPlayer = &player;
  sPlayerControlInput = &input;
  player.actor.id = ACTOR_PLAYER;
  player.transformation = PLAYER_FORM_HUMAN;
  const int rods[] = {PLAYER_IA_ROD_FIRE, PLAYER_IA_ROD_ICE,
                      PLAYER_IA_ROD_LIGHT};
  if (argc > 1 && !strcmp(argv[1], "input"))
    goto input_checks;
  for (int action = PLAYER_IA_SWORD_KOKIRI;
       action <= PLAYER_IA_SWORD_TWO_HANDED; ++action) {
    player.heldItemAction = action;
    for (int cost = 0; cost <= 2; ++cost)
      check_spawn(&play, &player, cost << 8, false,
                  action - PLAYER_IA_SWORD_KOKIRI);
  }
  player.heldItemAction = PLAYER_IA_SWORD_MASTER;
  check_spawn(&play, &player, 2 << 8, false, ENMTHUNDER_TYPE_GILDED_SWORD);
  trueMaster = 1;
  gSaveContext.save.saveInfo.playerData.health =
      gSaveContext.save.saveInfo.playerData.healthCapacity = 48;
  check_spawn(&play, &player, 0, true, ENMTHUNDER_TYPE_GILDED_SWORD);
  assert(spawnedFree && !gNeiMasterBeamFree && consumed == 0);
  trueMaster = 0;
  player.transformation = PLAYER_FORM_FIERCE_DEITY;
  player.heldItemAction = PLAYER_IA_SWORD_TWO_HANDED;
  player.focusActor = &target;
  gSaveContext.save.saveInfo.playerData.magic = 10;
  check_spawn(&play, &player, 0, true, ENMTHUNDER_TYPE_GREAT_FAIRYS_SWORD);
  assert(!spawnedFree && spawnedPitch == 1234 && consumed == 1 &&
         consumedType == MAGIC_CONSUME_DEITY_BEAM);
  player.focusActor = NULL;
  player.transformation = PLAYER_FORM_HUMAN;
  for (unsigned r = 0; r < ARRAY_COUNT(rods); ++r) {
    player.heldItemAction = rods[r];
    assert(Player_GetMeleeWeaponHeld(&player) == PLAYER_MELEEWEAPON_DEKU_STICK);
    for (int cost = 0; cost <= 2; ++cost)
      check_spawn(&play, &player, cost << 8, false,
                  ENMTHUNDER_TYPE_GREAT_FAIRYS_SWORD);
  }
  puts("PASS MM: rod spin types bound both real damage tables; native swords, "
       "Master/FD beams and magic-cost bits preserved");
  if (argc > 1 && !strcmp(argv[1], "spin"))
    return 0;

input_checks:
  int tapFailures = 0;
  const int slots[] = {EQUIP_SLOT_B, EQUIP_SLOT_C_RIGHT,
                       DPAD_TO_HELD_ITEM(EQUIP_SLOT_D_LEFT)};
  for (enhancement = 0; enhancement <= 1; ++enhancement) {
    for (unsigned slot = 0; slot < ARRAY_COUNT(slots); ++slot) {
      player.heldItemButton = slots[slot];
      u16 mask = enhancement
                     ? (IS_HELD_DPAD(slots[slot])
                            ? sDpadItemButtons[HELD_ITEM_TO_DPAD(slots[slot])]
                            : sPlayerItemButtons[slots[slot]])
                     : BTN_B;
      for (unsigned r = 0; r < ARRAY_COUNT(rods); ++r) {
        player.heldItemAction = rods[r];
        player.unk_ADC = 1;
        input.cur.button = input.press.button = mask;
        nativeCharges = 0;
        s32 entered = Player_ActionHandler_8(&player, &play);
        if (entered || nativeCharges != 0) {
          fprintf(stderr,
                  "FAIL rod tap: action=0x%02x heldSlot=%d enhancement=%d "
                  "mask=0x%04x entered=%d\n",
                  rods[r], slots[slot], enhancement, mask, nativeCharges);
          ++tapFailures;
        }
        input.cur.button = input.press.button = 0;
        func_8083A548(&player);
        assert(player.unk_ADC == -1 && !Player_ActionHandler_8(&player, &play));
      }
      player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
      player.unk_ADC = 1;
      input.cur.button = mask;
      nativeCharges = 0;
      assert(Player_ActionHandler_8(&player, &play) && nativeCharges == 1);
      input.cur.button = 0;
      func_8083A548(&player);
      assert(player.unk_ADC == -1 && !Player_ActionHandler_8(&player, &play));
    }
  }
  enhancement = 0;
  player.transformation = PLAYER_FORM_ZORA;
  player.heldItemAction = PLAYER_IA_ZORA_BOOMERANG;
  player.unk_ADC = 2;
  input.cur.button = BTN_B;
  assert(Player_ActionHandler_8(&player, &play) && zoraCharges == 1);
  assert(tapFailures == 0);
  puts(
      "PASS MM: one-frame rod taps at native charge window do not enter sword "
      "charge; B/C/D-pad policy, native sword release and Zora entry retained");
  return 0;
}
