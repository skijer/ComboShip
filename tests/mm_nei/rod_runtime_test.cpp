// Native MM production drawers compiled against MM's real Player/PlayState/GBI.
#include "mods/items/custom_items.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/items/logic/item_rod_common.h"
#include "mods/forms/custom_forms.h"
#include "mods/nei_oot_compat.h"
#include "variables.h"
#include "z64.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
extern "C" void ItemEquip_CaptureLeftHandMatrix(void) __attribute__((weak));
extern "C" void ItemEquip_ReleaseHandMatrix(void) __attribute__((weak));
namespace {
MtxF current{};
std::vector<MtxF> matrices, poses;
std::vector<int> held, shots, trails;
std::vector<std::pair<const void *, int>> children;
RodProjSet sets[3][5]{};
bool local[3]{};
int nativeDraws = 0;
#ifdef MM_REAL_RENDERER
std::vector<const Gfx*> nativeDisplayLists;
#endif
bool resources = true;
} // namespace
extern "C" {
PlayState *gPlayState = nullptr;
void RigFit_Set(u8 ready, u8 child, s32 adult, s32 custom);
SaveContext gSaveContext{};
CustomItemState gCustomItemState{};
u8 FireRod_HasAnyActiveSet() { return local[0]; }
u8 IceRod_HasAnyActiveSet() { return local[1]; }
u8 LightRod_HasAnyActiveSet() { return local[2]; }
RodProjSet *FireRod_GetProjSets() { return sets[0]; }
RodProjSet *IceRod_GetProjSets() { return sets[1]; }
RodProjSet *LightRod_GetProjSets() { return sets[2]; }
u8 ResourceMgr_FileExists(const char *) { return 1; }
int ResourceMgr_IsModAssetForGame(const char*, const char*) { return 0; }
Gfx *ResourceMgr_LoadGfxByName(const char *) {
  static Gfx dl[1];
  return dl;
}
void *OotAssets_LoadGfx(const char* path) {
#ifdef NEI_GI_REWARD_FIXTURE
  extern void* RewardFixture_LoadNativeGfx(const char*);
  return RewardFixture_LoadNativeGfx(path);
#else
  static Gfx dl[1];
  return dl;
#endif
}
void Matrix_Get(MtxF *p) { *p = current; }
void Matrix_Put(MtxF *p) { current = *p; }
void Matrix_Push() { matrices.push_back(current); }
void Matrix_Pop() {
  assert(!matrices.empty());
  current = matrices.back();
  matrices.pop_back();
}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode) {
  current.xx *= x;
  current.yx *= x;
  current.zx *= x;
  current.xy *= y;
  current.yy *= y;
  current.zy *= y;
  current.xz *= z;
  current.yz *= z;
  current.zz *= z;
}
void Matrix_MultVec3f(Vec3f *a, Vec3f *b) {
  *b = {current.xx * a->x + current.xy * a->y + current.xz * a->z + current.xw,
        current.yx * a->x + current.yy * a->y + current.yz * a->z + current.yw,
        current.zx * a->x + current.zy * a->y + current.zz * a->z + current.zw};
}
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode mode) {
  if (mode == MTXMODE_NEW) {
    current = {};
    current.xx = current.yy = current.zz = current.ww = 1;
  }
  Vec3f a{x, y, z}, b;
  Matrix_MultVec3f(&a, &b);
  current.xw = b.x;
  current.yw = b.y;
  current.zw = b.z;
}
void Matrix_RotateXF(f32, MatrixMode);
void Matrix_RotateYF(f32, MatrixMode);
void Matrix_RotateZF(f32, MatrixMode);
void Matrix_ReplaceRotation(MtxF *) {}
Mtx *Matrix_Finalize(GraphicsContext *gfx) {
#ifdef MM_REAL_RENDERER
  return static_cast<Mtx *>(GRAPH_ALLOC(gfx, sizeof(Mtx)));
#else
  static Mtx m;
  return &m;
#endif
}
#ifdef MM_REAL_RENDERER
#include "mm_nei_graph.inc"
static Gfx setupDl;
void Gfx_SetupDL25_Opa(GraphicsContext *gfx) {
  OPEN_DISPS(gfx);
  __gSPDisplayList(POLY_OPA_DISP++, &setupDl);
  CLOSE_DISPS(gfx);
}
void Gfx_SetupDL25_Xlu(GraphicsContext *gfx) {
  OPEN_DISPS(gfx);
  __gSPDisplayList(POLY_XLU_DISP++, &setupDl);
  CLOSE_DISPS(gfx);
}
#else
void Gfx_SetupDL25_Opa(GraphicsContext *) {}
void Gfx_SetupDL25_Xlu(GraphicsContext *) {}
void Graph_OpenDisps(Gfx **, Gfx *, GraphicsContext *, const char *, s32) {}
void Graph_CloseDisps(Gfx **, Gfx *, GraphicsContext *, const char *, s32) {}
#endif
void Gfx_SetupDL26_Opa(GraphicsContext *gfx) { Gfx_SetupDL25_Opa(gfx); }
void gSPDisplayList(Gfx *, Gfx *list) {
  ++nativeDraws;
#ifdef MM_REAL_RENDERER
  nativeDisplayLists.push_back(list);
#endif
}
void gSPSegment(void *, int, uintptr_t) {}
Gfx *Gfx_TwoTexScroll(GraphicsContext *, s32, u32, u32, s32, s32, s32, u32, u32,
                      s32, s32) {
  return nullptr;
}
s16 Camera_GetCamDirYaw(Camera *) { return 0; }
f32 Rand_ZeroOne() { return 0; }
void FrameInterpolation_RecordOpenChild(const void *p, int id) {
  children.push_back({p, id});
}
void FrameInterpolation_RecordCloseChild() {}

