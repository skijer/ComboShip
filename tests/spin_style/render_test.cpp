#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
extern "C" {
#ifdef HOST_MM
#include "global.h"
#else
#include "functions.h"
#include "macros.h"
#include "z64.h"
#endif
#include "objects/gameplay_keep/gameplay_keep.h"
#include "overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.h"
}
#include "combo/render/SpinAttackNativeMaterial.h"
#include "shipped_resources.h"
#include <libultraship/bridge/consolevariablebridge.h>

static Gfx xlu[2048];
static Gfx debugOverlay[32];
alignas(16) static uint8_t allocation[32768];
static Gfx *opa = (Gfx *)allocation;
static size_t allocated;
static size_t allocationCursor, submittedVertices;
static std::vector<uint32_t> particleColors;
static std::vector<uint8_t> particleAlphas;
static std::vector<Gfx *> submittedLists;
static Gfx nativeList[8], privateList[6], brokenList[2];
static Gfx accentNativeList[3][14];
static uint8_t textureByte;
static bool missing, malformed, cosmeticEdited, accentMissing, accentMalformed,
    allocationFailure, faithfulArena;
static GraphicsContext gfx;
static PlayState play;
static MtxF parentMatrix;
static std::vector<std::pair<Mtx *, MtxF>> matrixSnapshots;
static unsigned xluSetups, hiliteCalls;
#include "spin_graph_production.inc"
extern "C" {
void Matrix_Get(MtxF *dest) { *dest = parentMatrix; }
Mtx *Matrix_MtxFToMtx(MtxF *src, Mtx *dest) {
  matrixSnapshots.emplace_back(dest, *src);
  std::memset(dest, 0, sizeof(*dest));
  return dest;
}
void gDPSetTileSizeLerp(Gfx *p, int tile, float u0, float v0, float r0,
                        float b0, float u1, float v1, float r1, float b1) {
  __gDPSetTileSizeLerp(p, tile, u0, v0, r0, b0, u1, v1, r1, b1);
}
void guLookAtHilite(Mtx *mtx, LookAt *look, Hilite *hilite, float, float, float,
                    float, float, float, float, float, float, float, float,
                    float, float, float, float, int32_t, int32_t) {
  ++hiliteCalls;
  std::memset(mtx, 0, sizeof(*mtx));
  std::memset(look, 0, sizeof(*look));
  std::memset(hilite, 0, sizeof(*hilite));
}
void *Graph_Alloc(GraphicsContext *context, size_t size) {
  if (allocationFailure)
    return nullptr;
  allocated = size;
  assert(size <= sizeof(allocation));
  if (faithfulArena) {
    context->polyOpa.d = reinterpret_cast<Gfx *>(
        reinterpret_cast<uint8_t *>(context->polyOpa.d) -
        ((size + 15) & ~(size_t)15));
    return context->polyOpa.d;
  }
  size_t at = allocationCursor;
  allocationCursor += (size + 15) & ~(size_t)15;
  assert(allocationCursor <= sizeof(allocation));
  return allocation + at;
}
void gSPVertex(Gfx *p, uintptr_t vtx, int n, int v0) {
  submittedVertices += n;
  __gSPVertex(p, vtx, n, v0);
}
void gSPDisplayList(Gfx *p, Gfx *dl) {
  submittedLists.push_back(dl);
  if ((dl[0].words.w0 >> 24) == 0x33) {
    uint32_t rgba = 0;
    for (const Gfx *command = xlu; command < p; ++command)
      if ((command->words.w0 >> 24) == 0xfa)
        rgba = command->words.w1;
    particleColors.push_back(rgba >> 8);
    particleAlphas.push_back(rgba & 255);
  }
  __gSPDisplayList(p, dl);
}
int32_t CVarGetInteger(const char *key, int32_t fallback) {
  return cosmeticEdited && strstr(key, ".Changed") ? 1 : fallback;
}
Color_RGB8 CVarGetColor24(const char *name, Color_RGB8 fallback) {
  return cosmeticEdited ? (strstr(name, "Secondary") ? Color_RGB8{70, 80, 90}
                                                     : Color_RGB8{10, 20, 30})
                        : fallback;
}
void *ResourceGetDataByName(const char *name) {
  if (strstr(name, "gEff") && accentMissing)
    return nullptr;
  if (strstr(name, "gEff") && accentMalformed)
    return brokenList;
  if (strstr(name, "gEffFire2DL"))
    return accentNativeList[0];
  if (strstr(name, "gEffIceFragment1DL"))
    return accentNativeList[1];
  if (strstr(name, "gEffSparklesDL"))
    return accentNativeList[2];
  if (missing && strstr(name, "cylinder"))
    return nullptr;
  if (shippedResources && IsShippedList(name)) {
    if (malformed)
      return brokenList;
    return const_cast<Gfx *>(ShippedList(name).data());
  }
  if (!strncmp(name, "native", 6) || strstr(name, "gSpinAttackChargingDL"))
    return nativeList;
  if (strstr(name, "burst0") || strstr(name, "burst1") ||
      strstr(name, "charge0") || strstr(name, "charge1"))
    return &textureByte;
  return malformed ? brokenList : privateList;
}
size_t ResourceGetSizeByName(const char *name) {
  if (shippedResources && IsShippedList(name))
    return malformed ? sizeof(brokenList)
                     : ShippedList(name).size() * sizeof(Gfx);
  if (strstr(name, "gEff"))
    return accentMalformed ? sizeof(brokenList) : sizeof(accentNativeList[0]);
  return (!strncmp(name, "native", 6) || strstr(name, "gSpinAttackChargingDL"))
             ? sizeof(nativeList)
             : sizeof(privateList);
}
uint64_t ResourceGetCrcByName(const char *name) {
  if (shippedResources)
    return ShippedResourceHash(name);
  return name[strlen(name) - 1] == '0' ? 0x1111222233334444ULL
                                       : 0x5555666677778888ULL;
}
#ifdef HOST_MM
Color_RGBA8 CosmeticEditor_GetChangedColor(u8 r, u8 g, u8 b, u8 a,
                                           const char *) {
  return cosmeticEdited ? Color_RGBA8{10, 20, 30, a} : Color_RGBA8{r, g, b, a};
}
#endif
#ifdef HOST_MM
void Graph_OpenDisps(Gfx **refs, Gfx *values, GraphicsContext *context,
                     const char *file, s32 line) {
  if (faithfulArena)
    SpinTest_Graph_OpenDisps(refs, values, context, file, line);
}
void Graph_CloseDisps(Gfx **refs, Gfx *values, GraphicsContext *context,
                      const char *file, s32 line) {
  if (faithfulArena)
    SpinTest_Graph_CloseDisps(refs, values, context, file, line);
}
void Gfx_SetupDL25_Xlu(GraphicsContext *) { ++xluSetups; }
#else
void Graph_OpenDisps(Gfx **refs, GraphicsContext *context, const char *file,
                     s32 line) {
  if (faithfulArena)
    SpinTest_Graph_OpenDisps(refs, context, file, line);
}
void Graph_CloseDisps(Gfx **refs, GraphicsContext *context, const char *file,
                      s32 line) {
  if (faithfulArena)
    SpinTest_Graph_CloseDisps(refs, context, file, line);
}
void Gfx_SetupDL_25Xlu(GraphicsContext *) { ++xluSetups; }
#endif
void FrameInterpolation_RecordOpenChild(const void *, int) {}
void FrameInterpolation_RecordCloseChild() {}
}
#include "combo/render/SpinAttackAccentsDraw.h"
#include "combo/render/SpinAttackDraw.h"
#include "spin_draw_production.inc"

