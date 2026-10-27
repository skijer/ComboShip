#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#ifdef HOST_MM
// Both engine and libultra headers define controller aliases. Load libultra
// once, then let the real MM controller header install its own names.
#include <libultraship/libultra/controller.h>
#undef BTN_A
#undef BTN_B
#undef BTN_Z
#undef BTN_START
#undef BTN_DUP
#undef BTN_DDOWN
#undef BTN_DLEFT
#undef BTN_DRIGHT
#undef BTN_L
#undef BTN_R
#undef BTN_CUP
#undef BTN_CDOWN
#undef BTN_CLEFT
#undef BTN_CRIGHT
#endif
extern "C" {
#ifdef HOST_MM
#include "global.h"
#else
#include "functions.h"
#include "macros.h"
#include "z64.h"
#endif
}

static Gfx opa[2048], xlu[2048];
static GraphicsContext gfx;
static PlayState play;
static MtxF incoming;
static int allocations, hilites;
static size_t allocatedBytes;
static bool failAllocation;
static std::vector<std::vector<Mtx>> arena;
static std::map<Mtx *, MtxF> conversions;
static std::string missingPath;
static std::vector<std::string> checked;
void FrameInterpolation_RecordOpenChild(const void *, int) {}
void FrameInterpolation_RecordCloseChild() {}
extern "C" {
void gSPSegment(void *cmd, int segment, uintptr_t address) {
  __gSPSegment((Gfx *)cmd, segment, address);
}
void gSPDisplayList(Gfx *cmd, Gfx *path) { __gSPDisplayList(cmd, path); }
void gDPSetTileSizeLerp(Gfx *cmd, int tile, float x0, float y0, float xx0,
                        float yy0, float x1, float y1, float xx1, float yy1) {
  __gDPSetTileSizeLerp(cmd, tile, x0, y0, xx0, yy0, x1, y1, xx1, yy1);
}
void Matrix_Get(MtxF *out) { *out = incoming; }
Mtx *Matrix_MtxFToMtx(MtxF *src, Mtx *dest) {
  conversions[dest] = *src;
  return dest;
}
void guLookAtHilite(Mtx *, LookAt *, Hilite *, float, float, float, float,
                    float, float, float, float, float, float, float, float,
                    float, float, float, int, int) {
  ++hilites;
}
#ifdef HOST_MM
void Gfx_SetupDL25_Xlu(GraphicsContext *) {}
void Graph_OpenDisps(Gfx **, Gfx *, GraphicsContext *, const char *, s32) {}
void Graph_CloseDisps(Gfx **, Gfx *, GraphicsContext *, const char *, s32) {}
#else
void Gfx_SetupDL_25Xlu(GraphicsContext *) {}
void Graph_OpenDisps(Gfx **, GraphicsContext *, const char *, s32) {}
void Graph_CloseDisps(Gfx **, GraphicsContext *, const char *, s32) {}
void *Graph_Alloc(GraphicsContext *, size_t bytes) {
  ++allocations;
  allocatedBytes = bytes;
  if (failAllocation)
    return nullptr;
  arena.emplace_back((bytes + sizeof(Mtx) - 1) / sizeof(Mtx));
  return arena.back().data();
}
#endif
}
static bool Available(const char *path, const char *owner) {
  assert(owner && !strcmp(owner, "oot"));
  assert(!strncmp(path, "__OTR__objects/object_mo/", 24));
  checked.emplace_back(path);
  return missingPath != path;
}
#ifdef HOST_MM
#define COMBO_MORPHA_GI_HOST_MM
#endif
#include "ComboMorphaGi.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "objects/object_mo/object_mo.h"

