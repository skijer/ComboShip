// Real rod object drawers, effect dispatch, batcher, native matrix operations
// and frame interpolator. Resource availability/allocation and held model are
// boundaries; no projectile drawing or interpolation function is replaced.
#include "2s2h/BenPort.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/Rando/NeiUsedMagicPresentation.h"
#include "functions.h"
#include "mods/items/custom_items.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/items/logic/item_rod_common.h"
#include "variables.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

OTRGlobals app;
OTRGlobals *OTRGlobals::Instance = &app;
namespace {
RodProjSet sets[2][ROD_MAX_PROJ_SETS]{};
std::vector<void *> allocations;
std::map<Mtx *, MtxF> submitted;
std::vector<Vec3f> visibleVertices;
std::vector<Vtx> packedVertices;
Gfx opa[100000], xlu[100000], overlay[2048];
GraphicsContext graphics{};
PlayState play{};
Player player{};
void *allocate(size_t n) {
  void *p = std::calloc(1, n);
  assert(p);
  allocations.push_back(p);
  return p;
}
} // namespace
extern "C" {
void Baseline_DrawCharge(PlayState *, Player *, int, float);
void Baseline_DrawProjectile(PlayState *, int, const Vec3f *, const Vec3f *,
                             float, unsigned);
void Baseline_DrawTrail(PlayState *, int, const Vec3f *, unsigned, float);
void Baseline_DrawBurst(PlayState *, int, const Vec3f *, float, float);
void Baseline_DrawChargeFocus(PlayState *, int);
void Baseline_DrawSpin(PlayState *, Player *, int, float, bool);
s32 AdultLink_UsesAdultPresentation(const Player *) { return 0; }
CustomItemState gCustomItemState{};
SaveContext gSaveContext{};
Gfx Cylinder_001_opaque_dl[1], ice_rod_opaque_dl[1], ice_rod_transparent_dl[1];
void *THA_AllocTailAlign16(TwoHeadArena *, size_t n) { return allocate(n); }
void *Graph_Alloc(GraphicsContext *, size_t n) { return allocate(n); }
void guMtxF2L(float mf[4][4], Mtx *destination) {
  std::memcpy(&submitted[destination], mf, sizeof(MtxF));
}
f32 Math_SinS(s16 a) { return std::sin(a * (3.14159265358979323846f / 32768)); }
f32 Math_CosS(s16 a) { return std::cos(a * (3.14159265358979323846f / 32768)); }
void Gfx_SetupDL25_Opa(GraphicsContext *) {}
void Gfx_SetupDL26_Opa(GraphicsContext *) {}
void Gfx_SetupDL25_Xlu(GraphicsContext *) {}
void gSPDisplayList(Gfx *, Gfx *) { std::abort(); } // Held model is available.
void gSPVertex(Gfx *, uintptr_t address, int count, int) {
  const auto *vertices = reinterpret_cast<const Vtx *>(address);
  for (int i = 0; i < count; ++i) {
    Vec3f local{float(vertices[i].v.ob[0]), float(vertices[i].v.ob[1]),
                float(vertices[i].v.ob[2])},
        world;
    Matrix_MultVec3f(&local, &world);
    visibleVertices.push_back(world);
    packedVertices.push_back(vertices[i]);
  }
}
void lusprintf(const char *, int32_t, int32_t, const char *, ...) {
  std::abort();
}
void Graph_OpenDisps(Gfx **, Gfx *, GraphicsContext *, const char *, s32) {}
void Graph_CloseDisps(Gfx **, Gfx *, GraphicsContext *, const char *, s32) {}
u8 ResourceMgr_FileExists(const char *) { return 1; }
int ResourceMgr_IsModAssetForGame(const char*, const char*) { return 0; }
int NeiResource_Available(const char *) { return 1; }
Gfx *ResourceMgr_LoadGfxByName(const char *) {
  static Gfx dl[1];
  return dl;
}
u8 ResourceMgr_FileAltExists(const char *) { return 0; }
bool ResourceMgr_IsAltAssetsEnabled() { return false; }
u8 ItemEquip_ApplyLeftHandPose(Player *, const ItemHandPose *) { return 1; }
bool NeiHeld_DrawRod(PlayState *, int) { return true; }
u8 FireRod_HasAnyActiveSet() {
  for (const auto &s : sets[0])
    if (s.active)
      return 1;
  return 0;
}
u8 IceRod_HasAnyActiveSet() {
  for (const auto &s : sets[1])
    if (s.active)
      return 1;
  return 0;
}
RodProjSet *FireRod_GetProjSets() { return sets[0]; }
RodProjSet *IceRod_GetProjSets() { return sets[1]; }
}