static size_t AccentDraws() {
  size_t count = 0;
  for (const auto *list : submittedLists)
    count += (list[0].words.w0 >> 24) == 0x33;
  return count;
}

static void CheckRestoredColors(uint32_t primary, uint32_t environment,
                                uint8_t alpha) {
  const Gfx *end = gfx.polyXlu.p;
  assert(((end - 2)->words.w0 >> 24) == 0xfa);
  assert((end - 2)->words.w1 == ((primary << 8) | alpha));
  assert(((end - 1)->words.w0 >> 24) == 0xfb);
  assert((end - 1)->words.w1 == ((environment << 8) | 128));
  const Gfx *lastMatrix = nullptr;
  for (const Gfx *command = xlu; command < end; ++command) {
    assert((command->words.w0 >> 24) != 0xdb);
    if ((command->words.w0 >> 24) == 0xda)
      lastMatrix = command;
  }
  assert(lastMatrix && !matrixSnapshots.empty());
  bool parentRestored = false;
  for (auto it = matrixSnapshots.rbegin(); it != matrixSnapshots.rend(); ++it)
    if (lastMatrix->words.w1 == (uintptr_t)it->first) {
      assert(std::memcmp(&it->second, &parentMatrix, sizeof(parentMatrix)) ==
             0);
      parentRestored = true;
      break;
    }
  assert(parentRestored);
}