#ifndef MM_REAL_RENDERER
bool NeiHeld_DrawRod(PlayState *, int element) {
  if (!resources)
    return false;
  held.push_back(element);
  poses.push_back(current);
  return true;
}
void NeiUsedMagic_DrawProjectile(PlayState *, int e, const Vec3f *,
                                 const Vec3f *, float, unsigned) {
  shots.push_back(e);
}
void NeiUsedMagic_DrawTrail(PlayState *, int e, const Vec3f *, unsigned count,
                            float) {
  assert(count == 6);
  trails.push_back(e);
}
#endif
}
void rotate(int a, int b, float angle) {
  float *m = (float *)&current;
  float c = std::cos(angle), s = std::sin(angle);
  for (int r = 0; r < 4; r++) {
    float x = m[a * 4 + r], y = m[b * 4 + r];
    m[a * 4 + r] = c * x + s * y;
    m[b * 4 + r] = -s * x + c * y;
  }
}
extern "C" void Matrix_RotateXF(f32 a, MatrixMode) { rotate(1, 2, a); }
extern "C" void Matrix_RotateYF(f32 a, MatrixMode) { rotate(2, 0, a); }
extern "C" void Matrix_RotateZF(f32 a, MatrixMode) { rotate(0, 1, a); }
Vec3f transform(const MtxF &m, Vec3f p) {
  return {m.xx * p.x + m.xy * p.y + m.xz * p.z + m.xw,
          m.yx * p.x + m.yy * p.y + m.yz * p.z + m.yw,
          m.zx * p.x + m.zy * p.y + m.zz * p.z + m.zw};
}
void near(Vec3f a, Vec3f b) {
  assert(std::abs(a.x - b.x) < .001 && std::abs(a.y - b.y) < .001 &&
         std::abs(a.z - b.z) < .001);
}