static void Reset(uint32_t frame) {
  memset(opa, 0, sizeof(opa));
  memset(xlu, 0, sizeof(xlu));
  gfx.polyOpa.p = opa;
  gfx.polyOpa.d = opa + 2048;
  gfx.polyXlu.p = xlu;
  gfx.polyXlu.d = xlu + 2048;
  play.state.gfxCtx = &gfx;
  play.gameplayFrames = frame;
  incoming = {};
  incoming.xx = 2;
  incoming.yy = 3;
  incoming.zz = 4;
  incoming.ww = 1;
  incoming.xw = 11;
  incoming.yw = 17;
  incoming.zw = -9;
  play.view.eye = {30, 50, 70};
  play.envCtx.dirLight1.params.dir.x = 10;
  play.envCtx.dirLight1.params.dir.y = 30;
  play.envCtx.dirLight1.params.dir.z = 20;
  conversions.clear();
  checked.clear();
  arena.clear();
  allocations = 0;
  allocatedBytes = 0;
  hilites = 0;
  failAllocation = false;
  missingPath.clear();
}
static std::vector<MtxF> CheckRendered() {
  int ownerDepth = 0, draws = 0, matrices = 0;
  uintptr_t segment12 = 0, segment8 = 0, restore12 = 0, restore8 = 0,
            restore9 = 0;
  uint32_t prim = 0, env = 0;
  std::vector<MtxF> nodes;
  for (Gfx *cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
    const unsigned op = cmd->words.w0 >> 24;
    if (op == G_COMBO_RM_PUSH) {
      assert(!strcmp((const char *)cmd->words.w1, "oot"));
      ++ownerDepth;
    }
    if (op == G_COMBO_RM_POP) {
      assert(ownerDepth == 1);
      --ownerDepth;
    }
    if (op == G_SETPRIMCOLOR)
      prim = cmd->words.w1;
    if (op == G_SETENVCOLOR)
      env = cmd->words.w1;
    if (op == G_MOVEWORD && ((cmd->words.w0 >> 16) & 255) == G_MW_SEGMENT) {
      const int segment = (cmd->words.w0 & 65535) / 4;
      if (segment == 12) {
        if (!segment12)
          segment12 = cmd->words.w1;
        else
          restore12 = cmd->words.w1;
      }
      if (segment == 8) {
        if (!segment8)
          segment8 = cmd->words.w1;
        else
          restore8 = cmd->words.w1;
      }
      if (segment == 9)
        restore9 = cmd->words.w1;
    }
    if (op == G_MTX) {
      auto *matrix = (Mtx *)cmd->words.w1;
      const auto value = conversions.at(matrix);
      if (matrices < 41) {
        assert(matrix == (Mtx *)segment12 + matrices);
        nodes.push_back(value);
      } else
        assert(!memcmp(&value, &incoming, sizeof(value)));
      ++matrices;
    }
    if (op == G_DL || op == G_DL_OTR_FILEPATH) {
      // gSPDisplayList would resolve unqualified OTR paths against the
      // CPU host RM before the deferred owner bracket executes.
      assert(op == G_DL_OTR_FILEPATH);
      assert(ownerDepth == 1 && prim == 0xC8FFFFB4 && env == 0x0064FF96);
      const std::string path = (const char *)cmd->words.w1;
      const std::string suffix =
          draws == 0 ? "gMorphaTentacleBaseDL"
                     : "gMorphaTentaclePart" + std::to_string(draws) + "DL";
      assert(path == "__OTR__objects/object_mo/" + suffix);
      ++draws;
    }
  }
  assert(draws == 41 && matrices == 42 && ownerDepth == 0 && hilites == 1);
  assert(segment8 && segment12 && restore8 && restore9 && restore12);
  for (uintptr_t ptr : {restore8, restore9, restore12})
    assert((((Gfx *)ptr)->words.w0 >> 24) == G_ENDDL);
  auto *scroll = (Gfx *)segment8;
  assert((scroll[0].words.w0 >> 24) == G_RDPTILESYNC &&
         (scroll[11].words.w0 >> 24) == G_ENDDL);
  assert((scroll[1].words.w0 >> 24) == G_SETTILESIZE_LERP &&
         ((scroll[6].words.w1 >> 24) & 7) == 1);
  auto coordinate = [](uintptr_t word) {
    uint32_t bits = uint32_t(word);
    float value;
    memcpy(&value, &bits, 4);
    return value;
  };
  // The two native 32x32 layers scroll +1/+1 and -3/+1 per tick.
  assert(coordinate(scroll[3].words.w0) - coordinate(scroll[2].words.w0) ==
         124);
  assert(coordinate(scroll[4].words.w0) - coordinate(scroll[2].words.w0) == 1);
  assert(coordinate(scroll[4].words.w1) - coordinate(scroll[2].words.w1) == 1);
  assert(coordinate(scroll[8].words.w0) - coordinate(scroll[7].words.w0) ==
         124);
  assert(coordinate(scroll[9].words.w0) - coordinate(scroll[7].words.w0) == -3);
  assert(coordinate(scroll[9].words.w1) - coordinate(scroll[7].words.w1) == 1);
  if (play.gameplayFrames == 47) {
    assert(coordinate(scroll[2].words.w0) == 47 &&
           coordinate(scroll[2].words.w1) == 47);
    assert(coordinate(scroll[7].words.w0) == 1907 &&
           coordinate(scroll[7].words.w1) == 47);
  }
  assert(nodes[0].xx == 0 && nodes[1].xx == 0);
  for (size_t i = 2; i < nodes.size(); ++i) {
    const auto &m = nodes[i];
    for (float value : {m.xx, m.xy, m.xz, m.xw, m.yx, m.yy, m.yz, m.yw, m.zx,
                        m.zy, m.zz, m.zw})
      assert(std::isfinite(value));
    assert(m.xw > -100 && m.xw < 110 && m.yw > -75 && m.yw < 110 &&
           m.zw > -50 && m.zw < 50);
  }
  return nodes;
}
static void TestTentacle() {
  Reset(47);
  const auto saved = incoming;
  assert(ComboDrawMorphaTentacleGi(&play, "oot", Available));
#ifdef HOST_MM
  const size_t frameBytes = reinterpret_cast<uintptr_t>(opa + 2048) -
                            reinterpret_cast<uintptr_t>(gfx.polyOpa.d);
#else
  const size_t frameBytes = (allocatedBytes + 15u) & ~size_t(15u);
#endif
  const auto baseline = CheckRendered();
  assert(!memcmp(&incoming, &saved, sizeof(saved)));
  assert(checked.size() == 41);
  assert(baseline[2].xw == 1 && baseline[2].yw == -64);
  Reset(47);
  incoming.xx = 0;
  incoming.yx = 2;
  incoming.xy = -3;
  incoming.yy = 0;
  const auto rotatedPose = incoming;
  assert(ComboDrawMorphaTentacleGi(&play, "oot", Available));
  const auto rotated = CheckRendered();
  assert(rotated[2].xw == 92 && rotated[2].yw == 7);
  assert(!memcmp(&incoming, &rotatedPose, sizeof(incoming)));
  Reset(47);
  assert(ComboDrawMorphaTentacleGi(&play, "oot", Available));
  const auto repeated = CheckRendered();
  assert(!memcmp(baseline.data(), repeated.data(),
                 baseline.size() * sizeof(MtxF)));
  Reset(48);
  assert(ComboDrawMorphaTentacleGi(&play, "oot", Available));
  const auto animated = CheckRendered();
  assert(
      memcmp(baseline.data(), animated.data(), baseline.size() * sizeof(MtxF)));
  for (uint32_t frame : {0u, 65535u, 0xFFFFFFFFu}) {
    Reset(frame);
    assert(ComboDrawMorphaTentacleGi(&play, "oot", Available));
    CheckRendered();
  }
  for (const char *suffix :
       {"gMorphaTentacleBaseDL", "gMorphaTentaclePart1DL",
        "gMorphaTentaclePart24DL", "gMorphaTentaclePart40DL"}) {
    Reset(47);
    missingPath = std::string("__OTR__objects/object_mo/") + suffix;
    assert(!ComboDrawMorphaTentacleGi(&play, "oot", Available));
    assert(gfx.polyXlu.p == xlu && conversions.empty());
  }
  Reset(47);
#ifdef HOST_MM
  gfx.polyOpa.d = gfx.polyOpa.p; // Native GRAPH_ALLOC otherwise subtracts
                                 // through exhausted memory.
#else
  failAllocation = true;
#endif
  assert(!ComboDrawMorphaTentacleGi(&play, "oot", Available));
  assert(gfx.polyXlu.p == xlu && conversions.empty());
  Reset(47);
  // A tentacle that fits by itself must leave the preserved core's two
  // native scroll DLs, matrix and MM segment cleanup room to allocate.
  gfx.polyOpa.d = gfx.polyOpa.p + (frameBytes + 16) / sizeof(Gfx);
  auto *const arenaEnd = gfx.polyOpa.d;
  assert(!ComboDrawMorphaTentacleGi(&play, "oot", Available));
  assert(gfx.polyXlu.p == xlu && gfx.polyOpa.d == arenaEnd &&
         conversions.empty() && allocations == 0);
  Reset(47);
  gfx.polyXlu.d = gfx.polyXlu.p + 10;
  assert(!ComboDrawMorphaTentacleGi(&play, "oot", Available));
  assert(gfx.polyXlu.p == xlu && conversions.empty());
  Reset(47);
  gfx.polyXlu.p = nullptr;
  assert(!ComboDrawMorphaTentacleGi(&play, "oot", Available));
  assert(conversions.empty());
  Reset(47);
  assert(!ComboDrawMorphaTentacleGi(nullptr, "oot", Available));
  assert(!ComboDrawMorphaTentacleGi(&play, nullptr, Available));
  assert(!ComboDrawMorphaTentacleGi(&play, "oot", nullptr));
  assert(gfx.polyXlu.p == xlu && conversions.empty());
  std::cout << "PASS Morpha 41-slot native tentacle: materials, owner, "
               "animation, pose, allocation and resource fallback\n";
}