static void ResetDraw() {
  gfx.polyXlu.p = xlu;
  gfx.overlay.p = debugOverlay;
  gfx.polyOpa.p = opa;
  gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
  submittedVertices = allocationCursor = xluSetups = hiliteCalls = 0;
  submittedLists.clear();
  particleColors.clear();
  particleAlphas.clear();
  matrixSnapshots.clear();
}

static void CheckShippedThemeBindings() {
  static const char *names[] = {"",           "fire",      "ice",
                                "light",      "lightning", "arcane",
#ifdef HOST_MM
                                "astral_mm",
#else
                                "astral_soh",
#endif
                                "dark_energy"};
  shippedResources = true;
  for (int style = 1; style < CW_SPIN_STYLE_COUNT; ++style) {
    const std::string prefix =
        std::string("objects/combo_spin/") + names[style] + "/";
    for (int great = 0; great <= 1; ++great) {
      const auto visual = CwSpinStyle_Resolve(style, 0, 0, 1);
      const std::string disk = prefix + (great ? "disk_great" : "disk_regular");
      const std::string cylinder =
          prefix + (great ? "cylinder_great" : "cylinder_regular");
      const auto beforeDisk = ShippedList(disk.c_str());
      const auto beforeCylinder = ShippedList(cylinder.c_str());
      ResetDraw();
      assert(CwSpinDraw_Burst(&play, visual, great, .5f, "native0", "native1"));
      assert(submittedLists.size() == 2);
      for (int part = 0; part < 2; ++part) {
        std::string first = prefix + "burst0";
        std::string second = prefix + "burst1";
        if (style == CW_SPIN_STYLE_ASTRAL) {
#ifndef HOST_MM
          first = prefix + "spin_astral_rupture_poc1/gAstralFlowTex";
#endif
          if (!great && !part)
            second = prefix + "spin_astral_rupture_poc1/gAstralSigil1Tex";
          else if (great)
            second =
                prefix + (part ? "spin_astral_rupture_poc1/gAstralCorona2Tex"
                               : "spin_astral_rupture_poc1/gAstralSigil2Tex");
        } else if (great && style != CW_SPIN_STYLE_ICE &&
                   style != CW_SPIN_STYLE_LIGHT) {
          second = prefix + "spin_lightning_mm_poc1/gSpinLightningLevel2Tex";
        }
        CheckShippedMaterial(submittedLists[part],
                             part ? beforeCylinder : beforeDisk, first, second);
      }
      assert(std::memcmp(beforeDisk.data(), ShippedList(disk.c_str()).data(),
                         beforeDisk.size() * sizeof(Gfx)) == 0);
      assert(std::memcmp(beforeCylinder.data(),
                         ShippedList(cylinder.c_str()).data(),
                         beforeCylinder.size() * sizeof(Gfx)) == 0);
      for (int failure = 0; failure < 2; ++failure) {
        missing = failure == 0;
        malformed = failure == 1;
        ResetDraw();
        assert(
            !CwSpinDraw_Burst(&play, visual, great, .5f, "native0", "native1"));
        assert(submittedLists.empty() && gfx.polyXlu.p == xlu);
      }
      missing = malformed = false;
      if (great &&
          (style == CW_SPIN_STYLE_ICE || style == CW_SPIN_STYLE_LIGHT)) {
#ifndef HOST_MM
        allocationFailure = true;
        ResetDraw();
        assert(
            !CwSpinDraw_Burst(&play, visual, great, .5f, "native0", "native1"));
        assert(submittedLists.empty() && gfx.polyXlu.p == xlu);
        allocationFailure = false;
#endif
        ResetDraw();
        gfx.polyOpa.p =
            reinterpret_cast<Gfx *>(allocation + sizeof(allocation) / 2);
        gfx.polyOpa.d = gfx.polyOpa.p + 1;
        const auto *tail = gfx.polyOpa.d;
        assert(
            !CwSpinDraw_Burst(&play, visual, great, .5f, "native0", "native1"));
        assert(submittedLists.empty() && gfx.polyXlu.p == xlu &&
               gfx.polyOpa.d == tail);
        // Exercise both native Open/CloseDisps bodies and real tail movement.
        // OoT removes its temporary OPA marker without restoring the word,
        // so a clone exactly touching the head must never be submitted damaged.
        for (int headroom = 0; headroom <= 1; ++headroom) {
          ResetDraw();
          faithfulArena = true;
          size_t bytes =
              2 * (beforeDisk.size() + beforeCylinder.size()) * sizeof(Gfx);
          gfx.polyOpa.p =
              reinterpret_cast<Gfx *>(allocation + sizeof(allocation) - bytes) -
              headroom;
          const int drawn =
              CwSpinDraw_Burst(&play, visual, great, .5f, "native0", "native1");
          if (drawn) {
            assert(submittedLists.size() == 2);
            CheckShippedMaterial(submittedLists[0], beforeDisk,
                                 prefix + "burst0", prefix + "burst1");
            CheckShippedMaterial(submittedLists[1], beforeCylinder,
                                 prefix + "burst0", prefix + "burst1");
          } else {
            assert(submittedLists.empty() && gfx.polyXlu.p == xlu);
          }
          if (headroom)
            assert(drawn);
          faithfulArena = false;
        }
      }
    }
  }
  shippedResources = false;
  ResetDraw();
  std::puts("PASS actual shipped themes at both levels, authored shell "
            "preservation and fallback");
}