#ifndef MM_REAL_RENDERER
int main() {
  Player p{};
  PlayState play{};
  GraphicsContext gfx{};
  Gfx opa[8192], xlu[8192];
  play.state.gfxCtx = &gfx;
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &p.actor;
  gPlayState = &play;
  p.transformation = PLAYER_FORM_HUMAN;
  p.actor.scale = {.01f, .01f, .01f};
  void (*draw[])(Player *, PlayState *) = {CustomItems_DrawFireRod,
                                           CustomItems_DrawIceRod,
                                           CustomItems_DrawLightRod};
  for (int age : {0, 1})
    for (int e = 0; e < 3; e++)
      for (int pose = 0; pose < 8; pose++) {
        gfx.polyOpa.p = opa;
        gfx.polyXlu.p = xlu;
        gSaveContext.save.linkAge = 0;
        RigFit_Set(1, age, !age, CUSTOM_FORM_NONE);
        gCustomItemState = {};
        gCustomItemState.fireRodActive = gCustomItemState.iceRodActive =
            gCustomItemState.lightRodActive = 1;
        Matrix_Translate(10, 20, 30, MTXMODE_NEW);
        Matrix_RotateXF(pose * .41f, MTXMODE_APPLY);
        Matrix_RotateYF(pose * .29f, MTXMODE_APPLY);
        Matrix_RotateZF(pose * .63f, MTXMODE_APPLY);
        Matrix_Scale(.01f, .01f, .01f, MTXMODE_APPLY);
        MtxF wrist = current;
        if (ItemEquip_CaptureLeftHandMatrix)
          ItemEquip_CaptureLeftHandMatrix();
        held.clear();
        poses.clear();
        draw[e](&p, &play);
        assert(held == std::vector<int>{e});
        Vec3f grip = {0, age ? 216.22f : 328.f, age ? 4.5f : -77.f};
        near(transform(poses.front(), {0, 0, 0}), transform(wrist, grip));
        float tip = (e == 1 ? 68.f : 66.f) * 4.f, angle = 32.f * M_PI / 180.f;
        near(transform(poses.front(),
                       {-std::sin(angle) * tip, std::cos(angle) * tip, 0}),
             transform(wrist, {tip * 5, grip.y, grip.z}));
        local[e] = true;
        sets[e][0].active = 1;
        sets[e][0].count = 1;
        sets[e][0].scale = 2;
        sets[e][0].vel[0] = {18, 0, 0};
        gCustomItemState.fireRodFirstPerson =
            gCustomItemState.iceRodFirstPerson =
                gCustomItemState.lightRodFirstPerson = 1;
        held.clear();
        shots.clear();
        trails.clear();
        nativeDraws = 0;
        draw[e](&p, &play);
        assert(held.empty() && nativeDraws == 0);
        assert(shots == std::vector<int>{e} && trails == std::vector<int>{e});
        Player remote = p;
        draw[e](&remote, &play);
        assert(held == std::vector<int>{e});
        near(transform(poses.back(), {0, 0, 0}),
             transform(wrist, {0, 216.22f, 4.5f}));
        local[e] = false;
        if (ItemEquip_ReleaseHandMatrix)
          ItemEquip_ReleaseHandMatrix();
        assert(matrices.empty());
      }
  // An adult-only custom mirror changes the local palm, never a native peer's.
  RigFit_Set(1, 0, 0, CUSTOM_FORM_KEATON);
  gCustomItemState.fireRodFirstPerson = 0;
  Matrix_Translate(10, 20, 30, MTXMODE_NEW);
  Matrix_Scale(.01f, .01f, .01f, MTXMODE_APPLY);
  MtxF customWrist = current;
  ItemEquip_CaptureLeftHandMatrix();
  held.clear();
  poses.clear();
  draw[0](&p, &play);
  near(transform(poses.back(), {0, 0, 0}),
       transform(customWrist, {0, 328.f, -77.f}));
  Player customPeer = p;
  draw[0](&customPeer, &play);
  near(transform(poses.back(), {0, 0, 0}),
       transform(customWrist, {0, 216.22f, 4.5f}));
  ItemEquip_ReleaseHandMatrix();
  // Released captures cannot leak into another actor, and every missing
  // replacement bundle uses the original native archive display list.
  const ItemHandPose neutral = {0, 0, 0, 0, 0, 0, 1};
  assert(!ItemEquip_ApplyLeftHandPose(&p, &neutral));
  for (int e = 0; e < 3; e++)
    for (bool capture : {false, true}) {
      gfx.polyOpa.p = opa;
      gfx.polyXlu.p = xlu;
      gCustomItemState.fireRodFirstPerson = gCustomItemState.iceRodFirstPerson =
          gCustomItemState.lightRodFirstPerson = 0;
      resources = false;
      if (capture)
        ItemEquip_CaptureLeftHandMatrix();
      else
        ItemEquip_ReleaseHandMatrix();
      held.clear();
      nativeDraws = 0;
      draw[e](&p, &play);
      assert(held.empty() && nativeDraws > 0);
    }
  ItemEquip_ReleaseHandMatrix();
  assert(!ItemEquip_ApplyHandPose(&p, &neutral));
  ItemEquip_CaptureLeftHandMatrix();
  p.transformation = PLAYER_FORM_DEKU;
  assert(!ItemEquip_ApplyLeftHandPose(&p, &neutral));
  std::cout << "PASS native MM three rods, two ages, eight wrist poses, local "
               "first person and remote live shots\n";
}

#endif