static Mtx coreMatrix;
static Gfx coreScrolls[4][12];
static int coreScrollCount;
static std::vector<MtxF> matrixStack;
static bool integrationSmallArena;
extern "C" {
void Matrix_Push() { matrixStack.push_back(incoming); }
void Matrix_Pop() {
  assert(!matrixStack.empty());
  incoming = matrixStack.back();
  matrixStack.pop_back();
}
#ifdef HOST_MM
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode) {
  incoming.xx *= x;
  incoming.yy *= y;
  incoming.zz *= z;
}
void Matrix_Translate(f32 x, f32 y, f32 z, MatrixMode) {
  incoming.xw += x;
  incoming.yw += y;
  incoming.zw += z;
}
void Matrix_RotateXF(f32, MatrixMode) {}
void Matrix_RotateZF(f32, MatrixMode) {}
Mtx *Matrix_Finalize(GraphicsContext *) {
  return Matrix_MtxFToMtx(&incoming, &coreMatrix);
}
f32 Rand_ZeroOne() { return 0; }
#else
void Matrix_Scale(f32 x, f32 y, f32 z, u8) {
  incoming.xx *= x;
  incoming.yy *= y;
  incoming.zz *= z;
}
void Matrix_Translate(f32 x, f32 y, f32 z, u8) {
  incoming.xw += x;
  incoming.yw += y;
  incoming.zw += z;
}
void Matrix_RotateX(f32, u8) {}
void Matrix_RotateZ(f32, u8) {}
Mtx *Matrix_NewMtx(GraphicsContext *, char *, s32) {
  return Matrix_MtxFToMtx(&incoming, &coreMatrix);
}
#endif
void Matrix_ReplaceRotation(MtxF *) {}
Gfx *Gfx_TwoTexScrollEx(GraphicsContext *, s32, u32, u32, s32, s32, s32, u32,
                        u32, s32, s32, s32, s32, s32, s32) {
  assert(coreScrollCount < 4);
  return coreScrolls[coreScrollCount++];
}
}
static bool IntegrationAvailable(const char *path) {
  if (integrationSmallArena)
    gfx.polyOpa.d = gfx.polyOpa.p + 32;
  return Available(path, "oot");
}
#ifdef HOST_MM
static int NeiResource_Available(const char *path) {
  return IntegrationAvailable(path);
}
PlayState *gPlayState = &play;
struct ComboForeignDrawInfoOOT {
  const char *dls[3];
  uint8_t primColorXlu[4];
};
#else
#define CVAR_PREFIX_RANDOMIZER_ENHANCEMENT "gRandoEnhancements"
static uint8_t ResourceMgr_FileExists(const char *path) {
  return IntegrationAvailable(path);
}
static uint8_t ResourceMgr_FileAltExists(const char *) { return false; }
static bool ResourceMgr_IsAltAssetsEnabled() { return false; }
static int CVarGetInteger(const char *, int) { return 0; }
static const char *gGiBlueFireFlameDL =
    "__OTR__objects/object_gi_fire/gGiBlueFireFlameDL";