static void CheckNativeAccents() {
  Gfx original[3][14];
  std::memcpy(original, accentNativeList, sizeof(original));
  for (int element = 1; element <= 3; ++element) {
    for (int great = 0; great <= 1; ++great) {
      ResetDraw();
      const MtxF before = parentMatrix;
      CwSpinAccents_Draw(&play, element, great, .3f, .5f);
      const size_t count = great ? 24 : 12;
      assert(AccentDraws() == count && submittedVertices == 0);
      assert(matrixSnapshots.size() == count + 1);
      assert(std::memcmp(&before, &parentMatrix, sizeof(before)) == 0);
      assert(xluSetups == 2 && hiliteCalls == (element == 2 ? count : 0));
      const Gfx *material = submittedLists[0];
      assert(material != accentNativeList[element - 1]);
      assert(material[1].words.w1 == (unsigned)element);
      // Actual native texture/UV-bearing vertices, combiner, translucency and
      // triangles survive. A missing texture, shade fan or wrong donor fails.
      for (int i : {2, 3, 4, 5, 6, 7, 8, 12, 13})
        assert(std::memcmp(material + i, original[element - 1] + i,
                           sizeof(Gfx)) == 0);
      assert((material[9].words.w0 >> 24) == 0xe7);
      assert((material[10].words.w0 >> 24) == 0xe7);
      if (element != 3) {
        assert((material[11].words.w0 >> 24) == 0xde);
        assert(material[11].words.w1 != 0x08000001);
        const auto *scroll = (const Gfx *)material[11].words.w1;
        assert((scroll[1].words.w0 >> 24) == G_SETTILESIZE_LERP);
        assert((scroll[6].words.w0 >> 24) == G_SETTILESIZE_LERP);
        assert((scroll[11].words.w0 >> 24) == 0xdf);
        uint32_t startBits = scroll[7].words.w1, endBits = scroll[9].words.w1;
        float start, end;
        std::memcpy(&start, &startBits, sizeof(start));
        std::memcpy(&end, &endBits, sizeof(end));
        assert(end - start == (element == 1 ? -20 : 2));
      }
      for (const Gfx *command = xlu; command < gfx.polyXlu.p; ++command)
        assert((command->words.w0 >> 24) != 0xdb); // No segment mutation.
      assert(((gfx.polyXlu.p - 1)->words.w0 >> 24) == 0xda);
      assert(
          (gfx.polyXlu.p - 1)->words.w1 ==
          (uintptr_t)matrixSnapshots[0].first); // Parent GPU matrix restored.
      assert(particleColors[0] == (element == 1   ? 0xff8c2d
                                   : element == 2 ? 0x91e1ff
                                                  : 0xfff2aa));
      assert(particleAlphas[0] ==
             85); // Native material cannot replace the fade with 255.
      ResetDraw();
      CwSpinAccents_DrawCharge(&play, element, great, 0, 1);
      assert(AccentDraws() == (great ? 8u : 4u));
      const auto &first = matrixSnapshots[1].second;
      assert(std::fabs(first.xw - (great ? -656.25f : -812.5f)) < .001f);
      float centerY = first.yw + (element == 1 ? first.yy * 8000 : 0);
      assert(std::fabs(centerY - .5f) < .001f);
    }
  }
  assert(std::memcmp(original, accentNativeList, sizeof(original)) == 0);
  // The burst's scroll phase advances by up to 2 per tick. Reusing it as a
  // flame frame would jump dozens of animation frames and visibly flicker.
  Gfx animation[12];
  ResetDraw();
  play.gameplayFrames = 10;
  CwSpinAccents_Draw(&play, 1, 0, .3f, .5f);
  const auto *nativeScroll = (const Gfx *)submittedLists[0][11].words.w1;
  std::memcpy(animation, nativeScroll, sizeof(animation));
  ResetDraw();
  CwSpinAccents_Draw(&play, 1, 0, 1.7f, .5f);
  nativeScroll = (const Gfx *)submittedLists[0][11].words.w1;
  assert(std::memcmp(animation, nativeScroll, sizeof(animation)) == 0);
  ResetDraw();
  ++play.gameplayFrames;
  CwSpinAccents_Draw(&play, 1, 0, 1.7f, .5f);
  nativeScroll = (const Gfx *)submittedLists[0][11].words.w1;
  assert(std::memcmp(animation, nativeScroll, sizeof(animation)) != 0);
  play.gameplayFrames = 0;
  Gfx uninspected[14], scroll[12];
  std::memcpy(uninspected, accentNativeList[0], sizeof(uninspected));
  uninspected[11].words.w1 = 0x09000000;
  assert(!CwSpinAccents_NativeMaterial(nullptr, uninspected, 14, scroll));
  uninspected[11].words.w0 = 0xda000003;
  assert(!CwSpinAccents_NativeMaterial(nullptr, uninspected, 14, scroll));
  assert(
      !CwSpinAccents_NativeMaterial(nullptr, accentNativeList[0], 3, scroll));
  assert(
      !CwSpinAccents_NativeMaterial(nullptr, accentNativeList[0], 129, scroll));
  ResetDraw();
  CwSpinAccents_DrawLimited(&play, 1, 1, .3f, .5f, 8, 0xe61946);
  assert(AccentDraws() == 8 && submittedLists[0][1].words.w1 == 3);
  assert(particleColors[0] ==
         0xe61946); // Native crimson sparks, not red flame fans.
  for (int failure = 0; failure < 3; ++failure) {
    ResetDraw();
    accentMissing = failure == 0;
    accentMalformed = failure == 1;
    if (failure == 2)
      gfx.polyOpa.d = opa + 1;
    const void *tail = gfx.polyOpa.d;
    CwSpinAccents_Draw(&play, 1, 1, .3f, .5f);
    assert(gfx.polyXlu.p == xlu && gfx.polyOpa.d == tail);
    assert(submittedLists.empty() && matrixSnapshots.empty());
  }
  accentMissing = accentMalformed = false;
#ifndef HOST_MM
  ResetDraw();
  allocationFailure = true;
  CwSpinAccents_Draw(&play, 1, 1, .3f, .5f);
  assert(gfx.polyXlu.p == xlu && submittedLists.empty());
  allocationFailure = false;
#endif
  // Camera changes affect flame/sparkle orientation; a shard remains a 3D
  // object in the scaled parent frame. Hand-computed rotated/scaled fixture.
  MtxF parent = {}, camera = {}, model;
  parent.xy = -3;
  parent.yx = 2;
  parent.zz = 4;
  parent.ww = 1;
  parent.xw = 100;
  parent.yw = 200;
  parent.zw = 300;
  camera.zx = -1;
  camera.yy = 1;
  camera.xz = 1;
  camera.ww = 1;
  CwSpinAccentParticle p = {10, 20, 30, 16, 255, 255, 255, 255};
  CwSpinAccents_NativeModel(&model, &parent, &camera, &p, 2, 0, 0);
  assert(model.xw == 40 && model.yw == 220 && model.zw == 420);
  assert(model.xy == -3 && model.yx == 2 && model.zz == 4);
  CwSpinAccents_NativeModel(&model, &parent, &camera, &p, 2, .5f, 0);
  assert(std::fabs(model.zx) > .1f && std::fabs(model.zy) > .1f);
  p.size = 14;
  CwSpinAccents_NativeModel(&model, &parent, &camera, &p, 3, 0, 0);
  assert(model.xw == 40 && model.yw == 220 && model.zw == 420);
  assert(model.xx == 0 && model.zx == -2 && model.yy == 3 && model.xz == 4);
  p.size = 8;
  CwSpinAccents_NativeModel(&model, &parent, &camera, &p, 1, 0, 0);
  assert(std::fabs(model.yw - 196) < .001f);
  assert(std::fabs(model.yw + model.yy * 8000 - 220) < .001f);
  assert(model.xx == 0 && model.xy == 0 && model.yx == 0 && model.zx < 0);
  ResetDraw();
}

