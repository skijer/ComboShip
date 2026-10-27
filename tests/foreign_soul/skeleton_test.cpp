#include <cassert>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using s32 = int32_t;
using u8 = uint8_t;
using s8 = int8_t;
using u32 = uint32_t;
using s16 = int16_t;
using f32 = float;
struct Color_RGB8 { uint8_t r, g, b; };
struct Vec3s {
  int16_t x = 0, y = 0, z = 0;
};
struct Vec3f {
  float x = 0, y = 0, z = 0;
};
struct Mtx {
  float x = 0, y = 0, z = 0;
  float sx = 1, sy = 1, sz = 1;
  float rx = 0;
  float padding[9]{};
};
static_assert(sizeof(Mtx) == 64);
struct Gfx {
  int kind = 0, seg = 0;
  uintptr_t target = 0;
};
struct Actor {};
struct GraphicsContext {
  Gfx opa[4096]{}, xlu[4096]{};
  Gfx *op = opa;
  Gfx *xp = xlu;
  std::vector<void *> allocations;
  ~GraphicsContext() {
    for (auto p : allocations)
      std::free(p);
  }
};
struct PlayState {
  struct {
    GraphicsContext *gfxCtx;
    uint32_t frames = 1;
  } state;
  uint32_t gameplayFrames = 1;
  Mtx billboardMtxF;
  Mtx **flexLimbOverrideMTX = nullptr;
};
static PlayState* gPlayState = nullptr;
struct StandardLimb {
  Vec3s jointPos;
  u8 child = 255, sibling = 255;
  Gfx *dList = nullptr;
};
#ifdef HOST_MM
struct SkeletonHeader {
  void **segment;
  u8 limbCount;
};
#else
struct SkeletonHeader {
  void **segment;
  u8 limbCount, skeletonType;
};
#endif
struct FlexSkeletonHeader {
  SkeletonHeader sh;
  u8 dListCount;
};
struct AnimationHeader {};
struct SkelAnime {
  void **skeleton = nullptr;
  Vec3s *jointTable = nullptr;
  u8 dListCount = 0;
  float playSpeed = 1;
  float curFrame = 0;
};
#define OPEN_DISPS(ctx) {
#define CLOSE_DISPS(ctx) }
#define POLY_OPA_DISP play->state.gfxCtx->op
#define POLY_XLU_DISP play->state.gfxCtx->xp
#define SEGMENTED_TO_VIRTUAL(p) ((StandardLimb *)(p))
#define MATRIX_NEWMTX(c) Matrix_NewMtx(c)
#define MATRIX_TOMTX(p) Matrix_ToMtx(p)
#define osSyncPrintf(...) ((void)0)
#define LIMB_DONE 255
#define LIMB_ROOT_POS 0
#define LIMB_ROOT_ROT 1
#define MTXMODE_APPLY 0
#define G_MTX_NOPUSH 0
#define G_MTX_LOAD 0
#define G_MTX_MODELVIEW 0
#define AA_EN 0
#define Z_CMP 0
#define IM_RD 0
#define CLR_ON_CVG 0
#define CVG_DST_WRAP 0
#define ZMODE_XLU 0
#define FORCE_BL 0
#define GBL_c1(...) 0
#define G_RM_AA_ZB_XLU_SURF2 0
#define M_PIf 3.14159265358979323846f
#define ANIMMODE_ONCE 2
#define gDPPipeSync(p) ((p)->kind = 0)
void gDPSetPrimColor(Gfx* p, int, int, int r, int g, int b, int a) {
  *p = {4, 0, (uintptr_t)(((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a)};
}
void gDPSetEnvColor(Gfx* p, int r, int g, int b, int a) {
  *p = {5, 0, (uintptr_t)(((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a)};
}
#define gDPSetRenderMode(p, ...) ((p)->kind = 0)
void gDPSetGrayscaleColor(Gfx* p, int r, int g, int b, int a) {
  *p = {6, 0, (uintptr_t)(((uint32_t)r << 24) | ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)a)};
}
void gSPGrayscale(Gfx* p, bool enable) { *p = {7, 0, (uintptr_t)enable}; }
void gSPComboRMPush(Gfx* p, const char* game) { *p = {8, 0, (uintptr_t)game}; }
void gSPComboRMPop(Gfx* p) { *p = {9, 0, 0}; }
#define gSPEndDisplayList(p) ((p)->kind = 0)
static std::function<void()> checkSkinOwner;
void gSPSegment(Gfx *p, int seg, uintptr_t target) {
  if (checkSkinOwner && seg == 8 && target && !strncmp((const char*)target, "__OTR__objects/object_boss02/gTwinmoldBlueSkinTex", 45)) checkSkinOwner();
  *p = {1, seg, target};
}
void gSPSegment(Gfx *p, int seg, void *target) {
  gSPSegment(p, seg, (uintptr_t)target);
}
void gSPMatrix(Gfx *p, Mtx *target, int) { *p = {2, 0, (uintptr_t)target}; }
void gSPDisplayList(Gfx *p, Gfx *target) { *p = {3, 0, (uintptr_t)target}; }
void *Graph_Alloc(GraphicsContext *c, size_t n) {
  auto p = std::calloc(1, n);
  assert(p);
  c->allocations.push_back(p);
  return p;
}
#define GRAPH_ALLOC(c, n) Graph_Alloc(c, n)
static Mtx current;
static std::vector<Mtx> matrices;
void Matrix_Push() { matrices.push_back(current); }
void Matrix_Pop() {
  current = matrices.back();
  matrices.pop_back();
}
void Matrix_Translate(float x, float y, float z, int) {
  current.x += x;
  current.y += y;
  current.z += z;
}
void Matrix_Scale(float x, float y, float z, int) {
  current.sx *= x; current.sy *= y; current.sz *= z;
}
void Matrix_RotateX(float a, int) { current.rx += a; }
void Matrix_RotateXF(float a, int mode) { Matrix_RotateX(a, mode); }
float Math_SinS(s16 a) { return std::sin(a * M_PIf / 32768.0f); }
float Math_CosS(s16 a) { return std::cos(a * M_PIf / 32768.0f); }
void Matrix_ReplaceRotation(Mtx *) {}
void Matrix_TranslateRotateZYX(Vec3f *p, Vec3s *) {
  Matrix_Translate(p->x, p->y, p->z, 0);
}
Mtx *Matrix_ToMtx(Mtx *p, char* = nullptr, int = 0) {
  *p = current;
  return p;
}
Mtx *Matrix_NewMtx(GraphicsContext *c, char * = nullptr, int = 0) {
  return Matrix_ToMtx((Mtx *)Graph_Alloc(c, sizeof(Mtx)));
}
#define MATRIX_FINALIZE_AND_LOAD(p, c) gSPMatrix(p, Matrix_NewMtx(c), 0)
template <class T> T *Lib_SegmentedToVirtual(T *p) { return p; }
void Gfx_SetupDL25_Opa(GraphicsContext *) {}
void Gfx_SetupDL25_Xlu(GraphicsContext *) {}
void Gfx_SetupDL_25Opa(GraphicsContext *) {}
void Gfx_SetupDL_25Xlu(GraphicsContext *) {}
static std::vector<std::array<int64_t, 16>> scrollRequests;
Gfx *Gfx_TwoTexScrollEx(GraphicsContext*, int a, u32 x1, u32 y1, int w1, int h1,
                      int b, u32 x2, u32 y2, int w2, int h2, int sx1, int sy1, int sx2, int sy2) {
  scrollRequests.push_back({a, x1, y1, w1, h1, b, x2, y2, w2, h2, sx1, sy1, sx2, sy2, 0, 0});
  static Gfx p;
  return &p;
}
template <class... T> Gfx *Gfx_TexScrollEx(T...) {
  static Gfx p;
  return &p;
}
static const char gameplay_keep_DL_01ACF0[] = "__OTR__objects/gameplay_keep/gameplay_keep_DL_01ACF0";
#ifdef HOST_MM
float Rand_ZeroOne() { return 0.0f; }
#include "native_soul.inc"
using LimbArg = Actor *;
#define COMBO_FOREIGN_ANIM_HOST_MM 1
#else
using LimbArg = void *;
#endif
using OverrideLimbDrawOpa = s32 (*)(PlayState *, s32, Gfx **, Vec3f *, Vec3s *,
                                    LimbArg);
using PostLimbDrawOpa = void (*)(PlayState *, s32, Gfx **, Vec3s *, LimbArg);
#ifndef HOST_MM
// Boss-soul fixtures have no native NPC skin plan. Execute the real native
// helper's null-plan path and reject unexpected palette blending.
using MtxF = Mtx;
static const void* NativeNpcPose_GetSkinPlan(void**, s32) { return nullptr; }
static float NativeNpcPose_GetDrawScale(void**) { return 1.f; }
static s32 NativeNpcPose_SkinMatrixCount(const void* plan) {
  assert(plan == nullptr);
  return 0;
}
static void NativeNpcPose_BlendSkinMatrix(const void*, s32, MtxF*, MtxF*) { assert(false); }
static void Matrix_MtxToMtxF(Mtx* source, MtxF* target) { *target = *source; }
static void Matrix_Put(MtxF* source) { current = *source; }
#endif
extern "C" {
#include "native_draw.inc"
}
static int normalInits = 0, flexInits = 0, updates = 0;
static FlexSkeletonHeader* resolveNativeFlexHeader(FlexSkeletonHeader* header);
static int freezes = 0;
int16_t Animation_GetLastFrame(void*) { return 42; }
void Animation_Change(SkelAnime* a, AnimationHeader*, float speed, float start, float end, u8 mode, float morph) {
  assert(speed == 1 && start == 42 && end == 42 && mode == ANIMMODE_ONCE && morph == 0);
  a->curFrame = start;
  freezes++;
}
void SkelAnime_Init(PlayState *, SkelAnime *a, SkeletonHeader *h,
                    AnimationHeader *, Vec3s *j, Vec3s *, int) {
  normalInits++;
  a->skeleton = h->segment;
  a->jointTable = j;
  j[0] = {10, 0, 0};
}
void SkelAnime_InitFlex(PlayState *, SkelAnime *a, FlexSkeletonHeader *h,
                        AnimationHeader *, Vec3s *j, Vec3s *, int) {
  flexInits++;
  h = resolveNativeFlexHeader(h);
  a->skeleton = h->sh.segment;
  a->jointTable = j;
  a->dListCount = h->dListCount;
  j[0] = {10, 0, 0};
}
s32 SkelAnime_Update(SkelAnime *) {
  updates++;
  return 0;
}
template <class... T>
Gfx *SkelAnime_DrawFlex(PlayState *, void **, Vec3s *, int, T...) {
  assert(false);
  return nullptr;
}
namespace Ship {
struct IResource {
  virtual ~IResource() = default;
  virtual void *GetRawPointer() = 0;
};
struct ResourceManager {
  bool alt = false;
  std::unordered_map<std::string, std::shared_ptr<IResource>> normal,
      replacement;
  bool IsAltAssetsEnabled() { return alt; }
  std::shared_ptr<IResource> LoadResource(const char *p) {
    auto &m = alt ? replacement : normal;
    return m[p];
  }
};
struct Context {
  std::shared_ptr<ResourceManager> rm;
  static Context *GetRawInstance() {
    static Context c;
    return &c;
  }
  std::shared_ptr<ResourceManager> GetResourceManager() { return rm; }
};
struct ResourceManagerScope {
  std::shared_ptr<ResourceManager> before;
  ResourceManagerScope(std::shared_ptr<ResourceManager> r) {
    auto c = Context::GetRawInstance();
    before = c->rm;
    c->rm = r;
  }
  ~ResourceManagerScope() { Context::GetRawInstance()->rm = before; }
};
struct CrossRMRegistry {
  inline static std::unordered_map<std::string,
                                   std::shared_ptr<ResourceManager>>
      owners;
  static std::shared_ptr<ResourceManager> Get(const char *s) {
    return owners[s];
  }
  static void RegisterTeardownListener(void (*)()){};
};
} // namespace Ship
static FlexSkeletonHeader* resolveNativeFlexHeader(FlexSkeletonHeader* header) {
  // Native initialization receives an OTR path; the shared consumer receives
  // the owning factory's loaded header. Resource I/O is the fixture boundary.
  if (!std::strncmp((const char*)header, "__OTR__", 7)) {
    const auto rm = Ship::Context::GetRawInstance()->GetResourceManager();
    const auto resource = rm->LoadResource((const char*)header);
    assert(resource);
    return (FlexSkeletonHeader*)resource->GetRawPointer();
  }
  return header;
}
extern "C" void FrameInterpolation_RecordOpenChild(const void *, int) {}
extern "C" void FrameInterpolation_RecordCloseChild() {}
#include "foreign_anim.h"
#ifndef SKIP_TWINMOLD_TESTS
#include "twinmold_production.inc"
#endif
#ifndef SKIP_BARINADE_TESTS
#include "barinade_production.inc"
#endif
struct SkeletonResource : Ship::IResource {
  CfaLoadedFlexSkeletonHeader h{};
  StandardLimb limbs[2]{};
  void *pointers[2]{};
  std::string head = "__OTR__head", jaw = "__OTR__jaw";
  SkeletonResource(uint8_t type) {
    h.sh = {pointers, 2, type};
    h.dListCount = 2;
    limbs[0].child = 1;
    limbs[0].dList = (Gfx *)head.c_str();
    limbs[1].jointPos = {3, 0, 0};
    limbs[1].dList = (Gfx *)jaw.c_str();
    pointers[0] = &limbs[0];
    pointers[1] = &limbs[1];
  }
  void *GetRawPointer() override { return &h; }
};
struct AnimationResource : Ship::IResource {
  AnimationHeader h;
  void *GetRawPointer() override { return &h; }
};
#if defined(HOST_MM) && !defined(SKIP_TWINMOLD_TESTS)
struct MmBossSkeletonResource : Ship::IResource {
  CfaLoadedFlexSkeletonHeader h{};
  std::vector<StandardLimb> limbs;
  std::vector<void*> pointers;
  std::string body, head;
  MmBossSkeletonResource(int limbCount, bool replacement)
      : limbs(limbCount - 1), pointers(limbCount - 1),
        body(replacement ? "__OTR__mm_soul_custom_body" : "__OTR__mm_soul_native_body"),
        head(replacement ? "__OTR__mm_soul_custom_head" : "__OTR__mm_soul_native_head") {
    h.sh = {pointers.data(), (u8)(limbCount - 1), (u8)(replacement ? 0 : 1)};
    h.dListCount = replacement ? 0 : 2;
    limbs[0].child = 1;
    limbs[0].dList = (Gfx*)body.c_str();
    limbs[1].dList = (Gfx*)head.c_str();
    limbs[1].jointPos = {3, 0, 0};
    for (int i = 0; i < limbCount - 1; ++i) pointers[i] = &limbs[i];
  }
  void* GetRawPointer() override { return &h; }
};
static void checkNativeMmBosses(std::shared_ptr<Ship::ResourceManager> owner) {
  Ship::Context::GetRawInstance()->rm = owner;
  for (auto id : {RI_SOUL_BOSS_GOHT, RI_SOUL_BOSS_GYORG, RI_SOUL_BOSS_ODOLWA}) {
    CwItemAnimDrawInfo info{};
    assert(MM_FillBossSoulAnim(id, &info));
    auto anim = std::make_shared<AnimationResource>();
    owner->normal[info.skelPath] = std::make_shared<MmBossSkeletonResource>(info.limbCount, false);
    owner->replacement[info.skelPath] = std::make_shared<MmBossSkeletonResource>(info.limbCount, true);
    owner->normal[info.animPath] = owner->replacement[info.animPath] = anim;
    CfaClearCaches();
    const int beforeNormal = normalInits, beforeFlex = flexInits;
#ifdef NATIVE_MM_BOSS_ALT_FIRST
    const bool selections[] = {true, false, true, false};
#else
    const bool selections[] = {false, true, false, true};
#endif
    for (bool alt : selections) {
      owner->alt = alt;
      GraphicsContext c; PlayState p{{&c, 221}, 70, {}};
      gPlayState = &p;
      current = {};
      switch (id) {
        case RI_SOUL_BOSS_GOHT: DrawGoht(); break;
        case RI_SOUL_BOSS_GYORG: DrawGyorg(); break;
        case RI_SOUL_BOSS_ODOLWA: DrawOdolwa(); break;
        default: assert(false);
      }
      int meshes = 0, matrixSegments = 0;
      const Mtx* modelMatrix = nullptr;
      for (Gfx* cmd = c.opa; cmd < c.op; ++cmd) {
        if (cmd->kind == 2) modelMatrix = (const Mtx*)cmd->target;
        if (cmd->kind == 3) {
          std::string path = (const char*)cmd->target;
          const auto marker = path.find("mm_soul_");
          assert(marker != std::string::npos);
          assert(path.substr(marker).starts_with(alt ? "mm_soul_custom_" : "mm_soul_native_"));
          assert(modelMatrix && modelMatrix->sx == info.scale && modelMatrix->sy == info.scale);
          assert(modelMatrix->y == info.translatePre[1]);
          meshes++;
        }
        matrixSegments += cmd->kind == 1 && cmd->seg == 13;
      }
      assert(meshes == 2);
      // A selected normal replacement must use the rigid drawer and never
      // write into the absent flex matrix array (dListCount is zero).
      assert(alt ? matrixSegments == 0 : matrixSegments >= 1);
      assert(Ship::Context::GetRawInstance()->rm == owner && matrices.empty());
      // Both selected rigs keep the native model transform and soul palette.
      const uintptr_t flameColor = (uintptr_t(info.flameColor[0]) << 24) |
                                   (uintptr_t(info.flameColor[1]) << 16) |
                                   (uintptr_t(info.flameColor[2]) << 8);
      int flames = 0, nativeColors = 0;
      const Mtx* flameMatrix = nullptr;
      for (Gfx* cmd = c.xlu; cmd < c.xp; ++cmd) {
        if (cmd->kind == 2) flameMatrix = (const Mtx*)cmd->target;
        flames += cmd->kind == 3 && !std::strcmp((const char*)cmd->target, gameplay_keep_DL_01ACF0);
        if (cmd->kind == 3) {
          assert(flameMatrix && flameMatrix->sx == info.scale * info.flameScale[0]);
          assert(flameMatrix->sy == info.scale * info.flameScale[1]);
        }
        nativeColors += cmd->kind == 4 && cmd->target == flameColor;
      }
      assert(flames == 1 && nativeColors == 1);
    }
    assert(normalInits == beforeNormal + 1 && flexInits == beforeFlex + 1);
    assert(current.y == 0 && current.sx == 1 && current.sy == 1 && current.sz == 1);
  }
  owner->alt = false;
  gPlayState = nullptr;
  CfaClearCaches();
  std::cout << "Native Goht/Gyorg/Odolwa: live owning Alt selection, selected rigid/flex dispatch, matrix bounds and native flame palettes passed\n";
}
#endif
#ifndef SKIP_TWINMOLD_TESTS
struct TwinmoldSkeletonResource : Ship::IResource {
  CfaLoadedFlexSkeletonHeader h{};
  StandardLimb limbs[12]{};
  void* pointers[12]{};
  std::string paths[12];
  TwinmoldSkeletonResource(bool flex) {
    h.sh = {pointers, 12, (u8)flex}; h.dListCount = 12;
    for (int i = 0; i < 12; ++i) {
      paths[i] = "__OTR__twinmold_" + std::string(flex ? "custom_" : "native_") + std::to_string(i);
      limbs[i].dList = (Gfx*)paths[i].c_str(); pointers[i] = &limbs[i];
      if (i > 0 && i < 11) limbs[i].sibling = i + 1;
    }
    limbs[0].child = 1;
    limbs[1].jointPos = {3, 0, 0};
  }
  void* GetRawPointer() override { return &h; }
};
static void checkTwinmold(std::shared_ptr<Ship::ResourceManager> owner,
                          std::shared_ptr<Ship::ResourceManager> host) {
  CwItemAnimDrawInfo info{};
  assert(MM_FillBossSoulAnim(RI_SOUL_BOSS_TWINMOLD, &info));
  assert(info.proceduralProfile == CW_ANIM_PROFILE_MM_TWINMOLD);
  auto anim = std::make_shared<AnimationResource>();
  owner->normal[info.skelPath] = std::make_shared<TwinmoldSkeletonResource>(false);
  owner->replacement[info.skelPath] = std::make_shared<TwinmoldSkeletonResource>(true);
  owner->normal[info.animPath] = owner->replacement[info.animPath] = anim;
  CfaClearCaches();
  checkSkinOwner = [owner] { assert(Ship::Context::GetRawInstance()->rm == owner); };
  for (bool alt : {false, true, false, true}) {
    owner->alt = alt;
    GraphicsContext c; PlayState p{{&c, 177}, 59, {}};
    current = {}; current.x = 17;
    assert(ComboForeignAnim_Draw(&info, "mm", &p));
    assert(Ship::Context::GetRawInstance()->rm == host && matrices.empty());
    int meshes = 0, matrixSegments = 0, skins = 0, flames = 0, yellow = 0;
    for (Gfx* cmd = c.opa; cmd < c.op; ++cmd) {
      if (cmd->kind == 3) {
        assert(std::string((const char*)cmd->target).starts_with(alt ? "__OTR__@mm:twinmold_custom_" : "__OTR__@mm:twinmold_native_"));
        meshes++;
      }
      if (cmd->kind == 1 && cmd->seg == 8 && !strcmp((const char*)cmd->target, gTwinmoldBlueSkinTex)) skins++;
      if (cmd->kind == 1 && cmd->seg == 13) {
        if (matrixSegments++ == 0) {
          auto matrices = (Mtx*)cmd->target;
          for (int i = 0; i < (alt ? 12 : 23); ++i)
            assert(matrices[i].x >= 17 && matrices[i].y == 0 && matrices[i].sx == .06f);
        }
      }
    }
    for (Gfx* cmd = c.xlu; cmd < c.xp; ++cmd) yellow += cmd->kind == 4 && cmd->target == 0xA8B41400;
    for (Gfx* cmd = c.xlu; cmd < c.xp; ++cmd) if (cmd->kind == 3) {
#ifdef HOST_MM
      assert(!strcmp((const char*)cmd->target, gameplay_keep_DL_01ACF0));
#else
      assert(!strcmp((const char*)cmd->target, "__OTR__@mm:objects/gameplay_keep/gameplay_keep_DL_01ACF0"));
#endif
      flames++;
    }
    assert(meshes == 12 && matrixSegments == 2 && skins == 1 && flames == 1 && yellow == 1);
  }
  {
    GraphicsContext c; PlayState p{{&c, 178}, 60, {}};
    gPlayState = &p; current = {}; current.x = 31;
    DrawTwinmold();
    assert(nativeTwinmoldFallbacks == 0);
    assert(current.x == 31 && current.sx == 1 && matrices.empty());
    twinmoldRecipeAvailable = false;
    c.op = c.opa; c.xp = c.xlu;
    assert(!ComboDrawNativeTwinmoldSoul() && c.op == c.opa && c.xp == c.xlu);
    assert(current.x == 31 && current.sx == 1 && matrices.empty());
    checkSkinOwner = {}; // Standalone native fallback does not run on an OoT host.
    DrawTwinmold();
    assert(nativeTwinmoldFallbacks == 1);
    current = {}; current.x = 31; c.op = c.opa; c.xp = c.xlu;
    twinmoldRecipeAvailable = true;
    Ship::CrossRMRegistry::owners["mm"] = nullptr;
    assert(!ComboDrawNativeTwinmoldSoul() && current.x == 31 && matrices.empty());
    Ship::CrossRMRegistry::owners["mm"] = owner;
    gPlayState = nullptr;
    assert(!ComboDrawNativeTwinmoldSoul());
  }
  for (int failure = 0; failure < 4; ++failure) {
    auto bad = info;
    if (failure == 0) bad.limbCount = 14;
    if (failure == 1) bad.segs[0].segment = 9;
    if (failure == 2) bad.segs[0].path = nullptr;
    if (failure == 3) bad.segCount = 0;
    GraphicsContext c; PlayState p{{&c, 179}, 60, {}};
    assert(!ComboForeignAnim_Draw(&bad, "mm", &p) && c.op == c.opa && c.xp == c.xlu);
  }
  CfaClearCaches();
  auto mismatch = std::make_shared<TwinmoldSkeletonResource>(true);
  mismatch->h.sh.limbCount = 11;
  owner->replacement[info.skelPath] = mismatch;
  GraphicsContext c; PlayState p{{&c, 179}, 60, {}};
  assert(!ComboForeignAnim_Draw(&info, "mm", &p) && c.op == c.opa && c.xp == c.xlu);
  CfaClearCaches();
  owner->alt = false;
  checkSkinOwner = {};
  std::cout << "Twinmold native head recipe: initialized rigid matrix13, replacement flex limbs, MM skin/flame, owner Alt/cache and fallback passed\n";
}
#endif
#ifndef SKIP_BARINADE_TESTS
struct BarinadeSkeletonResource : Ship::IResource {
  CfaLoadedFlexSkeletonHeader h{};
  StandardLimb limbs[63]{};
  void* pointers[63]{};
  std::string paths[63];
  BarinadeSkeletonResource(bool flex) {
    h.sh = {pointers, 63, (u8)flex}; h.dListCount = 63;
    for (int i = 0; i < 63; ++i) {
      paths[i] = "__OTR__barinade_limb_" + std::to_string(i + 1);
      limbs[i].dList = (Gfx*)paths[i].c_str(); pointers[i] = &limbs[i];
      if (i > 0 && i < 62) limbs[i].sibling = i + 1;
    }
    limbs[0].child = 1;
  }
  void* GetRawPointer() override { return &h; }
};
// Barinade's selected flex meshes also draw on XLU and read other limbs via
// segment 13 (the reported crash command samples offset 0x100, matrix 4).
// Replay the real recorded streams independently, after the palette is filled.
static uintptr_t checkBarinadeXluMatrices(GraphicsContext& c, Gfx* opaBegin = nullptr,
                                          Gfx* xluBegin = nullptr, uintptr_t inheritedPalette = 0) {
  uintptr_t opaPalette = 0;
  uintptr_t opaFinalPalette = 0;
  for (Gfx* p = opaBegin ? opaBegin : c.opa; p < c.op; ++p) {
    if (p->kind == 1 && p->seg == 13) {
      if (!opaPalette) opaPalette = p->target;
      opaFinalPalette = p->target;
    }
  }
  assert(opaPalette);
  uintptr_t xluPalette = inheritedPalette;
  int meshes = 0;
  for (Gfx* p = xluBegin ? xluBegin : c.xlu; p < c.xp; ++p) {
    if (p->kind == 1 && p->seg == 13) xluPalette = p->target;
    if (p->kind != 3) continue;
    const std::string path((const char*)p->target);
    if (path.find(":barinade_limb_") == std::string::npos) continue;
    assert(xluPalette && "flex Barinade XLU mesh has no matrix segment 13");
    assert(xluPalette == opaPalette && "XLU must use this draw's actual limb palette");
    const Mtx* sampled = (const Mtx*)(xluPalette + 0x100);
    assert(sampled->x == 10 && "referenced limb matrix must be initialized before playback");
    ++meshes;
  }
  assert(meshes == 27);
  assert(xluPalette != opaPalette && "XLU must release the flex matrix palette after drawing");
  assert(xluPalette == opaFinalPalette && "both streams must restore the same empty display list");
  const Gfx* cleanup = (const Gfx*)xluPalette;
  for (int i = 0; i < CFA_EMPTY_DL_ENTRIES; ++i) assert(cleanup[i].kind == 0);
  return opaPalette;
}
static void checkBarinade(std::shared_ptr<Ship::ResourceManager> owner,
                          std::shared_ptr<Ship::ResourceManager> host) {
  CwItemAnimDrawInfo info{};
  assert(OOT_BossSoulUsesSkeleton(RG_BARINADE_SOUL));
  assert(OOT_FillBossSoulAnim(2, &info));
  assert(info.proceduralProfile == CW_ANIM_PROFILE_OOT_BARINADE && info.freezeLastFrame);
  assert(info.limbCount == 64 && info.scale == .03f && info.translatePre[1] == -25);
  assert(!strcmp(info.skelPath, gBarinadeBodySkel) && !strcmp(info.animPath, gBarinadeBodyAnim));
  assert(!strcmp(info.proceduralDlPaths[0], gBarinadeDL_008D70));
  assert(!strcmp(info.proceduralDlPaths[1], gBarinadeDL_008BB8));
  // Compare the production foreign callback against the actual native callback
  // at every limb, with gameplayFrames distinct from state.frames.
  const char* flamePath = info.flameDlPath;
  info.flameDlPath = nullptr;
  sCfaInfo = &info; sCfaCurrentGame = "oot";
  for (u32 frame : {0U, 7U, 600U}) {
    for (int limb = 1; limb < 64; ++limb) {
      GraphicsContext a, b;
      PlayState pa{{&a, 111}, frame, {}}, pb{{&b, 111}, frame, {}};
      Gfx* da = (Gfx*)"__OTR__limb", *db = da;
      Vec3f posa{}, posb{}; Vec3s ra{13, 14, 15}, rb = ra;
      current = {}; scrollRequests.clear();
      OverrideLimbDrawBarinade(&pa, limb, &da, &posa, &ra, nullptr);
      const Mtx expected = current; const auto expectedScroll = scrollRequests;
      current = {}; scrollRequests.clear();
      CfaOverrideLimbDrawOpa(&pb, limb, &db, &posb, &rb, nullptr);
      assert(ra.x == rb.x && ra.y == rb.y && ra.z == rb.z && (da == nullptr) == (db == nullptr));
      assert(current.rx == expected.rx && current.sx == expected.sx && current.sy == expected.sy && current.sz == expected.sz);
      assert(scrollRequests == expectedScroll);
      a.op = a.opa; a.xp = a.xlu; b.op = b.opa; b.xp = b.xlu;
      da = db = (Gfx*)"__OTR__limb"; current = {}; scrollRequests.clear();
      PostLimbDrawBarinade(&pa, limb, &da, &ra, nullptr);
      const auto postScroll = scrollRequests;
      current = {}; scrollRequests.clear();
      CfaPostLimbDrawOpa(&pb, limb, &db, &rb, nullptr);
      assert(scrollRequests == postScroll);
      std::vector<std::string> nativeDls, foreignDls;
      for (Gfx* p = a.xlu; p < a.xp; ++p) if (p->kind == 3) nativeDls.emplace_back((const char*)p->target);
      for (Gfx* p = b.xlu; p < b.xp; ++p) if (p->kind == 3) foreignDls.emplace_back((const char*)p->target);
      assert(nativeDls.size() == foreignDls.size());
      for (size_t i = 0; i < nativeDls.size(); ++i)
        assert(foreignDls[i] == "__OTR__@oot:" + nativeDls[i].substr(7));
    }
  }
  sCfaInfo = nullptr;
  auto anim = std::make_shared<AnimationResource>();
  owner->normal[info.skelPath] = std::make_shared<BarinadeSkeletonResource>(false);
  owner->replacement[info.skelPath] = std::make_shared<BarinadeSkeletonResource>(true);
  owner->normal[info.animPath] = owner->replacement[info.animPath] = anim;
  CfaClearCaches(); freezes = 0;
  for (bool alt : {false, true, false, true}) {
    owner->alt = alt;
    GraphicsContext c; PlayState play{{&c, 51}, 7, {}};
    current = {}; scrollRequests.clear();
    assert(ComboForeignAnim_Draw(&info, "oot", &play));
    assert(Ship::Context::GetRawInstance()->rm == host && matrices.empty());
    if (alt) checkBarinadeXluMatrices(c);
    int opa = 0, xlu = 0, seg13 = 0, ring = 0, electric = 0;
    for (Gfx* p = c.opa; p < c.op; ++p) {
      if (p->kind == 1 && p->seg == 13) seg13++;
      if (p->kind == 3) {
        auto path = std::string((const char*)p->target);
        assert(path.starts_with("__OTR__@oot:barinade_limb_")); opa++;
        const int limb = std::stoi(path.substr(path.rfind('_') + 1));
        assert(limb < 10 || limb >= 20);
      }
    }
    for (Gfx* p = c.xlu; p < c.xp; ++p) if (p->kind == 3) {
      auto path = std::string((const char*)p->target);
      assert(path.starts_with("__OTR__@oot:") && path.find("@oot:@oot:") == std::string::npos);
      ring += path.ends_with("gBarinadeDL_008D70"); electric += path.ends_with("gBarinadeDL_008BB8"); xlu++;
    }
    assert(opa == 53 && xlu == 38 && ring == 1 && electric == 10 && seg13 == (alt ? 2 : 0));
    assert(scrollRequests.size() == 4);
    assert(scrollRequests[0][7] == (u32)(7U * -10) % 16 && scrollRequests[1][2] == (u32)(7U * -10) % 32);
    assert(scrollRequests[2][7] == (u32)(7U * -2) % 64 && scrollRequests[3][2] == 70);
    assert(c.op[-1].kind == 1 && c.op[-1].seg == 9 && ((Gfx*)c.op[-1].target)[0].kind == 0);
  }
  assert(freezes == 2); // one frozen initialization per owning Alt selection, cache retained
  // The original native opaque entry must retain its previous XLU state.
  {
    auto flex = std::static_pointer_cast<BarinadeSkeletonResource>(owner->replacement[info.skelPath]);
    Vec3s joints[64]{};
    joints[0].x = 10;
    GraphicsContext c; PlayState p{{&c, 60}, 8, {}};
    current = {};
    SkelAnime_DrawFlexOpa(&p, flex->h.sh.segment, joints, 63, nullptr, nullptr, nullptr);
    assert(c.op != c.opa && c.xp == c.xlu);
  }
  // Two models in one frame must rebind their own palettes, even after another
  // model or scene left a stale translucent matrix segment behind.
  {
    owner->alt = true;
    GraphicsContext c; PlayState p{{&c, 61}, 8, {}};
    current = {};
    assert(ComboForeignAnim_Draw(&info, "oot", &p));
    const uintptr_t first = checkBarinadeXluMatrices(c, nullptr, nullptr, 0xDEADBEEF);
    Gfx* opaBegin = c.op;
    Gfx* xluBegin = c.xp;
    current = {};
    assert(ComboForeignAnim_Draw(&info, "oot", &p));
    const uintptr_t second = checkBarinadeXluMatrices(c, opaBegin, xluBegin, first);
    assert(first != second && matrices.empty());
  }
  // Compatible selected custom rigs keep their native electricity, ring and limb motion.
  barinadeCustom = true;
  CwItemAnimDrawInfo custom{};
  assert(OOT_FillBossSoulAnim(2, &custom));
  custom.flameDlPath = nullptr;
  {
    GraphicsContext c; PlayState p{{&c, 69}, 9, {}};
    current = {}; scrollRequests.clear();
    assert(ComboForeignAnim_Draw(&custom, "oot", &p));
    int meshes = 0;
    for (Gfx* cmd = c.opa; cmd < c.op; ++cmd) meshes += cmd->kind == 3;
    assert(meshes == 53 && scrollRequests.size() == 4);
    int effects = 0;
    for (Gfx* cmd = c.xlu; cmd < c.xp; ++cmd) effects += cmd->kind == 3;
    assert(effects == 38);
    GraphicsContext native;
    PlayState n{{&native, 69}, 9, {}};
    Gfx* dl = (Gfx*)"__OTR__limb"; Vec3f pos{}; Vec3s rot{13, 14, 15};
    const Mtx before = current;
    OverrideLimbDrawBarinade(&n, 10, &dl, &pos, &rot, nullptr);
    PostLimbDrawBarinade(&n, 25, &dl, &rot, nullptr);
    assert(!dl && rot.x == 13 - 0x4000 && current.sx == before.sx && current.rx == before.rx);
    assert(native.xp != native.xlu);
  }
  barinadeCustom = false;
  info.flameDlPath = flamePath;
  {
    GraphicsContext c; PlayState p{{&c, 70}, 10, {}};
    current = {};
    assert(ComboForeignAnim_Draw(&info, "oot", &p));
    int draws = 0;
    for (Gfx* cmd = c.xlu; cmd < c.xp; ++cmd) if (cmd->kind == 3) {
      if (draws == 0) {
#ifdef HOST_MM
        assert(!strcmp((const char*)cmd->target, gameplay_keep_DL_01ACF0));
#else
        assert(!strcmp((const char*)cmd->target, "__OTR__@oot:objects/object_gi_fire/gGiBlueFireFlameDL"));
#endif
      }
      draws++;
    }
    assert(draws == 39 && matrices.empty());
  }
  info.flameDlPath = nullptr;
  for (int failure = 0; failure < 4; ++failure) {
    auto bad = info;
    if (failure == 0) bad.proceduralDlPaths[0] = nullptr;
    if (failure == 1) bad.proceduralProfile = 99;
    if (failure == 2) bad.limbCount = 63;
    if (failure == 3) bad.segCount = CW_ANIM_MAX_SEGS;
    GraphicsContext c; PlayState p{{&c, 4}, 9, {}};
    assert(!ComboForeignAnim_Draw(&bad, "oot", &p) && c.op == c.opa && c.xp == c.xlu);
  }
  CfaClearCaches();
  auto incompatible = std::make_shared<BarinadeSkeletonResource>(true);
  incompatible->h.sh.limbCount = 62;
  owner->replacement[info.skelPath] = incompatible;
  {
    GraphicsContext c; PlayState p{{&c, 71}, 11, {}};
    assert(!ComboForeignAnim_Draw(&info, "oot", &p) && c.op == c.opa && c.xp == c.xlu);
  }
  CfaClearCaches();
  std::cout << "Barinade production recipe/native callbacks: frozen pose, all limb surgery, XLU postdraw, normal/flex Alt matrices and cleanup passed\n";
}
#endif
// Replay the recorded native draw stream after all matrices have been written,
// as the renderer does. The jaw samples root matrix 0 plus its own matrix 1
// through segment 13.
void checkDraw(GraphicsContext &c, bool flex, const char *game) {
  uintptr_t seg13 = 0;
  Mtx *model = nullptr;
  int meshes = 0, bindings = 0;
  bool jaw = false;
  for (Gfx *p = c.opa; p < c.op; p++) {
    if (p->kind == 1 && p->seg == 13) {
      seg13 = p->target;
      bindings++;
    }
    if (p->kind == 2)
      model = (Mtx *)p->target;
    if (p->kind == 3) {
      meshes++;
      const char *path = (const char *)p->target;
      assert(
          std::string(path).starts_with(std::string("__OTR__@") + game + ":"));
      if (std::string(path).ends_with(":jaw")) {
        jaw = true;
        assert(model && model->x == 13);
        if (flex) {
          assert(seg13);
          auto m = (Mtx *)seg13;
          assert(m[0].x == 10 && m[1].x == 13);
        }
      }
    }
  }
  assert(meshes == 2 && jaw);
  assert(bindings == (flex ? 2 : 0));
  assert(matrices.empty());
}
// Replay with deliberately transparent inherited shelf state. A replacement flame
// that omits local prim/env commands must still receive an opaque neutral base,
// its boss tint, the owner RM and a valid scroll without perturbing the jaw.
void checkFlame(GraphicsContext& c, const char* game) {
  uintptr_t prim = 0, env = 0, tint = 0, segment8 = 0;
  bool grayscale = false, saw = false;
  std::vector<std::string> owners;
  for (Gfx* p = c.xlu; p < c.xp; ++p) {
    if (p->kind == 1 && p->seg == 8) segment8 = p->target;
    if (p->kind == 4) prim = p->target;
    if (p->kind == 5) env = p->target;
    if (p->kind == 6) tint = p->target;
    if (p->kind == 7) grayscale = p->target;
    if (p->kind == 8) owners.emplace_back((const char*)p->target);
    if (p->kind == 9) { assert(!owners.empty()); owners.pop_back(); }
    if (p->kind == 3) {
      assert(std::string((const char*)p->target) == std::string("__OTR__@") + game + ":flame");
      assert(prim == 0xffffffff && env == 0xffffffff);
      assert(grayscale && tint == 0xed5f5fff && segment8);
      assert(!owners.empty() && owners.back() == game);
      saw = true;
    }
  }
  assert(saw && !grayscale && owners.empty());
}
int main() {
  // A resource reload may reuse the same address for a different path. Owner
  // routes must key the path contents while keeping earlier submitted strings stable.
  sCfaCurrentGame = "oot";
  char reused[64] = "__OTR__reload_first";
  const char* first = (const char*)CfaRouteLimbDList((Gfx*)reused);
  std::strcpy(reused, "__OTR__reload_second");
  const char* second = (const char*)CfaRouteLimbDList((Gfx*)reused);
  assert(std::string(first) == "__OTR__@oot:reload_first");
  assert(std::string(second) == "__OTR__@oot:reload_second");
  auto oot = std::make_shared<Ship::ResourceManager>(),
       mm = std::make_shared<Ship::ResourceManager>();
  Ship::CrossRMRegistry::owners = {{"oot", oot}, {"mm", mm}};
  Ship::Context::GetRawInstance()->rm = mm;
#ifndef SKIP_BARINADE_TESTS
  checkBarinade(oot, mm);
#endif
  #ifndef SKIP_TWINMOLD_TESTS
#ifdef HOST_MM
  auto twinmoldHost = mm;
#else
  auto twinmoldHost = oot;
#endif
  Ship::Context::GetRawInstance()->rm = twinmoldHost;
  checkTwinmold(mm, twinmoldHost);
#ifdef HOST_MM
  checkNativeMmBosses(mm);
#endif
  Ship::Context::GetRawInstance()->rm = mm;
  current = {};
  #endif
  auto anim = std::make_shared<AnimationResource>();
  for (auto owner : {oot, mm}) {
    owner->normal = {{"__OTR__skel", std::make_shared<SkeletonResource>(0)},
                     {"__OTR__anim", anim}};
    owner->replacement = {
        {"__OTR__skel", std::make_shared<SkeletonResource>(1)},
        {"__OTR__anim", anim}};
  }
  CwItemAnimDrawInfo info{};
  info.opa = 1;
  info.skelPath = "__OTR__skel";
  info.animPath = "__OTR__anim";
  info.limbCount = 3;
  info.hiddenLimb = -1;
  info.nonFlexSkeleton = 1;
  // Start directly on the replacement: this isolates loaded-type dispatch from
  // stale caching.
  oot->alt = true;
  {
    GraphicsContext c;
    PlayState p{{&c, 3}, 3, {}};
    assert(ComboForeignAnim_Draw(&info, "oot", &p));
    checkDraw(c, true, "oot");
  }
  CfaClearCaches();
  normalInits = flexInits = updates = 0;
  for (const char *game : {"oot", "mm"})
    for (bool alt : {false, true, false, true}) {
      auto owner = Ship::CrossRMRegistry::Get(game);
      owner->alt = alt;
      GraphicsContext c;
      PlayState play{{&c, 4}, 4, {}};
      current = {};
      assert(ComboForeignAnim_Draw(&info, game, &play) == 1);
      checkDraw(c, alt, game);
      assert(Ship::Context::GetRawInstance()->rm == mm);
    }
  assert(normalInits == 2 && flexInits == 2 &&
         updates == 4); // owners and selections cache independently
  // Full composite path with the flame present, before the model, in both
  // directions and repeatedly toggled between rigid vanilla and flex Alt.
  info.flameDlPath = "__OTR__flame";
  info.flameGrayscale = 1;
  info.flameColor[0] = 237; info.flameColor[1] = 95; info.flameColor[2] = 95;
  info.flameHasSeg = 1;
  info.flameSeg.kind = CW_ANIM_SEG_TEXSCROLL;
  info.flameSeg.segment = 8; info.flameSeg.onXlu = 1;
  info.flameSeg.width1 = info.flameSeg.width2 = 16;
  info.flameSeg.height1 = info.flameSeg.height2 = 32;
  info.flameSeg.xStep2 = 1; info.flameSeg.yStep2 = -8;
  for (const char* game : {"oot", "mm"}) {
    for (bool alt : {false, true, false}) {
      Ship::CrossRMRegistry::Get(game)->alt = alt;
      GraphicsContext c;
      PlayState p{{&c, 7}, 7, {}};
      current = {};
      assert(ComboForeignAnim_Draw(&info, game, &p));
      checkDraw(c, alt, game);
      checkFlame(c, game);
    }
  }
  info.flameDlPath = nullptr;
#ifdef HOST_MM
  // The actual native MM soul renderer surrounds the owner-routed OoT model.
  // Repeated OoT Alt switches must change the jaw dispatch, never effect ownership.
  info.flameDlPath = "__OTR__objects/object_gi_fire/gGiBlueFireFlameDL";
  info.flameTranslate[1] = -70.0f;
  info.flameScale[0] = info.flameScale[1] = info.flameScale[2] = 5.0f;
  for (bool alt : {false, true, false}) {
    oot->alt = alt;
    GraphicsContext c;
    PlayState p{{&c, 8}, 8, {}};
    current = {};
    assert(ComboForeignAnim_Draw(&info, "oot", &p));
    checkDraw(c, alt, "oot");
    std::vector<std::string> owners;
    uintptr_t prim = 0, env = 0, seg8 = 0;
    bool grayscale = true, sawNative = false;
    Mtx* flameMatrix = nullptr;
    for (Gfx* cmd = c.xlu; cmd < c.xp; ++cmd) {
      if (cmd->kind == 8) owners.emplace_back((const char*)cmd->target);
      if (cmd->kind == 9) { assert(!owners.empty()); owners.pop_back(); }
      if (cmd->kind == 4) prim = cmd->target;
      if (cmd->kind == 5) env = cmd->target;
      if (cmd->kind == 7) grayscale = cmd->target;
      if (cmd->kind == 1 && cmd->seg == 8) seg8 = cmd->target;
      if (cmd->kind == 2) flameMatrix = (Mtx*)cmd->target;
      if (cmd->kind == 3) {
        assert(std::string((const char*)cmd->target) == gameplay_keep_DL_01ACF0);
        assert(owners.back() == "mm" && seg8 && !grayscale);
        assert(prim == 0xed5f5f00 && env == 0xed5f5f00);
        assert(flameMatrix && fabs(flameMatrix->sx * 400 - 60) < .001f);
        assert(fabs(flameMatrix->y - flameMatrix->sy * 480 + 60) < .001f);
        assert(fabs(flameMatrix->y + flameMatrix->sy * 1440 - 140) < .001f);
        assert(fabs(flameMatrix->sz - .15f) < .001f);
        sawNative = true;
      }
    }
    assert(sawNative && owners.empty() && !grayscale);
    assert(prim == 0xffffffff && env == 0xffffffff);
    assert(((Gfx*)seg8)[0].kind == 0 && matrices.empty());
  }
  info.flameDlPath = nullptr;
#endif
  // Recipe flag intentionally disagrees in the other direction too: loaded
  // normal wins.
  CfaClearCaches();
  info.nonFlexSkeleton = 0;
  oot->alt = false;
  {
    GraphicsContext c;
    PlayState p{{&c, 5}, 5, {}};
    assert(ComboForeignAnim_Draw(&info, "oot", &p));
    checkDraw(c, false, "oot");
  }
  // Unsupported curve, mismatched limb count, absent owner and missing
  // animation emit nothing.
  for (int failure = 0; failure < 4; failure++) {
    CfaClearCaches();
    auto bad = std::make_shared<SkeletonResource>(failure == 0 ? 2 : 0);
    oot->normal["__OTR__skel"] = bad;
    info.limbCount = failure == 1 ? 4 : 3;
    oot->normal["__OTR__anim"] = failure == 3 ? nullptr : anim;
    GraphicsContext c;
    PlayState p{{&c, 6}, 6, {}};
    assert(!ComboForeignAnim_Draw(&info, failure == 2 ? "absent" : "oot", &p));
    assert(c.op == c.opa && c.xp == c.xlu);
  }
  CfaClearCaches();
  std::cout << "Foreign soul loaded-type dispatch, native flex matrix/jaw "
               "replay, Alt/cache and fallback checks passed\n";
}