static const char *gBossSoulSkullDL =
    "__OTR__objects/object_boss_soul/gGIBossSoulSkullDL";
extern "C" {
void DrawGohma(PlayState *) { assert(false); }
void DrawKingDodongo(PlayState *) { assert(false); }
void DrawBarinade(PlayState *) { assert(false); }
void DrawPhantomGanon(PlayState *) { assert(false); }
void DrawVolvagia(PlayState *) { assert(false); }
void DrawBongoBongo(PlayState *) { assert(false); }
void DrawKotake(PlayState *) { assert(false); }
void DrawGanon(PlayState *) { assert(false); }
}
#endif
/* PRODUCTION_INTEGRATION */

static void TestIntegration() {
  for (int mode : {0, 1, 2}) {
    Reset(47);
    play.state.frames = 47;
    coreScrollCount = 0;
    matrixStack.clear();
    integrationSmallArena = mode == 2;
    if (mode == 1)
      missingPath = "__OTR__objects/object_mo/gMorphaTentaclePart24DL";
#ifndef HOST_MM
    if (mode == 2)
      failAllocation = true;
#endif
    const auto saved = incoming;
#ifdef HOST_MM
    ComboForeignDrawInfoOOT info{};
    info.dls[1] = "__OTR__@oot:objects/object_mo/gMorphaCoreMembraneDL";
    info.dls[2] = "__OTR__@oot:objects/object_mo/gMorphaCoreNucleusDL";
    info.primColorXlu[0] = 85;
    info.primColorXlu[1] = 180;
    info.primColorXlu[2] = 223;
    info.primColorXlu[3] = 255;
    MM_DrawForeignMorphaSoul(&info);
    const char *flame = gameplay_keep_DL_01ACF0;
    const char *membrane = info.dls[1];
    const char *nucleus = info.dls[2];
#else
    GetItemEntry entry{};
    entry.getItemId = RG_MORPHA_SOUL;
    Matrix_Push();
    Randomizer_DrawBossSoul(&play, &entry);
    Matrix_Pop();
    const char *flame = gGiBlueFireFlameDL;
    const char *membrane = gMorphaCoreMembraneDL;
    const char *nucleus = gMorphaCoreNucleusDL;
#endif
    assert(!memcmp(&incoming, &saved, sizeof(saved)) && matrixStack.empty());
    int tentacleDraws = 0, flameDraws = 0, membraneDraws = 0, nucleusDraws = 0,
        ownerDepth = 0;
    for (Gfx *cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
      unsigned op = cmd->words.w0 >> 24;
      if (op == G_COMBO_RM_PUSH)
        ++ownerDepth;
      if (op == G_COMBO_RM_POP)
        --ownerDepth;
      if (op == G_DL || op == G_DL_OTR_FILEPATH) {
        const char *path = (const char *)cmd->words.w1;
        if (strstr(path, "gMorphaTentacle"))
          ++tentacleDraws;
        if (!strcmp(path, flame))
          ++flameDraws;
        if (!strcmp(path, membrane))
          ++membraneDraws;
        if (!strcmp(path, nucleus))
          ++nucleusDraws;
      }
    }
    assert(ownerDepth == 0 && coreScrollCount == 3);
    assert(flameDraws == 1 && membraneDraws == 1 && nucleusDraws == 1);
    assert(tentacleDraws == 0);
    assert(checked.empty()); // Retired tentacle cannot load or replace the selected core.
  }
  std::cout << "PASS actual Morpha native OoT/MM integration: core and flame "
               "remain without tentacle submissions or resource requests\n";
}
int main() {
  TestTentacle();
  TestIntegration();
}