static void CheckBurstTemporalMotion() {
  for (int element = 0; element <= 3; ++element) {
    for (int great = 0; great <= 1; ++great) {
      auto visual = CwSpinStyle_Resolve(element ? CW_SPIN_STYLE_LIGHTNING
                                                : CW_SPIN_STYLE_DARK_ENERGY,
                                        1, element, 1);
      ResetDraw();
      play.gameplayFrames = 10;
      CwSpinDraw_Accents(&play, visual, great, 0, 1);
      const auto previous = matrixSnapshots;
      ResetDraw();
      ++play.gameplayFrames;
      // Both native burst actors advance their texture counter by 2*alpha.
      CwSpinDraw_Accents(&play, visual, great, 2, 1);
      const size_t particles = element ? (great ? 24 : 12) : (great ? 8 : 4);
      assert(previous.size() == matrixSnapshots.size() &&
             matrixSnapshots.size() == particles + 1);
      for (size_t i = 1; i < matrixSnapshots.size(); ++i) {
        const MtxF &a = previous[i].second, &b = matrixSnapshots[i].second;
        assert(std::sqrt((a.xw - b.xw) * (a.xw - b.xw) +
                         (a.yw - b.yw) * (a.yw - b.yw) +
                         (a.zw - b.zw) * (a.zw - b.zw)) < 350);
        const float axes[][6] = {{a.xx, a.yx, a.zx, b.xx, b.yx, b.zx},
                                 {a.xy, a.yy, a.zy, b.xy, b.yy, b.zy},
                                 {a.xz, a.yz, a.zz, b.xz, b.yz, b.zz}};
        for (const auto &v : axes) {
          float from = 0, to = 0, midpoint = 0;
          for (int j = 0; j < 3; ++j) {
            from += v[j] * v[j];
            to += v[j + 3] * v[j + 3];
            midpoint += (v[j] + v[j + 3]) * (v[j] + v[j + 3]) * .25f;
          }
          // The real frame interpolator lerps these matrix axes. A large
          // per-tick turn collapses a shard/sparkle between rendered frames.
          assert(std::sqrt(midpoint) >
                 .98f * (std::sqrt(from) + std::sqrt(to)) * .5f);
        }
      }
    }
  }
  ResetDraw();
}

