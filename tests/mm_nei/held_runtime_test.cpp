#define MM_REAL_RENDERER
#include "2s2h/Rando/NeiHeldPresentation.h"
#include "mods/items/logic/item_beetle.h"
#include "mods/items/logic/item_time_gate.h"
#include "mods/forms/custom_forms.h"
#include "rod_runtime_test.cpp"
extern "C" {
#include "mods/actors/somaria_cubes.h"
#include "mods/items/logic/item_cane_of_somaria.h"
}
#include "2s2h/Rando/NeiArticulatedPresentation.h"
std::vector<std::string> models;
std::string missing;
int caneType = 0;
std::vector<void *> allocations;
extern "C" {
void RigFit_Set(u8 ready, u8 child, s32 adult, s32 custom);
void *Graph_Alloc(GraphicsContext *, size_t size) {
  void *p = calloc(1, size);
  allocations.push_back(p);
  return p;
}
u8 Cane_GetType() { return caneType; }
u8 Cane_GetActiveSkill() { return 0; }
u8 Cane_IsAiming() { return 0; }
void Pacci_UltrahandDrawVfx(PlayState *, Player *) {}
void Pacci_FuseDrawPreview(PlayState *) {}
void PacciFlipVfx_Draw(PlayState *, Player *) {}
void CaneSummon_DrawPreview(PlayState *, CaneSummonKind, Vec3f *, s16, u8) {}
f32 Math_SinS(s16 v) { return sinf(v * M_PI / 32768); }
f32 Math_CosS(s16 v) { return cosf(v * M_PI / 32768); }
f32 Math_FAtan2F(f32 y, f32 x) { return atan2f(y, x); }
bool NeiHeld_HasResources(const char *path, const char *) {
  return resources && missing != path;
}
bool NeiHeld_DrawModel(PlayState *, const char *p, const char *) {
  if (!NeiHeld_HasResources(p, nullptr))
    return false;
  models.push_back(p);
  poses.push_back(current);
  return true;
}
bool NeiHeld_DrawMitts(Player *, PlayState *) {
  if (!resources)
    return false;
  models.push_back("mitts");
  return true;
}
bool NeiHeld_DrawGustJar(Player *, PlayState *, int, float) {
  if (!resources)
    return false;
  models.push_back("jar");
  return true;
}
void NeiUsedMagic_DrawPortal(PlayState *, Player *, float, float) {
  models.push_back("portal");
}
}
int main() {
  Player p{};
  PlayState play{};
  GraphicsContext gfx{};
  Gfx opa[8192]{}, xlu[8192]{}, overlay[8192]{};
  play.state.gfxCtx = &gfx;
  p.transformation = PLAYER_FORM_HUMAN;
  p.actor.scale = {.01f, .01f, .01f};
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &p.actor;
  gPlayState = &play;
  gCustomItemState.ballAndChainThrown = 1;
  gCustomItemState.timer2 = 1;
  gfx.polyOpa.p = opa;
  gfx.polyOpa.d = opa + 8192;
  gfx.polyXlu.p = xlu;
  gfx.polyXlu.d = xlu + 8192;
  gfx.overlay.p = overlay;
  gfx.overlay.d = overlay + 8192;
  CustomItems_DrawBallChain(&p, &play);
  assert(models.size() == 1 && models[0] == NEI_HELD_PATH("ball"));

  auto reset = [&]() {
    models.clear();
    poses.clear();
    nativeDraws = 0;
    gfx.polyOpa.p = opa;
    gfx.polyXlu.p = xlu;
    gfx.overlay.p = overlay;
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
  };
  for (int age : {0, 1}) {
    gSaveContext.save.linkAge = 0;
    RigFit_Set(1, age, !age, CUSTOM_FORM_NONE);
    reset();
    gCustomItemState.dekuLeafBlowing = 1;
    CustomItems_DrawDekuLeaf(&p, &play);
    assert(models == std::vector<std::string>{NEI_HELD_PATH("deku_leaf")});
    reset();
    gCustomItemState.dekuLeafBlowing = 0;
    gCustomItemState.dekuLeafGliding = 1;
    CustomItems_DrawDekuLeaf(&p, &play);
    assert(models == std::vector<std::string>{NEI_HELD_PATH("deku_leaf")});
    reset();
    gCustomItemState.shovelActive = 1;
    CustomItems_DrawShovel(&p, &play);
    assert(models == std::vector<std::string>{NEI_HELD_PATH("shovel")});
    auto m = poses[0];
    float scale = sqrtf(m.xx * m.xx + m.yx * m.yx + m.zx * m.zx);
    assert(fabs(scale - (age ? .048f : .054f)) < .00001f);
    Player peer = p;
    reset();
    CustomItems_DrawShovel(&peer, &play);
    m = poses[0];
    scale = sqrtf(m.xx * m.xx + m.yx * m.yx + m.zx * m.zx);
    assert(fabs(scale - .048f) < .00001f);
    reset();
    gCustomItemState.mogmaMittsActive = 1;
    CustomItems_DrawMogmaMitts(&p, &play);
    assert(models == std::vector<std::string>{"mitts"});
    reset();
    gCustomItemState.gustJarEquipped = 1;
    CustomItems_DrawGustJar(&p, &play);
    assert(models == std::vector<std::string>{"jar"});
    for (int state :
         {BEETLE_STATE_AIMING, BEETLE_STATE_FLYING, BEETLE_STATE_RETURNING}) {
      reset();
      beetleActive = 1;
      beetleState = state;
      CustomItems_DrawBeetle(&p, &play);
      assert(
          (models == std::vector<std::string>{NEI_HELD_PATH("beetle_body"),
                                              NEI_HELD_PATH("beetle_wings")}));
      missing = NEI_HELD_PATH("beetle_wings");
      reset();
      CustomItems_DrawBeetle(&p, &play);
      assert(models.empty() && nativeDraws == 2);
      missing.clear();
    }
    reset();
    gCustomItemState.spinnerActive = 1;
    play.gameplayFrames = 8;
    p.actor.world.pos = {12, 25, 37};
    CustomItems_DrawSpinner(&p, &play);
    assert(models == std::vector<std::string>{NEI_HELD_PATH("spinner")});
    near(transform(poses[0], {0, 0, 0}), p.actor.world.pos);
    near(transform(poses[0], {0, 0, 10}), {14, 25, 37});
    for (int type = 0; type < 4; type++) {
      reset();
      Matrix_Scale(.01f, .01f, .01f, MTXMODE_APPLY);
      MtxF wrist = current;
      ItemEquip_CaptureHandMatrix();
      gCustomItemState.somariaActive = 1;
      caneType = type;
      CustomItems_DrawCaneOfSomaria(&p, &play);
      if (type == CANE_TYPE_SOMARIA)
        assert(models ==
               std::vector<std::string>{NEI_HELD_PATH("cane_of_somaria")});
      else
        assert(models.empty());
      if (type == CANE_TYPE_ULTRAHAND)
        assert(nativeDraws == 0);
      if (type == CANE_TYPE_SOMARIA) {
        near(transform(poses[0], {0, 0, 0}),
             transform(wrist, {0, age ? 216.22f : 328.f,
                               age ? -4.5f : 77.f}));
        reset();
        Matrix_Scale(.01f, .01f, .01f, MTXMODE_APPLY);
        wrist = current;
        ItemEquip_CaptureHandMatrix();
        CustomItems_DrawCaneOfSomaria(&peer, &play);
        assert(models ==
               std::vector<std::string>{NEI_HELD_PATH("cane_of_somaria")});
        near(transform(poses[0], {0, 0, 0}),
             transform(wrist, {0, 216.22f, -4.5f}));
      }
    }
    reset();
    tgItemVisible = 1;
    tgActive = 1;
    CustomItems_DrawTimeGate(&p, &play);
    assert(models.empty() && nativeDraws == 0);
    tgPortalActive = 1;
    tgPortalAlpha = 230;
    tgPortalScale = 1;
    CustomItems_DrawTimeGatePortal(&p, &play);
    assert(models == std::vector<std::string>{"portal"});
    reset();
    tgActive = 0;
    CustomItems_DrawTimeGate(&p, &play);
    assert(models == std::vector<std::string>{NEI_HELD_PATH("time_gate")});
    for (int state = 1; state <= 6; state++) {
      reset();
      Matrix_Scale(.01f, .01f, .01f, MTXMODE_APPLY);
      ItemEquip_CaptureHandMatrix();
      gCustomItemState.whipActive = 1;
      gCustomItemState.whipState = state;
      gCustomItemState.whipTipPos = {40, 20, 50};
      gCustomItemState.whipAttachPos = {45, 25, 55};
      CustomItems_DrawWhip(&p, &play);
      assert(models.front() == (state == 1 ? NEI_HELD_PATH("whip")
                                           : NEI_HELD_PATH("whip_handle")));
      if (state != 1) {
        assert(models[1] == NEI_HELD_PATH("whip_segment"));
        assert(models.back() == NEI_HELD_PATH("whip_tip"));
        Vec3f socket = transform(poses[0], {0, 7.2f, 0});
        near(transform(poses[1], {0, 0, -500}), socket);
        for (size_t i = 1; i + 2 < poses.size(); i++)
          near(transform(poses[i], {0, 0, 500}),
               transform(poses[i + 1], {0, 0, -500}));
        Vec3f end = (state == 4 || state == 5) ? gCustomItemState.whipAttachPos
                                               : gCustomItemState.whipTipPos;
        near(transform(poses[poses.size() - 2], {0, 0, 500}), end);
        near(transform(poses.back(), {0, 0, 0}), end);
      }
      assert(matrices.empty());
    }
  }
  // A local adult-only custom form leaves a native peer on child object fits.
  RigFit_Set(1, 0, 0, CUSTOM_FORM_KEATON);
  Player customPeer = p;
  auto customScale = [](const MtxF &m) {
    return sqrtf(m.xx * m.xx + m.yx * m.yx + m.zx * m.zx);
  };
  reset();
  CustomItems_DrawShovel(&p, &play);
  assert(fabs(customScale(poses[0]) - .054f) < .00001f);
  reset();
  CustomItems_DrawShovel(&customPeer, &play);
  assert(fabs(customScale(poses[0]) - .048f) < .00001f);
  caneType = CANE_TYPE_SOMARIA;
  reset();
  Matrix_Scale(.01f, .01f, .01f, MTXMODE_APPLY);
  MtxF customWrist = current;
  ItemEquip_CaptureHandMatrix();
  CustomItems_DrawCaneOfSomaria(&p, &play);
  near(transform(poses[0], {0, 0, 0}),
       transform(customWrist, {0, 328.f, 77.f}));
  reset();
  Matrix_Scale(.01f, .01f, .01f, MTXMODE_APPLY);
  customWrist = current;
  ItemEquip_CaptureHandMatrix();
  CustomItems_DrawCaneOfSomaria(&customPeer, &play);
  near(transform(poses[0], {0, 0, 0}),
       transform(customWrist, {0, 216.22f, -4.5f}));
  // Released and nonhuman wrists take the legacy branch, never an unrelated
  // actor's pose.
  for (bool nonhuman : {false, true}) {
    reset();
    caneType = CANE_TYPE_SOMARIA;
    if (nonhuman) {
      ItemEquip_CaptureHandMatrix();
      p.transformation = PLAYER_FORM_DEKU;
    } else
      ItemEquip_ReleaseHandMatrix();
    CustomItems_DrawCaneOfSomaria(&p, &play);
    assert(models.empty() && nativeDraws > 0);
  }
  p.transformation = PLAYER_FORM_HUMAN;
  // Every connected drawer keeps a native fallback when the bundle is absent.
  resources = false;
  gCustomItemState.whipState = 1;
  for (auto draw : {CustomItems_DrawBallChain, CustomItems_DrawDekuLeaf,
                    CustomItems_DrawShovel, CustomItems_DrawMogmaMitts,
                    CustomItems_DrawGustJar, CustomItems_DrawBeetle,
                    CustomItems_DrawSpinner, CustomItems_DrawCaneOfSomaria,
                    CustomItems_DrawTimeGate, CustomItems_DrawWhip}) {
    reset();
    draw(&p, &play);
    assert(models.empty() && nativeDraws > 0);
  }
  resources = true;
  // Switch Hook hand compound preserves the resolved native fist and per-draw
  // allocation.
  p.heldItemId = ITEM_SWITCH_HOOK;
  Gfx fist[1]{}, *limb = fist;
  Actor hook{};
  p.heldActor = &hook;
  assert(NeiArticulated_ApplySwitchHookHand(&play, &p, &limb, fist));
  auto docked = limb;
  assert(limb[0].words.w1 == (uintptr_t)fist);
  assert(std::string((char *)limb[1].words.w1) ==
         "__OTR__@oot:objects/nei_held_redesign/switch_hook/gi_dl");
  reset();
  assert(NeiArticulated_DrawSwitchHookTip(&play, &p, &hook));
  assert(models.empty());
  p.heldActor = nullptr;
  assert(NeiArticulated_ApplySwitchHookHand(&play, &p, &limb, fist));
  assert(limb != docked);
  assert(std::string((char *)limb[1].words.w1) ==
         "__OTR__@oot:objects/nei_held_redesign/switch_hook_body/gi_dl");
  reset();
  assert(NeiArticulated_DrawSwitchHookTip(&play, &p, &hook));
  assert(models == std::vector<std::string>{NEI_HELD_PATH("switch_hook_tip")});
  for (auto path :
       {NEI_HELD_PATH("switch_hook"), NEI_HELD_PATH("switch_hook_body"),
        NEI_HELD_PATH("switch_hook_tip")}) {
    missing = path;
    assert(!NeiArticulated_ApplySwitchHookHand(&play, &p, &limb, fist));
    assert(!NeiArticulated_DrawSwitchHookTip(&play, &p, &hook));
  }
  missing.clear();
  for (int form = 0; form < PLAYER_FORM_HUMAN; form++) {
    p.transformation = form;
    assert(!NeiArticulated_UsesSwitchHook(&p));
  }
  p.transformation = PLAYER_FORM_HUMAN;
  for (auto path : {NEI_HELD_PATH("whip"), NEI_HELD_PATH("whip_handle"),
                    NEI_HELD_PATH("whip_segment"), NEI_HELD_PATH("whip_tip")}) {
    missing = path;
    assert(!NeiArticulated_HasWhip());
  }
  missing.clear();
  for (void *ptr : allocations)
    free(ptr);
  std::cout << "PASS native MM held model dispatch\n";
}
