// Engine services are boundaries; init/charge/destroy, selection and damage
// dispatch below are production code compiled against the actual native ABI.
#include "din_fire_sword.h"
#include "global.h"
#include "mods/nei_save.h"
#include "overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.h"
#include "spin_attack_style.h"
#include "sword_element.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(c)                                                               \
  do {                                                                         \
    if (!(c)) {                                                                \
      fprintf(stderr, "FAIL native spin line %d: %s\n", __LINE__, #c);         \
      abort();                                                                 \
    }                                                                          \
  } while (0)
SaveContext gSaveContext;
f32 gSfxDefaultFreqAndVolScale = 1;
s8 gSfxDefaultReverb;
static PlayState play;
static Player player;
static NeiSaveData nei;
static int style = 6, effects = 1, element, denied, killed, consumes, resets,
           transformed, cone;
static LightNode node;

#ifdef HOST_MM
#include "overlays/actors/ovl_Eff_Dust/z_eff_dust.h"
#define INFO(c) (&(c)->elem)
#define FLAGS(c) ((c)->elem.atDmgInfo.dmgFlags)
#define CHARGE EnMThunder_Charge
#define CHARGE_AMOUNT player.unk_B08
#define SPIN_FLAG PLAYER_STATE2_20000
#define LIST(cat) play.actorCtx.actorLists[cat].first
#define MAGIC_ACQUIRED gSaveContext.save.saveInfo.playerData.isMagicAcquired
#define MAGIC_AMOUNT gSaveContext.save.saveInfo.playerData.magic
#define SCENE play.sceneId
#define FIRE DMG_FIRE_ARROW
#define ICE DMG_ICE_ARROW
#define LIGHT DMG_LIGHT_ARROW
u8 gNeiMasterBeamFree;
s32 CustomForms_ActiveForm(void) { return transformed ? 0 : -1; }
s32 AdultLink_IsActive(void) { return 0; }
void EnMThunder_Spin_Attack(EnMThunder *s, PlayState *p) {
  (void)s;
  (void)p;
}
void EnMThunder_Spin_AttackNoMagic(EnMThunder *s, PlayState *p) {
  (void)s;
  (void)p;
}
void EnMThunder_SwordBeam_Attack(EnMThunder *s, PlayState *p) {
  (void)s;
  (void)p;
}
void EnMThunder_UnkType_Setup(EnMThunder *s, PlayState *p) {
  (void)s;
  (void)p;
}
void EnMThunder_AdjustLights(PlayState *p, f32 f) {
  (void)p;
  (void)f;
}
s32 Magic_Consume(PlayState *p, s16 amount, s16 mode) {
  (void)p;
  ++consumes;
  if (denied || MAGIC_AMOUNT < amount)
    return false;
  if (mode == MAGIC_CONSUME_NOW)
    MAGIC_AMOUNT -= amount;
  return true;
}
void AudioSfx_PlaySfx(u16 a, Vec3f *b, u8 c, f32 *d, f32 *e, s8 *f) {}
void Audio_PlaySfx_SwordCharge(Vec3f *a, u8 b) {}
bool Play_InCsMode(PlayState *p) { return false; }
#else
#define INFO(c) (&(c)->info)
#define FLAGS(c) ((c)->info.toucher.dmgFlags)
#define CHARGE EnMThunder_ChargingSpinAttack
#define CHARGE_AMOUNT player.unk_858
#define SPIN_FLAG PLAYER_STATE2_SPIN_ATTACKING
#define LIST(cat) play.actorCtx.actorLists[cat].head
#define MAGIC_ACQUIRED gSaveContext.isMagicAcquired
#define MAGIC_AMOUNT gSaveContext.magic
#define SCENE play.sceneNum
#define FIRE DMG_ARROW_FIRE
#define ICE DMG_ARROW_ICE
#define LIGHT DMG_ARROW_LIGHT
u8 TransformMasks_IsTransformedAny(void) { return transformed; }
u8 GerudoMhr_UsesConeBurst(Player *p) { return cone; }
void EnMThunder_SpinAttacking(EnMThunder *s, PlayState *p) {
  (void)s;
  (void)p;
}
void EnMThunder_EmptySpinAttack(EnMThunder *s, PlayState *p) {
  (void)s;
  (void)p;
}
static void EnMThunder_SwordBeamAction(EnMThunder *s, PlayState *p) {
  (void)s;
  (void)p;
}
void EnMThunder_AdjustEnvLights(PlayState *p, f32 f) {
  (void)p;
  (void)f;
}
s32 Magic_RequestChange(PlayState *p, s16 amount, s16 mode) {
  (void)p;
  ++consumes;
  if (denied || MAGIC_AMOUNT < amount)
    return false;
  if (mode == MAGIC_CONSUME_NOW)
    MAGIC_AMOUNT -= amount;
  return true;
}
void Audio_PlaySfxGeneral(u16 a, Vec3f *b, u8 c, f32 *d, f32 *e, s8 *f) {}
void func_800F4254(Vec3f *a, u8 b) {}
s32 Play_InCsMode(PlayState *p) { return false; }
#endif

static void live(Actor *a, PlayState *p) {
  (void)a;
  (void)p;
}
NeiSaveData *Nei_Save(void) { return &nei; }
u8 WeaponUpgrade_KokiriLevel(void) { return 0; }
u8 WeaponUpgrade_HasTrueMaster(void) { return 0; }
u8 WeaponUpgrade_HasGreatFairy(void) { return 0; }
u8 ExtEquip_ShouldHideSwordDL(void) { return 0; }
u8 FourSword_IsEquipped(void) { return 0; }
s32 BossRemains_IsOdolwaWorn(void) { return 0; }
s32 BossRemains_IsGohtWorn(void) { return 0; }
int32_t CVarGetInteger(const char *key, int32_t fallback) {
  if (!strcmp(key, CW_SPIN_STYLE_CVAR))
    return style;
  if (!strcmp(key, CW_SPIN_ELEMENT_EFFECTS_CVAR))
    return effects;
  if (!strcmp(key, "gEnhancements.DinFireSwordDamage"))
    return element == 1;
  if (!strcmp(key, "gEnhancements.IceSwordDamage"))
    return element == 2;
  if (!strcmp(key, "gEnhancements.LightSwordDamage"))
    return element == 3;
  return fallback;
}
s32 Collider_InitCylinder(PlayState *p, ColliderCylinder *c) {
  memset(c, 0, sizeof(*c));
  return 1;
}
s32 Collider_SetCylinder(PlayState *p, ColliderCylinder *c, Actor *a,
                         ColliderCylinderInit *init) {
  c->base.actor = a;
#ifdef HOST_MM
  c->elem.atDmgInfo.dmgFlags = init->elem.atDmgInfo.dmgFlags;
#else
  c->info.toucher.dmgFlags = init->info.toucher.dmgFlags;
#endif
  return 1;
}
s32 Collider_DestroyCylinder(PlayState *p, ColliderCylinder *c) { return 1; }
void Lights_PointNoGlowSetInfo(LightInfo *i, s16 x, s16 y, s16 z, u8 r, u8 g,
                               u8 b, s16 rad) {}
LightNode *LightContext_InsertLight(PlayState *p, LightContext *l,
                                    LightInfo *i) {
  return &node;
}
void LightContext_RemoveLight(PlayState *p, LightContext *l, LightNode *n) {}
void Actor_SetScale(Actor *a, f32 s) {
  a->scale.x = a->scale.y = a->scale.z = s;
}
void Actor_Kill(Actor *a) {
  ++killed;
  a->update = NULL;
  a->draw = NULL;
}
void Magic_Reset(PlayState *p) { ++resets; }
void Rumble_Request(f32 dist, u8 intensity, u8 timer, u8 step) {}
Actor *Actor_SpawnAsChild(ActorContext *c, Actor *a, PlayState *p, s16 id,
                          f32 x, f32 y, f32 z, s16 rx, s16 ry, s16 rz,
#ifdef HOST_MM
                          s32 params
#else
                           s16 params
#endif
) {
  return NULL;
}

void CHARGE(EnMThunder *s, PlayState *p);
#include "spin_actor_production.inc"

static void setup(EnMThunder *s) {
  memset(&play, 0, sizeof(play));
  memset(&player, 0, sizeof(player));
  memset(&gSaveContext, 0, sizeof(gSaveContext));
  memset(&nei, 0, sizeof(nei));
  memset(s, 0, sizeof(*s));
  LIST(ACTORCAT_PLAYER) = &player.actor;
  LIST(ACTORCAT_ITEMACTION) = &s->actor;
  player.actor.scale.y = 1;
  player.heldItemId = ITEM_SWORD_KOKIRI;
  player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
  player.stateFlags1 = PLAYER_STATE1_CHARGING_SPIN_ATTACK;
#ifdef HOST_MM
  player.transformation = PLAYER_FORM_HUMAN;
  player.meleeWeaponAnimation = PLAYER_MWA_SPIN_ATTACK_1H;
  s->actor.params = 0x100;
#else
  player.meleeWeaponAnimation = 0x18;
  s->actor.params = 0x101;
#endif
  s->actor.id = ACTOR_EN_M_THUNDER;
  s->actor.category = ACTORCAT_ITEMACTION;
  s->actor.update = live;
  MAGIC_ACQUIRED = 1;
  MAGIC_AMOUNT = 30;
  style = 6;
  effects = 1;
  element = 0;
  denied = killed = consumes = resets = transformed = cone = 0;
}

int main(void) {
  EnMThunder s;
  setup(&s);
  element = 1;
  EnMThunder_Init(&s.actor, &play);
  CHECK(s.spinStyle.style == CW_SPIN_STYLE_ASTRAL && s.spinStyle.element == 1 &&
        !s.spinStyle.released);
  style = 7;
  effects = 0;
  CHARGE_AMOUNT = .5f;
  CHARGE(&s, &play);
  CHECK(s.spinStyle.style == CW_SPIN_STYLE_ASTRAL &&
        s.spinStyle.effectsEnabled == 1);
  CHECK(consumes == 1 && MAGIC_AMOUNT == 30);
  element = 2;
  play.pauseCtx.state = 1;
  CHARGE(&s, &play);
  CHECK(s.spinStyle.element == 1);
  play.pauseCtx.state = 0;
  CHARGE(&s, &play);
  CHECK(s.spinStyle.element == 2);
  player.stateFlags2 |= SPIN_FLAG;
  CHARGE(&s, &play);
  CHECK(s.spinStyle.released && s.spinStyle.element == 2);
  CHECK(FLAGS(&s.collider) == (s.spinStyle.originalFlags | ICE));
  CHECK(SpinAttack_OriginalDamageFlags(&play, INFO(&s.collider)) ==
        s.spinStyle.originalFlags);
  CHECK(DinFireSword_OriginalDamageFlags(&play, INFO(&s.collider)) ==
        s.spinStyle.originalFlags);
  s.actor.update = NULL;
  CHECK(SpinAttack_OriginalDamageFlags(&play, INFO(&s.collider)) ==
        FLAGS(&s.collider));
  s.actor.update = live;
  LIST(ACTORCAT_ITEMACTION) = NULL;
  CHECK(SpinAttack_OriginalDamageFlags(&play, INFO(&s.collider)) ==
        FLAGS(&s.collider));
  LIST(ACTORCAT_ITEMACTION) = &s.actor;
  element = 3;
  EnMThunder_PreviewStyle(&s, &play);
  CHECK(s.spinStyle.element == 2);
  CHECK(EnMThunder_VisualStyle(&s, &play).accentElement == 2);
  ColliderCylinder unrelated = s.collider;
  CHECK(SpinAttack_OriginalDamageFlags(&play, INFO(&unrelated)) ==
        FLAGS(&unrelated));
  ++SCENE;
  CHECK(SpinAttack_OriginalDamageFlags(&play, INFO(&s.collider)) ==
        FLAGS(&s.collider));
  --SCENE;
  const u32 original = s.spinStyle.originalFlags;
  EnMThunder_Destroy(&s.actor, &play);
  CHECK(!s.spinStyle.eligible && FLAGS(&s.collider) == original);

  for (int variant = 0; variant < CW_SPIN_STYLE_COUNT; ++variant) {
    setup(&s);
    style = variant;
    player.stateFlags2 = SPIN_FLAG;
    EnMThunder_Init(&s.actor, &play);
    CHECK(s.spinStyle.released && s.spinStyle.element == 0 && consumes == 1);
    CHECK(EnMThunder_VisualStyle(&s, &play).style == variant);
  }
#ifdef HOST_MM
  for (int upgrade = 0; upgrade <= 1; ++upgrade) {
    setup(&s);
    element = 1;
    player.stateFlags2 = SPIN_FLAG;
    if (upgrade)
      WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK >> 8) |= WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK & 0xFF;
    EnMThunder_Init(&s.actor, &play);
    CHECK(s.subtype == (upgrade ? 0 : 1) && s.scaleTarget == (upgrade ? 3 : 2));
    CHECK(s.spinStyle.released &&
          FLAGS(&s.collider) == (s.spinStyle.originalFlags | FIRE));
  }
  setup(&s);
  element = 1;
  player.stateFlags2 = SPIN_FLAG;
  player.meleeWeaponAnimation = 0;
  EnMThunder_Init(&s.actor, &play);
  CHECK(s.subtype >= 2 && FLAGS(&s.collider) == DMG_SWORD_BEAM &&
        !s.spinStyle.released);
  CHECK(EnMThunder_VisualStyle(&s, &play).style == CW_SPIN_STYLE_NATIVE);
#else
  setup(&s);
  element = 1;
  s.actor.params |= EN_M_THUNDER_SWORD_BEAM_FLAG;
  EnMThunder_Init(&s.actor, &play);
  CHECK(s.attackStrength == 2 && FLAGS(&s.collider) == 0x02000000 &&
        !s.spinStyle.released && consumes == 0);
  CHECK(EnMThunder_VisualStyle(&s, &play).style == CW_SPIN_STYLE_NATIVE);
  setup(&s);
  element = 1;
  cone = 1;
  player.stateFlags2 = SPIN_FLAG;
  EnMThunder_Init(&s.actor, &play);
  CHECK(!s.spinStyle.eligible && s.spinStyle.element == 0);
  CHECK(EnMThunder_VisualStyle(&s, &play).style == CW_SPIN_STYLE_NATIVE);
#endif

  setup(&s);
  MAGIC_ACQUIRED = 0;
  player.stateFlags2 = SPIN_FLAG;
  EnMThunder_Init(&s.actor, &play);
  CHECK(killed == 1 && !s.spinStyle.released);
  setup(&s);
  MAGIC_AMOUNT = 0;
  player.stateFlags2 = SPIN_FLAG;
  EnMThunder_Init(&s.actor, &play);
  CHECK(killed == 1 && !s.spinStyle.released);
  setup(&s);
  denied = 1;
  player.stateFlags2 = SPIN_FLAG;
  EnMThunder_Init(&s.actor, &play);
  CHECK(killed == 1 && !s.spinStyle.released);

  for (int e = 0; e <= 3; ++e)
    for (int great = 0; great <= 1; ++great) {
      setup(&s);
      element = e;
      effects = 0;
      CHARGE_AMOUNT = great ? 1.0f : .5f;
      EnMThunder_Init(&s.actor, &play);
      player.stateFlags2 = SPIN_FLAG;
      CHARGE(&s, &play);
      const u32 added = e == 1 ? FIRE : e == 2 ? ICE : e == 3 ? LIGHT : 0;
      CHECK(s.spinStyle.released &&
            FLAGS(&s.collider) == (s.spinStyle.originalFlags | added));
      CHECK(s.collider.dim.height == 40 && s.collider.dim.yShift == -20);
#ifdef HOST_MM
      CHECK(s.subtype == (great ? 0 : 1));
      CHECK(s.scaleTarget == (great ? 3 : 2));
      CHECK(s.collider.elem.atDmgInfo.damage == 1 && s.timer == 8);
#else
      CHECK(s.attackStrength == (great ? 0 : 1));
      CHECK(s.targetScale == (great ? 8 : 4));
      CHECK(s.followPlayerTimer == 8);
#endif
      Actor target = {0};
      DamageTable table;
      memset(&table, 4, sizeof(table));
      target.colChkInfo.damageTable = &table;
#ifdef HOST_MM
      table.attack[11] = 0xE1;
      table.attack[12] = 0x31;
      table.attack[13] = 0x41;
#else
      table.table[11] = 0xE1;
      table.table[12] = 0x31;
      table.table[13] = 0x41;
#endif
      target.id = ACTOR_EN_FIREFLY;
      const u8 result =
          DinFireSword_DamageEntry(&play, &target, INFO(&s.collider), ~0u, 4);
      CHECK((result & 15) == 4);
      CHECK(e == 0   ? result == 4
            : e == 1 ? (result >> 4) == 14
            : e == 2 ? (result >> 4) == 3
                     : 1);
      target.id = ACTOR_EN_DEKUBABA;
      CHECK(DinFireSword_DamageEntry(&play, &target, INFO(&s.collider),
                                     s.spinStyle.originalFlags, 4) == 4);
    }
  setup(&s);
  transformed = 1;
  element = 1;
  player.stateFlags2 = SPIN_FLAG;
  EnMThunder_Init(&s.actor, &play);
  CHECK(!s.spinStyle.eligible && s.spinStyle.element == 0);
  setup(&s);
  Actor foreign = {0};
  s.actor.parent = &foreign;
  element = 1;
  player.stateFlags2 = SPIN_FLAG;
  EnMThunder_Init(&s.actor, &play);
  CHECK(!s.spinStyle.eligible && s.spinStyle.element == 0);
  puts("PASS native spin gates, independent styles, release snapshots, exact "
       "collider ownership and damage parity");
  return 0;
}