int main() {
  parentMatrix.xx = parentMatrix.yy = parentMatrix.zz = parentMatrix.ww = 1;
  play.billboardMtxF = parentMatrix;
  // Native effect fixtures retain hashed texture/UV-bearing geometry, native
  // blend/combine commands and the flame/ice scroll call. Do not replace the
  // renderer with a fake: inspect the production GBI it submits below.
  for (int i = 0; i < 3; ++i) {
    auto *dl = accentNativeList[i];
    dl[0].words.w0 = 0x33000000;
    dl[1].words.w1 = i + 1;
    dl[2].words.w0 = 0x20900000;
    dl[3].words.w0 = 0x12345678;
    dl[3].words.w1 = 0x9abcdef0;
    dl[4].words.w0 = 0x32004008;
    dl[5].words.w0 = 0x98765432;
    dl[5].words.w1 = 0x10fedcba;
    dl[6].words.w0 = i == 2 ? 0xfc3097ff : 0xfc272c60;
    dl[6].words.w1 = i == 2 ? 0x5ffefe38 : i ? 0x3514937f : 0x150ce37f;
    dl[7].words.w0 = 0xe200001c;
    dl[7].words.w1 = i == 1 ? 0x0c1849d8 : 0x0c184b50;
    dl[8].words.w0 = 0xd9ffffff;
    dl[8].words.w1 = i == 1 ? 0x000e0000 : 0;
    dl[9].words.w0 = 0xfa000080;
    dl[9].words.w1 = 0xaaffffff;
    dl[10].words.w0 = 0xfb000000;
    dl[10].words.w1 = 0x003264ff;
    dl[11].words.w0 = i == 2 ? 0xe7000000 : 0xde000000;
    dl[11].words.w1 = i == 2 ? 0 : 0x08000001;
    dl[12].words.w0 = 0x06000204;
    dl[12].words.w1 = 0x00000406;
    dl[13].words.w0 = 0xdf000000;
  }
  // Catch accidental global patching, lost geometry, bad expanded-op lengths,
  // unsupported branch handling, and overwriting the caller's alpha.
  Gfx source[8] = {}, copy[8], output[20];
  source[0].words.w0 = 0xFB000000;
  source[0].words.w1 = 0x1122335b;
  source[1].words.w0 = 0x20900000;
  source[2].words.w0 = 0x1234;
  source[2].words.w1 = 0x5678;
  source[3].words.w0 = 0x32003006;
  source[4].words.w0 = 0x1357;
  source[4].words.w1 = 0x2468;
  source[5].words.w0 = 0x05000204;
  source[6].words.w0 = 0xFD900000;
  source[7].words.w0 = 0xDF000000;
  std::memcpy(copy, source, sizeof(source));
  assert(CwSpinMaterial_Clone(output, 20, source, 8, 0x1111222233334444ULL,
                              0x5555666677778888ULL, 0xaabbcc, 0x778899) == 9);
  assert(std::memcmp(copy, source, sizeof(source)) == 0);
  assert(output[0].words.w1 == 0x7788995b);
  assert(output[2].words.w0 == 0x11112222 && output[2].words.w1 == 0x33334444);
  assert(std::memcmp(output + 3, source + 3, 3 * sizeof(Gfx)) == 0);
  assert(output[7].words.w0 == 0x55556666 && output[7].words.w1 == 0x77778888);
  assert(CwSpinMaterial_Clone(output, 1, source, 8, 1, 2, 0, 0) == 0);
  source[3].words.w0 =
      0x31000000; // Uninspected child DL must retain native fallback.
  assert(CwSpinMaterial_Clone(output, 20, source, 8, 1, 2, 0, 0) == 0);
  source[0].words.w0 = 0x20900000;
  assert(CwSpinMaterial_Clone(output, 20, source, 1, 1, 2, 0, 0) == 0);
  Gfx flatTint[3] = {};
  flatTint[0].words.w0 = 0xFD900000;
  flatTint[1].words.w0 = 0xFCFF95FF;
  flatTint[1].words.w1 = 0xFF0DFEFF;
  flatTint[2].words.w0 = 0xDF000000;
  assert(CwSpinMaterial_Clone(output, 20, flatTint, 3, 1, 2, 0x0A141E,
                              0x46505A) == 4);
  assert(((output[2].words.w0 >> 20) & 15) == 3);
  assert(((output[2].words.w1 >> 28) & 15) == 5);
  play.state.gfxCtx = &gfx;
  gfx.polyXlu.p = xlu;
  gfx.polyOpa.p = opa;
  gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
  CwSpinAccents_Draw(&play, 2, 1, .3f, .5f);
  // Regression: elemental accents must submit the native textured effect
  // geometry. The former shade-only triangle fans never submit a donor DL.
  assert(!submittedLists.empty());
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p > xlu);
#ifdef HOST_MM
  allocated = allocation + sizeof(allocation) - (uint8_t *)gfx.polyOpa.d;