namespace {
void draw(int element) {
  submitted.clear();
  visibleVertices.clear();
  packedVertices.clear();
  graphics.polyOpa.p = opa;
  graphics.polyOpa.d = opa + 100000;
  graphics.polyXlu.p = xlu;
  graphics.polyXlu.d = std::end(xlu);
  graphics.overlay.p = overlay;
  graphics.overlay.d = std::end(overlay);
  FrameInterpolation_StartRecord();
  Matrix_Translate(0, 0, 0, MTXMODE_NEW);
  if (element == 0)
    CustomItems_DrawFireRod(&player, &play);
  else
    CustomItems_DrawIceRod(&player, &play);
  FrameInterpolation_StopRecord();
  ++play.gameplayFrames;
}
void setPosition(RodProjSet &set, float x) {
  set.active = 1;
  set.count = 3;
  set.scale = 2;
  set.timer = 20;
  for (int p = 0; p < 3; ++p) {
    set.pos[p] = {x, 30, float(p * 80)};
    set.vel[p] = {15, 0, 0};
  }
  for (int i = 0; i < 6; ++i)
    set.trail[i] = {x - i * 15.f, 30, 0};
}
void check(float currentX, float expectedX) {
  const auto replacement = FrameInterpolation_Interpolate(.5f);
  unsigned checked = 0;
  for (const auto &[address, matrix] : submitted) {
    if (std::abs(matrix.xw - currentX) > .001f)
      continue;
    const auto &result = replacement.at(address);
    if (std::abs(result.xw - expectedX) > .001f) {
      std::fprintf(
          stderr,
          "FAIL: projectile at %.1f interpolated to %.1f; expected %.1f\n",
          currentX, result.xw, expectedX);
      std::exit(1);
    }
    ++checked;
  }
  assert(checked >= 6); // Both production meshes of all three heads.
}
struct Snapshot {
  std::vector<Vec3f> positions;
  std::vector<Vtx> vertices;
  std::vector<std::pair<uintptr_t, uintptr_t>> commands;
  std::vector<std::string> textures;
};
Snapshot effectDraw(bool baseline, bool release, float value, bool big,
                    int element = 2, int mode = 0) {
  graphics.polyOpa.p = opa;
  graphics.polyOpa.d = opa + 100000;
  graphics.polyXlu.p = xlu;
  graphics.polyXlu.d = std::end(xlu);
  graphics.overlay.p = overlay;
  graphics.overlay.d = std::end(overlay);
  visibleVertices.clear();
  packedVertices.clear();
  Matrix_Translate(10, 40, 5, MTXMODE_NEW);
  if (mode == 1) {
    Vec3f velocity{15, 2, -4};
    Vec3f trail[6] = {{80, 30, 0},  {65, 28, 4},  {50, 26, 8},
                      {35, 24, 12}, {20, 22, 16}, {5, 20, 20}};
    (baseline ? Baseline_DrawProjectile : NeiUsedMagic_DrawProjectile)(
        &play, element, &trail[0], &velocity, value, 7);
    (baseline ? Baseline_DrawTrail : NeiUsedMagic_DrawTrail)(&play, element,
                                                             trail, 6, value);
    (baseline ? Baseline_DrawBurst
              : NeiUsedMagic_DrawBurst)(&play, element, &trail[0], value, .5f);
  } else if (release) {
    (baseline ? Baseline_DrawSpin : NeiUsedMagic_DrawSpin)(&play, &player,
                                                           element, value, big);
  } else {
    gCustomItemState.iceRodCharging = gCustomItemState.fireRodCharging =
        gCustomItemState.lightRodCharging = 1;
    gCustomItemState.iceRodChargeLevel = gCustomItemState.fireRodChargeLevel =
        gCustomItemState.lightRodChargeLevel = value;
    (baseline ? Baseline_DrawCharge : NeiUsedMagic_DrawCharge)(&play, &player,
                                                               element, value);
    (baseline ? Baseline_DrawChargeFocus
              : NeiUsedMagic_DrawChargeFocus)(&play, element);
  }
  Snapshot result{visibleVertices, packedVertices, {}, {}};
  for (auto *command = xlu; command < graphics.polyXlu.p; ++command) {
    const auto opcode = (command->words.w0 >> 24) & 255;
    if (opcode == G_COMBO_RM_PUSH || opcode == G_COMBO_RM_POP)
      continue;
    auto data = command->words.w1;
    if (opcode == G_MTX || opcode == G_VTX)
      data = 0; // Allocation address only.
    if (opcode == G_SETTIMG || opcode == G_SETTIMG_OTR_FILEPATH) {
      result.textures.emplace_back(reinterpret_cast<const char *>(data));
      data = 0;
    }
    result.commands.emplace_back(command->words.w0, data);
  }
  // Keep only the native matrix stack allocation; snapshots own their data.
  for (size_t i = 1; i < allocations.size(); ++i)
    std::free(allocations[i]);
  allocations.resize(1);
  return result;
}
void sameLight(const Snapshot &before, const Snapshot &after) {
  assert(before.positions.size() == after.positions.size());
  assert(before.vertices.size() == after.vertices.size());
  assert(std::memcmp(before.positions.data(), after.positions.data(),
                     before.positions.size() * sizeof(Vec3f)) == 0);
  assert(std::memcmp(before.vertices.data(), after.vertices.data(),
                     before.vertices.size() * sizeof(Vtx)) == 0);
  assert(before.commands == after.commands &&
         before.textures == after.textures);
}
} // namespace
int main() {
  play.state.gfxCtx = &graphics;
  play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz =
      play.billboardMtxF.ww = 1;
  player.actor.scale = {.01f, .01f, .01f};
  Matrix_Init(&play.state);
  gCustomItemState.fireRodActive = gCustomItemState.iceRodActive = 1;
  for (int element = 0; element < 2; ++element) {
    std::memset(sets, 0, sizeof(sets));
    draw(element);
    setPosition(sets[element][0], 0);
    setPosition(sets[element][1], 300);
    draw(element);
    setPosition(sets[element][0], 15);
    setPosition(sets[element][1], 315);
    draw(element);
    check(315, 307.5f); // Control: ordinary flight must still interpolate.
    sets[element][0].active = 0;
    setPosition(sets[element][1], 330);
    draw(element);
    check(330,
          322.5f); // Removing the earlier volley must not steal its matrix.
    setPosition(sets[element][1], -100);
    ++sets[element][1].drawEpoch;
    draw(element);
    check(-100,
          -100); // A recycled slot must not interpolate from its old launch.
    setPosition(sets[element][1], -85);
    draw(element);
    check(-85, -92.5f); // Its following update resumes ordinary interpolation.

    std::memset(sets, 0, sizeof(sets));
    auto &hit = sets[element][0];
    setPosition(hit, 0);
    hit.count = 1;
    hit.yaw = hit.pitch = 0;
    hit.vel[0] = {0, 0, 15};
    draw(element);
    const auto moving = visibleVertices;
    --play.gameplayFrames; // Same sampled phase isolates heading from
                           // animation.
    hit.vel[0] = {};
    draw(element);
    assert(moving.size() == visibleVertices.size());
    for (size_t i = 0; i < moving.size(); ++i) {
      const auto &a = moving[i];
      const auto &b = visibleVertices[i];
      if (std::abs(a.x - b.x) + std::abs(a.y - b.y) + std::abs(a.z - b.z) >
          .2f) {
        std::fprintf(
            stderr,
            "FAIL: %s projectile changes heading when hit velocity is zeroed\n",
            element == 0 ? "Fire" : "Ice");
        return 1;
      }
    }
  }
  std::puts("PASS production Fire/Ice interpolation: volley removal, slot "
            "reuse, flight and impact heading");
  for (int element = 0; element < 3; ++element)
    for (unsigned frame = 0; frame < 180; ++frame) {
      play.gameplayFrames = frame;
      sameLight(effectDraw(true, false, 1.f, false, element, 1),
                effectDraw(false, false, 1.f, false, element, 1));
      for (float charge : {.2f, .5f, 1.f})
        sameLight(effectDraw(true, false, charge, false, element),
                  effectDraw(false, false, charge, false, element));
      for (bool big : {false, true})
        for (float radius : {80.f, 230.f, 500.f})
          sameLight(effectDraw(true, true, radius, big, element),
                    effectDraw(false, true, radius, big, element));
    }
  std::puts("PASS native MM Fire/Ice/Light "
            "charge/focus/release/projectile/trail/burst match accepted donor "
            "across 180 phases: packed vertices, positions, UV/color/alpha, "
            "textures, graphics commands");
  for (void *p : allocations)
    std::free(p);
}