#endif
  assert(allocated > 0 && allocated <= 24 * 9 * sizeof(Vtx));
  const auto end = gfx.polyXlu.p;
  CwSpinAccents_Draw(&play, 0, 1, .3f, .5f);
  assert(gfx.polyXlu.p == end);
  CheckNativeAccents();
  CheckBurstTemporalMotion();
  std::memcpy(nativeList, copy, sizeof(nativeList));
  privateList[0].words.w0 = privateList[2].words.w0 = 0x20900000;
  privateList[1].words.w0 = privateList[3].words.w0 = 0x12345678;
  privateList[1].words.w1 = privateList[3].words.w1 = 0x9abcdef0;
  privateList[4].words.w0 = 0xE7000000;
  privateList[5].words.w0 = 0xDF000000;
  CheckShippedThemeBindings();
  for (unsigned element = 1; element <= 3; ++element) {
    ResetDraw();
    assert(CwSpinDraw_Burst(&play, CwSpinStyle_Resolve(0, 1, element, 1), 0,
                            .5f, "native0", "native1"));
    assert(submittedLists.size() == 2 && submittedLists[0] != privateList &&
           submittedLists[1] != privateList);
    assert(std::memcmp(nativeList, copy, sizeof(nativeList)) == 0);
  }
  const auto beforeFallback = gfx.polyXlu.p;
  auto noLayer = CwSpinStyle_Resolve(0, 1, 0, 1);
  assert(!CwSpinDraw_Burst(&play, noLayer, 0, .5f, "native0", "native1"));
  assert(gfx.polyXlu.p == beforeFallback);
  missing = true;
  auto arcane = CwSpinStyle_Resolve(5, 1, 2, 1);
  assert(!CwSpinDraw_Burst(&play, arcane, 0, .5f, "native0", "native1"));
  assert(gfx.polyXlu.p == beforeFallback);
  missing = false;
  malformed = true;
  assert(!CwSpinDraw_Burst(&play, arcane, 0, .5f, "native0", "native1"));
  assert(gfx.polyXlu.p == beforeFallback);
  malformed = false;
  assert(CwSpinDraw_Burst(&play, arcane, 0, .5f, "native0", "native1"));
  assert(CwSpinDraw_Burst(&play, CwSpinStyle_Resolve(0, 1, 2, 1), 0, .5f,
                          "native0", "native1"));
  cosmeticEdited = true;
  for (int style = 0; style < CW_SPIN_STYLE_COUNT; ++style) {
    for (int great = 0; great <= 1; ++great) {
      gfx.polyXlu.p = xlu;
      gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
      submittedVertices = allocationCursor = 0;
      submittedLists.clear();
      auto visual = CwSpinStyle_Resolve(style, 1, 2, 1);
      assert(CwSpinDraw_Charge(&play, visual, great, .25f, "native0"));
      assert((xlu[0].words.w1 >> 8) == 0x0A141E);
#ifdef HOST_MM
      assert((xlu[1].words.w1 >> 8) == 0x0A141E);
#else
      assert((xlu[1].words.w1 >> 8) == 0x46505A);
#endif
      assert(AccentDraws() == (great ? 8u : 4u) && submittedVertices == 0);
#ifdef HOST_MM
      CheckRestoredColors(0x0a141e, 0x0a141e, 63);
#else
      CheckRestoredColors(0x0a141e, 0x46505a, 63);
#endif
      gfx.polyXlu.p = xlu;
      gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
      allocationCursor = 0;
      assert(CwSpinDraw_Burst(&play, visual, great, .5f, "native0", "native1"));
      assert((xlu[0].words.w1 >> 8) == 0x0A141E);
#ifdef HOST_MM
      assert((xlu[1].words.w1 >> 8) == 0x0A141E);
#else
      assert((xlu[1].words.w1 >> 8) == 0x46505A);
#endif
    }
  }
  Player owner = {};
#ifdef HOST_MM
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &owner.actor;
#else
  play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &owner.actor;
#endif
  EnMThunder thunder = {};
#ifdef HOST_MM
  thunder.type = ENMTHUNDER_TYPE_KOKIRI_SWORD;
  thunder.subtype = ENMTHUNDER_SUBTYPE_SPIN_REGULAR;
  thunder.unk1B0 = .5f;
  thunder.chargingAlpha = 128;
  const int scene = play.sceneId;
#else
  thunder.attackStrength = 1;
  thunder.spinChargePercent = .5f;
  thunder.chargeAlpha = 128;
  const int scene = play.sceneNum;
#endif
  CwSpinAttack_Init(&thunder.spinStyle, 5, 1, 1, (uintptr_t)&owner,
                    (uintptr_t)&play, scene);
  for (int step = 0; step < 3; ++step) {
    CwSpinAttack_Preview(&thunder.spinStyle, step == 0 ? 1 : 2, 1, step == 1);
    gfx.polyXlu.p = xlu;
    gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
    submittedVertices = allocationCursor = 0;
    submittedLists.clear();
    particleColors.clear();
    assert(EnMThunder_DrawChargeStyle(&thunder, &play));
    assert(AccentDraws() == 4 && submittedVertices == 0);
    assert(particleColors[0] == (step < 2 ? 0xFF8C2D : 0x91E1FF));
  }
  CwSpinAttack_Release(&thunder.spinStyle, 2, 1, 0x01000000, 0x800, 0x1000,
                       0x2000);
  CwSpinAttack_Preview(&thunder.spinStyle, 3, 1, 0);
  submittedVertices = allocationCursor = 0;
  submittedLists.clear();
  gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
  assert(EnMThunder_DrawChargeStyle(&thunder, &play));
  assert(thunder.spinStyle.element == 2 && AccentDraws() == 0);
  assert(CwSpinDraw_Charge(&play, arcane, 1, .25f, "native0"));
  for (int element = 0; element <= 3; ++element) {
    submittedVertices = allocationCursor = 0;
    submittedLists.clear();
    gfx.polyOpa.d = (Gfx *)(allocation + sizeof(allocation));
    CwSpinDraw_Accents(&play, CwSpinStyle_Resolve(7, 1, element, 1), 1, .4f,
                       .6f);
    assert(AccentDraws() == (element ? 24u : 8u) && submittedVertices == 0);
#ifdef HOST_MM
    CheckRestoredColors(0x0a141e, 0x0a141e, 153);
#else
    CheckRestoredColors(0x0a141e, 0x46505a, 153);
#endif
    assert(gfx.polyOpa.p == opa);
  }
  ResetDraw();
  accentMissing = true;
  CwSpinDraw_Accents(&play, CwSpinStyle_Resolve(7, 1, 2, 1), 1, .4f, .6f);
  assert(gfx.polyXlu.p == xlu && matrixSnapshots.empty());
  assert(CwSpinDraw_Charge(&play, arcane, 1, .25f, "native0"));
  assert(AccentDraws() == 0 && ((gfx.polyXlu.p - 1)->words.w0 >> 24) == 0xde);
  accentMissing = false;
  std::puts("PASS real-header native material clones, alpha preservation, "
            "bounded XLU accent submission");
  return 0;
}
